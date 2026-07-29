#include "api.h"
#include "config.h"
#include "utils.h"
#include <ESP8266WiFi.h>
ApiService::ApiService(ESP8266WebServer &serverRef, StorageService &storageRef, AuthService &authRef, RelayManager &relayRef, SchedulerService &schedulerRef, NTPClient &timeRef) : server(serverRef), storage(storageRef), auth(authRef), relays(relayRef), scheduler(schedulerRef), timeClient(timeRef) {}
void ApiService::begin() {
  const char *headers[] = {"Cookie", "X-CSRF-Token"}; server.collectHeaders(headers, 2);
  server.on("/api/login", HTTP_POST, [&]() { DynamicJsonDocument in(512); deserializeJson(in, body()); String session, csrf; StaticJsonDocument<256> out; bool ok = auth.login(in["username"] | "", in["password"] | "", session, csrf); out["ok"] = ok; if (ok) { server.sendHeader("Set-Cookie", "SFSESSION=" + session + "; HttpOnly; SameSite=Strict"); out["csrf"] = csrf; } sendJson(out); });
  server.on("/api/logout", HTTP_POST, [&]() { auth.logout(""); server.sendHeader("Set-Cookie", "SFSESSION=; Max-Age=0"); StaticJsonDocument<64> out; out["ok"] = true; sendJson(out); });
  server.on("/api/status", HTTP_GET, [&]() { if (!requireAuth()) return; StaticJsonDocument<4096> out; out["app"] = APP_NAME; out["version"] = APP_VERSION; out["time"] = timeClient.getFormattedTime(); out["epoch"] = timeClient.getEpochTime(); out["wifi"] = WiFi.status() == WL_CONNECTED; out["ip"] = WiFi.localIP().toString(); out["rssi"] = WiFi.RSSI(); out["uptime"] = Utils::uptimeText(); out["heap"] = ESP.getFreeHeap(); out["cpu"] = ESP.getCpuFreqMHz(); out["scheduleRunning"] = scheduler.isAnyRunning(); relays.toJson(out.createNestedArray("relays")); sendJson(out); });
  server.on("/api/relay", HTTP_GET, [&]() { if (!requireAuth()) return; StaticJsonDocument<4096> out; relays.toJson(out.createNestedArray("relays")); sendJson(out); });
  server.on("/api/relay", HTTP_POST, [&]() { if (!requireCsrf()) return; DynamicJsonDocument in(512); deserializeJson(in, body()); bool ok = relays.setState(in["id"] | 0, in["state"] | false, "manual"); StaticJsonDocument<64> out; out["ok"] = ok; sendJson(out); });
  server.on("/api/schedule", HTTP_GET, [&]() { if (!requireAuth()) return; StaticJsonDocument<4096> out; scheduler.toJson(out.createNestedArray("schedules")); sendJson(out); });
  server.on("/api/schedule", HTTP_POST, [&]() { if (!requireCsrf()) return; DynamicJsonDocument in(768); deserializeJson(in, body()); bool ok = scheduler.upsert(in["id"] | 0, in["name"] | "Schedule", in["relayId"] | 1, in["daysMask"] | 127, in["startMinute"] | 360, in["endMinute"] | 390, in["enabled"] | true); StaticJsonDocument<64> out; out["ok"] = ok; sendJson(out); });
  server.on("/api/users", HTTP_GET, [&]() { if (!auth.isAdmin(server)) { server.send(403); return; } StaticJsonDocument<2048> out; auth.usersToJson(out.createNestedArray("users")); sendJson(out); });
  server.on("/api/settings", HTTP_GET, [&]() { if (!requireAuth()) return; DynamicJsonDocument out(1024); storage.readJson(SETTINGS_FILE, out); sendJson(out); });
  server.on("/api/backup/export", HTTP_GET, [&]() { if (!auth.isAdmin(server)) { server.send(403); return; } String payload; storage.exportConfig(payload); server.send(200, "application/json", payload); });
  server.on("/api/backup/import", HTTP_POST, [&]() { if (!auth.isAdmin(server) || !auth.validateCsrf(server)) { server.send(403); return; } StaticJsonDocument<64> out; out["ok"] = storage.importConfig(body()); sendJson(out); });
  server.on("/api/logs", HTTP_GET, [&]() { if (!auth.isAdmin(server)) { server.send(403); return; } server.send(200, "text/plain", storage.readLog()); });
  server.on("/api/factory-reset", HTTP_POST, [&]() { if (!auth.isAdmin(server) || !auth.validateCsrf(server)) { server.send(403); return; } storage.factoryReset(); });
}
void ApiService::sendJson(JsonDocument &doc) { String out; serializeJson(doc, out); server.send(200, "application/json", out); }
bool ApiService::requireAuth() { if (!auth.isAuthenticated(server)) { server.send(401, "application/json", "{\"error\":\"unauthorized\"}"); return false; } return true; }
bool ApiService::requireCsrf() { if (!auth.validateCsrf(server)) { server.send(403, "application/json", "{\"error\":\"csrf\"}"); return false; } return true; }
String ApiService::body() { return server.hasArg("plain") ? server.arg("plain") : "{}"; }
