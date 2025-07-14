#include <Wire.h>

// AD5171 I2C addresses
#define U6_ADDR 0x2C
#define U7_ADDR 0x2D

# check max via monitor
const int throttleMax = 818;

// Button pins
const int buttonLeft = 2;
const int buttonRight = 4;

// BTS7960 pins
const int motor1_LPWM = 5;
const int motor1_RPWM = 3;
const int motor2_LPWM = 9;
const int motor2_RPWM = 6;
const int motorEnable = 11;

// Run times
unsigned long motor1_time = 3000;
unsigned long motor2_time = 5000;

enum State {IDLE, RUNNING_LEFT, RUNNING_RIGHT, PAUSED};
State state = IDLE;

bool lastLeft = false;
bool lastRight = false;

// Which motor we're running now (1 or 2)
int currentMotor = 0;
unsigned long motorStartTime = 0;
unsigned long motorRemainingTime = 0;

// PWM values
const int pwmValue = 200;

void setup() {
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(motor1_LPWM, OUTPUT);
  pinMode(motor1_RPWM, OUTPUT);
  pinMode(motor2_LPWM, OUTPUT);
  pinMode(motor2_RPWM, OUTPUT);
  pinMode(motorEnable, OUTPUT);
  digitalWrite(motorEnable, HIGH);

  Wire.begin();
  Serial.begin(9600);
}

void setDigipot(byte addr, byte value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

void stopMotors() {
  analogWrite(motor1_LPWM, 0);
  analogWrite(motor1_RPWM, 0);
  analogWrite(motor2_LPWM, 0);
  analogWrite(motor2_RPWM, 0);
}

void loop() {
  int throttle = analogRead(A0);
  byte digipotValue = map(throttle, 0, throttleMax, 0, 255);
  setDigipot(U6_ADDR, digipotValue);
  setDigipot(U7_ADDR, digipotValue);

  bool leftNow = digitalRead(buttonLeft) == LOW;
  bool rightNow = digitalRead(buttonRight) == LOW;

  // detect rising edge (press → release)
  bool leftTapped = (!lastLeft && leftNow == false && digitalRead(buttonLeft) == HIGH);
  bool rightTapped = (!lastRight && rightNow == false && digitalRead(buttonRight) == HIGH);

  lastLeft = leftNow;
  lastRight = rightNow;

  // Handle taps
  if (leftTapped || rightTapped) {
    if (state == IDLE) {
      // start moving in tapped direction
      state = leftTapped ? RUNNING_LEFT : RUNNING_RIGHT;
      currentMotor = 1;
      motorRemainingTime = motor1_time;
      motorStartTime = millis();
      Serial.println("Started");
    }
    else if (state == RUNNING_LEFT || state == RUNNING_RIGHT) {
      // stop immediately
      stopMotors();
      motorRemainingTime -= millis() - motorStartTime;
      if (motorRemainingTime < 0) motorRemainingTime = 0;
      state = PAUSED;
      Serial.println("Paused");
    }
    else if (state == PAUSED) {
      // resume or reverse
      if ((leftTapped && state == PAUSED && lastDirection() == RUNNING_LEFT) ||
          (rightTapped && state == PAUSED && lastDirection() == RUNNING_RIGHT)) {
        // same direction: resume
        motorStartTime = millis();
        state = lastDirection();
        Serial.println("Resumed");
      } else {
        // other direction: restart from start
        state = leftTapped ? RUNNING_LEFT : RUNNING_RIGHT;
        currentMotor = 1;
        motorRemainingTime = motor1_time;
        motorStartTime = millis();
        Serial.println("Restarted other side");
      }
    }
  }

  // Run motors if needed
  if (state == RUNNING_LEFT || state == RUNNING_RIGHT) {
    unsigned long elapsed = millis() - motorStartTime;
    if (elapsed >= motorRemainingTime) {
      stopMotors();
      if (currentMotor == 1) {
        // move to motor2
        currentMotor = 2;
        motorRemainingTime = motor2_time;
        motorStartTime = millis();
      } else {
        // finished
        state = IDLE;
        Serial.println("Finished");
      }
    } else {
      // run current motor
      runCurrentMotor(state == RUNNING_LEFT, currentMotor);
    }
  }

  delay(10);
}

// helper: which direction we were running before pause
State lastDirection() {
  static State dir = RUNNING_LEFT;
  if (state == RUNNING_LEFT || state == RUNNING_RIGHT) dir = state;
  return dir;
}

void runCurrentMotor(bool leftDirection, int motorNumber) {
  if (motorNumber == 1) {
    if (leftDirection) {
      analogWrite(motor1_LPWM, pwmValue);
      analogWrite(motor1_RPWM, 0);
    } else {
      analogWrite(motor1_LPWM, 0);
      analogWrite(motor1_RPWM, pwmValue);
    }
    analogWrite(motor2_LPWM, 0);
    analogWrite(motor2_RPWM, 0);
  } else {
    if (leftDirection) {
      analogWrite(motor2_LPWM, pwmValue);
      analogWrite(motor2_RPWM, 0);
    } else {
      analogWrite(motor2_LPWM, 0);
      analogWrite(motor2_RPWM, pwmValue);
    }
    analogWrite(motor1_LPWM, 0);
    analogWrite(motor1_RPWM, 0);
  }
}
