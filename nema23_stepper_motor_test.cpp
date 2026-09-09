#include <Arduino.h>

const int STEP_PIN = 26;  // PUL+
const int DIR_PIN  = 27;  // DIR+

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, HIGH);

  delay(1000);
}

void loop() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(5000);

  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(5000);
}
