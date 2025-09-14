Operation Trinetra: AirShield
Real-Time Wireless Security for Defense Networks

📌 Problem Statement

Military Wi-Fi networks are at risk from scanning and probing attacks.
Current systems cannot detect, block, or log these threats effectively.

🚀 Solution – AirShield

AirShield is a hardware-based Wi-Fi security system built to monitor, analyze, and protect wireless networks in real-time, specifically designed for defense and critical infrastructure.

It provides:

Real-time packet monitoring
Automated threat detection & blocking
Permanent attacker logging
Web-based monitoring dashboard

⚙️ System Architecture
🔹 Components

ESP8266

Primary networking module
Captures Wi-Fi signals and packets
Works with Atheros AR9271 Wi-Fi adapter
Detects unauthorized access attempts & suspicious activities

Arduino Uno

Acts as interface between ESP8266 and Raspberry Pi Pico
Handles filtering, request management, and communication

Raspberry Pi Pico

Stores detailed logs on SD card
Logs include attacker IP, MAC, reason for blocking
Ensures permanent record-keeping

SD Card Module

Stores logs for post-event analysis

Web-Based Dashboard

Real-time packet flow monitoring (similar to Wireshark)
Hardware health & device status reports
Visualizations of active security events

🛡️ Core Functionalities

✔️ Real-time detection of unauthorized access & scanning attacks
✔️ Automatic blocking of attackers, even if IP/MAC are spoofed
✔️ Hardware-level packet inspection & analytics
✔️ Centralized monitoring through a dashboard
✔️ Permanent logging for audit & defense reporting

📊 Workflow Overview

ESP8266 + Atheros AR9271 → Captures Wi-Fi packets
Arduino Uno → Filters & forwards to Raspberry Pi Pico
Raspberry Pi Pico + SD Card → Logs attackers permanently
Web Dashboard → Displays real-time threat analytics

🔐 Why AirShield?

Tailored for defense networks
Works independently of OS-level software (hardware-layer protection)
Resistant to IP/MAC spoofing
Provides visibility & control in high-security environments

📸 System Diagram (to be added)

Include block diagram of ESP8266 → Arduino Uno → Raspberry Pi Pico → SD Card + Dashboard

👨‍💻 Team Name - Born To Exploit

1) Muthakeen Ansari
2) Mohamed Yasir
3) Mujahith
