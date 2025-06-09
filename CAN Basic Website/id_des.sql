CREATE DATABASE id_des;
USE id_des;

CREATE TABLE can (
    id INT AUTO_INCREMENT PRIMARY KEY,
    model ENUM('Standard', 'Extended') NOT NULL,
    can_id VARCHAR(10) NOT NULL,
    description VARCHAR(256) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci
);

INSERT INTO can (model, can_id, description)
VALUES
('Standard', '1A3', 'Cảm biến tốc độ bánh xe bên trái');

INSERT INTO can (model, can_id, description)
VALUES
('Standard', '2BC', 'Dữ liệu nhiệt độ động cơ');

INSERT INTO can (model, can_id, description)
VALUES
('Extended', '1ABCDE0F', 'Gói dữ liệu GPS với ID mở rộng');

INSERT INTO can (model, can_id, description)
VALUES
('Extended', '1C0FFEE0', 'Gửi thông tin hệ thống giải trí');

INSERT INTO can (model, can_id, description)
VALUES
('Standard', '0F5', 'Trạng thái pin ắc quy');


CREATE TABLE Transmit (
    id INT AUTO_INCREMENT PRIMARY KEY,
    model ENUM('Standard', 'Extended') NOT NULL,
    can_id VARCHAR(10) NOT NULL,
    data VARCHAR(256),
    description VARCHAR(256),
    direction ENUM('Tx', 'Rx') NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    cyclic SMALLINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE Receive (
    id INT AUTO_INCREMENT PRIMARY KEY,
    model ENUM('Standard', 'Extended') NOT NULL,
    can_id VARCHAR(10) NOT NULL,
    data VARCHAR(256),
    description VARCHAR(256),
    direction ENUM('Tx', 'Rx') NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);