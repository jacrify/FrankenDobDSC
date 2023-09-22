#include "AlpacaWebServer.h"
#include "AlpacaDiscovery.h"
#include "AsyncUDP.h"
#include "Encoders.h"
#include "Logging.h"
#include "TimePoint.h"
#include <ArduinoJson.h> // Include the library
#include <ESPAsyncWebServer.h>

#include <WebSocketsClient.h>
#include <time.h>

#include "AlpacaGeneric.h"
#include "AlpacaManagement.h"
#include "WebUI.h"

#define WEBSERVER_PORT 80

// used to store last time position was received from EQ

AsyncWebServer alpacaWebServer(WEBSERVER_PORT);

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

void moveAxis(AsyncWebServerRequest *request, EQPlatform &platform) {
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

  platform.sendEQCommand("moveaxis", parsedRate);

  return returnNoError(request);
}
void setTracking(AsyncWebServerRequest *request, EQPlatform &platform) {
  String trackingStr = request->arg("Tracking");
  if (trackingStr != NULL) {
    log("Received parameterName: %s", trackingStr.c_str());

    int tracking = trackingStr == "True" ? 1 : 0;
    log("Parsed tracking value: %d", tracking);
    platform.sendEQCommand("track", tracking);
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

// Calculate position.
// Triggered from ra or dec request. Should only run for one of them and
// then cache for a few millis
void updatePosition(TelescopeModel &model, EQPlatform &platform) {

  TimePoint timeAtMiddleOfRun = platform.calculateAdjustedTime();
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  model.calculateCurrentPosition(timeAtMiddleOfRun);
}

void syncToCoords(AsyncWebServerRequest *request, TelescopeModel &model,
                  EQPlatform &platform) {
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

  TimePoint timeAtMiddleOfRun = platform.calculateAdjustedTime();
  log("Encoder values: %ld,%ld", getEncoderAl(), getEncoderAz());
  // log("Timestamp for middle of run: %llu", timeAtMiddleOfRunSeconds);
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  model.syncPositionRaDec(parsedRAHours, parsedDecDegrees, timeAtMiddleOfRun);
  updatePosition(model, platform);
  // model.saveEncoderCalibrationPoint();

  returnNoError(request);
}
void getRA(AsyncWebServerRequest *request, TelescopeModel &model,
           EQPlatform &platform) {
  if (platform.checkStalePositionAndUpdate()) {
    updatePosition(model, platform);
  }

  returnSingleDouble(request, model.getRACoord());
}

void getDec(AsyncWebServerRequest *request, TelescopeModel &model,
            EQPlatform &platform) {
  if (platform.checkStalePositionAndUpdate()) {
    updatePosition(model, platform);
  }

  returnSingleDouble(request, model.getDecCoord());
}

void setupWebServer(TelescopeModel &model, Preferences &prefs,
                    EQPlatform &platform) {
  

  // GETS. Mostly default flags.
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/.*$", HTTP_GET,
      [&model, &platform](AsyncWebServerRequest *request) {
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
          return returnSingleBool(request, platform.currentlyRunning);
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
          return getDec(request, model, platform);

        if (subPath == "rightascension")
          return getRA(request, model, platform);

        return handleNotFound(request);
      });

  // ======================================
  //  PUTS implementation

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/.*$", HTTP_PUT,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       String url = request->url();
                       log("Processing PUT on url %s", url.c_str());
                       // Strip off the initial portion of the URL
                       String subPath = url.substring(
                           String("/api/v1/telescope/0/").length());

                       if (subPath.startsWith("connected"))
                         return returnNoError(request);

                       if (subPath.startsWith("synctocoordinates"))
                         return syncToCoords(request, model, platform);

                       if (subPath.startsWith("sitelatitude"))
                         return setSiteLatitude(request, model);

                       if (subPath.startsWith("sitelongitude"))
                         return setSiteLongitude(request, model);

                       if (subPath.startsWith("utcdate"))
                         return setUTCDate(request, model);

                       if (subPath.startsWith("tracking"))
                         return setTracking(request, platform);

                       if (subPath.startsWith("park"))
                         return platform.sendEQCommand("park", 0);

                       if (subPath.startsWith("findhome"))
                         return platform.sendEQCommand("home", 0);

                       if (subPath.startsWith("moveaxis"))
                         return moveAxis(request, platform);
                       // Add more routes here as needed

                       // If no match found, return a 404 or appropriate
                       // response
                       return handleNotFound(request);
                     });

  // ===============================

  setupAlpacaManagment(alpacaWebServer);
  setupWebUI(alpacaWebServer, model, platform, prefs);

  

  alpacaWebServer.onNotFound(
      [](AsyncWebServerRequest *request) { handleNotFound(request); });

  alpacaWebServer.begin();
  setupAlpacaDiscovery(WEBSERVER_PORT);
  log("Server started");
  return;
}
