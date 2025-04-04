from flask import Flask, request, jsonify
from db_config import get_db_connection

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
            if result > 0:
                return jsonify({"status": "success", "message": "RFID is valid", "rfid": rfid})
            else:
                return jsonify({"status": "error", "message": "RFID not found"})
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)})
    else:
        return jsonify({"status": "error", "message": "Invalid input"})


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
