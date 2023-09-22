#ifndef WEBUI
#define WEBUI
#include "TelescopeModel.h"
#include "EQPlatform.h"
#include <Preferences.h>

#include <ESPAsyncWebServer.h>
void setupWebUI(AsyncWebServer &alpacaWebServer, TelescopeModel &model,
                EQPlatform &platform,  Preferences &prefs);

#endif
