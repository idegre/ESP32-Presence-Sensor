#include "wiz_udp.h"
#include <WiFiUdp.h>
#include "secrets.h"
#include <ArduinoJson.h>

WiFiUDP udp;

unsigned int localPort = 38899;

const char* onString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": true }}";
const char* offString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": false }}";

bool state = 0;
bool lightState = 0;

const char* targetLights[] = LIGHTS_ARRAY;
const int arraySize = sizeof(targetLights) / sizeof(targetLights[0]);
StaticJsonDocument<300> doc;

const byte networkRange = NETWORKRANGE

byte *ipArr = (byte*)malloc(sizeof(byte)*arraySize);

int isTargetLight(const char* target) {
  for (int i = 0; i < arraySize; i++) {
    if (strcmp(target, targetLights[i]) == 0) {
      return i;
    }
  }
  return -1;
}

void setupUDP() {
  udp.begin(localPort);
}

void peopleInInterrupt() {
  if(lightState == 0) {
    for(int i = 0; i < arraySize; i++) {
      udp.beginPacket(IPAddress(192,168,networkRange,ipArr[i]), localPort);
      udp.print(onString);
      udp.endPacket();
    }
    lightState = 1;
  }
}

void peopleOutInterrupt() {
  if(lightState == 1) {
    for(int i = 0; i < arraySize; i++) {
      udp.beginPacket(IPAddress(192,168,1,ipArr[i]), localPort);
      udp.print(offString);
      udp.endPacket();
    }
    lightState = 0;
  }
}

void sendBroadcast() {
  if(ipArr == NULL){
    ESP.restart();
  }

  udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
  udp.print("{\"method\":\"getPilot\",\"params\":{}}");
  udp.endPacket();

  delay(150); 
  
  int packetSize = udp.parsePacket();
  byte p = 0;
  while (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    incomingPacket[len] = 0;
    DeserializationError error = deserializeJson(doc, incomingPacket);
    if (error) {
      continue;
    }
    int lampIndex = isTargetLight(doc["result"]["mac"]);
    if(lampIndex >= 0){
      ipArr[lampIndex] = udp.remoteIP()[3];
    }
    packetSize = udp.parsePacket();
    p++;
  }
}