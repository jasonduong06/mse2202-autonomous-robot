// test commit

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Function declarations
void doHeartbeat();
void setMotor(int dir, int pwm, int in1, int in2);
void ARDUINO_ISR_ATTR buttonISR(void* arg);
void ARDUINO_ISR_ATTR encoderISR(void* arg);

// Button structure
struct Button {
  const int pin;                                                       // GPIO pin for button
  volatile uint32_t numberPresses;                                     // tracker for number of button presses
  uint32_t nextPressTime;                                              // time of next allowable press
  volatile bool pressed;                                               // Tracker for button press
};

// Encoder structure
struct Encoder {
  const int PinA;                                                      // GPIO pin for encoder Channel 1
  const int PinB;                                                      // GPIO pin for encoder Channel 2
  volatile long pos;                                                   // current encoder position
};

// Function declarations
void doHeartbeat();
void setMotor(int dir, int pwm, int in1, int in2);
void ARDUINO_ISR_ATTR buttonISR(void* arg);
void ARDUINO_ISR_ATTR encoderISR(void* arg);

// Constants
const int cHeartbeatInterval = 75;                                     // heartbeat update interval time
const int cSmartLED          = 23;                                     // when DIP switch S1-4 is on the LED is connected to GPIO23
const int cSmartLEDCount     = 1;                                      // number of Smart LEDs in use
const long cDebounceDelay    = 170;                                    // switch debounce delay
const int cNumMotors         = 2;                                      // Number of DC motors that are used
const int cIN1Pin[]          = {26, 16};                               // GPIO pins for IN1 for left and right motors (1, 2)
const int cIN2Pin[]          = {27, 17};                               // GPIO pins for IN2 for left and right motors (1, 2)
const int cPWMRes            = 8;                                      // bit resolution for PWM
const int cMinPWM            = 150;                                    // PWM value for minimum speed that turns motor
const int cMaxPWM            = pow(2, cPWMRes) - 1;                    // PWM value for maximum speed
const int cPWMFreq           = 20000;                                  // frequency of PWM the signal
const int cPotPin            = 36;                                     // GPIO pin for drive speed potentiometer (A0)
const int cMotorEnablePin    = 39;                                     // GPIO pin for motor enable switch (DIP S1-1)

// Adjustments
const int cMotorAdjustment[] = {0, 0};                                 // PWM adjustment for motors to run closer to the same speed

// Variables
boolean motorsEnabled        = true;                                   // motors enabled flag
boolean timeUp3sec           = false;                                  // 3 second timer elapsed flag
boolean timeUp2sec           = false;                                  // 2 second timer elapsed flag
uint32_t lastHeartbeat       = 0;                                      // time of last heartbeat state change
uint32_t curMillis           = 0;                                      // current time, in milliseconds
uint32_t timerCount3sec      = 0;                                      // 3 second timer count in milliseconds
uint32_t timerCount2sec      = 0;                                      // 2 second timer count in milliseconds
uint32_t robotModeIndex      = 0;                                      // robot operational state                              
uint32_t lastTime            = 0;                                      // last time of motor control was updated
Button modeButton            = {0, 0, 0, false};                       // NO pushbutton PB1 on GPIO 0, low state when pressed
Encoder encoder[]            = {{35, 32, 0},                           // left encoder (A) on GPIO 35 and 32, 0 position 
                                {33, 25, 0}};                          // right encoder (B) on GPIO 33 and 25, 0 position
uint8_t driveSpeed           = 0;                                      // motor drive speed (0-255)
uint8_t driveIndex           = 0;                                      // state index for run mode
long leftPos                 = 0;                                      // varaible to track the position of the left motor
long rightPos                 = 0;                                     // varaible to track the position of the right motor
bool firstRun               = true;                                    // tracks if the robot has run the newest code yet


//LED setup
Adafruit_NeoPixel SmartLEDs(cSmartLEDCount, cSmartLED, NEO_RGB + NEO_KHZ800);

// the LED brightness for heartbeat
unsigned char LEDBrightnessIndex = 0; 
unsigned char LEDBrightnessLevels[] = {0, 0, 0, 5, 15, 30, 45, 60, 75, 90, 105, 120, 135, 
                                       150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 5, 0};

uint32_t modeIndicator[6]    = {                                       // colours for different modes
  SmartLEDs.Color(255, 0, 0),                                          //   red 
  SmartLEDs.Color(0, 255, 0),                                          //   green 
  SmartLEDs.Color(0, 0, 255),                                          //   blue 
  SmartLEDs.Color(255, 255, 0),                                        //   yellow 
  SmartLEDs.Color(0, 255, 255),                                        //   cyan 
  SmartLEDs.Color(255, 0, 255)                                         //   magenta 
};                                                                     

