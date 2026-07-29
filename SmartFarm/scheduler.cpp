#include "scheduler.h"
#include "config.h"
#include "utils.h"
SchedulerService::SchedulerService(StorageService &storageRef, RelayManager &relayRef, NTPClient &timeRef) : storage(storageRef), relays(relayRef), timeClient(timeRef) {}
void SchedulerService::begin() { load(); }
void SchedulerService::loop() {
  if (millis() - lastTick < SCHEDULER_TICK_MS || timeClient.getEpochTime() < 100000) return; lastTick = millis();
  uint16_t minute = timeClient.getHours() * 60 + timeClient.getMinutes(); uint8_t dayBit = 1 << Utils::weekdayFromEpoch(timeClient.getEpochTime());
  for (uint8_t i = 0; i < scheduleCount; i++) {
    ScheduleItem &s = schedules[i]; if (!s.enabled || (s.daysMask & dayBit) == 0) continue;
    RelayChannel *relay = relays.find(s.relayId); if (relay == nullptr || relay->mode != RELAY_AUTO) continue;
    bool active = s.startMinute <= s.endMinute ? (minute >= s.startMinute && minute < s.endMinute) : (minute >= s.startMinute || minute < s.endMinute);
    if (active != s.running) { s.running = active; relays.setState(s.relayId, active, "schedule:" + s.name); }
  }
}
void SchedulerService::toJson(JsonArray array) const { for (uint8_t i = 0; i < scheduleCount; i++) { JsonObject o = array.createNestedObject(); o["id"] = schedules[i].id; o["name"] = schedules[i].name; o["relayId"] = schedules[i].relayId; o["daysMask"] = schedules[i].daysMask; o["startMinute"] = schedules[i].startMinute; o["endMinute"] = schedules[i].endMinute; o["duration"] = schedules[i].endMinute >= schedules[i].startMinute ? schedules[i].endMinute - schedules[i].startMinute : 1440 - schedules[i].startMinute + schedules[i].endMinute; o["enabled"] = schedules[i].enabled; o["running"] = schedules[i].running; } }
bool SchedulerService::upsert(uint8_t id, const String &name, uint8_t relayId, uint8_t daysMask, uint16_t startMinute, uint16_t endMinute, bool enabled) { if (!Utils::isSafeName(name, 24) || relays.find(relayId) == nullptr || daysMask == 0 || startMinute > 1439 || endMinute > 1439) return false; ScheduleItem *s = find(id); if (s == nullptr) { if (scheduleCount >= MAX_SCHEDULES) return false; s = &schedules[scheduleCount++]; s->id = nextId(); s->running = false; } s->name = name; s->relayId = relayId; s->daysMask = daysMask; s->startMinute = startMinute; s->endMinute = endMinute; s->enabled = enabled; return save(); }
bool SchedulerService::remove(uint8_t id) { for (uint8_t i = 0; i < scheduleCount; i++) if (schedules[i].id == id) { for (uint8_t j = i; j + 1 < scheduleCount; j++) schedules[j] = schedules[j + 1]; scheduleCount--; return save(); } return false; }
bool SchedulerService::isAnyRunning() const { for (uint8_t i = 0; i < scheduleCount; i++) if (schedules[i].running) return true; return false; }
bool SchedulerService::load() { StaticJsonDocument<4096> doc; if (!storage.readJson(SCHEDULES_FILE, doc)) return false; scheduleCount = 0; for (JsonObject item : doc["schedules"].as<JsonArray>()) { if (scheduleCount >= MAX_SCHEDULES) break; schedules[scheduleCount++] = {item["id"] | scheduleCount, item["name"] | "Schedule", item["relayId"] | 1, item["daysMask"] | 127, item["startMinute"] | 360, item["endMinute"] | 390, item["enabled"] | true, false}; } return true; }
bool SchedulerService::save() { StaticJsonDocument<4096> doc; JsonArray array = doc.createNestedArray("schedules"); toJson(array); return storage.writeJson(SCHEDULES_FILE, doc); }
ScheduleItem *SchedulerService::find(uint8_t id) { for (uint8_t i = 0; i < scheduleCount; i++) if (schedules[i].id == id) return &schedules[i]; return nullptr; }
uint8_t SchedulerService::nextId() const { uint8_t id = 1; for (uint8_t i = 0; i < scheduleCount; i++) if (schedules[i].id >= id) id = schedules[i].id + 1; return id; }
