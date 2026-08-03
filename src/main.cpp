#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <Preferences.h>
#include <WebServer.h>
#include "html_page.h"
#include <qrcode.h>
#include <MFRC522.h>
#include "video_data.h"

// ================================================
//  CONFIGURATION & VENDING MACHINE IDENTIFIER
// ================================================
#define USE_2_PIN_MODE false
const char* VENDING_ID = "5552"; // Online Vending Machine #5552

// MQTT Cloud Broker Settings
const char* mqttServer = "broker.hivemq.com";
const int   mqttPort   = 1883;

// Topics
String subCmdTopic = "aether/vending/" + String(VENDING_ID) + "/cmd";
String pubStatTopic = "aether/vending/" + String(VENDING_ID) + "/status";

// Motor Pins (DRV8833)
const int IN1 = 6;
const int IN2 = 7;
const int IN3 = 10;
const int IN4 = 11;
const int EEP_PIN = 9;

// TFT Display (Software SPI)
#define TFT_CS   16
#define TFT_DC   15
#define TFT_RST  14
#define TFT_BLK  17
#define TFT_SCLK 12
#define TFT_MOSI 13

// Encoder & Buttons
#define ENC_A    18
#define ENC_B    8
#define ENC_SW   3
#define KEY0     46

// Buzzer
#define BUZZER_PIN 5

// RC522 RFID Reader Pins
#define RFID_SS_PIN   9   // Freed 9th pin as Chip Select (SDA)
#define RFID_RST_PIN  2   // Reset pin
#define RFID_MISO_PIN 1   // MISO pin
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
unsigned long lastRfidScanMs = 0;

// ================================================
//  GLOBAL MOTOR & MOVEMENT SETTINGS
//  (Max Torque & Pushing Power for 5V DRV8833 Driver)
// ================================================
const int totalSteps = 3200;
const int maxSpeed   = 1800; // 1800us per step = MAXIMUM HIGH-TORQUE CRUISE SPEED AT 5V
const int startSpeed = 2600; // 2600us startup delay for solid breakaway torque
const int rampSteps  = 80;   // Short 80-step ramp to deliver instant push force

// ================================================
//  QR CODE DATA MATRIX (www.taqiabbas.com/aether)
// ================================================
const bool qrMatrix[27][27] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0},
  {0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
  {0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0},
  {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0},
  {0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0},
  {0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0},
  {0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0},
  {0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0},
  {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0},
  {0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
  {0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// ================================================
//  GLOBALS
// ================================================
int  motorPos  = 0;
int  targetPos = 0;
bool stopNow   = false;
bool motorBusy = false;
int  sweepState = 0;
unsigned long lastStatusMs = 0;
unsigned long lastActionMs = 0;
unsigned long lastMqttRetryMs = 0;

// WiFi & MQTT Clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);
bool triggerHoming = false;

// GLOBAL THEME STATE (Default = Light Theme)
bool isDarkTheme = false;

// GLOBAL MOTOR LIMIT & CALIBRATION (Default = 80mm)
const int maxPhysicalMm = 80;
int maxLimitMm = 80;

void loadMotorPreferences() {
  Preferences prefs;
  prefs.begin("motor-store", true);
  maxLimitMm = prefs.getInt("max_mm", 80);
  maxLimitMm = constrain(maxLimitMm, 10, 80);
  prefs.end();
}

void saveMotorPreferences() {
  Preferences prefs;
  prefs.begin("motor-store", false);
  prefs.putInt("max_mm", maxLimitMm);
  prefs.end();
}

// Hardware SPI display (shared SPI bus with RC522)
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
int SW, SH;

// Encoder Quadrature State Machine (Instant Zero-Latency & Noise Filter for Long Wires)
volatile int encCount = 0;
volatile uint8_t encState = 0;
const int8_t KNOB_STATES[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void IRAM_ATTR encISR() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);
  encState = ((encState << 2) | (a << 1) | b) & 0x0F;
  int8_t step = KNOB_STATES[encState];
  if (step != 0) {
    encCount += step;
  }
}

// Button debounce helper
bool btnPress(int pin, bool &last) {
  bool cur = digitalRead(pin);
  bool p = (cur == LOW && last == HIGH);
  last = cur;
  return p;
}
bool lastPSH = HIGH, lastK0 = HIGH;

// Audio Beep helper (non-blocking tone)
void beep(unsigned int freq, unsigned long durMs) {
  tone(BUZZER_PIN, freq, durMs);
}

// Built-in Board RGB LED Controller (ESP32-S3)
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif

void setRgbLed(uint8_t r, uint8_t g, uint8_t b) {
  #ifdef NEOPIXEL_POWER
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
  #endif
  #ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, r, g, b);
  #endif
  neopixelWrite(48, r, g, b);
  neopixelWrite(38, r, g, b);
  neopixelWrite(21, r, g, b);
  neopixelWrite(47, r, g, b);
}

// UI State
enum Screen { 
  SCR_STARTUP,
  SCR_STANDBY, 
  SCR_MENU, 
  SCR_SUB_STOCK, 
  SCR_SUB_MOTOR, 
  SCR_SUB_RFID, 
  SCR_SUB_SYS, 
  SCR_ACTUATOR, 
  SCR_CALIB, 
  SCR_STOCK, 
  SCR_DIAG_TOF, 
  SCR_RFID, 
  SCR_CARD_LOGS, 
  SCR_CARD_DETAIL, 
  SCR_ADMIN_SETUP,
  SCR_WIFI, 
  SCR_WIFI_PASS, 
  SCR_WIFI_CONNECTING, 
  SCR_INFO, 
  SCR_DISPENSE,
  SCR_SCREENSAVER
};
Screen curScreen = SCR_STANDBY;

// Stealth Mode & Screensaver State Variables
bool isStealthMode = false;
int comboK0Count = 0;
int comboKnobCount = 0;
unsigned long comboStartMs = 0;
unsigned long lastUserActivityMs = 0;

unsigned long lastK0PressMs = 0; // K0 Double-click Kill-Switch detection

// Categorized Main Menu (Short punchy labels for Size 2 font - NO WRAPPING!)
const int NMENU = 4;
const char* mLabel[] = {
  "1. STOCK SENSOR",
  "2. MOTOR CTRL",
  "3. RFID & ADMIN",
  "4. SYS & WIFI"
};

// Submenu labels
const int NSUB_STOCK = 2;
const char* mLabelStock[] = {
  "1. LIVE STOCK",
  "2. TOF DIAG"
};

const int NSUB_MOTOR = 3;
const char* mLabelMotor[] = {
  "1. MANUAL MOVE",
  "2. LIMIT CALIB",
  "3. MOTOR SWEEP"
};

const int NSUB_RFID = 3;
const char* mLabelRfid[] = {
  "1. RFID TEST",
  "2. CARD HISTORY",
  "3. ADMIN SETUP"
};

const int NSUB_SYS = 3;
const char* mLabelSys[] = {
  "1. WIFI SETUP",
  "2. THEME TOGGLE",
  "3. SYSTEM INFO"
};

int mSel = 0, subSel = 0, lastEnc = 0;

// WiFi Direct Connect state variables
int wifiCount = 0;
int selectedNetIdx = 0;
String wifiPassword = "";
int gridSel = 0;

const char* grid[] = {
  "0", "1", "2", "3", "4", "5",
  "6", "7", "8", "9", ".", "_",
  "-", "@", "#", "!", "?", " ",
  "A", "B", "C", "D", "E", "F",
  "G", "H", "I", "J", "K", "L",
  "M", "N", "O", "P", "Q", "R",
  "S", "T", "U", "V", "W", "X",
  "Y", "Z", "a", "b", "c", "d",
  "e", "f", "g", "h", "i", "j",
  "k", "l", "m", "n", "o", "p",
  "q", "r", "s", "t", "u", "v",
  "w", "x", "y", "z", "DEL", "CON"
};

// ── HIGH CONTRAST THEME COLOR PALETTE DEFINITIONS ──
#define C_BK   0x0000
#define C_CY   0x07FF
#define C_GL   0xFFE0
#define C_MG   0xF81F
#define C_PU   0x881F
#define C_GN   0x07E0
#define C_RD   0xF800
#define C_WH   0xFFFF
#define C_GR   0x7BEF
#define C_DG   0x4208

// Palette Getters (High-Contrast Cyberpunk Palette)
uint16_t getBgColor()     { return isDarkTheme ? C_BK : C_WH; }
uint16_t getTextMain()   { return isDarkTheme ? C_WH : C_BK; }
uint16_t getTextSub()    { return isDarkTheme ? C_GR : 0x4208; }
uint16_t getAccentCyan() { return isDarkTheme ? C_CY : 0x001F; }
uint16_t getAccentGold() { return isDarkTheme ? C_GL : 0xE500; }
uint16_t getFooterBg()   { return isDarkTheme ? 0x1082 : 0xDF17; }

uint16_t getCardBg(bool sel) {
  if (isDarkTheme) return sel ? 0x1082 : C_BK;
  return sel ? 0xE71C : C_WH;
}

uint16_t getCardFg(bool sel) {
  if (isDarkTheme) return C_WH;
  return C_BK;
}

uint16_t getCardBar(bool sel) {
  if (isDarkTheme) return sel ? C_BK : C_WH;
  return sel ? C_WH : C_BK;
}

#define HDR_H   44
#define ITEM_H  42
#define ITEM_SP 48
#define ITEM_Y0 52
#define FTR_H   24

int lastBw = -1;

int pwStep = 0;
unsigned long pwLastStepMs = 0;
const int expectedDir[4] = { 1, 1, -1, -1 };

void pwReset() { pwStep = 0; }

// Forward declarations
void handleSerial();
void handleRFID();
void sendStat(const char* s);
void publishMqttStatus();
void homing();
void moveToTargetSmooth(int destinationPos);
void executeGlobalMotorMove(int targetMm);
void rotate3D(float x, float y, float z, float ax, float ay, int &screenX, int &screenY, int cx, int cy);
void eraseOctahedron(int* px, int* py);
void drawOctahedron(float ax, float ay, int cx, int cy, uint16_t color, int* prevX, int* prevY);
void drawLogoText(const char* txt, int tx, int ty, int size);
void drawQRCode(int x0, int y0, int pixelSize);
void drawHudBrackets();
void loadThemePreference();
void saveThemePreference();
void sendCorsHeaders();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMqtt();
void startup();
void standbyScreen();
void menuFull();
void menuItem(int i, bool sel);
void menuItemView(int viewSlot, int itemIdx, bool sel);
void actScreen();
void actUpdate(const char* s);
void wifiScreen();
void wifiItem(int i, bool sel);
void drawPasswordScreen();
void drawPasswordText();
void drawPassGridItem(int idx, bool sel);
void connectToWiFi();
void infoScreen();
void drawCalibScreen();
void drawStockScreen();
void drawStockData();
void drawI2cDiagScreen();
void drawI2cDiagData();
void drawSubMenu(const char* title, const char** labels, int count, int sel);
void drawSubMenuItem(const char** labels, int idx, bool sel, int scrollOffset = 0);
void updateSubMenuSelection(const char** labels, int oldSel, int newSel);
void playStartupMelody();
bool checkK0KillSwitch();
void drawRfidScreen();
void drawCardLogsScreen();
void drawCardDetailScreen(int idx);
void drawAdminSetupScreen();
void drawCareDispenseFrame();
void drawCareDispenseAnimation(int cx, int cy, float frameAngle, const char* titleMsg, const char* subMsg);
void runDispenseWorkflow(const char* cardUid);
void updateRainbowRgbLed();
void processStealthCombo(bool isK0, bool isKnob);
void drawHexagon(int cx, int cy, int r, uint16_t color);
void runScreensaverFrame();

// ================================================
//  TOF050C / VL6180X TOF STOCK DISTANCE SENSOR
// ================================================
#include <Wire.h>
#include <Adafruit_VL6180X.h>

#define I2C_SDA_PIN 41
#define I2C_SCL_PIN 42

Adafruit_VL6180X vl6180x = Adafruit_VL6180X();
bool tofOnline = false;
bool diagFrameDrawn = false;
bool stockFrameDrawn = false;
int liveStockDistanceMm = 0;

// Multi-Admin Card State Variables (NVS Persistent)
#define MAX_ADMIN_CARDS 5
String adminCards[MAX_ADMIN_CARDS];
int adminCardCount = 0;
bool adminSessionActive = false;
bool adminRegisterMode = false;
String adminStatusBanner = "";
unsigned long adminBannerMs = 0;
int adminOptSel = 0;

String cleanUid(String raw) {
  String s = "";
  for (int i = 0; i < raw.length(); i++) {
    char c = raw.charAt(i);
    if (c != ' ' && c != '\r' && c != '\n') {
      s += (char)toupper(c);
    }
  }
  return s;
}

void loadAdminCardsFromNvs() {
  Preferences prefs;
  prefs.begin("admin-store", true);
  adminCardCount = prefs.getInt("count", 0);
  adminCardCount = constrain(adminCardCount, 0, MAX_ADMIN_CARDS);
  for (int i = 0; i < adminCardCount; i++) {
    String k = "uid_" + String(i);
    adminCards[i] = cleanUid(prefs.getString(k.c_str(), ""));
  }
  prefs.end();
}

bool isAdminCard(String rawUid) {
  String u = cleanUid(rawUid);
  if (u.length() == 0) return false;
  for (int i = 0; i < adminCardCount; i++) {
    if (adminCards[i].equalsIgnoreCase(u)) return true;
  }
  return false;
}

bool addAdminCardToNvs(String rawUid) {
  String u = cleanUid(rawUid);
  if (u.length() == 0) return false;
  if (isAdminCard(u)) return true;
  if (adminCardCount >= MAX_ADMIN_CARDS) return false;

  Preferences prefs;
  prefs.begin("admin-store", false);
  String k = "uid_" + String(adminCardCount);
  prefs.putString(k.c_str(), u);
  adminCards[adminCardCount] = u;
  adminCardCount++;
  prefs.putInt("count", adminCardCount);
  prefs.end();
  return true;
}

void clearAllAdminCardsFromNvs() {
  Preferences prefs;
  prefs.begin("admin-store", false);
  prefs.clear();
  prefs.end();
  adminCardCount = 0;
  for (int i = 0; i < MAX_ADMIN_CARDS; i++) adminCards[i] = "";
}
int emptyStockDepthMm = 350; // Zero stock distance threshold (Configurable via Menu)
int fullStockDepthMm  = 20;  // Full stock distance threshold
int currentStockPercent = 100;
int menuTopIdx = 0; // Viewport scrolling top index

void loadStockPreferences() {
  Preferences prefs;
  prefs.begin("stock-store", true);
  emptyStockDepthMm = prefs.getInt("empty_mm", 350);
  emptyStockDepthMm = constrain(emptyStockDepthMm, 50, 500);
  prefs.end();
}

void saveStockPreferences() {
  Preferences prefs;
  prefs.begin("stock-store", false);
  prefs.putInt("empty_mm", emptyStockDepthMm);
  prefs.end();
}

void writeReg16(uint8_t addr, uint16_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)(reg & 0xFF));
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readReg16(uint8_t addr, uint16_t reg) {
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)(reg & 0xFF));
  if (Wire.endTransmission() != 0) return 0xFF;
  Wire.requestFrom(addr, (uint8_t)1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void initStockSensor() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000); // 100kHz standard mode for long wires
  delay(30);

  Wire.beginTransmission(0x29);
  if (Wire.endTransmission() == 0) {
    tofOnline = true;
    Serial.println("VL6180X / TOF050C Sensor Detected on I2C (0x29)");

    // 1. Clear any pending interrupt status (SYSTEM__INTERRUPT_CLEAR 0x0015 = 0x07)
    writeReg16(0x29, 0x0015, 0x07);

    // 2. Set Max Convergence Time to 49ms (0x001C = 0x31) for deep 500mm stack readings
    writeReg16(0x29, 0x001C, 0x31);

    // 3. Set Inter-measurement Period to 50ms (0x001B = 0x04) for fast continuous background updates
    writeReg16(0x29, 0x001B, 0x04);

    // 4. Start Continuous Ranging Mode: SYSRANGE__START (0x0018) = 0x03
    writeReg16(0x29, 0x0018, 0x03);

    // Initialize Adafruit library object
    vl6180x.begin(&Wire);
  } else {
    tofOnline = false;
    Serial.println("No I2C device response at 0x29 on GPIO 41/42");
  }
}

