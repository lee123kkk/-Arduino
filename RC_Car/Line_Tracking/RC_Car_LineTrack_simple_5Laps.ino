// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 센서 핀 (현재는 연결만 유지, 코드 사용 안 함) ---
const int encoderLeftPin = 2;
const int encoderRightPin = 3;

// --- 추가된 적외선 센서 핀 ---
const int irLeftPin = A0;   // 왼쪽 센서 OUT
const int irRightPin = A1;  // 오른쪽 센서 OUT

// --- 색상 인식 기준 ---
const int BLACK = HIGH; 
const int WHITE = LOW;

// --- 주행 설정 변수 ---
int baseSpeed = 80;   // 직진 기본 속도 (부드러운 주행을 위해 80으로 세팅)

int lapCount = 0;          // 현재 바퀴 수 (최대 5)
int intersectionCount = 0; // 누적 교차로 감지 횟수
bool isRunning = false;    
bool missionComplete = false;

// 🌟 1바퀴당 인식되는 교차로 개수 설정
// 변경: 한 바퀴에 만나는 교차로 2개(위, 시작선)를 모두 세도록 설정
const int INTERSECTIONS_PER_LAP = 2;

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(irLeftPin, INPUT);
  pinMode(irRightPin, INPUT);

  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);

  stopMotors();
  Serial.println(">>> Simple 5-Lap Mission Ready. Send 'F' to Start, 'S' to Stop.");
}

void loop() {
  // 1. 블루투스 명령 수신
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; // 대문자 변환
    if (cmd == 'F' && !isRunning && !missionComplete) startMission();
    else if (cmd == 'S') emergencyStop();
  }

  // 2. 심플 라인 트래킹 주행
  if (isRunning && !missionComplete) {
    int leftVal = digitalRead(irLeftPin);
    int rightVal = digitalRead(irRightPin);

    // [상황 1] 교차로 발견 후보 (양쪽 모두 검은선)
    if (leftVal == BLACK && rightVal == BLACK) {
      delay(50); // 45도 일반 코너 오인식 방지용 짧은 대기
      
      // 50ms 후 다시 확인
      if (digitalRead(irLeftPin) == BLACK && digitalRead(irRightPin) == BLACK) {
        handleIntersection();
      }
    }
    // [상황 2] 왼쪽으로 치우침 -> 좌회전으로 보정
    else if (leftVal == BLACK && rightVal == WHITE) {
      // 기존: setMotorSpeed(0, baseSpeed);
      // 변경: 왼쪽 바퀴를 살짝 역회전(-40)시켜 회전 반경을 날카롭게 만듦
      setMotorSpeed(-40, baseSpeed);
    }
    // [상황 3] 오른쪽으로 치우침 -> 우회전으로 보정
    else if (leftVal == WHITE && rightVal == BLACK) {
      // 기존: setMotorSpeed(baseSpeed, 0);
      // 변경: 오른쪽 바퀴를 살짝 역회전시켜 날카롭게 우회전
      setMotorSpeed(baseSpeed, -40);
    }
    // [상황 4] 양쪽 모두 흰색 -> 단순 직진 (복잡한 기억 삭제)
    else {
      setMotorSpeed(baseSpeed, baseSpeed);
    }
  }
}

// ==========================================
// 🚗 교차로 처리 함수 (외곽선 전용, 무조건 통과)
// ==========================================
void handleIntersection() {
  intersectionCount++;
  Serial.print(">>> 교차로 감지! 누적: ");
  Serial.println(intersectionCount);

  // 무조건 직진하여 교차로/지름길 입구를 무시하고 통과합니다.
  setMotorSpeed(baseSpeed, baseSpeed);
  
  // 기존: delay(400); 
  // 변경: 센서가 가로선만 살짝 넘어가도록 시간을 반 이상 뚝 줄입니다!
  delay(150); 

  // 바퀴 수 계산
  if (intersectionCount % INTERSECTIONS_PER_LAP == 0) {
    lapCount++;
    Serial.print(">>> 바퀴 완료! 현재 바퀴 수: ");
    Serial.println(lapCount);

    if (lapCount >= 5) {
      finishMission();
    }
  }
}

// ==========================================
// ⚙️ 모터 제어 유틸리티 함수 (전후진 반전 로직 유지)
// ==========================================
void setMotorSpeed(int leftPwm, int rightPwm) {
  // 왼쪽 모터
  if (leftPwm >= 0) {
    digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, leftPwm);
  } else {
    analogWrite(leftMotor_IA, -leftPwm); digitalWrite(leftMotor_IB, LOW);
  }
  
  // 오른쪽 모터
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