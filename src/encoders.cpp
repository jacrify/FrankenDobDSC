#include "encoders.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>



String firmwareVersion = "2.2";

const long resolution_az = 108229; // 36900 on paper.
const long resolution_alt = 30000;

WiFiServer server(4030);
WiFiClient client;

// #define enc_az_A 34
// #define enc_az_B 35
// #define enc_al_A 25
// #define enc_al_B 26
#define enc_az_A 25
#define enc_az_B 26
#define enc_al_A 27
#define enc_al_B 14

volatile int lastEncodedAl = 0, lastEncodedAz = 0;
volatile long encoderValueAl = 0, encoderValueAz = 0;

volatile int lastDirectionAz = 0;
volatile unsigned long lastChangeTimeAz = 0;

volatile int lastDirectionAl = 0;
volatile unsigned long lastChangeTimeAl = 0;

volatile long alSkipCount=0;
volatile long azSkipCount = 0;

const unsigned long directionChangeThreshold = 50;

void printFirmware() {
  client.print("Magig DSC ");
  client.print(firmwareVersion);
  client.print(", az rezolution = ");
  client.print(resolution_az);
  client.print(", alt rezolution = ");
  client.print(resolution_alt);
  client.print("\r");
}

void printResolution() {


  client.print(resolution_az);
  client.print("-");
  client.print(resolution_alt);
}

volatile long getEncoderAz() { return encoderValueAz;}
volatile long getEncoderAl() { return encoderValueAl; }

volatile long getEncoderSkipsAz() { return azSkipCount; }
volatile long getEncoderSkipsAl() { return alSkipCount; }

void EncoderAl() {
  unsigned long currentTime = millis();

  int encodedAl = (digitalRead(enc_al_A) << 1) | digitalRead(enc_al_B);
  int sum = (lastEncodedAl << 2) | encodedAl;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    if (lastDirectionAl == -1 &&
        (currentTime - lastChangeTimeAl) > directionChangeThreshold) {
      alSkipCount++;
      lastChangeTimeAl = currentTime;
    }
    encoderValueAl++;
    lastDirectionAl = 1;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    if (lastDirectionAl == 1 &&
        (currentTime - lastChangeTimeAl) > directionChangeThreshold) {
      Serial.println("Missed tick detected in EncoderAl (CCW)!");
      lastChangeTimeAl = currentTime;
    }
    encoderValueAl--;
    lastDirectionAl = -1;
  }
  lastEncodedAl = encodedAl;
}

void EncoderAz() {
  unsigned long currentTime = millis();

  int encodedAz = (digitalRead(enc_az_A) << 1) | digitalRead(enc_az_B);
  int sum = (lastEncodedAz << 2) | encodedAz;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    if (lastDirectionAz == -1 &&
        (currentTime - lastChangeTimeAz) > directionChangeThreshold) {
      azSkipCount++;
      lastChangeTimeAz = currentTime;
    }
    encoderValueAz++;
    lastDirectionAz = 1;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    if (lastDirectionAz == 1 &&
        (currentTime - lastChangeTimeAz) > directionChangeThreshold) {
      Serial.println("Missed tick detected in EncoderAz (CCW)!");
      lastChangeTimeAz = currentTime;
    }
    encoderValueAz--;
    lastDirectionAz = -1;
  }
  lastEncodedAz = encodedAz;
}

void setupEncoders() {
  delay(1000);
  pinMode(enc_al_A, INPUT_PULLUP);
  pinMode(enc_al_B, INPUT_PULLUP);
  pinMode(enc_az_A, INPUT_PULLUP);
  pinMode(enc_az_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(enc_al_A), EncoderAl, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_al_B), EncoderAl, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_az_A), EncoderAz, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_az_B), EncoderAz, CHANGE);

  server.begin();
}
void printEncoderValue(long val) {

  unsigned long aval;

  if (val < 0)
    client.print("-");
  else
    client.print("+");

  aval = abs(val);

  if (aval < 10)
    client.print("0000");
  else if (aval < 100)
    client.print("000");
  else if (aval < 1000)
    client.print("00");
  else if (aval < 10000)
    client.print("0");

  client.print(aval);
}

void loopEncoders() {
  //this is for sky safari "direct encoder support"

  if (server.hasClient()) {
    client = server.available();

    int c = client.read();
    if (c == 81) {
      char response[50];
      sprintf(response, "%+05d\t%+05d\r", encoderValueAz, encoderValueAl);
      client.print(response);
      
      printEncoderValue(encoderValueAz);
      client.print("\t");
      printEncoderValue(encoderValueAl);
      client.print("\r");
      
    } else if (c == 86) {
      Serial.println("Client requested firmware.");
      printFirmware();
    } else if (c == 72) {
      Serial.println("Client requested res.");
      printResolution();
      client.print("\r");
    }
  }

  // close the connection:
  // client.stop();
  // Serial.println("Client Disconnected.");
  // delay(50);
}




