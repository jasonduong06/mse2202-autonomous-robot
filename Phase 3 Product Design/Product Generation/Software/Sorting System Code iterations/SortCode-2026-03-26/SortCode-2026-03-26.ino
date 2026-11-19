//
// Combined stepper motor + TCS34725 colour sensor + servo code
//
// Stepper runs continuously
// Colour sensor reads and prints RGBC values
// Servo on pin 4:
//   - when R < 12, G > 15, C > 40:
//       wait 10 seconds
//       go to 0 degrees
//       stay there for 8 seconds
//       return to 90 degrees
//

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include "Adafruit_TCS34725.h"
#include <ESP32Servo.h>

// Function declarations
void doHeartbeat();
void ARDUINO_ISR_ATTR timerISR();

// Constants
const int cHeartbeatInterval = 75;
const int cSmartLED          = 23;
const int cSmartLEDCount     = 1;

// I2C pins
const int cSDA               = 21;
const int cSCL               = 22;

// Colour sensor pins
const int cTCSLED            = 14;
const int cLEDSwitch         = 34;

// Stepper pins
const int cStepPin           = 19;
const int cDirPin            = 18;

// Servo pin
const int cServoPin          = 4;

// Servo angles and timing
const int cServoDownAngle    = 150;
const int cServoUpAngle      = 5;
const unsigned long cServoWaitTime = 9000;   // wait 10 seconds before moving
const unsigned long cServoUpTime   = 6500;    // stay at 0 degrees for 8 seconds

// Variables
unsigned long lastHeartbeat  = 0;
unsigned long curMillis      = 0;
hw_timer_t * pTimer          = NULL;
boolean stepDir              = false;
volatile bool stepPinState   = false;
bool tcsFlag                 = false;

// Servo state machine
Servo myServo;
bool colourConditionLatched  = false;
bool servoWaiting            = false;
bool servoHoldingUp          = false;
unsigned long servoEventStartTime = 0;

// Smart LED object
Adafruit_NeoPixel SmartLEDs(cSmartLEDCount, cSmartLED, NEO_RGB + NEO_KHZ800);

// Smart LED brightness for heartbeat
unsigned char LEDBrightnessIndex = 0;
unsigned char LEDBrightnessLevels[] = {
  0, 0, 0, 5, 15, 30, 45, 60, 75, 90, 105, 120, 135,
  150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 5, 0
};

// TCS34725 colour sensor
Adafruit_TCS34725 tcs =
  Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);

void setup() {
  Serial.begin(115200);

  // Set up SmartLED
  SmartLEDs.begin();
  SmartLEDs.clear();
  SmartLEDs.setPixelColor(0, SmartLEDs.Color(0, 0, 0));
  SmartLEDs.setBrightness(0);
  SmartLEDs.show();

  // Set up colour sensor
  Wire.setPins(cSDA, cSCL);
  pinMode(cTCSLED, OUTPUT);
  pinMode(cLEDSwitch, INPUT_PULLUP);

  if (tcs.begin()) {
    Serial.println("Found TCS34725 colour sensor");
    tcsFlag = true;
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    tcsFlag = false;
  }

  // Set up stepper
  pinMode(cStepPin, OUTPUT);
  pinMode(cDirPin, OUTPUT);
  digitalWrite(cDirPin, stepDir);

  // Timer for continuous step pulses
  pTimer = timerBegin(1000000);
  timerAttachInterrupt(pTimer, &timerISR);
  timerAlarm(pTimer, 40000, true, 0);

  // Set up servo
  myServo.setPeriodHertz(50);
  myServo.attach(cServoPin);
  myServo.write(cServoDownAngle);   // start at 90 degrees
}

void loop() {
  uint16_t r, g, b, c;

  // Turn sensor LED on when switch is ON
  digitalWrite(cTCSLED, !digitalRead(cLEDSwitch));

  // Read and print colour values
  if (tcsFlag) {
    tcs.getRawData(&r, &g, &b, &c);
    Serial.printf("R: %d, G: %d, B: %d, C: %d\n", r, g, b, c);

    bool colourConditionMet = (r < 12) && (g > 15) && (c > 40);

    // Start the 10-second wait only once when condition first becomes true
    if (colourConditionMet && !colourConditionLatched && !servoWaiting && !servoHoldingUp) {
      servoEventStartTime = millis();
      servoWaiting = true;
      colourConditionLatched = true;
    }

    // Reset latch after object/condition is gone and full cycle is finished
    if (!colourConditionMet && !servoWaiting && !servoHoldingUp) {
      colourConditionLatched = false;
    }
  }

  // After waiting 10 seconds, move servo to 0 degrees
  if (servoWaiting && (millis() - servoEventStartTime >= cServoWaitTime)) {
    myServo.write(cServoUpAngle);
    servoEventStartTime = millis();
    servoWaiting = false;
    servoHoldingUp = true;
  }

  // After holding 8 seconds at 0 degrees, move back to 90 degrees
  if (servoHoldingUp && (millis() - servoEventStartTime >= cServoUpTime)) {
    myServo.write(cServoDownAngle);
    servoHoldingUp = false;
  }

  doHeartbeat();
  delay(100);
}

// Heartbeat LED update
void doHeartbeat() {
  curMillis = millis();

  if ((curMillis - lastHeartbeat) > cHeartbeatInterval) {
    lastHeartbeat = curMillis;
    LEDBrightnessIndex++;

    if (LEDBrightnessIndex >= sizeof(LEDBrightnessLevels)) {
      LEDBrightnessIndex = 0;
    }

    SmartLEDs.setBrightness(LEDBrightnessLevels[LEDBrightnessIndex]);
    SmartLEDs.setPixelColor(0, SmartLEDs.Color(0, 250, 0));
    SmartLEDs.show();
  }
}

// Timer ISR for continuous stepper pulses
void ARDUINO_ISR_ATTR timerISR() {
  stepPinState = !stepPinState;
  digitalWrite(cStepPin, stepPinState);
}