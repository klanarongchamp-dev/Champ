#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <DHT.h>
#include <time.h>

// ==========================================
// การตั้งค่า MQTT (HiveMQ Cloud)
// ==========================================
const char* mqtt_server = "e384381d24534ec1bdf7413845bacfa4.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
// ต้องตรงกับ username/password ใน index.html
const char* mqtt_user = "smartfarm";
const char* mqtt_pass = "Kla12345";
const char* ota_hostname = "smartfarm-esp8266";
const char* ota_password = "SmartFarmOTA";

// MQTT Topics
const char* topic_pump     = "farm/pump"; // รองรับคำสั่งเดิมสำหรับรีเลย์ช่อง 1
const char* topic_status   = "farm/status";
const char* topic_time     = "farm/time";
const char* topic_mode_command = "farm/mode/set";
const char* topic_mode_state   = "farm/mode/state";
const char* topic_schedule = "farm/schedule";
// รีเลย์ช่อง 1-4 ใช้ farm/relay/<ช่อง>/command และ /status

// ==========================================
// การตั้งค่า Hardware
// ==========================================
#define RELAY_COUNT 4
#define RELAY_ACTIVE_LOW true
const uint8_t relayPins[RELAY_COUNT] = {D5, D6, D7, D8};
#define DHT_PIN D4
#define DHT_TYPE DHT11
#define I2C_SDA D2   // ขา SDA ของ RTC
#define I2C_SCL D1   // ขา SCL ของ RTC

// ==========================================
// โครงสร้างข้อมูลสำหรับ Schedule
// ==========================================
struct ScheduleData {
  char onTime1[6];  // "HH:MM"
  char offTime1[6]; // "HH:MM"
  char onTime2[6];  // "HH:MM"
  char offTime2[6]; // "HH:MM"
};

#define EEPROM_SIZE 32
#define EEPROM_MODE_OFFSET sizeof(ScheduleData)
#define EEPROM_MODE_MAGIC 0xA5

ScheduleData schedules;
String lastScheduleAction = "";

// ==========================================
// ตัวแปรระบบ
// ==========================================
bool isAutoMode = true; // โหมดการทำงาน (true = Auto, false = Manual)
bool rtcAvailable = false;
bool rtcDetected = false;
bool ntpAvailable = false;
bool ntpSyncLogged = false;
bool relayState[RELAY_COUNT] = {false, false, false, false};

// ตัวแปรสำหรับจัดการเวลา (ไม่ต้องใช้ delay)
unsigned long lastHeartbeat = 0;
unsigned long lastRTCUpdate = 0;
unsigned long lastDHTRead = 0;
unsigned long lastMQTTReconnect = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000; // 30 วินาที
const unsigned long RTC_INTERVAL = 1000;        // 1 วินาที
const unsigned long DHT_INTERVAL = 5000;        // อ่าน DHT11 ทุก 5 วินาที
const unsigned long MQTT_RECONNECT_INTERVAL = 5000; // 5 วินาที
const time_t VALID_TIME_THRESHOLD = 1700000000; // ป้องกันเวลาเริ่มต้นปี 1970

// ==========================================
// ออบเจ็กต์ต่างๆ
// ==========================================
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ==========================================
// ฟังก์ชันอ่าน/เขียน EEPROM
// ==========================================
void loadScheduleFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, schedules);
  
  // ตรวจสอบข้อมูลขยะ ถ้าใช่ให้ตั้งค่าเริ่มต้น
  if (schedules.onTime1[2] != ':' || schedules.offTime1[2] != ':') {
    strcpy(schedules.onTime1, "06:00");
    strcpy(schedules.offTime1, "06:10");
    strcpy(schedules.onTime2, "17:00");
    strcpy(schedules.offTime2, "17:10");
    EEPROM.put(0, schedules);
    EEPROM.commit();
    Serial.println("Initialized Default Schedule in EEPROM");
  } else {
    Serial.println("Loaded Schedule from EEPROM");
  }
  uint8_t modeMagic = EEPROM.read(EEPROM_MODE_OFFSET);
  uint8_t storedMode = EEPROM.read(EEPROM_MODE_OFFSET + 1);
  if (modeMagic == EEPROM_MODE_MAGIC && storedMode <= 1) {
    isAutoMode = storedMode == 1;
    Serial.print("Loaded mode from EEPROM: ");
    Serial.println(isAutoMode ? "AUTO" : "MANUAL");
  } else {
    isAutoMode = true;
    EEPROM.write(EEPROM_MODE_OFFSET, EEPROM_MODE_MAGIC);
    EEPROM.write(EEPROM_MODE_OFFSET + 1, 1);
    EEPROM.commit();
    Serial.println("Initialized default mode: AUTO");
  }
  EEPROM.end();
}

void saveScheduleToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, schedules);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Saved Schedule to EEPROM");
}

void saveModeToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_MODE_OFFSET, EEPROM_MODE_MAGIC);
  EEPROM.write(EEPROM_MODE_OFFSET + 1, isAutoMode ? 1 : 0);
  EEPROM.commit();
  EEPROM.end();
}

// ==========================================
// ฟังก์ชันควบคุมปั๊มน้ำ
// ==========================================
void setupNTP() {
  // ประเทศไทยใช้ UTC+7 และไม่มี daylight saving time
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  Serial.println("NTP fallback enabled (Asia/Bangkok, UTC+7)");
}

void updateNTPStatus() {
  if (rtcAvailable || WiFi.status() != WL_CONNECTED) return;

  time_t now = time(nullptr);
  if (now >= VALID_TIME_THRESHOLD) {
    ntpAvailable = true;
    if (!ntpSyncLogged) {
      ntpSyncLogged = true;
      Serial.println("RTC unavailable; using internet time from NTP");
      if (rtcDetected) {
        rtc.adjust(DateTime(now));
        rtcAvailable = true;
        Serial.println("RTC synchronized from NTP");
      }
    }
  }
}

bool hasValidTime() {
  updateNTPStatus();
  return rtcAvailable || ntpAvailable;
}

DateTime currentDateTime() {
  if (rtcAvailable) return rtc.now();
  return DateTime(time(nullptr));
}

const char* currentTimeSource() {
  return rtcAvailable ? "RTC" : (ntpAvailable ? "NTP" : "NONE");
}

void publishRelayStatus(uint8_t relayIndex) {
  char topic[32];
  snprintf(topic, sizeof(topic), "farm/relay/%u/status", relayIndex + 1);
  client.publish(topic, relayState[relayIndex] ? "ON" : "OFF", true);
}

void setRelay(uint8_t relayIndex, bool state) {
  if (relayIndex >= RELAY_COUNT || relayState[relayIndex] == state) return;
  relayState[relayIndex] = state;
  digitalWrite(relayPins[relayIndex], RELAY_ACTIVE_LOW ? (state ? LOW : HIGH) : (state ? HIGH : LOW));
  Serial.printf("Relay %u turned %s\n", relayIndex + 1, state ? "ON" : "OFF");
  if (client.connected()) publishRelayStatus(relayIndex);
}

void setPump(bool state) {
  setRelay(0, state); // ตารางเวลาเดิมควบคุมรีเลย์ช่อง 1
}

// ==========================================
// ฟังก์ชันส่งข้อมูลผ่าน MQTT (Publish)
// ==========================================
void publishTime() {
  if (!hasValidTime()) return;
  DateTime now = currentDateTime();
  char timeString[20];
  snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  client.publish(topic_time, timeString, true);
}

void publishHeartbeat() {
  if (client.connected() && client.publish(topic_status, "ONLINE", true)) {
    Serial.println("Heartbeat sent: ONLINE");
  }
}

void publishDHT() {
  if (!client.connected()) return;

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT11 read failed");
    return;
  }

  char temperatureValue[12];
  char humidityValue[12];
  snprintf(temperatureValue, sizeof(temperatureValue), "%.1f", temperature);
  snprintf(humidityValue, sizeof(humidityValue), "%.1f", humidity);
  client.publish("farm/temp", temperatureValue, true);
  client.publish("farm/hum", humidityValue, true);
  Serial.printf("DHT11: %.1f C, %.1f %%\n", temperature, humidity);
}

void publishMode() {
  client.publish(topic_mode_state, isAutoMode ? "AUTO" : "MANUAL", true);
}

void setupOTA() {
  ArduinoOTA.setHostname(ota_hostname);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.setPort(8266);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA update started");
    // หยุดรีเลย์ทั้งหมดระหว่างอัปเดตเพื่อความปลอดภัย
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      digitalWrite(relayPins[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update finished");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
    else if (error == OTA_END_ERROR) Serial.println("End failed");
  });
  ArduinoOTA.begin();
  Serial.print("OTA ready: ");
  Serial.print(ota_hostname);
  Serial.println(" (port 8266)");
}

