#include "EQPlatform.h"

#include "Logging.h"
#include "TimePoint.h"
#include "WiFi.h"
#include <ArduinoJson.h>


#define STALE_POSITION_TIME 200
#define IPBROADCASTPERIOD 10000
#define IPBROADCASTPORT 50375
#define STALE_EQ_WARNING_THRESHOLD_SECONDS 10 // used to detect packet loss

// used to track ra/dec requests, so we do for one but not both
unsigned long long lastPositionCalculatedTime;
TimePoint lastPositionReceivedTime;

void EQPlatform::sendEQCommand(String command, double parm) {
  // Check if the device is connected to the WiFi
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (eqUDPOut.connect(
          IPAddress(255, 255, 255, 255),
          IPBROADCASTPORT)) { // Choose any available port, e.g., 12345
    char response[400];

    snprintf(response, sizeof(response),
             "DSC:{ "
             "\"command\": %s, "
             "\"parameter\": %.5lf"
             " }",
             command, parm);
    eqUDPOut.print(response);
    log("Status Packet sent");
  }
}

void EQPlatform::processPacket(AsyncUDPPacket &packet) {
  unsigned long now = millis();
  String msg = packet.readString();
  log("UDP Broadcast received: %s", msg.c_str());

  // Check if the broadcast is from EQ Platform
  if (msg.startsWith("EQ:")) {
    msg = msg.substring(3);
    log("Got payload from eq plaform");

    // Create a JSON document to hold the payload
    const size_t capacity =
        JSON_OBJECT_SIZE(5) + 40; // Reserve some memory for the JSON document
    StaticJsonDocument<capacity> doc;

    // Deserialize the JSON payload
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      log("Failed to parse payload %s with error %s", msg.c_str(),
          error.c_str());
      return;
    }

    if (doc.containsKey("timeToCenter") && doc.containsKey("timeToEnd") &&
        doc.containsKey("isTracking") && doc["timeToCenter"].is<double>() &&
        doc["timeToEnd"].is<double>()) {
      runtimeFromCenterSeconds = doc["timeToCenter"];
      timeToEnd = doc["timeToEnd"];
      currentlyRunning = doc["isTracking"];
      lastPositionReceivedTime = getNow();

      IPAddress remoteIp = packet.remoteIP();

      // Convert the IP address to a string
      eqPlatformIP = remoteIp.toString();
      log("Distance from center %lf, running %d", runtimeFromCenterSeconds,
          currentlyRunning);
    } else {
      log("Payload missing required fields.");
      return;
    }
  } else {
    log("Message has bad starting chars");
  }
}

void EQPlatform::setupEQListener() {
  // set up listening for ip address from eq platform

  // Initialize UDP to listen for broadcasts.

  if (eqUdpIn.listen(IPBROADCASTPORT)) {
    log("Listening for eq platform broadcasts");
    eqUdpIn.onPacket(
        [this](AsyncUDPPacket packet) { this->processPacket(packet); });
  }
}

void EQPlatform::checkConnectionStatus() {
  TimePoint now = getNow();
  if (differenceInSeconds(lastPositionReceivedTime, now) >
      STALE_EQ_WARNING_THRESHOLD_SECONDS) {
    platformConnected = false;
  } else {
    platformConnected = true;
  }
}

/**
 * Returns an time (epoch in millis) to be used for model calculations.
 * This is obstensibly the time that the platform is
 * as the middle of the run, it can be in the past or in the future.
 * Checks to see if a packet arrived from eq platform recently
 *  If not mark platform as not connected.
 *
 */
TimePoint EQPlatform::calculateAdjustedTime() {
  TimePoint now = getNow();

  checkConnectionStatus();
  log("Calculating  adjusted time from (now): %s",
      timePointToString(now).c_str());
  // unsigned long now = millis();
  // log("Now millis since start: %ld", now);

  // if platform is running, then it has moved on since packet
  // recieved. The time since the packet received needs to be added to
  // the timeToCenter.
  // eg if time to center is 100s, and packet was received a second ago,
  // time to center should be considered as 99s.
  double interpolationTimeSeconds = 0;
  if (currentlyRunning) {
    // log("setting interpolation time");
    interpolationTimeSeconds =
        differenceInSeconds(lastPositionReceivedTime, now);
  }
  TimePoint adjustedTime = addSecondsToTime(now, runtimeFromCenterSeconds -
                                                     interpolationTimeSeconds);

  log("Returned adjusted time: %s", timePointToString(adjustedTime).c_str());
  return adjustedTime;
}

bool EQPlatform::checkStalePositionAndUpdate() {
  unsigned long now = millis();
  if ((now - lastPositionCalculatedTime) > STALE_POSITION_TIME) {
    lastPositionCalculatedTime = now;
    return true;
  }
  return false;
}
EQPlatform::EQPlatform() {
  eqPlatformIP = "";
  lastPositionCalculatedTime = 0;
  lastPositionReceivedTime = getNow();
  currentlyRunning = false;
  platformConnected = false;
}
