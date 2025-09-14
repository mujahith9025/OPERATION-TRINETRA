// ESP8266_firewall.ino
// Requires: ESP8266WiFi, PubSubClient, ArduinoJson
// Connect ESP8266 to WiFi and MQTT broker; blink LED on startup, Wi-Fi, and detection; evaluate IP↔MAC spoofing; forward alerts to Arduino.

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//////////////////// CONFIG ////////////////////
const char* WIFI_SSID = "DEAD-SEC_4G";       // Your Wi-Fi SSID
const char* WIFI_PASS = "192.168.4.1";   // Your Wi-Fi Password

const char* MQTT_SERVER = "192.168.0.101";     // Linux controller IP (Mosquitto)
const uint16_t MQTT_PORT = 1883;

const char* MQTT_PKT_TOPIC = "firewall/packets";  // Incoming packets
const char* MQTT_ALERT_TOPIC = "firewall/alerts"; // Alerts out

const uint8_t LED_PIN = LED_BUILTIN; // built-in LED (active LOW)

//////////////////// DETECTION SETTINGS ////////////////////
const int SPOOF_REPEAT_THRESHOLD = 3;       // Mismatches before alert
const unsigned long SPOOF_WINDOW_MS = 10000; // 10s window

struct Mapping {
  String mac;
  String ip;
  int count;
  unsigned long first_ts;
};

#define MAX_TRACKED 64
Mapping tracked[MAX_TRACKED];

//////////////////// LED HELPERS ////////////////////
#define LED_ON()  digitalWrite(LED_PIN, LOW)  // active LOW
#define LED_OFF() digitalWrite(LED_PIN, HIGH)

void blinkLED(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    LED_ON(); delay(onMs);
    LED_OFF(); delay(offMs);
  }
}

void blinkStartup(int times=3, int onMs=150, int offMs=150) {
  pinMode(LED_PIN, OUTPUT);
  blinkLED(times, onMs, offMs);
}

void alertBlink() {
  LED_ON(); delay(100);
  LED_OFF(); delay(100);
}

void waitBlink() {
  LED_ON(); delay(50);
  LED_OFF(); delay(50);
}

//////////////////// TRACKING ////////////////////
int findSlot(const String &mac, const String &ip) {
  unsigned long now = millis();
  for (int i = 0; i < MAX_TRACKED; i++) {
    if (tracked[i].mac == mac || tracked[i].ip == ip) return i;
  }
  for (int i = 0; i < MAX_TRACKED; i++) {
    if (tracked[i].mac.length() == 0 && tracked[i].ip.length() == 0) {
      tracked[i].mac = mac; tracked[i].ip = ip;
      tracked[i].count = 0; tracked[i].first_ts = now;
      return i;
    }
  }
  tracked[0].mac = mac; tracked[0].ip = ip; tracked[0].count = 0; tracked[0].first_ts = now;
  return 0;
}

void resetSlot(int idx) {
  tracked[idx].mac = "";
  tracked[idx].ip = "";
  tracked[idx].count = 0;
  tracked[idx].first_ts = 0;
}

//////////////////// MQTT & ACTIONS ////////////////////
WiFiClient espClient;
PubSubClient mqtt(espClient);

void sendAlertMQTT(const String &jsonStr) {
  mqtt.publish(MQTT_ALERT_TOPIC, jsonStr.c_str());
}

void forwardToArduino(const String &jsonStr) {
  Serial.println(jsonStr); // send newline-terminated JSON
}

//////////////////// DETECTION ////////////////////
void processPacketJson(const String &payload) {
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  const char* src_mac = doc["src_mac"] | "";
  const char* src_ip = doc["src_ip"] | "";
  const char* bssid = doc["bssid"] | "";
  const char* frame = doc["frame_type"] | "";
  unsigned long now = millis();

  if (strlen(src_mac) == 0 || strlen(src_ip) == 0) return;

  int idx = findSlot(String(src_mac), String(src_ip));
  Mapping &m = tracked[idx];

  bool mismatch = false;
  if (m.mac.length() && m.ip.length()) {
    if (m.mac != String(src_mac) || m.ip != String(src_ip)) mismatch = true;
  }

  if (mismatch) {
    if (m.first_ts == 0) m.first_ts = now;
    m.count++;
    if (now - m.first_ts > SPOOF_WINDOW_MS) {
      m.count = 1;
      m.first_ts = now;
    }
  } else {
    m.mac = String(src_mac);
    m.ip = String(src_ip);
    m.count = 0;
    m.first_ts = now;
  }

  if (m.count >= SPOOF_REPEAT_THRESHOLD) {
    StaticJsonDocument<512> alert;
    alert["ts"] = doc["ts"] | "";
    alert["src_mac"] = src_mac;
    alert["src_ip"] = src_ip;
    alert["bssid"] = bssid;
    alert["frame_type"] = frame;
    alert["reason"] = "ip_mac_spoof_repeat";
    alert["confidence"] = m.count;
    if (doc.containsKey("raw_hex")) {
      String raw = String((const char*)doc["raw_hex"]);
      if (raw.length() > 1000) raw = raw.substring(0,1000);
      alert["raw_hex"] = raw;
    }
    String out; serializeJson(alert, out);
    sendAlertMQTT(out);
    forwardToArduino(out);
    resetSlot(idx);

    // Rapid blink for detection
    for (int i = 0; i < 5; i++) { LED_ON(); delay(80); LED_OFF(); delay(80); }
  }
}

//////////////////// MQTT CALLBACK ////////////////////
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String s;
  for (unsigned int i = 0; i < length; i++) s += (char)payload[i];
  if (String(topic) == MQTT_PKT_TOPIC) {
    processPacketJson(s);
  }
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting MQTT...");
    if (mqtt.connect("esp8266_firewall_client")) {
      Serial.println("connected");
      mqtt.subscribe(MQTT_PKT_TOPIC);
    } else {
      Serial.printf("failed, rc=%d try again in 2s\n", mqtt.state());
      delay(2000);
    }
  }
}

//////////////////// SETUP ////////////////////
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  LED_OFF();

  blinkStartup(3,120,120);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    waitBlink(); // fast blink while connecting
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connect failed - enter alert loop");
    while(true) alertBlink(); // infinite slow blink
  }
  Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  connectMQTT();
}

//////////////////// LOOP ////////////////////
void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  if (Serial.available()) {
    String in = Serial.readStringUntil('\n');
    if (in.length() > 0) Serial.printf("From Arduino: %s\n", in.c_str());
  }
}
