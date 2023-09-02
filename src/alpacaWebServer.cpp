#include "Logging.h"

#include "AsyncUDP.h"
#include "alpacaWebServer.h"
#include <ArduinoJson.h> // Include the library
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WebSocketsClient.h>
#include "encoders.h"

#define IPBROADCASTPORT 50375         // EQ platform broadcasts its ip on this
#define EQ_PLATFORM_WEBSOCKET_PORT 80 // eq platform listens on this

unsigned const int localPort = 32227; // The Alpaca Discovery test port
unsigned const int alpacaPort =
    80; // The  port that the Alpaca API would be available on

IPAddress eqPlatformIP;
bool wsConnected;
AsyncUDP udp;
WebSocketsClient webSocket;

long runtimeFromCenter;
bool currentlyRunning;

AsyncWebServer alpacaWebServer(80);

// handles return response from eq platform. Stores responses.
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    log("[WSc] Disconnected!\n");
    wsConnected = false;
    break;
  case WStype_CONNECTED:
    log("[WSc] Connected to URL: %s\n", payload);
    wsConnected = true;
    break;
  case WStype_TEXT: {
    String msg = String((char *)payload);

    // Create a JSON document to hold the payload
    const size_t capacity =
        JSON_OBJECT_SIZE(2) + 40; // Reserve some memory for the JSON document
    StaticJsonDocument<capacity> doc;

    // Deserialize the JSON payload
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      log("Failed to parse payload %s", msg);
      return;
    }

    runtimeFromCenter = doc["runtimeFromCenter"];
    currentlyRunning = doc["currentlyRunning"];
    log("Got payload from eq plaform. Distance from center %lf, running %d",
        runtimeFromCenter, currentlyRunning);

    break;
    // Handle other types as needed...
  }
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

