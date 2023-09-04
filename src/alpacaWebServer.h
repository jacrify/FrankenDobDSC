#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H
#include <ESPAsyncWebServer.h>
#include "telescopeModel.h"


void setupWebServer(TelescopeModel &model);
void webServerloop();
#endif
