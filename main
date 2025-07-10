#include <EEPROM.h>

// Pin config
const int switchLeftPin = 2;
const int switchRightPin = 4;

const int motor1RPWM = 3;
const int motor1LPWM = 5;
const int motor1EN = 10;

const int motor2RPWM = 6;
const int motor2LPWM = 9;
const int motor2EN = 11;

// LED pins
const int ledLeft = A0;
const int ledRight = A1;
const int ledError = A2;

enum State { IDLE, M1_MOVING, M2_MOVING };
State state = IDLE;

unsigned long motionStart = 0;
const unsigned long motionDuration = 500; // 0.5s

bool directionLeft = true;

void setup() {
  pinMode(switchLeftPin, INPUT_PULLUP);
  pinMode(switchRightPin, INPUT_PULLUP);

  pinMode(motor1RPWM, OUTPUT);
  pinMode(motor1LPWM, OUTPUT);
  pinMode(motor1EN, OUTPUT);
  pinMode(motor2RPWM, OUTPUT);
  pinMode(motor2LPWM, OUTPUT);
  pinMode(motor2EN, OUTPUT);

  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
  pinMode(ledError, OUTPUT);

  // Load last direction from EEPROM
  byte saved = EEPROM.read(0);
  directionLeft = (saved == 0);

  updateLEDs();
}

void loop() {
  bool leftPressed = digitalRead(switchLeftPin) == LOW;
  bool rightPressed = digitalRead(switchRightPin) == LOW;
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (leftPressed) {
        directionLeft = true;
        EEPROM.update(0, 0);  // Save direction
        updateLEDs();
        startMotor(1);
        state = M1_MOVING;
        motionStart = now;
      } else if (rightPressed) {
        directionLeft = false;
        EEPROM.update(0, 1);  // Save direction
        updateLEDs();
        startMotor(1);
        state = M1_MOVING;
        motionStart = now;
      }
      break;

    case M1_MOVING:
      if (switchChangedDirection(leftPressed, rightPressed)) {
        stopAllMotors();
        blinkError();
        state = IDLE;
        break;
      }
      if (handleAcceleration(motor1RPWM, motor1LPWM, now - motionStart)) {
        startMotor(2);
        state = M2_MOVING;
        motionStart = now;
      }
      break;

    case M2_MOVING:
      if (switchChangedDirection(leftPressed, rightPressed)) {
        stopAllMotors();
        blinkError();
        state = IDLE;
        break;
      }
      if (handleAcceleration(motor2RPWM, motor2LPWM, now - motionStart)) {
        stopAllMotors();
        state = IDLE;
      }
      break;
  }
}

void startMotor(int motorNum) {
  if (motorNum == 1) {
    digitalWrite(motor1EN, HIGH);
    analogWrite(motor1RPWM, 0);
    analogWrite(motor1LPWM, 0);
  } else {
    digitalWrite(motor2EN, HIGH);
    analogWrite(motor2RPWM, 0);
    analogWrite(motor2LPWM, 0);
  }
}

bool handleAcceleration(int rPin, int lPin, unsigned long elapsed) {
  if (elapsed >= motionDuration) return true;

  int pwmVal;
  float progress = elapsed / (float)motionDuration;

  if (progress <= 0.5) {
    pwmVal = map(progress * 200, 0, 100, 0, 255);
  } else {
    pwmVal = map((1 - progress) * 200, 0, 100, 0, 255);
  }

  if (directionLeft) {
    analogWrite(lPin, pwmVal);
    analogWrite(rPin, 0);
  } else {
    analogWrite(rPin, pwmVal);
    analogWrite(lPin, 0);
  }

  return false;
}

void stopAllMotors() {
  analogWrite(motor1RPWM, 0);
  analogWrite(motor1LPWM, 0);
  digitalWrite(motor1EN, LOW);

  analogWrite(motor2RPWM, 0);
  analogWrite(motor2LPWM, 0);
  digitalWrite(motor2EN, LOW);
}

bool switchChangedDirection(bool left, bool right) {
  return (directionLeft && right) || (!directionLeft && left);
}

void updateLEDs() {
  digitalWrite(ledLeft, directionLeft ? HIGH : LOW);
  digitalWrite(ledRight, directionLeft ? LOW : HIGH);
  digitalWrite(ledError, LOW);
}

void blinkError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledError, HIGH);
    delay(150);
    digitalWrite(ledError, LOW);
    delay(150);
  }
}
