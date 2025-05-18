#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <mysql/mysql.h>

#define MAX_BUFFER 8192
#define PORT 5566
#define MAX_PASSENGERS 10
#define MAX_NAME_LEN 64

MYSQL *conn;  

// Initialize MySQL connection
void initializeDatabase() {
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "mysqldb", "Shivam.1257", 
                           "Train", 3306, NULL, 0)) {
        fprintf(stderr, "Database error: %s\n", mysql_error(conn));
        exit(1);
    }
}

// Utility: send string response
void sendResponse(int sock, const char *response) {
    send(sock, response, strlen(response), 0);
}

// Handler: REGISTER|fname|lname|email|password|mobile|gender
void handleRegistration(int sock, char *data) {
    char *fname = strtok(data, "|");
    char *lname = strtok(NULL, "|");
    char *email = strtok(NULL, "|");
    char *password = strtok(NULL, "|");
    char *mobile = strtok(NULL, "|");
    char *gender = strtok(NULL, "|");
    char *dateofbirth = strtok(NULL, "|") ;

    char esc_fname[100], esc_lname[100], esc_email[200], esc_password[100], esc_mobile[30],esc_dateofbirth[30];
    mysql_real_escape_string(conn, esc_fname, fname, strlen(fname));
    mysql_real_escape_string(conn, esc_lname, lname, strlen(lname));
    mysql_real_escape_string(conn, esc_email, email, strlen(email));
    mysql_real_escape_string(conn, esc_password, password, strlen(password));
    mysql_real_escape_string(conn, esc_mobile, mobile, strlen(mobile));
    mysql_real_escape_string(conn, esc_dateofbirth, dateofbirth, strlen(dateofbirth));

    // Check if email or mobile exists
    char query[MAX_BUFFER];
    snprintf(query, sizeof(query),
        "SELECT user_id FROM user_details WHERE emailid = '%s' OR mobile_number = '%s'", 
        esc_email, esc_mobile);
    MYSQL_RES *cr ; 
    if (mysql_query(conn, query) || !(cr = mysql_store_result(conn))) {
        sendResponse(sock, "ERROR|Database error during registration\n");
        return;
    }
    if (cr && (mysql_num_rows(cr) > 0)) {
        sendResponse(sock, "ERROR|Email or mobile already exists\n");
        mysql_free_result(cr);
        return;
    }
    // Insert user
    snprintf(query, sizeof(query),
        "INSERT INTO user_details (fname,lname,emailid,password,mobile_number,gender,dob)"
        "VALUES ('%s','%s','%s','%s','%s','%c','%s')",
        esc_fname, esc_lname, esc_email, esc_password, esc_mobile, gender[0],esc_dateofbirth);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
        sendResponse(sock, "ERROR|Registration failed\n");
        return;
    }
    // Fetch and send user details
    snprintf(query, sizeof(query),
        "SELECT user_id,fname,lname,emailid,mobile_number,gender,dob "
        "FROM user_details WHERE emailid='%s'", esc_email);
    if (mysql_query(conn, query)) {
            fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            return;
        }
    MYSQL_RES *ur = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(ur);
    if (row) {
        char buf[MAX_BUFFER];
        snprintf(buf, sizeof(buf),
            "SUCCESS|%s|%s|%s|%s|%s|%s|%s\n",
            row[0], row[1], row[2], row[3], row[4], row[5], row[6]);
        sendResponse(sock, buf);
    } else {
        sendResponse(sock, "ERROR|User not found\n");
    }
    mysql_free_result(ur);
}

// Handler: LOGIN|email|password
void handleLogin(int sock, char *data) {
    char *email = strtok(data, "|");
    char *password = strtok(NULL, "|");
    char esc_email[200], esc_password[100];
    mysql_real_escape_string(conn, esc_email, email, strlen(email));
    mysql_real_escape_string(conn, esc_password, password, strlen(password));

    char query[MAX_BUFFER];
    snprintf(query, sizeof(query),
        "SELECT user_id,fname,lname,emailid,mobile_number,gender,dob "
        "FROM user_details WHERE emailid = '%s' AND password = '%s'",
        esc_email, esc_password);
    if (mysql_query(conn, query)) {
        sendResponse(sock, "ERROR|Login failed\n");
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        char buf[MAX_BUFFER];
        snprintf(buf, sizeof(buf),
            "SUCCESS|%s|%s|%s|%s|%s|%s|%s\n",
            row[0], row[1], row[2], row[3], row[4], row[5], row[6]);
        sendResponse(sock, buf);
    } else {
        sendResponse(sock, "ERROR|Invalid credentials\n");
    }
    mysql_free_result(res);
}

// Struct for passenger info
typedef struct {
    char name[MAX_NAME_LEN];
    int age;
    char seat_no[64];
} Passenger;

