// Updated: Removed coach index selection from client side; server decides allocation logic
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>
#include <iomanip>
#include <map>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>

using namespace std;

class User {
private:
    int user_id;
    string fname;
    string lname;
    string email;
    string mobile_number;
    char gender;
    string reg_date;

public:
    User(int id, string fn, string ln, string em, string mod, char g, string dt)
        : user_id(id), fname(fn), lname(ln), email(em), mobile_number(mod), gender(g), reg_date(dt) {}

    int getId() const { return user_id; }
    string getFullName() const { return fname + " " + lname; }
};

struct TicketDetails {
    string train_no, train_name;
    string journey_date,from, to;
    string dep_time, arr_time;
    string class_name, status ;
    string transaction_id, pnr_no, mode_of_payment;
    double ticket_price;
    vector<pair<string, string>> passengers; // name and seat info

    // void display() const {
    //     cout << "\n====== Ticket Details ======" << endl;
    //     cout << "Train: " << train_no << " - " << train_name << endl;
    //     cout << "Date of journey: " << journey_date <<endl;
    //     cout << "From: " << from << "  To: " << to << endl;
    //     cout << "Departure: " << dep_time << "  Arrival: " << arr_time << endl;
    //     cout << "Class: " << class_name << endl; 
    //     cout << "Transaction ID: " << transaction_id << endl;
    //     cout << "Mode of payment: " << mode_of_payment << endl;
    //     cout << "Status: " << status << endl ;
    //     cout << "Reservation ID(Pnr): " << pnr_no << endl;
    //     cout << "Total Fare: Rs. " << ticket_price << endl;
    //     cout << "Passengers: " << endl;
    //     for (const auto& p : passengers) {
    //         cout << "  " << p.first << " - Seat: " << p.second << endl;
    //     }
    //     cout << "============================\n";
    // }
    void display() const {
        cout << "Train Ticket Details\n";
        cout << "===============================================\n";
        cout << "Train Number           : " << train_no << endl;
        cout << "Train Name             : " << train_name << endl;
        cout << "Boarding Station       : " << from << endl;
        cout << "Destination Station    : " << to << endl;
        cout << "Departure from Boarding: " << dep_time << endl;
        cout << "Arrival Time to Destination: " << arr_time << endl;
        cout << "Date of Journey        : " << journey_date << endl;
        cout << "===============================================\n\n";

        cout << "Passenger Details:\n";
        cout << "===============================================\n";
        cout << left << setw(25) << "Passenger Name(s)"
            << setw(10) << "Seat No" << endl;

        for (size_t i = 0; i < passengers.size(); ++i) {
            cout << left << setw(25) << passengers[i].first
                << setw(10) << passengers[i].second << endl;
        }

        cout << "===============================================\n\n";

        cout << "Ticket Details:\n";
        cout << "===============================================\n";
        cout << "Ticket Price           : " << fixed << setprecision(2) << ticket_price << endl;
        cout << "Pnr                    : " << pnr_no << endl;
        cout << "Status                 : " << status << endl;
        cout << "Transaction ID         : " << transaction_id << endl;
        cout << "===============================================\n";
    }
};

vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) tokens.push_back(token);
    return tokens;
}

// CoachInfo and TrainInfo definitions remain unchanged
struct CoachInfo {
    string class_name;
    int avail_seats;
    double fare;
};

struct TrainInfo {
    string train_no;
    string name;
    string dep_time;
    string arr_time;
    int distance;
    vector<CoachInfo> coaches;
};

std::string getCurrentDateTime() {
    std::time_t now = std::time(nullptr);      // Get current time
    std::tm* localTime = std::localtime(&now); // Convert to local time

    std::ostringstream oss;
    oss << (localTime->tm_year + 1900) << "-" // Year (since 1900)
        << (localTime->tm_mon + 1) << "-"     // Month (0-based, so add 1)
        << localTime->tm_mday << " "
        << localTime->tm_hour << ":"
        << localTime->tm_min << ":"
        << localTime->tm_sec;

    return oss.str();
}

void sendRequestToServer(const string& request, string& response) {
    const char* ip = "127.0.0.1";
    const int port = 5566;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Socket error"); exit(1); }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection error"); close(sock); return;
    }
    send(sock, request.c_str(), request.size(), 0);
    char buffer[8192] = {0};
    recv(sock, buffer, sizeof(buffer), 0);
    response = string(buffer);
    close(sock);
}

