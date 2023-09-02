#include <WiFiManager.h>
WiFiManager wifiManager;
// #include <WiFi.h>
void setupWifi() {
  wifiManager.setConnectTimeout(10);
  wifiManager.autoConnect();
  }
