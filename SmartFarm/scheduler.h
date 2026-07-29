#ifndef SCHEDULER_SERVICE_H
#define SCHEDULER_SERVICE_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "relay.h"
#include "storage.h"
struct ScheduleItem { uint8_t id; String name; uint8_t relayId; uint8_t daysMask; uint16_t startMinute; uint16_t endMinute; bool enabled; bool running; };
class SchedulerService {
 public:
  SchedulerService(StorageService &storageRef, RelayManager &relayRef, NTPClient &timeRef);
  void begin(); void loop(); void toJson(JsonArray array) const; bool upsert(uint8_t id, const String &name, uint8_t relayId, uint8_t daysMask, uint16_t startMinute, uint16_t endMinute, bool enabled); bool remove(uint8_t id); bool isAnyRunning() const;
 private:
  StorageService &storage; RelayManager &relays; NTPClient &timeClient; ScheduleItem schedules[MAX_SCHEDULES]; uint8_t scheduleCount = 0; uint32_t lastTick = 0; bool load(); bool save(); ScheduleItem *find(uint8_t id); uint8_t nextId() const;
};
#endif
