#include "utils.h"
#include <Hash.h>

String Utils::randomToken(uint8_t bytes) {
  String token;
  for (uint8_t i = 0; i < bytes; i++) {
    uint8_t value = random(0, 255);
    if (value < 16) token += '0';
    token += String(value, HEX);
  }
  return token;
}

String Utils::hashPassword(const String &password, const String &salt) {
  return sha1(salt + ":" + password + ":smart-farm");
}

bool Utils::isSafeName(const String &value, uint8_t maxLength) {
  if (value.length() == 0 || value.length() > maxLength) return false;
  for (uint16_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (!isAlphaNumeric(c) && c != ' ' && c != '_' && c != '-') return false;
  }
  return true;
}

String Utils::uptimeText() {
  uint32_t seconds = millis() / 1000;
  uint16_t days = seconds / 86400;
  seconds %= 86400;
  uint8_t hours = seconds / 3600;
  seconds %= 3600;
  uint8_t minutes = seconds / 60;
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%ud %02uh %02um", days, hours, minutes);
  return String(buffer);
}

uint8_t Utils::weekdayFromEpoch(uint32_t epoch) { return ((epoch / 86400UL) + 4) % 7; }
