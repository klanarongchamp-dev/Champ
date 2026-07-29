#include "relay.h"
#include "config.h"
#include "utils.h"

RelayManager::RelayManager(StorageService &storageRef) : storage(storageRef) {}

void RelayManager::begin() {
  if (!load()) {
    const char *names[] = {"Pump", "Zone1", "Zone2", "Zone3"};
    relayCount = DEFAULT_RELAY_COUNT;
    for (uint8_t i = 0; i < relayCount; i++) {
      relays[i] = {static_cast<uint8_t>(i + 1), static_cast<uint8_t>(DEFAULT_RELAY_PINS[i]), names[i], false, RELAY_AUTO, i};
    }
    save();
  }
  for (uint8_t i = 0; i < relayCount; i++) {
    pinMode(relays[i].pin, OUTPUT);
    digitalWrite(relays[i].pin, relays[i].state ? LOW : HIGH);
  }
}

void RelayManager::loop() {}
uint8_t RelayManager::count() const { return relayCount; }

bool RelayManager::setState(uint8_t id, bool state, const String &source) {
  RelayChannel *relay = find(id);
  if (relay == nullptr) return false;
  relay->state = state;
  digitalWrite(relay->pin, state ? LOW : HIGH);
  storage.log("relay", relay->name + (state ? " ON by " : " OFF by ") + source);
  return save();
}

bool RelayManager::setMode(uint8_t id, RelayMode mode) {
  RelayChannel *relay = find(id);
  if (relay == nullptr) return false;
  relay->mode = mode;
  return save();
}

bool RelayManager::upsert(uint8_t id, const String &name, uint8_t pin, uint8_t order) {
  if (!Utils::isSafeName(name, 24)) return false;
  RelayChannel *relay = find(id);
  if (relay == nullptr) {
    if (relayCount >= MAX_RELAYS) return false;
    relay = &relays[relayCount++];
    relay->id = nextId();
    relay->state = false;
    relay->mode = RELAY_AUTO;
  }
  relay->name = name;
  relay->pin = pin;
  relay->order = order;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, relay->state ? LOW : HIGH);
  return save();
}

bool RelayManager::remove(uint8_t id) {
  for (uint8_t i = 0; i < relayCount; i++) {
    if (relays[i].id == id) {
      digitalWrite(relays[i].pin, HIGH);
      for (uint8_t j = i; j + 1 < relayCount; j++) relays[j] = relays[j + 1];
      relayCount--;
      return save();
    }
  }
  return false;
}

RelayChannel *RelayManager::find(uint8_t id) {
  for (uint8_t i = 0; i < relayCount; i++) if (relays[i].id == id) return &relays[i];
  return nullptr;
}

void RelayManager::toJson(JsonArray array) const {
  for (uint8_t i = 0; i < relayCount; i++) {
    JsonObject item = array.createNestedObject();
    item["id"] = relays[i].id;
    item["name"] = relays[i].name;
    item["pin"] = relays[i].pin;
    item["state"] = relays[i].state;
    item["mode"] = relays[i].mode == RELAY_AUTO ? "AUTO" : "MANUAL";
    item["order"] = relays[i].order;
  }
}

bool RelayManager::load() {
  StaticJsonDocument<4096> doc;
  if (!storage.readJson(RELAYS_FILE, doc)) return false;
  relayCount = 0;
  for (JsonObject item : doc["relays"].as<JsonArray>()) {
    if (relayCount >= MAX_RELAYS) break;
    relays[relayCount++] = {item["id"] | relayCount, item["pin"] | D1, item["name"] | "Relay", item["state"] | false, String(item["mode"] | "AUTO") == "AUTO" ? RELAY_AUTO : RELAY_MANUAL, item["order"] | relayCount};
  }
  return relayCount > 0;
}

bool RelayManager::save() {
  StaticJsonDocument<4096> doc;
  JsonArray array = doc.createNestedArray("relays");
  toJson(array);
  return storage.writeJson(RELAYS_FILE, doc);
}

uint8_t RelayManager::nextId() const {
  uint8_t id = 1;
  for (uint8_t i = 0; i < relayCount; i++) if (relays[i].id >= id) id = relays[i].id + 1;
  return id;
}
