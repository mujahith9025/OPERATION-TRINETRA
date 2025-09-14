// Arduino_UNO_Firewall_Interface.ino
// Acts as the "heart" of the hardware: ESP8266 → Arduino → Pico
// Forwards JSON alerts, monitors hardware, heartbeat, and basic filtering

#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// =================== PIN CONFIG ===================
// ESP8266 connection (SoftwareSerial)
#define ESP_RX_PIN 10  // Arduino RX from ESP TX
#define ESP_TX_PIN 11  // Arduino TX to ESP RX (optional)
SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);

// Pico connection (SoftwareSerial or hardware Serial1 on Mega)
#define PICO_RX_PIN 8  // Arduino RX from Pico TX
#define PICO_TX_PIN 9  // Arduino TX to Pico RX
SoftwareSerial picoSerial(PICO_RX_PIN, PICO_TX_PIN);

// LED for Arduino status
#define LED_PIN 13

// Heartbeat interval (ms)
#define HEARTBEAT_INTERVAL 10000

// ================== GLOBALS =====================
unsigned long lastHeartbeat = 0;

// ================== LED HELPERS =================
#define LED_ON()  digitalWrite(LED_PIN, HIGH)
#define LED_OFF() digitalWrite(LED_PIN, LOW)

void blinkLED(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    LED_ON(); delay(onMs);
    LED_OFF(); delay(offMs);
  }
}

// ================== SETUP ======================
void setup() {
  pinMode(LED_PIN, OUTPUT);
  LED_OFF();
  Serial.begin(115200);       // debug
  espSerial.begin(115200);    // ESP8266
  picoSerial.begin(115200);   // Pico

  // Startup blink
  blinkLED(3, 200, 200);
  Serial.println("Arduino UNO Firewall Interface started");
}

// ================== LOOP =======================
void loop() {
  // 1. Forward messages from ESP8266 → Pico
  if (espSerial.available()) {
    String jsonMsg = espSerial.readStringUntil('\n');
    Serial.print("From ESP: ");
    Serial.println(jsonMsg);

    // Optional: validate JSON
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, jsonMsg);
    if (!err) {
      // Forward to Pico
      picoSerial.println(jsonMsg);
      // Blink LED briefly to show forwarding
      LED_ON(); delay(50); LED_OFF();
    } else {
      Serial.println("Invalid JSON from ESP, discarded");
    }
  }

  // 2. Forward messages from Pico → ESP (if needed)
  if (picoSerial.available()) {
    String msg = picoSerial.readStringUntil('\n');
    Serial.print("From Pico: ");
    Serial.println(msg);
    espSerial.println(msg);
    LED_ON(); delay(50); LED_OFF();
  }

  // 3. Send heartbeat to Pico
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    StaticJsonDocument<128> heartbeat;
    heartbeat["type"] = "heartbeat";
    heartbeat["timestamp"] = millis();
    String hbStr;
    serializeJson(heartbeat, hbStr);
    picoSerial.println(hbStr);
    lastHeartbeat = millis();
    LED_ON(); delay(50); LED_OFF();
  }
}
