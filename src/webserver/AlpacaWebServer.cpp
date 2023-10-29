#include "AlpacaWebServer.h"
#include "AlpacaDiscovery.h"
#include "AsyncUDP.h"
#include "Encoders.h"
#include "Logging.h"
#include "TimePoint.h"
#include <ArduinoJson.h> // Include the library
#include <ESPAsyncWebServer.h>
// #include <WebSerial.h>
#include <WebSocketsClient.h>
#include <time.h>

#include "AlpacaGeneric.h"
#include "AlpacaManagement.h"
#include "WebUI.h"

#define WEBSERVER_PORT 80
const int BUFFER_SIZE = 300;
#define STALE_POSITION_TIME 200
// used to track ra/dec requests, so we do for one but not both
unsigned long long lastPositionCalculatedTime;

// used to store last time position was received from EQ

AsyncWebServer alpacaWebServer(WEBSERVER_PORT);
/**
 * Checks when last position was calculated. Assume position will be calced
 * after this is called.
 */
bool checkStalePositionAndUpdate() {
  unsigned long now = millis();
  if ((now - lastPositionCalculatedTime) > STALE_POSITION_TIME) {
    lastPositionCalculatedTime = now;
    return true;
  }
  return false;
}

/**
 * Returns the rates of the various axis.
 * We only have one axis.
 */
void returnAxisRates(AsyncWebServerRequest *request, EQPlatform &platform) {
  log("Return Axis rates url is  %s", request->url().c_str());
  // Client passed an "Axis" here.
  // Sharpcap doesn't handle multiple axis speeds well
  // so we say that both axis move the same
  int parsedAxis = 0;
  String axis = request->arg("Axis");
  if (axis != NULL) {
    // log("Received axis: %s", axis.c_str());
    parsedAxis = strtol(axis.c_str(), NULL, 10);
    // log("Parsed rate value: %ld", parsedAxis);
  }
  char buffer[BUFFER_SIZE];
  // TODO fix this later
  // double axisRateMax = platform.axisMoveRateMax;
  double axisRateMax = 21;

  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": [
               {
               "Maximum": %lf,
                "Minimum": 0
              }
             ],
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           axisRateMax, getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}
void returnTrackingRates(AsyncWebServerRequest *request) {
  log("Return tracking rates url is  %s", request->url().c_str());
  // TODO #4 implement lunar tracking
  /**
   * DriveRate object corresponding to one of the standard drive rates
   * driveSidereal = 0 - Sidereal tracking rate (15.041 arcseconds per second).
   * driveLunar = 1 - Lunar tracking rate (14.685 arcseconds per second).
   * driveSolar = 2 - Solar tracking rate (15.0 arcseconds per second).
   * driveKing = 3 - King tracking rate (15.0369 arcseconds per second).
   */
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
           "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": [ 0 , 1 ],
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}

/** Parse and set longitude passed as a double*/
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
/** Parse and set latitude passed as a double*/
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
/** Alpaca queries axis by axis to see if the can move
 * Ugly implemention to say just one axis moves.
 */
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

void abortSlew(AsyncWebServerRequest *request, EQPlatform &platform) {
  platform.moveAxis(0.0);
  return returnNoError(request);
}
/** Parse movement rate (degrees sec) and ask plaform to move*/
void moveAxis(AsyncWebServerRequest *request, EQPlatform &platform) {
  String rate = request->arg("Rate");
  double parsedRate;
  double parsedAxis;
  if (rate != NULL) {
    // log("Received rate: %s", rate.c_str());
    parsedRate = strtod(rate.c_str(), NULL);
    // log("Parsed rate value: %lf", parsedRate);
  }
  String axis = request->arg("Axis");
  if (axis != NULL) {
    // log("Received axis: %s", axis.c_str());
    parsedAxis = strtod(axis.c_str(), NULL);
    // log("Parsed rate value: %lf", parsedAxis);
  }

  if (axis != "0") {
    log("Error: can only move on one axis");
    // TODO make this return an error
    return returnNoError(request);
  }

  platform.moveAxis(parsedRate);

  return returnNoError(request);
}

void pulseGuide(AsyncWebServerRequest *request, EQPlatform &platform) {
  String direction = request->arg("Direction");
  int parsedDirection;
  long parsedDuration;
  if (direction != NULL) {
    log("Received parameterName: %s", direction.c_str());
    parsedDirection = strtol(direction.c_str(), NULL, 10);
    log("Parsed directino value: %lf", parsedDirection);
  }
  String duration = request->arg("Duration");
  if (duration != NULL) {
    log("Received parameterName: %s", duration.c_str());
    parsedDuration = strtol(duration.c_str(), NULL, 10);
    log("Parsed rate value: %lf", parsedDuration);
  }
  log("Pulse guiding in direction %d for %d millis", parsedDirection,
      parsedDuration);
  platform.pulseGuide(parsedDirection, parsedDuration);

  return returnNoError(request);
}

/**
 *  Turn tracking on and off
 *
 */
void setTrackingRate(AsyncWebServerRequest *request, EQPlatform &platform) {
  String trackingRateStr = request->arg("TrackingRate");
  if (trackingRateStr != NULL) {
    log("Received trackingRateStr: %s", trackingRateStr.c_str());
    int trackinRate = strtol(trackingRateStr.c_str(), NULL, 10);
    // TODO implement rating rate on platform
  } else {
    log("No Tracking parm found");
  }
  return returnNoError(request);
}
/**
 *  Turn tracking on and off
 *
 */
