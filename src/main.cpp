#include <Arduino.h>
#include "Network.h"
#include "encoders.h"
#include "alpacaWebServer.h"
#include "telescopeModel.h"

TelescopeModel model;

void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  model.setAltEncoderStepsPerRevolution(108229);
  model.setAzEncoderStepsPerRevolution(-30000);
  setupWifi();
  setupEncoders();
  setupWebServer(model);
  delay(3000);
  
}

void loop() {
  loopEncoders();
}

