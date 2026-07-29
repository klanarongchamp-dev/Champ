#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
class OtaService { public: explicit OtaService(ESP8266WebServer &serverRef); void begin(const char *hostname); private: ESP8266WebServer &server; };
#endif
