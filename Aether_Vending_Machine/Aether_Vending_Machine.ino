
/*
 * SANITARY VENDING MACHINE — ESP32-C3  v13.0
 * CINEMATIC BOOT SEQUENCE
 * Particle burst, glitch reveal, expanding logo, synced motor,
 * typewriter IP reveal, and a premium living READY screen.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ================================================
//  PINS & HARDWARE MAP (ESP32-C3 SUPER MINI)
// ================================================
// DRV8833 Stepper Motor Driver Input Pins
const int AIN1 = 5;
const int AIN2 = 6;
const int BIN1 = 7;
const int BIN2 = 8;

// Passive Piezo Buzzer Audio Notification Pin
const int BUZZER_PIN = 4;

// MFRC522 RFID SPI Connection Pins
#define RST_PIN  10
#define SS_PIN    0   // SDA / SPI Chip Select
#define MOSI_PIN  1   // SPI MOSI
#define MISO_PIN  2   // SPI MISO
#define SCK_PIN   3   // SPI Clock

// SSD1306 OLED Display I2C Connection Pins
#define OLED_SCL      20  // I2C Clock
#define OLED_SDA      21  // I2C Data
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32
#define OLED_RESET     -1  // No physical reset pin

// ================================================
//  MOTOR CONSTANTS
// ================================================
const int TOTAL_STEPS  = 4800;
const int MM_MAX       = 60;

const int SPEED_HOME   = 1800;
const int SPEED_SLOW   = 1000;
const int SPEED_CRUISE =  400;
const int SPEED_RAMP   =  400;
const int RAMP_DELTA   = (SPEED_SLOW - SPEED_CRUISE) / SPEED_RAMP;

// ================================================
//  DRV8833 HALF-STEP TABLE
// ================================================
const uint8_t HALF_STEP[8][4] = {
    {1, 0, 1, 0},
    {1, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 1},
    {0, 1, 0, 1},
    {1, 1, 0, 1},
    {1, 0, 0, 1},
    {1, 0, 1, 1},
};

// ================================================
//  NETWORKING
// ================================================
#define WS_PORT   81
#define HTTP_PORT 80

const int NUM_NETWORKS = 2;
const char* WIFI_SSIDS[NUM_NETWORKS]  = { "Taqi IP15", "VOO-2413CT6" };
const char* WIFI_PASSES[NUM_NETWORKS] = { "12345679", "12345679" };
String activeSSID = "None";

int  wifiNetIndex      = 0;
bool wifiServersUp     = false;
unsigned long wifiAttemptStart = 0;
const unsigned long WIFI_ATTEMPT_TIMEOUT = 3000;
const unsigned long WIFI_RETRY_GAP       = 1000;
unsigned long wifiNextActionAt = 0;

// ================================================
//  STATE
// ================================================
int  motorPos      = 0;
bool stopNow       = false;
int  cmdSteps      = -1;
bool motorBusy     = false;
bool oledDirty     = false;
bool ipShown       = false;
String globalState = "BOOT";
int  animationFrame = 0;

WebServer        server(HTTP_PORT);
WebSocketsServer ws(WS_PORT);
MFRC522          rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void updateOLED();
void handleSerialCommands();
void sendStatus(const char* state);
void transitionState(String newState);
void playTone(int frequency, int durationMs);
void wifiTask();
void runVendSequence();
void homing();
void scrollIP();
void cinematicBoot();

// ================================================
//  DASHBOARD HTML
// ================================================
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AETHER VEND OS v13.0</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
               background: #0a0c10; color: #e1e7ed; margin: 0; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .card { background: #12161f; border-radius: 12px; padding: 24px;
                box-shadow: 0 8px 24px rgba(0,0,0,0.5); margin-bottom: 20px;
                border: 1px solid #1a2230; }
        h1, h2 { margin-top: 0; color: #ffffff; letter-spacing: 0.5px; }
        h1 { font-size: 22px; text-transform: uppercase; color: #00ea87;
             border-bottom: 1px solid #1a2230; padding-bottom: 10px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .label { font-size: 12px; color: #768390; text-transform: uppercase; font-weight: 600; }
        .value { font-size: 18px; font-weight: bold; color: #ffffff; margin-top: 4px; }
        .oled-emulation { background: #000; border: 3px solid #222c3c; border-radius: 6px;
                          padding: 15px; text-align: center; font-family: "Courier New", monospace;
                          height: 40px; display: flex; align-items: center; justify-content: center;
                          margin: 15px 0; position: relative; }
        .oled-text { font-size: 24px; font-weight: bold; color: #00ccff; letter-spacing: 2px; }
        .oled-wifi-dot { position: absolute; top: 4px; right: 4px; width: 4px; height: 4px;
                         background: #00ccff; border-radius: 50%; display: none; }
        .btn { display: block; width: 100%; background: #00ea87; color: #0a0c10; border: none;
               padding: 14px; font-size: 15px; font-weight: bold; border-radius: 8px;
               cursor: pointer; text-transform: uppercase; transition: all 0.2s ease;
               text-align: center; margin-top: 10px; box-sizing: border-box; }
        .btn:hover { background: #00b769; transform: translateY(-1px); }
        .btn-secondary { background: #1f293d; color: #fff; border: 1px solid #2d3d5a; }
        .btn-secondary:hover { background: #28354f; }
    </style>
</head>
<body onload="init()">
<div class="container">
    <div class="card">
        <h1>AETHER TECHNOLOGIES</h1>
        <div class="label">Hardware Panel Output Emulation</div>
        <div class="oled-emulation">
            <div id="wifiDot" class="oled-wifi-dot"></div>
            <div id="oledText" class="oled-text">BOOT</div>
        </div>
        <div class="grid">
            <div>
                <div class="label">Machine Status</div>
                <div id="sysState" class="value" style="color:#00ea87;">STANDBY</div>
            </div>
            <div>
                <div class="label">Linear Displacement</div>
                <div id="linearPos" class="value">0 mm</div>
            </div>
        </div>
    </div>
    <div class="card">
        <h2>OPERATIONAL COMMANDS</h2>
        <button class="btn" onclick="sendCmd('vendswipe')">Dispense Product (Simulate Card)</button>
        <button class="btn btn-secondary" onclick="sendCmd('home')">Calibrate Zero Axis (Home)</button>
    </div>
    <div class="card">
        <h2>NETWORK DIAGNOSTICS</h2>
        <div class="grid">
            <div>
                <div class="label">Connected SSID</div>
                <div id="netSsid" class="value">Searching...</div>
            </div>
            <div>
                <div class="label">Signal Strength</div>
                <div id="netRssi" class="value">0 dBm</div>
            </div>
        </div>
    </div>
</div>
<script>
    var ws;
    function init() {
        ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        ws.onmessage = function(evt) {
            var d = JSON.parse(evt.data);
            document.getElementById("linearPos").innerText = d.pos_mm + " mm";
            document.getElementById("netSsid").innerText   = d.wifi_ssid;
            document.getElementById("netRssi").innerText   = d.wifi_rssi + " dBm";
            var s = d.state.toUpperCase();
            document.getElementById("sysState").innerText = s;
            var ot = document.getElementById("oledText");
            var dt = document.getElementById("wifiDot");
            dt.style.display = "block";
            if (s === "EXTENDING" || s === "RETRACTING" || s === "VENDING") {
                ot.innerText = "VENDING..."; ot.style.color = "#00ea87";
            } else if (s === "IDLE") {
                ot.innerText = "READY"; ot.style.color = "#00ccff";
            } else {
                ot.innerText = s; ot.style.color = "#ffcc00";
            }
        };
        ws.onclose = function() {
            document.getElementById("oledText").innerText = "OFFLINE";
            document.getElementById("oledText").style.color = "#ff3333";
            document.getElementById("wifiDot").style.display = "none";
            setTimeout(init, 2000);
        };
    }
    function sendCmd(msg) {
        if (ws && ws.readyState === WebSocket.OPEN) ws.send(msg);
    }
</script>
</body>
</html>
)=====";

// ================================================
//  BUZZER
// ================================================
void playTone(int frequency, int durationMs) {
    if (frequency <= 0) return;
    int halfPeriod = 1000000 / (frequency * 2);
    unsigned long start = millis();
    while (millis() - start < (unsigned long)durationMs) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

// Quick non blocking-ish swept tone for whooshes
void sweepTone(int fStart, int fEnd, int durationMs) {
    int steps = 20;
    int stepMs = durationMs / steps;
    if (stepMs < 1) stepMs = 1;
    for (int i = 0; i <= steps; i++) {
        int f = fStart + (fEnd - fStart) * i / steps;
        playTone(f, stepMs);
    }
}

// ================================================
//  STATE CONTROLLER
// ================================================
void transitionState(String newState) {
    globalState = newState;
    oledDirty   = true;
    updateOLED();
    if (globalState == "READY") {
        playTone(2800, 10);
        delay(20);
        playTone(3200, 15);
    } else if (globalState == "WAIT") {
        playTone(900, 25);
    } else if (globalState == "COMPLETE") {
        int melody[5] = {1960, 2200, 2637, 2937, 3492};
        for (int i = 0; i < 5; i++) { playTone(melody[i], 15); delay(5); }
    }
}

// ================================================
//  MOTOR DRIVER
// ================================================
inline void doStep(int stepIndex) {
    int s = ((stepIndex % 8) + 8) % 8;
    digitalWrite(AIN1, HALF_STEP[s][0]);
    digitalWrite(AIN2, HALF_STEP[s][1]);
    digitalWrite(BIN1, HALF_STEP[s][2]);
    digitalWrite(BIN2, HALF_STEP[s][3]);
}

void stopCoils() {
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

void brakeCoils() {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
}

// ================================================
//  TRAVEL ENGINE
// ================================================
void moveTo(int target) {
    target = constrain(target, 0, TOTAL_STEPS);
    if (target == motorPos) { brakeCoils(); return; }

    motorBusy = true;
    bool fwd        = (target > motorPos);
    int stepsToMove = abs(target - motorPos);
    int currentDelay = SPEED_SLOW;
    int decelStart  = stepsToMove - SPEED_RAMP;

    if (globalState != "VENDING") transitionState("VENDING");
    sendStatus(fwd ? "extending" : "retracting");

    for (int i = 0; i < stepsToMove; i++) {
        if (stopNow) break;
        if (fwd) { motorPos++; } else { motorPos--; }
        doStep(motorPos);

        if (i < SPEED_RAMP && currentDelay > SPEED_CRUISE) {
            currentDelay -= RAMP_DELTA;
            if (currentDelay < SPEED_CRUISE) currentDelay = SPEED_CRUISE;
        } else if (i >= decelStart && currentDelay < SPEED_SLOW) {
            currentDelay += RAMP_DELTA;
            if (currentDelay > SPEED_SLOW) currentDelay = SPEED_SLOW;
        }
        delayMicroseconds(currentDelay);

        if (i % 200 == 0) {
            if (wifiServersUp) { ws.loop(); server.handleClient(); }
            animationFrame++;
            oledDirty = true;
        }
    }

    brakeCoils();
    motorBusy = false;
    stopNow   = false;
}

// Raw move used inside cinematic boot (no state change, returns to start tracking)
void rawMove(int steps, bool forward, int speedUs) {
    for (int i = 0; i < steps; i++) {
        if (forward) { motorPos++; } else { motorPos--; }
        motorPos = constrain(motorPos, 0, TOTAL_STEPS);
        doStep(motorPos);
        delayMicroseconds(speedUs);
    }
}

// ================================================
//  CINEMATIC BOOT SEQUENCE
//  Drawn entirely with direct display calls (not gated
//  by oledDirty) since it owns the screen during boot.
//
//  Five acts:
//    ACT 1  Particle starfield warp-in
//    ACT 2  Glitch flicker reveal of "AETHER"
//    ACT 3  Logo locks, energy rings pulse out, motor jolts
//    ACT 4  Loading bar fills, synced to a full motor sweep
//    ACT 5  "TECHNOLOGIES" wipes in, screen flash, settle
// ================================================
void cinematicBoot() {

    // ---------- ACT 1: PARTICLE WARP-IN ----------
    // Stars stream from center outward, accelerating.
    const int NSTAR = 26;
    float sx[NSTAR], sy[NSTAR], sa[NSTAR];  // x, y, angle
    for (int i = 0; i < NSTAR; i++) {
        sa[i] = (float)random(0, 628) / 100.0; // 0..2pi
        sx[i] = 64; sy[i] = 16;
    }
    playTone(300, 20);
    for (int frame = 0; frame < 36; frame++) {
        display.clearDisplay();
        float speed = 0.4 + frame * 0.12;  // accelerating warp
        for (int i = 0; i < NSTAR; i++) {
            sx[i] += cos(sa[i]) * speed;
            sy[i] += sin(sa[i]) * speed * 0.5; // squash vertically for 128x32
            // streak: draw a short trail
            int tx = sx[i] - cos(sa[i]) * 3;
            int ty = sy[i] - sin(sa[i]) * 1.5;
            if (sx[i] >= 0 && sx[i] < SCREEN_WIDTH && sy[i] >= 0 && sy[i] < SCREEN_HEIGHT) {
                display.drawLine(tx, ty, (int)sx[i], (int)sy[i], SSD1306_WHITE);
            } else {
                // respawn star at center
                sa[i] = (float)random(0, 628) / 100.0;
                sx[i] = 64; sy[i] = 16;
            }
        }
        display.display();
        if (frame % 6 == 0) playTone(400 + frame * 40, 3);
        delay(22);
    }

    // ---------- ACT 2: GLITCH REVEAL ----------
    // "AETHER" flickers in with horizontal glitch slices.
    for (int frame = 0; frame < 16; frame++) {
        display.clearDisplay();
        display.setTextSize(3);
        display.setTextColor(SSD1306_WHITE);

        // Random horizontal offset glitch on some frames
        int jitter = (frame % 3 == 0) ? random(-6, 7) : 0;
        display.setCursor(8 + jitter, 4);
        // Flicker: skip drawing on some frames for strobe feel
        if (frame % 4 != 1) display.print("AETHER");

        // Glitch slices: invert random horizontal bands
        if (frame % 2 == 0) {
            int by = random(0, SCREEN_HEIGHT - 4);
            display.fillRect(0, by, SCREEN_WIDTH, random(1, 4), SSD1306_INVERSE);
        }
        // Static noise dots
        for (int n = 0; n < 8; n++) {
            display.drawPixel(random(0, SCREEN_WIDTH), random(0, SCREEN_HEIGHT), SSD1306_WHITE);
        }
        display.display();
        playTone(random(800, 2600), 2); // glitchy chirps
        delay(45);
    }

    // ---------- ACT 3: LOGO LOCK + ENERGY RINGS + MOTOR JOLT ----------
    // Logo snaps clean, concentric rings burst outward.
    sweepTone(600, 2400, 120);
    rawMove(120, true, 600);   // sharp motor jolt forward (the "power on" kick)
    for (int r = 0; r < 70; r += 6) {
        display.clearDisplay();
        display.setTextSize(3);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(8, 4);
        display.print("AETHER");
        // expanding ring from center
        display.drawCircle(64, 16, r, SSD1306_WHITE);
        display.drawCircle(64, 16, r / 2, SSD1306_WHITE);
        display.display();
        playTone(2000 - r * 12, 4);
        delay(28);
    }
    rawMove(120, false, 600);  // motor settles back

    // ---------- ACT 4: LOADING BAR SYNCED TO MOTOR SWEEP ----------
    // Bar fills left to right while motor does one smooth full sweep.
    // We interleave: each bar segment = a chunk of motor steps.
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(30, 2);
    display.print("INITIALIZING");
    display.drawRect(8, 18, 112, 10, SSD1306_WHITE);
    display.display();
    delay(150);

    const int SEGMENTS = 24;
    const int stepsPerSeg = 60; // 24 * 60 = 1440 steps total sweep out
    for (int seg = 0; seg < SEGMENTS; seg++) {
        // fill this bar segment
        int fillW = (108 * (seg + 1)) / SEGMENTS;
        display.fillRect(10, 20, fillW, 6, SSD1306_WHITE);
        // percentage text
        display.fillRect(40, 2, 50, 10, SSD1306_BLACK);
        display.setCursor(48, 2);
        display.setTextSize(1);
        display.print((seg + 1) * 100 / SEGMENTS);
        display.print("%");
        display.display();

        // motor moves a chunk forward, synced to the fill
        rawMove(stepsPerSeg, true, 700);
        playTone(1200 + seg * 50, 3);
    }
    // bring motor back home smoothly after the sweep
    rawMove(SEGMENTS * stepsPerSeg, false, 500);

    // ---------- ACT 5: TECHNOLOGIES WIPE + FLASH ----------
    // Wipe in subtitle then full screen flash, then settle.
    String sub = "TECHNOLOGIES";
    for (int w = 0; w <= SCREEN_WIDTH; w += 8) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 2);
        display.print("AETHER");
        display.setTextSize(1);
        display.setCursor(28, 22);
        display.print(sub);
        // wipe mask: black rectangle retreating to the right
        display.fillRect(w, 18, SCREEN_WIDTH - w, 14, SSD1306_BLACK);
        display.display();
        delay(18);
    }
    delay(250);

    // Triple flash
    for (int f = 0; f < 3; f++) {
        display.fillScreen(SSD1306_WHITE);
        display.display();
        playTone(3200, 12);
        delay(40);
        display.clearDisplay();
        display.display();
        delay(60);
    }

    // Final settle frame
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 2);
    display.print("AETHER");
    display.setTextSize(1);
    display.setCursor(28, 22);
    display.print(sub);
    display.display();
    sweepTone(2400, 600, 100);
    delay(400);
}

// ================================================
//  HOMING
// ================================================
void homing() {
    Serial.println("[HOMING] Start...");
    transitionState("WAIT");
    motorBusy = true;

    int homingSteps = TOTAL_STEPS + 400;
    for (int i = 0; i < homingSteps; i++) {
        doStep(-i);
        delayMicroseconds(SPEED_HOME);
        if (i % 200 == 0) { animationFrame++; oledDirty = true; }
    }
    delay(100);
    for (int i = 0; i < 60; i++) { doStep(i);      delayMicroseconds(SPEED_HOME); }
    delay(60);
    for (int i = 0; i < 60; i++) { doStep(60 - i); delayMicroseconds(SPEED_HOME); }

    brakeCoils();
    motorPos  = 0;
    motorBusy = false;
    transitionState("READY");
    Serial.println("[HOMING] Zero set.");
}

// ================================================
//  WEBSOCKET BROADCAST
// ================================================
void sendStatus(const char* state) {
    if (!wifiServersUp) return;
    int mm = map(motorPos, 0, TOTAL_STEPS, 0, MM_MAX);
    char buf[200];
    snprintf(buf, sizeof(buf),
        "{\"pos_mm\":%d,\"state\":\"%s\",\"wifi_ssid\":\"%s\",\"wifi_rssi\":%d,\"wifi_ip\":\"%s\"}",
        mm, state, activeSSID.c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());
    ws.broadcastTXT(buf);
}

void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    switch (type) {
        case WStype_CONNECTED: sendStatus("idle"); break;
        case WStype_TEXT: {
            String msg = String((char*)payload, len);
            msg.trim();
            if      (msg == "stop")      { stopNow = true; }
            else if (msg == "home")      { cmdSteps = -2; stopNow = false; }
            else if (msg == "vendswipe") { cmdSteps = 9999; stopNow = false; }
            else {
                int mm = msg.toInt();
                if (mm >= 0 && mm <= MM_MAX) {
                    cmdSteps = map(mm, 0, MM_MAX, 0, TOTAL_STEPS);
                    stopNow  = false;
                }
            }
            break;
        }
        default: break;
    }
}

// ================================================
//  HTTP
// ================================================
void handleRootFile() { server.send_P(200, "text/html", INDEX_HTML); }

// ================================================
//  PREMIUM IP REVEAL — TYPEWRITER + RADAR
//  Types out the IP char by char with a blinking cursor
//  and a sweeping radar arc, then holds, then dissolves.
// ================================================
void scrollIP() {
    if (WiFi.status() != WL_CONNECTED) return;
    String ip   = WiFi.localIP().toString();
    String head = "NETWORK ONLINE";

    // Intro: "NETWORK ONLINE" pulses in
    for (int b = 0; b < 3; b++) {
        display.clearDisplay();
        if (b % 2 == 0) {
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(18, 12);
            display.print(head);
        }
        display.display();
        playTone(2000 + b * 300, 25);
        delay(140);
    }
    delay(150);

    // Typewriter the IP, char by char, with radar sweep behind
    String typed = "";
    for (unsigned int c = 0; c < ip.length(); c++) {
        typed += ip[c];
        display.clearDisplay();

        // radar sweep arc on left
        float ang = (c * 0.6);
        int rx = 14, ry = 16;
        display.drawCircle(rx, ry, 12, SSD1306_WHITE);
        display.drawLine(rx, ry, rx + cos(ang) * 12, ry + sin(ang) * 12, SSD1306_WHITE);

        // label
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(34, 2);
        display.print("IP ADDRESS");

        // typed IP, larger
        display.setCursor(34, 16);
        display.print(typed);
        // blinking cursor block
        if (c % 2 == 0) display.print("_");

        display.display();
        playTone(c % 2 ? 1500 : 1700, 6); // typewriter clicks
        delay(180); // readable typing pace
    }

    // Hold full IP steady so it can be read
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(6, 4);
    display.print("IP  ");
    display.print(ip);
    display.setCursor(6, 18);
    display.print("NET ");
    display.print(activeSSID);
    display.display();
    delay(4000); // comfortable reading hold

    // Dissolve: randomly erase pixels until blank
    for (int pass = 0; pass < 60; pass++) {
        for (int n = 0; n < 40; n++) {
            display.drawPixel(random(0, SCREEN_WIDTH), random(0, SCREEN_HEIGHT), SSD1306_BLACK);
        }
        display.display();
        delay(12);
    }

    ipShown = true;
}

// ================================================
//  NON-BLOCKING WIFI STATE MACHINE
// ================================================
void wifiTask() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiServersUp) {
            activeSSID = WIFI_SSIDS[wifiNetIndex == 0 ? NUM_NETWORKS - 1 : wifiNetIndex - 1];
            Serial.printf("[WIFI] Connected: %s | IP %s | RSSI %d\n",
                activeSSID.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
            server.on("/", handleRootFile);
            server.begin();
            ws.begin();
            ws.onEvent(wsEvent);
            wifiServersUp = true;
        }
        return;
    }

    unsigned long now = millis();
    if (now < wifiNextActionAt) return;

    if (wifiAttemptStart == 0 || (now - wifiAttemptStart > WIFI_ATTEMPT_TIMEOUT)) {
        WiFi.disconnect();
        Serial.printf("[WIFI] Trying: %s\n", WIFI_SSIDS[wifiNetIndex]);
        WiFi.begin(WIFI_SSIDS[wifiNetIndex], WIFI_PASSES[wifiNetIndex]);
        wifiAttemptStart = now;
        wifiNetIndex = (wifiNetIndex + 1) % NUM_NETWORKS;
        if (wifiNetIndex == 0) wifiNextActionAt = now + WIFI_RETRY_GAP;
    }
}

// ================================================
//  OLED — RUNTIME SCREENS (DIRTY FLAG GATED)
//  Upgraded READY screen with breathing logo and
//  flowing energy ribbon.
// ================================================
void updateOLED() {
    if (!oledDirty) return;
    oledDirty = false;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    bool linked = (WiFi.status() == WL_CONNECTED);

    // Subtle WiFi status indicator in the top-right corner
    if (linked) {
        // Solid 3x3 block when connected
        display.fillRect(SCREEN_WIDTH - 4, 0, 3, 3, SSD1306_WHITE);
    } else {
        // Blinking 3x3 block when connecting
        if ((animationFrame / 4) % 2 == 0) {
            display.fillRect(SCREEN_WIDTH - 4, 0, 3, 3, SSD1306_WHITE);
        }
    }



    if (globalState == "READY") {
        // Breathing logo: text size pulses subtly via position shimmer
        display.setTextSize(2);
        int shimmer = (int)(1.5 * sin(animationFrame * 0.18));
        display.setCursor(6, 2 + shimmer);
        display.print("AETHER");

        display.setTextSize(1);
        // Blinking TAP prompt
        if ((animationFrame / 8) % 2 == 0) {
            display.setCursor(90, 4);
            display.print("TAP");
        }

        // Dual flowing energy ribbons (two sine waves phase shifted)
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int y1 = 26 + (int)(3.5 * sin((x + animationFrame * 6) * 0.13));
            int y2 = 28 + (int)(2.5 * sin((x - animationFrame * 4) * 0.10 + 1.5));
            display.drawPixel(x, y1, SSD1306_WHITE);
            display.drawPixel(x, y2, SSD1306_WHITE);
        }
        // Travelling spark along the ribbon
        int spark = (animationFrame * 5) % SCREEN_WIDTH;
        int sy = 26 + (int)(3.5 * sin((spark + animationFrame * 6) * 0.13));
        display.fillCircle(spark, sy, 1, SSD1306_WHITE);
    }
    else if (globalState == "WAIT") {
        display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        if ((animationFrame / 2) % 2 == 0) {
            display.fillRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        }
        display.setTextSize(2);
        display.setCursor(40, 9);
        display.print("WAIT");
    }
    else if (globalState == "VENDING") {
        float t = animationFrame * 0.25;
        
        // Background: Streaming energy flow particles
        for (int p = 0; p < 12; p++) {
            int px = (p * 12 + (animationFrame * 2)) % SCREEN_WIDTH;
            int py = 16 + (int)(7.0 * sin((px + animationFrame * 4) * 0.06));
            display.drawPixel(px, py, SSD1306_WHITE);
        }

        // Foreground: Twisting 3D Double Helix with Spindle Envelope
        for (int x = 8; x < SCREEN_WIDTH - 8; x += 5) {
            // Spindle shape envelope (thicker in middle, tapers to zero at edges)
            float envelope = sin((x - 8) * 3.14159 / (SCREEN_WIDTH - 16));
            float amp = 14.0 * envelope;
            
            // Generate two anti-phase sine waves
            int y1 = 16 + (int)(amp * sin(x * 0.15 + t));
            int y2 = 16 - (int)(amp * sin(x * 0.15 + t));
            
            // Draw connection line (representing DNA bonds / energy linkage)
            display.drawLine(x, y1, x, y2, SSD1306_WHITE);
            
            // Draw nodes at endpoints
            display.fillRect(x - 1, y1 - 1, 3, 3, SSD1306_WHITE);
            display.fillRect(x - 1, y2 - 1, 3, 3, SSD1306_WHITE);
        }
    }
    else if (globalState == "COMPLETE") {
        display.setTextSize(2);
        display.setCursor(20, 2);
        display.print("THANK");
        display.setCursor(34, 18);
        display.print("YOU");
        // sparkle burst
        for (int s = 0; s < 6; s++) {
            float a = animationFrame * 0.3 + s * 1.05;
            int r = (animationFrame * 2) % 20;
            int x = 110 + cos(a) * r / 3;
            int y = 8 + sin(a) * r / 3;
            display.drawPixel(x, y, SSD1306_WHITE);
        }
    }

    display.display();
}

// ================================================
//  SHARED VEND SEQUENCE
// ================================================
void runVendSequence() {
    transitionState("VENDING");
    moveTo(TOTAL_STEPS);
    for (int f = 0; f < 20; f++) { animationFrame++; oledDirty = true; updateOLED(); delay(50); }
    moveTo(0);
    transitionState("COMPLETE");
    for (int f = 0; f < 30; f++) { animationFrame++; oledDirty = true; updateOLED(); delay(45); }
    transitionState("READY");
    sendStatus("idle");
}

// ================================================
//  SETUP
// ================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== AETHER VEND OS v13.0 | CINEMATIC ===");

    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(800000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        Serial.println("[ERROR] OLED failed.");

    randomSeed(esp_random());

    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    stopCoils();

    // The cinematic boot drives the motor itself, so run it
    // before homing. motorPos is reset by homing afterward.
    cinematicBoot();

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    rfid.PCD_Init();

    homing();

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Disable modem sleep to speed up connection handshake
    WiFi.begin(WIFI_SSIDS[0], WIFI_PASSES[0]);
    wifiAttemptStart = millis();
    wifiNetIndex     = 1;

    transitionState("READY");
    Serial.printf("[BOOT] Ready. Cruise: %dus = %d steps/sec\n",
        SPEED_CRUISE, 1000000 / SPEED_CRUISE);
}

// ================================================
//  LOOP
// ================================================
void loop() {
    wifiTask();

    if (wifiServersUp && !ipShown) {
        scrollIP();
        oledDirty = true;
    }

    if (wifiServersUp) {
        server.handleClient();
        ws.loop();
    }

    handleSerialCommands();

    if (!motorBusy) {
        static unsigned long lastOledTick = 0;
        if (millis() - lastOledTick > 45) {
            lastOledTick = millis();
            animationFrame++;
            oledDirty = true;
        }
    }
    updateOLED();

    if (cmdSteps == -2 && !motorBusy) {
        cmdSteps = -1;
        homing();
    }

    if (cmdSteps >= 0 && !motorBusy) {
        int target = cmdSteps;
        cmdSteps = -1;
        if (target == 9999) {
            runVendSequence();
        } else {
            transitionState("VENDING");
            moveTo(target);
            transitionState("READY");
            sendStatus("idle");
        }
    }

    if (!motorBusy && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        Serial.println("[RFID] Card detected.");
        playTone(4000, 10); playTone(4500, 10);
        runVendSequence();
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}

// ================================================
//  SERIAL COMMANDS
// ================================================
void handleSerialCommands() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();

    if      (cmd == "test")   { stopNow = false; runVendSequence(); }
    else if (cmd == "home")   { homing(); }
    else if (cmd == "stop")   { stopNow = true; }
    else if (cmd == "ip")     { ipShown = false; }
    else if (cmd == "boot")   { cinematicBoot(); transitionState("READY"); } // replay intro
    else if (cmd == "status") {
        Serial.printf("[STATUS] Pos: %d steps (%dmm) | State: %s | WiFi: %s\n",
            motorPos, map(motorPos, 0, TOTAL_STEPS, 0, MM_MAX),
            globalState.c_str(), activeSSID.c_str());
    }
    else if (cmd.startsWith("go:")) {
        int mm = constrain(cmd.substring(3).toInt(), 0, MM_MAX);
        stopNow = false;
        transitionState("VENDING");
        moveTo(map(mm, 0, MM_MAX, 0, TOTAL_STEPS));
        transitionState("READY");
    }
}

