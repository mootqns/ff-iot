CREATE TABLE IF NOT EXISTS valid_RFIDs (
    rfid VARCHAR(50) PRIMARY KEY,
    username VARCHAR(50)
);

CREATE TABLE IF NOT EXISTS `session` (
    rfid VARCHAR(8),
    start_time DATETIME,
    end_time DATETIME,
    session_length SMALLINT UNSIGNED,
    PRIMARY KEY (rfid, start_time)
);
