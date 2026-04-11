#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;   // Create Bluetooth serial instance


// RFID Pin configuration for ESP32
#define SS_PIN   25   // SDA / SS
#define MOSI_PIN 26
#define SCK_PIN  27
#define MISO_PIN 14
#define RST_PIN  12 

// LDR + Servo
#define LDR_PIN   16   // LDR ansz alog pin
#define SERVO_PIN  2   // Servo control pin

// Table color buttons
#define TABLE_BTN_PURPLE   18
#define TABLE_BTN_BLUE   19
#define TABLE_BTN_PINK    21
#define TABLE_BTN_ORANGE  5

// Flavor buttons
#define FLAVOR_BTN_FANTA  22
#define FLAVOR_BTN_PEPSI  23

// LEDs
#define LED1 4
#define LED2 13
#define LED3 17
#define LED4 15

// Signal pin to Arduino Nano
#define SIGNAL_PIN 32   // <-- Add a jumper from ESP32 pin 32 → Arduino Nano D7

MFRC522 mfrc522(SS_PIN, RST_PIN);  // RFID instance
Servo myServo;

// Allowed RFID UIDs
byte allowedUIDs[][4] = {
  {0x23, 0x43, 0x3A, 0x03},
  {0x82, 0xB6, 0x52, 0x3C},
  {0xF3, 0x10, 0x99, 0x03},
  {0x8A, 0xCA, 0x8E, 0x19},
  {0x43, 0x94, 0xC5, 0x03}
};
const int allowedCount = 5;

// LED control
int ledPins[] = {LED1, LED2, LED3, LED4};
int ledIndex = 0;

// LDR threshold (tune this)
int threshold = 2000;

void setup() {
  Serial.begin(115200);

  // SPI + RFID setup
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();

  // Servo setup
  myServo.attach(SERVO_PIN);
  myServo.write(90); // Default mid position

  SerialBT.begin("ESP32_TableUnit",true);  // ESP32 Bluetooth name
  SerialBT.connect("HC-05");

  Serial.println("Bluetooth Started: Pair with 'ESP32_TableUnit'");


  // Buttons setup
  pinMode(TABLE_BTN_PURPLE, INPUT_PULLUP);
  pinMode(TABLE_BTN_BLUE, INPUT_PULLUP);
  pinMode(TABLE_BTN_ORANGE, INPUT_PULLUP);
  pinMode(TABLE_BTN_PINK, INPUT_PULLUP);
  pinMode(FLAVOR_BTN_FANTA, INPUT_PULLUP);
  pinMode(FLAVOR_BTN_PEPSI, INPUT_PULLUP);

  // LEDs setup (all ON initially)
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }
    // Signal pin setup
  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW);  // default LOW
  Serial.println("System Ready: RFID + LDR + Servo + Buttons + LEDs");
}

void loop() {
  // Wait for new card
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Print UID
  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Check authorization
  bool authorized = false;
  for (int i = 0; i < allowedCount; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (mfrc522.uid.uidByte[j] != allowedUIDs[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      authorized = true;
      break;
    }
  }

  if (authorized) {
    Serial.println("Access Granted");

    // --- LDR + Servo Section ---
    int ldrValue = analogRead(LDR_PIN);
    Serial.print("LDR Value: ");
    Serial.println(ldrValue);

    if (ldrValue < threshold) { // light interrupted → card falls
      Serial.println("Light interrupted! Moving servo to 180°...");
      myServo.write(60);
      delay(2000);              // Hold at 180° for 2 seconds
      myServo.write(90);        // Return to default
      Serial.println("Servo returned to 90°");
    } else {
      Serial.println("No card detected by LDR, servo stays at 90°");
      myServo.write(90);
    }

    // --- Table Color Selection ---
    Serial.println("Waiting for table color button...");
    bool selected = false;
    while (!selected) {
      if (digitalRead(TABLE_BTN_PURPLE) == LOW) {
        Serial.println("Table Color: PURPLE");
        SerialBT.println("PURPLE"); // 🔹 send via Bluetooth
        selected = true;
      } else if (digitalRead(TABLE_BTN_BLUE) == LOW) {
        Serial.println("Table Color: BLUE");
        SerialBT.println("BLUE");    // 🔹 send via Bluetooth
        selected = true;
      } else if (digitalRead(TABLE_BTN_PINK) == LOW) {
        Serial.println("Table Color: PINK");
        SerialBT.println("PINK");     // 🔹 send via Bluetooth
        selected = true;
      } else if (digitalRead(TABLE_BTN_ORANGE) == LOW) {
        Serial.println("Table Color: ORANGE");
        SerialBT.println("ORANGE");   // 🔹 send via Bluetooth
        selected = true;
      }
    }
    delay(500);

    // --- Flavor Selection ---
    selected = false;
    Serial.println("Waiting for flavor button...");
    while (!selected) {
      if (digitalRead(FLAVOR_BTN_FANTA) == LOW) {
        Serial.println("Flavor Selected: FANTA");
        selected = true;


        // --- Send Signal to Arduino Nano ---
        digitalWrite(SIGNAL_PIN, HIGH);
        Serial.println("Signal sent to Nano (HIGH)");
        delay(1000);  // 1 second pulse
        digitalWrite(SIGNAL_PIN, LOW);
        Serial.println("Signal reset to LOW");


      } else if (digitalRead(FLAVOR_BTN_PEPSI) == LOW) {
        Serial.println("Flavor Selected: PEPSI");
        selected = true;

        // Turn OFF LEDs sequentially
        if (ledIndex < 4) {
          digitalWrite(ledPins[ledIndex], LOW);
          Serial.print("LED turned OFF: ");
          Serial.println(ledIndex + 1);
          ledIndex++;
         
        } else {
          Serial.println("All LEDs already OFF./ Flavour is empty ");
        }
         // *** SEND FLAVOUR 2 TO ARDUINO VIA BLUETOOTH ***
       SerialBT.println("FLAVOUR2");
       Serial.println("Bluetooth: Sent FLAVOUR2");
      }
    }

    delay(500);
    Serial.println("Selection completed.\n");

  } else {
    Serial.println("Access Denied");
  }

  mfrc522.PICC_HaltA(); // Stop reading card
}
