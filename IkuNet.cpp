#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// ====== CONFIGURE PER DEVICE ======
#define DEVICE_ID 1  // <<< CHANGE per device (1 or 2)

// ====== PIN DEFINITIONS ======
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define BUTTON_PIN 33
#define LED_PIN 27
#define BUZZER_PIN 25
#define BUILTIN_LED 2  // For debugging

// ====== GLOBALS ======
AsyncWebServer server(80);
unsigned long lastSent = 0;
bool lastButtonState = HIGH;

// ACK tracking
bool waitingForAck = false;
unsigned long ackTimeout = 0;
#define ACK_TIMEOUT 5000  // 5 seconds

// Store recent messages in memory
struct ChatMsg {
  String from;
  String type; // "text","alert","ack"
  String text;
  bool acknowledged;
};
#define MAX_MSGS 20
ChatMsg messages[MAX_MSGS];
int msgIndex = 0;

void addMessage(String from, String type, String text, bool acknowledged = false) {
  int idx = msgIndex % MAX_MSGS;
  messages[idx].from = from;
  messages[idx].type = type;
  messages[idx].text = text;
  messages[idx].acknowledged = acknowledged;
  msgIndex++;
}

// ====== SEND TEXT FUNCTION ======
void sendTextMessage(String text) {
  String msg = "ID:" + String(DEVICE_ID) + "|TEXT|" + text;
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  Serial.println("Sent: " + msg);

  // Visual feedback
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);

  addMessage("me", "text", text);
}

// ====== SEND ALERT WITH ACK ======
void sendAlert() {
  String msg = "ID:" + String(DEVICE_ID) + "|ALERT";
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  Serial.println("Alert sent: " + msg);

  waitingForAck = true;
  ackTimeout = millis() + ACK_TIMEOUT;

  // Visual feedback
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  addMessage("me", "alert", "Alert sent - waiting for ACK");
}

