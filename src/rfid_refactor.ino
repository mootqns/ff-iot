#include <WiFi.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include "time.h"

const char* ssid = "TMOBILE-43AA";
const char* password = "";

MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

String curUid = "";

void setup() {
  Serial.begin(115200);  // Initialize serial communication

  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  //time set up
  const char* ntpServer = "pool.ntp.org";
  const long  gmtOffset_sec = 0;        // Adjust for your timezone, e.g., -18000 for EST (-5 hours)
  const int   daylightOffset_sec = 0;   // Add 3600 if DST is in effect

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  
  mfrc522.PCD_Init();    // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
	Serial.println(F("Scan PICC to see UID"));
}

void loop() {
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if (!mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards.
	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

    struct tm timeInfo;
    getLocalTime(&timeinfo)
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  // Save the UID in a String variable
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0"; 
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  if (curUid != uidString){
    Serial.println(uidString);
    curUid = uidString;
  }
  
}