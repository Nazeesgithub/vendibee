/*
 

    Receives table color from ESP32 via Bluetooth:
       "PURPLE", "BLUE", "PINK", "ORANGE"

    Moves only AFTER:
        1) Bottle detected via side ultrasonic
        2) Bluetooth command received

    Goes to table color → stop 2s → continue forward

    Detect GREEN → home reached → stop forever

    Obstacle handling: wait until obstacle moves
*/

// ================= LIBRARIES ======================
#include <Servo.h>
#include <NewPing.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TCS34725.h>
#include <SoftwareSerial.h>

// ================= BLUETOOTH ======================
SoftwareSerial BT(13, 12);   // HC05 (TX, RX)

// ================= PIN DEFINITIONS =================
#define in1 2
#define in2 3
#define in3 4
#define in4 7
#define enA 5
#define enB 6

#define FRONT_TRIGGER_PIN 8
#define FRONT_ECHO_PIN 9

#define SIDE_TRIGGER_PIN 10
#define SIDE_ECHO_PIN 12

#define SERVO_PIN 11
#define LEFT_SENSOR_PIN A1
#define RIGHT_SENSOR_PIN A0

// ================= CONSTANTS ======================
#define MAX_DISTANCE 200
#define OBSTACLE_DISTANCE 20
#define BOTTLE_DISTANCE 12   // Adjust as needed

// ================= MOTOR SPEED ====================
int BaseSpeed = 170;
int TurnSpeed = 150;

// ================= OBJECTS ========================
Servo myServo;
NewPing frontSonar(FRONT_TRIGGER_PIN, FRONT_ECHO_PIN, MAX_DISTANCE);
NewPing sideSonar(SIDE_TRIGGER_PIN, SIDE_ECHO_PIN, MAX_DISTANCE);

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// ================= FLAGS ==========================
bool homeReached = false;
bool bottleDetected = false;

String targetTable = "";
bool commandReceived = false;


// ================= SETUP ==========================
void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  pinMode(enA, OUTPUT); pinMode(enB, OUTPUT);

  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(92);

  if (!tcs.begin()) {
    Serial.println("Color sensor error");
    while (1);
  }

  Serial.println("Waiting Bluetooth table color...");
}


// ==================================================
// MAIN LOOP
// ==================================================
void loop() {

  // ----- WAIT FOR BLUETOOTH COMMAND -----
  if (!commandReceived) {
    if (BT.available()) {
      targetTable = BT.readStringUntil('\n');
      targetTable.trim();

      Serial.print("Table selected: ");
      Serial.println(targetTable);

      commandReceived = true;
      delay(300);
    }
    return;
  }

  // ----- HOME ALREADY REACHED -----
  if (homeReached) {
    Stop();
    Serial.println("HOME reached. Robot stopped.");
    while (1);
  }

  // ----- OBSTACLE CHECK -----
  int frontDistance = frontSonar.ping_cm();
  if (frontDistance > 0 && frontDistance < OBSTACLE_DISTANCE) {
    Stop();
    Serial.println("Obstacle detected  waiting...");

    while (true) {
      int d = frontSonar.ping_cm();
      if (d == 0 || d > OBSTACLE_DISTANCE) break;
      delay(100);
    }

    Serial.println("Obstacle cleared. Continuing...");
  }

  // ----- BOTTLE DETECTION -----
  int sideDistance = sideSonar.ping_cm();
  if (sideDistance > 0 && sideDistance < BOTTLE_DISTANCE) {
    bottleDetected = true;
  }

  // Robot moves only if bottle was detected
  if (!bottleDetected) {
    Stop();
    return;
  }

  // ----- COLOR DETECTION -----
  detectAndActOnColor();

  // ----- FOLLOW LINE -----
  followLine();
}


// ==================================================
// LINE FOLLOWING
// ==================================================
void followLine() {
  int left = !digitalRead(LEFT_SENSOR_PIN);
  int right = !digitalRead(RIGHT_SENSOR_PIN);

  if (left && right) forward();
  else if (left && !right) leftTurn();
  else if (!left && right) rightTurn();
  else Stop();
}


// ==================================================
// MOTOR CONTROL
// ==================================================
void forward() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  analogWrite(enA, BaseSpeed);
  analogWrite(enB, BaseSpeed);
}

void leftTurn() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);  digitalWrite(in4, HIGH);
}

void rightTurn() {
  digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
}

void Stop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}


// ==================================================
// COLOR CONVERSION
// ==================================================
void rgbToHsv(float r, float g, float b, float &h, float &s, float &v) {
  r /= 255.0; g /= 255.0; b /= 255.0;
  float maxVal = max(r, max(g, b));
  float minVal = min(r, min(g, b));
  v = maxVal;

  float d = maxVal - minVal;
  s = maxVal ? d / maxVal : 0;

  if (!d) h = 0;
  else {
    if (maxVal == r) h = (g - b) / d + (g < b ? 6 : 0);
    else if (maxVal == g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h *= 60;
  }
}


// ==================================================
// COLOR DETECT & ACTION
// ==================================================
void detectAndActOnColor() {

  float r, g, b;
  tcs.getRGB(&r, &g, &b);

  float h, s, v;
  rgbToHsv(r, g, b, h, s, v);

  String detected = "";

  // ----- COLOR CLASSIFICATION -----
  if (h >= 260 && h <= 300) detected = "PURPLE";
  else if (h < 45) detected = "ORANGE";
  else if (h >= 45 && h < 120) detected = "GREEN";
  else if (h >= 120 && h < 210) detected = "BLUE";
  else if (h >= 300 && h <= 345) detected = "PINK";

  Serial.print("Detected: ");
  Serial.println(detected);

  // ----- TABLE COLOR REACHED -----
  if (detected == targetTable) {
    Stop();
    Serial.println("TABLE REACHED → stopping 2 seconds");
    delay(2000);
  }

  // ----- HOME COLOR DETECTED -----
  if (detected == "GREEN") {
    Stop();
    homeReached = true;
  }
}



