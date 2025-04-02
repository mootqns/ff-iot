#include <WiFi.h>

const char* ssid = "HomelessShelter";
const char* password = "";
const int PIN_TO_SENSOR = 14;
// LOW readings indicate no movement, whereas HIGH indicates a sensor trip
int currentState = LOW;
int previousState = LOW;

void connect(){
  delay(1000);

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
}

void detect(){
  previousState = currentState;
  currentState = digitalRead(PIN_TO_SENSOR); // read from the sensor

  if (previousState == LOW && currentState == HIGH) { // movement detected!
    Serial.println("WEE WOO");
  }
  else if (previousState == HIGH && currentState == LOW) // the person has moved from the motion sensor's proximity
  {
    Serial.println("movement no longer within range");  
  }
  // last two cases aren't important
}

void setup(){
  Serial.begin(115200);
  connect();
  pinMode(PIN_TO_SENSOR, INPUT); // reading sensor data from pin 14
}

void loop(){
  detect();
}