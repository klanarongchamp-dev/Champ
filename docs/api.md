# API Documentation

All endpoints return JSON unless otherwise noted. Authenticated write calls require header `X-CSRF-Token` from login.

| Method | Path | Purpose |
| --- | --- | --- |
| POST | `/api/login` | Body `{ "username": "admin", "password": "admin1234" }`; sets `SFSESSION` cookie and returns CSRF token. |
| POST | `/api/logout` | Clears active session. |
| GET | `/api/status` | Returns time, WiFi, IP, RSSI, uptime, heap, CPU, relay status, and scheduler state. |
| GET | `/api/relay` | Lists relays. |
| POST | `/api/relay` | Body `{ "id": 1, "state": true }` manually switches a relay. |
| GET | `/api/schedule` | Lists schedules. |
| POST | `/api/schedule` | Creates or updates schedule with `name`, `relayId`, `daysMask`, `startMinute`, `endMinute`, `enabled`. |
| GET | `/api/users` | Admin user listing. |
| GET | `/api/settings` | Reads settings. |
| GET | `/api/backup/export` | Admin config export. |
| POST | `/api/backup/import` | Admin config import JSON. |
| GET | `/api/logs` | Admin log output as text. |
| POST | `/api/factory-reset` | Admin reset and restart. |

`daysMask` uses Sunday as bit 0 through Saturday as bit 6. Every day is `127`.
