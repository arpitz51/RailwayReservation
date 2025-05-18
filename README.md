
#  Railway Reservation System

A C++ based terminal application simulating a multi-user train ticket booking system. It uses a client-server architecture over TCP sockets, backed by a MySQL database. The system handles registration, login, train search, ticket booking (with discounts and seat preferences), payment, ticket generation, and cancellation.
This project is a simple Railway Reservation System built as part of an Operating Systems mini project. It demonstrates the use of process management, file handling, and basic synchronization concepts in C. The system allows users to perform operations like booking tickets, viewing available trains, checking reservation details, and cancelling tickets. It's a command-line based project that mimics real-world railway reservation functionalities in a simplified form, helping understand core OS-level operations through practical implementation.

## Concepts Used:

**Multithreading:** To handle multiple client requests simultaneously.

**Sockets & Networking:** For communication between client and server modules.

**Database Integration:** To store and retrieve user, train, and booking information.

**Transactions:** To ensure consistent and reliable booking and cancellation processes.

## Reservation Policy:

**Priority Allocation:** Seats are allocated based on priority (e.g., senior citizens, early bookings).

**Cancellation Rules:** Penalties and refund rules based on cancellation timing.

**Dynamic Pricing:** Ticket prices vary based on demand and availability.

**Group Discounts:** Discounts applied for group bookings.

**Waitlisting:** System maintains a waitlist and updates status as seats become available.


##  Modular Design Overview

| **Module**              | **Purpose**                                                       | **Functions**                              |
|-------------------------|--------------------------------------------------------------------|---------------------------------------------|
| `User Authentication`   | Register/Login with validation                                     | `handleLogin`, `handleRegistration`         |
| `Train Listing`         | Display available trains & coaches                                 | `handleListTrains`, `handleCheckAvailability`|
| `Booking System`        | Seat locking, fare calc, ticket assignment                         | `handleBookTicket`, `processBooking`        |
| `Payment Gateway`       | 2-min window, txn logging, DB validation                           | `get_timed_input`, `handlePaymentStatus`    |
| `Ticket Management`     | Ticket formatting & response creation                              | `parseTicketDetails`, `handleShowTickets`   |
| `Seat Map Manager`      | Visual seat matrix generation                                      | `handleSeatMap`                             |
| `Cancel Reservation`    | Cancels ticket, restores seat state                                | `handleCancelTicket`, `handleCancel`        |
| `Communication Layer`   | Send/receive via TCP sockets                                       | `sendRequestToServer`, `sendResponse`       |
| `Thread Manager`        | Handles clients & bookings in parallel                             | `clientHandler`, `pthread_create()`         |
| `Database Layer`        | Executes SQL queries (MySQL C API)                                 | `mysql_*` usage in all server functions     |

---

## -Setup Instructions

1. **Install MySQL** 
   - Using railway.sql populate the database named **Train**.
   
2. **Install C++ Toolchain**
   - Linux: `g++`, `libmysqlclient-dev`, `pthread`
   - Windows: Microsoft Visual Studio C++ (see msvci.pdf) with pthreads for windows(see pthreadi.md) and linking `libmysql`, `winsock2` (see msvci.pdf).

2. **Create and Populate DB**
   - Create `Train` DB and necessary tables (`user_details`, `reservation`, `classseats`, etc.)
   - Make sure all schema match those used in code (e.g., `transaction`, `passenger_details`)

3. **Build** (for linux)
   ```bash
   g++ server.c -o server -lmysqlclient -lpthread
   g++ client.cpp -o client
   ```

    **Run**
   - Start server: `./server` (in one terminal)
   - Launch client: `./client` (in another terminal same machine)
   
   **For windows see mvsi.pdf**
---

## Request Flow Breakdown

###  LOGIN / REGISTER

**Client**
- Sends `LOGIN|email|password` or `REGISTER|details`

**Server**
- Validates user in `user_details` table
- Sends `SUCCESS|user_data` or `ERROR|message`

###  LIST_TRAINS

**Client**
- Sends `LIST_TRAINS|date|from|to`

**Server**
- Joins `train_schedule`, `train_details`, `classseats` to fetch all valid trains/coaches
- Responds with all train/coaches data

###  SEATMAP

**Client**
- Sends `GET_SEATMAP|train|date|from|to|class`

**Server**
- Fetches `SeatNo1...20` from `classseats` for each coach
- Formats visual seat map (status: `X`/`A`)

###  BOOKING REQUEST

**Client**
- Sends `BOOK_REQUEST|user_id|date|from|to|train_no|class|fare|seniors|others|name1:age1;...`

**Server**
- Inside thread:
  - Locks seats using semaphores (prefer `L` for seniors)
  - Computes discounted fare
  - Generates `transaction_id = time(NULL)`
  - Sends `PAYMENT_REQUEST|amount|available|transaction_id`

###  PAYMENT PROCESS

**Client**
- Waits for user to press `P` (with 2-minute timeout)
- Sends `PAYMENT|amount|txn_id|SUCCESS/FAILED`

**Server**
- `handlePayment()` logs into `transaction` table

**Booking thread (after sleep):**
- Queries `transaction` table by `txn_id`
- If `SUCCESS`, finalizes booking, inserts into `reservation` & `passenger_details`
- Sends `TICKET|...` response
- Else, unlocks semaphores, sends `TRANSACTION_FAILED`

###  CANCEL TICKET

**Client**
- Sends `CANCEL|pnr`

**Server**
- Checks if journey hasn’t started
- Reverts booked seats (`SeatNoX → original seat`)
- Updates seat/fare availability
- Marks ticket `CANCELLED`

###  SHOW_TICKETS

**Client**
- Sends `SHOW_TICKETS|user_id`

**Server**
- Fetches from `reservation` and `passenger_details`
- Sends ticket details per booking

---

##  Logic Highlights

- **Seat Preference:** Seniors get lower seats if available, else fallback to any.
- **Discounts:** 
  - 3+ passengers → 10%
  - 5+ passengers → 20%
- **Dynamic Fare Surge:** Fare increases by 20% if 80% occupancy is crossed (only once).
- **Timeout Handling:** Client has 2 mins to respond to payment, else it's auto-cancelled.

---
