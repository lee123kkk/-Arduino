// 핀 설정
const int motor_A_IA = 10;
const int motor_A_IB = 9;
const int BUTTON_PIN = 2; // 버튼 S 핀
const int ENCODER = 3;    // 엔코더 (우노는 2, 3번만 인터럽트 가능)

// 상태 저장 변수
int motorState = 0;         // 0: 정지, 1: 1단(느림), 2: 2단(빠름)
int lastButtonState = HIGH; // 버튼의 이전 상태

// 엔코더 변수
volatile int count = 0;
unsigned long oldTime = 0;

void ISRencoder() {
  count++;
}

void setup() {
  Serial.begin(115200);
  
  pinMode(motor_A_IA, OUTPUT);
  pinMode(motor_A_IB, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 버튼 핀 풀업 설정
  pinMode(ENCODER, INPUT_PULLUP);
  
  // 엔코더 핀이 3번일 때만 작동합니다. (현재는 작동 안해도 모터 제어엔 문제없음)
  attachInterrupt(digitalPinToInterrupt(ENCODER), ISRencoder, FALLING);
  
  Serial.println("System Ready. Press the button!");
}

void loop() {
  // 1. 현재 버튼 상태 읽기
  int currentButtonState = digitalRead(BUTTON_PIN);

  // 2. 버튼이 막 눌린 순간 감지 (HIGH에서 LOW로 떨어지는 순간)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    
    // 기계적 떨림(노이즈)을 무시하기 위해 50ms 대기 (디바운싱)
    delay(50); 
    
    // 50ms 후에도 여전히 버튼이 눌려있다면(LOW), 진짜 눌린 것으로 판단
    if (digitalRead(BUTTON_PIN) == LOW) {
      
      // 상태 변경 (0 -> 1 -> 2 -> 0 순환)
      motorState++;
      if (motorState > 2) {
        motorState = 0; 
      }
      
      Serial.print("Button Pressed! Mode: ");
      Serial.println(motorState);

      // 3. 상태에 따라 모터 동작
      if (motorState == 0) { // 정지
        analogWrite(motor_A_IA, 0);
        analogWrite(motor_A_IB, 0);
        Serial.println(">> Motor STOP");
      } 
      else if (motorState == 1) { // 1단
        analogWrite(motor_A_IA, 100); // 100/255 속도
        analogWrite(motor_A_IB, 0);
        Serial.println(">> Motor LOW SPEED");
      } 
      else if (motorState == 2) { // 2단
        analogWrite(motor_A_IA, 255); // 255/255 최대 속도
        analogWrite(motor_A_IB, 0);
        Serial.println(">> Motor HIGH SPEED");
      }
    }
  }
  
  // 4. 다음 루프를 위해 현재 상태를 저장
  lastButtonState = currentButtonState; 

  // --- 엔코더 출력 (1초마다) ---
  unsigned long newTime = millis();
  if (newTime - oldTime > 1000) {
    oldTime = newTime;
    noInterrupts();
    int currentCount = count;
    interrupts();
    // Serial.print("Encoder cnt : ");
    // Serial.println(currentCount);
  }
}