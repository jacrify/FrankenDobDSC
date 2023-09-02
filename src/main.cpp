#include <Arduino.h>
#include "Network.h"
#include "encoders.h"
#include "alpacaWebServer.h"
#include "telescopeModel.h"

TelescopeModel model;

void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  setupWifi();

  setupWebServer(model);
  delay(3000);
  // setupEncoders();
}

void loop() {
  // loopEncoders();
}