// Booking arguments
typedef struct {
    int client_sock;
    int user_id;
    char train_no[32];
    char class_name[16];
    int class_fare ;
    char date[16];
    char from[64], to[64];
    int passenger_count;
    int requested_age_ge_60;
    int requested_age_lt_60; 
    Passenger pax[MAX_PASSENGERS]; 
} BookingArgs;

// Booking thread
void *processBooking(void *arg) {BookingArgs *ba = (BookingArgs*)arg;
    printf("\nInside Process booking");
    fflush(stdout);
    char semName[256], query[MAX_BUFFER], line[MAX_BUFFER];
    sem_t *sems[MAX_PASSENGERS] = {0};
    char *lockedSeats[MAX_PASSENGERS] = {0};
    int lockedCoach[MAX_PASSENGERS] = {0};
    int lockedCount = 0;

    // 1) compute total available seats across all coaches
    int totalAvail = 0;
    snprintf(query, sizeof(query),
        "SELECT SUM(available_seats) FROM classseats "
        "WHERE train_number='%s' AND journey_date='%s' "
        "AND start_point='%s' AND destination_point='%s' "
        "AND class_name='%s'",
        ba->train_no, ba->date, ba->from, ba->to, ba->class_name);
    printf("Executing query: %s\n", query);
        fflush(stdout) ;
        if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            } 
    MYSQL_RES *tres = mysql_store_result(conn);
    MYSQL_ROW Trow = mysql_fetch_row(tres);
    if (Trow && Trow[0]) totalAvail = atoi(Trow[0]);
    mysql_free_result(tres);

    int toLock = (ba->passenger_count <= totalAvail) ? ba->passenger_count : totalAvail;
    // assume you’ve parsed from the client into these two ints:
    int reqLower = ba->requested_age_ge_60;  // number of seniors
    int reqUpper = ba->requested_age_lt_60;  // number of others

    // 1) figure out how many lower seats remain overall
    int totalLower = 0;
    snprintf(query, sizeof(query),
        "SELECT SUM(lower_seats) FROM classseats "
        " WHERE train_number='%s' AND journey_date='%s' "
        "   AND start_point='%s' AND destination_point='%s' "
        "   AND class_name='%s';",
        ba->train_no, ba->date, ba->from, ba->to, ba->class_name);
    printf("Executing query: %s\n", query);
        fflush(stdout) ;
        if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            } 
    {
    MYSQL_RES *r = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(r);
    if (row && row[0]) totalLower = atoi(row[0]);
    mysql_free_result(r);
    }
    int totalUpper = totalAvail - totalLower; 
    // 3) scan all coaches/columns in seat‐order, but only lock seats that you still “need”:
    //     - a lower‐berth (col ends in 'L') only if reqLower>0
    //     - an upper‐berth (col ends in 'U') only if reqUpper>0
    //     - otherwise skip until your remaining needed seats drop to zero
    for (int coach = 1; coach <= 4 && lockedCount < toLock; ++coach) {
      // fetch the 20 SeatNo columns
      snprintf(query, sizeof(query),
          "SELECT SeatNo1,SeatNo2,SeatNo3,SeatNo4,SeatNo5,"  
            "SeatNo6,SeatNo7,SeatNo8,SeatNo9,SeatNo10," 
            "SeatNo11,SeatNo12,SeatNo13,SeatNo14,SeatNo15," 
            "SeatNo16,SeatNo17,SeatNo18,SeatNo19,SeatNo20 "
            "FROM classseats "
            "WHERE train_number='%s' AND journey_date='%s' "
            "AND start_point='%s' AND destination_point='%s' "
            "AND class_name='%s' AND coach_number=%d",
            ba->train_no, ba->date, ba->from, ba->to,
            ba->class_name, coach);
      printf("Executing query: %s\n", query);
      fflush(stdout) ;
        if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            } 
      MYSQL_RES *cres = mysql_store_result(conn);
      MYSQL_ROW crow  = mysql_fetch_row(cres);
      if (!crow) { mysql_free_result(cres); continue; }

      // crow[0]…crow[19] are the seats, crow[20] is lower_seats for this coach
      for (int col = 0; col < 20 && lockedCount < toLock; ++col) {
      const char *seat = crow[col];
      if (strcmp(seat, "X")==0) 
      continue;  // already booked
    // 2) decide strategy: if totalLower == 0, fall back to “first‐come” locking
    //bool enforcePreference = (totalLower > reqLower);

    char last = seat[strlen(seat)-1];  // 'L' or 'U'
    bool wantThis = false;
    if(last == 'L' && (reqLower > 0 ) ) {wantThis = true;  reqLower--; totalLower--; }
    if(last == 'U' && (reqUpper > 0 ) ) {wantThis = true;  reqUpper--; totalUpper--; }
    if(last == 'U' && (reqLower > 0 && totalLower < reqLower) ) {wantThis = true;  reqLower--; totalUpper--; }
    if(last == 'L' && (reqUpper > 0 && totalUpper < reqUpper) ) {wantThis = true;  reqUpper--; totalLower--; }

    if (!wantThis) 
      continue;

    // attempt semaphore
    snprintf(semName, sizeof(semName),
      "/%s_%s_%s_%s_coach%d_seat%s",
      ba->train_no, ba->date, ba->from, ba->class_name,
      coach, seat);
    sems[lockedCount] = sem_open(semName, O_CREAT, 0644, 1);
    if (sems[lockedCount] && sem_trywait(sems[lockedCount])==0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s%d-%s", ba->class_name, coach, seat);
        lockedSeats[lockedCount] = strdup(buf);
        //use strdup in linux and _strdup in windows
        lockedCoach[lockedCount] = coach;
        lockedCount++;
        } else if (sems[lockedCount]) {
        sem_close(sems[lockedCount]);
        }
    }
    mysql_free_result(cres);
    }

    ///////////////Payment 
    double baseFare = ba->class_fare * ba->passenger_count;
    double discount = 0;
    if (ba->passenger_count >= 5)           discount = 0.20;
    else if (ba->passenger_count >= 3)      discount = 0.10;
    double payable = baseFare * (1 - discount);

    // 4) Fetch current available seats (sum of available_seats)
    int avail = 0;
    snprintf(query, sizeof(query),
    "SELECT SUM(available_seats) FROM classseats "
    " WHERE train_number='%s' AND journey_date='%s' "
    "   AND start_point='%s' AND destination_point='%s' "
    "   AND class_name='%s';",
    ba->train_no, ba->date, ba->from, ba->to, ba->class_name);
    printf("Executing query: %s\n", query);
        fflush(stdout) ;
        if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            } 
    {
    MYSQL_RES *r = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(r);
    if (row && row[0]) avail = atoi(row[0]);
    mysql_free_result(r);
    }

    // 5) Send payment request
    char txn[32];
    snprintf(txn, sizeof(txn), "%ld", time(NULL));
    
    char payReq[256];
    snprintf(payReq,sizeof(payReq),
      "PAYMENT_REQUEST|%.2f|%d|%s\n", payable, avail, txn);
    sendResponse(ba->client_sock, payReq);

    snprintf(query, sizeof(query),
    "SELECT payment_amount, status, sock FROM transaction "
    "WHERE transaction_id = %s", txn);    

    // 5) Check transaction table
    bool ok = false;
    for(int i = 0 ; i < 25 ; i++){
        sleep(5);
        if(!mysql_query(conn, query)){
            printf("Query Executed: %s",query);
            fflush(stdout);
        }
        MYSQL_RES *tres = mysql_store_result(conn);
        MYSQL_ROW trow = mysql_fetch_row(tres);

        if (trow) {
            double paid = atof(trow[0]);
            if (paid == payable && strcmp(trow[1], "SUCCESS")==0) {
                ok = true;
                printf("status: %s, sock: %d ",trow[1],atoi(trow[2]));
                fflush(stdout);
                ba->client_sock = atoi(trow[2]);
                break; 
            }
            else if (paid == payable && strcmp(trow[1], "FAILED")==0) {
                ok = false;
                printf("status: %s, sock: %d ",trow[1],atoi(trow[2]));
                fflush(stdout);
                ba->client_sock = atoi(trow[2]);
                break; 
            }
        }
        mysql_free_result(tres);
    }
    if (!ok) {
        // release any held semaphores before aborting
        for (int i = 0; i < lockedCount; ++i) {
            sem_post(sems[i]);
            sem_close(sems[i]);
            free(lockedSeats[i]);
        }
        sendResponse(ba->client_sock, "TRANSACTION_FAILED|Cancelled");
        return NULL;
    }

    ///////////////

    int* arr = malloc(sizeof(int) * lockedCount);
    int left = 0, right = lockedCount - 1;

    for (int i = 0; i < lockedCount; ++i) {
        size_t len = strlen(lockedSeats[i]);
        if (len && lockedSeats[i][len-1] == 'L') {
            arr[left++] = i;
        } else {
            arr[right--] = i;
        }
    }
    free(arr);
    // Assign seats:
    for (int i = 0; i < ba->passenger_count; ++i) {
        if (i < lockedCount) {
            strcpy(ba->pax[i].seat_no, lockedSeats[arr[i]]);
        } else {
            strcpy(ba->pax[i].seat_no, "WL");
        }
    }
    int lowerCount[4] = {0};     // only lowers per coach

    // 4) update DB: mark locked seats X and adjust counts per coach efficiently
    int count[4] = {0};
    char setClause[4][256] = {"", "", "", ""};
    for (int i = 0 ; i < lockedCount ; i++) {
        char temp[64];
        // extract seat number after the dash
        char *dash = strchr(lockedSeats[i], '-');
        int seatNum = atoi(dash + 1) ;
        //const char *seatNum = dash+1 ;
        snprintf(temp, sizeof(temp), "SeatNo%d='X'", seatNum);
        int idx = lockedCoach[i] - 1;
        if (count[idx] > 0) strcat(setClause[idx], ", ");
        strcat(setClause[idx], temp);
        count[idx]++;

        size_t len = strlen(lockedSeats[i]);
        if (len > 0 && lockedSeats[i][len-1] == 'L') {
            lowerCount[idx]++;
        }
    }

    for (int i = 0 ; i < 4 ; i++) {
        if (count[i] > 0) {
            snprintf(query, sizeof(query),
                "UPDATE classseats SET available_seats=available_seats-%d, "
                "booked_seats=booked_seats+%d,%s,lower_seats = lower_seats -  %d "
                "WHERE train_number='%s' AND journey_date='%s' "
                "AND start_point='%s' AND destination_point='%s' "
                "AND class_name='%s' AND coach_number=%d",
                count[i], count[i], setClause[i],lowerCount[i],
                ba->train_no, ba->date, ba->from, ba->to,
                ba->class_name, i+1);
            mysql_query(conn, query);
        }
    }

    // Dynamic fare surge: one-time 20% hike when crossing 80% occupancy
    snprintf(query, sizeof(query),
    "UPDATE classseats SET ticket_fare1 = ROUND(base_fare * 1.2, 2) "
    "WHERE train_number='%s' AND journey_date='%s' "
    "AND start_point='%s' AND destination_point='%s' "
    "AND class_name='%s' "
    "AND ticket_fare1 = base_fare "
    "AND (booked_seats * 100) >= 80 * (booked_seats + available_seats)",
    ba->train_no, ba->date, ba->from, ba->to, ba->class_name);
    mysql_query(conn, query);
    // end dynamic fare surge

    // 5) release all locks
    for (int i=0; i<lockedCount; ++i) {
        sem_post(sems[i]);
        sem_close(sems[i]);
        free(lockedSeats[i]);
    }

    // 6) proceed with reservation insert & response

    // 3) Fetch train_name, dep/arr times
    char trainName[64], depTime[16], arrTime[16];
    int distFrom=0, distTo=0;

    // train_name
    snprintf(query, sizeof(query),
        "SELECT train_name FROM train_details "
        "WHERE train_number='%s'", ba->train_no);
    mysql_query(conn, query);
    MYSQL_RES *Tres = mysql_store_result(conn);
    MYSQL_ROW trow = mysql_fetch_row(Tres);
    if (trow) strncpy(trainName, trow[0], sizeof(trainName));
    mysql_free_result(Tres);

    // departure at boarding
    snprintf(query, sizeof(query),
        "SELECT departure_time, distance FROM train_schedule "
        "WHERE train_number='%s' AND station_name='%s'",
        ba->train_no, ba->from);
    mysql_query(conn, query);
    tres = mysql_store_result(conn);
    trow = mysql_fetch_row(tres);
    if (trow) {
        strncpy(depTime, trow[0], sizeof(depTime));
        distFrom = atoi(trow[1]);
    }
    mysql_free_result(tres);

    // arrival at destination
    snprintf(query, sizeof(query),
        "SELECT arrival_time, distance FROM train_schedule "
        "WHERE train_number='%s' AND station_name='%s'",
        ba->train_no, ba->to);
    mysql_query(conn, query);
    tres = mysql_store_result(conn);
    trow = mysql_fetch_row(tres);
    if (trow) {
        strncpy(arrTime, trow[0], sizeof(arrTime));
        distTo = atoi(trow[1]);
    }
    mysql_free_result(tres);
    
    int distance = distTo - distFrom;
    double totalFare = ba->passenger_count * /*fetch per‐ticket fare from classseats if needed*/ ba->class_fare;
    if(ba->passenger_count >= 5 ) totalFare = totalFare * 0.8 ;
    else if (ba->passenger_count >= 3 ) totalFare = totalFare * 0.9 ;

    // 4) Create entries in reservation & passenger_details
    char pnr[32];
    snprintf(pnr, sizeof(pnr), "%ld", time(NULL) + 100);
    snprintf(txn, sizeof(txn), "%ld", time(NULL));

    // reservation insert
    snprintf(query, sizeof(query),
        "INSERT INTO reservation "
        "(pnr_number, user_id, train_number, start_point, destination_point, journey_date, "
        " ticket_fare1, class_name, number_of_seats, ticket_status, quota, transaction_id, mode_of_payment) "
        "VALUES (%s, %d, %s, '%s', '%s', '%s', %.0f, '%s', %d, '%s', '%s','%s','ONLINE')",
        pnr, ba->user_id, ba->train_no,
        ba->from, ba->to, ba->date,
        totalFare, ba->class_name, ba->passenger_count,
        (lockedCount == ba->passenger_count) ? "CONFIRMED" : "Some or all WAITLISTED", "GENERAL",txn);
    mysql_query(conn, query);

    // passenger_details insert
    for (int i = 0; i < ba->passenger_count; ++i) {
        snprintf(query, sizeof(query),
            "INSERT INTO passenger_details "
            "(pnr_number, passenger_name, passenger_age, passenger_gender,ticket_coach, seat_number, berth) "
            "VALUES (%s, '%s', %d, '%c', '%s', '%s', '%s')",
            pnr,
            ba->pax[i].name,
            ba->pax[i].age,
            /* you’ll need to pass gender in BookingArgs */'M',
            ba->class_name,
            ba->pax[i].seat_no,
            /* simple berth logic: */ (ba->pax[i].age > 60 ? "LOWER" : "UPPER"));
            printf("Executing query: %s\n", query);
            fflush(stdout) ;
            if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            }            
    }

    // 5) Build and send response
    char response[MAX_BUFFER] = {0};
    snprintf(line, sizeof(line),
        "TICKET|%s|%s|%s|%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%s\n",
        ba->train_no,trainName,ba->date,
        ba->from,
        ba->to,
        depTime,
        arrTime,ba->class_name,(lockedCount == ba->passenger_count) ? "CONFIRMED" : "Some or all WAITLISTED",
        totalFare,
        txn,
        "ONLINE",
        pnr);
    strcat(response, line);

    for (int i = 0; i < ba->passenger_count; ++i) {
        snprintf(line, sizeof(line),
            "PASSENGER|%s|%s\n",
            ba->pax[i].name,
            ba->pax[i].seat_no);
        strcat(response, line);
    }

    sendResponse(ba->client_sock, response);
    free(ba);
    return NULL;
}

