#include "alpacaWebServer.h"
#include "AsyncUDP.h"
#include "Logging.h"
#include "TimePoint.h"
#include "encoders.h"
#include <ArduinoJson.h> // Include the library
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WebSocketsClient.h>
#include <time.h>

#define PREF_ALT_STEPS_KEY "AltStepsKey"
#define PREF_AZ_STEPS_KEY "AzStepsKey"

// TimePoint epochMillisBase;

unsigned const int localPort = 32227; // The Alpaca Discovery test port
unsigned const int alpacaPort =
    80; // The  port that the Alpaca API would be available on

// used to track ra/dec requests, so we do for one but not both
unsigned long long lastPositionCalculatedTime;
#define STALE_POSITION_TIME 200

AsyncUDP alpacaUdp;
AsyncUDP eqUdpIn;
AsyncUDP eqUDPOut;
#define IPBROADCASTPORT 50375

double runtimeFromCenterSeconds;
String eqPlatformIP;
double timeToEnd;
bool currentlyRunning;
bool platformConnected;
// used to store last time position was received from EQ
#define STALE_EQ_WARNING_THRESHOLD_SECONDS 10 // used to detect packet loss
TimePoint lastPositionReceivedTime;

AsyncWebServer alpacaWebServer(80);

void sendEQCommand(String command, double parm) {
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

void handleNotFound(AsyncWebServerRequest *request) {
  log("Not found URL is %s", request->url().c_str());
  String method;

  switch (request->method()) {
  case HTTP_GET:
    method = "GET";
    break;
  case HTTP_POST:
    method = "POST";
    break;
  case HTTP_DELETE:
    method = "DELETE";
    break;
  case HTTP_PUT:
    method = "PUT";
    break;
  case HTTP_PATCH:
    method = "PATCH";
    break;
  case HTTP_HEAD:
    method = "HEAD";
    break;
  case HTTP_OPTIONS:
    method = "OPTIONS";
    break;
  default:
    method = "UNKNOWN";
    break;
  }

  log("Not found Method is %s", method.c_str());

  request->send(404, "application/json");
}
void saveAltEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                         Preferences &prefs) {

  long alt = model.calculateAltEncoderStepsPerRevolution();
  log("Setting new value for encoder alt steps  to %ld", alt);
  model.setAltEncoderStepsPerRevolution(alt);

  prefs.putLong(PREF_ALT_STEPS_KEY, alt);
  request->send(200);
}

void saveAzEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                        Preferences &prefs) {

  long az = model.calculateAzEncoderStepsPerRevolution();
  log("Setting new value for encoder az steps to %ld", az);
  model.setAzEncoderStepsPerRevolution(az);

  prefs.putLong(PREF_AZ_STEPS_KEY, az);
  request->send(200);
}

void loadPreferences(Preferences &prefs, TelescopeModel &model) {
  model.setAltEncoderStepsPerRevolution(
      prefs.getLong(PREF_ALT_STEPS_KEY, -30000));

  model.setAzEncoderStepsPerRevolution(
      prefs.getLong(PREF_AZ_STEPS_KEY, 108531));
}

void clearAlignment(AsyncWebServerRequest *request, TelescopeModel &model) {
  log("Clearing alignment");
  model.clearAlignment();
  request->send(200);
}
// safety: clears encoder steps
void clearPrefs(AsyncWebServerRequest *request, Preferences &prefs,
                TelescopeModel &model) {

  prefs.remove(PREF_ALT_STEPS_KEY);
  prefs.remove(PREF_AZ_STEPS_KEY);

  loadPreferences(prefs, model);
  request->send(200);
}
void getScopeStatus(AsyncWebServerRequest *request, TelescopeModel &model) {
  // log("/getStatus");

  TimePoint now = getNow();
  if (differenceInSeconds(lastPositionReceivedTime, now) >
      STALE_EQ_WARNING_THRESHOLD_SECONDS) {
    platformConnected = false;
  } else {
    platformConnected = true;
  }
  char buffer[2000];
  sprintf(buffer,
          R"({
    
    "calculateAltEncoderStepsPerRevolution" : %ld,
    "calculateAzEncoderStepsPerRevolution" : %ld,
    "actualAltEncoderStepsPerRevolution" : %ld,
    "actualAzEncoderStepsPerRevolution" : %ld,
    "lastSyncedRa" : %lf,
    "lastSyncedDec" : %lf,
    "lastSyncedAlt" : %lf,
    "lastSyncedAz" : %lf,
    "eqPlatformIP" : "%s",
    "platformTracking" : %s,
    "timeToMiddle" : %.1lf,
    "timeToEnd" : %.1lf,
    
    "platformConnected" : %s
})",

          model.calculateAltEncoderStepsPerRevolution(),
          model.calculateAzEncoderStepsPerRevolution(),
          model.getAltEncoderStepsPerRevolution(),
          model.getAzEncoderStepsPerRevolution(),
          model.lastSyncPoint.eqCoord.getRAInDegrees(),
          model.lastSyncPoint.eqCoord.getDecInDegrees(),
          model.lastSyncPoint.horizCoord.altInDegrees,
          model.lastSyncPoint.horizCoord.aziInDegrees,

          eqPlatformIP.c_str(), currentlyRunning ? "true" : "false",
          runtimeFromCenterSeconds / 60, timeToEnd / 60,
          platformConnected ? "true" : "false");

  String json = buffer;

  // String json = "{\"timestamp\":" + String(times.get()) + ",\"position\":"
  // + String(positions.get()) + ",\"velocity\":" + String(velocities.get()) +
  // "}";
  request->send(200, "application/json", json);
}

