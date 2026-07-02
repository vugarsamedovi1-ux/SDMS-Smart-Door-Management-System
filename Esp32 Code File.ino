#define BLYNK_TEMPLATE_ID "TMPL65dgZJ0Wu"
#define BLYNK_TEMPLATE_NAME "SDMS"
#define BLYNK_AUTH_TOKEN "RmLdaYTx3z17WE9GGXOLg25HlPpOOzU3"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Preferences.h>

char ssid[] = "";
char pass[] = "";

// Pin definitions
#define SERVO_PIN 14
#define REED_PIN 26
#define RED_LED 4
#define GREEN_LED 5
#define BUZZER_PIN 23
#define BUTTON_PIN 12

Servo lockServo;
BlynkTimer timer;
Preferences prefs;

// System variables
bool isDoorLocked = false;
bool lastReedState = false;
bool silentMode = false;
bool systemActive = true;
bool alarmActive = false;

int servoLockAngle = 0;
int servoUnlockAngle = 180;

unsigned long buttonPressTime = 0;
bool buttonHeld = false;

// ---- Persistent storage helpers ----
void saveLockState(bool locked) {
prefs.putBool("doorLocked", locked);
}

BLYNK_CONNECTED() {
Blynk.syncVirtual(V3);
Blynk.syncVirtual(V4);
Blynk.syncVirtual(V5);
Blynk.virtualWrite(V0, isDoorLocked ? 1 : 0);
}

void moveServo(int angle) {
lockServo.attach(SERVO_PIN, 400, 2600);
lockServo.write(angle);
delay(800);
lockServo.detach();
}

void systemBuzzer(int times) {
for (int i = 0; i < times; i++) {
digitalWrite(BUZZER_PIN, HIGH);
delay(300);
digitalWrite(BUZZER_PIN, LOW);
delay(200);
}
}

void startAlarm() {
alarmActive = true;
Serial.println("ALARM STARTED!");
}

void stopAlarm() {
alarmActive = false;
digitalWrite(BUZZER_PIN, LOW);
Serial.println("Alarm stopped");
}

void runAlarm() {
if (!alarmActive) return;
if (silentMode) return;

static unsigned long lastBeep = 0;
static bool buzzerState = false;
unsigned long now = millis();

if (now - lastBeep >= 100) {
lastBeep = now;
buzzerState = !buzzerState;
digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
}
}

void lockDoor() {
if (!systemActive) return;
moveServo(servoLockAngle);
isDoorLocked = true;
digitalWrite(RED_LED, HIGH);
digitalWrite(GREEN_LED, LOW);
Blynk.virtualWrite(V0, 1);
saveLockState(true);
}

void unlockDoor() {
if (!systemActive) return;
moveServo(servoUnlockAngle);
isDoorLocked = false;
digitalWrite(RED_LED, LOW);
digitalWrite(GREEN_LED, HIGH);
Blynk.virtualWrite(V0, 0);
saveLockState(false);
}

void restoreDoorState(bool locked, int angle) {
moveServo(angle);
isDoorLocked = locked;
digitalWrite(RED_LED, locked ? HIGH : LOW);
digitalWrite(GREEN_LED, locked ? LOW : HIGH);
}

BLYNK_WRITE(V0) {
if (!systemActive) return;
int val = param.asInt();
if (val == 1) lockDoor();
else unlockDoor();
}

BLYNK_WRITE(V2) {
if (!systemActive) return;
if (param.asInt() == 1) startAlarm();
else stopAlarm();
}

BLYNK_WRITE(V3) {
servoLockAngle = param.asInt();
if (isDoorLocked) moveServo(servoLockAngle);
}

BLYNK_WRITE(V4) {
servoUnlockAngle = param.asInt();
if (!isDoorLocked) moveServo(servoUnlockAngle);
}

BLYNK_WRITE(V5) {
silentMode = param.asInt();
if (silentMode) stopAlarm();
}

void checkDoorSensor() {
if (!systemActive) return;

bool reedState = digitalRead(REED_PIN);

if (reedState != lastReedState) {
Blynk.virtualWrite(V1, reedState ? 1 : 0);

if (isDoorLocked && reedState == HIGH) {
Blynk.logEvent("security_alert", "WARNING: Door forced open!");
startAlarm();
}

if (!isDoorLocked && reedState == HIGH) {
Blynk.logEvent("door_opened", "Door opened");
}

if (reedState == LOW) {
Blynk.logEvent("door_closed", "Door closed");
}

lastReedState = reedState;
}
}

void setup() {
Serial.begin(115200);

pinMode(RED_LED, OUTPUT);
pinMode(GREEN_LED, OUTPUT);
pinMode(BUZZER_PIN, OUTPUT);
pinMode(BUTTON_PIN, INPUT_PULLUP);
pinMode(REED_PIN, INPUT_PULLUP);

prefs.begin("sdms", false);
bool savedLocked = prefs.getBool("doorLocked", false);

Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

timer.setInterval(2000L, checkDoorSensor);

restoreDoorState(savedLocked, savedLocked ? servoLockAngle : servoUnlockAngle);

Serial.println("SDMS System Ready!");
}

void loop() {
Blynk.run();
timer.run();
}
