// 
// Combined stepper motor + TCS34725 colour sensor + servo code
//
// Stepper runs continuously
// Colour sensor reads and prints RGBC values
// Servo on pin 4:
//   
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
const int cServoUpAngle      = 10;
const unsigned long cServoWaitTime = 9000;   // wait 9 seconds before moving
const unsigned long cServoUpTime   = 8000;    // stay at flat position for 8 seconds

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
int consecutiveGoodReadings  = 0;

// Smart LED object
Adafruit_NeoPixel SmartLEDs(cSmartLEDCount, cSmartLED, NEO_RGB + NEO_KHZ800);

// Smart LED brightness for heartbeat
unsigned char LEDBrightnessIndex = 0;
unsigned char LEDBrightnessLevels[] = {
  0, 0, 0, 5, 15, 30, 45, 60, 75, 90, 105, 120, 135,
  150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 5, 0
};

// TCS34725 colour sensor
// Increased Integration Time and Gain to give much higher resolution readings
Adafruit_TCS34725 tcs =
  Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_60X);

// Structure to hold minimum threshold RGBC values for colour identification
struct ColorThreshold {
  const char* name;
  uint16_t thresh_R;
  uint16_t thresh_G;
  uint16_t thresh_B;
  uint16_t thresh_C;
};

// Define all observed colours and orientations here. 
// Input your 4 calculated threshold values (R, G, B, C) for each.
ColorThreshold knownColors[] = {
  {"Black Base",    1300, 1100, 1200, 3000},
  {"Green Up",      2100, 2900, 1700, 6300},
  {"Green Down",    3600, 6000, 3300, 12000},
  {"L Green Up",    2800, 2600, 1800, 6600},
  {"L Green Down",  5100, 5300, 3300, 13000},
  {"Yellow Up",     3200, 2900, 1600, 7200},
  {"Yellow Down",   6800, 6300, 3100, 14000},
  {"Orange Up",     3500, 1500, 1300, 5800},
  {"Orange Down",   6800, 2300, 2100, 10000},
  {"Blue Up",       1500, 1600, 1500, 4100},
  {"Blue Down",     1100, 1900, 2200, 4800},
  {"Purple Up",     1600, 1400, 1300, 3700},
  {"Purple Down",   1600, 1400, 1400, 4000},
  {"Pink Up",       4300, 1600, 1700, 7000},
  {"Pink Down",     3800, 1600, 1700, 6500}
};

// Easily swap out the target "good" sample name here.
// Using a substring (like "Green") will match any colour containing that word 
// (e.g., "Green Up", "L Green Down").
const char* targetGoodColor = "Green";

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
    
    const char* detectedColorName = "Unknown";
    bool colourConditionMet = false;
    float bestDistance = 99999999.0;

    // Check reading against all known colours to find the BEST match.
    // We only consider colours where the read value exceeds the established thresholds.
    for (int i = 0; i < sizeof(knownColors) / sizeof(knownColors[0]); i++) {
      if (r >= knownColors[i].thresh_R &&
          g >= knownColors[i].thresh_G &&
          b >= knownColors[i].thresh_B &&
          c >= knownColors[i].thresh_C) {
          
        // Calculate squared distance to the threshold to find the "tightest" fit.
        // If a reading passes multiple thresholds, the one it is mathematically closest to wins!
        float dist = (r - knownColors[i].thresh_R)*(r - knownColors[i].thresh_R) + 
                     (g - knownColors[i].thresh_G)*(g - knownColors[i].thresh_G) + 
                     (b - knownColors[i].thresh_B)*(b - knownColors[i].thresh_B) + 
                     (c - knownColors[i].thresh_C)*(c - knownColors[i].thresh_C);
                     
        if (dist < bestDistance) {
          bestDistance = dist;
          detectedColorName = knownColors[i].name;
        }
      }
    }

    // If a valid colour was found, check if it contains the target string
    if (strcmp(detectedColorName, "Unknown") != 0 && strstr(detectedColorName, targetGoodColor) != NULL) {
      colourConditionMet = true;
    }

    if (colourConditionMet) {
      consecutiveGoodReadings++;
    } else {
      consecutiveGoodReadings = 0;
    }

    Serial.printf("R: %d, G: %d, B: %d, C: %d | Color: %s | Streak: %d\n", r, g, b, c, detectedColorName, consecutiveGoodReadings);

    // Start the 10-second wait only once when we have 5 straight good readings
    if (consecutiveGoodReadings >= 5 && !colourConditionLatched && !servoWaiting && !servoHoldingUp) {
      servoEventStartTime = millis();
      servoWaiting = true;
      colourConditionLatched = true;
    }

    // Reset latch after object/condition is gone and full cycle is finished
    if (consecutiveGoodReadings == 0 && !servoWaiting && !servoHoldingUp) {
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