#include <Arduino.h>
#include <Audio.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi
const char* ssid = "CPS-Cyber-01";
const char* password = "unhappy3sandstonedeflector";

// I2S bus configuration
#define I2S_DOUT 5
#define I2S_BCLK 18
#define I2S_LRC 19

// OLED Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Motion Detector
const int PIN_TO_SENSOR = 13;
int currentState = LOW;
int previousState = LOW;
bool localMotionDetected = false;

// MP3 file path
const char* mp3FilePath = "/boing.mp3";

// OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Audio object
Audio audio;

// Incoming data from ESP-NOW
String bathroomUser = "";

// Server IP
const char* serverIP = "35.185.229.173"; 

bool dataReceived = false; // Flag to track if data has been received

String queryDB(String rfid) {
  int http_code = -1;
  int retry_count = 0;

  while (http_code < 0 && retry_count < 5) {
    HTTPClient http;
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.status());
    String url = "http://" + String(serverIP) + ":5000/average?rfid=" + rfid;
    Serial.println("Sending request to: " + url);

    http.begin(url);
    http_code = http.GET();

    if (http_code > 0) {
      Serial.println("HTTP Code: " + String(http_code));
      String response = http.getString();
      Serial.println("RFID Check Response: " + response);
      http.end();
      return response;
    } else {
      Serial.println("GET failed, code: " + String(http_code));
      retry_count++;
    }
    http.end();
    delay(5000);
  }
  return "bad";
}

// ESP-NOW receive callback
void OnDataRecv(const uint8_t* mac, const uint8_t *incomingData, int len) {
  char incomingMessage[128]; // Adjust size as needed
  if (len < sizeof(incomingMessage)) {
    memcpy(incomingMessage, incomingData, len);
    incomingMessage[len] = '\0';  // Null-terminate the string
    bathroomUser = String(incomingMessage);
    Serial.print("Received via ESP-NOW: ");
    Serial.println(bathroomUser);
    dataReceived = true;
   
  } 
  else {
    Serial.println("Received data unexpected size");
  }
}

// Motion detection logic
void detectMotion() {
  previousState = currentState;
  currentState = digitalRead(PIN_TO_SENSOR);

  if (previousState == LOW && currentState == HIGH) {
    localMotionDetected = true;
    Serial.println("Motion detected (local)");
  } 
  else if (previousState == HIGH && currentState == LOW) {
    localMotionDetected = false;
    Serial.println("Motion ended (local)");
  }
}

void setup() {
  Serial.begin(115200);

  // Motion sensor pin
  pinMode(PIN_TO_SENSOR, INPUT);

  // Initialize ESP-NOW
  WiFi.mode(WIFI_AP);

  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }
  Serial.println("\nConnected to the WiFi network");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Channel Debug Stuff
  int currentChannel = WiFi.channel(); 
  Serial.println("Channel: ");
  Serial.println(currentChannel);
  
  // Initialize I2C communication for OLED
  Wire.begin(23, 22); // SDA, SCL

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("boing.mp3"));
  display.display();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Initialization Failed");
    return;
  }
  Serial.println("SPIFFS Initialized");

  // Initialize I2S audio
  Serial.println("Initializing I2S...");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10); // Set volume (0–21)

  // Start MP3 playback
  if (SPIFFS.exists(mp3FilePath)) {
    Serial.print("Playing MP3 from SPIFFS: ");
    Serial.println(mp3FilePath);
    audio.connecttoFS(SPIFFS, mp3FilePath);
  } 
  else {
    Serial.println("Error: MP3 file not found in SPIFFS");
  }
}

void loop() {
  audio.loop(); // Keep the audio looping

  // if (dataReceived) {
  //   String test = queryDB(bathroomUser);
  //   Serial.println(test);
  //   dataReceived = false;  // Reset flag to prevent querying again until new data is received
  // }
}
