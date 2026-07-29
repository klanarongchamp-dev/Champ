#include "wifi.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
void WiFiService::begin(const char *apName) { WiFi.mode(WIFI_STA); WiFiManager manager; manager.setConfigPortalTimeout(120); if (!manager.autoConnect(apName)) { WiFi.mode(WIFI_AP_STA); WiFi.softAP(apName); } }
void WiFiService::loop() { if (millis() - lastCheck > 30000) { lastCheck = millis(); if (WiFi.status() != WL_CONNECTED && WiFi.SSID().length() > 0) WiFi.reconnect(); } }
String WiFiService::ip() const { return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
int32_t WiFiService::rssi() const { return WiFi.RSSI(); }
bool WiFiService::connected() const { return WiFi.status() == WL_CONNECTED; }
