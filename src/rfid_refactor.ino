#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include "time.h"

const char* ssid = "CenturyLink2609-2.4G";
const char* password = "";

MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};       // Create SPI driver
MFRC522 mfrc522{driver};               // Create MFRC522 instance

String curUid = "";
String sessionStartTime;
const int minSessionTime = 60;         // in seconds

// Replace with server IP:
const char* serverIP = "";

String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }

  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

bool checkValidRfid(String rfid) {
  int http_code = -1;
  int retry_count = 0;
  
  while (http_code < 0 && retry_count < 5) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "http://" + String(serverIP) + ":5000/check?rfid=" + rfid;
      http.begin(url);

      http_code = http.GET();
      if (http_code > 0) {
        String response = http.getString();
        Serial.println("RFID Check Response: " + response);
        http.end();
        return response == "true";
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

void postSession(String rfid, String startTime, String endTime) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://" + String(serverIP) + ":5000/update");
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"rfid\":\"" + rfid + "\",\"start_time\":\"" + startTime + "\",\"end_time\":\"" + endTime + "\"}";
    int code = http.POST(payload);

    if (code > 0) {
      String response = http.getString();
      Serial.println("POST Response:");
      Serial.println(response);
    } else {
      Serial.println("POST failed: " + http.errorToString(code));
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // Set up time
  configTime(0, 0, "pool.ntp.org"); // Adjust timezone if needed

  // Initialize RFID
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println("Scan a card");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Convert UID to hex string
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }

  Serial.println("Card Scanned: " + uidString);

  if (!checkValidRfid(uidString)) {
    Serial.println("Invalid RFID");
    delay(1000);
    return;
  }

  String currentTime = getCurrentTime();

  if (curUid == "") {
    // Start new session
    curUid = uidString;
    sessionStartTime = currentTime;
    Serial.println("Session started at " + sessionStartTime);
  }
  else if (curUid != uidString) {
    Serial.println("Station occupied by: " + curUid);
  }
  else {
    // Same UID, check if enough time passed
    struct tm startTm, nowTm;
    strptime(sessionStartTime.c_str(), "%Y-%m-%d %H:%M:%S", &startTm);
    strptime(currentTime.c_str(), "%Y-%m-%d %H:%M:%S", &nowTm);

    time_t startEpoch = mktime(&startTm);
    time_t nowEpoch = mktime(&nowTm);
    double secondsElapsed = difftime(nowEpoch, startEpoch);

    if (secondsElapsed >= minSessionTime) {
      postSession(uidString, sessionStartTime, currentTime);
      curUid = "";
      Serial.println("Session logged.");
    } else {
      Serial.println("Not enough time elapsed.");
    }
  }

  delay(1000); // small delay to avoid reading the same card repeatedly
}