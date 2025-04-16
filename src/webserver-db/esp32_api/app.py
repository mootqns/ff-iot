from flask import Flask, request, jsonify
from db_config import get_db_connection
import smtplib
from email.mime.text import MIMEText
from confidential import AUTHOR_EMAIL, AUTHOR_PASSWORD, RECIPIENT_SMS

app = Flask(__name__)

# Route to insert data
@app.route('/update', methods=['POST'])
def insert_data():
    data = request.json

    # Extract data from JSON
    rfid = data.get('rfid')
    start_time = data.get('start_time')
    end_time = data.get('end_time')

    # Check for valid data
    if rfid and start_time is not None and end_time is not None:
        try:
            conn = get_db_connection()
            cursor = conn.cursor()

            # Call the stored procedure
            cursor.callproc('InsertSession', (rfid, start_time, end_time))
            conn.commit()

            cursor.close()
            conn.close()

            return jsonify({"status": "success", "message": "Data inserted successfully"})
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)})
    else:
        return jsonify({"status": "error", "message": "Invalid input"})


# Route to check if RFID exists in valid_RFIDs
@app.route('/check', methods=['GET'])
def check_data():
    rfid = request.args.get('rfid')  # Get data from URL parameters

    if rfid:
        try:
            conn = get_db_connection()
            cursor = conn.cursor()

            # SQL to check if RFID exists
            sql = "SELECT COUNT(*) FROM valid_RFIDs WHERE rfid=%s"
            values = (rfid,)
            cursor.execute(sql, values)

            # Get the result of the query
            result = cursor.fetchone()[0]
            
            cursor.close()
            conn.close()

            # Check if the RFID exists
            return "true" if result > 0 else "false"
        except Exception as e:
            return "false"
    return "false"

# Route to check if RFID exists in valid_RFIDs
@app.route('/average', methods=['GET'])
def get_average():
    rfid = request.args.get('rfid')  # Get data from URL parameters

    if rfid:
        try:
            conn = get_db_connection()
            cursor = conn.cursor()

            # Call the stored procedure
            cursor.callproc('GetAverageLength', (rfid,))
            
            # Get the result of the query
            average = None
            for result in cursor.stored_results():
                row = result.fetchone()
                if row:
                    average = row[0]

            cursor.close()
            conn.close()

            return str(average)
        except Exception as e:
            return None
    else:
        None

# Route to send text notifications
@app.route('/intruder', methods=['GET'])
def get_average():
    msg = MIMEText("Intruder!")
    msg['From'] = AUTHOR_EMAIL
    msg['To'] = RECIPIENT_SMS # sends message to mobile carrier SMS gateway, which converts SMTP to SMS

    try:
        # connect to the SMTP (gmail) server via a secure SSL connection
        with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp_server:
            smtp_server.login(AUTHOR_EMAIL, AUTHOR_PASSWORD)
            smtp_server.sendmail(AUTHOR_EMAIL, RECIPIENT_SMS, msg.as_string())

        return "Success"
    except Exception as e:
        return "Fail"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
