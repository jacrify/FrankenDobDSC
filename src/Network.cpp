// #include <WiFiManager.h>
// WiFiManager wifiManager;
#include <WiFi.h>
void setupWifi() {

  WiFi.softAP("dontlookup");
  // wifiManager.setConnectTimeout(10);
  // // wifiManager.autoConnect("dontlookup");
  // wifiManager.startConfigPortal("dontlookup");
}
