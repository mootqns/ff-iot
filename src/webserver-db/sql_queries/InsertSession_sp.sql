-- Switch to a different delimiter
DELIMITER $$

-- Create the procedure
CREATE PROCEDURE InsertSession (
    IN p_rfid VARCHAR(8),
    IN p_start DATETIME,
    IN p_end DATETIME
)
BEGIN
    INSERT INTO `session` (rfid, start_time, end_time, session_length)
    VALUES (
        p_rfid,
        p_start,
        p_end,
        TIMESTAMPDIFF(SECOND, p_start, p_end)
    );
END $$

-- Switch back to the default delimiter
DELIMITER ;


