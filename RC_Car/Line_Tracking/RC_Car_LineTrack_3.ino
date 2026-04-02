// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

// --- 적외선 센서 핀 ---
const int irLeftPin = A0;   
const int irRightPin = A1;  

// --- 색상 인식 기준 ---
const int BLACK = HIGH; 
const int WHITE = LOW;

// --- 🌟 주행 설정 변수 ---
int baseSpeed = 90;   // 직진 기본 속도
const int INTERSECTIONS_PER_LAP = 1; // 1바퀴당 인식할 교차로 수

// --- 🌟 엔코더 및 PID 변수 (test3.ino에서 가져옴) ---
volatile long leftCount = 0;
volatile long rightCount = 0;
float Kp_sync = 1.5; 
float Kd_sync = 1.0; 
long prevSyncError = 0;
unsigned long prevTime = 0;

int lapCount = 0;          
int intersectionCount = 0; 
bool isRunning = false;    
bool missionComplete = false;
int lastDirection = 0; 

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  pinMode(irLeftPin, INPUT); pinMode(irRightPin, INPUT);
  
  // 엔코더 인터럽트 설정
  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();
  Serial.println(">>> PID Line Tracking Ready. Send 'F' to Start.");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; 
    if (cmd == 'F' && !isRunning && !missionComplete) startMission();
    else if (cmd == 'S') emergencyStop();
  }

  if (isRunning && !missionComplete) {
    unsigned long currentTime = millis();
    int dt = currentTime - prevTime;
    
    // 루프 속도 조절 (너무 잦은 계산 방지)
    if (dt < 20) return; 
    prevTime = currentTime;

    int leftVal = digitalRead(irLeftPin);
    int rightVal = digitalRead(irRightPin);

    // [상황 1] 교차로 발견 후보
    if (leftVal == BLACK && rightVal == BLACK) {
      delay(50); 
      if (digitalRead(irLeftPin) == BLACK && digitalRead(irRightPin) == BLACK) {
        handleIntersection();
      }
    }
    // [상황 2] 왼쪽으로 치우침 -> 좌회전 보정
    else if (leftVal == BLACK && rightVal == WHITE) {
      // 🌟 부드러운 코너링: 한쪽을 멈추지 않고 40% 속도로 굴려 곡선을 그림
      setMotorSpeed(baseSpeed * 0.4, baseSpeed);
      lastDirection = 1; 
      resetEncoders(); // 회전 중에는 동기화 에러 누적을 막기 위해 리셋
    }
    // [상황 3] 오른쪽으로 치우침 -> 우회전 보정
    else if (leftVal == WHITE && rightVal == BLACK) {
      setMotorSpeed(baseSpeed, baseSpeed * 0.4);
      lastDirection = 2; 
      resetEncoders(); 
    }
    // [상황 4] 양쪽 모두 흰색 -> 🌟 완벽한 직진 (엔코더 PID 동기화 적용!)
    else {
      long syncError = leftCount - rightCount;
      int turnOffset = (Kp_sync * syncError) + (Kd_sync * ((syncError - prevSyncError) / dt));
      
      int lPwm = constrain(baseSpeed - turnOffset, 0, 180);
      int rPwm = constrain(baseSpeed + turnOffset, 0, 180);
      
      setMotorSpeed(lPwm, rPwm);
      prevSyncError = syncError;
      lastDirection = 0;
    }
  }
}

// ==========================================
// 🚗 교차로(교차점) 처리 함수
// ==========================================
void handleIntersection() {
  intersectionCount++;
  Serial.print(">>> 교차로 감지! 누적: "); Serial.println(intersectionCount);

  // 🌟 지름길 철벽 방어: 엔코더를 활용한 "정밀 거리" 교차로 무시 돌파
  crossIntersectionWithEncoders(30); // 30 카운트만큼 직진 (수치 조절 필요)

  if (intersectionCount % INTERSECTIONS_PER_LAP == 0) {
    lapCount++;
    Serial.print(">>> 바퀴 완료! 현재 바퀴 수: "); Serial.println(lapCount);
    if (lapCount >= 5) finishMission();
  }
}

// 🌟 엔코더로 일정한 거리를 똑바로 직진하는 함수
void crossIntersectionWithEncoders(long targetCounts) {
  resetEncoders();
  long avgCount = 0;
  
  // 지정한 카운트(거리)에 도달할 때까지 센서 무시하고 직진
  while(avgCount < targetCounts) {
    long syncError = leftCount - rightCount;
    int turnOffset = (Kp_sync * syncError);
    setMotorSpeed(baseSpeed - turnOffset, baseSpeed + turnOffset);
    
    avgCount = (leftCount + rightCount) / 2;
    delay(10); 
  }
  resetEncoders(); // 직진 끝나면 다시 초기화
}

// ==========================================
// ⚙️ 유틸리티 함수들
// ==========================================
void resetEncoders() {
  leftCount = 0; 
  rightCount = 0; 
  prevSyncError = 0;
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void setMotorSpeed(int leftPwm, int rightPwm) {
  if (leftPwm >= 0) {
    digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, leftPwm);
  } else {
    analogWrite(leftMotor_IA, -leftPwm); digitalWrite(leftMotor_IB, LOW);
  }
  if (rightPwm >= 0) {
    digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPwm);
  } else {
    analogWrite(rightMotor_IA, -rightPwm); digitalWrite(rightMotor_IB, LOW);
  }
}

void startMission() {
  resetEncoders();
  lapCount = 0; intersectionCount = 0; lastDirection = 0;
  missionComplete = false; isRunning = true;
  prevTime = millis();
  Serial.println("---------- MISSION START ----------");
}

void emergencyStop() {
  stopMotors(); isRunning = false; Serial.println("!!! STOP !!!");
}

void finishMission() {
  stopMotors(); isRunning = false; missionComplete = true;
  Serial.println("---------- COMPLETE ----------");
}

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}