TicketDetails parseTicketDetails(string& response) {
    TicketDetails ticket;
    vector<string> lines = split(response, '\n');
    for (const string& line : lines) {
        vector<string> parts = split(line, '|');
        if (parts.empty()) continue;
        if (parts[0] == "TICKET") {
            ticket.train_no = parts[1];
            ticket.train_name = parts[2];
            ticket.journey_date = parts[3];
            ticket.from = parts[4];
            ticket.to = parts[5];
            ticket.dep_time = parts[6];
            ticket.arr_time = parts[7];
            ticket.class_name = parts[8] ;
            ticket.status = parts[9] ;
            ticket.ticket_price = stod(parts[10]);
            ticket.transaction_id = parts[11];
            ticket.mode_of_payment = parts[12];
            ticket.pnr_no = parts[13];
        } else if (parts[0] == "PASSENGER") {
            ticket.passengers.emplace_back(parts[1], parts[2]);
        }
    }
    return ticket;
}

void displayHomeMenu() {
    cout << "\n===== Home Menu =====" << endl;
    cout << "1. Show My Tickets" << endl;
    cout << "2. Check Seat Availability" << endl;
    cout << "3. Book Ticket" << endl;
    cout << "4. Cancel Ticket" << endl;
    cout << "5. Logout" << endl;
    cout << "Enter choice: ";
}

void handleShowTickets(const User* user) {
    string req = "SHOW_TICKETS|" + to_string(user->getId());
    string resp;
    sendRequestToServer(req, resp);

    // split response into individual ticket blocks
    vector<string> blocks;
    size_t start = 0;
    while (true) {
        size_t pos = resp.find("TICKET|", start + 1);
        if (pos == string::npos) {
            blocks.push_back(resp.substr(start));
            break;
        }
        blocks.push_back(resp.substr(start, pos - start));
        start = pos;
    }

    // parse & display each ticket
    bool any = false;
    for (auto &blk : blocks) {
        if (blk.find("TICKET|") != string::npos) {
            TicketDetails ticket = parseTicketDetails(blk);
            ticket.display();
            any = true;
        }
    }
    if (!any) {
        cout << "You have no tickets.";
    }
}

