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
// 📏 로봇 물리 스펙 설정 (기어비 제거, 3m 목표)
// ========================================================
const float WHEEL_DIAMETER_MM = 67.0; 
const int ENCODER_HOLES = 20;         
const float TARGET_DISTANCE_MM = 3000.0; // ★ 수정: 목표 거리 3미터

long targetCount = 0; 

// ========================================================
// ⚙️ PID 게인 튜닝 (최고 속도 지향)
// ========================================================
// 거리 제어 (빠르게 접근하고 급브레이크)
float Kp_dist = 5.0;  
float Kd_dist = 4.0;  
long prevDistError = 0;

// 직진 제어 (최고 속도에서도 직진 유지)
float Kp_sync = 5.0; 
float Kd_sync = 3.0;  
long prevSyncError = 0;

unsigned long prevTime = 0;
const int DT = 50; 
bool missionComplete = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(encoderLeftPin, INPUT_PULLUP); pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();

  // ★ 기어비를 제외한 순수 목표 카운트 계산
  // 3000 / (67 * 3.14159) * 20 = 약 285 카운트
  float wheelCircumference = WHEEL_DIAMETER_MM * 3.14159;
  targetCount = (TARGET_DISTANCE_MM / wheelCircumference) * ENCODER_HOLES;

  delay(3000); // 3초 뒤 출발
  prevTime = millis();
}

void loop() {
  if (missionComplete) return; 

  unsigned long currentTime = millis();
  int dt = currentTime - prevTime;

  if (dt >= DT) {
    prevTime = currentTime;

    long avgCount = (leftCount + rightCount) / 2;
    long distError = targetCount - avgCount;     
    long syncError = leftCount - rightCount;     

    // 1. 거리 제어 
    float distP = Kp_dist * distError;
    float distD = Kd_dist * ((distError - prevDistError) / dt);
    
    int basePWM = distP + distD;

    // 목표 도달 판단 (오차가 0 이하가 되면 즉시 정지)
    if (distError <= 0) {
      stopMotors();
      missionComplete = true;
      Serial.println("0,0,0,0"); // 플로터 그래프 바닥으로 내림
      return;
    }

    if (basePWM > 255) basePWM = 255;
    if (basePWM < 80) basePWM = 80;

    // 2. 직진 제어 
    float syncP = Kp_sync * syncError;
    float syncD = Kd_sync * ((syncError - prevSyncError) / dt);
    
    int turnOffset = syncP + syncD;

    int leftPWM = basePWM - turnOffset;
    int rightPWM = basePWM + turnOffset;

    if (leftPWM > 255) leftPWM = 255;  if (leftPWM < 0) leftPWM = 0;
    if (rightPWM > 255) rightPWM = 255; if (rightPWM < 0) rightPWM = 0;

    digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, leftPWM);
    digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPWM);

    prevDistError = distError;
    prevSyncError = syncError;

    // ----------------------------------------------------
    // 📊 시리얼 플로터 출력
    // Value 1: 목표 카운트 (약 285)
    // Value 2: 왼쪽 바퀴 카운트
    // Value 3: 오른쪽 바퀴 카운트
    // Value 4: 현재 기준 속도 (PWM)
    // ----------------------------------------------------
    Serial.print(targetCount); Serial.print(","); 
    Serial.print(leftCount);   Serial.print(","); 
    Serial.print(rightCount);  Serial.print(","); 
    Serial.println(basePWM); 
  }
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}