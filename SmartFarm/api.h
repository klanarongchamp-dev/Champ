#ifndef API_SERVICE_H
#define API_SERVICE_H
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include "auth.h"
#include "relay.h"
#include "scheduler.h"
class ApiService { public: ApiService(ESP8266WebServer &serverRef, StorageService &storageRef, AuthService &authRef, RelayManager &relayRef, SchedulerService &schedulerRef, NTPClient &timeRef); void begin(); private: ESP8266WebServer &server; StorageService &storage; AuthService &auth; RelayManager &relays; SchedulerService &scheduler; NTPClient &timeClient; void sendJson(JsonDocument &doc); bool requireAuth(); bool requireCsrf(); String body(); };
#endif