int readRawTofDistanceMm() {
  if (!tofOnline) return -1;

  // 1. Direct 16-Bit Register Read from RESULT__RANGE_VAL (0x0062) in continuous mode
  uint8_t rawVal = readReg16(0x29, 0x0062);
  writeReg16(0x29, 0x0015, 0x07); // Clear interrupt status

  if (rawVal > 0 && rawVal < 255) {
    liveStockDistanceMm = rawVal;
    return liveStockDistanceMm;
  }

  // 2. Fallback to Adafruit library reading
  uint8_t range = vl6180x.readRange();
  if (range > 0 && range < 255) {
    liveStockDistanceMm = range;
    return liveStockDistanceMm;
  }

  return liveStockDistanceMm;
}

int readLiveStockDistanceMm() {
  return readRawTofDistanceMm();
}

int readFilteredStockDistanceMm() {
  if (!tofOnline) return -1;
  int samples[5];
  int validCount = 0;
  for (int i = 0; i < 5; i++) {
    int d = readRawTofDistanceMm();
    if (d > 0 && d < 600) {
      samples[validCount++] = d;
    }
    delay(4);
  }
  if (validCount == 0) return readRawTofDistanceMm();

  // 5-Point Median Sort
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = i + 1; j < validCount; j++) {
      if (samples[i] > samples[j]) {
        int temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  return samples[validCount / 2];
}

String lastStockCalibNote = "";

bool calibrateZeroStockLimit() {
  int curDist = readFilteredStockDistanceMm();
  if (curDist <= 0) {
    curDist = readRawTofDistanceMm();
  }
  if (curDist <= 0 || curDist == 255) {
    curDist = 350; // Safety maximum stack limit if sensor out of reach
  }

  emptyStockDepthMm = curDist; // Save exact measured physical distance (RAM)
  saveStockPreferences();      // Save permanently in NVS Flash memory!

  char noteBuf[45];
  snprintf(noteBuf, sizeof(noteBuf), "ZERO STOCK INITIATED: %d mm", emptyStockDepthMm);
  lastStockCalibNote = String(noteBuf);

  Serial.printf("DYNAMIC ZERO STOCK INITIATED AT: %d mm\n", emptyStockDepthMm);
  return true;
}

int getStockPercentage() {
  if (!tofOnline) return 0;

  int dist = readFilteredStockDistanceMm();

  // 1. If distance is at or past empty initialized length D_empty or out-of-bounds: 0% EMPTY!
  if (dist <= 0 || dist >= emptyStockDepthMm || dist == 255) {
    return 0;
  }

  // 2. If distance is right in front of sensor (D <= 15mm): 100% FULL!
  if (dist <= 15) {
    return 100;
  }

  // 3. Distribute percentage proportionally between 15mm (100%) and D_empty (0%):
  int span = emptyStockDepthMm - 15;
  if (span <= 0) return 0;

  int pct = ((emptyStockDepthMm - dist) * 100) / span;
  currentStockPercent = constrain(pct, 0, 100);
  return currentStockPercent;
}

// ================================================
//  UNIQUE CARD DATABASE & FIFO AUTO-PRUNING LOGS
// ================================================
struct CardRecord {
  char uid[20];
  uint32_t totalScans;
  uint32_t monthScans;
  uint32_t firstSeenSec;
  uint32_t lastSeenSec;
};

const int MAX_CARD_RECORDS = 35; // Maximum unique cards stored safely in NVS
CardRecord cardDb[MAX_CARD_RECORDS];
int totalUniqueCards = 0;
uint32_t oldestDataSec = 0; // Timestamp of oldest retained data record
int cardDbSel = 0;

void loadCardDbFromNvs() {
  Preferences prefs;
  prefs.begin("card-db", true);
  totalUniqueCards = prefs.getInt("count", 0);
  totalUniqueCards = constrain(totalUniqueCards, 0, MAX_CARD_RECORDS);
  oldestDataSec = prefs.getUInt("oldest_sec", 0);

  for (int i = 0; i < totalUniqueCards; i++) {
    String uk = "u" + String(i);
    String tk = "t" + String(i);
    String mk = "m" + String(i);
    String fk = "f" + String(i);
    String lk = "l" + String(i);

    String uStr = prefs.getString(uk.c_str(), "NONE");
    snprintf(cardDb[i].uid, sizeof(cardDb[i].uid), "%s", uStr.c_str());
    cardDb[i].totalScans = prefs.getUInt(tk.c_str(), 1);
    cardDb[i].monthScans = prefs.getUInt(mk.c_str(), 1);
    cardDb[i].firstSeenSec = prefs.getUInt(fk.c_str(), 0);
    cardDb[i].lastSeenSec = prefs.getUInt(lk.c_str(), 0);
  }
  prefs.end();
}

void saveCardDbToNvs() {
  Preferences prefs;
  prefs.begin("card-db", false);
  prefs.putInt("count", totalUniqueCards);
  prefs.putUInt("oldest_sec", oldestDataSec);

  for (int i = 0; i < totalUniqueCards; i++) {
    String uk = "u" + String(i);
    String tk = "t" + String(i);
    String mk = "m" + String(i);
    String fk = "f" + String(i);
    String lk = "l" + String(i);

    prefs.putString(uk.c_str(), cardDb[i].uid);
    prefs.putUInt(tk.c_str(), cardDb[i].totalScans);
    prefs.putUInt(mk.c_str(), cardDb[i].monthScans);
    prefs.putUInt(fk.c_str(), cardDb[i].firstSeenSec);
    prefs.putUInt(lk.c_str(), cardDb[i].lastSeenSec);
  }
  prefs.end();
}

void saveCardScanToDb(const char* uid) {
  uint32_t nowSec = millis() / 1000;
  if (oldestDataSec == 0) {
    oldestDataSec = nowSec;
  }

  // 1. Search for existing card UID in database
  int foundIdx = -1;
  for (int i = 0; i < totalUniqueCards; i++) {
    if (strcmp(cardDb[i].uid, uid) == 0) {
      foundIdx = i;
      break;
    }
  }

  if (foundIdx >= 0) {
    // Card exists -> Update counts & timestamp
    cardDb[foundIdx].totalScans++;
    cardDb[foundIdx].monthScans++;
    cardDb[foundIdx].lastSeenSec = nowSec;
  } else {
    // New card UID -> If DB is full, run FIFO auto-pruning
    if (totalUniqueCards >= MAX_CARD_RECORDS) {
      oldestDataSec = cardDb[1].firstSeenSec; // Update "Data Available From" timestamp
      for (int i = 0; i < MAX_CARD_RECORDS - 1; i++) {
        cardDb[i] = cardDb[i + 1];
      }
      totalUniqueCards = MAX_CARD_RECORDS - 1;
    }

    // Insert new card
    snprintf(cardDb[totalUniqueCards].uid, sizeof(cardDb[totalUniqueCards].uid), "%s", uid);
    cardDb[totalUniqueCards].totalScans = 1;
    cardDb[totalUniqueCards].monthScans = 1;
    cardDb[totalUniqueCards].firstSeenSec = nowSec;
    cardDb[totalUniqueCards].lastSeenSec = nowSec;
    totalUniqueCards++;
  }

  saveCardDbToNvs();
}

// ================================================
//  MOTOR DRIVER COIL STEPPER
// ================================================
void doStep(int p) {
  if (USE_2_PIN_MODE) {
    switch(p%4) {
      case 0: digitalWrite(IN1,HIGH); digitalWrite(IN3,HIGH); break;
      case 1: digitalWrite(IN1,LOW);  digitalWrite(IN3,HIGH); break;
      case 2: digitalWrite(IN1,LOW);  digitalWrite(IN3,LOW);  break;
      case 3: digitalWrite(IN1,HIGH); digitalWrite(IN3,LOW);  break;
    }
  } else {
    switch(p%4) {
      case 0: digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW);  break;
      case 1: digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW);  break;
      case 2: digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); break;
      case 3: digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); break;
    }
  }
}

void stopCoils() {
  digitalWrite(IN1,LOW); digitalWrite(IN3,LOW);
  if(!USE_2_PIN_MODE) { digitalWrite(IN2,LOW); digitalWrite(IN4,LOW); }
}

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

// ================================================
//  UNIFIED GLOBAL MOTOR MOVEMENT HANDLER
//  (Global entry point for Knob, Web, MQTT, Serial)
// ================================================
void moveToTargetSmooth(int destinationPos) {
  lastUserActivityMs = millis();
  if (curScreen == SCR_SCREENSAVER) {
    curScreen = SCR_STANDBY;
    standbyScreen();
  }
  destinationPos = constrain(destinationPos, 0, totalSteps);
  if (motorPos == destinationPos) return;

  motorBusy = true;
  stopNow = false;
  
  // Power Optimization: Turn OFF RGB LED during motor move to provide 100% power headroom to motor & TFT screen!
  setRgbLed(0, 0, 0);

  sendStat("moving");
  publishMqttStatus();

  int totalDistance = abs(destinationPos - motorPos);
  int stepsMoved = 0;
  static unsigned long lastUiUpdateMs = 0;

  while (motorPos != destinationPos && !stopNow) {
    stepsMoved++;
    int remaining = abs(destinationPos - motorPos);

    // Smooth linear acceleration / deceleration ramp
    int dly = startSpeed;
    if (stepsMoved < rampSteps) {
      dly = startSpeed - ((startSpeed - maxSpeed) * stepsMoved / rampSteps);
    } else if (remaining < rampSteps) {
      dly = maxSpeed + ((startSpeed - maxSpeed) * (rampSteps - remaining) / rampSteps);
    } else {
      dly = maxSpeed;
    }
    dly = constrain(dly, maxSpeed, startSpeed);

    if (destinationPos > motorPos) {
      motorPos++;
      doStep(motorPos);
    } else {
      motorPos--;
      doStep(motorPos);
    }
    delayMicroseconds(dly);

    // Non-blocking UI update every 120ms to eliminate motor stutter/jerking
    if ((curScreen == SCR_ACTUATOR) && (millis() - lastUiUpdateMs > 120)) {
      lastUiUpdateMs = millis();
      actUpdate("MOVING");
    }
  }

  stopCoils();
  targetPos = motorPos; // Sync target
  motorBusy = false;
  sendStat("idle");
  publishMqttStatus();

  // Turn LED OFF when movement finishes
  if (curScreen != SCR_DISPENSE) {
    setRgbLed(0, 0, 0);
  }

  if (curScreen == SCR_ACTUATOR) {
    actUpdate("IDLE");
  }
}

// Universal MM Movement Dispatcher
void executeGlobalMotorMove(int targetMm) {
  targetMm = constrain(targetMm, 0, maxLimitMm);
  int destSteps = map(targetMm, 0, maxPhysicalMm, 0, totalSteps);
  moveToTargetSmooth(destSteps);
}

// ================================================
//  MQTT CLOUD MESSAGING HANDLERS
// ================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim(); msg.toUpperCase();

  beep(1800, 25);
  Serial.printf("MQTT IN [%s]: %s\n", topic, msg.c_str());

  if (msg.startsWith("GO:")) {
    int mm = msg.substring(3).toInt();
    executeGlobalMotorMove(mm);
  } else if (msg == "HOME" || msg == "CALIBRATE") {
    homing();
  } else if (msg == "ZERO_STOCK" || msg == "CALIBRATE_ZERO") {
    calibrateZeroStockLimit();
  } else if (msg == "SWEEP") {
    executeGlobalMotorMove(80);
    if (!stopNow) executeGlobalMotorMove(0);
  } else if (msg == "STOP") {
    stopNow = true;
    targetPos = motorPos;
    stopCoils();
  } else if (msg == "STATUS") {
    publishMqttStatus();
  }
}

void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;
  if (millis() - lastMqttRetryMs < 5000) return;

  lastMqttRetryMs = millis();
  String clientId = "Aether-Machine-" + String(VENDING_ID) + "-" + String(random(0xffff), HEX);
  
  if (mqttClient.connect(clientId.c_str())) {
    mqttClient.subscribe(subCmdTopic.c_str());
    publishMqttStatus();
    beep(1500, 40);
  }
}

