#ifndef SMART_FARM_CONFIG_H
#define SMART_FARM_CONFIG_H

#include <Arduino.h>

static const char *APP_NAME = "Smart Farm Controller Pro";
static const char *APP_VERSION = "1.0.0";
static const char *AP_SSID = "SmartFarm_Setup";
static const uint32_t SESSION_TTL_SECONDS = 3600;
static const uint8_t MAX_RELAYS = 16;
static const uint8_t MAX_USERS = 8;
static const uint8_t MAX_SCHEDULES = 24;
static const uint32_t STATUS_INTERVAL_MS = 1000;
static const uint32_t SCHEDULER_TICK_MS = 1000;
static const uint32_t SAVE_DEBOUNCE_MS = 1500;
static const int DEFAULT_RELAY_PINS[] = {D1, D2, D5, D6};
static const uint8_t DEFAULT_RELAY_COUNT = 4;

static const char *SETTINGS_FILE = "/settings.json";
static const char *RELAYS_FILE = "/relays.json";
static const char *USERS_FILE = "/users.json";
static const char *SCHEDULES_FILE = "/schedules.json";
static const char *LOG_FILE = "/system.log";

#endif
