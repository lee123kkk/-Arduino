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
// 📏 로봇 물리 스펙 및 목표 (3m)
// ========================================================
const float WHEEL_DIAMETER_MM = 67.0; 
const int ENCODER_HOLES = 20;         
const float TARGET_DISTANCE_MM = 3000.0; 

long targetCount = 0; 

// ========================================================
// ⚙️ 최종 튜닝된 PID 게인 (사용자 최적화 값 반영)
// ========================================================
float Kp_dist = 5.0; float Kd_dist = 4.0;
float Kp_sync = 5.0; float Kd_sync = 3.0;

long prevDistError = 0;
long prevSyncError = 0;

// 상태 제어 변수
unsigned long prevTime = 0;
unsigned long startTime = 0;
unsigned long lastLogTime = 0;
const int DT = 50; 
const int LOG_INTERVAL = 200; // 0.2초마다 데이터 전송

bool isRunning = false;
bool missionComplete = false;

void setup() {
  // 1. 모든 행동을 멈추고 ESP32가 완전히 부팅될 때까지 3초간 조용히 기다립니다.
  delay(3000); 

  // 2. 3초가 지난 후에야 비로소 통신을 시작합니다.
  Serial.begin(9600); 
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  // 목표 카운트 계산
  float wheelCircumference = WHEEL_DIAMETER_MM * 3.14159;
  targetCount = (TARGET_DISTANCE_MM / wheelCircumference) * ENCODER_HOLES;

  stopMotors();

  // 3. 이제 ESP32가 통신을 받을 준비가 완벽히 끝났으므로 메시지를 보냅니다.
  Serial.println(">>> System Ready. Send 'F' to Start, 'S' to Emergency Stop.");
}

void loop() {
  // 1. 블루투스 명령 처리
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; // 대문자 변환

    if (cmd == 'F' && !isRunning) {
      startMission();
    } 
    else if (cmd == 'S') {
      emergencyStop();
    }
  }

  // 2. 미션 주행 로직
  if (isRunning && !missionComplete) {
    unsigned long currentTime = millis();
    if (currentTime - prevTime >= DT) {
      int dt = currentTime - prevTime;
      prevTime = currentTime;

      long avgCount = (leftCount + rightCount) / 2;
      long distError = targetCount - avgCount;     
      long syncError = leftCount - rightCount;     

      // 목표 도달 판단
      if (distError <= 0) {
        finishMission();
        return;
      }

      // 거리 PD 제어
      int basePWM = (Kp_dist * distError) + (Kd_dist * ((distError - prevDistError) / dt));
      basePWM = constrain(basePWM, 80, 255);

      // 직진 PD 제어
      int turnOffset = (Kp_sync * syncError) + (Kd_sync * ((syncError - prevSyncError) / dt));
      
      int leftPWM = constrain(basePWM - turnOffset, 0, 180);
      int rightPWM = constrain(basePWM + turnOffset, 0, 180);

      // 모터 구동
      digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, leftPWM);
      digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPWM);

      prevDistError = distError;
      prevSyncError = syncError;

      // 실시간 로그 전송 (LOG_INTERVAL 마다)
      if (currentTime - lastLogTime >= LOG_INTERVAL) {
        lastLogTime = currentTime;
        sendLiveLog(leftPWM, rightPWM);
      }
    }
  }
}

// 미션 시작 함수
void startMission() {
  leftCount = 0; rightCount = 0;
  prevDistError = 0; prevSyncError = 0;
  missionComplete = false;
  isRunning = true;
  startTime = millis();
  
  Serial.println("---------- MISSION START ----------");
  Serial.print("Target Count: "); Serial.println(targetCount);
  Serial.print("Start Time (ms): "); Serial.println(startTime);
}

// 긴급 정지 함수
void emergencyStop() {
  stopMotors();
  isRunning = false;
  missionComplete = true;
  Serial.println("!!! EMERGENCY STOP EXECUTED !!!");
}

// 미션 완료 함수
void finishMission() {
  stopMotors();
  unsigned long endTime = millis();
  isRunning = false;
  missionComplete = true;
  
  Serial.println("---------- MISSION COMPLETE ----------");
  Serial.print("End Time (ms): "); Serial.println(endTime);
  Serial.print("Total Duration (ms): "); Serial.println(endTime - startTime);
  Serial.print("Final L_Cnt: "); Serial.print(leftCount);
  Serial.print(" | R_Cnt: "); Serial.println(rightCount);
}

// 실시간 데이터 전송 함수
void sendLiveLog(int lpwm, int rpwm) {
  // [L_PWM, R_PWM, L_Cnt, R_Cnt] 형식으로 전송
  Serial.print("DATA >> PWM("); Serial.print(lpwm);
  Serial.print(","); Serial.print(rpwm);
  Serial.print(") | CNT("); Serial.print(leftCount);
  Serial.print(","); Serial.print(rightCount);
  Serial.println(")");
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}