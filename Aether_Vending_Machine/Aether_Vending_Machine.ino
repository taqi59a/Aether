/*
 * SANITARY VENDING MACHINE — ESP32-C3  v13.0 (INTERNET GATEWAY)
 * CINEMATIC BOOT SEQUENCE + MQTT REMOTE GATEWAY
 * Particle burst, glitch reveal, expanding logo, synced motor,
 * and a premium living READY screen displaying Device ID.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ================================================
//  PINS & HARDWARE MAP (ESP32-C3 SUPER MINI)
// ================================================
const int AIN1 = 5;
const int AIN2 = 6;
const int BIN1 = 7;
const int BIN2 = 8;
const int BUZZER_PIN = 4;

#define RST_PIN  10
#define SS_PIN    0   // SDA / SPI Chip Select
#define MOSI_PIN  1   // SPI MOSI
#define MISO_PIN  2   // SPI MISO
#define SCK_PIN   3   // SPI Clock

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
//  NETWORKING & MQTT
// ================================================
const int NUM_NETWORKS = 2;
const char* WIFI_SSIDS[NUM_NETWORKS]  = { "VOO-2413CT6", "Taqi IP15" };
const char* WIFI_PASSES[NUM_NETWORKS] = { "12345679", "12345679" };
String activeSSID = "None";

enum WifiState {
    WIFI_IDLE,
    WIFI_SCANNING,
    WIFI_CONNECTING,
    WIFI_CONNECTED_STATE
};
WifiState currentWifiState = WIFI_SCANNING;
unsigned long wifiStateTimer = 0;
const unsigned long WIFI_CONN_TIMEOUT = 10000; // 10 seconds timeout for WPA/DHCP handshake
const unsigned long WIFI_SCAN_INTERVAL = 15000; // Scan every 15 seconds if disconnected

// MQTT Settings
const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

String deviceId    = "";
String cmdTopic    = "";
String statusTopic = "";

unsigned long lastMqttRetry = 0;
const unsigned long MQTT_RETRY_GAP = 10000; // Retry connection every 10 seconds if offline

// ================================================
//  STATE
// ================================================
int  motorPos      = 0;
bool stopNow       = false;
int  cmdSteps      = -1;
bool motorBusy     = false;
bool oledDirty     = false;
bool welcomeShown  = false;
String globalState = "BOOT";
int  animationFrame = 0;

MFRC522          rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void updateOLED();
void handleSerialCommands();
void publishStatus(const char* onlineState);
void transitionState(String newState);
void playTone(int frequency, int durationMs);
void wifiTask();
void mqttKeepAlive();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void runVendSequence();
void homing();
void cinematicBoot();

// ================================================
//  BUZZER
// ================================================
void playTone(int frequency, int durationMs) {
    if (frequency <= 0) return;
    tone(BUZZER_PIN, frequency, durationMs);
    delay(durationMs);
}

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
    publishStatus("online");
    
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

    if (globalState != "VENDING") {
        globalState = "VENDING";
        oledDirty   = true;
        updateOLED();
        publishStatus("online");
    }

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

        if (i % 240 == 0) {
            animationFrame++;
            oledDirty = true;
            updateOLED();
        }
    }

    brakeCoils();
    motorBusy = false;
    stopNow   = false;
}

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
// ================================================
void cinematicBoot() {
    const int NSTAR = 26;
    float sx[NSTAR], sy[NSTAR], sa[NSTAR];
    for (int i = 0; i < NSTAR; i++) {
        sa[i] = (float)random(0, 628) / 100.0;
        sx[i] = 64; sy[i] = 16;
    }
    playTone(300, 20);
    for (int frame = 0; frame < 36; frame++) {
        display.clearDisplay();
        float speed = 0.4 + frame * 0.12;
        for (int i = 0; i < NSTAR; i++) {
            sx[i] += cos(sa[i]) * speed;
            sy[i] += sin(sa[i]) * speed * 0.5;
            int tx = sx[i] - cos(sa[i]) * 3;
            int ty = sy[i] - sin(sa[i]) * 1.5;
            if (sx[i] >= 0 && sx[i] < SCREEN_WIDTH && sy[i] >= 0 && sy[i] < SCREEN_HEIGHT) {
                display.drawLine(tx, ty, (int)sx[i], (int)sy[i], SSD1306_WHITE);
            } else {
                sa[i] = (float)random(0, 628) / 100.0;
                sx[i] = 64; sy[i] = 16;
            }
        }
        display.display();
        if (frame % 6 == 0) playTone(400 + frame * 40, 3);
        delay(22);
    }

    for (int frame = 0; frame < 16; frame++) {
        display.clearDisplay();
        display.setTextSize(3);
        display.setTextColor(SSD1306_WHITE);
        int jitter = (frame % 3 == 0) ? random(-6, 7) : 0;
        display.setCursor(8 + jitter, 4);
        if (frame % 4 != 1) display.print("AETHER");
        if (frame % 2 == 0) {
            int by = random(0, SCREEN_HEIGHT - 4);
            display.fillRect(0, by, SCREEN_WIDTH, random(1, 4), SSD1306_INVERSE);
        }
        for (int n = 0; n < 8; n++) {
            display.drawPixel(random(0, SCREEN_WIDTH), random(0, SCREEN_HEIGHT), SSD1306_WHITE);
        }
        display.display();
        playTone(random(800, 2600), 2);
        delay(45);
    }

    sweepTone(600, 2400, 120);
    rawMove(120, true, 600);
    for (int r = 0; r < 70; r += 6) {
        display.clearDisplay();
        display.setTextSize(3);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(8, 4);
        display.print("AETHER");
        display.drawCircle(64, 16, r, SSD1306_WHITE);
        display.drawCircle(64, 16, r / 2, SSD1306_WHITE);
        display.display();
        playTone(2000 - r * 12, 4);
        delay(28);
    }
    rawMove(120, false, 600);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(30, 2);
    display.print("INITIALIZING");
    display.drawRect(8, 18, 112, 10, SSD1306_WHITE);
    display.display();
    delay(150);

    const int SEGMENTS = 24;
    const int stepsPerSeg = 60;
    for (int seg = 0; seg < SEGMENTS; seg++) {
        int fillW = (108 * (seg + 1)) / SEGMENTS;
        display.fillRect(10, 20, fillW, 6, SSD1306_WHITE);
        display.fillRect(40, 2, 50, 10, SSD1306_BLACK);
        display.setCursor(48, 2);
        display.setTextSize(1);
        display.print((seg + 1) * 100 / SEGMENTS);
        display.print("%");
        display.display();
        rawMove(stepsPerSeg, true, 700);
        playTone(1200 + seg * 50, 3);
    }
    rawMove(SEGMENTS * stepsPerSeg, false, 500);

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
        display.fillRect(w, 18, SCREEN_WIDTH - w, 14, SSD1306_BLACK);
        display.display();
        delay(18);
    }
    delay(250);

    for (int f = 0; f < 3; f++) {
        display.fillScreen(SSD1306_WHITE);
        display.display();
        playTone(3200, 12);
        delay(40);
        display.clearDisplay();
        display.display();
        delay(60);
    }

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
    globalState = "WAIT";
    oledDirty = true;
    updateOLED();
    motorBusy = true;

    int homingSteps = TOTAL_STEPS + 400;
    for (int i = 0; i < homingSteps; i++) {
        doStep(-i);
        delayMicroseconds(SPEED_HOME);
        if (i % 200 == 0) { 
            animationFrame++; 
            oledDirty = true; 
            updateOLED(); 
        }
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
//  MQTT COMMUNICATIONS
// ================================================
void publishStatus(const char* onlineState) {
    if (!mqttClient.connected()) return;
    int mm = map(motorPos, 0, TOTAL_STEPS, 0, MM_MAX);
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"device\":\"%s\",\"status\":\"%s\",\"state\":\"%s\",\"pos_mm\":%d,\"wifi_ssid\":\"%s\",\"wifi_rssi\":%d}",
        deviceId.c_str(), onlineState, globalState.c_str(), mm, activeSSID.c_str(), WiFi.RSSI());
    
    // Publish as a retained message so the dashboard gets the latest state instantly on page load
    mqttClient.publish(statusTopic.c_str(), buf, true);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    msg.trim();
    Serial.printf("[MQTT] Command received: %s\n", msg.c_str());

    if (msg == "stop") {
        stopNow = true;
    } else if (msg == "home") {
        cmdSteps = -2;
        stopNow = false;
    } else if (msg == "vendswipe") {
        cmdSteps = 9999;
        stopNow = false;
    } else {
        int mm = msg.toInt();
        if (mm >= 0 && mm <= MM_MAX) {
            cmdSteps = map(mm, 0, MM_MAX, 0, TOTAL_STEPS);
            stopNow = false;
        }
    }
}

void mqttKeepAlive() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (mqttClient.connected()) {
        mqttClient.loop();
        return;
    }

    unsigned long now = millis();
    if (now - lastMqttRetry > MQTT_RETRY_GAP) {
        lastMqttRetry = now;
        Serial.printf("[MQTT] Connecting to broker %s... ", MQTT_BROKER);

        // Register Last Will & Testament (LWT) as offline status
        String lwtPayload = "{\"device\":\"" + deviceId + "\",\"status\":\"offline\"}";
        if (mqttClient.connect(deviceId.c_str(), statusTopic.c_str(), 0, true, lwtPayload.c_str())) {
            Serial.println("connected!");
            mqttClient.subscribe(cmdTopic.c_str());
            publishStatus("online");
        } else {
            Serial.printf("failed, rc=%d. Retrying...\n", mqttClient.state());
        }
    }
}

// ================================================
//  NON-BLOCKING WIFI STATE MACHINE (WiFiMulti)
// ================================================
void wifiTask() {
    if (WiFi.status() == WL_CONNECTED) {
        if (activeSSID == "None") {
            activeSSID = WiFi.SSID();
            Serial.printf("[WIFI] Connected: %s | IP %s | RSSI %d\n",
                activeSSID.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
            publishStatus("online");
            currentWifiState = WIFI_CONNECTED_STATE;
        }
        return;
    }

    // Disconnected state
    if (currentWifiState == WIFI_CONNECTED_STATE) {
        Serial.println("[WIFI] Connection lost. Starting scan...");
        activeSSID = "None";
        WiFi.disconnect();
        currentWifiState = WIFI_SCANNING;
        WiFi.scanNetworks(true); // Trigger non-blocking scan
        wifiStateTimer = millis();
        return;
    }

    unsigned long now = millis();

    switch (currentWifiState) {
        case WIFI_SCANNING: {
            int n = WiFi.scanComplete();
            if (n >= 0) { // Scan completed
                Serial.printf("[WIFI] Scan finished: %d networks found.\n", n);
                String bestSSID = "";
                String bestPass = "";
                
                // Search in order of priority (VOO first, then Taqi)
                bool foundVOO = false;
                bool foundTaqi = false;
                for (int i = 0; i < n; ++i) {
                    String ssid = WiFi.SSID(i);
                    if (ssid == "VOO-2413CT6") foundVOO = true;
                    if (ssid == "Taqi IP15") foundTaqi = true;
                }
                WiFi.scanDelete();

                if (foundVOO) {
                    bestSSID = "VOO-2413CT6";
                    bestPass = "12345679";
                } else if (foundTaqi) {
                    bestSSID = "Taqi IP15";
                    bestPass = "12345679";
                }

                if (bestSSID != "") {
                    Serial.printf("[WIFI] Found configured network in air: %s. Initiating handshake...\n", bestSSID.c_str());
                    WiFi.begin(bestSSID.c_str(), bestPass.c_str());
                    currentWifiState = WIFI_CONNECTING;
                    wifiStateTimer = now;
                } else {
                    Serial.println("[WIFI] Configured networks not found in scan. Waiting to re-scan.");
                    currentWifiState = WIFI_IDLE;
                    wifiStateTimer = now;
                }
            } else if (n == -2) {
                Serial.println("[WIFI] Scan failed (-2). Backing off for 5 seconds before retry...");
                currentWifiState = WIFI_IDLE;
                wifiStateTimer = now - (WIFI_SCAN_INTERVAL - 5000);
            }
            break;
        }

        case WIFI_CONNECTING: {
            if (now - wifiStateTimer > WIFI_CONN_TIMEOUT) {
                Serial.println("[WIFI] Connection timed out. Initiating re-scan...");
                WiFi.disconnect();
                currentWifiState = WIFI_SCANNING;
                WiFi.scanNetworks(true);
                wifiStateTimer = now;
            }
            break;
        }

        case WIFI_IDLE: {
            if (now - wifiStateTimer > WIFI_SCAN_INTERVAL) {
                Serial.println("[WIFI] Retrying scan...");
                currentWifiState = WIFI_SCANNING;
                WiFi.scanNetworks(true);
                wifiStateTimer = now;
            }
            break;
        }
        
        default:
            break;
    }
}

// ================================================
//  OLED — RUNTIME SCREENS
// ================================================
void updateOLED() {
    if (!oledDirty) return;
    oledDirty = false;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    bool linked = (WiFi.status() == WL_CONNECTED && mqttClient.connected());

    // Wi-Fi/Broker status block in top-right corner
    if (linked) {
        display.fillRect(SCREEN_WIDTH - 4, 0, 3, 3, SSD1306_WHITE);
    } else {
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

        // Display paired unique ID on the right
        display.setTextSize(1);
        display.setCursor(86, 2);
        display.print("ID:");
        display.setCursor(86, 12);
        // Show device ID hex string suffix
        if (deviceId.length() > 7) {
            display.print(deviceId.substring(7));
        } else {
            display.print("----");
        }

        // Dual flowing energy ribbons
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int y1 = 26 + (int)(3.5 * sin((x + animationFrame * 6) * 0.13));
            int y2 = 28 + (int)(2.5 * sin((x - animationFrame * 4) * 0.10 + 1.5));
            display.drawPixel(x, y1, SSD1306_WHITE);
            display.drawPixel(x, y2, SSD1306_WHITE);
        }
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
        
        // Background particles
        for (int p = 0; p < 12; p++) {
            int px = (p * 12 + (animationFrame * 2)) % SCREEN_WIDTH;
            int py = 16 + (int)(7.0 * sin((px + animationFrame * 4) * 0.06));
            display.drawPixel(px, py, SSD1306_WHITE);
        }

        // Foreground rotating 3D Double Helix
        for (int x = 8; x < SCREEN_WIDTH - 8; x += 5) {
            float envelope = sin((x - 8) * 3.14159 / (SCREEN_WIDTH - 16));
            float amp = 14.0 * envelope;
            
            int y1 = 16 + (int)(amp * sin(x * 0.15 + t));
            int y2 = 16 - (int)(amp * sin(x * 0.15 + t));
            
            display.drawLine(x, y1, x, y2, SSD1306_WHITE);
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
}

// ================================================
//  SETUP
// ================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== AETHER VEND OS v13.0 | INTERNET GATEWAY ===");

    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(800000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        Serial.println("[ERROR] OLED failed.");

    randomSeed(esp_random());

    // Generate Device ID from MAC Address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char idBuf[32];
    snprintf(idBuf, sizeof(idBuf), "AETHER-%02X%02X", mac[4], mac[5]);
    deviceId = String(idBuf);
    cmdTopic = "aether/vending/" + deviceId + "/cmd";
    statusTopic = "aether/vending/" + deviceId + "/status";

    Serial.printf("[SYSTEM] Device ID: %s\n", deviceId.c_str());
    Serial.printf("[SYSTEM] Command Topic: %s\n", cmdTopic.c_str());
    Serial.printf("[SYSTEM] Status Topic: %s\n", statusTopic.c_str());

    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    stopCoils();

    cinematicBoot();

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    rfid.PCD_Init();

    homing();

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect();
    
    espClient.setTimeout(1); // Set socket timeout to 1 second to prevent blocking hangs during MQTT connection
    
    Serial.println("[WIFI] Launching initial background network scan...");
    WiFi.scanNetworks(true); // Start non-blocking scan on boot
    currentWifiState = WIFI_SCANNING;
    wifiStateTimer = millis();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    transitionState("READY");
    Serial.printf("[BOOT] Ready. Cruise: %dus\n", SPEED_CRUISE);
}

// ================================================
//  LOOP
// ================================================
void loop() {
    wifiTask();
    mqttKeepAlive();

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
        }
    }

    static unsigned long lastRfidScan = 0;
    if (!motorBusy && (millis() - lastRfidScan > 150)) {
        lastRfidScan = millis();
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            Serial.println("[RFID] Card detected.");
            playTone(4000, 10); playTone(4500, 10);
            runVendSequence();
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }

    if (!motorBusy) {
        delay(2);
    }
}

// ================================================
//  SERIAL COMMANDS
// ================================================
void handleSerialCommands() {
    static String rxBuffer = "";
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            rxBuffer.trim();
            if (rxBuffer.length() > 0) {
                rxBuffer.toLowerCase();
                if      (rxBuffer == "test")   { stopNow = false; runVendSequence(); }
                else if (rxBuffer == "home")   { homing(); }
                else if (rxBuffer == "stop")   { stopNow = true; }
                else if (rxBuffer == "boot")   { cinematicBoot(); transitionState("READY"); }
                else if (rxBuffer == "status") {
                    Serial.printf("[STATUS] Pos: %d steps (%dmm) | State: %s | WiFi: %s | Broker: %s\n",
                        motorPos, map(motorPos, 0, TOTAL_STEPS, 0, MM_MAX),
                        globalState.c_str(), activeSSID.c_str(), mqttClient.connected() ? "Connected" : "Disconnected");
                }
                else if (rxBuffer.startsWith("go:")) {
                    int mm = constrain(rxBuffer.substring(3).toInt(), 0, MM_MAX);
                    stopNow = false;
                    transitionState("VENDING");
                    moveTo(map(mm, 0, MM_MAX, 0, TOTAL_STEPS));
                    transitionState("READY");
                }
            }
            rxBuffer = "";
        } else {
            if (rxBuffer.length() < 64) {
                rxBuffer += c;
            }
        }
    }
}