// ==========================================
// ฟังก์ชันรับข้อมูลจาก MQTT (Callback)
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.println("=== MQTT Message Received ===");
  Serial.print("Topic: "); Serial.println(topic);
  Serial.print("Message: "); Serial.println(msg);
  
  // 1. ควบคุมปั๊มน้ำแบบ Manual
  String topicString = String(topic);
  if (topicString == topic_pump) {
    if (!isAutoMode) {
      if (msg == "ON") setPump(true);
      else if (msg == "OFF") setPump(false);
    } else {
      Serial.println("Ignored: System is in AUTO mode");
    }
  }
  else if (topicString.startsWith("farm/relay/") && topicString.endsWith("/command")) {
    int relayNumber = topicString.substring(11, topicString.length() - 8).toInt();
    if (relayNumber >= 1 && relayNumber <= RELAY_COUNT && !isAutoMode) {
      if (msg == "ON") setRelay(relayNumber - 1, true);
      else if (msg == "OFF") setRelay(relayNumber - 1, false);
    }
  }
  // 2. เปลี่ยนโหมด Auto/Manual
  else if (topicString == topic_mode_command) {
    bool previousMode = isAutoMode;
    if (msg == "AUTO") {
      isAutoMode = true;
    } else if (msg == "MANUAL") {
      isAutoMode = false;
    } else {
      Serial.println("Ignored: invalid mode");
      return;
    }
    saveModeToEEPROM();
    if (previousMode != isAutoMode) {
      Serial.print("Mode changed to ");
      Serial.println(isAutoMode ? "AUTO" : "MANUAL");
      publishMode();
    } else {
      Serial.println("Mode unchanged; no MQTT reply");
    }
  }
  // 3. ตั้งค่า Schedule (รูปแบบ: HH:MM,HH:MM,HH:MM,HH:MM)
  else if (String(topic) == topic_schedule) {
    // แยกข้อความด้วยเครื่องหมาย comma
    int firstComma = msg.indexOf(',');
    int secondComma = msg.indexOf(',', firstComma + 1);
    int thirdComma = msg.indexOf(',', secondComma + 1);
    
    if (firstComma > 0 && secondComma > 0 && thirdComma > 0) {
      msg.substring(0, firstComma).toCharArray(schedules.onTime1, 6);
      msg.substring(firstComma + 1, secondComma).toCharArray(schedules.offTime1, 6);
      msg.substring(secondComma + 1, thirdComma).toCharArray(schedules.onTime2, 6);
      msg.substring(thirdComma + 1).toCharArray(schedules.offTime2, 6);
      
      saveScheduleToEEPROM();
      Serial.println("Schedule updated via MQTT");
    }
  }
}

// ==========================================
// ฟังก์ชันเชื่อมต่อ MQTT
// ==========================================
void connectMQTT() {
  if (client.connected()) return;
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastMQTTReconnect >= MQTT_RECONNECT_INTERVAL) {
    lastMQTTReconnect = currentMillis;
    
    Serial.print("Connecting to MQTT...");
    // กำหนด Last Will and Testament (LWT) สำหรับแจ้ง Offline
    String clientId = String("ESP8266Farm-") + String(ESP.getChipId(), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass, topic_status, 0, true, "OFFLINE")) {
      Serial.println("Connected!");
      
      // สมัครรับข้อมูล Topics ที่ต้องการ
      // ล้าง retained command เก่าที่อาจถูกส่งซ้ำตอน reconnect
      client.publish(topic_mode_command, "", true);
      client.subscribe(topic_pump);
      client.subscribe("farm/relay/+/command");
      client.subscribe(topic_mode_command);
      client.subscribe(topic_schedule);

      // ส่งเฉพาะสถานะเอาต์พุตที่เป็นของบอร์ดนี้
      for (uint8_t i = 0; i < RELAY_COUNT; i++) publishRelayStatus(i);
      // โหมดเป็น state ของบอร์ด ไม่รับ retained state กลับมาเป็นคำสั่ง
      publishMode();
      publishHeartbeat();
      lastHeartbeat = millis();
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
    }
  }
}

