SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

-- Database: railway
CREATE DATABASE IF NOT EXISTS Train;
USE Train;
table user_details ;

desc user_details ;
-- Table structure for table admin_details
CREATE TABLE admin_details (
  admin_id int NOT NULL COMMENT 'Admin_ID',
  username varchar(50) NOT NULL COMMENT 'First_Name',
  password varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Dumping data for table admin_details
INSERT INTO admin_details (admin_id, username, password) VALUES
(1, 'anup', 'anup007'),
(2, 'ganesh', 'ganesh'),
(3, 'cs315', 'cs315');

-- Table structure for table cancel_ticket
CREATE TABLE cancel_ticket (
  pnr_number int NOT NULL,
  ticket_fare int DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Table structure for table classseats

CREATE TABLE classseats (
    train_number INT NOT NULL,
    start_point VARCHAR(50) NOT NULL COMMENT 'Starting_Point',
    destination_point VARCHAR(50) NOT NULL COMMENT 'Destination_Point',
    journey_date DATE NOT NULL,
    class_name VARCHAR(10) NOT NULL,
    coach_number VARCHAR(10) NOT NULL DEFAULT '1',
    ticket_fare1 INT NOT NULL COMMENT 'Ticket_fare',
    available_seats INT NOT NULL DEFAULT 20 CHECK (available_seats BETWEEN 0 AND 20),
    booked_seats INT NOT NULL DEFAULT 0 CHECK (booked_seats BETWEEN 0 AND 20),
    SeatNo1 VARCHAR(10) DEFAULT '1L',
    SeatNo2 VARCHAR(10) DEFAULT '2U',
    SeatNo3 VARCHAR(10) DEFAULT '3L',
    SeatNo4 VARCHAR(10) DEFAULT '4U',
    SeatNo5 VARCHAR(10) DEFAULT '5L',
    SeatNo6 VARCHAR(10) DEFAULT '6U',
    SeatNo7 VARCHAR(10) DEFAULT '7L',
    SeatNo8 VARCHAR(10) DEFAULT '8U',
    SeatNo9 VARCHAR(10) DEFAULT '9L',
    SeatNo10 VARCHAR(10) DEFAULT '10U',
    SeatNo11 VARCHAR(10) DEFAULT '11L',
    SeatNo12 VARCHAR(10) DEFAULT '12U',
    SeatNo13 VARCHAR(10) DEFAULT '13L',
    SeatNo14 VARCHAR(10) DEFAULT '14U',
    SeatNo15 VARCHAR(10) DEFAULT '15L',
    SeatNo16 VARCHAR(10) DEFAULT '16U',
    SeatNo17 VARCHAR(10) DEFAULT '17L',
    SeatNo18 VARCHAR(10) DEFAULT '18U',
    SeatNo19 VARCHAR(10) DEFAULT '19L',
    SeatNo20 VARCHAR(10) DEFAULT '20U',
    int base_fare default ticket_fare1,
    int lower_seat default 10,
    PRIMARY KEY (train_number, start_point, destination_point, journey_date, class_name),
    KEY class_name (class_name),
    KEY start_point (start_point, destination_point),
    KEY destination_point (destination_point),
    CONSTRAINT chk_total_seats CHECK (available_seats + booked_seats <= 20)
) ;

-- To insert values in classseats see classseats.txt 

-- Triggers for table classseats
DELIMITER $$
CREATE TRIGGER add_classseats BEFORE INSERT ON classseats FOR EACH ROW begin
        if datediff(new.journey_date,curdate())<0 then
            SIGNAL SQLSTATE '45000' 
            SET MESSAGE_TEXT = 'Booking Date can not be earlier than current date';
        end if;
        if new.available_seats<=0 then 
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Number Of Seats you are looking for are not available at momemnt';
        end if;
        if new.ticket_fare1<=0 then 
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Ticket Fare can not be less than or equal to zero';
        end if;
    end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER update_classseats BEFORE UPDATE ON classseats FOR EACH ROW begin
            if datediff(curdate(),new.journey_date)>0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Booking Date can not be earlier than current date';
            end if;
            if new.ticket_fare1<=0 then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Ticket Fare can not be less than or equal to zero';
            end if;
            if new.available_seats<=0 then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Number Of Seats you are looking for are not available at momemnt';
            end if;
        end
$$
DELIMITER ;

-- Table structure for table passenger_details
CREATE TABLE passenger_details (
  pnr_number int NOT NULL,
  passenger_name varchar(50) NOT NULL COMMENT 'PASSENGER_NAME',
  passenger_age int NOT NULL COMMENT 'PASSENGER_AGE',
  passenger_gender varchar(10) NOT NULL COMMENT 'PASSENGER GENDER',
  ticket_coach varchar(10) NOT NULL COMMENT 'TICKET COACH',
  seat_number int NOT NULL COMMENT 'SEAT NUMBER',
  berth varchar(30) NOT NULL COMMENT 'BERTH'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
ALTER TABLE passenger_details
ADD COLUMN passenger_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
ADD COLUMN booking_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP;

-- Triggers for table passenger_details
DELIMITER $$
CREATE TRIGGER add_passenger_details BEFORE INSERT ON passenger_details FOR EACH ROW begin
            if new.passenger_gender NOT IN ('M','F','O') then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Enter M:Male F:Female O:Others';
            end if;
        end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER update_passenger_details BEFORE UPDATE ON passenger_details FOR EACH ROW begin
            if new.passenger_gender NOT IN ('M','F','O') then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Enter M:Male F:Female O:Others';
            end if;
        end
$$
DELIMITER ;

-- Table structure for table reservation
CREATE TABLE reservation (
  pnr_number int NOT NULL,
  user_id int NOT NULL,
  train_number int NOT NULL,
  start_point varchar(50) NOT NULL,
  destination_point varchar(50) NOT NULL,
  journey_date date NOT NULL,
  ticket_fare1 int NOT NULL COMMENT 'Ticket_Cost',
  class_name varchar(50) NOT NULL,
  number_of_seats int NOT NULL COMMENT 'Ticket Count',
  ticket_status varchar(50) NOT NULL,
  quota varchar(50) NOT NULL DEFAULT 'GENERAL',
  transaction_id int default 123456789,
  mode_of_payment varchar(20) default 'ONLINE'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Triggers for table reservation
DELIMITER $$
CREATE TRIGGER add_reservation BEFORE INSERT ON reservation FOR EACH ROW begin
            if new.ticket_fare1<0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Ticket Fare can not have value less than zero';
            end if;
            if new.number_of_seats<=0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Number Of Seats you are booking can not be less than zero';
            end if;
            if (select available_seats from classseats where train_number=new.train_number AND class_name=new.class_name AND journey_date=new.journey_date AND start_point=new.start_point AND destination_point=new.destination_point) < new.number_of_seats then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Number Of Seats You are looking for are not available at the momemnt.Sorry for inconvinience';
            end if;
            if datediff(new.journey_date,curdate())<0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Journey Date can not smaller than current date';
            end if;
                SET new.ticket_status='BOOKED';
            end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER update_reservation BEFORE UPDATE ON reservation FOR EACH ROW begin
            if new.ticket_fare1<0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Ticket Fare can not have value less than zero';
            end if;
            if new.number_of_seats<=0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Number Of Seats you are booking can not be less than zero';
            end if;
            if (select available_seats from classseats where train_number=new.train_number AND class_name=new.class_name AND journey_date=new.journey_date AND start_point=new.start_point AND destination_point=new.destination_point) < new.number_of_seats then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Journey Date can not smaller than current date';
            end if;
        end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER update_reservation_later AFTER UPDATE ON reservation FOR EACH ROW begin
            if (new.ticket_status='CANCELLED' AND datediff(new.journey_date,curdate())<0 ) then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'You can not cancel ticket of the journey that you have completed';
            end if;
            if (new.ticket_status='CANCELLED' AND datediff(new.journey_date,curdate())>=0 )then
                if datediff(new.journey_date,curdate())>=10 then 
                    INSERT INTO cancel_ticket values (new.pnr_number,new.ticket_fare1);
                end if;
                if (datediff(new.journey_date,curdate())<10) and (datediff(new.journey_date,curdate())>2) then 
                    INSERT INTO cancel_ticket values (new.pnr_number,0.2*new.ticket_fare1);
                end if;
                if datediff(new.journey_date,curdate())<=2 then 
                    INSERT INTO cancel_ticket values (new.pnr_number,0.5*new.ticket_fare1);
                end if;
            end if;
        end
$$
DELIMITER ;

-- Table structure for table station_details
CREATE TABLE station_details (
  station_id int NOT NULL,
  station_name varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


-- Table structure for table tatkal_seats
CREATE TABLE tatkal_seats (
  train_number int NOT NULL,
  start_point varchar(50) NOT NULL COMMENT 'Starting_Point',
  destination_point varchar(50) NOT NULL COMMENT 'Destination_Point',
  journey_date date NOT NULL,
  class_name varchar(10) NOT NULL,
  ticket_fare1 int NOT NULL COMMENT 'Ticket_fare',
  available_seats int NOT NULL,
  booked_seats int NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Triggers for table tatkal_seats
DELIMITER $$
CREATE TRIGGER add_tatkal_seats BEFORE INSERT ON tatkal_seats FOR EACH ROW begin
        if datediff(new.journey_date,curdate())<0 then
            SIGNAL SQLSTATE '45000' 
            SET MESSAGE_TEXT = 'Booking Date can not be earlier than current date';
        end if;
        if new.available_seats<=0 then 
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Number Of Seats you are looking for are not available at momemnt';
        end if;
        if new.ticket_fare1<=0 then 
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Ticket Fare can not be less than or equal to zero';
        end if;
    end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER update_tatkal_seats BEFORE UPDATE ON tatkal_seats FOR EACH ROW begin
            if datediff(curdate(),new.journey_date)>0 then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Booking Date can not be earlier than current date';
            end if;
            if new.ticket_fare1<=0 then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Ticket Fare can not be less than or equal to zero';
            end if;
            if new.available_seats<=0 then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Number Of Seats you are looking for are not available at momemnt';
            end if;
        end
$$
DELIMITER ;

-- Table structure for table ticket_class
CREATE TABLE ticket_class (
  class_name varchar(10) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Dumping data for table ticket_class
INSERT INTO ticket_class (class_name) VALUES
('1A'),
('2A'),
('2S'),
('3A'),
('CC'),
('SL');

-- Table structure for table train_details
CREATE TABLE train_details (
  train_number int NOT NULL,
  train_name varchar(50) NOT NULL,
  start_point varchar(50) NOT NULL,
  arrival_time time NOT NULL COMMENT 'Arrival_Time',
  destination_point varchar(50) NOT NULL,
  destination_time time NOT NULL,
  arrival_day varchar(10) DEFAULT NULL COMMENT ' Arrival_Day',
  distance int NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


DELIMITER $$
CREATE TRIGGER `add_train` BEFORE INSERT ON train_details FOR EACH ROW begin
            if (new.destination_time<new.arrival_time AND new.arrival_day='Day 1') then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Improper Timings';
            end if;
            if (new.destination_point=new.start_point) then
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Same Starting & Destination Points not allowed';
            end if;
        end
$$
DELIMITER ;
DELIMITER $$
CREATE TRIGGER `update_train` BEFORE UPDATE ON train_details FOR EACH ROW begin
          if (new.destination_time<new.arrival_time AND new.arrival_day='Day 1') then
              SIGNAL SQLSTATE '45000'
              SET MESSAGE_TEXT = 'Improper Timings';
          end if;
          end
$$
DELIMITER ;

CREATE TABLE train_schedule (
  schedule_id int NOT NULL,
  train_number int NOT NULL,
  station_name varchar(50) NOT NULL,
  arrival_time time NOT NULL,
  departure_time time NOT NULL DEFAULT '00:00:00',
  distance int NOT NULL
) ;

-- Table structure for table user_details
use Train2 ;
CREATE TABLE user_details (
  user_id int NOT NULL,
  fname varchar(50) NOT NULL COMMENT 'First_Name',
  lname varchar(50) NOT NULL COMMENT 'Last_Name',
  emailid varchar(50) NOT NULL,
  passw varchar(50) NOT NULL,
  mobile_number varchar(20) NOT NULL,
  gender char(1) NOT NULL,
  dob date NOT NULL COMMENT 'Date Of Birth'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

use Train ;
table user_details ;
table classseats ;
table reservation ;
table passenger_details ;
table train_details ;
table train_schedule ;
table cancel_ticket ;

CREATE TABLE User_details (
  user_id int NOT NULL,
  fname varchar(50) NOT NULL COMMENT 'First_Name',
  lname varchar(50) NOT NULL COMMENT 'Last_Name',
  emailid varchar(50) NOT NULL,
  passw varchar(50) NOT NULL,
  mobile_number varchar(20) NOT NULL,
  gender char(1) NOT NULL,
  dob date NOT NULL COMMENT 'Date Of Birth'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE transaction (
    transaction_id INT NOT NULL PRIMARY KEY,
    payment_amount DECIMAL(10,2) NOT NULL,
    sock INT NOT NULL,
    status VARCHAR(20) NOT NULL CHECK (status IN ('SUCCESS', 'FAILED'))
);

-- Dumping data for table user_details
INSERT INTO user_details (user_id, fname, lname, emailid, password, mobile_number, gender, dob) VALUES
(44, 'Anup', 'Dumalwad', 'anupdumalwad12@gmail.com', 'anup007', '1234567890', 'M', '2000-05-17');

-- Triggers for table user_details
DELIMITER $$
CREATE TRIGGER add_user_details BEFORE INSERT ON user_details FOR EACH ROW begin
        if (year(curdate())-year(new.dob))<18 then 
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Minimum age bar of 18 years.';
            end if;
    end
$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER before_update_on_user BEFORE UPDATE ON user_details FOR EACH ROW begin
            if (year(curdate())-year(new.dob))<18 then 
                SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = 'Minimum age bar of 18 years.';
            end if;
            end
$$
DELIMITER ;

-- Indexes for table admin_details
ALTER TABLE admin_details
  ADD PRIMARY KEY (admin_id),
  ADD UNIQUE KEY unique_admin_username (username);

-- Indexes for table cancel_ticket
ALTER TABLE cancel_ticket
  ADD PRIMARY KEY (pnr_number);

-- Indexes for table classseats
ALTER TABLE classseats
  ADD PRIMARY KEY (train_number, start_point, destination_point, journey_date, class_name),
  ADD KEY class_name (class_name),
  ADD KEY start_point (start_point, destination_point),
  ADD KEY destination_point (destination_point);

-- Indexes for table passenger_details
ALTER TABLE passenger_details
  ADD KEY passenger_details_pnr_number (pnr_number);

-- Indexes for table reservation
ALTER TABLE reservation
  ADD PRIMARY KEY (pnr_number),
  ADD KEY user_id_reservation (user_id),
  ADD KEY journey_date_train_number_class (train_number, journey_date, class_name),
  ADD KEY class_name (class_name),
  ADD KEY start_point (start_point, destination_point),
  ADD KEY destination_point (destination_point);

-- Indexes for table station_details
ALTER TABLE station_details
  ADD PRIMARY KEY (station_name),
  ADD KEY station_id (station_id);

-- Indexes for table tatkal_seats
ALTER TABLE tatkal_seats
  ADD PRIMARY KEY (train_number, start_point, destination_point, journey_date, class_name),
  ADD KEY class_name (class_name),
  ADD KEY start_point (start_point, destination_point),
  ADD KEY destination_point (destination_point);

-- Indexes for table ticket_class
ALTER TABLE ticket_class
  ADD PRIMARY KEY (class_name) COMMENT 'Ticket_Class';

-- Indexes for table train_details
ALTER TABLE train_details
  ADD PRIMARY KEY (train_number),
  ADD KEY start_point (start_point),
  ADD KEY destination_point (destination_point);

-- Indexes for table train_schedule
ALTER TABLE train_schedule
  ADD PRIMARY KEY (schedule_id),
  ADD KEY train_number (train_number),
  ADD KEY station_name (station_name),
  ADD KEY schedule_id (schedule_id),
  ADD KEY distance (distance),
  ADD KEY schedule_id2 (schedule_id);

-- Indexes for table user_details
ALTER TABLE user_details
  ADD PRIMARY KEY (user_id),
  ADD UNIQUE KEY unique_user_mobile_number (mobile_number),
  ADD UNIQUE KEY unique_user_emailid (emailid);

-- AUTO_INCREMENT for table admin_details
ALTER TABLE admin_details
  MODIFY admin_id int NOT NULL AUTO_INCREMENT COMMENT 'Admin_ID', AUTO_INCREMENT=4;

-- AUTO_INCREMENT for table reservation
ALTER TABLE reservation
  MODIFY pnr_number int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=97;

-- AUTO_INCREMENT for table station_details
ALTER TABLE station_details
  MODIFY station_id int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=30;

-- AUTO_INCREMENT for table train_details
ALTER TABLE train_details
  MODIFY train_number int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=15;

-- AUTO_INCREMENT for table train_schedule
ALTER TABLE train_schedule
  MODIFY schedule_id int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=70;

-- AUTO_INCREMENT for table user_details
ALTER TABLE user_details
  MODIFY user_id int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=45;

-- Constraints for table cancel_ticket
ALTER TABLE cancel_ticket
  ADD CONSTRAINT cancel_ticket_foreign_key_1 FOREIGN KEY (pnr_number) REFERENCES reservation (pnr_number);

-- Constraints for table classseats
-- ALTER TABLE classseats
--    ADD CONSTRAINT reservation_foreign_key_2 FOREIGN KEY (start_point) REFERENCES station_details (station_name),
--   ADD CONSTRAINT reservation_foreign_key_3 FOREIGN KEY (destination_point) REFERENCES station_details (station_name),
--   ADD CONSTRAINT classseats_foreign_key_4 FOREIGN KEY (class_name) REFERENCES ticket_class (class_name);

-- Constraints for table passenger_details
ALTER TABLE passenger_details
  ADD CONSTRAINT passenger_details_foreign_key_1 FOREIGN KEY (pnr_number) REFERENCES reservation (pnr_number);

-- Constraints for table reservation
ALTER TABLE reservation
  ADD CONSTRAINT reservation_foreign_key_1 FOREIGN KEY (train_number) REFERENCES train_details (train_number),
  ADD CONSTRAINT reservation_foreign_key_2 FOREIGN KEY (start_point) REFERENCES station_details (station_name),
  ADD CONSTRAINT reservation_foreign_key_3 FOREIGN KEY (destination_point) REFERENCES station_details (station_name),
  ADD CONSTRAINT reservation_foreign_key_4 FOREIGN KEY (user_id) REFERENCES user_details (user_id);

-- Constraints for table tatkal_seats
ALTER TABLE tatkal_seats
  ADD CONSTRAINT tatkal_seats_foreign_key_1 FOREIGN KEY (train_number) REFERENCES train_details (train_number),
  ADD CONSTRAINT tatkal_seats_foreign_key_2 FOREIGN KEY (start_point) REFERENCES station_details (station_name),
  ADD CONSTRAINT tatkal_seats_foreign_key_3 FOREIGN KEY (destination_point) REFERENCES station_details (station_name),
  ADD CONSTRAINT tatkal_seats_foreign_key_4 FOREIGN KEY (class_name) REFERENCES ticket_class (class_name);

-- Constraints for table train_details
ALTER TABLE train_details
  ADD CONSTRAINT train_details_foreign_key_1 FOREIGN KEY (start_point) REFERENCES station_details (station_name),
  ADD CONSTRAINT train_details_foreign_key_2 FOREIGN KEY (destination_point) REFERENCES station_details (station_name);

-- Constraints for table train_schedule
-- ALTER TABLE train_schedule
--   ADD CONSTRAINT train_schedule_foreign_key_1 FOREIGN KEY (station_name) REFERENCES station_details (station_name),
--   ADD CONSTRAINT train_schedule_foreign_key_2 FOREIGN KEY (train_number) REFERENCES train_details (train_number);

COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;