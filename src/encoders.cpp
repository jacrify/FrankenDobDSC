#include "Encoders.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <ESP32Encoder.h>
ESP32Encoder altEncoder;
ESP32Encoder aziEncoder;



const long resolution_az = 108531; // 36900 on paper.
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



void printFirmware() {
  client.print("Frankendob DSB ");
  client.print("1.0");
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

volatile long getEncoderAz() { return -aziEncoder.getCount(); }
volatile long getEncoderAl() { return -altEncoder.getCount(); }

void setupEncoders() {
  delay(1000);
  pinMode(enc_al_A, INPUT_PULLUP);
  pinMode(enc_al_B, INPUT_PULLUP);
  pinMode(enc_az_A, INPUT_PULLUP);
  pinMode(enc_az_B, INPUT_PULLUP);
  aziEncoder.attachFullQuad(enc_az_A, enc_az_B);
  aziEncoder.setCount(0);

  altEncoder.attachFullQuad(enc_al_A, enc_al_B);
  altEncoder.setCount(0);

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
      sprintf(response, "%+05d\t%+05d\r", getEncoderAl(), getEncoderAz());
      client.print(response);

      printEncoderValue(getEncoderAz());
      client.print("\t");
      printEncoderValue(getEncoderAl());
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




