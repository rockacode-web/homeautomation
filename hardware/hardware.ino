//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##                                                                                                              ##
//##################################################################################################################

#include <Arduino.h>
#include <rom/rtc.h>
#include <math.h>
#include <pgmspace.h>

// ---------------------- TFT ----------------------
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// TFT PINS
#define TFT_DC    17
#define TFT_CS    5
#define TFT_RST   16
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// ---------------------- IMAGES ----------------------
#include "lockclose.h"
#include "lockopen.h"

// ---------------------- WIFI / HTTP ----------------------
#include <WiFi.h>
#include <HTTPClient.h>

// ------------------ USER CONFIG ------------------

// MQTT CLIENT CONFIG
static const char* pubtopic      = "620171712";
static const char* subtopic[]    = {"620171712_sub", "/elet2415"};
static const char* mqtt_server   = "www.yanacreations.com";
static uint16_t    mqtt_port     = 1883;

// WIFI CREDENTIALS
const char* ssid     = "Galaxy A15 4257";
const char* password = "12345876";

// *** IMPORTANT: Replace with the actual IP of the PC running your backend ***
#define BACKEND_IP "10.77.247.87"

// BUTTONS + POT
#define BTN1     25
#define BTN2     26
#define BTN3     27
#define POT_PIN  35

// ------------------ TASK HANDLES ------------------
TaskHandle_t xMQTT_Connect       = NULL;
TaskHandle_t xNTPHandle          = NULL;
TaskHandle_t xLOOPHandle         = NULL;
TaskHandle_t xUpdateHandle       = NULL;
TaskHandle_t xButtonCheckeHandle = NULL;

// ------------------ REQUIRED VARIABLES (LAB) ------------------
uint8_t currentDigit = 1;         // 1..4 — which digit the pot edits
bool    lockState    = false;     // false = locked, true = unlocked
uint8_t passcode[4]  = {0,0,0,0}; // 4-digit passcode

// ------------------ FUNCTION DECLARATIONS ------------------
void callback(char* topic, byte* payload, unsigned int length);
void vButtonCheck(void* pvParameters);
void vUpdate(void* pvParameters);

void checkHEAP(const char* Name);
void initMQTT(void);
unsigned long getTimeStamp(void);
void initialize(void);
bool publish(const char* topic, const char* payload);

void digit1(uint8_t number);
void digit2(uint8_t number);
void digit3(uint8_t number);
void digit4(uint8_t number);

void checkPasscode(void);
void showLockState(void);
void resetPasscode(void);

void vButtonCheckFunction(void);

// ---------------------- Include lab headers ----------------------
#include "NTP.h"
#include "mqtt.h"

// ---------------------- UI layout constants ----------------------
static const int TITLE_H  = 42;
static const int INFO_Y   = 52;
static const int INFO_H   = 78;
static const int LOCK_Y   = 135;
static const int STATUS_Y = 215;
static const int DIGIT_Y  = 252;

// ---------------------- Potentiometer helper ----------------------
static int readPotAvg() {
  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(POT_PIN);
    delayMicroseconds(500);
  }
  return (int)(sum / 8);
}

// ---------------------- UI draw helpers ----------------------
static void drawDigitBox(int x, int y, int w, int h, uint8_t n, bool selected) {
  tft.fillRoundRect(x, y, w, h, 6, ILI9341_GREEN);
  uint16_t border = selected ? ILI9341_YELLOW : ILI9341_BLACK;
  tft.drawRoundRect(x, y, w, h, 6, border);

  tft.setTextSize(3);
  tft.setTextColor(ILI9341_BLACK, ILI9341_GREEN);
  tft.setCursor(x + 16, y + 12);
  tft.print((int)(n % 10));
}

static void drawAllDigits() {
  const int w   = 45;
  const int h   = 55;
  const int gap = 10;
  const int x0  = 15;

  drawDigitBox(x0 + 0*(w+gap), DIGIT_Y, w, h, passcode[0], currentDigit == 1);
  drawDigitBox(x0 + 1*(w+gap), DIGIT_Y, w, h, passcode[1], currentDigit == 2);
  drawDigitBox(x0 + 2*(w+gap), DIGIT_Y, w, h, passcode[2], currentDigit == 3);
  drawDigitBox(x0 + 3*(w+gap), DIGIT_Y, w, h, passcode[3], currentDigit == 4);
}

