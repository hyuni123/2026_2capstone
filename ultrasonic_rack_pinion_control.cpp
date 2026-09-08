#include <Arduino.h>

// ==================================================
// HC-SR04 초음파센서 핀
// ==================================================

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

// ==================================================
// ULN2003 드라이버 핀
// ==================================================

const int IN1 = 13;
const int IN2 = 12;
const int IN3 = 14;
const int IN4 = 27;

// ==================================================
// 랙앤피니언 설정
// ==================================================

// 피니언:
// 톱니 수 16개
// 모듈 1.25
// 피치원 지름 20mm
//
// 풀스텝 구동 기준 약 326스텝/cm
const float STEPS_PER_CM = 326.0;

// 최대 다리 연장거리
const float MAX_DISTANCE_CM = 7.0;

// HC-SR04 권장 최소 측정거리
const float MIN_DISTANCE_CM = 2.0;

// 랙이 들어가는 방향으로 움직이면 false로 변경
const bool EXTEND_DIRECTION = true;

// 값이 클수록 천천히 회전
const int STEP_DELAY_US = 2000;

// ==================================================
// 28BYJ-48 풀스텝 구동 순서
// ==================================================

const byte FULL_STEP[4][4] = {
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

int stepIndex = 0;

// ==================================================
// 모터 제어
// ==================================================

void setMotorOutput(int index)
{
    digitalWrite(IN1, FULL_STEP[index][0]);
    digitalWrite(IN2, FULL_STEP[index][1]);
    digitalWrite(IN3, FULL_STEP[index][2]);
    digitalWrite(IN4, FULL_STEP[index][3]);
}

void motorOff()
{
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void moveMotor(long steps)
{
    int direction = (steps >= 0) ? 1 : -1;
    long totalSteps = abs(steps);

    for (long i = 0; i < totalSteps; i++)
    {
        stepIndex += direction;

        if (stepIndex > 3)
        {
            stepIndex = 0;
        }

        if (stepIndex < 0)
        {
            stepIndex = 3;
        }

        setMotorOutput(stepIndex);
        delayMicroseconds(STEP_DELAY_US);
    }

    motorOff();
}

void moveRackCm(float distanceCm)
{
    long requiredSteps =
        round(distanceCm * STEPS_PER_CM);

    if (!EXTEND_DIRECTION)
    {
        requiredSteps = -requiredSteps;
    }

    Serial.println();
    Serial.println("----- 모터 작동 정보 -----");

    Serial.print("랙 목표 이동거리: ");
    Serial.print(distanceCm, 2);
    Serial.println(" cm");

    Serial.print("모터 작동 스텝 수: ");
    Serial.println(requiredSteps);

    Serial.println("랙 이동 시작");

    moveMotor(requiredSteps);

    Serial.println("랙 이동 완료");
}

// ==================================================
// HC-SR04 거리 측정
// ==================================================

float measureDistanceOnce()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration =
        pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0)
    {
        return -1.0;
    }

    return duration * 0.0343 / 2.0;
}

float measureAverageDistance()
{
    const int SAMPLE_COUNT = 7;

    float distanceSum = 0.0;
    int validCount = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float distance = measureDistanceOnce();

        Serial.print("측정 ");
        Serial.print(i + 1);
        Serial.print(": ");

        if (distance >= MIN_DISTANCE_CM &&
            distance <= 100.0)
        {
            Serial.print(distance, 2);
            Serial.println(" cm");

            distanceSum += distance;
            validCount++;
        }
        else
        {
            Serial.println("실패");
        }

        delay(80);
    }

    if (validCount == 0)
    {
        return -1.0;
    }

    return distanceSum / validCount;
}

// ==================================================
// 초기 실행
// ==================================================

void setup()
{
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    digitalWrite(TRIG_PIN, LOW);
    motorOff();

    Serial.println();
    Serial.println("====================================");
    Serial.println("초음파센서 + 랙앤피니언 통합 제어");
    Serial.println("3초 후 거리를 측정합니다.");
    Serial.println("====================================");

    delay(3000);

    float measuredDistance =
        measureAverageDistance();

    if (measuredDistance < 0)
    {
        Serial.println();
        Serial.println("거리 측정 실패");
        Serial.println("모터를 작동하지 않습니다.");

        motorOff();
        return;
    }

    Serial.println();
    Serial.print("최종 평균거리: ");
    Serial.print(measuredDistance, 2);
    Serial.println(" cm");

    if (measuredDistance > MAX_DISTANCE_CM)
    {
        Serial.println("측정값이 최대 이동거리 7cm를 초과했습니다.");
        Serial.println("모터를 작동하지 않습니다.");

        motorOff();
        return;
    }

    moveRackCm(measuredDistance);
}

void loop()
{
    // 반복 이동 방지: 전원을 켰을 때 한 번만 작동
}