void returnDeviceDescription(AsyncWebServerRequest *request) {
  log("returnDeviceDescription url is  %s", request->url().c_str());

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
         "Value": {
          "ServerName": "myserver",
          "Manufacturer": "me",
          "ManufacturerVersion": "1",
          "Location": "here"
          },
        "ClientTransactionID": 0,
         "ServerTransactionID": 0
        })");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnConfiguredDevices(AsyncWebServerRequest *request) {
  log("returnConfiguredDevices url is  %s", request->url().c_str());

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
            "Value": [ {
          "DeviceName": "Frankendob",
          "DeviceType": "Telescope",
         "DeviceNumber": 0,
          "UniqueID": "booboo"
          }],
        "ClientTransactionID": 0,
         "ServerTransactionID": 0
        })");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnAxisRates(AsyncWebServerRequest *request) {
  log("Return Axis rates url is  %s", request->url().c_str());
  // Client passed an "Axis" here.
  // TODO check for bad axis
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ClientTransactionID": 0,
            "ServerTransactionID": 0,
           "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": [
               {
               "Maximum": 1,
                "Minimum": 0
              }
             ]
      })");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnApiVersions(AsyncWebServerRequest *request) {
  log("returnApiVersions url is  %s", request->url().c_str());

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
         "Value": [1],
        "ClientTransactionID": 0,
         "ServerTransactionID": 0
        })");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnEmptyArray(AsyncWebServerRequest *request) {
  log("Empty array value url is %s", request->url().c_str());

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": []
      })");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleString(AsyncWebServerRequest *request, String s) {
  log("Single string value url is %s, string is %s", request->url().c_str(), s);

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": "%s"
      })",
           s);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleInteger(AsyncWebServerRequest *request, int value) {
  log("Single int value url is %s, int is %ld", request->url().c_str(), value);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %ld
      })",
           value);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleDouble(AsyncWebServerRequest *request, double d) {
  // log("Single double value url is %s, double is %lf", request->url().c_str(),
  //     d);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %lf
      })",
           d);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleBool(AsyncWebServerRequest *request, bool b) {
  // log("Single bool value url is %s, bool is %d", request->url().c_str(), b);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %s
      })",
           b ? "true" : "false");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnNoError(AsyncWebServerRequest *request) {

  // log("Returning no error for url %s ", request->url().c_str());

  static const char *json = R"({
           "ClientTransactionID": 0,
           "ServerTransactionID": 0,
           "ErrorNumber": 0,
           "ErrorMessage": ""
          })";
  request->send(200, "application/json", json);
}

void setSiteLatitude(AsyncWebServerRequest *request, TelescopeModel &model) {
  String lat = request->arg("SiteLatitude");
  if (lat != NULL) {
    log("Received parameterName: %s", lat.c_str());

    double parsedValue = strtod(lat.c_str(), NULL);
    log("Parsed lat value: %lf", parsedValue);
    model.setLatitude(parsedValue);
  }
  return returnNoError(request);
}

void setSiteLongitude(AsyncWebServerRequest *request, TelescopeModel &model) {
  String lng = request->arg("SiteLongitude");
  if (lng != NULL) {
    log("Received parameterName: %s", lng.c_str());

    double parsedValue = strtod(lng.c_str(), NULL);
    log("Parsed lng value: %lf", parsedValue);
    model.setLongitude(parsedValue);
    log("Long set");
  }
  return returnNoError(request);
}
void canMoveAxis(AsyncWebServerRequest *request) {
  String axis = request->arg("Axis");
  if (axis != NULL) {
    if (axis == "0") {
      log("CanMoveAxis0=true");
      return returnSingleBool(request, true);
    }
  }
  log("CanMoveAxis (other)=false");
  return returnSingleBool(request, false);
}