// ====== SEND ACK ======
void sendAck(String toDeviceId) {
  String msg = "ID:" + String(DEVICE_ID) + "|ACK|TO:" + toDeviceId;
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  Serial.println("ACK sent: " + msg);
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== IkuNet Node ===");
  Serial.println("Device ID: " + String(DEVICE_ID));

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);

  // --- Init WiFi AP ---
  String ssidName = "IkuNet_Node" + String(DEVICE_ID);
  WiFi.softAP(ssidName.c_str(), "12345678");
  Serial.println("WiFi AP: " + ssidName);
  Serial.println("IP: " + WiFi.softAPIP().toString());

  // --- Web Server Routes ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>IkuNet - Node )rawliteral" + String(DEVICE_ID) + R"rawliteral(</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        body { font-family: Arial; background: #0d1117; color: #c9d1d9; margin:0; padding:10px; }
        .container { max-width: 800px; margin: 0 auto; }
        h2 { text-align:center; color: #00b4a6; margin-bottom: 10px; }
        .status { background: #161b22; padding:10px; border-radius:8px; margin:10px 0; text-align:center; font-size:0.85em; color:#6a9f6a; }
        #chatbox { height: 60vh; overflow-y: auto; padding:15px; background:#161b22; border-radius:8px; margin:10px 0; }
        .msg { padding:10px 15px; margin:8px 0; border-radius:15px; max-width:75%; clear:both; }
        .me { background:#238636; float:right; text-align:right; }
        .other { background:#21262d; float:left; border:1px solid #30363d; }
        .alert { background:#da3633; text-align:center; width:90%; margin:10px auto; float:none; }
        .ack { background:#d29922; font-style:italic; font-size:0.9em; }
        .controls { background:#161b22; padding:15px; border-radius:8px; }
        input[type=text] { flex:1; padding:12px; border-radius:8px; border:1px solid #30363d; background:#0d1117; color:white; font-size:16px; }
        input[type=submit], button { padding:12px 20px; border:none; border-radius:8px; background:#238636; color:white; cursor:pointer; font-weight:bold; font-size:15px; }
        .alert-btn { background:#da3633; width:100%; margin-top:10px; padding:16px; font-size:16px; }
        .row { display:flex; gap:10px; margin:10px 0; }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>📡 IkuNet Node )rawliteral" + String(DEVICE_ID) + R"rawliteral(</h2>
        <div class="status" id="status">✅ Connected to Node )rawliteral" + String(DEVICE_ID) + R"rawliteral( &nbsp;·&nbsp; Iku means to reach.</div>
        <div id="chatbox"></div>

        <div class="controls">
          <div class="row">
            <form id="textForm" style="display:flex; gap:10px; width:100%;">
              <input type="text" id="msg" name="msg" placeholder="Type message..." required>
              <input type="submit" value="Send">
            </form>
          </div>
          <button class="alert-btn" id="alertBtn">🚨 EMERGENCY ALERT</button>
        </div>
      </div>

      <script>
        async function loadChat() {
          try {
            let res = await fetch('/messages');
            let msgs = await res.json();
            let box = document.getElementById('chatbox');
            box.innerHTML = "";

            msgs.forEach(m => {
              let div = document.createElement("div");
              div.className = "msg " +
                (m.from == 'me' ? 'me' : 'other') +
                (m.type == 'alert' ? ' alert' : '') +
                (m.type == 'ack' ? ' ack' : '');

              if (m.type == "text") {
                div.innerText = m.text;
              } else if (m.type == "alert") {
                let ackIcon = m.acknowledged ? " ✅" : " ⏳";
                div.innerHTML = "<b>🚨 ALERT!</b>" + ackIcon;
              } else if (m.type == "ack") {
                div.innerHTML = "✓ Acknowledged by " + m.text;
              }
              box.appendChild(div);
            });
            box.scrollTop = box.scrollHeight;
          } catch (e) {
            console.error("Failed to load chat:", e);
          }
        }

        setInterval(loadChat, 1000);
        loadChat();

        document.getElementById("textForm").addEventListener("submit", async (e) => {
          e.preventDefault();
          let val = document.getElementById("msg").value.trim();
          if (val === "") return;
          try {
            await fetch("/sendText", {
              method: "POST",
              headers: { "Content-Type": "application/x-www-form-urlencoded" },
              body: new URLSearchParams({ msg: val })
            });
            document.getElementById("msg").value = "";
            loadChat();
          } catch (e) {
            alert("Failed to send message");
          }
        });

        document.getElementById("alertBtn").addEventListener("click", async () => {
          if (confirm("Send emergency alert?")) {
            try {
              await fetch("/sendAlert", { method: "POST" });
              loadChat();
            } catch (e) {
              alert("Failed to send alert");
            }
          }
        });
      </script>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", page);
  });

  server.on("/sendText", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("msg", true)) {
      String message = request->getParam("msg", true)->value();
      sendTextMessage(message);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing message");
    }
  });

  server.on("/sendAlert", HTTP_POST, [](AsyncWebServerRequest *request) {
    sendAlert();
    request->send(200, "text/plain", "Alert sent");
  });

  server.on("/messages", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    int start = max(0, msgIndex - MAX_MSGS);
    for (int i = start; i < msgIndex; i++) {
      int idx = i % MAX_MSGS;
      json += "{";
      json += "\"from\":\"" + messages[idx].from + "\",";
      json += "\"type\":\"" + messages[idx].type + "\",";
      json += "\"text\":\"" + messages[idx].text + "\",";
      json += "\"acknowledged\":" + String(messages[idx].acknowledged ? "true" : "false");
      json += "}";
      if (i < msgIndex - 1) json += ",";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web server started");

  // --- Init LoRa ---
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed! Check wiring.");
    while (true);
  }

  // CRITICAL SETTINGS (MUST MATCH ON BOTH DEVICES!)
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0xF3);
  LoRa.setPreambleLength(8);
  LoRa.idle();

  Serial.println("✅ LoRa initialized at 433MHz");
  Serial.println("========================");
  Serial.println("System Ready!");
  Serial.println("Connect to WiFi: IkuNet_Node" + String(DEVICE_ID));
  Serial.println("Password: 12345678");
  Serial.println("Go to: http://192.168.4.1");
  Serial.println("========================");
}

// ====== LOOP ======
void loop() {
  static unsigned long lastStatus = 0;

  // Status update every 30 seconds
  if (millis() - lastStatus > 30000) {
    Serial.println("System active - Messages: " + String(msgIndex));
    lastStatus = millis();
  }

  // Check ACK timeout
  if (waitingForAck && millis() > ackTimeout) {
    Serial.println("⏰ ACK timeout");
    waitingForAck = false;
    for (int i = max(0, msgIndex - 10); i < msgIndex; i++) {
      int idx = i % MAX_MSGS;
      if (messages[idx].type == "alert" && messages[idx].from == "me") {
        messages[idx].acknowledged = false;
        messages[idx].text = "Alert sent - NO ACK";
        break;
      }
    }
  }

  // Button alert
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastSent > 3000) {
    sendAlert();
    lastSent = millis();
  }
  lastButtonState = buttonState;

  // Receive LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    digitalWrite(BUILTIN_LED, HIGH);

    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    Serial.println("\n📡 Received " + String(packetSize) + " bytes");
    Serial.println("RSSI: " + String(LoRa.packetRssi()) + " dBm");

    if (incoming.length() > 50) {
      Serial.println("Message: " + incoming.substring(0, 50) + "...");
    } else {
      Serial.println("Message: " + incoming);
    }

    // Ignore own messages
    if (incoming.startsWith("ID:" + String(DEVICE_ID) + "|")) {
      Serial.println("⚠️ Ignoring own message");
      digitalWrite(BUILTIN_LED, LOW);
      return;
    }

    // ACK Message
    if (incoming.indexOf("|ACK|") != -1) {
      Serial.println("✅ ACK received");

      int toIndex = incoming.indexOf("|TO:") + 4;
      int toEnd = incoming.indexOf("|", toIndex);
      if (toEnd == -1) toEnd = incoming.length();
      String targetDevice = incoming.substring(toIndex, toEnd);

      if (targetDevice.toInt() == DEVICE_ID) {
        waitingForAck = false;

        int idStart = incoming.indexOf("ID:") + 3;
        int idEnd = incoming.indexOf("|", idStart);
        String ackFrom = incoming.substring(idStart, idEnd);

        Serial.println("ACK from device " + ackFrom);
        addMessage("other", "ack", "Device " + ackFrom);

        // Update alert status
        for (int i = max(0, msgIndex - 10); i < msgIndex; i++) {
          int idx = i % MAX_MSGS;
          if (messages[idx].type == "alert" && messages[idx].from == "me") {
            messages[idx].acknowledged = true;
            messages[idx].text = "Alert ACK from device " + ackFrom;
            break;
          }
        }

        // Visual feedback
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(100);
          digitalWrite(LED_PIN, LOW);
          delay(100);
        }
      }
    }
    // ALERT Message
    else if (incoming.indexOf("|ALERT") != -1) {
      Serial.println("🚨 ALERT received");

      int idStart = incoming.indexOf("ID:") + 3;
      int idEnd = incoming.indexOf("|", idStart);
      String alertFrom = (idEnd != -1) ? incoming.substring(idStart, idEnd) : "unknown";

      Serial.println("From device: " + alertFrom);

      // Audio/visual alert
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(300);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        delay(300);
      }

      addMessage("other", "alert", "Alert from device " + alertFrom);

      // Send ACK
      if (alertFrom != "unknown") {
        sendAck(alertFrom);
      }
    }
    // TEXT Message
    else if (incoming.indexOf("|TEXT|") != -1) {
      Serial.println("💬 TEXT received");

      int msgStart = incoming.indexOf("|TEXT|") + 6;
      if (msgStart < incoming.length()) {
        String text = incoming.substring(msgStart);

        int idStart = incoming.indexOf("ID:") + 3;
        int idEnd = incoming.indexOf("|", idStart);
        String sender = (idEnd != -1) ? incoming.substring(idStart, idEnd) : "unknown";

        Serial.println("From " + sender + ": " + text);

        // Visual feedback
        for (int i = 0; i < 2; i++) {
          digitalWrite(BUILTIN_LED, HIGH);
          delay(100);
          digitalWrite(BUILTIN_LED, LOW);
          delay(100);
        }

        addMessage("other", "text", "[" + sender + "]: " + text);
      }
    }
    else {
      Serial.println("❓ Unknown format");
    }

    digitalWrite(BUILTIN_LED, LOW);
  }

  delay(10);  // Prevent watchdog
}