void showAvailableTrains(const vector<TrainInfo>& trains) {
    cout << "\n===== Available Trains =====" << endl;
    for (size_t i = 0; i < trains.size(); ++i) {
        const auto &t = trains[i];
        cout << i+1 << ") Train " << t.train_no << " - " << t.name
             << " (Dep: " << t.dep_time << ", Arr: " << t.arr_time
             << ", Dist: " << t.distance << "km)" << endl;
        for (const auto &c : t.coaches) {
            cout << "     Class: " << c.class_name
                 << ", Seats: " << c.avail_seats
                 << ", Fare: " << c.fare << endl;
        }
    }
}
void handleCheckAvailability() {
    // string date, from, to;
    // cout << "Date (YYYY-MM-DD): "; cin >> date;
    // cout << "Boarding Station: "; cin >> from;
    // cout << "Destination Station: "; cin >> to;
    // string req = "LIST_TRAINS|" + date + "|" + from + "|" + to;
    // string resp;
    // sendRequestToServer(req, resp);
    // cout << "\nAvailable Trains and Coaches:\n" << resp << endl;
    string date, from, to;
    cout << "Date (YYYY-MM-DD): "; cin >> date;
    cout << "Boarding Station: "; cin >> from;
    cout << "Destination Station: "; cin >> to;

    // 1) List trains as before...
    string listReq = "LIST_TRAINS|" + date + "|" + from + "|" + to;
    string listResp;
    sendRequestToServer(listReq, listResp);
    vector<TrainInfo> trains;
    istringstream respStream(listResp);
    string line;
    TrainInfo current;
    while (getline(respStream, line)) {
        if (line.empty()) continue;
        auto parts = split(line, '|');
        if (parts[0] == "TRAIN") {
            current = TrainInfo();
            current.train_no = parts[1];
            current.name     = parts[2];
            current.dep_time = parts[3];
            current.arr_time = parts[4];
            current.distance = stoi(parts[5]);
            trains.push_back(current);
        } else if (parts[0] == "COACH") {
            CoachInfo ci;
            ci.class_name  = parts[1];
            ci.avail_seats = stoi(parts[3]);
            ci.fare        = stod(parts[4]);
            trains.back().coaches.push_back(ci);
        } else if (parts[0] == "END_TRAINS") {
            break;
        }
    }

    if (trains.empty()) {
        cout << "No trains available for this route/date.\n";
        return;
    }
    
    // Display all train details
    showAvailableTrains(trains);

    // 2) User selects train and class
    string trainNo, className;
    cout << "Enter train number: "; cin >> trainNo;
    cout << "Enter class: ";      cin >> className;

    // 3) Request seat map
    ostringstream ss;
    ss << "GET_SEATMAP|" << trainNo << "|" << date
       << "|" << from << "|" << to << "|" << className;
    string resp;
    cout << "Availability request: \n" << ss.str() << endl ;
    sendRequestToServer(ss.str(), resp);
    cout << "Availabillity response: \n" << resp << endl ;
    //4) Parse into coach->vector(seatNo,status)
    istringstream in(resp);
    map<int, vector<pair<int,char>>> coachSeats;
    int coach = 0;
    while (getline(in, line)) {
        auto parts = split(line, '|');
        if (parts[0] == "SEATMAP") {
            coach = stoi(parts[6]);
            coachSeats[coach].clear();
        } else if (parts[0] == "S") {
            coachSeats[coach].emplace_back(stoi(parts[1]), parts[2][0]);
        } //else if (parts[0] == "END_SEATMAP") continue ;
    }

    // 5) Display seat layout for each coach
    cout << "Train Details " << endl;
    cout << "================================================" << endl;
    cout << "Train: " << trainNo << " - " << current.name << endl;
    cout << "Date of journey: " << date <<endl;
    cout << "From: " << from << "  To: " << to << endl;
    cout << "Departure: " << current.dep_time << "  Arrival: " << current.arr_time << endl;
    cout << "Distance: " << current.distance << " km" << endl;
    cout << "Class: " << className << endl;
    cout << "Available Seats as on " << getCurrentDateTime() << endl ;  
    for (int c = 1; c <= 1/*4*/; ++c) {
        auto &seats = coachSeats[c];
        cout << "\nClass: " << className << "  CoachNo: " << c << "\n";
        cout << string(32, '-') << "\n\n";
        cout << string(14, '-') << "    " << string(14, '-') << "\n\n";
        for (int row = 0; row < 5; ++row) {
            // left 2 seats
            for (int col = 0; col < 2; ++col) {
                int idx = row*4 + col;
                if (idx < (int)seats.size()) {
                    std::pair<int, char> seat = seats[idx];
                    int num = seat.first + 1;
                    char st = seat.second;
                    cout << " [" << setw(3) << (st == 'X' ? std::string("X") : std::to_string(num)) << "] ";
                } else cout << "     ";
            }
            cout << "    ";
            // right 2 seats
            for (int col = 2; col < 4; ++col) {
                int idx = row*4 + col;
                if (idx < (int)seats.size()) {
                    std::pair<int, char> seat = seats[idx];
                    int num = seat.first + 1;
                    char st = seat.second;
                    cout << " [" << setw(3) << (st == 'X' ? std::string("X") : std::to_string(num)) << "] ";
                } else cout << "     ";
            }
            cout << "\n\n";
        }
        cout << string(14, '-') << "    " << string(14, '-') << "\n\n";
        cout << string(32, '-') << "\n";
    }
    //On selected train , req is send for fetching All coaches seat map to be shown in matrix form 
}

int get_timed_input(int timeout_seconds) {
        // printf("Enter input (waiting %d seconds", timeout_seconds);
        // fflush(stdout);
        char buf[8] ; 
#ifdef _WIN32
        // Windows implementation
        #include <conio.h>
        #include <windows.h>

        DWORD start = GetTickCount();
        while ((GetTickCount() - start) / 1000 < (DWORD)timeout_seconds) {
            if (_kbhit()) {
                cin >> buf ;
                if(!strcmp(buf,"P") || !strcmp(buf,"p") )
                    return 1; // Got input
                else return 0;
            }
            Sleep(100); // Check every 100ms
        }
        return 0; // Timeout

    #else
        // Linux/Unix implementation
        #include <unistd.h>
        #include <sys/select.h>

        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(0, &fds); // 0 = stdin

        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;

        int ret = select(1, &fds, NULL, NULL, &tv);
        if (ret > 0) {
            cin >> buf ; 
            if(!strcmp(buf,"P") || !strcmp(buf,"p"))
                return 1; // Got input
            else return 0; 
        } else if (ret == 0) {
            return 0; // Timeout
        } else {
            perror("select");
            return -1; // Error
        }
    #endif
}

