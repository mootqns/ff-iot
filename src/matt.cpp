#include <Arduino.h>
#include <Audio.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>

// I2S pin configuration
#define I2S_DOUT 5
#define I2S_BCLK 18
#define I2S_LRC 19

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Addressable LED Strip Pin
#define LED_PIN  13
#define NUM_LEDS 14

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create the NeoPixel LED strip object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// File path in SPIFFS
const char* mp3FilePath = "/boing.mp3";  

// Audio object
Audio audio;

void setup() {
  Serial.begin(115200);

  // Initialize I2C communication
  Wire.begin(23, 22); // SDA SCL

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    for (;;);
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);  // Small text size

  // Set cursor position
  display.setCursor(0, 0);

  // Print Hello World
  display.print(F("boing.mp3"));

  // Display the text
  display.display();

  // Initialize the LED strip
  strip.begin();
  strip.show();  
  strip.setBrightness(100);  // Set brightness (0-255)
  
  // Set initial LED colors
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 255)); 
  }
  strip.show();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Initialization Failed!");
    return;
  }
  Serial.println("SPIFFS Initialized!");

  // Initialize I2S audio
  Serial.println("Initializing I2S...");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(15); // Set volume (0-21)

  // Start MP3 playback from SPIFFS
  if (SPIFFS.exists(mp3FilePath)) {
    Serial.print("Playing MP3 from SPIFFS: ");
    Serial.println(mp3FilePath);
    audio.connecttoFS(SPIFFS, mp3FilePath);
  } else {
    Serial.println("Error: MP3 file not found in SPIFFS!");
  }
}

void loop() {
  audio.loop();  
}

// Debugging functions
void audio_info(const char* info) { Serial.print("Info: "); Serial.println(info); }
void audio_eof_mp3(const char* info) { Serial.print("End of MP3: "); Serial.println(info); }