// Handler: BOOK_REQUEST|user|date|from|to|train|class|p1:a1;p2:a2;...
void handleBookingRequest(int sock, char *data) {
    BookingArgs *ba = (BookingArgs*)calloc(1, sizeof(BookingArgs));
    ba->client_sock = sock;
    ba->user_id = atoi(strtok(data, "|"));
    strcpy(ba->date, strtok(NULL, "|"));
    strcpy(ba->from, strtok(NULL, "|"));
    strcpy(ba->to, strtok(NULL, "|"));
    strcpy(ba->train_no, strtok(NULL, "|"));
    strcpy(ba->class_name, strtok(NULL, "|"));
    ba->class_fare = atoi(strtok(NULL,"|")) ;
    ba->requested_age_ge_60 = atoi(strtok(NULL,"|"));
    ba->requested_age_lt_60 = atoi(strtok(NULL,"|"));
    char *plist = strtok(NULL, "|");
    // Parse passengers
    char *p = plist ;int i = 0;
    while (p && ba->passenger_count < MAX_PASSENGERS) {
        char *name;
        if(i==0) {name = strtok(p, ":");i++;}
        else name = strtok(NULL,":") ;
        if(name == NULL) break;
        char *age = strtok(NULL, ";");
        strncpy(ba->pax[ba->passenger_count].name, name, MAX_NAME_LEN);
        ba->pax[ba->passenger_count].age = atoi(age);
        printf("\nName: %s Age: %d",ba->pax[ba->passenger_count].name,ba->pax[ba->passenger_count].age);
        fflush(stdout);
        ba->passenger_count++;
        //p = strtok(NULL, ";");
    }
    printf("\nTo Process booking");
    fflush(stdout);
    pthread_t tid;
    pthread_create(&tid, NULL, processBooking, ba);
    pthread_detach(tid);
}

