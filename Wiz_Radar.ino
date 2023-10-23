#include <WiFi.h>
#include <ld2410.h>
#include "secrets.h"
#include "wiz_udp.h"
#include "camera.h"

#include <HTTPSServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ResourceNode.hpp>

using namespace httpsserver;

ld2410 radar;

uint32_t lastReading = 0;
bool radarConnected = false;

#define RADAR_RX_PIN 12
#define RADAR_TX_PIN 13
#define RADAR_OUT_PIN 2
#define LEDPIN 4
#define REDLEDPIN 33

#define SERIAL_ENABLED true

HTTPServer server = HTTPServer();

void handleRoot(HTTPRequest * req, HTTPResponse * res);
void handleImage(HTTPRequest * req, HTTPResponse * res);
void handleLED(HTTPRequest * req, HTTPResponse * res);
void handleRadarReq(HTTPRequest * req, HTTPResponse * res);

void setupRadar();
void readRadar();
void IRAM_ATTR ISR();

const char* ssid = WIFI_SSID;
const char* password = WIFIPASS;
const char* wifiName = WIFINAME;

bool changeToHigh = 0;
bool changeToLow = 0;
bool ledState = false;

void setup() {
  if(SERIAL_ENABLED)Serial.begin(115200);
  pinMode(REDLEDPIN, OUTPUT);
  pinMode(RADAR_OUT_PIN, INPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  WiFi.setHostname(wifiName);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(REDLEDPIN, HIGH);
    delay(1000);
    digitalWrite(REDLEDPIN, LOW);
    if(SERIAL_ENABLED)Serial.println("Connecting to WiFi...");
  }
  if(SERIAL_ENABLED)Serial.println("Connected to WiFi");

  Serial.print("Connected. IP=");
  Serial.println(WiFi.localIP());
  ResourceNode * nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode * nodeRadar = new ResourceNode("/radar", "GET", &handleRadarReq);
  ResourceNode * nodeImage = new ResourceNode("/img", "GET", &handleImage);
  ResourceNode * nodeLED = new ResourceNode("/LED", "GET", &handleLED);
  server.registerNode(nodeRoot);
  server.registerNode(nodeImage);
  server.registerNode(nodeLED);
  server.registerNode(nodeRadar);

  configCamera();

  Serial.println("Starting server...");
  server.start();

  if (server.isRunning()) {
    Serial.println("Server ready.");
    for(int i = 0; i < 10; i++) {
      digitalWrite(REDLEDPIN, HIGH);
      delay(200);
      digitalWrite(REDLEDPIN, LOW);
    }
  }

  setupUDP();

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

void loop() {
  server.loop();
  if ((WiFi.status() != WL_CONNECTED)) {
    if(SERIAL_ENABLED)Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
  radar.read();
  if(changeToHigh) {
    if(SERIAL_ENABLED)Serial.println("presenceDetected");
    peopleInInterrupt();
    changeToHigh = 0;
  }
  if(changeToLow) {
    if(SERIAL_ENABLED)Serial.println("presenceUndetected");
    peopleOutInterrupt();
    changeToLow = 0;
  }
}

void handleImage(HTTPRequest * req, HTTPResponse * res) {
  camera_fb_t * fb = NULL; // pointer
    res->setHeader("Content-Type", "image/jpg");

  // Take a photo with the camera
  Serial.println("Taking a photo...");

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    res->setStatusCode(400);
    return;
  }
  res->write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return;
}

void handleRoot(HTTPRequest * req, HTTPResponse * res) {
  // Status code is 200 OK by default.
  // We want to deliver a simple HTML page, so we send a corresponding content type:
  res->setHeader("Content-Type", "text/html");

  // The response implements the Print interface, so you can use it just like
  // you would write to Serial etc.
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Hello World!</title></head>");
  res->println("<body>");
  res->println("<h1>Hello World!</h1>");
  res->print("<p>Your server is running for ");
  // A bit of dynamic data: Show the uptime
  res->print((int)(millis() / 1000), DEC);
  res->println(" seconds.</p>");
  res->println("</body>");
  res->println("</html>");
}

void handleRadarReq(HTTPRequest * req, HTTPResponse * res) {
  delay(20);
  if(radar.isConnected()){
    res->setHeader("Content-Type", "application/json");
    res->print("{\n\"stationary\":{\n\"distance\":");
    res->print(radar.stationaryTargetDistance());
    res->println(",\n");
    res->print("\"energy\":");
    res->print(radar.stationaryTargetEnergy());
    res->println("\n},");
    res->print("{\n\"moving\":{\n\"distance\":");
    res->print(radar.movingTargetDistance());
    res->println(",\n");
    res->print("\"energy\":");
    res->print(radar.movingTargetEnergy());
    res->println("\n},");
    res->println("}");
  }
}

void handleLED(HTTPRequest * req, HTTPResponse * res){
  if (ledState) {
    digitalWrite(LEDPIN, LOW);
  } else {
    digitalWrite(LEDPIN, HIGH);
  }
  ledState = !ledState;
}

void readRadar() {
  radar.read();
  if((radar.isConnected() && millis() - lastReading > 1000) && (SERIAL_ENABLED))  //Report every 1000ms
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
      Serial.println(F("No target"));
    }
  }
}

void setupRadar() {
  Serial2.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); //UART for monitoring the radar
  delay(500);
  if(SERIAL_ENABLED)Serial.print(F("LD2410 radar sensor initialising: "));
  if(radar.begin(Serial2) && (SERIAL_ENABLED))
  {
    Serial.println(F("OK"));
    Serial.print(F("LD2410 firmware version: "));
    Serial.print(radar.firmware_major_version);
    Serial.print('.');
    Serial.print(radar.firmware_minor_version);
    Serial.print('.');
    Serial.println(radar.firmware_bugfix_version, HEX);
    radar.setGateSensitivityThreshold(7, 15, 5);
    radar.setGateSensitivityThreshold(8, 15, 5);
  }
  else
  {
    Serial.println(F("not connected"));
  }
}

