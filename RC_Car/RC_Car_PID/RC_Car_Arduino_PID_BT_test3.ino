// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 (원상 복구 완료) ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

volatile long leftCount = 0;
volatile long rightCount = 0;

// ========================================================
// 📏 목표 카운트 (약 3미터)
long targetCount = 455; // 428 * (300 / 282) 계산 결과 반영

// ========================================================
// ⚙️ 최종 튜닝된 PID 게인 (진동에 둔감하도록 하향 조정)
float Kp_dist = 4.0; float Kd_dist = 2.0;
float Kp_sync = 1.5; float Kd_sync = 1.0; 
// ========================================================

long prevDistError = 0;
long prevSyncError = 0;

unsigned long prevTime = 0;
unsigned long startTime = 0;
unsigned long lastLogTime = 0;
const int DT = 50; 
const int LOG_INTERVAL = 200; 

bool isRunning = false;
bool missionComplete = false;

void setup() {
  delay(3000); 
  Serial.begin(9600); 
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();
  Serial.println(">>> System Ready. Send 'F' to Start, 'S' to Emergency Stop.");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; 

    if (cmd == 'F' && !isRunning) startMission();
    else if (cmd == 'S') emergencyStop();
  }

  if (isRunning && !missionComplete) {
    unsigned long currentTime = millis();
    if (currentTime - prevTime >= DT) {
      int dt = currentTime - prevTime;
      prevTime = currentTime;

      long avgCount = (leftCount + rightCount) / 2;
      long distError = targetCount - avgCount;     
      long syncError = leftCount - rightCount;     

      if (distError <= 0) {
        finishMission();
        return;
      }

      // 거리 PD 제어 (Base 속도는 100~150 사이로 부드럽게)
      int basePWM = (Kp_dist * distError) + (Kd_dist * ((distError - prevDistError) / dt));
      int basePWM_limited = constrain(basePWM, 80, 110);  //속도를 줄여서 바퀴가 헛도는 현상이나 심한 진동 방지

      // 직진 PD 제어
      int turnOffset = (Kp_sync * syncError) + (Kd_sync * ((syncError - prevSyncError) / dt));
      
      // 최종 모터 출력 계산 (최대 180 제한)
      int leftPWM = constrain(basePWM_limited - turnOffset, 0, 180);
      int rightPWM = constrain(basePWM_limited + turnOffset, 0, 180);

      //기존 후륜 구동
      //digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, leftPWM);
      //digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPWM);
      //새로운 전륜 구동
      analogWrite(leftMotor_IA, leftPWM);  digitalWrite(leftMotor_IB, LOW);
      analogWrite(rightMotor_IA, rightPWM); digitalWrite(rightMotor_IB, LOW);

      prevDistError = distError;
      prevSyncError = syncError;

      if (currentTime - lastLogTime >= LOG_INTERVAL) {
        lastLogTime = currentTime;
        sendLiveLog(currentTime, distError, syncError, basePWM_limited, turnOffset, leftPWM, rightPWM);
      }
    }
  }
}

void startMission() {
  leftCount = 0; rightCount = 0;
  prevDistError = 0; prevSyncError = 0;
  missionComplete = false;
  isRunning = true;
  startTime = millis();
  Serial.println("---------- MISSION START ----------");
}

void emergencyStop() {
  stopMotors();
  isRunning = false; missionComplete = true;
  Serial.println("!!! EMERGENCY STOP EXECUTED !!!");
}

void finishMission() {
  stopMotors();
  isRunning = false; missionComplete = true;
  Serial.println("---------- MISSION COMPLETE ----------");
}

void sendLiveLog(unsigned long currentMs, long dErr, long sErr, int bPwm, int tOffset, int lPwm, int rPwm) {
  unsigned long runTime = currentMs - startTime; 
  Serial.print("["); Serial.print(runTime); Serial.print("ms] CNT(L:"); Serial.print(leftCount);
  Serial.print(", R:"); Serial.print(rightCount); Serial.print(") | ERR(Dist:"); Serial.print(dErr);
  Serial.print(", Sync:"); Serial.print(sErr); Serial.print(") | PID(Base:"); Serial.print(bPwm);
  Serial.print(", Turn:"); Serial.print(tOffset); Serial.print(") => OUT(L:"); Serial.print(lPwm);
  Serial.print(", R:"); Serial.println(rPwm);
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}