void setup() {
  Serial.begin(115200);                                                // Standard baud rate for ESP32 serial monitor
  // Set up the LED
  SmartLEDs.begin();                                                   // initialize LED
  SmartLEDs.clear();                                                   
  SmartLEDs.setPixelColor(0, SmartLEDs.Color(0,0,0));                  
  SmartLEDs.setBrightness(0);                                          // set brightness and colour
  SmartLEDs.show();                                                    // update the LED


  // setup motors with encoders
  for (int k = 0; k < cNumMotors; k++) {
    ledcAttach(cIN1Pin[k], cPWMFreq, cPWMRes);                         // set up INT1 GPIO PWM 
    ledcAttach(cIN2Pin[k], cPWMFreq, cPWMRes);                         // set up INT2 GPIO PWM
    pinMode(encoder[k].PinA, INPUT);                                   // set up GPIO for encoder Channel 1 input
    pinMode(encoder[k].PinB, INPUT);                                   // set up the GPIO for encoder Channel 2 input
    attachInterruptArg(encoder[k].PinA, encoderISR, &encoder[k], RISING); // set up the encoder to trigger the interrupt when rising on Channel 1
  }

  pinMode(modeButton.pin, INPUT_PULLUP);                               // set up GPIO for the push button
  attachInterruptArg(modeButton.pin, buttonISR, &modeButton, FALLING); // set up ISR to trigger on low
  
  pinMode(cPotPin, INPUT);                                             // set up drive speed pot
  pinMode(cMotorEnablePin, INPUT);                                     // set up motor enable switch
}

void loop() {
  long pos[] = {0, 0};                                                 // current motor positions
  int pot = 0;                                                         // value of the pot
         
  // store encoder position to avoid conflicts with ISR updates
  noInterrupts();                                                      // turn off the interrupts
  for (int k = 0; k < cNumMotors; k++) {
      pos[k] = encoder[k].pos;                                         // store the positions of the motors
  }
  interrupts();                                                        // turn the interrupts back on
 
  uint32_t curTime = micros();                                         // capture current time
  if (curTime - lastTime > 1000) {                                     
    lastTime = curTime;                                                // update the starting timer for the next cycle

    // 3 second timer count
    timerCount3sec += 1;                                               // increases timer count
    if (timerCount3sec > 3000) {                                       // if 3 seconds has happened then reset the timer count
      timerCount3sec = 0;                                              
      timeUp3sec = true;                                               
    }  
   
    // 2 second timer count
    timerCount2sec += 1;                                               // increases timer count
    if (timerCount2sec > 2000) {                                       // if 2 seconds has happened then reset the timer count
      timerCount2sec = 0;                                              
      timeUp2sec = true;                                               
    }

    if (modeButton.pressed) {                                          // change mode if the button is pressed
      robotModeIndex++;                                                // increase mode index to next mode
      robotModeIndex = robotModeIndex & 1;                             // set mode index between 0 and 3
      timerCount3sec = 0;                                              // reset 3 second timer count
      timeUp3sec = false;                                              
      modeButton.pressed = false;                                     
    }

 
    motorsEnabled = !digitalRead(cMotorEnablePin);                     // check if SW1-1 is on to enable motors


    // 0, first mode for when robot is stopped
    // 1, second mode to run robot
    switch (robotModeIndex) {
      case 0: // Robot stopped
        setMotor(0, 0, cIN1Pin[0], cIN2Pin[0]);                        // stop both motors
        setMotor(0, 0, cIN1Pin[1], cIN2Pin[1]);                        
        encoder[0].pos = 0;                                            // clear left and right encoder
        encoder[1].pos = 0;                                            
        driveIndex = 0;                                                // reset drive index
        firstRun = true;
        timerCount2sec = 0;                                            // reset 2 second timer count
        timeUp2sec = false;                                            
        break;

      case 1: // Run robot
        if (timeUp3sec && motorsEnabled) {                             // pause for 3 sec if motors are enabled
          pot = analogRead(cPotPin);                                   // update drive motor speed to the pot
          driveSpeed = map(pot, 0, 4095, cMinPWM, cMaxPWM);
          if (timeUp2sec) {                                            // update drive state after 2 seconds
            timerCount2sec = 0;                                        // reset 2 second timer count and tracker
            timeUp2sec = false;                                       
            switch(driveIndex) {                                       // cycle through all the drive states
              case 0: // Stop for two seconds
                  setMotor(0, 0, cIN1Pin[0], cIN2Pin[0]);              // stop both motors
                  setMotor(0, 0, cIN1Pin[1], cIN2Pin[1]);              
                  driveIndex = 1;                                      // next state: drive forward
                  break;

              case 1: // Drive forward quickly for 2 seconds
                  setMotor(1, driveSpeed + cMotorAdjustment[0], cIN1Pin[0], cIN2Pin[0]);                  // turn both motors in the oposite directions to move forwards
                  setMotor(-1, driveSpeed + cMotorAdjustment[1], cIN1Pin[1], cIN2Pin[1]);
                  driveIndex = 0;                                      // next state: turn
                  break;

              case 2: // Turn right for 2 seconds
                  setMotor(-1, driveSpeed*0.8 + cMotorAdjustment[0], cIN1Pin[0], cIN2Pin[0]);                  // reverse both motors to turn (drive speed adjusted for exactly 90 degrees)
                  setMotor(-1, driveSpeed*0.8 + cMotorAdjustment[1], cIN1Pin[1], cIN2Pin[1]);
                  driveIndex = 3;                                      // next state: forwards slowly
                  break;

              case 3: // Move forward slowly for 2 seconds
                  setMotor(1, driveSpeed*0.8 + cMotorAdjustment[0], cIN1Pin[0], cIN2Pin[0]);                  // turn both motors in the oposite directions to move forwards (drive speed adjusted for desired length)
                  setMotor(-1, driveSpeed*0.8 + cMotorAdjustment[1], cIN1Pin[1], cIN2Pin[1]);
                  driveIndex = 4;                                      // next state: turn
                  break;

              case 4: // Turn right for 2 seconds
                  setMotor(-1, driveSpeed*0.8 + cMotorAdjustment[0], cIN1Pin[0], cIN2Pin[0]);                  // reverse both motors to turn (drive speed adjusted for exactly 90 degrees)
                  setMotor(-1, driveSpeed*0.8 + cMotorAdjustment[1], cIN1Pin[1], cIN2Pin[1]);
                  driveIndex = 1;                                      // next state: forward quickly
                  break;
            }
          }
        }
    }
  }
  doHeartbeat();                                                       // update the LED
}

