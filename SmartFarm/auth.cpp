#include "auth.h"
#include "config.h"
#include "utils.h"

AuthService::AuthService(StorageService &storageRef) : storage(storageRef) {}
void AuthService::begin() {
  if (!load()) upsertUser(1, "admin", "admin1234", "admin", 0xFFFFFFFFUL);
}

bool AuthService::login(const String &username, const String &password, String &sessionToken, String &csrfToken) {
  AppUser *user = findByName(username);
  if (user == nullptr || user->passwordHash != Utils::hashPassword(password, user->salt)) {
    storage.log("login", "Failed login for " + username);
    return false;
  }
  activeSession = Utils::randomToken(16);
  activeCsrf = Utils::randomToken(16);
  activeUserId = user->id;
  sessionStarted = millis() / 1000;
  sessionToken = activeSession;
  csrfToken = activeCsrf;
  storage.log("login", "Login success for " + username);
  return true;
}
void AuthService::logout(const String &sessionToken) { if (sessionToken.length() == 0 || sessionToken == activeSession) activeSession = ""; }
bool AuthService::isAuthenticated(ESP8266WebServer &server) {
  String token = cookieValue(server, "SFSESSION");
  if (token.length() == 0 || token != activeSession) return false;
  if ((millis() / 1000) - sessionStarted > SESSION_TTL_SECONDS) { activeSession = ""; return false; }
  return true;
}
bool AuthService::isAdmin(ESP8266WebServer &server) { return isAuthenticated(server) && currentRole(cookieValue(server, "SFSESSION")) == "admin"; }
bool AuthService::validateCsrf(ESP8266WebServer &server) { return isAuthenticated(server) && server.header("X-CSRF-Token") == activeCsrf; }
void AuthService::usersToJson(JsonArray array) const {
  for (uint8_t i = 0; i < userCount; i++) { JsonObject u = array.createNestedObject(); u["id"] = users[i].id; u["username"] = users[i].username; u["role"] = users[i].role; u["relayMask"] = users[i].relayMask; }
}
bool AuthService::upsertUser(uint8_t id, const String &username, const String &password, const String &role, uint32_t relayMask) {
  if (!Utils::isSafeName(username, 20) || (role != "admin" && role != "user")) return false;
  AppUser *user = findById(id);
  if (user == nullptr) { if (userCount >= MAX_USERS) return false; user = &users[userCount++]; user->id = id == 0 ? userCount : id; }
  user->username = username; user->role = role; user->relayMask = relayMask;
  if (password.length() > 0) { user->salt = Utils::randomToken(8); user->passwordHash = Utils::hashPassword(password, user->salt); }
  return save();
}
bool AuthService::removeUser(uint8_t id) {
  if (userCount <= 1) return false;
  for (uint8_t i = 0; i < userCount; i++) if (users[i].id == id) { for (uint8_t j = i; j + 1 < userCount; j++) users[j] = users[j + 1]; userCount--; return save(); }
  return false;
}
String AuthService::currentRole(const String &sessionToken) const {
  if (sessionToken != activeSession) return "";
  for (uint8_t i = 0; i < userCount; i++) if (users[i].id == activeUserId) return users[i].role;
  return "";
}
bool AuthService::load() {
  StaticJsonDocument<4096> doc; if (!storage.readJson(USERS_FILE, doc)) return false; userCount = 0;
  for (JsonObject item : doc["users"].as<JsonArray>()) { if (userCount >= MAX_USERS) break; users[userCount++] = {item["id"] | userCount, item["username"] | "user", item["role"] | "user", item["salt"] | "", item["passwordHash"] | "", item["relayMask"] | 0UL}; }
  return userCount > 0;
}
bool AuthService::save() {
  StaticJsonDocument<4096> doc; JsonArray array = doc.createNestedArray("users");
  for (uint8_t i = 0; i < userCount; i++) { JsonObject u = array.createNestedObject(); u["id"] = users[i].id; u["username"] = users[i].username; u["role"] = users[i].role; u["salt"] = users[i].salt; u["passwordHash"] = users[i].passwordHash; u["relayMask"] = users[i].relayMask; }
  return storage.writeJson(USERS_FILE, doc);
}
AppUser *AuthService::findByName(const String &username) { for (uint8_t i = 0; i < userCount; i++) if (users[i].username == username) return &users[i]; return nullptr; }
AppUser *AuthService::findById(uint8_t id) { for (uint8_t i = 0; i < userCount; i++) if (users[i].id == id) return &users[i]; return nullptr; }
String AuthService::cookieValue(ESP8266WebServer &server, const String &name) const {
  String cookie = server.header("Cookie"); int start = cookie.indexOf(name + "="); if (start < 0) return ""; start += name.length() + 1; int end = cookie.indexOf(';', start); return cookie.substring(start, end < 0 ? cookie.length() : end);
}
