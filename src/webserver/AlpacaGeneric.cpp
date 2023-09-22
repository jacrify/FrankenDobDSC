
#include "AlpacaGeneric.h"
#include "Logging.h"
const int BUFFER_SIZE = 300;
/**
 * Hold generic alpaca return functions
*/

void returnSingleString(AsyncWebServerRequest *request, String s) {
  log("Single string value url is %s, string is %s", request->url().c_str(), s);

  char buffer[BUFFER_SIZE];
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

void returnEmptyArray(AsyncWebServerRequest *request) {
  log("Empty array value url is %s", request->url().c_str());

  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer),
           R"({
             "ErrorNumber": 0,
             "ErrorMessage": "",
             "Value": []
      })");

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

void returnSingleDouble(AsyncWebServerRequest *request, double d) {
  // log("Single double value url is %s, double is %lf", request->url().c_str(),
  //     d);
  char buffer[BUFFER_SIZE];
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
  char buffer[BUFFER_SIZE];
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

void returnSingleInteger(AsyncWebServerRequest *request, int value) {
  log("Single int value url is %s, int is %ld", request->url().c_str(), value);
  char buffer[BUFFER_SIZE];
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