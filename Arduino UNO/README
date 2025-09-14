Arduino Uno – Operation Trinetra: AirShield
Interface & Filtering Layer for Real-Time Wireless Security
📌 Role of Arduino Uno

The Arduino Uno acts as the intermediate controller between the ESP8266 (Wi-Fi capture module) and the Raspberry Pi Pico (logging & storage unit).

Its primary purpose is to:

Filter incoming data packets received from the ESP8266

Manage requests (packet validation, communication handshakes)

Forward authorized data to the Raspberry Pi Pico for permanent logging

Ensure reliable communication between modules

⚙️ Features

✔️ Packet pre-processing and filtering
✔️ Command relay between ESP8266 and Raspberry Pi Pico
✔️ Basic attacker identification (flags suspicious packets)
✔️ Low-level request management
✔️ Lightweight & real-time response

🔄 Data Flow

ESP8266 captures Wi-Fi packets & suspicious activity

Arduino Uno receives packets via serial communication

Filters valid packets and marks suspected intrusions

Forwards clean + flagged data to Raspberry Pi Pico

Pico stores final logs on the SD card