void publishMqttStatus() {
  if (!mqttClient.connected()) return;
  int mm = map(motorPos, 0, totalSteps, 0, 80);
  String stat = "idle";
  if (motorBusy) {
    stat = (sweepState > 0) ? "sweeping" : "moving";
  }
  String json = "{\"id\":\"" + String(VENDING_ID) + "\",\"status\":\"" + stat + "\",\"pos\":" + String(motorPos) + ",\"mm\":" + String(mm) + "}";
  mqttClient.publish(pubStatTopic.c_str(), json.c_str());
}

// ================================================
//  THEME NVS STORAGE
// ================================================
void loadThemePreference() {
  Preferences prefs;
  prefs.begin("theme-config", true);
  isDarkTheme = prefs.getBool("is_dark", false); // Default LIGHT theme
  prefs.end();
  mLabelSys[1] = isDarkTheme ? "Theme: DARK" : "Theme: LIGHT";
}

void saveThemePreference() {
  Preferences prefs;
  prefs.begin("theme-config", false);
  prefs.putBool("is_dark", isDarkTheme);
  prefs.end();
}

// ================================================
//  HOMING (Calibration Step)
// ================================================
void homing() {
  sendStat("calibrating");
  publishMqttStatus();
  if (curScreen == SCR_ACTUATOR) actUpdate("HOMING");
  motorBusy = true;
  int dly = startSpeed;

  for (int i = 0; i < 2400; i++) {
    if (stopNow) break;
    doStep(2400 - i);
    if (i < rampSteps) {
      dly = startSpeed - ((startSpeed - maxSpeed) * i / rampSteps);
    } else if (i > 2400 - rampSteps) {
      dly = maxSpeed + ((startSpeed - maxSpeed) * (i - (2400 - rampSteps)) / rampSteps);
    } else {
      dly = maxSpeed;
    }
    dly = constrain(dly, maxSpeed, startSpeed);
    delayMicroseconds(dly);
  }
  delay(100);

  if (!stopNow) {
    dly = startSpeed;
    for (int i = 0; i < 300; i++) { doStep(i); delayMicroseconds(dly); }
    delay(80);
    dly = startSpeed + 200;
    for (int i = 0; i < 300; i++) { doStep(300-i); delayMicroseconds(dly); }
  }

  stopCoils(); 
  motorPos = 0; 
  targetPos = 0; 
  motorBusy = false; 
  stopNow = false;
  sendStat("idle");
  publishMqttStatus();
  beep(1200, 60);
  if (curScreen == SCR_ACTUATOR) {
    lastBw = -1;
    actUpdate("IDLE");
  }
}

// ================================================
//  STATUS REPORT
// ================================================
void sendStat(const char* s) {
  int mm = map(motorPos, 0, totalSteps, 0, 80);
  Serial.printf("STATUS:%s|POS:%d|MM:%d\n", s, motorPos, mm);
}

// ================================================
//  RC522 RFID CARD SCANNER HANDLER
// ================================================
String lastScannedUid = "NONE";
String rfidStatusText = "READY - TAP CARD";

void handleRFID() {
  if (motorBusy) return;
  if (millis() - lastRfidScanMs < 200) return;
  lastRfidScanMs = millis();

  // Deselect TFT CS to ensure clear SPI bus for RC522
  digitalWrite(TFT_CS, HIGH);

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  beep(2200, 80);

  String cardUid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) cardUid += "0";
    cardUid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) cardUid += " ";
  }
  cardUid.toUpperCase();
  lastScannedUid = cardUid;

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  String cleanScanned = cleanUid(cardUid);

  // 1. If inside Admin Setup screen -> IMMEDIATELY SAVE ANY TAPPED CARD AS ADMIN CARD!
  if (curScreen == SCR_ADMIN_SETUP) {
    adminRegisterMode = false;
    if (addAdminCardToNvs(cleanScanned)) {
      adminStatusBanner = "[+] SAVED: " + cleanScanned;
      adminBannerMs = millis();
      beep(1200, 70); delay(80); beep(1600, 70); delay(80); beep(2000, 150);
    } else {
      adminStatusBanner = "[!] CARD ALREADY IN LIST";
      adminBannerMs = millis();
      beep(800, 150);
    }
    drawAdminSetupScreen();
    return; // NEVER DISPENSE IN ADMIN SETUP!
  }

  // 2. Check if scanned card is Registered Admin Card -> Toggle Admin Session Lock & OPEN MENU!
  if (isAdminCard(cleanScanned)) {
    if (!adminSessionActive) {
      // UNLOCK MENU via Admin Card!
      adminSessionActive = true;
      beep(1200, 50); delay(60); beep(1600, 50); delay(60); beep(2000, 100);
      curScreen = SCR_MENU;
      menuFull();
    } else {
      // LOCK & CLOSE MENU via Admin Card!
      adminSessionActive = false;
      beep(1800, 50); delay(60); beep(1200, 100);
      curScreen = SCR_STANDBY;
      standbyScreen();
    }
    return; // NEVER DISPENSE ON ADMIN CARD TAP!
  }

  // 3. If currently inside ANY Menu/Submenu/Test screen -> Do NOT trigger dispensing!
  if (curScreen != SCR_STANDBY) {
    if (curScreen == SCR_RFID) {
      drawRfidScreen(); // Update test screen display
    }
    return; // DO NOT DISPENSE WHEN INSIDE MENUS!
  }

  // 4. Normal Sanitary Card Tap Workflow (ONLY ON SCR_STANDBY!)
  rfidStatusText = "CARD READ -> SWEEPING...";
  Serial.printf("RFID Card Scanned! UID: %s\n", cardUid.c_str());

  if (mqttClient.connected()) {
    String payload = "{\"id\":\"" + String(VENDING_ID) + "\",\"event\":\"card_scanned\",\"uid\":\"" + cardUid + "\"}";
    mqttClient.publish(pubStatTopic.c_str(), payload.c_str());
  }

  // Save Card Scan to Unique Card Database (NVS)
  saveCardScanToDb(cardUid.c_str());

  // Launch Dispense Workflow
  runDispenseWorkflow(cardUid.c_str());
}

