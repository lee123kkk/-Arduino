// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 (현재는 연결만 유지) ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

// --- 적외선 센서 핀 ---
const int irLeftPin = A0;   
const int irRightPin = A1;  

// --- 색상 인식 기준 ---
const int BLACK = HIGH; 
const int WHITE = LOW;

// --- 주행 설정 변수 ---
int baseSpeed = 90;   // 기존 80 -> 90으로 상향 (모터 멈춤 방지)
int turnSpeed = 150;  // 회복 및 코너링 힘

int lapCount = 0;          
int intersectionCount = 0; // 누적 교차로 감지 횟수
bool isRunning = false;    
bool missionComplete = false;

// 🌟 선을 놓치기 직전 상태 기억 (0:직진, 1:좌, 2:우)
int lastDirection = 0; 

// 🌟 1바퀴당 인식되는 교차로 개수 설정 (매우 중요!)
// 만약 차가 '시작 가로선'만 유일하게 교차로로 인식한다면 이 값을 1로 설정하세요.
// (일단 1로 설정해서 테스트해 보시는 것을 추천합니다.)
const int INTERSECTIONS_PER_LAP = 1; 

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(irLeftPin, INPUT);
  pinMode(irRightPin, INPUT);

  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);

  stopMotors();
  Serial.println(">>> Outer 5-Lap Mission Ready. Send 'F' to Start, 'S' to Stop.");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; 
    if (cmd == 'F' && !isRunning && !missionComplete) startMission();
    else if (cmd == 'S') emergencyStop();
  }

  if (isRunning && !missionComplete) {
    int leftVal = digitalRead(irLeftPin);
    int rightVal = digitalRead(irRightPin);

    // [상황 1] 교차로 발견 후보 (양쪽 모두 검은선)
    if (leftVal == BLACK && rightVal == BLACK) {
      delay(50); 
      int doubleCheckLeft = digitalRead(irLeftPin);
      int doubleCheckRight = digitalRead(irRightPin);
      
      if (doubleCheckLeft == BLACK && doubleCheckRight == BLACK) {
        handleIntersection();
      }
    }
    // [상황 2] 왼쪽으로 치우침 -> 좌회전으로 보정
    else if (leftVal == BLACK && rightVal == WHITE) {
      setMotorSpeed(0, baseSpeed);
      lastDirection = 1; 
    }
    // [상황 3] 오른쪽으로 치우침 -> 우회전으로 보정
    else if (leftVal == WHITE && rightVal == BLACK) {
      setMotorSpeed(baseSpeed, 0);
      lastDirection = 2; 
    }
    // [상황 4] 양쪽 모두 흰색 (선을 이탈함!)
    else {
      // 🌟 부드러운 곡선 복귀 방식 적용 (오실레이션 및 스톨 완벽 해결)
      if (lastDirection == 1) {
        // 선이 왼쪽에 있었으므로, 왼쪽 바퀴를 멈추고 오른쪽 바퀴만 굴려 왼쪽으로 휘어지게 함
        setMotorSpeed(0, baseSpeed); 
      } 
      else if (lastDirection == 2) {
        // 선이 오른쪽에 있었으므로, 오른쪽 바퀴를 멈추고 왼쪽 바퀴만 굴려 오른쪽으로 휘어지게 함
        setMotorSpeed(baseSpeed, 0);
      } 
      else {
        setMotorSpeed(baseSpeed, baseSpeed);
      }
    }
  }
}
// ==========================================
// 🚗 교차로(교차점) 처리 함수 (5바퀴 외곽선 직진)
// ==========================================
void handleIntersection() {
  stopMotors();
  delay(200); // 멈춰서 안정화

  intersectionCount++;
  Serial.print(">>> 교차로 감지! 누적 횟수: ");
  Serial.println(intersectionCount);

  // 교차로를 만날 때마다 무조건 직진하여 통과
  goStraightOverIntersection();

  // 바퀴 수 계산 로직
  if (intersectionCount % INTERSECTIONS_PER_LAP == 0) {
    lapCount++;
    Serial.print(">>> 바퀴 완료! 현재 바퀴 수: ");
    Serial.println(lapCount);

    if (lapCount >= 5) {
      finishMission();
    }
  }
}

// ⬆️ 교차로를 직진해서 넘어가는 함수
void goStraightOverIntersection() {
  setMotorSpeed(baseSpeed, baseSpeed);
  delay(400); // 센서가 굵은 선을 완전히 넘어갈 때까지 강제 직진
}

// ==========================================
// ⚙️ 모터 제어 유틸리티 함수 (반전 로직 적용됨)
// ==========================================
void setMotorSpeed(int leftPwm, int rightPwm) {
  // 왼쪽 모터 (전후진 반전)
  if (leftPwm >= 0) {
    digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, leftPwm);
  } else {
    analogWrite(leftMotor_IA, -leftPwm); digitalWrite(leftMotor_IB, LOW);
  }
  
  // 오른쪽 모터 (전후진 반전)
  if (rightPwm >= 0) {
    digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPwm);
  } else {
    analogWrite(rightMotor_IA, -rightPwm); digitalWrite(rightMotor_IB, LOW);
  }
}

void startMission() {
  lapCount = 0;
  intersectionCount = 0;
  lastDirection = 0;
  missionComplete = false;
  isRunning = true;
  Serial.println("---------- MISSION START ----------");
}

void emergencyStop() {
  stopMotors();
  isRunning = false;
  Serial.println("!!! EMERGENCY STOP !!!");
}

void finishMission() {
  stopMotors();
  isRunning = false; 
  missionComplete = true;
  Serial.println("---------- MISSION COMPLETE (5 Laps) ----------");
}

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}