// Handler: SHOW_TICKETS|user_id
void handleShowTickets(int sock, char *data) {
    int user_id = atoi(data);
    char query[MAX_BUFFER], response[MAX_BUFFER] = "";
    MYSQL_RES *res;
    MYSQL_ROW row;

    // Step 1: Get reservations for the user
    snprintf(query, sizeof(query),
    "SELECT r.pnr_number, r.train_number, td.train_name, r.journey_date, r.start_point, "
    "ts1.departure_time, ts2.arrival_time, r.destination_point, r.ticket_status, r.class_name, r.ticket_fare1, "
    "r.transaction_id, r.mode_of_payment "
    "FROM reservation r "
    "JOIN train_details td ON r.train_number = td.train_number "
    "JOIN train_schedule ts1 ON r.train_number = ts1.train_number AND ts1.station_name = r.start_point "
    "JOIN train_schedule ts2 ON r.train_number = ts2.train_number AND ts2.station_name = r.destination_point "
    "WHERE r.user_id = %d", user_id);

    if (mysql_query(conn, query)) {
        sendResponse(sock, "ERROR|Unable to fetch reservations\n");
        return;
    }

    res = mysql_store_result(conn);
    while ((row = mysql_fetch_row(res))) {
        char pnr[32], train_no[16], train_name[64], date[16], from[32], to[32], status[16];
        char class_name[16], fare_str[16], txn_id[20], payment_mode[20];
        char dept_time[16] , arr_time[16] ;
        strncpy(pnr, row[0], sizeof(pnr));
        strncpy(train_no, row[1], sizeof(train_no));
        strncpy(train_name, row[2], sizeof(train_name));
        strncpy(date, row[3], sizeof(date));
        strncpy(from, row[4], sizeof(from));
        strncpy(dept_time, row[5], sizeof(dept_time)) ;
        strncpy(arr_time, row[6], sizeof(arr_time)) ;
        strncpy(to, row[7], sizeof(to));
        strncpy(status, row[8], sizeof(status));
        strncpy(class_name, row[9], sizeof(class_name));
        snprintf(fare_str, sizeof(fare_str), "%s", row[10]);
        strncpy(txn_id, row[11], sizeof(txn_id));
        strncpy(payment_mode, row[12], sizeof(payment_mode));

        // Step 2: Add ticket header to response
        char line[MAX_BUFFER];
        snprintf(line, sizeof(line),
            "TICKET|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
            train_no, train_name, date, from, to, dept_time, arr_time,   
            class_name, status, fare_str, txn_id, payment_mode, pnr);
        strcat(response, line);

        // Step 3: Get passenger details for the PNR
        snprintf(query, sizeof(query),
            "SELECT passenger_name, seat_number, berth "
            "FROM passenger_details WHERE pnr_number = %s", pnr);
        if (mysql_query(conn, query)) continue;

        MYSQL_RES *pres = mysql_store_result(conn);
        MYSQL_ROW prow;
        while ((prow = mysql_fetch_row(pres))) {
            snprintf(line, sizeof(line),
                "PASSENGER|%s|%s|%s\n", prow[0], prow[1], prow[2]);
            strcat(response, line);
        }
        mysql_free_result(pres);
    }
    mysql_free_result(res);

    if (strlen(response) == 0) {
        strcpy(response, "INFO|No tickets found\n");
    }

    sendResponse(sock, response);
}

