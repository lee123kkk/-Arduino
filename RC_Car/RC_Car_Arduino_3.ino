// --- 모터 드라이버 제어 핀 (PWM 지원 핀) ---
const int motorA_IA = 5; // 왼쪽 모터 핀 1
const int motorA_IB = 6; // 왼쪽 모터 핀 2
const int motorB_IA = 9; // 오른쪽 모터 핀 1
const int motorB_IB = 10;// 오른쪽 모터 핀 2

// --- 엔코더 센서 핀 ---
const int encoderLeftPin = 2;  // 왼쪽 엔코더
const int encoderRightPin = 3; // 오른쪽 엔코더

// --- 엔코더 카운트 변수 ---
volatile long leftCount = 0;
volatile long rightCount = 0;

// --- 속도 제어 변수 (PWM) ---
int BASE_SPEED = 200; // 기준 속도 (조금 더 느린 왼쪽 바퀴를 기준으로 고정)
int leftPWM = 200;
int rightPWM = 200;

// ★ 비례 제어(P 제어) 상수: 숫자가 클수록 오차를 더 강하게 교정합니다. (1.0 ~ 3.0 사이 추천)
float Kp = 1.8; 

char currentCommand = 'S'; // 현재 주행 상태 저장

// 데이터 전송 및 제어 타이머
unsigned long prevBalanceMillis = 0;
unsigned long prevPrintMillis = 0;

void setup() {
  Serial.begin(9600); // ESP32와 통신

  // 모터 핀 출력 설정
  pinMode(motorA_IA, OUTPUT);
  pinMode(motorA_IB, OUTPUT);
  pinMode(motorB_IA, OUTPUT);
  pinMode(motorB_IB, OUTPUT);
  
  // 엔코더 핀 입력 설정 (내장 풀업 저항)
  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);

  // 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();
}

void loop() {
  // 1. ESP32로부터 명령 수신
  if (Serial.available() > 0) {
    char incoming = Serial.read();
    
    // 소문자가 들어와도 대문자로 변환하여 처리
    if (incoming >= 'a' && incoming <= 'z') incoming -= 32; 
    
    // 유효한 명령일 경우에만 처리
    if (incoming == 'F' || incoming == 'B' || incoming == 'L' || incoming == 'R' || incoming == 'S') {
      // 새로운 명령이 들어왔을 때만 카운트와 속도 초기화
      if (incoming != currentCommand) {
        leftCount = 0;
        rightCount = 0;
        leftPWM = BASE_SPEED;
        rightPWM = BASE_SPEED;
        currentCommand = incoming;
        applyMotorControl(); // 모터 동작 실행
      }
    }
  }

  unsigned long currentMillis = millis();

  // 2. [자동 속도 보정] 50ms마다 직진(F) 또는 후진(B) 상태일 때만 실행
  if ((currentCommand == 'F' || currentCommand == 'B') && (currentMillis - prevBalanceMillis >= 50)) {
    prevBalanceMillis = currentMillis;

    // 양쪽 바퀴의 회전수 차이 계산 (오차)
    long error = leftCount - rightCount; 
    
    // 오른쪽 모터 속도 자동 조절 
    // (오른쪽이 더 많이 돌았으면 error가 음수(-)이므로 rightPWM이 BASE_SPEED보다 작아짐)
    rightPWM = BASE_SPEED + (error * Kp);

    // PWM 값 상하한선 제한 (0~255)
    if (rightPWM > 255) rightPWM = 255;
    if (rightPWM < 80) rightPWM = 80; // 전압이 너무 낮아져서 모터가 아예 멈추는 것 방지

    applyMotorControl(); // 보정된 PWM 즉시 적용
  }

  // 3. 1초마다 스마트폰으로 상태 전송 (카운트와 현재 PWM 확인용)
  if (currentMillis - prevPrintMillis >= 1000) {
    prevPrintMillis = currentMillis;
    Serial.print("L_Cnt:"); Serial.print(leftCount);
    Serial.print(" (PWM:"); Serial.print(leftPWM);
    Serial.print(") | R_Cnt:"); Serial.print(rightCount);
    Serial.print(" (PWM:"); Serial.print(rightPWM);
    Serial.println(")");
  }
}

// --- 인터럽트 함수 ---
void countLeft() { leftCount++; }
void countRight() { rightCount++; }

// --- 모터 제어 적용 함수 ---
void applyMotorControl() {
  switch (currentCommand) {
    case 'F': // 전진 (방향 반대 현상 수정 유지)
      digitalWrite(motorA_IA, LOW);  analogWrite(motorA_IB, leftPWM);
      digitalWrite(motorB_IA, LOW);  analogWrite(motorB_IB, rightPWM);
      break;
    case 'B': // 후진
      analogWrite(motorA_IA, leftPWM); digitalWrite(motorA_IB, LOW);
      analogWrite(motorB_IA, rightPWM); digitalWrite(motorB_IB, LOW);
      break;
    case 'L': // 제자리 좌회전 (회전 시에는 밸런싱 없이 기본 속도로 구동)
      analogWrite(motorA_IA, BASE_SPEED); digitalWrite(motorA_IB, LOW);
      digitalWrite(motorB_IA, LOW);       analogWrite(motorB_IB, BASE_SPEED);
      break;
    case 'R': // 제자리 우회전
      digitalWrite(motorA_IA, LOW);       analogWrite(motorA_IB, BASE_SPEED);
      analogWrite(motorB_IA, BASE_SPEED); digitalWrite(motorB_IB, LOW);
      break;
    case 'S': // 정지
    default:
      stopMotors();
      break;
  }
}

void stopMotors() {
  digitalWrite(motorA_IA, LOW); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, LOW); digitalWrite(motorB_IB, LOW);
}