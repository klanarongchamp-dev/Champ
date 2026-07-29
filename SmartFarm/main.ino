#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "api.h"
#include "auth.h"
#include "config.h"
#include "ota.h"
#include "relay.h"
#include "scheduler.h"
#include "storage.h"
#include "utils.h"
#include "web.h"
#include "wifi.h"

ESP8266WebServer server(80);
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org", 7 * 3600, 60000);

StorageService storage;
RelayManager relayManager(storage);
AuthService authService(storage);
SchedulerService scheduler(storage, relayManager, timeClient);
WiFiService wifiService;
OtaService otaService(server);
ApiService apiService(server, storage, authService, relayManager, scheduler, timeClient);
WebService webService(server, authService);

void setup() {
  ESP.wdtEnable(WDTO_8S);
  Serial.begin(115200);
  Serial.println();
  Serial.println(APP_NAME);

  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed, formatting"));
    LittleFS.format();
    LittleFS.begin();
  }

  storage.begin();
  relayManager.begin();
  authService.begin();
  wifiService.begin(AP_SSID);
  timeClient.begin();
  timeClient.update();
  scheduler.begin();
  webService.begin();
  apiService.begin();
  otaService.begin(APP_NAME);
  server.begin();
  storage.log("system", "Boot completed");
}

void loop() {
  ESP.wdtFeed();
  server.handleClient();
  ArduinoOTA.handle();
  wifiService.loop();
  timeClient.update();
  scheduler.loop();
  relayManager.loop();
  storage.loop();
}
