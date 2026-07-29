#ifndef SMART_FARM_UTILS_H
#define SMART_FARM_UTILS_H
#include <Arduino.h>
class Utils {
 public:
  static String randomToken(uint8_t bytes);
  static String hashPassword(const String &password, const String &salt);
  static bool isSafeName(const String &value, uint8_t maxLength);
  static String uptimeText();
  static uint8_t weekdayFromEpoch(uint32_t epoch);
};
#endif
