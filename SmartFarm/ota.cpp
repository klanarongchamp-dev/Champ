#include "ota.h"
#include <ESP8266HTTPUpdateServer.h>
static ESP8266HTTPUpdateServer httpUpdater;
OtaService::OtaService(ESP8266WebServer &serverRef) : server(serverRef) {}
void OtaService::begin(const char *hostname) { ArduinoOTA.setHostname(hostname); ArduinoOTA.begin(); httpUpdater.setup(&server, "/ota"); }
