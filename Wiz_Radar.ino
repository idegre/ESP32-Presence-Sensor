#include <WiFi.h>
#include <WiFiUdp.h>

void sendBroadcast();

const char* onString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": true }}";
const char* offString = "{\"id\":1,\"method\":\"setState\",\"params\":{\"state\": false }}";
const char* ssid = "";
const char* password = "";

WiFiUDP udp;
byte ipArr[10];

bool state = 0;

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

  sendBroadcast();
}

void loop() {
    // udp.beginPacket(IPAddress(255,255,255,255), localPort);
    // if(state){
    //   udp.print(onString);
    //   state = 0;
    // }else {
    //   udp.print(offString);
    //   state = 1;
    // }
    // udp.endPacket();
    int il = 0;
    while (il <= 9) {
      Serial.println(il, ipArr[il]);
      if(ipArr[il] != 0 ) {
        udp.beginPacket(IPAddress(192,168,1,ipArr[il]), localPort);
        if(state){
          udp.print(onString);
        }else {
          udp.print(offString);
        }
        Serial.println(IPAddress(192,168,1,ipArr[il]).toString().c_str());
        udp.endPacket();
        delay(2000);
      }
      il = il + 1;
    }
  Serial.println("end of send");
  if(state){
    state = 0;
  } else {
    state = 1;
  }
  delay(3000);
}

void sendBroadcast() {
  udp.beginPacket(IPAddress(255,255,255,255), localPort); // broadcast address
  udp.print("{\"method\":\"getPilot\",\"params\":{}}");
  udp.endPacket();
  Serial.printf("sent discovery\n");

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