// update heartbeat LED
void doHeartbeat() {
  curMillis = millis();                                                // track the current time
   if ((curMillis - lastHeartbeat) > cHeartbeatInterval) {             // check to see if elapsed time matches the heartbeat interval
    lastHeartbeat = curMillis;                                         // update the heartbeat timer and change to the next brightness level
    LEDBrightnessIndex++;                                              
    if (LEDBrightnessIndex >= sizeof(LEDBrightnessLevels)) {           // if all defined levels have been used reset to original brightness
      LEDBrightnessIndex = 0;                                          
    }
    SmartLEDs.setBrightness(LEDBrightnessLevels[LEDBrightnessIndex]);  // set brightness and colours of the LED
    SmartLEDs.setPixelColor(0, modeIndicator[robotModeIndex]);         
    SmartLEDs.show();                                                  // update LED light
  }
}

// Set motor control signals based on given direction and pulse width
void setMotor(int dir, int pwm, int in1, int in2) {
  if (dir == 1) {                                                      // forward direction
    ledcWrite(in1, pwm);
    ledcWrite(in2, 0);
  }
  else if (dir == -1) {                                                // reverse direction
    ledcWrite(in1, 0);
    ledcWrite(in2, pwm);
  }
  else {                                                               // everything else is stopped
    ledcWrite(in1, 0);
    ledcWrite(in2, 0);
  }
}

// button interrupt service routine with static button structure to let multiple instances of the button ISR to be made
void ARDUINO_ISR_ATTR buttonISR(void* arg) {
  Button* s = static_cast<Button*>(arg);                               // Make pointer static
  uint32_t pressTime = millis();                                       // capture current time
  if (pressTime > s->nextPressTime) {                                  // if enough time has passed to consider a valid press increase press counter and change press state
    s->numberPresses += 1;                                            
    s->pressed = true;                                                 
    s->nextPressTime = pressTime + cDebounceDelay;                     // update time for next valid press
  }  
}

// Encoder inturrupt service routine with static encoder structure to let multiple instances of the encoder ISE to be made
void ARDUINO_ISR_ATTR encoderISR(void* arg) {
  Encoder* s = static_cast<Encoder*>(arg);                             // make pointer static
  int b = digitalRead(s->PinB);                                        // read the state of channel 2
  if (b > 0) {                                                         // high for leading channel 1
    s->pos++;                                                          // increase position
  }
  else {                                                               // low for lagging channel 1
    s->pos--;                                                          // decrease position
  }
}


