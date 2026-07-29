#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>

class StorageService {
 public:
  void begin();
  void loop();
  bool readJson(const char *path, JsonDocument &doc);
  bool writeJson(const char *path, const JsonDocument &doc);
  bool removeFile(const char *path);
  void scheduleSave();
  void log(const String &channel, const String &message);
  String readLog();
  bool exportConfig(String &payload);
  bool importConfig(const String &payload);
  void factoryReset();

 private:
  uint32_t lastSaveRequestMs = 0;
  bool savePending = false;
};

#endif
