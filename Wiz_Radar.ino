#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "";
const char* password = "";

WiFiUDP udp;
byte ipArr[10];

unsigned int localPort = 38899;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  udp.begin(localPort);
}

void loop() {
  sendBroadcast();
    for (byte i = 0; i <= 10; i++) {
    Serial.println(ipArr[i]);
    delay(10);
  }
  delay(10000);
}

void sendBroadcast() {
  udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
  udp.print("{\"method\":\"getPilot\",\"params\":{}}");
  udp.endPacket();

  delay(100); 
  
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

