#include <Arduino.h>

// ---------------- MOTOR ----------------
const int motorIn1 = 26;
const int motorIn2 = 27;

const int pwmFreq = 20000;
const int pwmResolution = 8;
const int pwmSpeed = 170;  // Adjust motor speed as needed

// ---------------- ULTRASONIC ----------------
const int cMaxEcho = 15000;   // max echo wait in microseconds
const int cTrigger = 11;      // trigger pulse length in microseconds

uint32_t cycles = 0;
uint32_t lastPulse = 0;

// stop motor if object is this close or closer
const int stopDistanceCm = 8;

// Modes
enum Mode {
  STOP_MODE = 1,
  RUN_MODE,
  REVERSE_MODE
};

Mode currentMode = STOP_MODE;  // Default to stop mode

// ---------------- BUTTON ----------------
const int buttonPin = 0;  // Button pin to switch modes
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

void setup() {
  Serial.begin(115200);

  // Set up motor PWM
  ledcAttach(motorIn1, pwmFreq, pwmResolution);
  ledcAttach(motorIn2, pwmFreq, pwmResolution);

  // Set up button and interrupt
  pinMode(buttonPin, INPUT_PULLUP);  // Button input with pullup
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);  // Button press ISR

  // Set up ultrasonic
  initUltrasonic(&ultrasonic);
}

void loop() {
  uint32_t pulseDuration = 0;

  // Check for button press to change modes
  if (buttonPressed) {
    buttonPressed = false;  // Reset the button press flag
    currentMode = static_cast<Mode>((static_cast<int>(currentMode) % 3) + 1);  // Cycle through 1, 2, 3
    Serial.printf("Switched to Mode %d\n", currentMode);
  }

  // Execute different behavior based on current mode
  switch (currentMode) {
    case STOP_MODE:
      stopMotors();  // Stop motors completely
      break;

    case RUN_MODE:
      runMotors();  // Run motor using ultrasonic logic
      break;

    case REVERSE_MODE:
      reverseMotors();  // Reverse motor direction
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
}

void runMotors() {
  uint32_t pulseDuration = 0;

  // start one ultrasonic measurement every ~100 ms
  if (cycles == 0) {
    ping(&ultrasonic);
  }

  cycles++;
  if (cycles == 100) {
    cycles = 0;
  }

  // check for new ultrasonic reading
  pulseDuration = getEchoTime(&ultrasonic);

  if (pulseDuration > 0) {
    lastPulse = pulseDuration;
    Serial.printf("Distance: %lu cm\n", usToCm(lastPulse));
  }

  // if object is close, stop motor
  if (lastPulse > 0 && usToCm(lastPulse) <= stopDistanceCm) {
    ledcWrite(motorIn1, 0);
    ledcWrite(motorIn2, 0);
  } 
  else {
    // otherwise spin motor at constant speed
    ledcWrite(motorIn1, pwmSpeed);
    ledcWrite(motorIn2, 0);
  }

}

void reverseMotors() {
  // Reverse the direction of the motor
  ledcWrite(motorIn1, 0);
  ledcWrite(motorIn2, pwmSpeed);  // Reverse motor direction
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