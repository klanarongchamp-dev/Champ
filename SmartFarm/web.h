#ifndef WEB_SERVICE_H
#define WEB_SERVICE_H
#include <ESP8266WebServer.h>
#include "auth.h"
class WebService { public: WebService(ESP8266WebServer &serverRef, AuthService &authRef); void begin(); private: ESP8266WebServer &server; AuthService &auth; void serveFile(const String &path, const String &type); };
#endif
