# RailwayReservation
🚆 Railway Reservation System
This project is a simple Railway Reservation System built as part of an Operating Systems mini project. It demonstrates the use of process management, file handling, and basic synchronization concepts in C. The system allows users to perform operations like booking tickets, viewing available trains, checking reservation details, and cancelling tickets. It's a command-line based project that mimics real-world railway reservation functionalities in a simplified form, helping understand core OS-level operations through practical implementation.

🧠 Concepts Used:

Multithreading: To handle multiple client requests simultaneously.

Sockets & Networking: For communication between client and server modules.

Database Integration: To store and retrieve user, train, and booking information.

Transactions: To ensure consistent and reliable booking and cancellation processes.

📜 Reservation Policy:

Priority Allocation: Seats are allocated based on priority (e.g., senior citizens, early bookings).

Cancellation Rules: Penalties and refund rules based on cancellation timing.

Dynamic Pricing: Ticket prices vary based on demand and availability.

Group Discounts: Discounts applied for group bookings.

Waitlisting: System maintains a waitlist and updates status as seats become available.
