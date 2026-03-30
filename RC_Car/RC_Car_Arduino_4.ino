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
// ⚙️ 1. 초기 밸런스 및 속도 설정 (정밀 튜닝 적용)
// ========================================================
// 직진/후진 초기 밸런스
const int LEFT_BASE_SPEED = 200;  
// 175는 너무 느렸으므로 190으로 상향 조정하여 양쪽 균형을 맞춥니다.
const int RIGHT_BASE_SPEED = 190; 

// 회전 속도 제한 
const int SPIN_TURN_SPEED = 150;     // 제자리 좌/우회전 속도
const int WIDE_TURN_FAST = 160;      // 크게 돌 때 바깥쪽 바퀴 속도
const int WIDE_TURN_SLOW = 80;       // 크게 돌 때 안쪽 바퀴 속도
// ========================================================

int leftPWM = LEFT_BASE_SPEED;
int rightPWM = RIGHT_BASE_SPEED;

float Kp = 1.5; // 직진 보정 강도
char currentCommand = 'S'; // 현재 주행 상태

unsigned long prevBalanceMillis = 0;
unsigned long prevPrintMillis = 0;

void setup() {
  delay(2000); // ESP32 부팅 대기
  Serial.begin(9600);

  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  pinMode(encoderLeftPin, INPUT_PULLUP); pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  stopMotors();
}

void loop() {
  if (Serial.available() > 0) {
    char incoming = Serial.read();
    
    if (incoming >= 'a' && incoming <= 'z') incoming -= 32; 
    
    if (incoming == 'F' || incoming == 'B' || incoming == 'L' || incoming == 'R' || 
        incoming == 'C' || incoming == 'V' || incoming == 'S') {
      
      if (incoming != currentCommand) {
        leftCount = 0;
        rightCount = 0;
        
        if (incoming == 'F' || incoming == 'B') {
          leftPWM = LEFT_BASE_SPEED;
          rightPWM = RIGHT_BASE_SPEED;
        }
        
        currentCommand = incoming;
        applyMotorControl(); 
      }
    }
  }

  unsigned long currentMillis = millis();

  // [자동 속도 보정]
  if ((currentCommand == 'F' || currentCommand == 'B') && (currentMillis - prevBalanceMillis >= 50)) {
    prevBalanceMillis = currentMillis;

    long error = leftCount - rightCount; 
    
    rightPWM = RIGHT_BASE_SPEED + (error * Kp);

    if (rightPWM > 255) rightPWM = 255;
    if (rightPWM < 80) rightPWM = 80; 

    applyMotorControl(); 
  }

  // 상태 출력
  if (currentMillis - prevPrintMillis >= 1000) {
    prevPrintMillis = currentMillis;
    Serial.print("L_Cnt:"); Serial.print(leftCount);
    Serial.print(" (PWM:"); Serial.print(leftPWM);
    Serial.print(") | R_Cnt:"); Serial.print(rightCount);
    Serial.print(" (PWM:"); Serial.print(rightPWM);
    Serial.println(")");
  }
}

void countLeft() { leftCount++; }
void countRight() { rightCount++; }

void applyMotorControl() {
  switch (currentCommand) {
    case 'F': 
      digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, leftPWM);
      digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, rightPWM);
      break;
    case 'B': 
      analogWrite(leftMotor_IA, leftPWM); digitalWrite(leftMotor_IB, LOW);
      analogWrite(rightMotor_IA, rightPWM); digitalWrite(rightMotor_IB, LOW);
      break;
    case 'L': 
      analogWrite(leftMotor_IA, SPIN_TURN_SPEED); digitalWrite(leftMotor_IB, LOW);
      digitalWrite(rightMotor_IA, LOW);           analogWrite(rightMotor_IB, SPIN_TURN_SPEED);
      break;
    case 'R': 
      digitalWrite(leftMotor_IA, LOW);            analogWrite(leftMotor_IB, SPIN_TURN_SPEED);
      analogWrite(rightMotor_IA, SPIN_TURN_SPEED); digitalWrite(rightMotor_IB, LOW);
      break;
    case 'C': 
      digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, WIDE_TURN_SLOW);
      digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, WIDE_TURN_FAST);
      break;
    case 'V': 
      digitalWrite(leftMotor_IA, LOW);  analogWrite(leftMotor_IB, WIDE_TURN_FAST);
      digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, WIDE_TURN_SLOW);
      break;
    case 'S': 
    default:
      stopMotors();
      break;
  }
}

void stopMotors() {
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);
}