void moveAxis(AsyncWebServerRequest *request) {
  String rate = request->arg("Rate");
  double parsedRate;
  double parsedAxis;
  if (rate != NULL) {
    log("Received parameterName: %s", rate.c_str());
    parsedRate = strtod(rate.c_str(), NULL);
    log("Parsed rate value: %lf", parsedRate);
  }
  String axis = request->arg("Axis");
  if (axis != NULL) {
    log("Received parameterName: %s", axis.c_str());
    parsedAxis = strtod(axis.c_str(), NULL);
    log("Parsed rate value: %lf", parsedAxis);
  }

  if (axis != 0) {
    log("Error: can only move on one axis");
    // TODO make this return an error
    return returnNoError(request);
  }

  sendEQCommand("moveaxis", parsedRate);

  return returnNoError(request);
}
void setTracking(AsyncWebServerRequest *request) {
  String trackingStr = request->arg("Tracking");
  if (trackingStr != NULL) {
    log("Received parameterName: %s", trackingStr.c_str());

    int tracking = trackingStr == "True" ? 1 : 0;
    log("Parsed tracking value: %d", tracking);
    sendEQCommand("track", tracking);
  } else {
    log("No Tracking parm found");
  }
  return returnNoError(request);
}

void setUTCDate(AsyncWebServerRequest *request, TelescopeModel &model) {
  String utc = request->arg("UTCDate");
  if (!utc.isEmpty())
    log("Received UTCDate: %s", utc.c_str());

  // int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

  // sscanf(utc.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &year, &month, &day,
  // &hour,
  //        &minute, &second);

  int year = utc.substring(0, 4).toInt();
  int month = utc.substring(5, 7).toInt();
  int day = utc.substring(8, 10).toInt();
  int hour = utc.substring(11, 13).toInt();
  int minute = utc.substring(14, 16).toInt();
  int second = utc.substring(17, 19).toInt();

  log("Parsed Date - Year: %d, Month: %d, Day: %d, Hour: %d, Minute: %d, "
      "Second: %d",
      year, month, day, hour, minute, second);
  TimePoint tp = createTimePoint(day, month, year, hour, minute, second);
  log("Setting system data to %s", timePointToString(tp).c_str());
  setSystemTime(tp);
  // epochMillisBase = createTimePoint(day, month, year, hour, minute,
  // second);

  // log("Stored base epoch time in millis: %llu", epochMillisBase);
  // epochMillisBase = addMillisToTime(epochMillisBase,millis());// we'll add
  // millis every time we need epoch time

  // log("Stored base epoch time in millis after offset : %llu",
  // epochMillisBase);
  returnNoError(request);
  log("finished utc");
}

/**
 * Returns an time (epoch in millis) to be used for model calculations.
 * This is obstensibly the time that the platform is
 * as the middle of the run, it can be in the past or in the future.
 * Checks to see if a packet arrived from eq platform recently
 *  If not mark platform as not connected.
 *
 */
TimePoint calculateAdjustedTime() {
  TimePoint now = getNow();
  log("Calculating  adjusted time from (now): %s",
      timePointToString(now).c_str());
  // unsigned long now = millis();
  // log("Now millis since start: %ld", now);
  if (differenceInSeconds(lastPositionReceivedTime, now) >
      STALE_EQ_WARNING_THRESHOLD_SECONDS) {
    log("No EQ platform, or packet loss: last packet recieved  "
        "at %s",
        timePointToString(lastPositionReceivedTime).c_str());
    platformConnected = false;
  } else {
    platformConnected = true;
  }
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

// Calculate position.
// Triggered from ra or dec request. Should only run for one of them and
// then cache for a few millis
void updatePosition(TelescopeModel &model) {

  TimePoint timeAtMiddleOfRun = calculateAdjustedTime();
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  model.calculateCurrentPosition(timeAtMiddleOfRun);
}

void syncToCoords(AsyncWebServerRequest *request, TelescopeModel &model) {
  String ra = request->arg("RightAscension");
  double parsedRAHours;
  double parsedDecDegrees;
  // TODO handle exceptions
  if (ra != NULL) {
    log("Received parameterName: %s", ra.c_str());

    parsedRAHours = strtod(ra.c_str(), NULL);
    log("Parsed ra value: %lf", parsedRAHours);
  } else {
    log("Could not parse ra arg!");
  }
  String dec = request->arg("Declination");
  if (dec != NULL) {
    log("Received parameterName: %s", ra.c_str());

    parsedDecDegrees = strtod(dec.c_str(), NULL);
    log("Parsed dec value: %lf", parsedDecDegrees);
  } else {
    log("Could not parse dec arg!");
  }

  TimePoint timeAtMiddleOfRun = calculateAdjustedTime();
  log("Encoder values: %ld,%ld", getEncoderAl(), getEncoderAz());
  // log("Timestamp for middle of run: %llu", timeAtMiddleOfRunSeconds);
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  model.syncPositionRaDec(parsedRAHours, parsedDecDegrees, timeAtMiddleOfRun);
  updatePosition(model);
  // model.saveEncoderCalibrationPoint();

  returnNoError(request);
}
void getRA(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionCalculatedTime) > STALE_POSITION_TIME) {
    lastPositionCalculatedTime = now;
    updatePosition(model);
  }

  returnSingleDouble(request, model.getRACoord());
}

