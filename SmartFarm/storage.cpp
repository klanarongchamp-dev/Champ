#include "storage.h"
#include "config.h"
#include "utils.h"
#include <LittleFS.h>

void StorageService::begin() {
  StaticJsonDocument<768> doc;
  if (!LittleFS.exists(SETTINGS_FILE)) {
    doc["deviceName"] = APP_NAME;
    doc["darkMode"] = false;
    doc["csrfSalt"] = Utils::randomToken(12);
    writeJson(SETTINGS_FILE, doc);
  }
}

void StorageService::loop() {
  if (savePending && millis() - lastSaveRequestMs > SAVE_DEBOUNCE_MS) {
    savePending = false;
    log("system", "Configuration saved");
  }
}

bool StorageService::readJson(const char *path, JsonDocument &doc) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  return !error;
}

bool StorageService::writeJson(const char *path, const JsonDocument &doc) {
  const String tempPath = String(path) + ".tmp";
  File file = LittleFS.open(tempPath, "w");
  if (!file) return false;
  bool ok = serializeJson(doc, file) > 0;
  file.close();
  if (!ok) return false;
  LittleFS.remove(path);
  ok = LittleFS.rename(tempPath, path);
  scheduleSave();
  return ok;
}

bool StorageService::removeFile(const char *path) { return LittleFS.remove(path); }

void StorageService::scheduleSave() {
  savePending = true;
  lastSaveRequestMs = millis();
}

void StorageService::log(const String &channel, const String &message) {
  File file = LittleFS.open(LOG_FILE, "a");
  if (!file) return;
  file.printf("%lu [%s] %s\n", millis(), channel.c_str(), message.c_str());
  file.close();
}

String StorageService::readLog() {
  File file = LittleFS.open(LOG_FILE, "r");
  if (!file) return "";
  String content = file.readString();
  file.close();
  return content;
}

bool StorageService::exportConfig(String &payload) {
  StaticJsonDocument<8192> out;
  const char *files[] = {SETTINGS_FILE, RELAYS_FILE, USERS_FILE, SCHEDULES_FILE};
  const char *keys[] = {"settings", "relays", "users", "schedules"};
  for (uint8_t i = 0; i < 4; i++) {
    DynamicJsonDocument doc(4096);
    readJson(files[i], doc);
    out[keys[i]] = doc.as<JsonVariant>();
  }
  payload = "";
  return serializeJson(out, payload) > 0;
}

bool StorageService::importConfig(const String &payload) {
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, payload)) return false;
  const char *files[] = {SETTINGS_FILE, RELAYS_FILE, USERS_FILE, SCHEDULES_FILE};
  const char *keys[] = {"settings", "relays", "users", "schedules"};
  for (uint8_t i = 0; i < 4; i++) {
    if (doc.containsKey(keys[i])) { DynamicJsonDocument part(4096); part.set(doc[keys[i]]); writeJson(files[i], part); }
  }
  log("system", "Configuration imported");
  return true;
}

void StorageService::factoryReset() {
  LittleFS.remove(SETTINGS_FILE);
  LittleFS.remove(RELAYS_FILE);
  LittleFS.remove(USERS_FILE);
  LittleFS.remove(SCHEDULES_FILE);
  LittleFS.remove(LOG_FILE);
  ESP.restart();
}
