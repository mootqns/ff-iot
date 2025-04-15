#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>
#include "secrets.h"

SMTPSession smtp;

const int PIN_TO_SENSOR = 14;
// LOW readings indicate no movement, whereas HIGH indicates a sensor trip
int currentState = LOW;
int previousState = LOW;

// const char* accountSID = TWILIO_SID;
// const char* authToken = TWILIO_AUTH;
// const char* to = TWILIO_TO; 
// const char* from = TWILIO_FROM; 

bool currState = false;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void connect(){
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

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
}

// void postSession() {
//   if (WiFi.status() == WL_CONNECTED) {
//     String url = "https://api.twilio.com/2010-04-01/Accounts/" + String(TWILIO_SID) + "/Messages.json";
//     HTTPClient http;
//     http.begin(url);
//     http.setAuthorization(TWILIO_SID, TWILIO_AUTH);
//     http.addHeader("Content-Type", "application/x-www-form-urlencoded");

//     String payload = 
//     "To=" + String(TWILIO_TO) +
//     "&From=" + String(TWILIO_FROM) + 
//     "&Body=" + "detected movement";
//     int code = http.POST(payload);

//     if (code > 0) {
//       String response = http.getString();
//       Serial.println("POST Response:");
//       Serial.println(response);
//     } else {
//       Serial.println("POST failed: " + http.errorToString(code));
//     }
//     http.end();
//   }
// }

void detect(){
  previousState = currentState;
  currentState = digitalRead(PIN_TO_SENSOR); // read from the sensor

  if (previousState == LOW && currentState == HIGH) { // movement detected!
    // currState = true;
    Serial.println("WEE WOO");
    // postSession();
  }
  else if (previousState == HIGH && currentState == LOW) // the person has moved from the motion sensor's proximity
  {
    // currState = false;
    Serial.println("movement no longer within range");  
  }
  // last two cases aren't important
  // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &currState, sizeof(currState));
}

void setup(){
  Serial.begin(115200);
  connect();
  pinMode(PIN_TO_SENSOR, INPUT); // reading sensor data from pin 14
}

void loop(){
  detect();
  delay(2000);
  // delay(60000); // every minute
}