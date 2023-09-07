#include "Logging.h"
#include "Network.h"
#include "alpacaWebServer.h"
#include "encoders.h"
#include "telescopeModel.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_system.h>

Preferences prefs;
TelescopeModel model;

void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  prefs.begin("DSC", false);

  setupWifi(prefs);
  LittleFS.begin();
  // model.setAltEncoderStepsPerRevolution(-30000);
  // model.setAzEncoderStepsPerRevolution(108229);

  setupEncoders();
  setupWebServer(model, prefs);
  delay(500);
}

void loop() {
  loopNetwork(prefs);
  loopEncoders();
}
