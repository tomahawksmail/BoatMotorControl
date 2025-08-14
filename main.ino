#include <Wire.h>

// I2C Digital Pot Addresses (dummy, adapt as needed)
#define U6_ADDR 0x2C
#define U7_ADDR 0x2D

// Input Buttons
const int buttonLeft = 2;
const int buttonRight = 4;

// Motor Pins
const int motor1_LPWM = 5;
const int motor1_RPWM = 3;
const int motor2_LPWM = 9;
const int motor2_RPWM = 6;
const int motor1Enable = 10;
const int motor2Enable = 11;

// LEDs
const int led1 = 12;  // StandBy
const int led2 = 8;  // RUN-1
const int led3 = 7; // RUN-2

// Hall sensor
const int hallPin = A0;
const int hallThreshold = 700;

// Timing
const unsigned long motor1_time = 1000;
const unsigned long motor2_time = 5000;
const unsigned long debounce = 150;

unsigned long stepStart = 0;
unsigned long lastButtonTime = 0;

bool led1_state = LOW;
unsigned long led1_lastToggle = 0;
int led1_phase = 0;

enum Direction { DIR_NONE, DIR_LEFT, DIR_RIGHT };
enum Step {
  STEP_IDLE,
  STEP_M1_LEFT,
  STEP_M2,
  STEP_M1_RIGHT,
  STEP_INTERRUPTED
};

Direction direction = DIR_NONE;
Direction lastCompletedDirection = DIR_NONE;
Step step = STEP_IDLE;
Step resumeStep = STEP_IDLE;
bool interrupted = false;

void setup() {
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);

  pinMode(motor1_LPWM, OUTPUT);
  pinMode(motor1_RPWM, OUTPUT);
  pinMode(motor2_LPWM, OUTPUT);
  pinMode(motor2_RPWM, OUTPUT);
  pinMode(motor1Enable, OUTPUT);
  pinMode(motor2Enable, OUTPUT);
  digitalWrite(motor1Enable, HIGH);
  digitalWrite(motor2Enable, HIGH);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  Wire.begin();
  Serial.begin(9600);
  stopMotors();
}

void stopMotors() {
  analogWrite(motor1_LPWM, 0);
  analogWrite(motor1_RPWM, 0);
  analogWrite(motor2_LPWM, 0);
  analogWrite(motor2_RPWM, 0);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
}

void runMotor1(bool left) {
  digitalWrite(led2, HIGH);
  analogWrite(motor1_LPWM, left ? 200 : 0);
  analogWrite(motor1_RPWM, left ? 0 : 200);
}

void runMotor2(bool left) {
  digitalWrite(led3, HIGH);
  analogWrite(motor2_LPWM, left ? 200 : 0);
  analogWrite(motor2_RPWM, left ? 0 : 200);
}

void updateDigipotFromHall() {
  int hallValue = analogRead(hallPin);
  byte value = (hallValue > hallThreshold) ? 255 : 0;
  Wire.beginTransmission(U6_ADDR);
  Wire.write(value);
  Wire.endTransmission();
  Wire.beginTransmission(U7_ADDR);
  Wire.write(value);
  Wire.endTransmission();
}

void loop() {
  unsigned long now = millis();

  updateDigipotFromHall();

  // Heartbeat LED
  switch (led1_phase) {
    case 0: // LED ON (first blink)
      if (now - led1_lastToggle >= 100) {
        digitalWrite(led1, LOW);
        led1_phase = 1;
        led1_lastToggle = now;
      } else {
        digitalWrite(led1, HIGH);
      }
      break;

    case 1: // short OFF gap
      if (now - led1_lastToggle >= 250) {
        led1_phase = 2;
        led1_lastToggle = now;
      }
      break;

    case 2: // LED ON (second blink)
      if (now - led1_lastToggle >= 100) {
        digitalWrite(led1, LOW);
        led1_phase = 3;
        led1_lastToggle = now;
      } else {
        digitalWrite(led1, HIGH);
      }
      break;

    case 3: // long OFF pause
      if (now - led1_lastToggle >= 3000) {
        led1_phase = 0;  // restart sequence
        led1_lastToggle = now;
      }
      break;
  }

  // Read buttons
  bool leftNow = digitalRead(buttonLeft) == LOW;
  bool rightNow = digitalRead(buttonRight) == LOW;
  static bool leftPrev = false;
  static bool rightPrev = false;
  bool leftPressed = leftNow && !leftPrev;
  bool rightPressed = rightNow && !rightPrev;
  leftPrev = leftNow;
  rightPrev = rightNow;



  // Emergency Stop if already running
  if (step != STEP_IDLE && step != STEP_INTERRUPTED && (leftPressed || rightPressed)) {
    if (now - lastButtonTime > 500) {
      Serial.println("== EMERGENCY STOP ==");
      interrupted = true;
      resumeStep = step;
      stopMotors();
      step = STEP_INTERRUPTED;
      lastButtonTime = now;
    }
    return;
  }

  // Resume or start new sequence
  if ((step == STEP_IDLE || step == STEP_INTERRUPTED) && (leftPressed || rightPressed)) {
    if (now - lastButtonTime > 500) {
      Direction newDir = leftPressed ? DIR_LEFT : DIR_RIGHT;

      // If completed already and not interrupted → do nothing
      if (!interrupted && step == STEP_IDLE && newDir == lastCompletedDirection) {
        return;
      }

      direction = newDir;
      digitalWrite(led2, LOW);
      digitalWrite(led3, LOW);
      stepStart = now;

      if (interrupted && direction == direction) {
        Serial.println("== RESUMING MOVEMENT ==");
        step = resumeStep;
      } else {
        Serial.print("== NEW MOVEMENT: ");
        Serial.println(direction == DIR_LEFT ? "LEFT" : "RIGHT");
        step = STEP_M1_LEFT;
      }

      interrupted = false;
      lastButtonTime = now;
    }
    return;
  }

  // Step sequencing
  switch (step) {
    case STEP_M1_LEFT:
      runMotor1(true);
      if (now - stepStart >= motor1_time) {
        step = STEP_M2;
        stepStart = now;
        stopMotors();
        Serial.println("STEP: M1 Left done → M2 Start");
      }
      break;

    case STEP_M2:
      runMotor2(direction == DIR_LEFT);
      if (now - stepStart >= motor2_time) {
        step = STEP_M1_RIGHT;
        stepStart = now;
        stopMotors();
        Serial.println("STEP: M2 done → M1 Right");
      }
      break;

    case STEP_M1_RIGHT:
      runMotor1(false);
      if (now - stepStart >= motor1_time) {
        step = STEP_IDLE;
        lastCompletedDirection = direction;
        stopMotors();
        Serial.println("STEP: M1 Right done → Finished");
      }
      break;

    case STEP_INTERRUPTED:
      // Wait for new button press
      break;

    default:
      break;
  }

  delay(5);
}