static void drawStartupScreen() {
  tft.fillScreen(ILI9341_WHITE);

  // Title bar
  tft.fillRect(0, 0, 240, TITLE_H, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.print("REMOTE LOCK");

  // Instruction panel
  tft.fillRoundRect(10, INFO_Y, 220, INFO_H, 10, ILI9341_NAVY);
  tft.drawRoundRect(10, INFO_Y, 220, INFO_H, 10, ILI9341_WHITE);

  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setTextSize(1);
  tft.setCursor(18, INFO_Y + 10); tft.print("BTN1: Select digit");
  tft.setCursor(18, INFO_Y + 26); tft.print("POT : Change value");
  tft.setCursor(18, INFO_Y + 42); tft.print("BTN2: Submit code");
  tft.setCursor(18, INFO_Y + 58); tft.print("BTN3: Lock reset");

  // Lab spec: display zero in all boxes on start-up
  passcode[0] = 0;
  passcode[1] = 0;
  passcode[2] = 0;
  passcode[3] = 0;
  currentDigit = 1;

  drawAllDigits();
  showLockState();
}

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(115200);

  // Buttons: INPUT_PULLUP — wire GPIO -> button -> GND
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  // TFT
  tft.begin();
  drawStartupScreen();

  // WiFi, MQTT, NTP (starter code)
  initialize();

  // Start button check task (starter code)
  vButtonCheckFunction();

  Serial.println("[SETUP] Setup complete.");

  // Redraw after tasks start to prevent overwrite
  drawStartupScreen();
}

// ------------------ LOOP ------------------
// The loop ONLY handles the potentiometer.
// Button logic lives exclusively in vButtonCheck to avoid double-triggers.
void loop() {
  static uint8_t lastDigit = 255;
  static int     lastRaw   = -1;

  int raw = readPotAvg();

  // Ignore jitter smaller than ~1% of full range
  if (lastRaw != -1 && abs(raw - lastRaw) < 40) {
    delay(60);
    return;
  }
  lastRaw = raw;

  // Map 12-bit ADC (0-4095) to 0-9 inclusive
  uint8_t digit = (uint8_t)map(raw, 0, 4095, 0, 9);

  if (digit != lastDigit) {
    lastDigit = digit;

    // Assign mapped pot value to the currently selected passcode digit
    if (digit != passcode[currentDigit - 1]) {
      passcode[currentDigit - 1] = digit;
      drawAllDigits();
    }
  }

  delay(60);
}

//####################################################################
//#                          UTIL FUNCTIONS                           #
//####################################################################

/*
 * vButtonCheck — runs in its own FreeRTOS task.
 * Button logic lives here ONLY (not in loop) to prevent double-triggers.
 *
 * BTN1: Increment currentDigit (1..4, wrap back to 1)
 * BTN2: Invoke checkPasscode
 * BTN3: Reset lockState to false, reset passcode digits to 0, redraw
 */
void vButtonCheck(void* pvParameters) {
  configASSERT(((uint32_t)pvParameters) == 1);

  Serial.println("[TASK] vButtonCheck started.");

  for (;;) {
    // --- BTN1: select next digit ---
    if (digitalRead(BTN1) == LOW) {
      currentDigit++;
      if (currentDigit > 4) currentDigit = 1;
      Serial.print("[BTN1] currentDigit = ");
      Serial.println(currentDigit);
      drawAllDigits();
      vTaskDelay(220 / portTICK_PERIOD_MS); // debounce
    }

    // --- BTN2: submit passcode ---
    if (digitalRead(BTN2) == LOW) {
      Serial.println("[BTN2] Checking passcode...");
      checkPasscode();
      vTaskDelay(220 / portTICK_PERIOD_MS);
    }

    // --- BTN3: lock reset — clears lockState AND resets displayed digits ---
    if (digitalRead(BTN3) == LOW) {
      Serial.println("[BTN3] Lock reset.");
      lockState = false;
      resetPasscode();      // zero out digits + redraw
      showLockState();      // update lock image / status text
      vTaskDelay(220 / portTICK_PERIOD_MS);
    }

    vTaskDelay(40 / portTICK_PERIOD_MS);
  }
}