void returnDeviceDescription(AsyncWebServerRequest *request) {
  log("returnDeviceDescription url is  %s", request->url().c_str());

  char buffer[300];
  sprintf(buffer,
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
  sprintf(buffer,
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
  sprintf(buffer,
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
  sprintf(buffer,
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
  sprintf(buffer,
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
  sprintf(buffer,
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
  log("Single double value url is %s, int is %lf", request->url().c_str(), d);
  char buffer[300];
  sprintf(buffer,
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
  sprintf(buffer,
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

  log("Returning no error for url %s with method %s", request->url().c_str(),
      method.c_str());

  char buffer[300];
  sprintf(buffer,
          R"({
           "ClientTransactionID": 0,
           "ServerTransactionID": 0,
           "ErrorNumber": 0,
          "ErrorMessage": ""
          })",
          0);

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
  returnNoError(request);
}

void setSiteLongitude(AsyncWebServerRequest *request, TelescopeModel &model) {
  String lng = request->arg("SiteLongitude");
  if (lng != NULL) {
    log("Received parameterName: %s", lng.c_str());

    double parsedValue = strtod(lng.c_str(), NULL);
    log("Parsed lng value: %lf", parsedValue);
    model.setLongitude(parsedValue);
  }
  returnNoError(request);
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
  returnNoError(request);
}

void setUTCDate(AsyncWebServerRequest *request, TelescopeModel &model) {
  String utc = request->arg("UTCDate");
  if (utc != NULL) {
    log("Received UTCDate: %s", utc.c_str());

    int year, month, day, hour, minute, second;

    sscanf(utc.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &year, &month, &day, &hour,
           &minute, &second);

    log("Parsed Date - Year: %d, Month: %d, Day: %d, Hour: %d, Minute: %d, "
        "Second: %d",
        year, month, day, hour, minute, second);

    model.setUTCYear(year);
    model.setUTCMonth(month);
    model.setUTCDay(day);
    model.setUTCHour(hour);
    model.setUTCMinute(minute);
    model.setUTCSecond(second);
  }
  returnNoError(request);
}

unsigned long lastPositionRequestTime;
#define STALE_POSITION_TIME 300

double ra;
double dec;

// poll eq platform for it's position. Calculate position.
// Triggered from ra or dec request. Should only run for one of them and then
// cache for a few millis
void updatePosition(TelescopeModel &model) {
  if (wsConnected) {
    webSocket.sendTXT("request_data");
    // Use a simple delay-based approach to wait for the response.
    delay(100); // wait for the data (adjust as necessary)
  } else {
    log("Calculating position but no eq platform");
  }
  //TODO make this oo
  model.setEncoderValues(getEncoderAl(), getEncoderAz());
  ra = model.getRACoord();
  dec = model.getDecCoord();
}

void getRA(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionRequestTime) > STALE_POSITION_TIME) {
    lastPositionRequestTime = now;
    updatePosition(model);
  }

  returnSingleDouble(request, ra);
}

void getDec(AsyncWebServerRequest *request, TelescopeModel &model) {
  unsigned long now = millis();
  if ((now - lastPositionRequestTime) > STALE_POSITION_TIME) {
    lastPositionRequestTime = now;
    updatePosition(model);
  }
  returnSingleDouble(request, dec);
}

void setupWebServer(TelescopeModel model) {

  // set up alpaca discovery udp
  if (udp.listen(32227)) {
    log("Listening for discovery requests...");
    udp.onPacket([](AsyncUDPPacket packet) {
      log("Received UDP Discovery packet ");
      if ((packet.length() >= 16) &&
          (strncmp("alpacadiscovery1", (char *)packet.data(), 16) == 0)) {
        log("Responsing to UDP Discovery packet ");

        packet.printf("{\"AlpacaPort\": %d}", alpacaPort);
      }
    });
  }

  // set up listening for ip address from eq platform

  // Initialize UDP to listen for broadcasts.
  if (udp.listen(IPBROADCASTPORT)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      String msg = packet.readString();
      log("UDP Broadcast received: %s\n", msg.c_str());

      // Check if the broadcast is from EQ Platform
      if (msg.startsWith("EQIP=")) {
        eqPlatformIP.fromString(msg.substring(5));
        log("EQ Platform IP is: %s\n", eqPlatformIP.toString().c_str());

        // If WebSocket is not connected, try connecting
        if (!wsConnected) {
          webSocket.begin(eqPlatformIP, EQ_PLATFORM_WEBSOCKET_PORT, "/ws");
          webSocket.onEvent(webSocketEvent);
        }
      }
    });
  }

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/alignmentmode.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleInteger(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/aperturearea.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/aperturediameter.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/athome.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/atpark.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canfindhome.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canpark.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canpulseguide.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansetdeclinationrate.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansetguiderates.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansetpark.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansetpierside.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canmoveaxis.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansetrightascensionrate.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansettracking.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canslew.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canslewaltaz.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canslewasync.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canslewaltazasync.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansync.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, true); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/cansyncaltaz.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/canunpark.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/declinationrate.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/doesrefraction.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  // https://ascom-standards.org/Help/Developer/html/T_ASCOM_DeviceInterface_EquatorialCoordinateType.htm
  // equTopocentric
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/equatorialsystem.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleInteger(request, 1); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/focallength.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/sideofpier.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/siderealtime.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/slewing.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/tracking.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, false); });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/driverversion.*$",
                     HTTP_GET, [](AsyncWebServerRequest *request) {
                       returnSingleString(request, "1.0");
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/driverinfo.*$", HTTP_GET,
                     [](AsyncWebServerRequest *request) {
                       returnSingleString(request, "Hackypoo");
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/description.*$", HTTP_GET,
                     [](AsyncWebServerRequest *request) {
                       returnSingleString(request, "Frankendob");
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/name.*$", HTTP_GET,
                     [](AsyncWebServerRequest *request) {
                       returnSingleString(request, "Frankendob");
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/interfaceversion.*$",
                     HTTP_GET, [](AsyncWebServerRequest *request) {
                       returnSingleInteger(request, 3);
                     }); //?

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/supportedactions.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnEmptyArray(request); });

  alpacaWebServer.on(

      "^\\/api\\/v1\\/telescope\\/0\\/connected.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleBool(request, true); });

  // Mangement API ================

  alpacaWebServer.on(
      "^\\/management\\/apiversions.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnApiVersions(request); });
  alpacaWebServer.on(
      "^\\/management\\/v1\\/description.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnDeviceDescription(request); });

  alpacaWebServer.on(
      "^\\/management\\/v1\\/configureddevices.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnConfiguredDevices(request); });

  // ======================================
  // Gets to be implemented
  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/utcdate.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleString(request, ""); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/siteelevation.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/sitelatitude.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/sitelongitude.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/azimuth.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/altitude.*$", HTTP_GET,
      [](AsyncWebServerRequest *request) { returnSingleDouble(request, 0); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/declination.*$", HTTP_GET,
      [&model](AsyncWebServerRequest *request) { getDec(request, model); });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/rightascension.*$", HTTP_GET,
      [&model](AsyncWebServerRequest *request) { getRA(request, model); });

  // ======================================
  // Boilplate ends. PUTS to be implemented here
  alpacaWebServer.on(

      "^\\/api\\/v1\\/telescope\\/0\\/connected.*$", HTTP_PUT,
      [](AsyncWebServerRequest *request) { returnNoError(request); });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/synctocoordinates.*$",
                     HTTP_PUT, [&model](AsyncWebServerRequest *request) {
                       syncToCoords(request, model);
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/sitelatitude.*$", HTTP_PUT,
                     [&model](AsyncWebServerRequest *request) {
                       setSiteLatitude(request, model);
                     });

  alpacaWebServer.on("^\\/api\\/v1\\/telescope\\/0\\/sitelongitude.*$",
                     HTTP_PUT, [&model](AsyncWebServerRequest *request) {
                       setSiteLongitude(request, model);
                     });

  alpacaWebServer.on(
      "^\\/api\\/v1\\/telescope\\/0\\/utcdate.*$", HTTP_PUT,
      [&model](AsyncWebServerRequest *request) { setUTCDate(request, model); });

  // ===============================

  alpacaWebServer.onNotFound(
      [](AsyncWebServerRequest *request) { handleNotFound(request); });
  alpacaWebServer.begin();
  log("Server started");
  return;
}
