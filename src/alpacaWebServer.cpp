#include "alpacaWebServer.h"
#include "AsyncUDP.h"
#include "Logging.h"
#include "encoders.h"
#include <ArduinoJson.h> // Include the library
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WebSocketsClient.h>
#include <time.h>

unsigned const int localPort = 32227; // The Alpaca Discovery test port
unsigned const int alpacaPort =
    80; // The  port that the Alpaca API would be available on

// used to track ra/dec requests, so we do for one but not both
unsigned long lastPositionRequestTime;
#define STALE_POSITION_TIME 200

AsyncUDP alpacaUdp;
AsyncUDP eqUdp;
#define IPBROADCASTPORT 50375

double runtimeFromCenter;
bool currentlyRunning;
// used to store last time position was received from EQ
#define STALE_EQ_WARNING_THRESHOLD 10000 // used to detect packet loss
unsigned long lastPositionReceivedTimeMillis;

AsyncWebServer alpacaWebServer(80);

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
             "ErrorNumber": %d,
             "ErrorMessage": "%s",
             "Value": []
      })",
           0, "");

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleString(AsyncWebServerRequest *request, String s) {
  log("Single string value url is %s, string is %s", request->url().c_str(), s);

  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": %d,
             "ErrorMessage": "%s",
             "Value": "%s"
      })",
           0, "", s);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleInteger(AsyncWebServerRequest *request, int value) {
  log("Single int value url is %s, int is %ld", request->url().c_str(), value);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": %d,
             "ErrorMessage": "%s",
             "Value": %ld
      })",
           0, "", value);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleDouble(AsyncWebServerRequest *request, double d) {
  log("Single double value url is %s, double is %lf", request->url().c_str(),
      d);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": %d,
             "ErrorMessage": "%s",
             "Value": %lf
      })",
           0, "", d);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleBool(AsyncWebServerRequest *request, bool b) {
  log("Single bool value url is %s, bool is %d", request->url().c_str(), b);
  char buffer[300];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": %d,
             "ErrorMessage": "%s",
             "Value": %s
      })",
           0, "", b ? "true" : "false");

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

void syncToCoords(AsyncWebServerRequest *request, TelescopeModel &model) {
  String ra = request->arg("RightAscension");
  double parsedRA;
  double parsedDec;
  // TODO handle exceptions
  if (ra != NULL) {
    log("Received parameterName: %s", ra.c_str());

    parsedRA = strtod(ra.c_str(), NULL);
    log("Parsed ra value: %lf", parsedRA);
  }
  String dec = request->arg("Declination");
  if (dec != NULL) {
    log("Received parameterName: %s", ra.c_str());

    parsedDec = strtod(dec.c_str(), NULL);
    log("Parsed dec value: %lf", parsedDec);
  }

  model.setPositionRaDec(parsedRA, parsedDec);
  updatePosition(model);
  model.saveEncoderCalibrationPoint();

  returnNoError(request);
}

void setUTCDate(AsyncWebServerRequest *request, TelescopeModel &model) {
  String utc = request->arg("UTCDate");
  if (!utc.isEmpty())
    log("Received UTCDate: %s", utc.c_str());

  // int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

  // sscanf(utc.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &year, &month, &day, &hour,
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

  struct tm timeinfo;
  struct timeval tv;

  // Setting up your timeinfo structure
  timeinfo.tm_year = year - 1900; // Years since 1900
  timeinfo.tm_mon = month - 1;    // Months are 0-based
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;

  // Convert our tm structure to timeval
  tv.tv_sec =
      mktime(&timeinfo); // Converts tm struct to seconds since the Unix epoch
  tv.tv_usec = 0;        // Microseconds - can be set to 0

  // Set the time
  settimeofday(&tv, NULL);

  model.setUTCYear(year);
  model.setUTCMonth(month);
  model.setUTCDay(day);
  model.setUTCHour(hour);
  model.setUTCMinute(minute);
  model.setUTCSecond(second);
  log("Model date after set: %d /%d/%d %d:%d:%d ", model.day, model.month,
      model.year, model.hour, model.min, model.sec);
  returnNoError(request);
  log("finished utc");
}