void getDec(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionCalculatedTime) > STALE_POSITION_TIME) {
    lastPositionCalculatedTime = now;
    updatePosition(model);
  }

  returnSingleDouble(request, model.getDecCoord());
}

void setupWebServer(TelescopeModel &model, Preferences &prefs) {
  loadPreferences(prefs, model);

  eqPlatformIP = "";
  lastPositionCalculatedTime = 0;
  lastPositionReceivedTime = getNow();
  currentlyRunning = false;
  platformConnected = false;

  // set up alpaca discovery udp
  if (alpacaUdp.listen(32227)) {
    log("Listening for alpaca discovery requests...");
    alpacaUdp.onPacket([](AsyncUDPPacket packet) {
      log("Received alpaca UDP Discovery packet ");
      if ((packet.length() >= 16) &&
          (strncmp("alpacadiscovery1", (char *)packet.data(), 16) == 0)) {
        log("Responding to alpaca UDP Discovery packet with my port number "
            "%d",
            alpacaPort);

        packet.printf("{\"AlpacaPort\": %d}", alpacaPort);
      }
    });
  }

  // set up listening for ip address from eq platform

  // Initialize UDP to listen for broadcasts.

  if (eqUdpIn.listen(IPBROADCASTPORT)) {
    log("Listening for eq platform broadcasts");
    eqUdpIn.onPacket([](AsyncUDPPacket packet) {
      unsigned long now = millis();
      String msg = packet.readString();
      log("UDP Broadcast received: %s", msg.c_str());

      // Check if the broadcast is from EQ Platform
      if (msg.startsWith("EQ:")) {
        msg = msg.substring(3);
        log("Got payload from eq plaform");

        // Create a JSON document to hold the payload
        const size_t capacity = JSON_OBJECT_SIZE(5) +
                                40; // Reserve some memory for the JSON document
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
    });
  }

  // GETS. Mostly default flags.
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/.*$", HTTP_GET,
      [&model](AsyncWebServerRequest *request) {
        String url = request->url();
        // log("Processing GET on url %s", url.c_str());

        // Strip off the initial portion of the URL
        String subPath = url.substring(String("/api/v1/telescope/0/").length());

        if (subPath == "alignmentmode")
          return returnSingleInteger(request, 0);
        if (subPath == "aperturearea")
          return returnSingleDouble(request, 0);
        if (subPath == "aperturediameter")
          return returnSingleDouble(request, 0);

        if (subPath == "athome" || subPath == "atpark" ||
            subPath == "cansetdeclinationrate" || subPath == "cansetpark" ||
            subPath == "cansetpierside" ||
            subPath == "cansetrightascensionrate" || subPath == "canslew" ||
            subPath == "canslewaltaz" || subPath == "canslewasync" ||
            subPath == "canslewaltazasync" || subPath == "cansyncaltaz" ||
            subPath == "canunpark" || subPath == "doesrefraction" ||
            subPath == "sideofpier" || subPath == "slewing") {
          return returnSingleBool(request, false);
        }
        if (subPath == "canmoveaxis") {
          return canMoveAxis(request);
        }
        if (subPath == "axisrates") {
          return returnAxisRates(request);
        }
        if (subPath == "cansettracking") {
          return returnSingleBool(request, true);
        }

        if (subPath == "canpark") {
          return returnSingleBool(request, true);
        }

        if (subPath == "canfindhome") {
          return returnSingleBool(request, true);
        }

        // TODO implement
        if (subPath == "canpulseguide") {
          return returnSingleBool(request, false);
        }
        if (subPath == "cansetguiderates") {
          return returnSingleBool(request, false);
        }
        if (subPath == "ispulseguiding") {
          return returnSingleBool(request, false);
        }

        if (subPath == "tracking") {
          return returnSingleBool(request, currentlyRunning);
        }

        if (subPath == "cansync") {
          return returnSingleBool(request, true);
        }

        if (subPath == "declinationrate" || subPath == "focallength" ||
            subPath == "siderealtime") {
          return returnSingleDouble(request, 0);
        }

        // TODO add sidereal rate when tracking
        if (subPath == "rightascensionrate") {
          return returnSingleDouble(request, 0);
        }

        if (subPath == "equatorialsystem") {
          return returnSingleInteger(request, 1);
        }

        if (subPath == "interfaceversion") {
          return returnSingleInteger(request, 3);
        }

        if (subPath == "driverversion" || subPath == "driverinfo" ||
            subPath == "description" || subPath == "name") {
          String responseValue = "Frankendob"; // Default
          if (subPath == "driverversion")
            responseValue = "1.0";
          else if (subPath == "driverinfo")
            responseValue = "Hackypoo";
          return returnSingleString(request, responseValue);
        }

        if (subPath == "supportedactions") {
          return returnEmptyArray(request);
        }

        if (subPath == "connected") {
          return returnSingleBool(request, true);
        }

        if (subPath == "utcdate")
          return returnSingleString(request, "");

        if (subPath == "siteelevation")
          return returnSingleDouble(request, 0);

        if (subPath == "sitelatitude")
          return returnSingleDouble(request, 0);

        if (subPath == "sitelongitude")
          return returnSingleDouble(request, 0);

        if (subPath == "azimuth")
          return returnSingleDouble(request, 0);

        if (subPath == "altitude")
          return returnSingleDouble(request, 0);

        if (subPath == "declination")
          return getDec(request, model);

        if (subPath == "rightascension")
          return getRA(request, model);

        return handleNotFound(request);
      });

  // Mangement API ================
  alpacaWebServer.on(
      "^\\/management\\/.*$", HTTP_GET, [](AsyncWebServerRequest *request) {
        String url = request->url().c_str();
        log("Processing GET on management url %s", url.c_str());
        // Strip off the initial portion of the URL
        String subPath = url.substring(String("/management/").length());

        if (subPath == "apiversions")
          return returnApiVersions(request);

        if (subPath == "v1/configureddevices")
          return returnConfiguredDevices(request);

        if (subPath == "v1/description")
          return returnDeviceDescription(request);

        return handleNotFound(request);
      });

  // ======================================
  //  PUTS implementation

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/.*$", HTTP_PUT,
                     [&model](AsyncWebServerRequest *request) {
                       String url = request->url();
                       log("Processing PUT on url %s", url.c_str());
                       // Strip off the initial portion of the URL
                       String subPath = url.substring(
                           String("/api/v1/telescope/0/").length());

                       if (subPath.startsWith("connected"))
                         return returnNoError(request);

                       if (subPath.startsWith("synctocoordinates"))
                         return syncToCoords(request, model);

                       if (subPath.startsWith("sitelatitude"))
                         return setSiteLatitude(request, model);

                       if (subPath.startsWith("sitelongitude"))
                         return setSiteLongitude(request, model);

                       if (subPath.startsWith("utcdate"))
                         return setUTCDate(request, model);

                       if (subPath.startsWith("tracking"))
                         return setTracking(request);

                       if (subPath.startsWith("park"))
                         return sendEQCommand("park", 0);

                       if (subPath.startsWith("findhome"))
                         return sendEQCommand("home", 0);

                       if (subPath.startsWith("moveaxis"))
                         return moveAxis(request);
                       // Add more routes here as needed

                       // If no match found, return a 404 or appropriate
                       // response
                       return handleNotFound(request);
                     });

  // ===============================

  alpacaWebServer.on("/getScopeStatus", HTTP_GET,
                     [&model](AsyncWebServerRequest *request) {
                       getScopeStatus(request, model);
                     });

  alpacaWebServer.on("/saveAltEncoderSteps", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       saveAltEncoderSteps(request, model, prefs);
                     });

  alpacaWebServer.on("/saveAzEncoderSteps", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       saveAzEncoderSteps(request, model, prefs);
                     });

  alpacaWebServer.on("/clearAlignment", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       clearAlignment(request, model);
                     });

  alpacaWebServer.on("/clearPrefs", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       clearPrefs(request, prefs, model);
                     });
  alpacaWebServer.serveStatic("/", LittleFS, "/fs/");

  alpacaWebServer.onNotFound(
      [](AsyncWebServerRequest *request) { handleNotFound(request); });

  alpacaWebServer.begin();
  log("Server started");
  return;
}
