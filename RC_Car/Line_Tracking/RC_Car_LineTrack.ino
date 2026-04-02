// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 (현재는 연결만 유지) ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

// --- 추가된 적외선 센서 핀 ---
const int irLeftPin = A0;   // 왼쪽 센서 OUT
const int irRightPin = A1;  // 오른쪽 센서 OUT

// --- 색상 인식 기준 ---
const int BLACK = HIGH; 
const int WHITE = LOW;

// --- 주행 설정 변수 ---
// --- 주행 설정 변수 ---
int baseSpeed = 80;   // 다시 80으로 낮춤 (코너 이탈 방지, 부드러운 라인 트래킹)
int turnSpeed = 150;  // 150 유지 (교차로에서 정지 마찰력을 이기고 제자리 회전할 강력한 힘)

int lapCount = 0;          // 현재 바퀴 수 (최대 5)
int intersectionCount = 0; // 한 바퀴 내에서 만난 교차점 수 (1~3)
bool isRunning = false;    
bool missionComplete = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(irLeftPin, INPUT);
  pinMode(irRightPin, INPUT);

  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);

  stopMotors();
  Serial.println(">>> Line Tracking Ready (Reversed). Send 'F' to Start, 'S' to Stop.");
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
      
      // --- 소프트웨어 필터링 (Double-Check) ---
      delay(50); // 50ms 대기 (차가 코너를 살짝 더 빠져나가도록 허용)
      
      // 50ms 후 센서 상태 다시 읽기
      int doubleCheckLeft = digitalRead(irLeftPin);
      int doubleCheckRight = digitalRead(irRightPin);
      
      // 다시 확인했을 때도 양쪽 모두 검은선이면 진짜 교차로 인정
      if (doubleCheckLeft == BLACK && doubleCheckRight == BLACK) {
        handleIntersection();
      }
      // 아닐 경우 아무것도 하지 않고 빠져나가면, 바로 아래의 상황 2, 3 로직에 의해 자연스럽게 코너를 돕니다.
    }
  
    // [상황 2] 왼쪽으로 치우침 -> 좌회전으로 보정 (왼쪽 속도 0, 오른쪽 속도 baseSpeed)
    else if (leftVal == BLACK && rightVal == WHITE) {
      setMotorSpeed(0, baseSpeed);
    }
    // [상황 3] 오른쪽으로 치우침 -> 우회전으로 보정 (왼쪽 속도 baseSpeed, 오른쪽 속도 0)
    else if (leftVal == WHITE && rightVal == BLACK) {
      setMotorSpeed(baseSpeed, 0);
    }
    // [상황 4] 양쪽 모두 흰색 -> 직진
    else {
      setMotorSpeed(baseSpeed, baseSpeed);
    }
  }
}

// ==========================================
// 🚗 교차로(교차점) 처리 함수 
// ==========================================
void handleIntersection() {
  stopMotors();
  delay(200); 

  intersectionCount++;

  if (intersectionCount == 1) { 
    Serial.println(">>> 1번 교차로 통과 (위쪽)");
    if (lapCount >= 1 && lapCount <= 3) {
      Serial.println(" -> 지름길로 진입 (좌회전)");
      turnLeftAtIntersection(); 
    } else {
      Serial.println(" -> 외곽선 코스 유지 (직진)");
      goStraightOverIntersection();          
    }
  } 
  else if (intersectionCount == 2) { 
    Serial.println(">>> 2번 교차로 통과 (아래쪽)");
    if (lapCount >= 1 && lapCount <= 3) {
      Serial.println(" -> 외곽선으로 합류 (좌회전)");
      turnLeftAtIntersection(); 
    } else {
      Serial.println(" -> 외곽선 코스 유지 (직진)");
      goStraightOverIntersection();          
    }
  } 
  else if (intersectionCount == 3) { 
    lapCount++;
    intersectionCount = 0; 
    
    Serial.print(">>> 바퀴 완료! 현재 완료한 바퀴 수: ");
    Serial.println(lapCount);

    if (lapCount >= 5) {
      finishMission();
    } else {
      goStraightOverIntersection(); 
    }
  }
}

// ⬅️ 교차로에서 90도 좌회전 하는 함수
void turnLeftAtIntersection() {
  // 1. 바퀴 중심축을 선에 맞추기 위해 살짝 직진
  // (baseSpeed가 100으로 올랐으므로 직진 시간도 조금 줄입니다)
  setMotorSpeed(baseSpeed, baseSpeed);
  delay(200); // 기존 300 -> 200
  
  // 2. 왼쪽으로 제자리 회전 시작 (turnSpeed가 150으로 강력해짐)
  setMotorSpeed(-turnSpeed, turnSpeed);
  // (속도가 빨라졌으니, 선을 벗어나는 강제 탈출 시간도 줄입니다)
  delay(250); // 기존 400 -> 250 (★차가 충분히 돌지 못하면 이 숫자를 300, 350으로 늘려주세요)
  
  // 3. 왼쪽 센서가 새로운 검은선을 찾을 때까지 계속 회전
  while (digitalRead(irLeftPin) == WHITE) {
    setMotorSpeed(-turnSpeed, turnSpeed);
  }
  
  // 4. 선을 찾으면 정지
  stopMotors();
  delay(200);
}

void goStraightOverIntersection() {
  setMotorSpeed(baseSpeed, baseSpeed);
  delay(400); 
}

// ==========================================
// ⚙️ 모터 제어 유틸리티 함수 (반전됨)
// ==========================================
void setMotorSpeed(int leftPwm, int rightPwm) {
  // === 수정된 부분: 전진/후진 명령이 반대로 들어가도록 변경 ===
  
  // 왼쪽 모터 반전 적용
  if (leftPwm >= 0) {
    // 양수(전진 명령) -> 실제로는 모터를 기존의 역방향으로 굴림
    digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, leftPwm);
  } else {
    // 음수(후진 명령) -> 실제로는 모터를 기존의 정방향으로 굴림
    analogWrite(leftMotor_IA, -leftPwm); digitalWrite(leftMotor_IB, LOW);
  }
  
  // 오른쪽 모터 반전 적용
  if (rightPwm >= 0) {
    digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPwm);
  } else {
    analogWrite(rightMotor_IA, -rightPwm); digitalWrite(rightMotor_IB, LOW);
  }
}

void startMission() {
  lapCount = 0;
  intersectionCount = 0;
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