// Calculate position.
// Triggered from ra or dec request. Should only run for one of them and then
// cache for a few millis
void updatePosition(TelescopeModel &model) {
  unsigned long nowmillis = millis();
  if ((nowmillis - lastPositionReceivedTimeMillis) > STALE_EQ_WARNING_THRESHOLD)
    log("No EQ platform, or packet loss: last packet recieved %d seconds ago "
        "at %ld",
        nowmillis - lastPositionReceivedTimeMillis, lastPositionReceivedTimeMillis);

  // if platform is running, then it has moved on since packet
  // recieved. The time since the packet received needs to be added to
  // the timeToCenter.
  // eg if time to center is 100s, and packet was received a second ago,
  // time to center should be considered as 99s.
  double interpolationTimeInSeconds = 0;
  if (currentlyRunning) {
    // log("setting interpolation time");
    interpolationTimeInSeconds =
        (nowmillis - lastPositionReceivedTimeMillis) / 1000.0;
  }
  
  // TODO interpolate platform values
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  log("Encoder Values %ld %ld", getEncoderAl(), getEncoderAz());

  // Using position from eq platform (expressed as a time delta) adjust
  // current time for calculation
  //  1. Get the current time
  struct tm timeInfo;
  time_t now; // in seconds
  time(&now);
  gmtime_r(&now, &timeInfo);
  
  // log("Base time for calc is %ld", now);
  // log("Runtime from center is %lf", runtimeFromCenter);
  // log("Interpolation time %lf", interpolationTimeInSeconds);
  // 2. Adjust the time by runtimeFromCenter
  now += (int)(runtimeFromCenter -
               interpolationTimeInSeconds); // Adjust by whole seconds

  int fractionalSecs =
      1000000 *
      (runtimeFromCenter -
       (int)runtimeFromCenter); // microseconds adjustment for fractional part
  timeval currentTimeVal;
  currentTimeVal.tv_sec = now;
  currentTimeVal.tv_usec = fractionalSecs;

  gmtime_r(&currentTimeVal.tv_sec, &timeInfo);

  // 3. Break down the time into its components
  int year =
      timeInfo.tm_year + 1900; // The tm_year is the number of years since 1900
  int month = timeInfo.tm_mon + 1; // The tm_mon range is 0-11
  int day = timeInfo.tm_mday;
  int hour = timeInfo.tm_hour;
  int min = timeInfo.tm_min;
  int sec = timeInfo.tm_sec;

  // 4. Set the time in your class
  model.setUTCYear(year);
  model.setUTCMonth(month);
  model.setUTCDay(day);
  model.setUTCHour(hour);
  model.setUTCMinute(min);
  model.setUTCSecond(sec);

  log("Model date for calcs: %02d/%02d/%02d %02d:%02d:%02d ", model.day,
      model.month, model.year, model.hour, model.min, model.sec);

  model.calculateCurrentPosition();
}

void getRA(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionRequestTime) > STALE_POSITION_TIME) {
    lastPositionRequestTime = now;
    updatePosition(model);
  }

  returnSingleDouble(request, model.getRACoord());
}

void getDec(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionRequestTime) > STALE_POSITION_TIME) {
    lastPositionRequestTime = now;
    updatePosition(model);
  }
  returnSingleDouble(request, model.getDecCoord());
}