void handleBookTicket(const User* user) {
    string date, from, to;
    cout << "Booking - Date (YYYY-MM-DD): "; cin >> date;
    cout << "Boarding Station: ";   cin >> from;
    cout << "Destination Station: ";cin >> to;

    string reqList = "LIST_TRAINS|" + date + "|" + from + "|" + to;
    string listResp;
    sendRequestToServer(reqList, listResp);

    vector<TrainInfo> trains;
    istringstream respStream(listResp);
    string line;
    TrainInfo current;
    while (getline(respStream, line)) {
        if (line.empty()) continue;
        auto parts = split(line, '|');
        if (parts[0] == "TRAIN") {
            current = TrainInfo();
            current.train_no = parts[1];
            current.name     = parts[2];
            current.dep_time = parts[3];
            current.arr_time = parts[4];
            current.distance = stoi(parts[5]);
            trains.push_back(current);
        } else if (parts[0] == "COACH") {
            CoachInfo ci;
            ci.class_name  = parts[1];
            ci.avail_seats = stoi(parts[3]);
            ci.fare        = stod(parts[4]);
            trains.back().coaches.push_back(ci);
        } else if (parts[0] == "END_TRAINS") {
            break;
        }
    }

    if (trains.empty()) {
        cout << "No trains available for this route/date.\n";
        return;
    }
    
    // Display all train details
    showAvailableTrains(trains);

    int tchoice;
    while (true) {
        cout << "\nSelect Train (0 to cancel):\n";
        for (size_t i = 0; i < trains.size(); ++i) {
            auto &t = trains[i];
            cout << i+1 << ") " << t.train_no << " " << t.name
                 << " Dep:" << t.dep_time
                 << " Arr:" << t.arr_time
                 << " Dist:" << t.distance << "km\n";
        }
        cout << "> "; cin >> tchoice;
        if (tchoice == 0) return;
        if (tchoice > 0 && tchoice <= (int)trains.size()) break;
        cout << "Invalid selection. Try again.\n";
    }
    TrainInfo &chosenTrain = trains[tchoice - 1];

    int cchoice;
    while (true) {
        cout << "\nSelect Class (0 to reselect train):\n";
        for (size_t i = 0; i < chosenTrain.coaches.size(); ++i) {
            auto &c = chosenTrain.coaches[i];
            cout << i+1 << ") Class:" << c.class_name
                 << " Seats:" << c.avail_seats
                 << " Fare:" << c.fare << "\n";
        }
        cout << "> "; cin >> cchoice;
        if (cchoice == 0) { handleBookTicket(user); return; }
        if (cchoice > 0 && cchoice <= (int)chosenTrain.coaches.size()) break;
        cout << "Invalid selection. Try again.\n";
    }
    CoachInfo &chosenClass = chosenTrain.coaches[cchoice - 1];

    vector<pair<string,int>> passengers;
    double totalFare = 0;
    int no_of_paxgt60 = 0; 
    int no_of_paxlt60 = 0;
    while (true) {
        cout << "\n1) Add Passenger  2) Get Fare & Confirm  3) Cancel Booking\n> ";
        int opt; cin >> opt;
        if (opt == 1) {
            string pname; int age;
            cout << "Passenger Name: "; cin >> ws; getline(cin, pname);
            cout << "Passenger Age: ";  cin >> age;
            if(age >=60) no_of_paxgt60 ++ ;
            else no_of_paxlt60 ++ ;
            passengers.emplace_back(pname, age);
            totalFare += chosenClass.fare;
            cout << "Added. Current total passengers: " << passengers.size() << "\n";
        }
        else if (opt == 2) {
            if (passengers.empty()) {
                cout << "No passengers added yet.\n";
                continue;
            }
            cout << "Total Passengers: " << passengers.size() << endl ;
            cout << "Total Fare: " << totalFare << endl; 
            if(passengers.size() >= 5 ) {
                totalFare = totalFare * 0.8 ;
                cout << "Discount applicable : " << totalFare * 0.2 << endl;
            }
            else if (passengers.size() >= 3 ) {
                cout << "Discount applicable : " << totalFare * 0.1 << endl;
                totalFare = totalFare * 0.9 ;
            }
            else {
                cout << "Discount applicable : " << totalFare * 0 << endl;
            }
            cout << "Amount to be paid: " << totalFare << "\n";
            cout << "Confirm Booking? (1=Yes, 2=No) > ";
            int conf; cin >> conf;
            ostringstream req;
            if (conf == 1) {
                sort(passengers.begin(), passengers.end(), [](auto &a, auto &b){return a.second > b.second;});
                req << "BOOK_REQUEST|" << user->getId() << "|"
                    << date << "|" << from << "|" << to << "|"
                    << chosenTrain.train_no << "|" << chosenClass.class_name << "|" << chosenClass.fare << "|"
                    << no_of_paxgt60 << "|" << no_of_paxlt60 << "|";
                for (auto &p : passengers) {
                    req << p.first << ":" << p.second << ";";
                }
                cout << "\nBooking Request :\n"<< req.str() << endl ;
                string response;
                sendRequestToServer(req.str(), response);

                ///////////////Payment 
                cout << response << endl ;

                auto parts = split(response, '|');
                if (parts.size() < 3 || parts[0] != "PAYMENT_REQUEST") {
                    cerr << "Protocol error: expected PAYMENT_REQUEST\n";
                    return;
                }


                double amount   = stod(parts[1]);
                int availSeats  = stoi(parts[2]);
                long int txn = stoi(parts[3]);

                cout << "Amount payable: Rs. " << fixed << setprecision(2) << amount << "\n";
                cout << "Seats still available: " << availSeats << "\n";
                cout << "You have 2 minutes to complete payment.\n";
                cout << "Press 'P' when payment done, anything else to cancel:\n";

                char input[100];
                int result = get_timed_input(120);
                cout << "\n\nProcessing payment........" << result << endl ;

                if (result == 1) {
                    snprintf(input,sizeof(input),"PAYMENT|%.2f|%ld|SUCCESS",amount,txn) ;
                    cout << input << endl ;
                    sendRequestToServer(input,response);
                    // Now expect actual ticket response
                    cout << "\nBooking Response:\n" << response << endl;
                    struct TicketDetails ticket = parseTicketDetails(response);
                    ticket.display();
                    return;
                } else if (result == 0) {
                    snprintf(input,sizeof(input),"PAYMENT|%f|%ld|FAILED",amount,txn);
                    cout << input << endl; 
                    sendRequestToServer(input,response);
                    // Now expect actual ticket response
                    cout << "\nBooking Response:\n" << response << endl;
                }
            }
        }
        else if (opt == 3) {
            cout << "Booking cancelled. Returning to menu.\n";
            return;
        }
        else {
            cout << "Invalid option.\n";
        }
    }
}

