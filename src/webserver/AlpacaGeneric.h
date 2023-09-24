#ifndef ALPACA_GENERIC_H
#define ALPACA_GENERIC_H

#include <ESPAsyncWebServer.h>

void returnEmptyArray(AsyncWebServerRequest *request);
void returnSingleBool(AsyncWebServerRequest *request, bool b);
void returnSingleDouble(AsyncWebServerRequest *request, double d);
void returnNoError(AsyncWebServerRequest *request);
void returnSingleInteger(AsyncWebServerRequest *request, int value);
void handleNotFound(AsyncWebServerRequest *request);
void returnSingleString(AsyncWebServerRequest *request, String s);
long getTransactionID(AsyncWebServerRequest *request);
long generateServerID();
#endif
