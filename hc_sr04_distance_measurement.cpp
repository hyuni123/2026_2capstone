#include <Arduino.h>

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

void setup()
{
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    digitalWrite(TRIG_PIN, LOW);

    Serial.println("HC-SR04 단독 진단 시작");
}

void loop()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration =
        pulseIn(ECHO_PIN, HIGH, 60000);

    Serial.print("ECHO 시간: ");
    Serial.print(duration);
    Serial.print(" us");

    if (duration == 0)
    {
        Serial.println(" → 신호 없음");
    }
    else
    {
        float distanceCm =
            duration * 0.0343 / 2.0;

        Serial.print(" / 거리: ");
        Serial.print(distanceCm, 2);
        Serial.println(" cm");
    }

    delay(500);
}
