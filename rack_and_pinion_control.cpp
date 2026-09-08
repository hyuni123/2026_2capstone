#include <Arduino.h>

// ESP32 → ULN2003
const int IN1 = 13;
const int IN2 = 12;
const int IN3 = 14;
const int IN4 = 27;

// 이동거리
// 기존 650보다 약 3배 이상 많이 회전하도록 설정
long TEST_STEPS = 2000;

// 반대 방향이면 false로 변경
const bool EXTEND_DIRECTION = true;

// 2000부터 시험
// 떨리거나 멈추면 3000으로 변경
const int STEP_DELAY_US = 2000;

// 두 코일을 동시에 사용하는 풀스텝 방식
// 하프스텝보다 이동 중 토크가 비교적 일정함
const byte FULL_STEP[4][4] = {
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

int stepIndex = 0;

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

void setup()
{
    Serial.begin(115200);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    motorOff();

    Serial.println();
    Serial.println("============================");
    Serial.println("랙앤피니언 토크 및 거리 시험");
    Serial.print("시험 스텝 수: ");
    Serial.println(TEST_STEPS);
    Serial.print("스텝 지연시간: ");
    Serial.println(STEP_DELAY_US);
    Serial.println("3초 후 작동합니다.");
    Serial.println("============================");

    delay(3000);

    long commandedSteps = TEST_STEPS;

    if (!EXTEND_DIRECTION)
    {
        commandedSteps = -commandedSteps;
    }

    moveMotor(commandedSteps);

    Serial.println("시험 완료");
    Serial.println("랙 이동거리를 mm 단위로 측정하세요.");
}

void loop()
{
    // 한 번만 작동
}
