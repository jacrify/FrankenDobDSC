#include <Arduino.h>
#include "Network.h"
#include "encoders.h"
#include "alpacaWebServer.h"



void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  setupWifi();
  setupWebServer();
  delay(3000);
  // setupEncoders();
}

void loop() {
  // loopEncoders();
}