// Handler: CANCEL|pnr
void handleCancel(int sock, char *data) {
    char *pnr = strtok(data, "|");
    char query[MAX_BUFFER];

    // 1) Fetch reservation metadata
    char train_no[32], journey_date[16], start_point[64], destination_point[64], class_name[16];
    snprintf(query, sizeof(query),
        "SELECT train_number, journey_date, start_point, destination_point, class_name "
        "FROM reservation WHERE pnr_number='%s'", pnr);
    mysql_query(conn, query);
    MYSQL_RES *rres = mysql_store_result(conn);
    MYSQL_ROW rrow = mysql_fetch_row(rres);
    if (!rrow) {
        mysql_free_result(rres);
        sendResponse(sock, "ERROR|No confirmed reservation found");
        return;
    }

    strcpy(train_no, rrow[0]);
    strcpy(journey_date, rrow[1]);
    strcpy(start_point, rrow[2]);
    strcpy(destination_point, rrow[3]);
    strcpy(class_name, rrow[4]);
    mysql_free_result(rres);

    // 2) Check if cancellation time is before departure
    snprintf(query, sizeof(query),
        "SELECT departure_time FROM train_schedule WHERE train_number='%s' AND station_name='%s'",
        train_no, start_point);
    mysql_query(conn, query);
    MYSQL_RES *sres = mysql_store_result(conn);
    MYSQL_ROW srow = mysql_fetch_row(sres);
    if (!srow) {
        mysql_free_result(sres);
        sendResponse(sock, "ERROR|Schedule info not found");
        return;
    }

    // Compare current time with departure time
    char departure_time[16];
    strcpy(departure_time, srow[0]);
    mysql_free_result(sres);

    snprintf(query, sizeof(query),
        "SELECT NOW() < STR_TO_DATE(CONCAT('%s', ' ', '%s'), '%%Y-%%m-%%d %%H:%%i:%%s')",
        journey_date, departure_time);
    mysql_query(conn, query);
    MYSQL_RES *tres = mysql_store_result(conn);
    MYSQL_ROW trow = mysql_fetch_row(tres);
    if (!trow || strcmp(trow[0], "0") == 0) {
        mysql_free_result(tres);
        sendResponse(sock, "ERROR|Cannot cancel ticket at or after departure time");
        return;
    }
    mysql_free_result(tres);

    // 3) Restore seats
    snprintf(query, sizeof(query),
        "SELECT seat_number FROM passenger_details WHERE pnr_number='%s' AND seat_number!='WL'", pnr);
    mysql_query(conn, query);
    MYSQL_RES *pres = mysql_store_result(conn);
    MYSQL_ROW prow;
    while ((prow = mysql_fetch_row(pres))) {
        char *seat = prow[0];
        char *dash = strchr(seat, '-');
        if (!dash) continue;
        int coach = atoi(dash - 1);
        char *seatNo = dash + 1;
        int no = atoi(seatNo);
        size_t len = strlen(dash);

        snprintf(query, sizeof(query),
            "UPDATE classseats SET available_seats=available_seats+1, "
            "booked_seats=booked_seats-1, SeatNo%d='%s', "
            "lower_seats=lower_seats+%d "
            "WHERE train_number='%s' AND journey_date='%s' "
            "AND start_point='%s' AND destination_point='%s' "
            "AND class_name='%s' AND coach_number=%d",
            no, seatNo, dash[len - 1] == 'L' ? 1 : 0,
            train_no, journey_date, start_point, destination_point,
            class_name, coach);
        if (mysql_query(conn, query)) {
            printf("\nExecuted Query: %s", query);
            fflush(stdout);
        }
    }
    mysql_free_result(pres);

    // 4) Cancel reservation
    snprintf(query, sizeof(query),
        "UPDATE reservation SET ticket_status='CANCELLED' WHERE pnr_number='%s'", pnr);
    if (mysql_query(conn, query) == 0) {
        sendResponse(sock, "CANCELLED|\n");
    } else {
        sendResponse(sock, "ERROR|Cancel failed\n");
    }

    // 5) Fare adjustment logic
    snprintf(query, sizeof(query),
        "UPDATE classseats SET ticket_fare1 = base_fare "
        "WHERE train_number='%s' AND journey_date='%s' "
        "AND start_point='%s' AND destination_point='%s' "
        "AND class_name='%s' "
        "AND ticket_fare1 != base_fare "
        "AND (booked_seats * 100) < 80 * (booked_seats + available_seats)",
        train_no, journey_date, start_point, destination_point, class_name);
    mysql_query(conn, query);
}

