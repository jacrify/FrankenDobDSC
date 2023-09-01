#include <Arduino.h>
#include "Network.h"
#include "encoders.h"



void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  setupWifi();
  delay(3000);
  setupEncoders();
}

void loop() {
  loopEncoders();
}