// ================================================
//  SETUP
// ================================================
void setup() {
  neopixelWrite(48, 0, 0, 0);

  Serial.begin(115200);
  delay(300);

  loadMotorPreferences();
  loadThemePreference();
  loadCardDbFromNvs();
  loadStockPreferences();
  loadAdminCardsFromNvs();

  // Initialize TOF050C / VL6180X I2C Distance Sensor
  initStockSensor();

  // Initialize shared Hardware SPI bus for TFT & RC522 RFID reader
  SPI.begin(TFT_SCLK, RFID_MISO_PIN, TFT_MOSI, RFID_SS_PIN);
  rfid.PCD_Init();

  pinMode(IN1, OUTPUT); pinMode(IN3, OUTPUT);
  if (!USE_2_PIN_MODE) { pinMode(IN2, OUTPUT); pinMode(IN4, OUTPUT); }
  pinMode(EEP_PIN, OUTPUT);
  digitalWrite(EEP_PIN, HIGH);
  stopCoils();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  tft.init(240, 320);
  tft.setRotation(0);
  SW = tft.width();
  SH = tft.height();

  WiFi.mode(WIFI_STA);
  Preferences prefs;
  prefs.begin("wifi-store", true);
  String savedSSID = prefs.getString("ssid", "");
  String savedPassword = prefs.getString("password", "");
  prefs.end();

  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  }

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  server.enableCORS(true);
  server.on("/", []() {
    sendCorsHeaders();
    server.send_P(200, "text/html", html_page);
  });
  server.on("/api/go", []() {
    sendCorsHeaders();
    if (server.hasArg("mm")) {
      int mm = server.arg("mm").toInt();
      beep(1500, 20);
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      executeGlobalMotorMove(mm);
    } else {
      server.send(400, "application/json", "{\"error\":\"missing mm\"}");
    }
  });
  server.on("/api/home", []() {
    sendCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    homing();
  });
  server.on("/api/calibrate_zero", []() {
    sendCorsHeaders();
    bool ok = calibrateZeroStockLimit();
    String json = "{\"status\":\"" + String(ok ? "ok" : "error") + "\",\"empty_depth_mm\":" + String(emptyStockDepthMm) + "}";
    server.send(200, "application/json", json);
  });
  server.on("/api/sweep", []() {
    sendCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    executeGlobalMotorMove(80);
    if (!stopNow) executeGlobalMotorMove(0);
  });
  server.on("/api/stop", []() {
    sendCorsHeaders();
    stopNow = true;
    targetPos = motorPos;
    stopCoils();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  server.on("/api/status", []() {
    sendCorsHeaders();
    int mm = map(motorPos, 0, totalSteps, 0, 80);
    String stat = "idle";
    if (motorBusy) {
      stat = (sweepState > 0) ? "sweeping" : "moving";
    }
    String json = "{\"id\":\"" + String(VENDING_ID) + "\",\"status\":\"" + stat + "\",\"pos\":" + String(motorPos) + ",\"mm\":" + String(mm) + "}";
    server.send(200, "application/json", json);
  });
  server.on("/api/stock", []() {
    sendCorsHeaders();
    int dist = readLiveStockDistanceMm();
    int pct = getStockPercentage();
    String json = "{\"sensor_online\":" + String(tofOnline ? "true" : "false") + ",\"distance_mm\":" + String(dist) + ",\"stock_percent\":" + String(pct) + ",\"empty_depth_mm\":" + String(emptyStockDepthMm) + "}";
    server.send(200, "application/json", json);
  });
  server.on("/api/logs", []() {
    sendCorsHeaders();
    String json = "{\"data_available_since_sec\":" + String(oldestDataSec) + ",\"total_unique_cards\":" + String(totalUniqueCards) + ",\"cards\":[";
    for (int i = 0; i < totalUniqueCards; i++) {
      json += "{\"uid\":\"" + String(cardDb[i].uid) + "\",\"total_scans\":" + String(cardDb[i].totalScans) + ",\"month_scans\":" + String(cardDb[i].monthScans) + ",\"first_seen_sec\":" + String(cardDb[i].firstSeenSec) + ",\"last_seen_sec\":" + String(cardDb[i].lastSeenSec) + "}";
      if (i < totalUniqueCards - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });
  server.begin();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(KEY0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);

  // Initialize Built-in Board RGB LED to OFF
  setRgbLed(0, 0, 0);

  startup();

  curScreen = SCR_STANDBY;
  standbyScreen();
  sendStat("idle");
}

// ================================================
//  MAIN LOOP
// ================================================
void loop() {
  handleSerial();
  server.handleClient();
  handleRFID();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMqtt();
    } else {
      mqttClient.loop();
    }
  }

  int diff = encCount;
  if (diff != 0) {
    encCount = 0;
  }

  bool psh = btnPress(ENC_SW, lastPSH);
  bool k0  = btnPress(KEY0, lastK0);

  if (psh) beep(2000, 10);
  if (k0)  beep(1600, 10);

  // Secret Stealth Mode Toggle (2x K0 + 2x Knob Button within 5s)
  if (psh || k0) {
    processStealthCombo(k0, psh);
    if (!isStealthMode && (curScreen == SCR_STANDBY)) {
      // Clear button flags on wakeup so Stealth Mode exit NEVER opens the menu!
      psh = false;
      k0 = false;
    }
  }

  if (isStealthMode) {
    setRgbLed(0, 0, 0);
    delay(20);
    return; // Pause UI execution while in Stealth Mode
  }

  // Activity tracker to control 30s Screensaver
  if (diff != 0 || psh || k0) {
    lastUserActivityMs = millis();
    if (curScreen == SCR_SCREENSAVER) {
      curScreen = SCR_STANDBY;
      standbyScreen();
      return;
    }
  }

  // Run 3D Octahedron Screensaver if idle in Standby for > 30 seconds
  if (curScreen == SCR_STANDBY && (millis() - lastUserActivityMs > 30000)) {
    curScreen = SCR_SCREENSAVER;
    tft.fillScreen(C_BK);
    drawHudBrackets();
  }

  // Continuous fluid RGB NeoPixel Rainbow rotation across ALL screens
  if (!motorBusy && curScreen != SCR_DISPENSE) {
    updateRainbowRgbLed();
  }

  if (curScreen == SCR_SCREENSAVER) {
    runScreensaverFrame();
    return;
  }

  if (curScreen != SCR_STANDBY && (diff != 0 || psh || k0)) {
    lastActionMs = millis();
  }



  if (curScreen == SCR_STANDBY) {
    if (pwStep > 0 && millis() - pwLastStepMs > 4000) {
      pwReset();
      beep(300, 100);
    }

    if (abs(diff) >= 2) {
      int dir = (diff > 0) ? 1 : -1;
      lastEnc = encCount;
      pwLastStepMs = millis();

      if (dir == expectedDir[pwStep]) {
        pwStep++;
        beep(900 + pwStep * 250, 30);

        if (pwStep == 4) {
          beep(1000, 50); delay(60);
          beep(1400, 50); delay(60);
          beep(1800, 100);

          curScreen = SCR_MENU;
          mSel = 0;
          lastEnc = encCount;
          pwReset();
          menuFull();
          lastActionMs = millis();
          delay(150);
          return;
        }
      } else {
        pwReset();
        beep(250, 120);
      }
    }

    if (psh) {
      beep(1500, 50);
      curScreen = SCR_MENU;
      mSel = 0;
      lastEnc = encCount;
      pwReset();
      menuFull();
      lastActionMs = millis();
      delay(150);
      return;
    }

    delay(20);
    return;
  }

  // 10-Second Inactivity Timeout: Return to Standby ONLY when on Root Main Menu (SCR_MENU) AND Admin Session is NOT Active
  if (curScreen == SCR_MENU && !adminSessionActive) {
    if (millis() - lastActionMs > 10000) {
      curScreen = SCR_STANDBY;
      standbyScreen();
      delay(150);
      return;
    }
  }

  // K0 Button Handling: Single click or Double-Click Kill-Switch
  if (k0) {
    lastActionMs = millis();
    if (checkK0KillSwitch()) {
      // DOUBLE CLICK KILL-SWITCH: Emergency cancel & return to Standby
      beep(400, 150);
      delay(100);
      beep(200, 200);
      curScreen = SCR_STANDBY;
      standbyScreen();
      lastEnc = encCount;
      return;
    }

    delay(100);
    if (curScreen == SCR_WIFI_PASS) {
      // Single K0 Click in Keyboard = FINAL ENTER / CONNECT!
      connectToWiFi();
    } else if (curScreen == SCR_SUB_STOCK || curScreen == SCR_SUB_MOTOR || curScreen == SCR_SUB_RFID || curScreen == SCR_SUB_SYS) {
      curScreen = SCR_MENU;
      menuFull();
    } else if (curScreen == SCR_MENU) {
      curScreen = SCR_STANDBY;
      standbyScreen();
    } else {
      curScreen = SCR_MENU;
      menuFull();
    }
    lastEnc = encCount;
    return;
  }

  if (curScreen == SCR_MENU) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      int oldSel = mSel;
      mSel += (diff > 0) ? 1 : -1;
      if (mSel < 0) mSel = NMENU - 1;
      if (mSel >= NMENU) mSel = 0;
      updateSubMenuSelection(mLabel, oldSel, mSel);
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      subSel = 0;
      switch (mSel) {
        case 0: curScreen = SCR_STOCK; stockFrameDrawn = false; drawStockScreen(); break;
        case 1: curScreen = SCR_SUB_MOTOR; drawSubMenu("MOTOR CONTROL", mLabelMotor, NSUB_MOTOR, 0); break;
        case 2: curScreen = SCR_SUB_RFID; drawSubMenu("RFID & CARD DB", mLabelRfid, NSUB_RFID, 0); break;
        case 3: curScreen = SCR_SUB_SYS; drawSubMenu("NETWORK & SYSTEM", mLabelSys, NSUB_SYS, 0); break;
      }
    }
  }
  else if (curScreen == SCR_SUB_STOCK) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      int oldSel = subSel;
      subSel += (diff > 0) ? 1 : -1;
      if (subSel < 0) subSel = NSUB_STOCK - 1;
      if (subSel >= NSUB_STOCK) subSel = 0;
      updateSubMenuSelection(mLabelStock, oldSel, subSel);
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      if (subSel == 0) { curScreen = SCR_STOCK; stockFrameDrawn = false; drawStockScreen(); }
      else if (subSel == 1) { curScreen = SCR_DIAG_TOF; diagFrameDrawn = false; drawI2cDiagScreen(); }
    }
  }
  else if (curScreen == SCR_SUB_MOTOR) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      int oldSel = subSel;
      subSel += (diff > 0) ? 1 : -1;
      if (subSel < 0) subSel = NSUB_MOTOR - 1;
      if (subSel >= NSUB_MOTOR) subSel = 0;
      updateSubMenuSelection(mLabelMotor, oldSel, subSel);
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      if (subSel == 0) { curScreen = SCR_ACTUATOR; targetPos = motorPos; lastBw = -1; actScreen(); }
      else if (subSel == 1) { curScreen = SCR_CALIB; drawCalibScreen(); }
      else if (subSel == 2) { executeGlobalMotorMove(maxLimitMm); if (!stopNow) executeGlobalMotorMove(0); lastEnc = encCount; }
    }
  }
  else if (curScreen == SCR_SUB_RFID) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      int oldSel = subSel;
      subSel += (diff > 0) ? 1 : -1;
      if (subSel < 0) subSel = NSUB_RFID - 1;
      if (subSel >= NSUB_RFID) subSel = 0;
      updateSubMenuSelection(mLabelRfid, oldSel, subSel);
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      if (subSel == 0) { curScreen = SCR_RFID; drawRfidScreen(); }
      else if (subSel == 1) { curScreen = SCR_CARD_LOGS; drawCardLogsScreen(); }
      else if (subSel == 2) { curScreen = SCR_ADMIN_SETUP; adminOptSel = 0; adminRegisterMode = false; drawAdminSetupScreen(); }
    }
  }
  else if (curScreen == SCR_ADMIN_SETUP) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      adminOptSel = (adminOptSel == 0) ? 1 : 0;
      drawAdminSetupScreen();
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      if (adminOptSel == 0) {
        adminRegisterMode = true;
        drawAdminSetupScreen();
      } else {
        clearAllAdminCardsFromNvs();
        adminRegisterMode = false;
        adminStatusBanner = "[!] CLEARED ALL ADMIN CARDS";
        adminBannerMs = millis();
        beep(1000, 100);
        drawAdminSetupScreen();
      }
    }
  }
  else if (curScreen == SCR_SUB_SYS) {
    if (abs(diff) >= 2) {
      lastActionMs = millis();
      lastEnc = encCount;
      int oldSel = subSel;
      subSel += (diff > 0) ? 1 : -1;
      if (subSel < 0) subSel = NSUB_SYS - 1;
      if (subSel >= NSUB_SYS) subSel = 0;
      updateSubMenuSelection(mLabelSys, oldSel, subSel);
    }
    if (psh) {
      lastActionMs = millis();
      delay(100);
      if (subSel == 0) { curScreen = SCR_WIFI; wifiScreen(); }
      else if (subSel == 1) {
        isDarkTheme = !isDarkTheme;
        mLabelSys[1] = isDarkTheme ? "Theme: DARK" : "Theme: LIGHT";
        saveThemePreference();
        drawSubMenu("NETWORK & SYSTEM", mLabelSys, NSUB_SYS, subSel);
      }
      else if (subSel == 2) { curScreen = SCR_INFO; infoScreen(); }
    }
  }
  else if (curScreen == SCR_ACTUATOR) {
    if (abs(diff) >= 2) {
      int clicks = diff / 2;
      lastEnc = encCount;
      int curMm = map(targetPos, 0, totalSteps, 0, 80);
      executeGlobalMotorMove(curMm + (clicks != 0 ? clicks : (diff > 0 ? 1 : -1)));
    }
    if (psh) { homing(); lastEnc = encCount; }
  }
  else if (curScreen == SCR_STOCK) {
    if (!stockFrameDrawn) {
      drawStockScreen();
      stockFrameDrawn = true;
    }
    static unsigned long lastStockRefreshMs = 0;
    if (millis() - lastStockRefreshMs > 60) {
      lastStockRefreshMs = millis();
      drawStockData();
    }
    if (psh) {
      delay(150);
      if (calibrateZeroStockLimit()) {
        beep(1800, 100);
      }
      drawStockScreen();
    }
    if (k0) {
      delay(150);
      stockFrameDrawn = false;
      curScreen = SCR_MENU;
      menuFull();
    }
  }
  else if (curScreen == SCR_DIAG_TOF) {
    if (!diagFrameDrawn) {
      drawI2cDiagScreen();
      diagFrameDrawn = true;
    }
    static unsigned long lastDiagRefreshMs = 0;
    if (millis() - lastDiagRefreshMs > 250) {
      lastDiagRefreshMs = millis();
      drawI2cDiagData();
    }
    if (psh) {
      delay(150);
      initStockSensor();
      beep(1500, 100);
      diagFrameDrawn = false;
      drawI2cDiagScreen();
      diagFrameDrawn = true;
    }
  }
  else if (curScreen == SCR_CALIB) {
    if (abs(diff) >= 2) {
      int clicks = diff / 2;
      lastEnc = encCount;
      maxLimitMm += (clicks != 0 ? clicks : (diff > 0 ? 1 : -1));
      maxLimitMm = constrain(maxLimitMm, 10, 80);
      drawCalibScreen();
    }
    if (psh) {
      delay(150);
      saveMotorPreferences();
      homing();
      curScreen = SCR_MENU;
      menuFull();
    }
  }
  else if (curScreen == SCR_RFID) {
    handleRFID();
    if (psh) {
      delay(150);
      curScreen = SCR_MENU;
      menuFull();
    }
  }
  else if (curScreen == SCR_CARD_LOGS) {
    if (abs(diff) >= 2) {
      int old = cardDbSel;
      cardDbSel += (diff > 0) ? 1 : -1;
      if (cardDbSel < 0) cardDbSel = max(0, totalUniqueCards - 1);
      if (cardDbSel >= totalUniqueCards) cardDbSel = 0;
      drawCardLogsScreen();
    }
    if (psh && totalUniqueCards > 0) {
      delay(150);
      curScreen = SCR_CARD_DETAIL;
      drawCardDetailScreen(cardDbSel);
    }
  }
  else if (curScreen == SCR_CARD_DETAIL) {
    if (psh) {
      delay(150);
      curScreen = SCR_CARD_LOGS;
      drawCardLogsScreen();
    }
  }
  else if (curScreen == SCR_WIFI) {
    if (abs(diff) >= 2) {
      lastEnc = encCount;
      int old = mSel;
      mSel += (diff > 0) ? 1 : -1;
      int maxVal = min(wifiCount, 6);
      if (mSel < 0) mSel = maxVal;
      if (mSel > maxVal) mSel = 0;
      wifiItem(old, false);
      wifiItem(mSel, true);
    }
    if (psh) {
      delay(150);
      if (mSel == 0) {
        wifiScreen();
      } else {
        selectedNetIdx = mSel - 1;
        wifiPassword = "";
        gridSel = 0;
        lastEnc = encCount;
        curScreen = SCR_WIFI_PASS;
        drawPasswordScreen();
      }
    }
  }
  else if (curScreen == SCR_WIFI_PASS) {
    if (abs(diff) >= 2) {
      lastEnc = encCount;
      int old = gridSel;
      gridSel += (diff > 0) ? 1 : -1;
      if (gridSel < 0) gridSel = 71;
      if (gridSel > 71) gridSel = 0;
      drawPassGridItem(old, false);
      drawPassGridItem(gridSel, true);
    }
    if (psh) {
      delay(150);
      if (gridSel < 70) {
        wifiPassword += grid[gridSel];
        drawPasswordText();
      } else if (gridSel == 70) {
        if (wifiPassword.length() > 0) {
          wifiPassword.remove(wifiPassword.length() - 1);
          drawPasswordText();
        }
      } else if (gridSel == 71) {
        curScreen = SCR_WIFI_CONNECTING;
        connectToWiFi();
      }
    }
  }

  if (millis() - lastStatusMs >= 250) {
    lastStatusMs = millis();
    if (!motorBusy) {
      sendStat("idle");
      publishMqttStatus();
    }
  }
  delay(1);
}

// ================================================
//  SERIAL HANDLER
// ================================================
void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim(); cmd.toUpperCase();
  if (cmd == "TEST" || cmd == "SWEEP") {
    executeGlobalMotorMove(80);
    if (!stopNow) executeGlobalMotorMove(0);
  } else if (cmd == "HOME" || cmd == "CALIBRATE") {
    homing();
  } else if (cmd == "STOP") {
    stopNow = true;
    targetPos = motorPos;
    stopCoils();
  } else if (cmd.startsWith("GO:")) {
    int mm = cmd.substring(3).toInt();
    executeGlobalMotorMove(mm);
  }
}

// ================================================
//  3D PERSPECTIVE MATRIX ROTATION
// ================================================
void rotate3D(float x, float y, float z, float ax, float ay, int &screenX, int &screenY, int cx, int cy) {
  float x1 = x * cos(ay) - z * sin(ay);
  float z1 = x * sin(ay) + z * cos(ay);
  float y2 = y * cos(ax) - z1 * sin(ax);
  float z2 = y * sin(ax) + z1 * cos(ax);
  float distance = 140.0;
  float scale = distance / (distance + z2);
  screenX = cx + (int)(x1 * scale * 1.5);
  screenY = cy + (int)(y2 * scale * 1.5);
}

void eraseOctahedron(int* px, int* py) {
  if (px[0] == 0 && py[0] == 0) return;
  uint16_t bg = getBgColor();
  tft.drawLine(px[0], py[0], px[2], py[2], bg);
  tft.drawLine(px[0], py[0], px[3], py[3], bg);
  tft.drawLine(px[0], py[0], px[4], py[4], bg);
  tft.drawLine(px[0], py[0], px[5], py[5], bg);
  
  tft.drawLine(px[1], py[1], px[2], py[2], bg);
  tft.drawLine(px[1], py[1], px[3], py[3], bg);
  tft.drawLine(px[1], py[1], px[4], py[4], bg);
  tft.drawLine(px[1], py[1], px[5], py[5], bg);
  
  tft.drawLine(px[2], py[2], px[3], py[3], bg);
  tft.drawLine(px[3], py[3], px[4], py[4], bg);
  tft.drawLine(px[4], py[4], px[5], py[5], bg);
  tft.drawLine(px[5], py[5], px[2], py[2], bg);
}

void drawOctahedron(float ax, float ay, int cx, int cy, uint16_t color, int* prevX, int* prevY) {
  float verts[6][3] = {
    {0, 32, 0},   // 0: Top
    {0, -32, 0},  // 1: Bottom
    {20, 0, 20},  // 2: Middle front-right
    {20, 0, -20}, // 3: Middle back-right
    {-20, 0, -20},// 4: Middle back-left
    {-20, 0, 20}  // 5: Middle front-left
  };
  int sx[6], sy[6];
  for (int i = 0; i < 6; i++) {
    rotate3D(verts[i][0], verts[i][1], verts[i][2], ax, ay, sx[i], sy[i], cx, cy);
  }
  
  tft.drawLine(sx[0], sy[0], sx[2], sy[2], color);
  tft.drawLine(sx[0], sy[0], sx[3], sy[3], color);
  tft.drawLine(sx[0], sy[0], sx[4], sy[4], color);
  tft.drawLine(sx[0], sy[0], sx[5], sy[5], color);
  
  tft.drawLine(sx[1], sy[1], sx[2], sy[2], color);
  tft.drawLine(sx[1], sy[1], sx[3], sy[3], color);
  tft.drawLine(sx[1], sy[1], sx[4], sy[4], color);
  tft.drawLine(sx[1], sy[1], sx[5], sy[5], color);
  
  tft.drawLine(sx[2], sy[2], sx[3], sy[3], C_PU);
  tft.drawLine(sx[3], sy[3], sx[4], sy[4], C_PU);
  tft.drawLine(sx[4], sy[4], sx[5], sy[5], C_PU);
  tft.drawLine(sx[5], sy[5], sx[2], sy[2], C_PU);

  for (int i = 0; i < 6; i++) {
    prevX[i] = sx[i];
    prevY[i] = sy[i];
  }
}

void drawLogoText(const char* txt, int tx, int ty, int size) {
  tft.setTextSize(size);
  tft.setTextColor(C_DG); // Dark gray shadow for depth
  tft.setCursor(tx + 2, ty + 2);
  tft.print(txt);
  
  tft.setTextColor(C_WH); // Pure white logo text
  tft.setCursor(tx, ty);
  tft.print(txt);
}

