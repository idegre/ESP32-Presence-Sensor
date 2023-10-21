#include <WiFi.h>
#include <WiFiUdp.h>
#include <ld2410.h>
#include "secrets.h"

ld2410 radar;

uint32_t lastReading = 0;
bool radarConnected = false;

#define RADAR_RX_PIN 12
#define RADAR_TX_PIN 13
#define RADAR_OUT_PIN 2

void sendBroadcast();
void setupRadar();
void readRadar();
void IRAM_ATTR ISR();
void peopleInInterrupt();
void peopleOutInterrupt();

const char* onString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": true }}";
const char* offString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": false }}";
const char* ssid = WIFI_SSID;
const char* password = WIFIPASS;

WiFiUDP udp;
byte ipArr[10];

bool state = 0;
bool lightState = 0;
bool changeToHigh = 0;
bool changeToLow = 0;

unsigned int localPort = 38899;

void setup() {
  Serial.begin(9600);
  pinMode(RADAR_OUT_PIN, INPUT);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  udp.begin(localPort);

  sendBroadcast();
  delay(150);
  setupRadar();
  attachInterrupt(RADAR_OUT_PIN, ISR, CHANGE);
}

void IRAM_ATTR ISR(){
  if (digitalRead(RADAR_OUT_PIN) == HIGH) {
    changeToHigh = 1;
  } else {
    changeToLow = 1;
  }
}

void peopleInInterrupt() {
  if(lightState == 0) {
    udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
    udp.print(onString);
    udp.endPacket();
    lightState = 1;
  }
  readRadar();
}

void peopleOutInterrupt() {
  if(lightState == 1) {
    udp.beginPacket(IPAddress(255,255,255,255), localPort);
    udp.print(offString);
    udp.endPacket();
    lightState = 0;
  }
}

void loop() {
  if(changeToHigh) {
    Serial.println("presenceDetected");
    peopleInInterrupt();
    changeToHigh = 0;
  }
  if(changeToLow) {
    Serial.println("presenceUndetected");
    peopleOutInterrupt();
    changeToLow = 0;
  }
}

void sendBroadcast() {
  udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
  udp.print("{\"method\":\"getPilot\",\"params\":{}}");
  udp.endPacket();
  Serial.printf("sent discovery\n");

  delay(150); 
  
  int packetSize = udp.parsePacket();
  byte p = 0;
  while (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    incomingPacket[len] = 0;  // null-terminate the string
    Serial.printf("Received %d bytes from %s: %s\n", packetSize, udp.remoteIP().toString().c_str(), incomingPacket);
    ipArr[p] = udp.remoteIP()[3];
    packetSize = udp.parsePacket();
    p++;
  }
}

void readRadar() {
  radar.read();
  if(radar.isConnected() && millis() - lastReading > 1000)  //Report every 1000ms
  {
    lastReading = millis();
    if(radar.presenceDetected()) {
      if(radar.stationaryTargetDetected())
      {
        Serial.print(F("Stationary target: "));
        Serial.print(radar.stationaryTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(radar.stationaryTargetEnergy());
        Serial.print(' ');
      }
      if(radar.movingTargetDetected())
      {
        Serial.print(F("Moving target: "));
        Serial.print(radar.movingTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(radar.movingTargetEnergy());
      }
      Serial.println();
    } else {
              udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
        udp.print(offString);
        udp.endPacket();
      Serial.println(F("No target"));
    }
  }
}

void setupRadar() {
  // radar.debug(Serial);
  Serial2.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); //UART for monitoring the radar
  delay(500);
  Serial.print(F("LD2410 radar sensor initialising: "));
  if(radar.begin(Serial2))
  {
    Serial.println(F("OK"));
    Serial.print(F("LD2410 firmware version: "));
    Serial.print(radar.firmware_major_version);
    Serial.print('.');
    Serial.print(radar.firmware_minor_version);
    Serial.print('.');
    Serial.println(radar.firmware_bugfix_version, HEX);
  }
  else
  {
    Serial.println(F("not connected"));
  }
}

