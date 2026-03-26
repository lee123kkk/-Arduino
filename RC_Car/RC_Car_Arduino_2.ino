// --- 모터 드라이버 제어 핀 (PWM 지원 핀: 5, 6, 9, 10) ---
const int motorA_IA = 5; // 왼쪽 모터 핀 1
const int motorA_IB = 6; // 왼쪽 모터 핀 2
const int motorB_IA = 9; // 오른쪽 모터 핀 1
const int motorB_IB = 10;// 오른쪽 모터 핀 2

// --- 모터 최대 속도 설정 (0 ~ 255) ---
// 255의 80% = 약 204
const int MAX_SPEED = 204; 

// --- 엔코더 센서 핀 ---
const int encoderLeftPin = 2;  // 왼쪽 엔코더
const int encoderRightPin = 3; // 오른쪽 엔코더

// --- 엔코더 카운트 변수 ---
volatile long leftCount = 0;
volatile long rightCount = 0;

// 데이터 전송 타이머 변수
unsigned long previousMillis = 0;
const long interval = 1000; // 1초(1000ms)마다 전송

void setup() {
  Serial.begin(9600); // ESP32와 통신

  // 모터 핀 설정
  pinMode(motorA_IA, OUTPUT);
  pinMode(motorA_IB, OUTPUT);
  pinMode(motorB_IA, OUTPUT);
  pinMode(motorB_IB, OUTPUT);
  stopMotors();

  // 엔코더 핀 설정 (내장 풀업 저항 사용)
  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT_PULLUP);

  // 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);
}

void loop() {
  // 1. ESP32로부터 명령 수신 및 모터 동작
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'F': case 'f': moveForward(); break;
      case 'B': case 'b': moveBackward(); break;
      case 'L': case 'l': turnLeft(); break;
      case 'R': case 'r': turnRight(); break;
      case 'S': case 's': stopMotors(); break;
    }
  }

  // 2. 1초마다 핸드폰으로 엔코더 카운트 전송
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    Serial.print("Left: ");
    Serial.print(leftCount);
    Serial.print(" | Right: ");
    Serial.println(rightCount);
  }
}

// --- 인터럽트 함수 ---
void countLeft() {
  leftCount++;
}

void countRight() {
  rightCount++;
}

// --- 모터 제어 함수 (PWM 적용) ---
// 아날로그 출력(analogWrite)을 사용하여 속도를 제어합니다.

void moveForward() {
  digitalWrite(motorA_IA, LOW);  analogWrite(motorA_IB, MAX_SPEED);
  digitalWrite(motorB_IA, LOW);  analogWrite(motorB_IB, MAX_SPEED);
}

void moveBackward() {
  analogWrite(motorA_IA, MAX_SPEED); digitalWrite(motorA_IB, LOW);
  analogWrite(motorB_IA, MAX_SPEED); digitalWrite(motorB_IB, LOW);
}

void turnLeft() {
  analogWrite(motorA_IA, MAX_SPEED); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, LOW);      analogWrite(motorB_IB, MAX_SPEED);
}

void turnRight() {
  digitalWrite(motorA_IA, LOW);      analogWrite(motorA_IB, MAX_SPEED);
  analogWrite(motorB_IA, MAX_SPEED); digitalWrite(motorB_IB, LOW);
}

void stopMotors() {
  digitalWrite(motorA_IA, LOW); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, LOW); digitalWrite(motorB_IB, LOW);
}