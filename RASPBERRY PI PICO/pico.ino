// Raspberry_Pico_Logger_Final.ino
// Firewall forensic logger on Raspberry Pi Pico
// Receives JSON alerts from Arduino UNO, validates, stores, rotates logs

#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>   // For timestamp & date handling (install Time library)

// ================== PIN CONFIG ==================
#define SD_CS_PIN   5       // Chip Select for SD card (adjust to wiring)
#define LED_PIN     25      // Built-in LED on Pico
#define ARDUINO_RX  0       // Pico RX (connect to Arduino TX)
#define ARDUINO_TX  1       // Pico TX (connect to Arduino RX)

// ================== GLOBALS ==================
File logFile;
String currentDate = "";

// ================== SETUP ==================
void setup() {
  Serial1.begin(115200);    // UART: Arduino ↔ Pico
  Serial.begin(115200);     // USB Debug
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD card init failed!");
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(200); }
  }
  Serial.println("✅ SD card initialized");

  // Initialize fake time (replace with RTC or NTP if available)
  setTime(12, 0, 0, 14, 9, 2025); // (HH,MM,SS,DD,MM,YYYY)
}

// ================== HELPER FUNCTIONS ==================
String getDateString() {
  char buf[11];
  sprintf(buf, "%04d-%02d-%02d", year(), month(), day());
  return String(buf);
}

void openLogFile() {
  currentDate = getDateString();
  String filename = currentDate + ".log";
  logFile = SD.open(filename, FILE_WRITE);
  if (logFile) {
    logFile.println("=== Log Started for " + currentDate + " ===");
    logFile.flush();
    Serial.println("📂 Logging to: " + filename);
  } else {
    Serial.println("❌ Failed to open log file");
  }
}

String bytesToHex(const char* data, size_t len) {
  String hexDump = "";
  for (size_t i = 0; i < len; i++) {
    char buf[4];
    sprintf(buf, "%02X ", (uint8_t)data[i]);
    hexDump += buf;
  }
  return hexDump;
}

// ================== MAIN LOOP ==================
void loop() {
  // Rotate log file daily
  String today = getDateString();
  if (today != currentDate) {
    if (logFile) logFile.close();
    openLogFile();
  }

  // Check incoming alerts from Arduino
  if (Serial1.available()) {
    String jsonMsg = Serial1.readStringUntil('\n');
    jsonMsg.trim();

    if (jsonMsg.length() == 0) return;

    Serial.print("🔔 From Arduino: ");
    Serial.println(jsonMsg);

    // Parse JSON
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, jsonMsg);
    if (err) {
      Serial.println("⚠️ Invalid JSON received");
      return;
    }

    // Extract fields
    const char* ts       = doc["ts"]      | "unknown";
    const char* src_ip   = doc["src_ip"]  | "unknown";
    const char* src_mac  = doc["src_mac"] | "unknown";
    const char* reason   = doc["reason"]  | "none";
    const char* raw_pkt  = doc["raw"]     | "";

    // Open file if not yet opened
    if (!logFile) openLogFile();

    // Log JSON summary
    if (logFile) {
      logFile.print("[");
      logFile.print(ts);
      logFile.print("] IP: ");
      logFile.print(src_ip);
      logFile.print(" MAC: ");
      logFile.print(src_mac);
      logFile.print(" REASON: ");
      logFile.println(reason);

      // Log raw packet hex (if available)
      if (strlen(raw_pkt) > 0) {
        logFile.print("RAW: ");
        logFile.println(raw_pkt);
      }

      logFile.flush();
    }

    // LED blink (confirmation)
    digitalWrite(LED_PIN, HIGH); delay(80);
    digitalWrite(LED_PIN, LOW);

    // Acknowledge back to Arduino
    Serial1.println("{\"status\":\"logged\",\"date\":\"" + currentDate + "\"}");
  }
}
