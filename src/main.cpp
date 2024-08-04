#include "EQPlatform.h"
#include "Logging.h"
#include "Network.h"
#include "webserver/AlpacaWebServer.h"
#include "Encoders.h"
#include "TelescopeModel.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_system.h>

Preferences prefs;
TelescopeModel model;
EQPlatform platform;
Network network(prefs, WE_ARE_DSC);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("starting");
  prefs.begin("DSC", false);
  // Fresh ESP32s need their wifi creds initialised (once off) as follows. Do
  // not commit. network.storeESP32WifiCreds("","");
  // network.storeHomeWifiCreds("", "");
  // network.storePhoneWifiCreds("", "");
  network.setupWifi();
  LittleFS.begin();
  // model.setAltEncoderStepsPerRevolution(-30000);
  // model.setAzEncoderStepsPerRevolution(108229);

  setupEncoders();
  platform.setupEQListener();
  setupWebServer(model, prefs, platform);
  delay(500);
}

void loop() { loopEncoders(); }
