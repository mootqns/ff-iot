#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <MFRC522v2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"

uint8_t broadcastAddress[] = {0x3C, 0x8A, 0x1F, 0xD4, 0x1F, 0xF8};
esp_now_peer_info_t peerInfo;

// Addressable LED Strip Pin
#define LED_PIN  13
#define NUM_LEDS 14

// Create the NeoPixel LED strip object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//wifi setup
const char* ssid = "CPS-Cyber-01";
const char* password = "unhappy3sandstonedeflector";

MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};       // Create SPI driver
MFRC522 mfrc522{driver};               // Create MFRC522 instance

String curUid = "";
String sessionStartTime;
const int minSessionTime = 10;         // in seconds

// Replace with server IP:
const char* serverIP = "35.185.229.173";

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

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

  WiFi.mode(WIFI_AP); //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }
  int currentChannel = WiFi.channel(); 
  Serial.println("Channel: ");
  Serial.println(currentChannel);

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Set up time
  configTime(0, 0, "pool.ntp.org"); // Adjust timezone if needed

  // Initialize RFID
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println("Scan a card");

  // Initialize the LED strip
  strip.begin();
  strip.show();  
  strip.setBrightness(100);  // Set brightness (0-255)

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

    //set red
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(255, 0, 0)); 
    }
    strip.show();
    delay(1000);
    //clear lights
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0)); 
    }
    strip.show();
    return;
  }
  else{
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 255, 0)); 
    }
    strip.show();
  }

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &uidString, sizeof(uidString)+1);

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

      //Clear lights
      for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); 
      }
      strip.show();
    } else {
      Serial.println("Not enough time elapsed.");
    }
  }

  delay(1000); // small delay to avoid reading the same card repeatedly
}