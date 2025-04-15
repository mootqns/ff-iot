#include <WiFi.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
string getAverageLength(String rfid) {
  int http_code = -1;
  int retry_count = 0;
  
  while (http_code < 0 && retry_count < 5) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "http://" + String(serverIP) + ":5000/average?rfid=" + rfid;
      http.begin(url);

      http_code = http.GET();
      if (http_code > 0) {
        String response = http.getString();
        Serial.println("RFID Check Response: " + response);
        http.end();
        return response;
      } else {
        retry_count++;
        Serial.println("GET failed: " + http_code);
      }
      http.end();
    }
    else{
      return false;
    }
    delay(1000); // Retry after 1 second if connection failed
  }
  return false;
}