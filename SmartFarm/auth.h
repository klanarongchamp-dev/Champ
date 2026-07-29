#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include "storage.h"

struct AppUser {
  uint8_t id;
  String username;
  String role;
  String salt;
  String passwordHash;
  uint32_t relayMask;
};

class AuthService {
 public:
  explicit AuthService(StorageService &storageRef);
  void begin();
  bool login(const String &username, const String &password, String &sessionToken, String &csrfToken);
  void logout(const String &sessionToken);
  bool isAuthenticated(ESP8266WebServer &server);
  bool isAdmin(ESP8266WebServer &server);
  bool validateCsrf(ESP8266WebServer &server);
  void usersToJson(JsonArray array) const;
  bool upsertUser(uint8_t id, const String &username, const String &password, const String &role, uint32_t relayMask);
  bool removeUser(uint8_t id);
  String currentRole(const String &sessionToken) const;

 private:
  StorageService &storage;
  AppUser users[MAX_USERS];
  uint8_t userCount = 0;
  String activeSession;
  String activeCsrf;
  uint8_t activeUserId = 0;
  uint32_t sessionStarted = 0;
  bool load();
  bool save();
  AppUser *findByName(const String &username);
  AppUser *findById(uint8_t id);
  String cookieValue(ESP8266WebServer &server, const String &name) const;
};
#endif
