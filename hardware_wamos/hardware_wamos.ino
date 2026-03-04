#include <math.h>
#include <SoftwareSerial.h>

//**********ENTER IP ADDRESS OF SERVER******************//
#define HOST_IP     "10.77.247.87"
#define HOST_PORT   "8080"
#define route       "api/update"
#define idNumber    "620171712"

// WIFI CREDENTIALS
#define SSID        "Galaxy A15 4257"
#define password    "12345876"

#define stay        150   // increased from 100 to give ESP more time

//**********ESP SERIAL PINS******************//
#define espRX         10
#define espTX         11
#define espTimeout_ms 500  // increased from 300

SoftwareSerial esp(espRX, espTX);

//**********ULTRASONIC PINS******************//
#define TRIG_PIN 2
#define ECHO_PIN 3

//**********TANK DIMENSIONS******************//
const float SENSOR_HEIGHT_IN = 94.5;
const float MAX_WATER_IN     = 77.763;
const float TANK_DIAM_IN     = 61.5;
const float IN3_PER_GAL      = 231.0;

//**********FUNCTION DECLARATIONS******************//
float readUltrasonicInches();
float clampf(float x, float lo, float hi);
float computeWaterHeight(float radar_in);
float computeReserveGallons(float waterHeight_in);
float computePercentage(float waterHeight_in);

void espSend(const char* command);
void espUpdate(char mssg[]);
void espInit();

//**********SETUP******************//
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delay(100);
  espInit();
}

void loop() {
  float radar_in = readUltrasonicInches();

  // If sensor returns NAN (no echo / timeout), skip this cycle
  if (isnan(radar_in)) {
    Serial.println("Ultrasonic: no echo / timeout - skipping");
    delay(1000);
    return;
  }

  float waterHeight_in = computeWaterHeight(radar_in);
  float reserve_gal    = computeReserveGallons(waterHeight_in);
  float pct            = computePercentage(waterHeight_in);

  Serial.print("radar(in): ");        Serial.print(radar_in, 2);
  Serial.print(" | waterheight(in): "); Serial.print(waterHeight_in, 2);
  Serial.print(" | reserve(gal): ");  Serial.print(reserve_gal, 2);
  Serial.print(" | %: ");             Serial.println(pct, 1);

  // Build JSON — schema required by lab
  char radarStr[12], whStr[12], resStr[12], pctStr[12];
  dtostrf(radar_in,       1, 2, radarStr);
  dtostrf(waterHeight_in, 1, 2, whStr);
  dtostrf(reserve_gal,    1, 2, resStr);
  dtostrf(pct,            1, 1, pctStr);

  // message buffer — 220 bytes is enough for JSON payload alone
  char message[220];
  snprintf(message, sizeof(message),
    "{\"id\":\"%s\",\"type\":\"ultrasonic\",\"radar\":%s,\"waterheight\":%s,\"reserve\":%s,\"percentage\":%s}",
    idNumber, radarStr, whStr, resStr, pctStr);

  Serial.print("JSON: ");
  Serial.println(message);

  espUpdate(message);
  delay(1000);
}

//==================== ULTRASONIC HELPERS ====================//
float readUltrasonicInches() {
  // Ensure trigger is low before pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);

  // Send 10us trigger pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo — timeout at 25ms (covers max ~170 inches)
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);

  Serial.print("duration_us = ");
  Serial.println(duration);

  if (duration == 0) return NAN;  // timeout / no object detected

  // Speed of sound: 0.0343 cm/us, divide by 2 for round trip
  float distance_cm = (float)duration * 0.01715f;  // = * 0.0343 / 2
  float distance_in = distance_cm / 2.54f;

  Serial.print("distance_in = ");
  Serial.println(distance_in, 2);

  // Sanity check — sensor range ~0.8in to 157in (HC-SR04 spec)
  if (distance_in < 0.8f || distance_in > 157.0f) return NAN;

  return distance_in;
}

float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

float computeWaterHeight(float radar_in) {
  float wh = SENSOR_HEIGHT_IN - radar_in;
  return clampf(wh, 0.0f, MAX_WATER_IN);
}

float computeReserveGallons(float waterHeight_in) {
  float r      = TANK_DIAM_IN / 2.0f;
  float vol_in3 = (float)M_PI * r * r * waterHeight_in;
  float gal    = vol_in3 / IN3_PER_GAL;
  return clampf(gal, 0.0f, 1000.0f);
}

float computePercentage(float waterHeight_in) {
  float pct = (waterHeight_in / MAX_WATER_IN) * 100.0f;
  return clampf(pct, 0.0f, 100.0f);
}

//==================== ESP AT COMMAND FUNCTIONS ==============//
void espSend(const char* command) {
  esp.print(command);
  unsigned long start = millis();
  while (millis() - start < espTimeout_ms) {
    while (esp.available()) {
      Serial.write(esp.read());
      start = millis();
    }
  }
}

void espUpdate(char mssg[]) {
  int msgLen = strlen(mssg);

  // Build full HTTP POST string first so we know exact length
  // Buffer: method + route + HTTP ver + headers + body = ~350 bytes max
  char post[400] = {0};
  snprintf(post, sizeof(post),
    "POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",
    route, HOST_IP, msgLen, mssg);

  int postLen = strlen(post);

  Serial.print("POST length: ");
  Serial.println(postLen);
  Serial.println(post);

  // Open TCP connection
  char espCommandString[60] = {0};
  snprintf(espCommandString, sizeof(espCommandString),
    "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", HOST_IP, HOST_PORT);
  espSend(espCommandString);
  delay(stay);

  // Tell ESP how many bytes to expect
  snprintf(espCommandString, sizeof(espCommandString),
    "AT+CIPSEND=%d\r\n", postLen);
  espSend(espCommandString);
  delay(stay);

  // Send the full HTTP request
  espSend(post);
  delay(stay);

  // Close connection
  espSend("AT+CIPCLOSE\r\n");
  delay(stay);
}

void espInit() {
  char connection[100] = {0};
  esp.begin(115200);
  Serial.println("Initializing ESP...");

  espSend("AT\r\n");
  delay(500);
  espSend("AT+CWMODE=1\r\n");
  delay(500);

  while (esp.available()) { Serial.println(esp.readString()); }

  snprintf(connection, sizeof(connection),
    "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, password);
  esp.print(connection);
  delay(5000);  // give ESP enough time to join network

  if (esp.available()) { Serial.print(esp.readString()); }

  espSend("AT+CIFSR\r\n");  // print IP so we can confirm connection
  delay(500);

  Serial.println("ESP Initialized.");
}
