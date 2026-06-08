// ================= PWM OUTPUT =================
const int pwmOut = 6;

// ================= SWITCHES =================
const int pinForward = 2;
const int pinBackward = 4;

// ================= BTS7960 =================
const int motorLPWM = 5;
const int motorRPWM = 3;
const int motorEnable = 10;

// ================= CURRENT SENSE =================
const int currentLpin = A1;
const int currentRpin = A2;

// ================= HALL SENSOR =================
const int hallPin = A0;

// measured limits
const int hallMin = 180;
const int hallMax = 860;

// stop zones
const int hallUpLimit = 200;
const int hallDownLimit = 840;

// ================= LEDS =================
const int ledForward = 7;
const int ledBackward = 8;
const int ledHeartbeat = 11;
const int ledError = 12;

// ================= CURRENT LIMITS =================
int currentSoftLimit = 750;
int currentHardLimit = 900;

// ================= STATE =================
bool errorState = false;
bool ledState = false;

unsigned long lastBlink = 0;

int motionDir = 0;   // 1 forward, -1 backward, 0 stop

// ================= FILTER =================
float hallFiltered = hallMin;
const float hallAlpha = 0.20;

// ================= CURRENT FILTER =================
const int samples = 10;

// =====================================================
// CURRENT READ
// =====================================================
int readCurrent(int pin) {

  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }

  return sum / samples;
}

// =====================================================
// HALL FILTER
// =====================================================
int readHallFiltered() {

  int raw = analogRead(hallPin);

  hallFiltered =
    hallFiltered +
    hallAlpha * ((float)raw - hallFiltered);

  return (int)hallFiltered;
}

// =====================================================
// PWM OUTPUT
// =====================================================
void updatePWMOutput(int hallValue) {

  hallValue = constrain(hallValue, hallMin, hallMax);

  int pwm = map(hallValue, hallMin, hallMax, 0, 255);

  analogWrite(pwmOut, pwm);
}

// =====================================================
// MOTOR STOP
// =====================================================
void stopMotor() {

  analogWrite(motorLPWM, 0);
  analogWrite(motorRPWM, 0);

  digitalWrite(motorEnable, LOW);

  digitalWrite(ledForward, LOW);
  digitalWrite(ledBackward, LOW);

  motionDir = 0;
}

// =====================================================
// MOVE FORWARD
// =====================================================
void moveForward() {

  digitalWrite(motorEnable, HIGH);

  analogWrite(motorLPWM, 200);
  analogWrite(motorRPWM, 0);

  digitalWrite(ledForward, HIGH);
  digitalWrite(ledBackward, LOW);

  motionDir = 1;
}

// =====================================================
// MOVE BACKWARD
// =====================================================
void moveBackward() {

  digitalWrite(motorEnable, HIGH);

  analogWrite(motorLPWM, 0);
  analogWrite(motorRPWM, 200);

  digitalWrite(ledForward, LOW);
  digitalWrite(ledBackward, HIGH);

  motionDir = -1;
}

// =====================================================
// CURRENT FAULT
// =====================================================
bool currentFault() {

  int L_IS = readCurrent(currentLpin);
  int R_IS = readCurrent(currentRpin);

  int maxC = max(L_IS, R_IS);

  if (maxC > currentHardLimit)
    return true;

  if (maxC > currentSoftLimit) {

    if (motionDir == 1)
      analogWrite(motorLPWM, 120);

    if (motionDir == -1)
      analogWrite(motorRPWM, 120);
  }

  Serial.print("L=");
  Serial.print(L_IS);

  Serial.print(" R=");
  Serial.print(R_IS);

  Serial.print(" MAX=");
  Serial.println(maxC);

  return false;
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  pinMode(pinForward, INPUT_PULLUP);
  pinMode(pinBackward, INPUT_PULLUP);

  pinMode(motorLPWM, OUTPUT);
  pinMode(motorRPWM, OUTPUT);
  pinMode(motorEnable, OUTPUT);

  pinMode(ledForward, OUTPUT);
  pinMode(ledBackward, OUTPUT);
  pinMode(ledHeartbeat, OUTPUT);
  pinMode(ledError, OUTPUT);

  pinMode(pwmOut, OUTPUT);

  Serial.begin(115200);

  hallFiltered = analogRead(hallPin);

  stopMotor();
}

// =====================================================
// LOOP
// =====================================================


void loop() {

  unsigned long now = millis();

  // ================= HEARTBEAT =================
  const unsigned long onTime = 150;
  const unsigned long offTime = 2000;

  if (!errorState) {

    if (!ledState && (now - lastBlink >= offTime)) {
      ledState = true;
      digitalWrite(ledHeartbeat, HIGH);
      lastBlink = now;
    }
    else if (ledState && (now - lastBlink >= onTime)) {
      ledState = false;
      digitalWrite(ledHeartbeat, LOW);
      lastBlink = now;
    }
  }

  // ================= INPUTS =================
  bool forwardCmd = (digitalRead(pinForward) == LOW);
  bool backwardCmd = (digitalRead(pinBackward) == LOW);

  // DEBUG (VERY IMPORTANT)
  Serial.print("F=");
  Serial.print(forwardCmd);
  Serial.print(" B=");
  Serial.print(backwardCmd);
  Serial.print(" DIR=");
  Serial.println(motionDir);

  // ================= HALL =================
  int hallValue = readHallFiltered();
  updatePWMOutput(hallValue);

  // ================= SAFETY =================
  if (errorState) {
    stopMotor();
    digitalWrite(ledError, HIGH);
    analogWrite(pwmOut, 0);
    return;
  }

  digitalWrite(ledError, LOW);

  // ================= LIMIT STOP =================
  if (motionDir == 1 && hallValue <= hallUpLimit) {
    stopMotor();
  }

  if (motionDir == -1 && hallValue >= hallDownLimit) {
    stopMotor();
  }

  // ================= MOTOR CONTROL (FIXED) =================

  if (forwardCmd && backwardCmd) {
    stopMotor();
  }

  else if (forwardCmd) {

    digitalWrite(motorEnable, HIGH);   // FORCE ENABLE

    if (motionDir != 1) {
      moveForward();
    }
  }

  else if (backwardCmd) {

    digitalWrite(motorEnable, HIGH);   // FORCE ENABLE

    if (motionDir != -1) {
      moveBackward();
    }
  }

  else {
    stopMotor();
  }

  delay(5);
}
