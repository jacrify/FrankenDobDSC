#include "WebUI.h"
#include "Encoders.h"
#include "Logging.h"
#include "TelescopeModel.h"
#include <EQPlatform.h>
#include <LittleFS.h>
#include <Preferences.h>
#define PREF_ALT_STEPS_KEY "AltStepsKey"
#define PREF_AZ_STEPS_KEY "AzStepsKey"

void getScopeStatus(AsyncWebServerRequest *request, TelescopeModel &model,
                    EQPlatform &platform) {
  // log("/getStatus");

  platform.checkConnectionStatus();

  char buffer[2000];
  sprintf(buffer,
          R"({
    
    "calculateAltEncoderStepsPerRevolution" : %ld,
    "calculateAzEncoderStepsPerRevolution" : %ld,
    "actualAltEncoderStepsPerRevolution" : %ld,
    "actualAzEncoderStepsPerRevolution" : %ld,
    "lastSyncedRa" : %lf,
    "lastSyncedDec" : %lf,
    "lastSyncedAlt" : %lf,
    "lastSyncedAz" : %lf,
    "lastSyncedErr", : %lf,
    "eqPlatformIP" : "%s",
    "platformTracking" : %s,
    "timeToMiddle" : %.1lf,
    "timeToEnd" : %.1lf,
    "platformConnected" : %s
})",

          model.calculateAltEncoderStepsPerRevolution(),
          model.calculateAzEncoderStepsPerRevolution(),
          model.getAltEncoderStepsPerRevolution(),
          model.getAzEncoderStepsPerRevolution(),
          model.lastSyncPoint.eqCoord.getRAInDegrees(),
          model.lastSyncPoint.eqCoord.getDecInDegrees(),
          model.lastSyncPoint.horizCoord.altInDegrees,
          model.lastSyncPoint.horizCoord.aziInDegrees,
          model.lastSyncPoint.errorInDegreesAtCreation,

          platform.eqPlatformIP.c_str(),
          platform.currentlyRunning ? "true" : "false",
          platform.runtimeFromCenterSeconds / 60, platform.timeToEnd / 60,
          platform.platformConnected ? "true" : "false");

  String json = buffer;

  // String json = "{\"timestamp\":" + String(times.get()) + ",\"position\":"
  // + String(positions.get()) + ",\"velocity\":" + String(velocities.get()) +
  // "}";
  request->send(200, "application/json", json);
}

void saveAltEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                         Preferences &prefs) {

  long alt = model.calculateAltEncoderStepsPerRevolution();
  log("Setting new value for encoder alt steps  to %ld", alt);
  model.setAltEncoderStepsPerRevolution(alt);

  prefs.putLong(PREF_ALT_STEPS_KEY, alt);
  request->send(200);
}

void saveAzEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                        Preferences &prefs) {

  long az = model.calculateAzEncoderStepsPerRevolution();
  log("Setting new value for encoder az steps to %ld", az);
  model.setAzEncoderStepsPerRevolution(az);

  prefs.putLong(PREF_AZ_STEPS_KEY, az);
  request->send(200);
}
void clearAlignment(AsyncWebServerRequest *request, TelescopeModel &model) {
  log("Clearing alignment");
  model.clearAlignment();
  request->send(200);
}
void loadPreferences(Preferences &prefs, TelescopeModel &model) {
  model.setAltEncoderStepsPerRevolution(
      prefs.getLong(PREF_ALT_STEPS_KEY, -30000));

  model.setAzEncoderStepsPerRevolution(
      prefs.getLong(PREF_AZ_STEPS_KEY, 108531));
}

// safety: clears encoder steps
void clearPrefs(AsyncWebServerRequest *request, Preferences &prefs,
                TelescopeModel &model) {

  prefs.remove(PREF_ALT_STEPS_KEY);
  prefs.remove(PREF_AZ_STEPS_KEY);

  loadPreferences(prefs, model);
  request->send(200);
}

void performZeroedAlignment(AsyncWebServerRequest *request,
                            EQPlatform &platform, TelescopeModel &model) {
  zeroEncoders();
  platform.zeroOffsetTime();
  TimePoint now = platform.calculateAdjustedTime();
  model.performZeroedAlignment(now);
  request->send(200);
}
void setupWebUI(AsyncWebServer &alpacaWebServer, TelescopeModel &model,
                EQPlatform &platform, Preferences &prefs) {
  loadPreferences(prefs, model);
  alpacaWebServer.on("/getScopeStatus", HTTP_GET,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       getScopeStatus(request, model, platform);
                     });

  alpacaWebServer.on("/saveAltEncoderSteps", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       saveAltEncoderSteps(request, model, prefs);
                     });

  alpacaWebServer.on("/saveAzEncoderSteps", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       saveAzEncoderSteps(request, model, prefs);
                     });

  alpacaWebServer.on("/clearAlignment", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       clearAlignment(request, model);
                     });

  alpacaWebServer.on("/clearPrefs", HTTP_POST,
                     [&model, &prefs](AsyncWebServerRequest *request) {
                       clearPrefs(request, prefs, model);
                     });

  alpacaWebServer.on("/performZeroedAlignment", HTTP_POST,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       performZeroedAlignment(request, platform, model);
                     });

  alpacaWebServer.on("/trackingOn", HTTP_GET,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       platform.setTracking(true);
                     });
  alpacaWebServer.on("/trackingOff", HTTP_GET,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       platform.setTracking(false);
                     });
  alpacaWebServer.serveStatic("/", LittleFS, "/fs/");
}