void handleCancelTicket(const User* user) {
    string pnr;
    cout << "Enter PNR to cancel: "; cin >> pnr;
    string req = "CANCEL|" + pnr;
    cout << req << endl ; 
    string resp;
    sendRequestToServer(req, resp);
    cout << "Cancel Response: " << resp << endl;
}

void runTcpClient(const User* user) {
    cout << "\nWelcome, " << user->getFullName() << "!" << endl;
    while (true) {
        displayHomeMenu();
        int choice; cin >> choice;
        switch (choice) {
            case 1: handleShowTickets(user);    break;
            case 2: handleCheckAvailability();  break;
            case 3: handleBookTicket(user);     break;
            case 4: handleCancelTicket(user);   break;
            case 5: cout << "Logging out...\n"; return;
            default: cout << "Invalid choice!" << endl;
        }
    }
}

User* handleLoginResponse(string& response) {
    auto parts = split(response, '|');
    if (parts.size() < 8 || parts[0] != "SUCCESS") return nullptr;
    int id = stoi(parts[1]);
    return new User(id, parts[2], parts[3], parts[4], parts[5], parts[6][0], parts[7]);
}

void registrationFlow() {
    string fname, lname, email, password, mobile, dateofbirth; char gender;
    cout << "\nRegistration\nFirst Name: "; cin >> fname;
    cout << "Last Name: ";      cin >> lname;
    cout << "Email: ";          cin >> email;
    cout << "Password: ";       cin >> password;
    cout << "Mobile: ";         cin >> mobile;
    cout << "Gender (M/F/O): ";  cin >> gender;
    cout << "Date of Birth(YYYY/MM/DD): ";       cin >> dateofbirth ;
    ostringstream req;
    req << "REGISTER|" << fname << '|' << lname << '|' << email << '|' << password
        << '|' << mobile << '|' << gender << '|' << dateofbirth ;
    string resp; sendRequestToServer(req.str(), resp);
    User* u = handleLoginResponse(resp);
    if (u) runTcpClient(u), delete u;
    else cout << "Registration failed: " << resp << endl;
}

void loginFlow() {
    string email, password;
    cout << "\nLogin\nEmail: ";    cin >> email;
    cout << "Password: ";             cin >> password;
    ostringstream req;
    req << "LOGIN|" << email << '|' << password;
    string resp; sendRequestToServer(req.str(), resp);
    User* u = handleLoginResponse(resp);
    if (u) runTcpClient(u), delete u;
    else cout << "Login failed: " << resp << endl;
}

int main() {
    while (true) {
        cout << "\n===== Railway Reservation System =====" << endl
             << "1. Register" << endl
             << "2. Login" << endl
             << "3. Exit" << endl
             << "Enter choice: ";
        int choice; cin >> choice;
        switch (choice) {
            case 1: registrationFlow(); break;
            case 2: loginFlow();        break;
            case 3: return 0;
            default: cout << "Invalid choice!" << endl;
        }
    }
}