void vUpdate(void* pvParameters) {
  configASSERT(((uint32_t)pvParameters) == 1);
  for (;;) vTaskDelay(1000 / portTICK_PERIOD_MS);
}

unsigned long getTimeStamp(void) {
  time_t now;
  time(&now);
  return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\n[MQTT] Message received on topic: %s\n", topic);
  char* received = new char[length + 1]{0};
  for (unsigned int i = 0; i < length; i++) received[i] = (char)payload[i];
  Serial.printf("[MQTT] Payload: %s\n", received);
  delete[] received;
}

bool publish(const char* topic, const char* payload) {
  bool res = false;
  try {
    res = mqtt.publish(topic, payload);
    if (!res) { res = false; throw false; }
  } catch (...) {
    Serial.printf("\n[MQTT] Error >> Unable to publish message\n");
  }
  return res;
}

// ---------------------- Digit functions ----------------------
// Each updates the corresponding passcode slot and redraws all boxes.
void digit1(uint8_t number) { passcode[0] = number % 10; drawAllDigits(); }
void digit2(uint8_t number) { passcode[1] = number % 10; drawAllDigits(); }
void digit3(uint8_t number) { passcode[2] = number % 10; drawAllDigits(); }
void digit4(uint8_t number) { passcode[3] = number % 10; drawAllDigits(); }

// ---------------------- resetPasscode ----------------------
// Zeros all digits, resets selected digit to 1, redraws boxes.
void resetPasscode(void) {
  passcode[0] = 0;
  passcode[1] = 0;
  passcode[2] = 0;
  passcode[3] = 0;
  currentDigit = 1;
  drawAllDigits();
}

// ---------------------- checkPasscode ----------------------
// Sends the 4-digit passcode to the backend via HTTP POST.
// Sets lockState based on the response.
void checkPasscode(void) {
  WiFiClient  client;
  HTTPClient  http;

  if (WiFi.status() == WL_CONNECTED) {
    // Use backend PC's IP — NOT localhost (localhost on ESP32 is the ESP itself)
    http.begin(client, "http://" BACKEND_IP ":8080/api/check/combination");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    char message[24];
    snprintf(message, sizeof(message),
             "passcode=%d%d%d%d",
             passcode[0], passcode[1], passcode[2], passcode[3]);

    Serial.print("[HTTP] Sending passcode: ");
    Serial.println(message);

    int httpResponseCode = http.POST(message);

    if (httpResponseCode > 0) {
      Serial.print("[HTTP] Response code: ");
      Serial.println(httpResponseCode);

      String received = http.getString();
      Serial.print("[HTTP] Response body: ");
      Serial.println(received);

      lockState = (received.indexOf("\"status\":\"complete\"") >= 0);
      showLockState();
    } else {
      Serial.print("[HTTP] POST failed, error: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("[HTTP] WiFi not connected.");
  }
}

// ---------------------- showLockState ----------------------
// Redraws the lock image and status text based on lockState.
// Only clears the lock + status region — leaves digits untouched.
void showLockState(void) {
  tft.fillRect(0, LOCK_Y,   240, 105, ILI9341_WHITE);
  tft.fillRect(0, STATUS_Y, 240, 28,  ILI9341_WHITE);

  if (lockState) {
    tft.drawRGBBitmap(68, LOCK_Y + 2, lockopen,  104, 97);
  } else {
    tft.drawRGBBitmap(68, LOCK_Y + 2, lockclose, 104, 103);
  }

  tft.setTextSize(2);
  if (lockState) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_WHITE);
    tft.setCursor(52, STATUS_Y + 5);
    tft.print("ACCESS OK");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_WHITE);
    tft.setCursor(25, STATUS_Y + 5);
    tft.print("ACCESS DENIED");
  }
}