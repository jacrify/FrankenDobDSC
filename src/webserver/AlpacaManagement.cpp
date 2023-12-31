#include "AlpacaManagement.h"
#include "AlpacaGeneric.h"
#include "Logging.h"

const int BUFFER_SIZE = 300;

void returnDeviceDescription(AsyncWebServerRequest *request) {
  log("returnDeviceDescription url is  %s", request->url().c_str());

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
         "Value": {
          "ServerName": "myserver",
          "Manufacturer": "me",
          "ManufacturerVersion": "1",
          "Location": "here"
          },
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnConfiguredDevices(AsyncWebServerRequest *request) {
  log("returnConfiguredDevices url is  %s", request->url().c_str());

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
            "Value": [ {
          "DeviceName": "Frankendob",
          "DeviceType": "Telescope",
         "DeviceNumber": 0,
          "UniqueID": "FrankenDobDSC"
          }],
          "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnApiVersions(AsyncWebServerRequest *request) {
  log("returnApiVersions url is  %s", request->url().c_str());

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
         "Value": [1],
          "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}

void setupAlpacaManagment(AsyncWebServer &alpacaWebServer) {

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
}
