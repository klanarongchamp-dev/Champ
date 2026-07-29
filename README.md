# Smart Farm Controller Pro

Production-oriented ESP8266 NodeMCU V3 smart farm controller built for Arduino IDE, LittleFS, ESP8266WebServer, ArduinoOTA, NTPClient, WiFiManager, and ArduinoJson.

## Features

- Responsive Thai dashboard with Bootstrap 5, light/dark mode, toast, and cards.
- Admin/user login with cookie session, CSRF token, SHA-1 salted password hash, and logout.
- Relay manager with default Pump, Zone1, Zone2, Zone3; ON/OFF and AUTO/MANUAL state model.
- Multi-schedule engine using NTP time and `millis()` ticks without blocking `delay()`.
- REST JSON API for status, relay, schedule, auth, users, settings, backup, logs, and reset.
- LittleFS persistence for settings, users, relay, schedule, logs, import/export backup.
- WiFiManager captive portal fallback using `SmartFarm_Setup`.
- Web OTA at `/ota` plus ArduinoOTA background handler.

## Installation Guide

1. Install Arduino IDE and ESP8266 board package.
2. Select **NodeMCU 1.0 (ESP-12E Module)**.
3. Install libraries: WiFiManager, ArduinoJson, NTPClient.
4. Install the ESP8266 LittleFS upload tool.
5. Open `SmartFarm/main.ino`.
6. Upload sketch, then upload `SmartFarm/data` to LittleFS.
7. Connect to WiFi or fallback AP `SmartFarm_Setup`.
8. Browse to device IP and login with `admin` / `admin1234`; change password immediately.

## Wiring Diagram

See [`docs/wiring-diagram.md`](docs/wiring-diagram.md). Default relay pins are D1, D2, D5, and D6.

## Flow Diagram

See [`docs/flow-diagram.md`](docs/flow-diagram.md) for boot, web, scheduler, and persistence flow.

## API Documentation

See [`docs/api.md`](docs/api.md).

## Future Expansion Guide

- Add sensor modules by following the same OOP pattern used by `RelayManager` and `SchedulerService`.
- Keep JSON files small and bounded for ESP8266 RAM safety.
- Add MQTT by publishing status snapshots from the main loop with non-blocking reconnect logic.
- Add role-specific relay permission checks before production multi-user deployment.
- Use HTTPS termination on a local gateway for internet-exposed installations.

## Validation Notes

The firmware is designed to compile in Arduino IDE with ESP8266 core. This repository also includes static checks that verify required files and blocking calls.
