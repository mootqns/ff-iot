DELIMITER $$

CREATE PROCEDURE GetAverageLength (
    IN p_rfid VARCHAR(8)
)
BEGIN
    SELECT AVG(session_length) AS avg_length
    FROM `session`
    WHERE rfid = p_rfid;
END $$

DELIMITER ;