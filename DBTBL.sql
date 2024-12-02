CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET utf8mb4;
USE `mydb`;

-- Table for stock
CREATE TABLE STOCK (
    ID INT AUTO_INCREMENT PRIMARY KEY, 
    ROW_ID CHAR(5) NOT NULL,
    PALLET INT,
    UPDATE_DATE DATE NOT NULL,
    UNIQUE (ROW_ID, UPDATE_DATE)
);

-- Table for row pallet
CREATE TABLE ROW_PALLET (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    ROW_ID CHAR(5) NOT NULL,
    PALLET_NO INT,
    EACH_PALLET INT,
    UPDATE_DATE DATE NOT NULL,
    FOREIGN KEY (ROW_ID, UPDATE_DATE) REFERENCES STOCK (ROW_ID, UPDATE_DATE)
);

-- Table for row each pallet
CREATE TABLE ROW_EACH_PALLET (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    ROW_ID CHAR(5) NOT NULL,
    PALLET_NO INT NOT NULL,
    DISTANCE DECIMAL(5,2) NOT NULL,
    ANGLE_X DECIMAL(5,2) NOT NULL,
    ANGLE_Y DECIMAL(5,2) NOT NULL,
    UPDATE_DATE DATE NOT NULL,
    FOREIGN KEY (ROW_ID, UPDATE_DATE) REFERENCES ROW_PALLET (ROW_ID, UPDATE_DATE)
);