void drawHudBrackets() {
  uint16_t col = C_WH; // Pure white corner brackets
  int len = 12, inset = 6;
  tft.drawFastHLine(inset, inset, len, col);
  tft.drawFastVLine(inset, inset, len, col);
  tft.drawFastHLine(SW - inset - len, inset, len, col);
  tft.drawFastVLine(SW - inset - 1, inset, len, col);
  tft.drawFastHLine(inset, SH - inset - 1, len, col);
  tft.drawFastVLine(inset, SH - inset - len, len, col);
  tft.drawFastHLine(SW - inset - len, SH - inset - 1, len, col);
  tft.drawFastVLine(SW - inset - 1, SH - inset - len, len, col);
}

void drawQRCode(int x0, int y0, int pixelSize) {
  // Create URL dynamically using VENDING_ID
  String qrUrl = "https://www.taqiabbas.com/aether?id=" + String(VENDING_ID);
  
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)]; 
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrUrl.c_str());

  uint16_t blockCol = C_WH;  // Always white QR modules
  uint16_t bgCol = C_BK;     // Always black QR background

  int qrSize = qrcode.size;
  tft.fillRect(x0 - 4, y0 - 4, (qrSize * pixelSize) + 8, (qrSize * pixelSize) + 8, bgCol);

  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      uint16_t col = qrcode_getModule(&qrcode, x, y) ? blockCol : bgCol;
      tft.fillRect(x0 + (x * pixelSize), y0 + (y * pixelSize), pixelSize, pixelSize, col);
    }
  }
}

void playStartupMelody() {
  beep(1047, 70); delay(90);  // C6
  beep(1318, 70); delay(90);  // E6
  beep(1568, 70); delay(90);  // G6
  beep(2093, 150); delay(160); // C7
}

bool checkK0KillSwitch() {
  unsigned long now = millis();
  if (now - lastK0PressMs < 2000 && lastK0PressMs > 0) {
    lastK0PressMs = 0;
    return true; // KILL SWITCH TRIGGERED!
  }
  lastK0PressMs = now;
  return false;
}

void drawHexagon(int cx, int cy, int r, uint16_t color) {
  for (int i = 0; i < 6; i++) {
    float a1 = i * M_PI / 3.0;
    float a2 = (i + 1) * M_PI / 3.0;
    int x1 = cx + (int)(r * cos(a1));
    int y1 = cy + (int)(r * sin(a1));
    int x2 = cx + (int)(r * cos(a2));
    int y2 = cy + (int)(r * sin(a2));
    tft.drawLine(x1, y1, x2, y2, color);
  }
}

void startup() {
  playStartupMelody();

  // --- Belgium Startup Screen ---
  tft.fillScreen(C_WH);
  
  tft.setTextSize(2);
  tft.setTextColor(C_BK);
  tft.setCursor(50, 30);
  tft.print("Made with");
  
  int heartX = 168;
  int heartY = 38;
  tft.fillCircle(heartX - 5, heartY, 5, C_RD);
  tft.fillCircle(heartX + 5, heartY, 5, C_RD);
  tft.fillTriangle(heartX - 10, heartY, heartX + 10, heartY, heartX, heartY + 11, C_RD);
  
  tft.setCursor(60, 52);
  tft.print("in Belgium");
  
  int flagX = (SW - 90) / 2;
  int flagY = 95;
  tft.drawRect(flagX - 1, flagY - 1, 92, 62, tft.color565(220, 220, 220));
  tft.fillRect(flagX, flagY, 30, 60, C_BK);
  tft.fillRect(flagX + 30, flagY, 30, 60, 0xFFE0);
  tft.fillRect(flagX + 60, flagY, 30, 60, C_RD);
  
  tft.setTextColor(C_BK);
  tft.setTextSize(2);
  tft.setCursor(60, 185);
  tft.print("a StartLab");
  tft.setCursor(24, 210);
  tft.print("Brussels Project");
  
  delay(1200);

  // --- CYBERPUNK WARP TUNNEL & HEXAGON CORE STARTUP ANIMATION ---
  tft.fillScreen(C_BK);
  int cx = SW / 2;
  int cy = SH / 2 - 10;

  drawHudBrackets();

  // 1. Cyber Warp Tunnel Lines converging into center
  for (int step = 0; step < 25; step++) {
    float progress = (float)step / 25.0;
    int r = (int)(progress * 48.0);
    
    drawHexagon(cx, cy, r, C_CY);
    tft.drawLine(0, 0, (int)(progress * cx), (int)(progress * cy), C_CY);
    tft.drawLine(SW, 0, (int)(SW - progress * cx), (int)(progress * cy), C_CY);
    tft.drawLine(0, SH, (int)(progress * cx), (int)(SH - progress * cy), C_GL);
    tft.drawLine(SW, SH, (int)(SW - progress * cx), (int)(SH - progress * cy), C_GL);

    beep(300 + step * 40, 12);
    delay(25);
  }

  // 2. Glowing Core Burst
  for (int r = 10; r < 75; r += 12) {
    drawHexagon(cx, cy, r, C_WH);
    beep(1200 + r * 15, 10);
    delay(20);
    drawHexagon(cx, cy, r, C_BK);
  }

  tft.fillScreen(C_BK);
  drawHudBrackets();

  // 3. Huge Holographic "AETHER" Logo Scan-in
  const char* txt = "AETHER";
  int tx = cx - 54;
  int ty = cy - 10;
  
  for (int x = tx - 10; x < tx + 120; x += 8) {
    tft.drawFastVLine(x, ty - 4, 32, C_WH);
    beep(1000 + x * 4, 8);
    delay(15);
    drawLogoText(txt, tx, ty, 3);
    tft.drawFastVLine(x, ty - 4, 32, C_BK);
  }
  noTone(BUZZER_PIN);
  drawLogoText(txt, tx, ty, 3);

  // 4. Typewriter Subtitle Text
  tft.setTextSize(1);
  tft.setTextColor(C_GL);
  tft.setCursor(44, ty + 32);
  const char* sub = "VENDING CARE SYSTEM #5552";
  for (int i = 0; i < strlen(sub); i++) {
    tft.print(sub[i]);
    beep(2000, 6);
    delay(15);
  }
  
  // 5. High-Tech Cyber Progress Bar
  int ly = SH - 25;
  tft.drawRoundRect(20, ly, SW - 40, 8, 4, C_DG);
  for (int i = 0; i < 10; i++) {
    tft.fillRect(24 + i * 20, ly + 2, 16, 4, C_CY);
    beep(600 + i * 70, 15);
    delay(50);
  }
  
  beep(880, 80);
  delay(90);
  beep(1100, 120);
}

void standbyScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);
  
  // Solid Bright White Header Box with Cyan Border
  tft.fillRect(0, 0, SW, 52, C_WH);
  tft.drawFastHLine(0, 52, SW, C_CY);

  // Solid Crisp Black "AETHER" Logo Title (Text Size 3, 100% Solid Black!)
  tft.setTextSize(3);
  tft.setTextColor(C_BK);
  tft.setCursor(SW / 2 - 52, 10);
  tft.print("AETHER");

  // Solid Black Subtitle Text
  tft.setTextSize(1);
  tft.setTextColor(C_BK);
  tft.setCursor(44, 36);
  tft.print("VENDING CARE #5552");

  int qrX = (SW - 29 * 7) / 2;
  int qrY = 64;
  drawQRCode(qrX, qrY, 7);

  // High-contrast bottom callout banner
  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextSize(1);
  tft.setTextColor(getTextMain());
  tft.setCursor(20, SH - 16);
  tft.print("SCAN QR OR TAP RFID CARD");

  pwReset();
  lastEnc = encCount;
  curScreen = SCR_STANDBY;
}

void menuFull() {
  drawSubMenu("MAIN MENU", mLabel, NMENU, mSel);
}

void drawSubMenu(const char* title, const char** labels, int count, int sel) {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  tft.fillRect(0, 0, SW, HDR_H, isDarkTheme ? C_BK : 0xE71C);
  tft.drawFastHLine(0, HDR_H, SW, getTextMain());
  tft.setTextSize(2);
  tft.setTextColor(getTextMain());
  tft.setCursor(12, 12);
  tft.print(title);

  // Ultra-Visible High-Contrast WiFi & Admin Badges in Header
  if (adminSessionActive) {
    tft.fillRoundRect(SW - 150, 10, 56, 24, 4, C_GL);
    tft.setTextSize(1);
    tft.setTextColor(C_BK);
    tft.setCursor(SW - 144, 18);
    tft.print("ADMIN");
  }

  bool isWifiOk = (WiFi.status() == WL_CONNECTED);
  uint16_t wifiBadgeCol = isWifiOk ? C_GN : C_RD;
  tft.fillRoundRect(SW - 88, 10, 78, 24, 4, wifiBadgeCol);
  tft.drawRoundRect(SW - 88, 10, 78, 24, 4, C_WH);
  tft.setTextSize(1);
  tft.setTextColor(isWifiOk ? C_BK : C_WH);
  tft.setCursor(SW - 80, 18);
  tft.print(isWifiOk ? "WIFI: ON" : "WIFI: OFF");

  for (int i = 0; i < count; i++) {
    drawSubMenuItem(labels, i, i == sel);
  }
}

void drawSubMenuItem(const char** labels, int idx, bool sel, int scrollOffset) {
  int y = ITEM_Y0 + idx * ITEM_SP;
  uint16_t boxBg = getCardBg(sel);
  uint16_t textFg = getCardFg(sel);

  // In-place box update with ZERO full-screen flicker!
  tft.fillRect(10, y, SW - 20, ITEM_H, boxBg);
  tft.drawRect(10, y, SW - 20, ITEM_H, getTextMain());

  tft.setTextWrap(false); // Disable line wrap to prevent text clipping
  tft.setTextSize(2); // LARGE, BOLD, ULTRA-READABLE MENU TEXT!
  tft.setTextColor(textFg); // High-contrast crisp text

  int textLen = strlen(labels[idx]);
  if (sel && textLen > 13) {
    // Smooth Marquee Ticker Scroll for long labels!
    int startX = 18 - scrollOffset;
    tft.setCursor(startX, y + 13);
    tft.print(labels[idx]);
  } else {
    tft.setCursor(18, y + 13);
    tft.print(labels[idx]);
  }
}

void updateSubMenuSelection(const char** labels, int oldSel, int newSel) {
  if (oldSel != newSel) {
    drawSubMenuItem(labels, oldSel, false);
    drawSubMenuItem(labels, newSel, true);
  }
}

void actScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(map(y, 0, HDR_H, 35, 70), map(y, 0, HDR_H, 12, 30), 0);
    } else {
      c = tft.color565(255, map(y, 0, HDR_H, 245, 225), map(y, 0, HDR_H, 220, 180));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, getAccentGold());
  tft.setTextSize(2);
  tft.setTextColor(getAccentGold());
  tft.setCursor(24, 14);
  tft.print("ACTUATOR CONTROL");

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Knob: Move | Click: Home | K0: Back");

  actUpdate("IDLE");
}

void actUpdate(const char* s) {
  uint16_t bg = getBgColor();
  tft.setTextSize(2);
  tft.setCursor(20, 70);
  tft.setTextColor(getTextSub(), bg);
  tft.print("STATE: ");
  if (strcmp(s, "MOVING") == 0 || strcmp(s, "HOMING") == 0)
    tft.setTextColor(getAccentGold(), bg);
  else
    tft.setTextColor(C_GN, bg);
  tft.printf("%-10s", s);

  tft.setTextColor(getTextMain(), bg);
  int mm = map(motorPos, 0, totalSteps, 0, 80);
  tft.setCursor(20, 110);
  tft.printf("POS:   %2d mm / 80 mm", mm);
  tft.setCursor(20, 150);
  tft.printf("STEPS: %4d / 3200 ", motorPos);

  int trackY = 210;
  int trackH = 10;
  int trackW = SW - 40;
  
  tft.drawRoundRect(20, trackY, trackW, trackH, 5, isDarkTheme ? C_DG : 0xC618);
  
  int bw = map(motorPos, 0, totalSteps, 0, trackW);
  if (bw > 0) {
    tft.fillRoundRect(20, trackY, bw, trackH, 5, getAccentCyan());
  }
  
  if (lastBw != bw) {
    if (lastBw >= 0) {
      tft.fillCircle(20 + lastBw, trackY + (trackH / 2), 8, bg);
      tft.drawRoundRect(20, trackY, trackW, trackH, 5, isDarkTheme ? C_DG : 0xC618);
      if (lastBw > 0) {
        tft.fillRoundRect(20, trackY, lastBw, trackH, 5, getAccentCyan());
      }
    }
    tft.fillCircle(20 + bw, trackY + (trackH / 2), 8, getAccentGold());
    lastBw = bw;
  }
}

void wifiScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_GN);
  tft.setTextSize(2);
  tft.setTextColor(C_GN);
  tft.setCursor(48, 14);
  tft.print("WIFI SCANNER");

  tft.setTextSize(1);
  tft.setTextColor(getAccentGold());
  tft.setCursor(20, 52);
  tft.print("Scanning Airwaves...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  wifiCount = min(n, 6);

  tft.fillRect(20, 50, 150, 12, bg);

  mSel = 0;
  lastEnc = encCount;

  for (int i = 0; i <= wifiCount; i++) {
    wifiItem(i, i == mSel);
  }

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Turn: Scroll | Press: Connect | K0: Back");
}

void wifiItem(int i, bool sel) {
  int y = 55 + i * 35;
  uint16_t bg = getCardBg(sel);
  uint16_t fg = getCardFg(sel);
  
  tft.fillRect(4, y, SW - 8, 30, bg);
  tft.fillRect(4, y, 4, 30, getCardBar(sel));
  
  if (i == 0) {
    tft.setTextSize(1);
    tft.setTextColor(sel ? fg : C_GN, bg);
    tft.setCursor(20, y + 11);
    tft.print("<< RESCAN NETWORKS >>");
  } else {
    int netIdx = i - 1;
    if (netIdx >= wifiCount) return;
    tft.setTextSize(1);
    tft.setTextColor(fg, bg);
    tft.setCursor(20, y + 4);
    String ssid = WiFi.SSID(netIdx);
    if (ssid.length() > 18) ssid = ssid.substring(0, 18);
    tft.print(ssid);
    
    tft.setTextColor(sel ? fg : getTextSub(), bg);
    tft.setCursor(20, y + 16);
    int rssi = WiFi.RSSI(netIdx);
    tft.printf("%d dBm  Ch %d", rssi, WiFi.channel(netIdx));
    
    int bx = SW - 26;
    int by = y + 18;
    uint16_t barCol = (rssi > -67) ? C_GN : (rssi > -80) ? getAccentGold() : C_RD;
    tft.fillRect(bx, by - 4, 3, 4, barCol);
    tft.fillRect(bx + 5, by - 8, 3, 8, (rssi > -80) ? barCol : (isDarkTheme ? C_DG : 0xC618));
    tft.fillRect(bx + 10, by - 12, 3, 12, (rssi > -67) ? barCol : (isDarkTheme ? C_DG : 0xC618));
  }
}

void drawPasswordScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);
  
  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_GN);
  tft.setTextSize(2);
  tft.setTextColor(C_GN);
  tft.setCursor(38, 14);
  tft.print("ENTER PASSWORD");
  
  tft.setTextSize(1);
  tft.setTextColor(getTextSub());
  tft.setCursor(12, 55);
  tft.print("SSID: ");
  tft.setTextColor(getTextMain());
  tft.print(WiFi.SSID(selectedNetIdx));
  
  tft.drawRoundRect(10, 75, SW - 20, 26, 4, getAccentCyan());
  drawPasswordText();
  
  for (int i = 0; i < 72; i++) {
    drawPassGridItem(i, i == gridSel);
  }
  
  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Knob: Move | Click: Select | K0: Cancel");
}

void drawPasswordText() {
  uint16_t bg = getBgColor();
  tft.fillRect(14, 79, SW - 28, 18, bg);
  tft.setTextSize(1);
  tft.setTextColor(getTextMain());
  tft.setCursor(16, 84);
  if (wifiPassword.length() == 0) {
    tft.setTextColor(getTextSub());
    tft.print("Enter password...");
  } else {
    tft.print(wifiPassword);
    tft.print("_");
  }
}

void drawPassGridItem(int idx, bool sel) {
  int row = idx / 6;
  int col = idx % 6;
  int x = col * 40;
  int y = 115 + row * 15;
  
  uint16_t bg = sel ? getAccentCyan() : (isDarkTheme ? C_BK : tft.color565(240, 242, 246));
  uint16_t fg = sel ? C_BK : getTextMain();
  
  if (idx == 71) {
    fg = sel ? C_BK : C_GN;
  } else if (idx == 70) {
    fg = sel ? C_BK : getAccentGold();
  }
  
  tft.fillRect(x + 2, y + 1, 36, 13, bg);
  tft.setTextSize(1);
  tft.setTextColor(fg, bg);
  
  const char* label = grid[idx];
  int len = strlen(label);
  int cx = x + 20 - (len * 3);
  tft.setCursor(cx, y + 4);
  tft.print(label);
}

void connectToWiFi() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);
  
  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_GN);
  tft.setTextSize(2);
  tft.setTextColor(C_GN);
  tft.setCursor(62, 14);
  tft.print("CONNECTING");
  
  tft.setTextSize(1);
  tft.setTextColor(getTextSub());
  tft.setCursor(20, 70);
  tft.print("Connecting to network:");
  tft.setTextColor(getTextMain());
  tft.setCursor(20, 90);
  tft.print(WiFi.SSID(selectedNetIdx));
  
  tft.setTextColor(getAccentGold());
  tft.setCursor(20, 130);
  tft.print("Please wait...");
  
  WiFi.begin(WiFi.SSID(selectedNetIdx).c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  bool connected = false;
  while (attempts < 20) {
    int lx = 20 + (attempts * 10);
    tft.fillRect(lx, 160, 8, 8, getAccentCyan());
    beep(1000 + attempts * 50, 20);
    delay(500);
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    attempts++;
  }
  
  if (connected) {
    Preferences prefs;
    prefs.begin("wifi-store", false);
    prefs.putString("ssid", WiFi.SSID(selectedNetIdx));
    prefs.putString("password", wifiPassword);
    prefs.end();

    beep(900, 70); delay(80); beep(1300, 70); delay(80); beep(1800, 100);

    tft.fillRect(20, 130, 200, 50, bg);
    tft.setTextSize(2);
    tft.setTextColor(C_GN);
    tft.setCursor(20, 130);
    tft.print("SUCCESS!");
    
    tft.setTextSize(1);
    tft.setTextColor(getTextSub());
    tft.setCursor(20, 165);
    tft.print("Connected successfully!");
    tft.setCursor(20, 185);
    tft.print("IP: ");
    tft.setTextColor(getTextMain());
    tft.print(WiFi.localIP().toString());
    
    reconnectMqtt();
    
    delay(3000);
    curScreen = SCR_MENU;
    menuFull();
  } else {
    beep(400, 150); delay(100); beep(300, 200);

    tft.fillRect(20, 130, 200, 50, bg);
    tft.setTextSize(2);
    tft.setTextColor(C_RD);
    tft.setCursor(20, 130);
    tft.print("FAILED");
    
    tft.setTextSize(1);
    tft.setTextColor(getTextSub());
    tft.setCursor(20, 165);
    tft.print("Connection timed out.");
    tft.setCursor(20, 185);
    tft.print("Returning to entry screen...");
    delay(3000);
    curScreen = SCR_WIFI_PASS;
    drawPasswordScreen();
  }
  lastEnc = encCount;
}

void infoScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(map(y, 0, HDR_H, 18, 40), 0, map(y, 0, HDR_H, 25, 60));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 230, 210), map(y, 0, HDR_H, 210, 190), map(y, 0, HDR_H, 250, 235));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_PU);
  tft.setTextSize(2);
  tft.setTextColor(C_PU);
  tft.setCursor(56, 14);
  tft.print("SYSTEM INFO");

  tft.setTextSize(2);
  tft.setTextColor(getAccentCyan());
  tft.setCursor(20, 65);
  tft.print("Aether Tech");

  tft.setTextColor(getAccentGold());
  tft.setCursor(20, 95);
  tft.printf("Machine #%s", VENDING_ID);

  tft.setTextSize(1);
  tft.setTextColor(getTextSub());
  tft.setCursor(20, 130);
  tft.print("CPU: ESP32-S3 @ 240 MHz");
  tft.setCursor(20, 150);
  tft.print("Cloud: HiveMQ MQTT Hub");
  tft.setCursor(20, 170);
  tft.print("MQTT: ");
  if (mqttClient.connected()) {
    tft.setTextColor(C_GN);
    tft.print("ONLINE [CONNECTED]");
  } else {
    tft.setTextColor(C_RD);
    tft.print("OFFLINE");
  }
  tft.setTextColor(getTextSub());
  tft.setCursor(20, 190);
  tft.printf("Local IP: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(C_GN);
    tft.print(WiFi.localIP().toString());
  } else {
    tft.setTextColor(C_RD);
    tft.print("Disconnected");
  }
  tft.setTextColor(getTextSub());
  tft.setCursor(20, 210);
  tft.printf("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
  tft.setCursor(20, 230);
  tft.printf("Motor: %d / 3200 steps", motorPos);

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("K0: Back to main menu");
}

void drawCalibScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(map(y, 0, HDR_H, 35, 70), map(y, 0, HDR_H, 12, 30), 0);
    } else {
      c = tft.color565(255, map(y, 0, HDR_H, 245, 225), map(y, 0, HDR_H, 220, 180));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, getAccentGold());
  tft.setTextSize(2);
  tft.setTextColor(getAccentGold());
  tft.setCursor(18, 14);
  tft.print("MOTOR CALIBRATION");

  int cardY = 60;
  tft.fillRect(10, cardY, SW - 20, 150, getCardBg(false));
  tft.drawRect(10, cardY, SW - 20, 150, getAccentCyan());

  tft.setTextSize(2);
  tft.setTextColor(getTextMain(), getCardBg(false));
  tft.setCursor(20, cardY + 15);
  tft.print("MAX LIMIT CONFIG");

  tft.setTextSize(3);
  tft.setTextColor(getAccentGold(), getCardBg(false));
  tft.setCursor(20, cardY + 50);
  tft.printf("LIMIT: %2d mm", maxLimitMm);

  tft.setTextSize(1);
  tft.setTextColor(getTextSub(), getCardBg(false));
  tft.setCursor(20, cardY + 95);
  tft.print("Rotate Knob : Adjust (10-80mm)");
  tft.setCursor(20, cardY + 115);
  tft.print("Press Knob  : Save & Run Home");

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Rotate: Adjust | Click: Save | K0: Back");
}

void drawRfidScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_GN);
  tft.setTextSize(2);
  tft.setTextColor(C_GN);
  tft.setCursor(36, 14);
  tft.print("RFID CARD TEST");

  digitalWrite(TFT_CS, HIGH);
  byte ver = rfid.PCD_ReadRegister(rfid.VersionReg);

  tft.fillRect(10, 55, SW - 20, 40, getCardBg(false));
  tft.setTextSize(1);
  tft.setCursor(20, 65);
  if (ver == 0x91 || ver == 0x92) {
    tft.setTextColor(C_GN, getCardBg(false));
    tft.printf("READER CHIP: ONLINE (v0x%02X)", ver);
  } else {
    tft.setTextColor(C_RD, getCardBg(false));
    tft.printf("READER CHIP: CHECK WIRING (0x%02X)", ver);
  }
  tft.setCursor(20, 80);
  tft.setTextColor(getTextSub(), getCardBg(false));
  tft.print("SPI: SS:9 | RST:2 | MISO:1 | SCK:12");

  tft.fillRect(10, 105, SW - 20, 105, getCardBg(true));
  tft.drawRect(10, 105, SW - 20, 105, getAccentCyan());

  tft.setTextSize(1);
  tft.setTextColor(getCardFg(true), getCardBg(true));
  tft.setCursor(20, 115);
  tft.print("LAST SCANNED CARD UID:");

  tft.setTextSize(2);
  tft.setTextColor(C_GL, getCardBg(true));
  tft.setCursor(20, 135);
  tft.print(lastScannedUid);

  tft.setTextSize(1);
  tft.setTextColor(getAccentCyan(), getCardBg(true));
  tft.setCursor(20, 175);
  tft.print(rfidStatusText);

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Tap Card to Sweep | K0: Back");
}

// -------------------------------------------------------------
//  BLOOMING FLOWER & PULSING HEART CARE ANIMATION (ZERO FLICKER!)
// -------------------------------------------------------------
void drawCareDispenseFrame() {
  uint16_t bg = isDarkTheme ? C_BK : C_WH;
  tft.fillScreen(bg);

  // Soft Vibrant Header Bar ("AETHER CARE")
  uint16_t hdrCol = isDarkTheme ? tft.color565(45, 10, 35) : tft.color565(255, 230, 242);
  tft.fillRect(0, 0, SW, HDR_H, hdrCol);
  tft.drawFastHLine(0, HDR_H, SW, C_MG);
  tft.setTextSize(2);
  tft.setTextColor(C_MG);
  tft.setCursor(50, 12);
  tft.print("AETHER CARE");

  // Footer Bar
  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Sanitary Vending Care  ❤️");

  // Outer Card Border
  tft.drawRoundRect(10, 48, SW - 20, 168, 8, isDarkTheme ? C_DG : 0xE71C);
}

void drawCareDispenseAnimation(int cx, int cy, float frameAngle, const char* titleMsg, const char* subMsg) {
  uint16_t bg = isDarkTheme ? C_BK : C_WH;

  // 1. Draw 8 Blooming Flower Petals (NO background wipe = ZERO FLICKER!)
  float bloomRadius = 26.0 + 4.0 * sin(frameAngle * 2.0);
  uint16_t petalColor = isDarkTheme ? tft.color565(255, 140, 185) : tft.color565(255, 160, 200);

  for (int i = 0; i < 8; i++) {
    float a = frameAngle + (i * M_PI / 4.0);
    int px = cx + (int)(bloomRadius * cos(a));
    int py = cy + (int)(bloomRadius * sin(a));
    tft.fillCircle(px, py, 11, petalColor);
    tft.drawCircle(px, py, 11, C_MG);
  }

  // 2. Center Glowing Flower Core
  tft.fillCircle(cx, cy, 17, 0xFFE0); // Warm Golden Yellow
  tft.drawCircle(cx, cy, 17, C_WH);

  // 3. Pulsing Center Heart
  int heartScale = (sin(frameAngle * 3.0) > 0) ? 1 : 0;
  int hx = cx;
  int hy = cy - 2;
  tft.fillCircle(hx - 4, hy - 3, 4 + heartScale, C_RD);
  tft.fillCircle(hx + 4, hy - 3, 4 + heartScale, C_RD);
  tft.fillTriangle(hx - 8 - heartScale, hy - 1, hx + 8 + heartScale, hy - 1, hx, hy + 7 + heartScale, C_RD);

  // 4. Overwrite Centered Title Message in-place (Text Size 2, Large & Bold!)
  tft.setTextSize(2);
  tft.setTextColor(C_MG, bg);
  int tLen = strlen(titleMsg);
  int tx = max(14, (SW - tLen * 12) / 2);
  tft.setCursor(tx, cy + 44);
  tft.print(titleMsg);

  // 5. Overwrite Centered Subtitle Message in-place (Text Size 1, Soft & Friendly)
  tft.setTextSize(1);
  tft.setTextColor(getTextMain(), bg);
  int sLen = strlen(subMsg);
  int sx = max(14, (SW - sLen * 6) / 2);
  tft.setCursor(sx, cy + 68);
  tft.print(subMsg);
}

