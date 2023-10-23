#include <WiFiUdp.h>
#include <Arduino.h>

void sendBroadcast();
void peopleInInterrupt();
void peopleOutInterrupt();
void setupUDP();
int isTargetLight(const char* target);