#include "Logging.h"
#include <WiFiManager.h>
WiFiManager wifiManager;

#include <Preferences.h>
#include <time.h>

// #include <WiFi.h>

const char *ntpServer = "pool.ntp.org";

#define HOMEWIFISSID "HOMEWIFISSID"
#define HOMEWIFIPASS "HOMEWIFIPASS"
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo,
                 "%A, %B %d %Y %H:%M:%S"); // Print the current time in a
                                           // human-readable format
}
void setupWifi(Preferences &prefs) {
  // prefs.putString(HOMEWIFISSID,"");
  // prefs.putString(HOMEWIFIPASS, "");

  log("Scanning for networks...");
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == "dontlookup") {
      log("Connecting to 'dontlookup'...");
      WiFi.begin("dontlookup", "dontlookdown");
      return;
    }
  }
  if (prefs.isKey(HOMEWIFISSID) && prefs.isKey(HOMEWIFIPASS)) {
    log("Connnecting to home wifi...");

    WiFi.begin(prefs.getString(HOMEWIFISSID).c_str(),
               prefs.getString(HOMEWIFIPASS).c_str());
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      log("Waiting for connection...");
    }

    // Once connected, log the IP address
    IPAddress ip = WiFi.localIP();
    log("Connected! IP address: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    // Init and get the time
    configTime(0, 0, ntpServer);
    printLocalTime();

  } else {
    log("No wifi details in prefs");
  }
}