void runDispenseWorkflow(const char* cardUid) {
  curScreen = SCR_DISPENSE;
  int cx = SW / 2;
  int cy = 110;

  // Draw static Care frame ONCE on entry
  drawCareDispenseFrame();

  // Phase 1: READ INITIAL DISTANCE (D_initial) & CHECK EMPTY STOCK
  drawCareDispenseAnimation(cx, cy, 0.0, "CHECKING STOCK...", "Measuring ToF Sensor...");
  int D_initial = readFilteredStockDistanceMm();

  // Check Out of Stock: D_initial >= emptyStockDepthMm - 10mm
  if (D_initial >= 0 && D_initial >= (emptyStockDepthMm - 10)) {
    setRgbLed(255, 0, 0); // Red LED
    beep(300, 300); // Alarm tone
    drawCareDispenseAnimation(cx, cy, 0.0, "EMPTY STOCK!", "Machine Out of Stock");
    delay(2500);
    setRgbLed(0, 0, 0);
    curScreen = SCR_STANDBY;
    standbyScreen();
    return;
  }

  // Phase 2: CARD AUTHENTICATION
  setRgbLed(0, 255, 0); // Green LED ON
  beep(1800, 50); delay(60); beep(2400, 80);
  drawCareDispenseAnimation(cx, cy, 0.4, "CARD VERIFIED!", "Dispensing Pad... ❤️");
  delay(200);

  // Phase 3: MOTOR EXTENDING FORWARD
  drawCareDispenseAnimation(cx, cy, 0.8, "DISPENSING CARE...", "Actuating Slider Motor...");
  executeGlobalMotorMove(maxLimitMm);
  delay(150);

  // Phase 4: READ FINAL DISTANCE (D_final) & CALCULATE DELTA
  int D_final = readFilteredStockDistanceMm();
  int deltaMm = D_final - D_initial;
  Serial.printf("TOF VEND EVAL: D_init=%d mm, D_final=%d mm, Delta=%d mm\n", D_initial, D_final, deltaMm);

  bool vendOk = false;
  bool isJamRetried = false;

  // Check 1: Unstable Drop / Negative Delta -> Re-sample after 200ms settlement
  if (deltaMm < 0) {
    drawCareDispenseAnimation(cx, cy, 1.0, "RE-SAMPLING...", "Settling stack...");
    delay(200);
    D_final = readFilteredStockDistanceMm();
    deltaMm = D_final - D_initial;
  }

  // Check 2: Valid Drop (0.7cm to 2.5cm = 7mm to 25mm drop)
  if (deltaMm >= 7 && deltaMm <= 25) {
    vendOk = true;
  } 
  // Check 3: Sensor offline or minor tolerance fallback (4mm to 30mm)
  else if (!tofOnline || (deltaMm >= 4 && deltaMm <= 30)) {
    vendOk = true;
  }
  // Check 4: Jam / Mechanical Failure (Delta < 5mm) -> RETRY CLEARANCE!
  else if (deltaMm < 5) {
    isJamRetried = true;
    setRgbLed(255, 180, 0); // Gold warning LED
    beep(1000, 100);
    drawCareDispenseAnimation(cx, cy, 1.2, "JAM DETECTED!", "RETRYING CLEARING JAM...");
    
    // Rapid clearing pulse: retract 10mm and push forward rapidly
    executeGlobalMotorMove(maxLimitMm - 10);
    delay(60);
    executeGlobalMotorMove(maxLimitMm);
    delay(150);

    // Re-sample ToF distance after retry
    int D_retry = readFilteredStockDistanceMm();
    int retryDelta = D_retry - D_initial;
    if (retryDelta >= 5) {
      vendOk = true;
      drawCareDispenseAnimation(cx, cy, 1.4, "JAM CLEARED!", "Pad Successfully Dropped!");
      delay(300);
    }
  }

  // Phase 5: RETRACT MOTOR TO HOME
  drawCareDispenseAnimation(cx, cy, 1.6, vendOk ? "PLEASE TAKE ITEM" : "RETRACTING SLIDER", vendOk ? "Prepared with Care ❤️" : "Clearing slider...");
  executeGlobalMotorMove(0);

  // Phase 6: FINAL STATUS DISPLAY
  if (vendOk) {
    setRgbLed(0, 255, 0);
    beep(1200, 80); delay(90); beep(1800, 80); delay(90); beep(2400, 150);
    drawCareDispenseAnimation(cx, cy, 2.4, isJamRetried ? "JAM CLEARED! SUCCESS" : "VEND SUCCESSFUL!", "Have a Great Day! ✨");
    delay(1800);
  } else {
    setRgbLed(255, 0, 0);
    beep(400, 150); delay(100); beep(300, 250);
    drawCareDispenseAnimation(cx, cy, 2.4, "MACHINE JAMMED!", "Mechanical Jam Detected");
    delay(2500);
  }

  setRgbLed(0, 0, 0); // LED OFF
  curScreen = SCR_STANDBY;
  standbyScreen();
}

void drawCardLogsScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_MG);
  tft.setTextSize(2);
  tft.setTextColor(C_MG);
  tft.setCursor(30, 14);
  tft.print("CARD DATABASE");

  if (totalUniqueCards == 0) {
    tft.setTextSize(1);
    tft.setTextColor(getTextSub());
    tft.setCursor(20, 100);
    tft.print("No unique cards registered yet.");
    tft.setCursor(20, 120);
    tft.print("Tap an RFID card to record data!");
  } else {
    tft.setTextSize(1);
    tft.setTextColor(getAccentGold());
    tft.setCursor(10, 48);
    uint32_t nowSec = millis() / 1000;
    tft.printf("DATA SINCE: %ds AGO | CARDS: %d", (int)(nowSec - oldestDataSec), totalUniqueCards);

    int maxDisplay = min(totalUniqueCards, 5);
    int startIdx = max(0, cardDbSel - 2);
    if (startIdx + maxDisplay > totalUniqueCards) {
      startIdx = max(0, totalUniqueCards - maxDisplay);
    }

    for (int i = 0; i < maxDisplay; i++) {
      int idx = startIdx + i;
      int y = 65 + i * 27;
      bool sel = (idx == cardDbSel);
      
      tft.fillRect(6, y, SW - 12, 24, getCardBg(sel));
      tft.drawRect(6, y, SW - 12, 24, sel ? getAccentCyan() : (isDarkTheme ? C_DG : 0xC618));
      
      tft.setTextSize(1);
      tft.setTextColor(sel ? getAccentGold() : getCardFg(false), getCardBg(sel));
      tft.setCursor(12, y + 8);
      tft.printf("#%02d", idx + 1);

      tft.setTextColor(getCardFg(sel), getCardBg(sel));
      tft.setCursor(42, y + 8);
      tft.print(cardDb[idx].uid);

      tft.setTextColor(sel ? C_MG : getTextSub(), getCardBg(sel));
      tft.setCursor(SW - 65, y + 8);
      tft.printf("%d Taps", cardDb[idx].totalScans);
    }
  }

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_DG : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Rotate: Select | Click: Detail | K0: Back");
}

void drawCardDetailScreen(int idx) {
  if (idx < 0 || idx >= totalUniqueCards) return;
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  for (int y = 0; y < HDR_H; y++) {
    uint16_t c;
    if (isDarkTheme) {
      c = tft.color565(0, map(y, 0, HDR_H, 20, 55), map(y, 0, HDR_H, 5, 18));
    } else {
      c = tft.color565(map(y, 0, HDR_H, 220, 200), map(y, 0, HDR_H, 255, 235), map(y, 0, HDR_H, 230, 210));
    }
    tft.drawFastHLine(0, y, SW, c);
  }
  tft.drawFastHLine(0, HDR_H, SW, C_MG);
  tft.setTextSize(2);
  tft.setTextColor(C_MG);
  tft.setCursor(24, 14);
  tft.print("CARD AUDIT STATS");

  int cardY = 55;
  tft.fillRect(10, cardY, SW - 20, 155, getCardBg(true));
  tft.drawRect(10, cardY, SW - 20, 155, getAccentCyan());

  tft.setTextSize(1);
  tft.setTextColor(getAccentCyan(), getCardBg(true));
  tft.setCursor(20, cardY + 12);
  tft.printf("RECORD #%02d DETAILS:", idx + 1);

  tft.setTextSize(2);
  tft.setTextColor(getAccentGold(), getCardBg(true));
  tft.setCursor(20, cardY + 28);
  tft.print(cardDb[idx].uid);

  tft.setTextSize(1);
  tft.setTextColor(getTextMain(), getCardBg(true));

  tft.setCursor(20, cardY + 58);
  tft.printf("TOTAL LIFETIME SCANS : %d", cardDb[idx].totalScans);

  tft.setCursor(20, cardY + 78);
  tft.printf("THIS MONTH SCANS     : %d", cardDb[idx].monthScans);

  uint32_t nowSec = millis() / 1000;
  tft.setCursor(20, cardY + 98);
  tft.printf("LAST SCANNED TIME    : %ds ago", (int)(nowSec - cardDb[idx].lastSeenSec));

  tft.setCursor(20, cardY + 118);
  tft.printf("FIRST REGISTERED TIME: %ds ago", (int)(nowSec - cardDb[idx].firstSeenSec));
}

void drawStockScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  tft.fillRect(0, 0, SW, HDR_H, isDarkTheme ? C_BK : 0xE71C);
  tft.drawFastHLine(0, HDR_H, SW, getTextMain());
  tft.setTextSize(2);
  tft.setTextColor(getTextMain());
  tft.setCursor(12, 12);
  tft.print("STOCK LEVEL");

  int cardY = 55;
  uint16_t cBg = getCardBg(false);
  tft.fillRect(10, cardY, SW - 20, 175, cBg);
  tft.drawRect(10, cardY, SW - 20, 175, getTextMain());

  tft.setTextSize(1);
  tft.setTextColor(tofOnline ? (isDarkTheme ? C_GN : 0x03E0) : C_RD, cBg);
  tft.setCursor(20, cardY + 12);
  tft.print(tofOnline ? "STOCK SENSOR: ACTIVE" : "STOCK SENSOR: OFFLINE");

  // Progress Bar Outer Rect (drawn ONCE)
  int barX = 20;
  int barY = cardY + 88;
  int barW = SW - 60;
  int barH = 24;
  tft.drawRect(barX, barY, barW, barH, getTextMain());

  drawStockData();

  tft.fillRect(0, SH - FTR_H, SW, FTR_H, getFooterBg());
  tft.setTextColor(isDarkTheme ? C_WH : C_BK);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Click Knob: Set Zero Stock | K0: Back");
}

void drawStockData() {
  // Overwrite dynamic values in-place (100% ZERO FLICKER!)
  int pct = getStockPercentage();
  int cardY = 55;
  uint16_t cBg = getCardBg(false);

  // 1. Bold Large Stock Percentage Title (Text Size 3)
  tft.fillRect(20, cardY + 36, SW - 60, 28, cBg);
  tft.setTextSize(3);
  if (pct == 0) {
    tft.setTextColor(C_RD, cBg);
    tft.setCursor(20, cardY + 36);
    tft.print("STOCK: 0%");
  } else if (pct >= 100) {
    tft.setTextColor(isDarkTheme ? C_GN : 0x03E0, cBg);
    tft.setCursor(20, cardY + 36);
    tft.print("STOCK: 100%");
  } else {
    tft.setTextColor(isDarkTheme ? C_GL : 0xE500, cBg);
    tft.setCursor(20, cardY + 36);
    tft.printf("STOCK: %d%%", pct);
  }

  // 2. Visual Progress Bar Fill Area
  int barX = 20;
  int barY = cardY + 88;
  int barW = SW - 60;
  int barH = 24;
  tft.fillRect(barX + 2, barY + 2, barW - 4, barH - 4, cBg);

  int fillW = map(pct, 0, 100, 0, barW - 4);
  fillW = constrain(fillW, 0, barW - 4);

  if (fillW > 0) {
    uint16_t barColor = (pct > 50) ? (isDarkTheme ? C_GN : 0x03E0) : ((pct > 20) ? (isDarkTheme ? C_GL : 0xE500) : C_RD);
    tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, barColor);
  }

  // 3. Zero Stock Note Banner
  tft.fillRect(20, cardY + 130, SW - 60, 16, cBg);
  tft.setTextSize(1);
  if (lastStockCalibNote.length() > 0) {
    tft.setTextColor(isDarkTheme ? C_CY : 0x001F, cBg);
    tft.setCursor(20, cardY + 130);
    tft.print(lastStockCalibNote);
  } else {
    tft.setTextColor(getTextMain(), cBg);
    tft.setCursor(20, cardY + 130);
    tft.printf("Zero Stock Length: %d mm", emptyStockDepthMm);
  }
}

void drawI2cDiagScreen() {
  tft.fillScreen(C_BK);

  // Header Bar (static frame)
  tft.fillRect(0, 0, SW, HDR_H, C_BK);
  tft.drawFastHLine(0, HDR_H, SW, C_WH);
  tft.setTextSize(2);
  tft.setTextColor(C_WH);
  tft.setCursor(12, 14);
  tft.print("I2C TOF DIAGNOSTIC");

  // I2C Bus Scan (run once on frame draw)
  tft.setTextSize(1);
  tft.setTextColor(C_WH);
  tft.setCursor(10, 46);
  tft.print("I2C Bus (SDA:41 SCL:42)");

  int foundCount = 0;
  char devList[64] = "";
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      char hexBuf[10];
      snprintf(hexBuf, sizeof(hexBuf), "0x%02X ", addr);
      strcat(devList, hexBuf);
      foundCount++;
    }
  }

  tft.fillRect(10, 58, SW - 20, 12, C_BK);
  tft.setCursor(10, 60);
  if (foundCount > 0) {
    tft.setTextColor(C_WH);
    tft.printf("FOUND (%d): %s", foundCount, devList);
  } else {
    tft.setTextColor(C_WH);
    tft.print("NO DEVICES! Check 3.3V/GND/41/42");
  }

  // Data card frame (static)
  int cardY = 78;
  tft.fillRect(10, cardY, SW - 20, 145, C_BK);
  tft.drawRect(10, cardY, SW - 20, 145, C_WH);

  // Static labels
  tft.setTextSize(1);
  tft.setTextColor(C_GR, C_BK);
  tft.setCursor(20, cardY + 10);
  tft.print("SENSOR STATUS  :");
  tft.setCursor(20, cardY + 28);
  tft.print("RAW RANGE (mm) :");
  tft.setCursor(20, cardY + 46);
  tft.print("STATUS CODE    :");
  tft.setCursor(20, cardY + 64);
  tft.print("SCALED DIST(mm):");
  tft.setCursor(20, cardY + 82);
  tft.print("STOCK LEVEL  %% :");
  tft.setCursor(20, cardY + 100);
  tft.print("EMPTY DEPTH(mm):");

  tft.setTextColor(C_GR, C_BK);
  tft.setCursor(20, cardY + 125);
  tft.print("Live refresh 4x/sec");

  // Footer (static)
  tft.fillRect(0, SH - FTR_H, SW, FTR_H, 0x1082);
  tft.setTextColor(C_WH);
  tft.setTextSize(1);
  tft.setCursor(10, SH - 16);
  tft.print("Click: Re-Init | K0: Exit");

  // Draw initial data values
  drawI2cDiagData();
}

