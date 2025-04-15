#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>
#include "secrets.h"

SMTPSession smtp;

Session_Config config;
SMTP_Message message;

// Callback function to get the Email sending status
// void smtpCallback(SMTP_Status status);

const int PIN_TO_SENSOR = 14;
// LOW readings indicate no movement, whereas HIGH indicates a sensor trip
int currentState = LOW;
int previousState = LOW;

bool currState = false;
// esp_now_peer_info_t peerInfo;

void smtpConfig() {
  config.server.host_name = "smtp.gmail.com"; 
  config.server.port = 465; // port 465 is used for encrypted SMTP connections... port 25 for unencrypted
  config.login.email = AUTHOR_EMAIL; 
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";
  
  config.secure.mode = esp_mail_secure_mode_ssl_tls;
  /*
   Set the NTP config time
   For times east of the Prime Meridian use 0-12
   For times west of the Prime Meridian add 12 to the offset.
   Ex. American/Denver GMT would be -6. 6 + 12 = 18
   See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
   */
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 20;
  config.time.day_light_offset = 0;

  // for debugging
  smtp.callback([](SMTP_Status status){
    Serial.println(status.info());
  });
}

void smtpMessage() {
  message.sender.name = "Flush Factory";
  message.sender.email = AUTHOR_EMAIL;

  String subject = "Test message";
  message.subject = subject;

  message.addRecipient(F("Arvand"), RECIPIENT_SMS);

  String body = "test";
  message.text.content = body;

  // enabling non-ASCII words in the message
  message.text.transfer_encoding = "base64";
  message.text.charSet = F("utf-8"); 

  if (!smtp.connect(&config)) {
    Serial.println("Failed to connect to mail server.");
    return;
  }
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println(smtp.errorReason());
  } else {
    Serial.println("Email sent successfully!");
  }  

  smtp.closeSession();
}

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

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

  // if (esp_now_init() != ESP_OK) {
  //   Serial.println("Error initializing ESP-NOW");
  //   return;
  // }

  // esp_now_register_send_cb(OnDataSent);

  // memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  // peerInfo.channel = 0;  
  // peerInfo.encrypt = false;

  // if (esp_now_add_peer(&peerInfo) != ESP_OK){
  //   Serial.println("Failed to add peer");
  //   return;
  // }
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
  smtpConfig();
  smtpMessage();
  pinMode(PIN_TO_SENSOR, INPUT); // reading sensor data from pin 14
}

void loop(){
  // detect();
  // delay(2000);
  // delay(60000); // every minute
}