#!/bin/bash

#add session
curl -X POST -H "Content-Type: application/json"     -d '{"rfid":"123456","start_time":"2025-03-21 10:00:00","end_time":"2025-03-21 11:00:00"}'     http://localhost:5000/update


curl -X GET "http://localhost:5000/check?rfid=1bbda80d"

curl -X GET "http://localhost:5000/average?rfid=3c84be79"

curl -X GET "http://localhost:5000/intruder"