// ==========================================
// ฟังก์ชันตรวจสอบตารางเวลา (Schedule)
// ==========================================
void checkSchedule() {
  if (!isAutoMode || !hasValidTime()) return;
  
  DateTime now = currentDateTime();
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", now.hour(), now.minute());
  String currentTime = String(buf);
  
  // Schedule 1 ON
  if (currentTime == schedules.onTime1 && lastScheduleAction != "S1ON") {
    setPump(true);
    Serial.println("Auto: Schedule 1 ON");
    lastScheduleAction = "S1ON";
  }
  // Schedule 1 OFF
  else if (currentTime == schedules.offTime1 && lastScheduleAction != "S1OFF") {
    setPump(false);
    Serial.println("Auto: Schedule 1 OFF");
    lastScheduleAction = "S1OFF";
  }
  // Schedule 2 ON
  else if (currentTime == schedules.onTime2 && lastScheduleAction != "S2ON") {
    setPump(true);
    Serial.println("Auto: Schedule 2 ON");
    lastScheduleAction = "S2ON";
  }
  // Schedule 2 OFF
  else if (currentTime == schedules.offTime2 && lastScheduleAction != "S2OFF") {
    setPump(false);
    Serial.println("Auto: Schedule 2 OFF");
    lastScheduleAction = "S2OFF";
  }
  
  // รีเซ็ตสถานะเมื่อผ่านไป 1 นาที (เพื่อรองรับวันถัดไป)
  if (currentTime != schedules.onTime1 && currentTime != schedules.offTime1 && 
      currentTime != schedules.onTime2 && currentTime != schedules.offTime2) {
    lastScheduleAction = "";
  }
}

// ==========================================
// Setup Function
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== SMART FARM SYSTEM STARTING =====");
  
  // ตั้งค่า Relay (ปิดปั๊มเป็นค่าเริ่มต้น)
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
  }

  // DHT11: ต่อขา DATA เข้าที่ D4 และใช้ไฟเลี้ยง 3.3V พร้อม GND
  dht.begin();
  
  // โหลด Schedule จาก EEPROM
  loadScheduleFromEEPROM();
  
  // ตั้งค่า WiFiManager (Auto Reconnect อยู่ในตัวแล้ว)
  WiFiManager wm;
  // รีเซ็ตค่า WiFi หากต้องการ (wm.resetSettings();)
  Serial.println("Connecting to WiFi...");
  if (!wm.autoConnect("SmartFarm_Setup")) {
    Serial.println("Failed to connect WiFi, restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  setupOTA();
  
  // ตั้งค่า RTC
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!rtc.begin()) {
    Serial.println("WARNING: RTC NOT FOUND!");
    rtcAvailable = false;
  } else {
    rtcDetected = true;
    Serial.println("RTC OK");
    if (rtc.lostPower()) {
      Serial.println("RTC lost power; waiting for NTP time");
      rtcAvailable = false;
    } else {
      rtcAvailable = true;
    }
  }

  setupNTP();
  
  // ตั้งค่า MQTT
  espClient.setInsecure(); // ไม่ตรวจสอบ Certificate
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(30);
  client.setCallback(mqttCallback);
  
  // เปิดใช้งาน Watchdog Timer
  ESP.wdtEnable(WDTO_8S); // รีเซ็ตบอร์ดถ้าค้างเกิน 8 วินาที
  
  Serial.println("System Initialized.");
}

// ==========================================
// Loop Function
// ==========================================
void loop() {
  // รีเซ็ต Watchdog Timer ทุกรอบ
  ESP.wdtFeed();
  
  // ตรวจสอบการเชื่อมต่อ WiFi (Auto Reconnect)
  if (WiFi.status() != WL_CONNECTED) {
    return; // ข้ามการทำงานส่วนอื่นไปก่อน
  }
  updateNTPStatus();
  ArduinoOTA.handle();
  
  // จัดการ MQTT
  if (!client.connected()) {
    connectMQTT();
  } else {
    client.loop();
  }
  
  unsigned long currentMillis = millis();
  
  // 1. ตรวจสอบเวลาทุก 1 วินาที (สำหรับส่งเวลาและเช็คตารางเวลา)
  if (currentMillis - lastRTCUpdate >= RTC_INTERVAL) {
    lastRTCUpdate = currentMillis;
    if (client.connected()) publishTime();
    checkSchedule();
  }
  
  // 2. ส่ง Heartbeat ทุก 30 วินาที
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentMillis;
    if (client.connected()) publishHeartbeat();
  }

  // 3. อ่านและส่งค่า DHT11 ทุก 5 วินาที
  if (currentMillis - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = currentMillis;
    publishDHT();
  }
}
