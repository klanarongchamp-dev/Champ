#include "web.h"
#include <LittleFS.h>
WebService::WebService(ESP8266WebServer &serverRef, AuthService &authRef) : server(serverRef), auth(authRef) {}
void WebService::begin() {
  server.on("/", HTTP_GET, [&]() { if (!auth.isAuthenticated(server)) { server.sendHeader("Location", "/login.html"); server.send(302); return; } serveFile("/index.html", "text/html"); });
  server.on("/login.html", HTTP_GET, [&]() { serveFile("/login.html", "text/html"); });
  server.on("/style.css", HTTP_GET, [&]() { serveFile("/style.css", "text/css"); });
  server.on("/app.js", HTTP_GET, [&]() { serveFile("/app.js", "application/javascript"); });
}
void WebService::serveFile(const String &path, const String &type) { File f = LittleFS.open(path, "r"); if (!f) { server.send(404, "text/plain", "Not found"); return; } server.streamFile(f, type); f.close(); }
