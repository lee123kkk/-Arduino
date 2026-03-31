//전원을 키면 3초후에 자동으로 2m이동

// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

volatile long leftCount = 0;
volatile long rightCount = 0;

// ========================================================
// 📏 로봇 물리 스펙 설정 (입력 필수!)
// ========================================================
const float WHEEL_DIAMETER_MM = 67.0; // 바퀴 지름 (mm)
const int ENCODER_HOLES = 20;         // 엔코더 원판의 구멍 개수 (보통 20개)
const float TARGET_DISTANCE_MM = 2000.0; // 목표 이동 거리 (2m = 2000mm)

// 목표 카운트 자동 계산 (2000mm 가려면 몇 번 구멍을 세어야 하는가?)
// 바퀴 둘레 = 67 * 3.14 = 210.48mm. 2000 / 210.48 = 약 9.5바퀴. 9.5 * 20 = 190 카운트.
long targetCount = 0; 

// ========================================================
// ⚙️ PID 게인 튜닝 변수 (지글러-니콜스 튜닝 위치)
// ========================================================
// 1. 거리 제어 (브레이크 제어)
float Kp_dist = 2.5;  // P: 목표까지 남은 거리에 비례해서 속도를 냄
float Kd_dist = 0.5;  // D: 속도가 너무 빠르면 미리 브레이크를 밟음 (오버슛 방지)
long prevDistError = 0;

// 2. 직진 동기화 제어 (좌우 밸런스)
float Kp_sync = 1.5;  // P: 좌우 카운트 차이가 날수록 강하게 핸들을 꺾음
float Kd_sync = 0.2;  // D: 핸들이 너무 휙휙 꺾이는 것을 방지(잔진동 억제)
long prevSyncError = 0;

// 제어 주기
unsigned long prevTime = 0;
const int DT = 50; // 50ms 마다 제어 (1초에 20번 연산)
bool missionComplete = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(encoderLeftPin, INPUT_PULLUP); pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();

  // 바퀴 둘레 계산 및 목표 카운트 설정
  float wheelCircumference = WHEEL_DIAMETER_MM * 3.14159;
  targetCount = (TARGET_DISTANCE_MM / wheelCircumference) * ENCODER_HOLES;

  delay(3000); // 전원 켜고 3초 뒤에 출발! (바닥에 놓을 시간 확보)
  prevTime = millis();
}

void loop() {
  if (missionComplete) return; // 미션 완료 시 루프 정지

  unsigned long currentTime = millis();
  int dt = currentTime - prevTime;

  if (dt >= DT) {
    prevTime = currentTime;

    // 1. 현재 상태 읽기
    long avgCount = (leftCount + rightCount) / 2;
    long distError = targetCount - avgCount;     // 거리 오차
    long syncError = leftCount - rightCount;     // 좌우 밸런스 오차

    // ----------------------------------------------------
    // 🎯 1단계: 거리 제어 (PD 제어) - 얼마나 세게 달릴까?
    // ----------------------------------------------------
    float distP = Kp_dist * distError;
    float distD = Kd_dist * ((distError - prevDistError) / dt);
    
    int basePWM = distP + distD;

    // 목표 도달 판단 (오차가 0 이하가 되면 정지)
    if (distError <= 0) {
      stopMotors();
      missionComplete = true;
      Serial.println("Mission Complete!");
      return;
    }

    // 기본 속도 제한 (최대 255, 최소 구동 전압 80)
    if (basePWM > 255) basePWM = 255;
    if (basePWM < 80) basePWM = 80;

    // ----------------------------------------------------
    // ⚖️ 2단계: 직진 제어 (PD 제어) - 핸들을 얼마나 꺾을까?
    // ----------------------------------------------------
    float syncP = Kp_sync * syncError;
    float syncD = Kd_sync * ((syncError - prevSyncError) / dt);
    
    int turnOffset = syncP + syncD;

    // 최종 모터 속도 계산 (기준 속도에 핸들 꺾는 값을 더하고 뺌)
    int leftPWM = basePWM - turnOffset;
    int rightPWM = basePWM + turnOffset;

    // 모터 속도 하드웨어 한계 제한
    if (leftPWM > 255) leftPWM = 255;  if (leftPWM < 0) leftPWM = 0;
    if (rightPWM > 255) rightPWM = 255; if (rightPWM < 0) rightPWM = 0;

    // 모터 구동 (전진)
    digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, leftPWM);
    digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPWM);

    // 과거 오차 업데이트
    prevDistError = distError;
    prevSyncError = syncError;

    // ----------------------------------------------------
    // 📊 시리얼 플로터 (그래프) 데이터 출력
    // 출력 형식: 목표거리, 현재거리, 기본PWM
    // ----------------------------------------------------
    Serial.print(targetCount); 
    Serial.print(","); 
    Serial.print(avgCount); 
    Serial.print(","); 
    Serial.println(basePWM); 
  }
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}
