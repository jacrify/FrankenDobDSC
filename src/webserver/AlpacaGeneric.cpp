
#include "AlpacaGeneric.h"
#include "Logging.h"
#include <cstdlib>
const int BUFFER_SIZE = 300;
/**
 * Hold generic alpaca return functions
 */

long getTransactionID(AsyncWebServerRequest *request) {

  String id = request->arg("ClientTransactionID");
  if (id != NULL) {
    // log("Received id: %s", id.c_str());

    long parsedValue = strtol(id.c_str(), NULL, 10);
    // log("Parsed client id value: %ld", parsedValue);
    return parsedValue;
  }
  return 0;
}
long serverTransactionID = 0;
long generateServerID() { return serverTransactionID++; }

void returnSingleString(AsyncWebServerRequest *request, String s) {
  log("Single string value url is %s, string is %s", request->url().c_str(), s);

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": "%s",
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           s, getTransactionID(request), generateServerID());

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnEmptyArray(AsyncWebServerRequest *request) {
  log("Empty array value url is %s", request->url().c_str());

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": [],
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID());

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnNoError(AsyncWebServerRequest *request) {

  // log("Returning no error for url %s ", request->url().c_str());
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
           "ErrorNumber": 0,
           "ErrorMessage": "",
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           getTransactionID(request), generateServerID());
  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleDouble(AsyncWebServerRequest *request, double d) {
  // log("Single double value url is %s, double is %lf", request->url().c_str(),
  //     d);
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %lf,
          "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           d, getTransactionID(request), generateServerID);

  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleBool(AsyncWebServerRequest *request, bool b) {
  log("Single bool value url is %s, bool is %d", request->url().c_str(), b);
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %s,
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           b ? "true" : "false", getTransactionID(request), generateServerID);


  String json = buffer;
  request->send(200, "application/json", json);
}

void returnSingleInteger(AsyncWebServerRequest *request, int value) {
  log("Single int value url is %s, int is %ld", request->url().c_str(), value);
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": %ld,
        "ClientTransactionID": %ld,
         "ServerTransactionID": %ld
        })",
           value,getTransactionID(request), generateServerID);


  String json = buffer;
  request->send(200, "application/json", json);
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