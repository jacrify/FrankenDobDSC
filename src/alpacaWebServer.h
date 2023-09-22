#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H
#include "telescopeModel.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "EQPlatform.h"

void setupWebServer(TelescopeModel &model,Preferences &prefs,EQPlatform &platform);

#endif