void setupWebServer(TelescopeModel &model) {
  lastPositionRequestTime = 0;
  lastPositionReceivedTimeMillis = 0;
  currentlyRunning = false;

  // set up alpaca discovery udp
  if (alpacaUdp.listen(32227)) {
    log("Listening for alpaca discovery requests...");
    alpacaUdp.onPacket([](AsyncUDPPacket packet) {
      log("Received alpaca UDP Discovery packet ");
      if ((packet.length() >= 16) &&
          (strncmp("alpacadiscovery1", (char *)packet.data(), 16) == 0)) {
        log("Responding to alpaca UDP Discovery packet with my port number %d",
            alpacaPort);

        packet.printf("{\"AlpacaPort\": %d}", alpacaPort);
      }
    });
  }

  // set up listening for ip address from eq platform

  // Initialize UDP to listen for broadcasts.

  if (eqUdp.listen(IPBROADCASTPORT)) {
    log("Listening for eq platform broadcasts");
    eqUdp.onPacket([](AsyncUDPPacket packet) {
      unsigned long now = millis();
      String msg = packet.readString();
      log("UDP Broadcast received: %s", msg.c_str());

      // Check if the broadcast is from EQ Platform
      if (msg.startsWith("EQ:")) {
        msg = msg.substring(3);
        log("Got payload from eq plaform");

        // Create a JSON document to hold the payload
        const size_t capacity = JSON_OBJECT_SIZE(2) +
                                40; // Reserve some memory for the JSON document
        StaticJsonDocument<capacity> doc;

        // Deserialize the JSON payload
        DeserializationError error = deserializeJson(doc, msg);
        if (error) {
          log("Failed to parse payload %s", msg.c_str());
          return;
        }

        if (doc.containsKey("timeToCenter") && doc.containsKey("isTracking") &&
            doc["timeToCenter"].is<double>()) {
          runtimeFromCenter = doc["timeToCenter"];
          currentlyRunning = doc["isTracking"];

          lastPositionReceivedTimeMillis = now;
          log("Distance from center %lf, running %d, at time %ld",
              runtimeFromCenter, currentlyRunning, lastPositionReceivedTimeMillis);
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
        log("Processing GET on url %s", url.c_str());

        // Strip off the initial portion of the URL
        String subPath = url.substring(String("/api/v1/telescope/0/").length());

        if (subPath == "alignmentmode")
          return returnSingleInteger(request, 0);
        if (subPath == "aperturearea")
          return returnSingleDouble(request, 0);
        if (subPath == "aperturediameter")
          return returnSingleDouble(request, 0);

        if (subPath == "athome" || subPath == "atpark" ||
            subPath == "canfindhome" || subPath == "canpark" ||
            subPath == "canpulseguide" || subPath == "cansetdeclinationrate" ||
            subPath == "cansetguiderates" || subPath == "cansetpark" ||
            subPath == "cansetpierside" || subPath == "canmoveaxis" ||
            subPath == "cansetrightascensionrate" ||
            subPath == "cansettracking" || subPath == "canslew" ||
            subPath == "canslewaltaz" || subPath == "canslewasync" ||
            subPath == "canslewaltazasync" || subPath == "cansyncaltaz" ||
            subPath == "canunpark" || subPath == "doesrefraction" ||
            subPath == "sideofpier" || subPath == "slewing" ||
            subPath == "tracking") {
          return returnSingleBool(request, false);
        }

        if (subPath == "cansync") {
          return returnSingleBool(request, true);
        }

        if (subPath == "declinationrate" || subPath == "focallength" ||
            subPath == "siderealtime") {
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
                       //  return returnNoError(request);

                       if (subPath.startsWith("sitelatitude"))
                         //  return returnNoError(request);
                         return setSiteLatitude(request, model);

                       if (subPath.startsWith("sitelongitude"))
                         //  return returnNoError(request);
                         return setSiteLongitude(request, model);

                       if (subPath.startsWith("utcdate"))
                         // return returnNoError(request);
                         return setUTCDate(request, model);

                       // Add more routes here as needed

                       // If no match found, return a 404 or appropriate
                       // response
                       return handleNotFound(request);
                     });

  // ===============================

  alpacaWebServer.onNotFound(
      [](AsyncWebServerRequest *request) { handleNotFound(request); });
  alpacaWebServer.begin();
  log("Server started");
  return;
}
