#include <Arduino.h>

// ==================================================
// HC-SR04 핀
// ==================================================

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

// ==================================================
// ULN2003 핀
// ==================================================

const int IN1 = 13;
const int IN2 = 12;
const int IN3 = 14;
const int IN4 = 27;

// ==================================================
// 제어 설정
// ==================================================

// 랙이 완전히 들어간 상태의 기준거리
const float BASE_DISTANCE_CM = 4.0;

// 피니언 피치원 지름 20mm
// 풀스텝 기준 약 326스텝/cm
const float STEPS_PER_CM = 326.0;

// 최대 랙 연장거리
const float MAX_EXTENSION_CM = 7.0;

// 측정값 변화가 이보다 작으면 무시
const float DEAD_ZONE_CM = 0.2;

// 랙이 반대로 움직이면 false로 변경
const bool EXTEND_DIRECTION = true;

// 모터 속도
const int STEP_DELAY_US = 2000;

// 측정 및 제어 주기
const int CONTROL_INTERVAL_MS = 1000;

// ==================================================
// 28BYJ-48 풀스텝 순서
// ==================================================

const byte FULL_STEP[4][4] = {
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

int stepIndex = 0;

// 현재 랙 연장 위치
long currentPositionSteps = 0;

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

// physicalSteps 양수: 랙 연장
// physicalSteps 음수: 랙 축소
void moveRackSteps(long physicalSteps)
{
    if (physicalSteps == 0)
    {
        return;
    }

    long motorSteps = physicalSteps;

    if (!EXTEND_DIRECTION)
    {
        motorSteps = -motorSteps;
    }

    int direction =
        (motorSteps > 0) ? 1 : -1;

    long totalSteps = abs(motorSteps);

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

    currentPositionSteps += physicalSteps;

    if (currentPositionSteps < 0)
    {
        currentPositionSteps = 0;
    }
}

// ==================================================
// 초음파센서 거리 측정
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

// 10회 측정 후 최솟값과 최댓값을 제외하고 평균 계산
float measureFilteredDistance()
{
    const int SAMPLE_COUNT = 10;

    float values[SAMPLE_COUNT];
    int validCount = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float distance = measureDistanceOnce();

        if (distance >= 2.0 &&
            distance <= 50.0)
        {
            values[validCount] = distance;
            validCount++;
        }

        delay(50);
    }

    if (validCount < 5)
    {
        return -1.0;
    }

    // 오름차순 정렬
    for (int i = 0; i < validCount - 1; i++)
    {
        for (int j = i + 1; j < validCount; j++)
        {
            if (values[i] > values[j])
            {
                float temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }

    // 최솟값과 최댓값을 제외한 평균
    float sum = 0.0;

    for (int i = 1; i < validCount - 1; i++)
    {
        sum += values[i];
    }

    return sum / (validCount - 2);
}

// ==================================================
// 랙 위치 제어
// ==================================================

void controlRack(float measuredDistanceCm)
{
    // 측정거리 - 기준거리
    float targetExtensionCm =
        measuredDistanceCm - BASE_DISTANCE_CM;

    // 4cm 이하에서는 완전 축소
    if (targetExtensionCm < 0)
    {
        targetExtensionCm = 0;
    }

    // 최대 연장거리 초과 확인
    if (targetExtensionCm > MAX_EXTENSION_CM)
    {
        Serial.println();
        Serial.println("필요한 연장거리가 7cm를 초과했습니다.");
        Serial.println("안전을 위해 모터를 작동하지 않습니다.");

        motorOff();
        return;
    }

    long targetPositionSteps =
        round(targetExtensionCm * STEPS_PER_CM);

    long differenceSteps =
        targetPositionSteps - currentPositionSteps;

    float currentPositionCm =
        currentPositionSteps / STEPS_PER_CM;

    float moveDistanceCm =
        differenceSteps / STEPS_PER_CM;

    Serial.println();
    Serial.println("----------- 제어 정보 -----------");

    Serial.print("초음파 측정거리: ");
    Serial.print(measuredDistanceCm, 2);
    Serial.println(" cm");

    Serial.print("기준거리: ");
    Serial.print(BASE_DISTANCE_CM, 2);
    Serial.println(" cm");

    Serial.print("목표 랙 연장길이: ");
    Serial.print(targetExtensionCm, 2);
    Serial.println(" cm");

    Serial.print("현재 랙 연장길이: ");
    Serial.print(currentPositionCm, 2);
    Serial.println(" cm");

    Serial.print("이번 이동거리: ");
    Serial.print(moveDistanceCm, 2);
    Serial.println(" cm");

    // 작은 측정 변화는 무시
    if (abs(moveDistanceCm) <= DEAD_ZONE_CM)
    {
        Serial.println("허용 오차 범위 → 모터 정지");

        motorOff();
        return;
    }

    if (differenceSteps > 0)
    {
        Serial.println("거리가 증가함 → 랙 연장");
    }
    else
    {
        Serial.println("거리가 감소함 → 랙 축소");
    }

    moveRackSteps(differenceSteps);

    Serial.print("제어 후 랙 위치: ");
    Serial.print(
        currentPositionSteps / STEPS_PER_CM,
        2
    );
    Serial.println(" cm");
}

// ==================================================
// 초기 설정
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

    // 전원을 켤 때 랙이 완전히 들어간 상태라고 가정
    currentPositionSteps = 0;

    Serial.println();
    Serial.println("================================");
    Serial.println("기준거리 4cm 랙앤피니언 제어");
    Serial.println("4cm 초과: 랙 연장");
    Serial.println("4cm 방향으로 감소: 랙 축소");
    Serial.println("================================");

    delay(2000);
}

// ==================================================
// 반복 실행
// ==================================================

void loop()
{
    float measuredDistance =
        measureFilteredDistance();

    if (measuredDistance < 0)
    {
        Serial.println("거리 측정 실패 → 모터 정지");

        motorOff();
        delay(CONTROL_INTERVAL_MS);
        return;
    }

    controlRack(measuredDistance);

    // 모터 진동이 끝난 후 다시 측정
    delay(CONTROL_INTERVAL_MS);
}
