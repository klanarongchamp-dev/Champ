#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H
#include <Arduino.h>
class WiFiService { public: void begin(const char *apName); void loop(); String ip() const; int32_t rssi() const; bool connected() const; private: uint32_t lastCheck = 0; };
#endif
