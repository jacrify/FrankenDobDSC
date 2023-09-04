#include <Arduino.h>
#include "Network.h"
#include "encoders.h"
#include "alpacaWebServer.h"
#include "telescopeModel.h"
#include "Logging.h"

TelescopeModel model;

void setup() {
  Serial.begin(115200);
  Serial.println("starting");

  model.setAltEncoderStepsPerRevolution(-30000);
  model.setAzEncoderStepsPerRevolution(108229);
  setupWifi();
  setupEncoders();
  setupWebServer(model);
  delay(3000);
  
}

void loop() {
  loopEncoders();
  
}