void setTracking(AsyncWebServerRequest *request, EQPlatform &platform) {
  String trackingStr = request->arg("Tracking");
  if (trackingStr != NULL) {
    log("Received trackingStr: %s", trackingStr.c_str());
    int tracking = (trackingStr == "True") ? 1 : 0;
    // log("Parsed tracking value: %d", tracking);
    platform.setTracking(tracking);
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
/**
 * Take ra dec passed by client, and set current ra/dec to this.
 * Lots of fancy logic inside model for this one.
 */
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

void slewToCoords(AsyncWebServerRequest *request, TelescopeModel &model,
                  EQPlatform &platform) {
  log("Slewing to coords requested");
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

  // get latest position
  updatePosition(model, platform);

  double targetRADegrees = parsedRAHours * 15.0;
  double modelledRADegrees = model.currentEqPosition.getRAInDegrees();

  // negative degree shifts move from limit(east)
  // to 0 (west).
  // ra increases as stars are more west
  // so if target is west of actual,
  //   modelledRADegrees -targetRADegreeswill be positive
  // so this is right.
  double raDeltaDegrees = modelledRADegrees- targetRADegrees  ;
  platform.slewByDegrees(raDeltaDegrees);
  log("Asked platform to slew by %lf degrees: target is %lf, actual eq is %lf",raDeltaDegrees,targetRADegrees,modelledRADegrees);

  returnNoError(request);
}

/**
 * The whole point. Return ra/dec back to client
 */
void getRA(AsyncWebServerRequest *request, TelescopeModel &model,
           EQPlatform &platform) {
  if (checkStalePositionAndUpdate()) {
    updatePosition(model, platform);
  }

  returnSingleDouble(request, model.getRACoord());
}

/**
 * The whole point. Return ra/dec back to client
 */
void getDec(AsyncWebServerRequest *request, TelescopeModel &model,
            EQPlatform &platform) {
  if (checkStalePositionAndUpdate()) {
    updatePosition(model, platform);
  }

  returnSingleDouble(request, model.getDecCoord());
}
/**
 * Map all the paths.
 */
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
            subPath == "cansetrightascensionrate" ||
            subPath == "cansyncaltaz" || subPath == "canunpark" ||
            subPath == "doesrefraction") {
          return returnSingleBool(request, false);
        }

        if (subPath == "slewing") {
          return returnSingleBool(request, platform.slewing);
        }
        if (subPath == "sideofpier") {
          return returnSingleInteger(request, -1);
        }
        if (subPath == "canmoveaxis") {
          return canMoveAxis(request);
        }
        if (subPath == "axisrates") {
          return returnAxisRates(request, platform);
        }
        if (subPath == "cansettracking") {
          return returnSingleBool(request, true);
        }

        if (subPath == "canslew") {
          return returnSingleBool(request, false);
        }
        if (subPath == "canslewaltaz") {
          return returnSingleBool(request, false);
        }
        if (subPath == "canslewasync") {
          return returnSingleBool(request, true);
        }
        // TODO expose as config
        if (subPath == "slewsettletime") {
          return returnSingleInteger(request, 1);
        }
        if (subPath == "canslewaltazasync") {
          return returnSingleBool(request, false);
        }
        if (subPath == "canpark") {
          return returnSingleBool(request, true);
        }

        if (subPath == "canfindhome") {
          return returnSingleBool(request, true);
        }

        if (subPath == "canpulseguide") {
          return returnSingleBool(request, true);
        }
        // TODO implement
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

        if (subPath == "declinationrate" || subPath == "guideratedeclination" ||
            subPath == "focallength" || subPath == "siderealtime") {
          return returnSingleDouble(request, 0);
        }

        if (subPath == "trackingrate") {
          return returnTrackingRates(request);
        }

        if (subPath == "rightascensionrate") {
          return returnSingleDouble(request, platform.trackingRate);
        }
        if (subPath == "guideraterightascension") {
          return returnSingleDouble(request, platform.pulseGuideRate);
        }

        if (subPath == "equatorialsystem") {
          return returnSingleInteger(
              request,
              1); // equTopocentric
                  // https://ascom-standards.org/Help/Developer/html/T_ASCOM_DeviceInterface_EquatorialCoordinateType.htm
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

                       if (subPath.startsWith("trackingrate"))
                         return setTrackingRate(request, platform);

                       if (subPath.startsWith("tracking"))
                         return setTracking(request, platform);

                       if (subPath.startsWith("park"))
                         return platform.park();

                       if (subPath.startsWith("findhome"))
                         return platform.findHome();

                       if (subPath.startsWith("moveaxis"))
                         return moveAxis(request, platform);

                       if (subPath.startsWith("pulseguide"))
                         return pulseGuide(request, platform);

                       if (subPath.startsWith("abortslew"))
                         return abortSlew(request, platform);

                       if (subPath.startsWith("slewtocoordinatesasync"))
                         return slewToCoords(request, model, platform);

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

  // // WebSerial is accessible at "<IP Address>/webserial" in browser
  // WebSerial.begin(&alpacaWebServer);
  // setWebSerialReady();

  lastPositionCalculatedTime = 0;
  setupAlpacaDiscovery(WEBSERVER_PORT);

  log("Server started");
  return;
}
