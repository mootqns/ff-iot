#include <Audio.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Prototypes
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len);
void playAudio(const char* path);
void detectMotion();
String sendHTTPRequest(String endpoint);
String queryDB(String scannedID);
void intruderNotification();

// WiFi
const char* ssid = "CPS-Cyber-01";
const char* password = "unhappy3sandstonedeflector";

// Server IP
const char* serverIP = "35.185.229.173"; 

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
const char* mp3FilePath = "/output.mp3";

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

// To ensure texts are sent at sane intervals
unsigned long textTime = 0;
const unsigned long maxTextDuration = 120000; 

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
  while(WiFi.status() != WL_CONNECTED){
      delay(100);
  }
  Serial.println("\nWiFi Connected");
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

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
  audio.setVolume(20); // Set volume (0–21)

  // Ensure first text goes through
  textTime = millis() - maxTextDuration;
}

void loop() {
  audio.loop();
  detectMotion();

  if (activeSession) {
    if (dataReceived) {
      String rawResponse = queryDB(bathroomUser);
      dataReceived = false;
      
      // Expecting format: "<time>,<name>"
      int commaIndex = rawResponse.indexOf(',');
      
      if (commaIndex != -1) {
        String rawTime = rawResponse.substring(0, commaIndex);
        String userName = rawResponse.substring(commaIndex + 1);
      
        float totalSeconds = rawTime.toFloat();
        int minutes = (int)(totalSeconds / 60);
        float seconds = fmod(totalSeconds, 60.0);
      
        String msg = "User: " + userName + "\nAvg Use Time:\n " + String(minutes) + "m " + String(seconds, 2) + "s";

        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.print(msg);
        display.display();
      } 
      else {
        Serial.println("Invalid response from server: " + rawResponse);
      }
    }
  }
  else {
    // on exit of the bathroom...
    display.clearDisplay();
    display.display();
  }
}

String sendHTTPRequest(String endpoint) {
  String response = "";
  int http_code = -1;
  int retry_count = 0;
  String url = "http://" + String(serverIP) + ":5000" + endpoint;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connected. Cannot send HTTP request.");
    return response; 
  }

  while (retry_count < 5) {
    HTTPClient http;
    Serial.println("Sending request to: " + url);

    http.begin(url);
    http_code = http.GET();

    if (http_code > 0) {
      Serial.println("HTTP Code: " + String(http_code));
      response = http.getString();
      Serial.println("HTTP Response: " + response);
      http.end();
      return response;
    } else {
      Serial.println("GET failed, code: " + String(http_code));
      retry_count++;
      http.end();
      delay(5000);
    }
  }

  return response;  // might be empty string if all retries failed
}

String queryDB(String scannedID) {
  return sendHTTPRequest("/average?rfid=" + scannedID);
}

void intruderNotification() {
  String response = sendHTTPRequest("/intruder");
  if (response != "Success") {
    Serial.println(response);
  }
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

void detectMotion() {
  previousState = currentState;
  currentState = digitalRead(MOTION_PIN);

  if (previousState == LOW && currentState == HIGH) {
    Serial.println("Motion detected");
    
    // if an intruder, sound alarm
    if (activeSession == false) {
      unsigned long now = millis();
      if (now - textTime >= maxTextDuration) {
        intruderNotification();
        textTime = now;  // update the last text time
      } 
      else {
        unsigned long remaining = maxTextDuration - (now - textTime);
        Serial.print("Intruder detection rate-limited. ");
        Serial.print("Seconds remaining: ");
        Serial.println(remaining / 1000.0, 2);
      }

      playAudio(mp3FilePath);
    } 
  }
  else if (previousState == HIGH && currentState == LOW) {
    Serial.println("Motion ended");
  }
}
