import mysql.connector

def get_db_connection():
    return mysql.connector.connect(
        host="127.0.0.1",
        user="esp32",
        password="esp32",
        database="esp32"
    )
