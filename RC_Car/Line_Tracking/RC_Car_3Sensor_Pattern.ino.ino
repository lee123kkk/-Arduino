// --- 모터 드라이버 제어 핀 ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 적외선 센서 핀 ---
const int irLeftPin = A0;   // 왼쪽 보조 센서
const int irRightPin = A1;  // 오른쪽 보조 센서
const int irCenterPin = A2; // 가운데 메인 센서

// --- 색상 인식 기준 (센서별 분리) ---
const int SIDE_BLACK = HIGH; 
const int SIDE_WHITE = LOW;
const int CENTER_BLACK = HIGH; 
const int CENTER_WHITE = LOW;

// --- 주행 설정 변수 ---
int baseSpeed = 80;   // 직진 기본 속도
int turnSpeed = 100;   // ★ 제자리 회전(Pivot) 전용 속도

const int INTERSECTIONS_PER_LAP = 1; 

int lapCount = 0;          
int intersectionCount = 0;
bool isRunning = false;    
bool missionComplete = false;
int lastState = 0; // 0:직진, 1:좌회전 중, 2:우회전 중

void setup() {
  Serial.begin(9600);
  
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(irLeftPin, INPUT);
  pinMode(irRightPin, INPUT);
  pinMode(irCenterPin, INPUT);

  stopMotors();
  Serial.println(">>> 3-Sensor Pivot Turn Logic Ready. Send 'F' to Start.");
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
    int centerVal = digitalRead(irCenterPin);
    int rightVal = digitalRead(irRightPin);

    // [상황 1] 세 센서 모두 검은색 (출발선 또는 십자 교차로)
    if (leftVal == SIDE_BLACK && centerVal == CENTER_BLACK && rightVal == SIDE_BLACK) {
      intersectionCount++;
      Serial.print(">>> ➕ 가로선 감지! 누적: ");
      Serial.println(intersectionCount);

      if (intersectionCount % INTERSECTIONS_PER_LAP == 0) {
        lapCount++;
        Serial.print(">>> 🏁 바퀴 완료! 현재 바퀴 수: ");
        Serial.println(lapCount);

        if (lapCount >= 5) {
          finishMission();
          return;
        }
      }
      
      setMotorSpeed(baseSpeed, baseSpeed);
      delay(300); 
    }
    // [상황 2] 가운데 센서가 선을 밟고 있음 -> ★무조건 직진★
    else if (centerVal == CENTER_BLACK) {
      setMotorSpeed(baseSpeed, baseSpeed);
      lastState = 0;
    }
    // [상황 3] 가운데 놓치고 왼쪽만 밟음 -> ★제자리 좌회전 집중★
    else if (leftVal == SIDE_BLACK && centerVal == CENTER_WHITE) {
      // 전진 없이, 왼쪽 바퀴는 뒤로 오른쪽 바퀴는 앞으로 굴려 제자리 회전
      setMotorSpeed(-turnSpeed, turnSpeed); 
      lastState = 1;
    }
    // [상황 4] 가운데 놓치고 오른쪽만 밟음 -> ★제자리 우회전 집중★
    else if (rightVal == SIDE_BLACK && centerVal == CENTER_WHITE) {
      // 전진 없이, 왼쪽 바퀴는 앞으로 오른쪽 바퀴는 뒤로 굴려 제자리 회전
      setMotorSpeed(turnSpeed, -turnSpeed); 
      lastState = 2;
    }
    // [상황 5] 선을 완전히 이탈함 (세 센서 모두 흰색)
    else {
      // 마지막 기억을 바탕으로 가운데 센서가 선을 찾을 때까지 제자리 회전 반복
      if (lastState == 1) setMotorSpeed(-turnSpeed, turnSpeed);
      else if (lastState == 2) setMotorSpeed(turnSpeed, -turnSpeed);
      else setMotorSpeed(baseSpeed, baseSpeed);
    }
  }
}

// ==========================================
// ⚙️ 모터 제어 (원상복구된 후륜 구동 방향)
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
  lapCount = 0; intersectionCount = 0; lastState = 0;
  missionComplete = false; isRunning = true;
  Serial.println("---------- MISSION START ----------");
}

void emergencyStop() {
  stopMotors(); isRunning = false;
  Serial.println("!!! STOP !!!");
}

void finishMission() {
  stopMotors(); isRunning = false; missionComplete = true;
  Serial.println("---------- COMPLETE ----------");
}

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}