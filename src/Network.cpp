#include "Logging.h"
#include <WiFiManager.h>
WiFiManager wifiManager;
#include <Preferences.h>
// #include <WiFi.h>

void loopNetwork(Preferences &prefs) {
  if (digitalRead(0) == LOW) {
    prefs.putBool("homeWifi", true);

    delay(300);
    esp_restart();
  }
}
void setupWifi(Preferences &prefs) {
  pinMode(0, INPUT); // boot button
  //set up as hotspot by default. 
  //If boot button pressed, reboot and connect to home wifi
  bool homeWifi = prefs.getBool("homeWifi");
  log("Home wifi flag value: %d", homeWifi);
  if (homeWifi) {
    log("Connecting to home wifi");
    prefs.putBool("homeWifi", false);

    wifiManager.setConnectTimeout(10);
    wifiManager.autoConnect();

  }  else {
    log("Creating access point");
    WiFi.softAP("dontlookup", "dontlookdown");
  }
  

}
