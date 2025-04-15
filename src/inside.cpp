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

bool dataReceived = false;
bool activeSession = false;

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
  char incomingMessage[64];
  if (len < sizeof(incomingMessage)) {
    memcpy(incomingMessage, incomingData, len);
    incomingMessage[len] = '\0'; 

    if (activeSession) {
      if (String(incomingMessage) == bathroomUser) {
          Serial.println("session ended");
          activeSession = false;
          return;
      }
    }

    bathroomUser = String(incomingMessage);
    Serial.print("Received via ESP-NOW: ");
    Serial.println(bathroomUser);

    dataReceived = true;
    activeSession = true;
  } 
  else {
    Serial.println("Received data unexpected size");
  }
}

// MP3 playing logic
void playAudio(const char* path) {
  if (SPIFFS.exists(path)) {
    Serial.print("Playing audio: ");
    Serial.println(path);
    audio.connecttoFS(SPIFFS, path);
  } else {
    Serial.print("Error: File not found: ");
    Serial.println(path);
  }
}

// Motion detection logic
void detectMotion() {
  previousState = currentState;
  currentState = digitalRead(PIN_TO_SENSOR);

  if (previousState == LOW && currentState == HIGH) {
    localMotionDetected = true;
    Serial.println("Motion detected");
    if (activeSession == false) {
      playAudio(mp3FilePath);
    }
  } 
  else if (previousState == HIGH && currentState == LOW) {
    localMotionDetected = false;
    Serial.println("Motion ended");
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
  playAudio(mp3FilePath);
}

void loop() {
  audio.loop();
  detectMotion();
  if (activeSession) {
    if (dataReceived) {
      String rawTime = queryDB(bathroomUser); // e.g., "67.8912"
      dataReceived = false;
  
      float totalSeconds = rawTime.toFloat();
      int minutes = (int)(totalSeconds / 60);
      float seconds = fmod(totalSeconds, 60.0);
  
      // Format string with 2 decimal places for seconds
      String msg = "User " + bathroomUser + " used: " + String(minutes) + " min " + String(seconds, 2) + " sec";
  
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(msg);
      display.display();
    }
  }
  else {
    display.clearDisplay();
    display.display();
  }
  
}

