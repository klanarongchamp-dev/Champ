#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include "storage.h"

enum RelayMode : uint8_t { RELAY_MANUAL = 0, RELAY_AUTO = 1 };

struct RelayChannel {
  uint8_t id;
  uint8_t pin;
  String name;
  bool state;
  RelayMode mode;
  uint8_t order;
};

class RelayManager {
 public:
  explicit RelayManager(StorageService &storageRef);
  void begin();
  void loop();
  uint8_t count() const;
  bool setState(uint8_t id, bool state, const String &source);
  bool setMode(uint8_t id, RelayMode mode);
  bool upsert(uint8_t id, const String &name, uint8_t pin, uint8_t order);
  bool remove(uint8_t id);
  RelayChannel *find(uint8_t id);
  void toJson(JsonArray array) const;
  bool load();
  bool save();

 private:
  StorageService &storage;
  RelayChannel relays[MAX_RELAYS];
  uint8_t relayCount = 0;
  uint8_t nextId() const;
};
#endif
