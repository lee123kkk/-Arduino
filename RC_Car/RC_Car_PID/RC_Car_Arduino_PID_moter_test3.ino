// --- 모터 핀 (4개 모두 필수!) ---
const int leftMotor_IA = 9; 
const int leftMotor_IB = 10;
const int rightMotor_IA = 6; 
const int rightMotor_IB = 5;

// --- 엔코더 핀 (바꾼 상태 유지) ---
const int encoderLeftPin = 3;  
const int encoderRightPin = 2; 

volatile long leftCount = 0;
volatile long rightCount = 0;

void setup() {
  Serial.begin(9600);
  
  // 핀 모드 설정
  pinMode(leftMotor_IA, OUTPUT); pinMode(leftMotor_IB, OUTPUT);
  pinMode(rightMotor_IA, OUTPUT); pinMode(rightMotor_IB, OUTPUT);
  
  // ★가장 중요한 초기화: 모든 모터를 확실하게 끕니다 (플로팅 방지)
  digitalWrite(leftMotor_IA, LOW); digitalWrite(leftMotor_IB, LOW);
  digitalWrite(rightMotor_IA, LOW); digitalWrite(rightMotor_IB, LOW);

  pinMode(encoderLeftPin, INPUT_PULLUP); pinMode(encoderRightPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRight, RISING);

  delay(3000); // 3초 대기 후 시작
  Serial.println("--- 🤖 하드웨어 정밀 진단 시작 ---");

  // 1. 왼쪽 모터만 2초간 회전
  Serial.println(">>> 1. 왼쪽(Left) 모터만 돌립니다.");
  digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, 150); 
  digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, 0);
  delay(2000);
  // 정지
  digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, 0); 
  
  // 결과 출력
  Serial.print("물리적으로 어느 바퀴가 돌았나요? | ");
  Serial.print("L_카운트: "); Serial.print(leftCount);
  Serial.print(" / R_카운트: "); Serial.println(rightCount);
  Serial.println("---------------------------------");
  
  leftCount = 0; rightCount = 0; // 카운트 초기화
  delay(2000);

  // 2. 오른쪽 모터만 2초간 회전
  Serial.println(">>> 2. 오른쪽(Right) 모터만 돌립니다.");
  digitalWrite(leftMotor_IA, LOW); analogWrite(leftMotor_IB, 0); 
  digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, 150);
  delay(2000);
  // 정지
  digitalWrite(rightMotor_IA, LOW); analogWrite(rightMotor_IB, 0);

  // 결과 출력
  Serial.print("물리적으로 어느 바퀴가 돌았나요? | ");
  Serial.print("L_카운트: "); Serial.print(leftCount);
  Serial.print(" / R_카운트: "); Serial.println(rightCount);
  Serial.println("--- 진단 종료 ---");
}

void loop() { }

void countLeft() { leftCount++; }
void countRight() { rightCount++; }