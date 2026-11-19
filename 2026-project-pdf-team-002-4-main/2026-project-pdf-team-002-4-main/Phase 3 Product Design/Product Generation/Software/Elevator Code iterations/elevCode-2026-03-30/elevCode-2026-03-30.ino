#include <Arduino.h>

// ---------------- MOTOR ----------------
const int motorIn1 = 26;
const int motorIn2 = 27;

const int motorIn3 = 16;
const int motorIn4 = 17;

const int pwmFreq = 20000;
const int pwmResolution = 8;
const int pwmSpeed = 200;

// timing variables
unsigned long motor2StartTime = 0;
bool motor2Pending = false;
bool motor2Running = false;

unsigned long runModeStartTime = 0;
bool runModeWaiting = false;

// ---------------- ULTRASONIC ----------------
const int cMaxEcho = 15000;
const int cTrigger = 11;

uint32_t cycles = 0;
uint32_t lastPulse = 0;

// stop motor if object is this close or closer
const int stopDistanceCm = 9;
boolean motorStopped = false;

// Modes
enum Mode {
  STOP_MODE = 1,
  RUN_MODE,
  REVERSE_MODE
};

Mode currentMode = STOP_MODE;

// ---------------- BUTTON ----------------
const int buttonPin = 0;
volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ---------------- ULTRASONIC ----------------
struct Ultrasonic {
  const int triggerPin;
  const int echoPin;
  volatile uint32_t pulseBegin = 0;
  volatile uint32_t pulseEnd = 0;
  volatile bool newEcho = false;
  volatile bool timeout = false;
  hw_timer_t* pTimer = NULL;
};

Ultrasonic ultrasonic = {21, 22};   // trig = 21, echo = 22

void initUltrasonic(Ultrasonic* us);
void ping(Ultrasonic* us);
uint32_t getEchoTime(Ultrasonic* us);
uint32_t usToCm(uint32_t us);
void ARDUINO_ISR_ATTR usTimerISR(void* arg);
void ARDUINO_ISR_ATTR echoISR(void* arg);

void stopMotors();
void runMotors();
void reverseMotors();
void vibrationEffect();

void setup() {
  Serial.begin(115200);

  ledcAttach(motorIn1, pwmFreq, pwmResolution);
  ledcAttach(motorIn2, pwmFreq, pwmResolution);
  ledcAttach(motorIn3, pwmFreq, pwmResolution);
  ledcAttach(motorIn4, pwmFreq, pwmResolution);

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);

  initUltrasonic(&ultrasonic);
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;
    currentMode = static_cast<Mode>((static_cast<int>(currentMode) % 3) + 1);
    Serial.printf("Switched to Mode %d\n", currentMode);

    if (currentMode == RUN_MODE) {
      runModeStartTime = millis();
      runModeWaiting = true;
    } else {
      runModeWaiting = false;
    }
  }

  switch (currentMode) {
    case STOP_MODE:
      stopMotors();
      break;

    case RUN_MODE:
      if (runModeWaiting) {
        if (millis() - runModeStartTime >= 20000) {
          runModeWaiting = false;
        } else {
          stopMotors();
          break;
        }
      }
      runMotors();
      break;

    case REVERSE_MODE:
      reverseMotors();
      break;

    default:
      stopMotors();
      break;
  }

  delay(1);
}

// ---------------- MOTOR CONTROL FUNCTIONS ----------------

void stopMotors() {
  ledcWrite(motorIn1, 0);
  ledcWrite(motorIn2, 0);
  ledcWrite(motorIn3, 0);
  ledcWrite(motorIn4, 0);

  motorStopped = false;
  motor2Pending = false;
  motor2Running = false;
}

void runMotors() {
  uint32_t pulseDuration = 0;

  if (cycles == 0) {
    ping(&ultrasonic);
  }

  cycles++;
  if (cycles == 100) {
    cycles = 0;
  }

  pulseDuration = getEchoTime(&ultrasonic);

  if (pulseDuration > 0) {
    lastPulse = pulseDuration;
    Serial.printf("Distance: %lu cm\n", usToCm(lastPulse));
  }

  if (lastPulse > 0 && usToCm(lastPulse) <= stopDistanceCm && !motorStopped) {
    ledcWrite(motorIn1, 0);
    ledcWrite(motorIn2, 0);

    motorStopped = true;
    motor2StartTime = millis();
    motor2Pending = true;
  } else {
    ledcWrite(motorIn1, pwmSpeed);
    ledcWrite(motorIn2, 0);
  }

  if (motorStopped) {
    vibrationEffect();
  }

  if (motor2Pending && millis() - motor2StartTime >= 5000) {
    ledcWrite(motorIn3, 225);
    ledcWrite(motorIn4, 0);
    motor2Pending = false;
    motor2Running = true;
  }
}

void reverseMotors() {
  ledcWrite(motorIn1, 0);
  ledcWrite(motorIn2, pwmSpeed);

  ledcWrite(motorIn3, 0);
  ledcWrite(motorIn4, 0);

  runModeWaiting = false;
}

void vibrationEffect() {
  for (int i = 0; i < 10; i++) {
    ledcWrite(motorIn1, 220);
    ledcWrite(motorIn2, 0);
    delay(80);

    ledcWrite(motorIn1, 0);
    ledcWrite(motorIn2, 210);
    delay(60);
  }
}

// ---------------- ULTRASONIC FUNCTIONS ----------------

void ping(Ultrasonic* us) {
  timerRestart(us->pTimer);
  timerAlarm(us->pTimer, cTrigger, false, 0);
  digitalWrite(us->triggerPin, HIGH);
  us->timeout = false;
}

void initUltrasonic(Ultrasonic* us) {
  pinMode(us->triggerPin, OUTPUT);
  pinMode(us->echoPin, INPUT);
  attachInterruptArg(us->echoPin, echoISR, us, CHANGE);
  us->pTimer = timerBegin(1000000);
  timerAttachInterruptArg(us->pTimer, usTimerISR, us);
}

uint32_t getEchoTime(Ultrasonic* us) {
  if (us->newEcho && !us->timeout) {
    us->newEcho = false;
    return us->pulseEnd - us->pulseBegin;
  } else {
    return 0;
  }
}

uint32_t usToCm(uint32_t us) {
  return (uint32_t)((double)us * 0.01724);
}

void ARDUINO_ISR_ATTR usTimerISR(void* arg) {
  Ultrasonic* us = static_cast<Ultrasonic*>(arg);

  if (timerRead(us->pTimer) < cTrigger + 10) {
    digitalWrite(us->triggerPin, LOW);
    timerRestart(us->pTimer);
    timerAlarm(us->pTimer, cMaxEcho, false, 0);
  } else {
    us->timeout = true;
  }
}

void ARDUINO_ISR_ATTR echoISR(void* arg) {
  Ultrasonic* us = static_cast<Ultrasonic*>(arg);

  if (digitalRead(us->echoPin)) {
    us->pulseBegin = micros();
  } else if (!us->timeout) {
    us->pulseEnd = micros();
    us->newEcho = true;
  }
}