void drawI2cDiagData() {
  // Only overwrite the 6 dynamic value regions in-place (ZERO FLICKER!)
  int cardY = 78;
  int valX = 130; // X offset for value column
  int valW = SW - 20 - valX + 10; // Width to clear value area

  uint8_t range = vl6180x.readRange();
  int distMm = readLiveStockDistanceMm();
  int pct = getStockPercentage();

  tft.setTextSize(1);
  tft.setTextColor(C_WH, C_BK);

  // 1. Sensor Status
  tft.fillRect(valX, cardY + 8, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 10);
  tft.printf("%-20s", tofOnline ? "ONLINE (0x29)" : "OFFLINE");

  // 2. Raw Range
  tft.fillRect(valX, cardY + 26, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 28);
  tft.printf("%-10d", range);

  // 3. Status Code
  tft.fillRect(valX, cardY + 44, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 46);
  tft.printf("%-10d", vl6180x.readRangeStatus());

  // 4. Scaled Distance
  tft.fillRect(valX, cardY + 62, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 64);
  tft.printf("%-10d", distMm);

  // 5. Stock Level
  tft.fillRect(valX, cardY + 80, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 82);
  tft.printf("%-10d", pct);

  // 6. Empty Depth Setting
  tft.fillRect(valX, cardY + 98, valW, 10, C_BK);
  tft.setCursor(valX, cardY + 100);
  tft.printf("%-10d", emptyStockDepthMm);
}

// -------------------------------------------------------------
//  RAINBOW RGB LED, STEALTH COMBO & SCREENSAVER ENGINE
// -------------------------------------------------------------
void updateRainbowRgbLed() {
  static float hue = 0.0;
  static unsigned long lastRainbowMs = 0;
  if (millis() - lastRainbowMs < 20) return; // 50Hz smooth updates
  lastRainbowMs = millis();

  hue += 0.01; // Smooth aesthetic rotation
  if (hue >= 1.0) hue = 0.0;

  float h = hue * 6.0;
  int i = floor(h);
  float f = h - i;
  uint8_t q = (uint8_t)(255 * (1.0 - f));
  uint8_t t = (uint8_t)(255 * f);

  uint8_t r = 0, g = 0, b = 0;
  switch (i % 6) {
    case 0: r = 255; g = t;   b = 0;   break;
    case 1: r = q;   g = 255; b = 0;   break;
    case 2: r = 0;   g = 255; b = t;   break;
    case 3: r = 0;   g = q;   b = 255; break;
    case 4: r = t;   g = 0;   b = 255; break;
    case 5: r = 255; g = 0;   b = q;   break;
  }

  setRgbLed(r, g, b);
}

void processStealthCombo(bool isK0, bool isKnob) {
  unsigned long now = millis();
  if (comboStartMs == 0 || now - comboStartMs > 5000) {
    comboStartMs = now;
    comboK0Count = 0;
    comboKnobCount = 0;
  }

  if (isK0)   comboK0Count++;
  if (isKnob) comboKnobCount++;

  if (comboK0Count >= 2 && comboKnobCount >= 2) {
    comboK0Count = 0;
    comboKnobCount = 0;
    comboStartMs = 0;

    isStealthMode = !isStealthMode;
    if (isStealthMode) {
      digitalWrite(TFT_BLK, LOW); // Turn OFF backlight
      tft.fillScreen(C_BK);
      setRgbLed(0, 0, 0);
      stopCoils();
      beep(1200, 50); delay(60); beep(800, 100);
    } else {
      digitalWrite(TFT_BLK, HIGH); // Turn ON backlight
      beep(800, 50); delay(60); beep(1600, 100);
      curScreen = SCR_STANDBY;
      standbyScreen();
    }
  }
}

// -------------------------------------------------------------
//  VIBRANT 3D CYBER MATRIX & PLASMA CORE VIDEO ENGINE
// -------------------------------------------------------------
struct StarParticle {
  float x, y, z;
};
const int NUM_STARS = 24;
StarParticle stars[NUM_STARS];
bool starsInited = false;

void initStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = (float)(random(-110, 110));
    stars[i].y = (float)(random(-110, 110));
    stars[i].z = (float)(random(10, 180));
  }
  starsInited = true;
}

void runScreensaverFrame() {
  if (curScreen != SCR_SCREENSAVER) return;
  updateRainbowRgbLed();

  static unsigned long stateStartMs = 0;
  static int savState = 0; // 0 = Plasma Core, 1 = Hologram Orbit, 2 = Particle Burst
  static float angle = 0.0;
  static unsigned long lastFrameMs = 0;

  if (millis() - lastFrameMs < 35) return;
  lastFrameMs = millis();

  int cx = SW / 2;
  int cy = SH / 2 - 10;

  if (!starsInited) initStarfield();

  if (stateStartMs == 0) {
    stateStartMs = millis();
    savState = 0;
    tft.fillScreen(C_BK);
  }

  unsigned long elapsed = millis() - stateStartMs;

  static int pSx[NUM_STARS] = {0}, pSy[NUM_STARS] = {0};
  static int pR = 0;
  static int pRx[6] = {0}, pRy[6] = {0};
  static int pPx[12] = {0}, pPy[12] = {0};
  static bool phase1Init = false;

  // -------------------------------------------------------------------
  // PHASE 0: 3D PLASMA CORE & WARP STARFIELD STREAM (1.6s)
  // -------------------------------------------------------------------
  if (savState == 0) {
    phase1Init = false;
    float progress = (float)elapsed / 1600.0;
    if (progress > 1.0) progress = 1.0;

    // Erase previous starfield particles in C_BK
    for (int i = 0; i < NUM_STARS; i++) {
      if (pSx[i] > 0 && pSy[i] > 0) {
        tft.drawPixel(pSx[i], pSy[i], C_BK);
      }
    }

    // Erase previous hexagon core
    if (pR > 0) drawHexagon(cx, cy, pR, C_BK);
    pR = (int)(progress * 38.0);

    // Update & draw 3D Warp Starfield stream
    for (int i = 0; i < NUM_STARS; i++) {
      stars[i].z -= 7.0f;
      if (stars[i].z <= 2.0f) {
        stars[i].x = (float)(random(-110, 110));
        stars[i].y = (float)(random(-110, 110));
        stars[i].z = 180.0f;
      }
      int sx = cx + (int)(stars[i].x * 120.0f / stars[i].z);
      int sy = cy + (int)(stars[i].y * 120.0f / stars[i].z);
      if (sx >= 0 && sx < SW && sy >= 0 && sy < SH) {
        tft.drawPixel(sx, sy, (i % 2 == 0) ? C_CY : C_GL);
        pSx[i] = sx;
        pSy[i] = sy;
      }
    }

    // Draw central pulsing Hexagon Core
    drawHexagon(cx, cy, pR, C_CY);
    drawHudBrackets();

    if (elapsed >= 1600) {
      savState = 1;
      stateStartMs = millis();
      tft.fillScreen(C_BK);
    }
  }
  // -------------------------------------------------------------------
  // PHASE 1: HUGE "AETHER" LOGO & 3D ORBITING PLASMA FLARES (3.0s)
  // -------------------------------------------------------------------
  else if (savState == 1) {
    if (!phase1Init) {
      phase1Init = true;
      tft.fillScreen(C_BK);

      drawHudBrackets();

      // Draw background Cyberpunk Hexagon Grid Mesh
      drawHexagon(cx - 72, cy - 32, 22, 0x10A4);
      drawHexagon(cx + 72, cy - 32, 22, 0x10A4);
      drawHexagon(cx - 72, cy + 32, 22, 0x10A4);
      drawHexagon(cx + 72, cy + 32, 22, 0x10A4);

      // Draw Static Huge "AETHER" Logo ONCE (Text Size 4 = 144px width = ~80% SW)
      tft.setTextSize(4);
      tft.setTextColor(C_GL, C_BK);
      tft.setCursor(cx - 70, cy - 16);
      tft.print("AETHER");

      // Draw Static Instruction Banners ONCE
      tft.setTextSize(1);
      tft.setTextColor(C_CY, C_BK);
      tft.setCursor(cx - 48, SH - 36);
      tft.print("TAP ACCESS CARD");

      tft.setTextColor(getAccentGold(), C_BK);
      tft.setCursor(cx - 78, SH - 20);
      tft.print("OR PRESS TOP BUTTON FOR QR");

      for (int i = 0; i < 6; i++) {
        pRx[i] = 0; pRy[i] = 0;
      }
    }

    angle += 0.06;

    // Erase previous 6 hexagon satellite nodes in C_BK
    for (int i = 0; i < 6; i++) {
      if (pRx[i] != 0 && pRy[i] != 0) {
        drawHexagon(pRx[i], pRy[i], 6, C_BK);
      }
    }

    // Draw new 6 orbiting hexagon satellite nodes
    for (int i = 0; i < 6; i++) {
      float a = angle + (i * M_PI / 3.0);
      int rx = cx + (int)(94.0 * cos(a));
      int ry = cy + (int)(44.0 * sin(a));
      uint16_t nodeCol = (i % 2 == 0) ? C_CY : 0xF81F; // Light Blue Cyan & Magenta
      drawHexagon(rx, ry, 6, nodeCol);
      pRx[i] = rx;
      pRy[i] = ry;
    }

    if (elapsed >= 3000) {
      savState = 2;
      stateStartMs = millis();
    }
  }
  // -------------------------------------------------------------------
  // PHASE 2: QUANTUM PARTICLE DISPERSION BURST (1.4s)
  // -------------------------------------------------------------------
  else if (savState == 2) {
    float progress = (float)elapsed / 1400.0;
    if (progress > 1.0) progress = 1.0;

    // Erase previous 12 particles & lines
    for (int i = 0; i < 12; i++) {
      if (pPx[i] != 0 || pPy[i] != 0) {
        tft.fillCircle(pPx[i], pPy[i], 3, C_BK);
        tft.drawLine(cx, cy, pPx[i], pPy[i], C_BK);
      }
    }

    // Draw new 12 particles & lines
    for (int i = 0; i < 12; i++) {
      float a = i * (2.0 * M_PI / 12.0);
      float dist = progress * 150.0;
      int px = cx + (int)(dist * cos(a));
      int py = cy + (int)(dist * sin(a));
      uint16_t col = (i % 2 == 0) ? C_CY : C_GL;
      tft.fillCircle(px, py, 3, col);
      tft.drawLine(cx, cy, px, py, col);
      pPx[i] = px;
      pPy[i] = py;
    }

    if (elapsed >= 1400) {
      savState = 0;
      stateStartMs = millis();
      tft.fillScreen(C_BK);
      for (int i = 0; i < 12; i++) { pPx[i] = 0; pPy[i] = 0; }
    }
  }
}

void drawAdminSetupScreen() {
  uint16_t bg = getBgColor();
  tft.fillScreen(bg);

  tft.fillRect(0, 0, SW, HDR_H, isDarkTheme ? C_BK : 0xE71C);
  tft.drawFastHLine(0, HDR_H, SW, getTextMain());
  tft.setTextSize(2);
  tft.setTextColor(getTextMain());
  tft.setCursor(12, 12);
  tft.print("ADMIN CARD SETUP");

  // Notification Banner (e.g. after scanning a card)
  int startY = 52;
  if (adminBannerMs > 0 && millis() - adminBannerMs < 3500) {
    tft.fillRect(10, 50, SW - 20, 24, C_GN);
    tft.drawRect(10, 50, SW - 20, 24, C_WH);
    tft.setTextSize(1);
    tft.setTextColor(C_BK, C_GN);
    tft.setCursor(16, 57);
    tft.print(adminStatusBanner);
    startY = 78;
  }

  // Admin Card List Box
  int cardY = startY;
  int cardH = 92;
  tft.fillRect(10, cardY, SW - 20, cardH, getCardBg(true));
  tft.drawRect(10, cardY, SW - 20, cardH, getTextMain());

  tft.setTextSize(1);
  tft.setTextColor(getCardFg(true), getCardBg(true));
  tft.setCursor(18, cardY + 8);
  tft.printf("REGISTERED ADMIN CARDS (%d/%d):", adminCardCount, MAX_ADMIN_CARDS);

  if (adminCardCount == 0) {
    tft.setTextColor(getAccentGold(), getCardBg(true));
    tft.setCursor(18, cardY + 28);
    tft.print("(No Admin Cards Registered)");
  } else {
    tft.setTextColor(getAccentGold(), getCardBg(true));
    for (int i = 0; i < adminCardCount && i < 3; i++) {
      tft.setCursor(18, cardY + 26 + (i * 16));
      tft.printf("%d. UID: %s", i + 1, adminCards[i].c_str());
    }
  }

  tft.setTextSize(1);
  if (adminRegisterMode) {
    tft.setTextColor(C_GN, getCardBg(true));
    tft.setCursor(18, cardY + cardH - 18);
    tft.print(">> TAP ANY RFID CARD NOW <<");
  } else {
    tft.setTextColor(getCardFg(true), getCardBg(true));
    tft.setCursor(18, cardY + cardH - 18);
    tft.print("Status: Ready to Add/Clear Cards");
  }

  // Option 1: Tap to Add Admin
  int opt1Y = cardY + cardH + 10;
  bool sel1 = (adminOptSel == 0);
  tft.fillRect(10, opt1Y, SW - 20, 36, sel1 ? getTextMain() : getCardBg(true));
  tft.drawRect(10, opt1Y, SW - 20, 36, getTextMain());
  tft.setTextColor(sel1 ? getCardBg(true) : getCardFg(true), sel1 ? getTextMain() : getCardBg(true));
  tft.setTextSize(2);
  tft.setCursor(20, opt1Y + 10);
  tft.print("1. ADD ADMIN CARD");

  // Option 2: Clear All Admin Cards
  int opt2Y = opt1Y + 44;
  bool sel2 = (adminOptSel == 1);
  tft.fillRect(10, opt2Y, SW - 20, 36, sel2 ? getTextMain() : getCardBg(true));
  tft.drawRect(10, opt2Y, SW - 20, 36, getTextMain());
  tft.setTextColor(sel2 ? getCardBg(true) : getCardFg(true), sel2 ? getTextMain() : getCardBg(true));
  tft.setTextSize(2);
  tft.setCursor(20, opt2Y + 10);
  tft.print("2. CLEAR CARDS");
}






