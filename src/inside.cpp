#include <Audio.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Prototypes
String queryDB(String scannedID);
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len);
void playAudio(const char* path);
void detectMotion();
void intruderNotification();

// WiFi
const char* ssid = "";
const char* password = "";

// Server IP
const char* serverIP = ""; 

// I2S bus (amp)
constexpr int I2S_DOUT = 5;
constexpr int I2S_BCLK = 18;
constexpr int I2S_LRC = 19;

// OLED display 
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr int SCREEN_ADDRESS = 0x3C;

// File path in SPIFFS
const char* mp3FilePath = "/boing.mp3";

// Motion detector
constexpr int MOTION_PIN = 13;
int currentState = LOW;
int previousState = LOW;
bool motionDetected = false;

// Data from ESP-NOW
String bathroomUser = "";

// To handle intruder v.s. valid user logic
bool dataReceived = false;
bool activeSession = false;

// OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Audio object
Audio audio;

void setup() {
  Serial.begin(115200);

  // Motion sensor setup
  pinMode(MOTION_PIN, INPUT);

  // Connect to WiFi
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Channel debug
  // int currentChannel = WiFi.channel(); 
  // Serial.println("Channel: ");
  // Serial.println(currentChannel);
  
  // Initialize I2C for OLED
  Wire.begin(23, 22); // (SDA, SCL)

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Initialization Failed");
    return;
  }

  // Initialize I2S audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10); // Set volume (0–21)
}

void loop() {
  audio.loop();
  detectMotion();

  if (activeSession) {
    if (dataReceived) {
      String rawTime = queryDB(bathroomUser);
      dataReceived = false;
  
      float totalSeconds = rawTime.toFloat();
      int minutes = (int)(totalSeconds / 60);
      float seconds = fmod(totalSeconds, 60.0);
  
      String msg = "User " + bathroomUser + " used: " + String(minutes) + " min " + String(seconds, 2) + " sec";
  
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(msg);
      display.display();
    }
  }
  else {
    // on exit of the bathroom...
    display.clearDisplay();
    display.display();
  }
}

String queryDB(String scannedID) {
  String response = "";
  int http_code = -1;
  int retry_count = 0;

  while (retry_count < 5) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + ":5000/average?rfid=" + scannedID;
    Serial.println("Sending request to: " + url);

    http.begin(url);
    http_code = http.GET();

    if (http_code > 0) {
      Serial.println("HTTP Code: " + String(http_code));
      response = http.getString();
      Serial.println("RFID Check Response: " + response);
      http.end();
      return response; 
    } 
    else {
      Serial.println("GET failed, code: " + String(http_code));
      retry_count++;
      http.end(); 
      delay(5000);
    }
  }

  return response; 
}

// ESP-NOW callback
void OnDataRecv(const uint8_t* mac, const uint8_t *incomingData, int len) {
  char scannedID[64];
  if (len < sizeof(scannedID)) {
    memcpy(scannedID, incomingData, len);
    scannedID[len] = '\0'; 
    
    // If we get the same RFID twice in a row
    if (activeSession && String(scannedID) == bathroomUser) {
          Serial.println("Session ended");
          activeSession = false;
          return;
    }

    bathroomUser = String(scannedID);
    Serial.print("Session started for: ");
    Serial.println(bathroomUser);
    
    // update bools so we can handle a valid bathroom user
    dataReceived = true;
    activeSession = true;
  } 
  else {
    Serial.println("Received unexpected data size");
  }
}

void playAudio(const char* path) {
  if (SPIFFS.exists(path)) {
    Serial.print("Playing: ");
    Serial.println(path);
    audio.connecttoFS(SPIFFS, path);
  } 
  else {
    Serial.print("SPIFFS file not found: ");
    Serial.println(path);
  }
}

void intruderNotification(){
  int http_code = -1;
  int retry_count = 0;

  while (retry_count < 5) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + ":5000/intruder";
    Serial.println("Sending request to: " + url);

    http.begin(url);
    http_code = http.GET();

    if (http_code > 0) {
      Serial.println("HTTP Code: " + String(http_code));
      response = http.getString();
      Serial.println(response);
      http.end();
    } 
    else {
      Serial.println("GET failed, code: " + String(http_code));
      retry_count++;
      http.end(); 
      delay(5000);
    }
  }
}

void detectMotion() {
  previousState = currentState;
  currentState = digitalRead(MOTION_PIN);

  if (previousState == LOW && currentState == HIGH) {
    Serial.println("Motion detected");
    
    // if an intruder, sound alarm
    if (activeSession == false) {
      playAudio(mp3FilePath);
      intruderNotification();
    }

  } 
  else if (previousState == HIGH && currentState == LOW) {
    Serial.println("Motion ended");
  }
}