void handleSeatMap(int sock, char *data) {
    // data: train|date|from|to|class
    char *train = strtok(data, "|");
    char *date  = strtok(NULL, "|");
    char *from  = strtok(NULL, "|");
    char *to    = strtok(NULL, "|");
    char *cls   = strtok(NULL, "|");
    char query[MAX_BUFFER], line[MAX_BUFFER];
    // build entire response in one C buffer
    static char fullResp[MAX_BUFFER * 6];
    int offset = 0;

    for (int coach = 1; coach <= 1/*4*/; ++coach) {
        // header for this coach
        offset += snprintf(fullResp + offset, sizeof(fullResp) - offset,
            "SEATMAP|%s|%s|%s|%s|%s|%d\n",
            train, date, from, to, cls, coach);

        // fetch seat columns
        snprintf(query, sizeof(query),
            "SELECT SeatNo1,SeatNo2,SeatNo3,SeatNo4,SeatNo5,"
            "SeatNo6,SeatNo7,SeatNo8,SeatNo9,SeatNo10,"
            "SeatNo11,SeatNo12,SeatNo13,SeatNo14,SeatNo15,"
            "SeatNo16,SeatNo17,SeatNo18,SeatNo19,SeatNo20 "
            "FROM classseats "
            "WHERE train_number='%s' AND journey_date='%s' "
            "AND start_point='%s' AND destination_point='%s' "
            "AND class_name='%s' AND coach_number='%d'",
            train, date, from, to, cls, coach);
        printf("Executing query: %s\n", query);
        fflush(stdout) ;
        if (mysql_query(conn, query)) {
                fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
            }   
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        for (int col = 0; col < 20; ++col) {
            const char *seatNo = row[col];
            char stat = (/*seatNo[0]=='X'*/strcmp(seatNo,"X") == 0 ) ? 'X' : 'A' ;
            offset += snprintf(fullResp + offset, sizeof(fullResp) - offset,
                "S|%d|%c\n", col, stat);
        }
        mysql_free_result(res);
        //offset += snprintf(fullResp + offset, sizeof(fullResp) - offset, "|END_SEATMAP");
    }
    // send once
    send(sock, fullResp, offset, 0);
}
void handlePayment(int sock, char *data) {
    // data == "<amount>|<txn_id>|<status>"
    char *amtStr  = strtok(data, "|");
    char *txnStr  = strtok(NULL, "|");
    char *statStr = strtok(NULL, "|");
    if (!amtStr||!txnStr||!statStr) {
        sendResponse(sock, "ERROR|Malformed PAYMENT\n");
        return;
    }
    // Insert into transaction table
    char query[MAX_BUFFER];
    snprintf(query, sizeof(query),
      "INSERT INTO transaction (transaction_id, payment_amount, status, sock) "
      "VALUES (%s, %s, '%s', %d)",
      txnStr, amtStr, statStr, sock);
      printf("Query: %s",query);
      fflush(stdout); 
    if (mysql_query(conn, query)) {
        sendResponse(sock, "ERROR|DB\n");
    }
}
// Handler: LIST_TRAINS|journey_date|from_station|to_station
void handleListTrains(int sock, char *data) {
    char *date = strtok(data, "|");
    char *from = strtok(NULL, "|");
    char *to = strtok(NULL, "|");

    char query[1024];
    char response[MAX_BUFFER] = "";

    // 1. Find trains going from 'from' to 'to'
    snprintf(query, sizeof(query),
        "SELECT DISTINCT ts1.train_number, td.train_name, ts1.departure_time, ts2.arrival_time, "
        "ts2.distance - ts1.distance AS distance "
        "FROM train_schedule ts1 "
        "JOIN train_schedule ts2 ON ts1.train_number = ts2.train_number "
        "JOIN train_details td ON ts1.train_number = td.train_number "
        "WHERE ts1.station_name = '%s' AND ts2.station_name = '%s' AND ts1.distance < ts2.distance",
        from, to);
    
    if (mysql_query(conn, query) != 0) {
        sendResponse(sock, "ERROR|Query failed\n");
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        char line[512];
        snprintf(line, sizeof(line), "TRAIN|%s|%s|%s|%s|%s\n",
                 row[0], row[1], row[2], row[3], row[4]);
        strcat(response, line);

        // Get coach/class info
        snprintf(query, sizeof(query),
            "SELECT class_name, coach_number, available_seats, ticket_fare1 "
            "FROM classseats WHERE train_number='%s' AND journey_date='%s' AND start_point='%s' AND destination_point='%s'",
            row[0], date, from, to);
        mysql_query(conn, query);
        MYSQL_RES *cr = mysql_store_result(conn);
        MYSQL_ROW crow;
        while ((crow = mysql_fetch_row(cr))) {
            snprintf(line, sizeof(line), "COACH|%s|%s|%s|%s\n",
                     crow[0], crow[1], crow[2], crow[3]);
            strcat(response, line);
        }
        mysql_free_result(cr);
    }
    mysql_free_result(res);

    strcat(response, "END_TRAINS\n");
    sendResponse(sock, response);
}
// Client dispatcher
void *clientHandler(void *arg) {
    int sock = (intptr_t)arg;
    char buffer[MAX_BUFFER];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        char *action = strtok(buffer, "|");
        char *data   = buffer + strlen(action) + 1;
        if (strcmp(action, "REGISTER")==0) handleRegistration(sock, data);
        else if (strcmp(action, "LOGIN")==0) handleLogin(sock, data);
        else if (strcmp(action, "LIST_TRAINS")==0) handleListTrains(sock, data);
        else if (strcmp(action, "BOOK_REQUEST")==0) handleBookingRequest(sock, data);
        else if (strcmp(action, "SHOW_TICKETS")==0) handleShowTickets(sock, data);
        else if (strcmp(action, "CANCEL")==0) handleCancel(sock, data);
        else if (strcmp(action, "GET_SEATMAP")==0) handleSeatMap(sock,data);
        else if (strcmp(action, "PAYMENT")==0) handlePayment(sock,data);
        else sendResponse(sock, "ERROR|Invalid action\n");
    }
    close(sock);
    return NULL;
}

int main() {
    initializeDatabase();
    int server_sock, client_sock;
    struct sockaddr_in server_addr;
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {perror("[-]Socket error");exit(1);}
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    //inet_pton(AF_INET, "192.168.29.125", &server_addr.sin_addr);
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("[-]Bind error");exit(1);}
    listen(server_sock, 20);
    printf("[+] Server listening on port %d\n", PORT);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        printf("[+] Client connected\n");
        pthread_t tid;
        pthread_create(&tid, NULL, clientHandler, (void*)(intptr_t)client_sock);
        pthread_detach(tid);
    }
    mysql_close(conn);
    return 0;
}
