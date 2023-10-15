#include "WebUI.h"
#include "Encoders.h"
#include "Logging.h"
#include "TelescopeModel.h"
#include <ArduinoJson.h>
#include <EQPlatform.h>
#include <LittleFS.h>
#include <Preferences.h>

#define PREF_ALT_STEPS_KEY "AltStepsKey"
#define PREF_AZ_STEPS_KEY "AzStepsKey"

void getScopeStatus(AsyncWebServerRequest *request, TelescopeModel &model,
                    EQPlatform &platform) {
  // log("/getStatus");
  platform.checkConnectionStatus();

  // Estimate JSON capacity
  const size_t capacity = JSON_OBJECT_SIZE(15);

  DynamicJsonDocument doc(capacity);

  // Populate the JSON object
  doc["calculateAltEncoderStepsPerRevolution"] = model.calculatedAltEncoderRes;
  doc["calculateAzEncoderStepsPerRevolution"] = model.calculatedAziEncoderRes;
  doc["actualAltEncoderStepsPerRevolution"] =
      model.getAltEncoderStepsPerRevolution();
  doc["actualAzEncoderStepsPerRevolution"] =
      model.getAzEncoderStepsPerRevolution();

  doc["eqPlatformIP"] = platform.eqPlatformIP.c_str();
  doc["platformTracking"] = platform.currentlyRunning;
  doc["timeToMiddle"] =
      static_cast<float>(platform.runtimeFromCenterSeconds) / 60.0f;
  doc["timeToEnd"] = static_cast<float>(platform.timeToEnd) / 60.0f;
  doc["platformConnected"] = platform.platformConnected;
  doc["lastAlignmentTimestamp"]=timePointToString(model.lastSyncPoint.timePoint);

  String json;
  serializeJson(doc, json);

  request->send(200, "application/json", json);
}

void getAlignmentData(AsyncWebServerRequest *request, TelescopeModel &model,
                      EQPlatform &platform) {
  const size_t capacity =
      JSON_ARRAY_SIZE(model.baseAlignmentSynchPoints.size()) +
      JSON_OBJECT_SIZE(3) +
      model.baseAlignmentSynchPoints.size() * JSON_OBJECT_SIZE(7) +
      JSON_OBJECT_SIZE(7);

  DynamicJsonDocument doc(capacity);

  doc["calculateAltEncoderStepsPerRevolution"] = model.calculatedAltEncoderRes;

  // Add baseAlignmentSynchPoints data
  JsonArray baseAlignmentSynchPoints =
      doc.createNestedArray("baseAlignmentSynchPoints");
  for (const SynchPoint &sp : model.baseAlignmentSynchPoints) {
    JsonObject point = baseAlignmentSynchPoints.createNestedObject();
    point["time"] = timePointToString( sp.timePoint);
    point["ra"] = sp.eqCoord.getRAInDegrees();
    point["dec"] = sp.eqCoord.getDecInDegrees();
    point["alt"] = sp.encoderAltAz.altInDegrees;
    point["az"] = sp.encoderAltAz.aziInDegrees;
  }

  // Add lastSyncPoint data
  JsonObject lastSyncPoint = doc.createNestedObject("lastSyncPoint");
  lastSyncPoint["time"] = timePointToString( model.lastSyncPoint.timePoint);
  lastSyncPoint["ra"] = model.lastSyncPoint.eqCoord.getRAInDegrees();
  lastSyncPoint["dec"] = model.lastSyncPoint.eqCoord.getDecInDegrees();
  lastSyncPoint["alt"] = model.lastSyncPoint.encoderAltAz.altInDegrees;
  lastSyncPoint["az"] = model.lastSyncPoint.encoderAltAz.aziInDegrees;
  lastSyncPoint["error"] = model.lastSyncPoint.errorInDegreesAtCreation;

  String json;
  serializeJson(doc, json);

  request->send(200, "application/json", json);
}

void saveAltEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                         Preferences &prefs) {

  long alt = model.calculatedAltEncoderRes;
  log("Setting new value for encoder alt steps  to %ld", alt);
  model.setAltEncoderStepsPerRevolution(alt);

  prefs.putLong(PREF_ALT_STEPS_KEY, alt);
  request->send(200);
}

void saveAzEncoderSteps(AsyncWebServerRequest *request, TelescopeModel &model,
                        Preferences &prefs) {

  long az = model.calculatedAziEncoderRes;
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

  alpacaWebServer.on("/getAlignmentData", HTTP_GET,
                     [&model, &platform](AsyncWebServerRequest *request) {
                       getAlignmentData(request, model, platform);
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
