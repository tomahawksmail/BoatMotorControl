#include <Wire.h>

// AD5171 I2C addresses
#define U6_ADDR 0x2C
#define U7_ADDR 0x2D

// Pins
const int buttonLeft = 2;
const int buttonRight = 4;

const int motor1_LPWM = 5;
const int motor1_RPWM = 3;
const int motor2_LPWM = 9;
const int motor2_RPWM = 6;

const int motor1Enable = 10;
const int motor2Enable = 11;

const int led1 = 8;
const int led2 = 7;
const int led3 = 12;

// Timings
const unsigned long motor1_time = 3000;  // t1
const unsigned long motor2_time = 5000;  // t2
const unsigned long wait_before_reverse = 1000;

// Hall Sensor
const int hallPin = A0;
const int hallThreshold = 700;

// LED blink
unsigned long led1_lastToggle = 0;
bool led1_state = false;

enum Direction { DIR_LEFT, DIR_RIGHT };
enum Step {
  STEP_IDLE,
  STEP_M1_FORWARD,
  STEP_M2,
  STEP_M1_BACK,
  STEP_WAIT_BEFORE_REVERSE,
  STEP_REVERSE_M2,
  STEP_REVERSE_M1
};

Step step = STEP_IDLE;
Direction direction;
unsigned long stepStart = 0;
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
}

void setDigipot(byte addr, byte value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

void updateDigipotFromHall() {
  int hallValue = analogRead(hallPin);
  if (hallValue > hallThreshold) {
    setDigipot(U6_ADDR, 255);
    setDigipot(U7_ADDR, 255);
  } else {
    setDigipot(U6_ADDR, 0);
    setDigipot(U7_ADDR, 0);
  }
}

void stopMotors() {
  analogWrite(motor1_LPWM, 0);
  analogWrite(motor1_RPWM, 0);
  analogWrite(motor2_LPWM, 0);
  analogWrite(motor2_RPWM, 0);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
}

void runMotor1(bool forward) {
  digitalWrite(led2, HIGH);
  analogWrite(motor1_LPWM, forward ? 200 : 0);
  analogWrite(motor1_RPWM, forward ? 0 : 200);
  analogWrite(motor2_LPWM, 0);
  analogWrite(motor2_RPWM, 0);
  digitalWrite(led3, LOW);
}

void runMotor2(bool forward) {
  digitalWrite(led3, HIGH);
  analogWrite(motor2_LPWM, forward ? 200 : 0);
  analogWrite(motor2_RPWM, forward ? 0 : 200);
  analogWrite(motor1_LPWM, 0);
  analogWrite(motor1_RPWM, 0);
  digitalWrite(led2, LOW);
}

void loop() {
  updateDigipotFromHall();

  // Blink LED1 every 1s
  if (millis() - led1_lastToggle >= 500) {
    led1_state = !led1_state;
    digitalWrite(led1, led1_state);
    led1_lastToggle = millis();
  }

  bool leftPressed = digitalRead(buttonLeft) == LOW;
  bool rightPressed = digitalRead(buttonRight) == LOW;
  bool isMoving = (step == STEP_M1_FORWARD || step == STEP_M2 || step == STEP_M1_BACK);

  // Emergency interrupt
  if (isMoving && (leftPressed || rightPressed) && !interrupted) {
    Serial.println("Interrupted! Stopping...");
    stopMotors();
    interrupted = true;
    step = STEP_WAIT_BEFORE_REVERSE;
    stepStart = millis();
    return;
  }

  // New command
  if (step == STEP_IDLE && (leftPressed || rightPressed)) {
    direction = leftPressed ? DIR_LEFT : DIR_RIGHT;
    step = STEP_M1_FORWARD;
    stepStart = millis();
    runMotor1(true);
    Serial.println(direction == DIR_LEFT ? "Left: M1 forward" : "Right: M1 forward");
  }

  // Step transitions
  unsigned long now = millis();
  switch (step) {
    case STEP_M1_FORWARD:
      if (now - stepStart >= motor1_time) {
        step = STEP_M2;
        stepStart = now;
        runMotor2(direction == DIR_LEFT);
        Serial.println("M2 running");
      }
      break;

    case STEP_M2:
      if (now - stepStart >= motor2_time) {
        step = STEP_M1_BACK;
        stepStart = now;
        runMotor1(false);
        Serial.println("M1 back");
      }
      break;

    case STEP_M1_BACK:
      if (now - stepStart >= motor1_time) {
        step = STEP_IDLE;
        stopMotors();
        interrupted = false;
        Serial.println("Done");
      }
      break;

    case STEP_WAIT_BEFORE_REVERSE:
      if (now - stepStart >= wait_before_reverse) {
        // Reverse order
        step = STEP_REVERSE_M2;
        stepStart = now;
        runMotor2(direction == DIR_LEFT ? false : true); // reverse direction
        Serial.println("Reverse M2");
      }
      break;

    case STEP_REVERSE_M2:
      if (now - stepStart >= motor2_time) {
        step = STEP_REVERSE_M1;
        stepStart = now;
        runMotor1(false); // always back
        Serial.println("Reverse M1");
      }
      break;

    case STEP_REVERSE_M1:
      if (now - stepStart >= motor1_time) {
        stopMotors();
        step = STEP_IDLE;
        interrupted = false;
        Serial.println("Returned to start");
      }
      break;

    default:
      break;
  }

  delay(10);
}
