# IKUNET

**Connecting the Unconnected**

A decentralized, internet-free communication platform powered by LoRa, enabling reliable messaging and alerts in areas without network infrastructure.

---

## 📌 Overview
Ikunet provides a resilient communication system for environments where internet or cellular connectivity is unreliable or unavailable. It enables real-time text messaging and emergency alerts using low-power, long-range communication.

---

## 🧠 Problem
- Limited or no internet access in rural areas
- Communication breakdown during outages or emergencies
- Dependence on telecom infrastructure

---

## 💡 Solution
Ikunet creates a **local, decentralized communication network** using LoRa and ESP32 devices.

- No internet required
- No mobile network required
- Works in isolated environments

---

## 🏗️ System Architecture

```
+-------------------+        LoRa        +-------------------+
|   Device A        |  <-------------->  |   Device B        |
| (ESP32 + LoRa)    |                   | (ESP32 + LoRa)    |
+--------+----------+                   +----------+--------+
         |                                         |
         | WiFi AP                                 | WiFi AP
         v                                         v
   +------------+                           +------------+
   |   User     |                           |   User     |
   | (Browser)  |                           | (Browser)  |
   +------------+                           +------------+
```

---

## ⚙️ How It Works
1. Each device creates a WiFi hotspot
2. Users connect via phone or laptop
3. Messages are sent through a web interface
4. Data is transmitted between devices using LoRa

---

## ✨ Features
- Internet-free messaging
- Emergency alert system with acknowledgment (ACK)
- Real-time communication
- Lightweight web interface
- Low power consumption

---

## 📍 Use Cases
- Rural communication
- Schools without internet
- Disaster and emergency response
- Local business coordination
- Community networks

---

## 📊 Current Status
- Prototype (MVP)
- Text messaging functional
- Alert system implemented
- Voice feature in development

---

## 🛠️ Tech Stack
- ESP32
- LoRa (433MHz)
- Arduino Framework
- SPIFFS
- ESPAsyncWebServer


---

## 🚀 Setup Guide

### Requirements
- ESP32 board
- LoRa module
- Arduino IDE

### Installation
```bash
git clone https://github.com/ajijijonathan/ikunet.git
```

### Configuration
```cpp
#define DEVICE_ID 1
```

### Run
- Upload code to ESP32
- Connect to WiFi AP
- Open browser at: http://192.168.4.1

---

## 📡 Demo
- Live Demo: (Add link)
- Video Demo: (Add link)

---

## 🌍 Vision
To build a scalable offline communication infrastructure that ensures connectivity for everyone, regardless of internet access.

---

## 🤝 Contribution
Contributions are welcome. Fork the repo and submit a pull request.

---

## 📄 License
MIT License

---

## 👤 Author
Jonathan Ajiji Luka

---

**Ikunet — Communication Without Limits**
