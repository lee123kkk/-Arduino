// --- 핀 설정 ---
const int ENCODER_CLK = 2; // 인터럽트 핀 (회전 감지)
const int ENCODER_DT  = 3; // 회전 방향 감지
const int ENCODER_SW  = 5; // 엔코더 버튼
const int SHOCK_PIN   = 4; // 충격 센서 신호

// RGB LED 핀 설정 (PWM 가능 핀)
const int LED_R = 9;
const int LED_G = 10;
const int LED_B = 11;

// --- 시스템 상태 변수 ---
// 0: 정상 가동, 1: 비상 정지, 2: 복구 대기
int systemState = 0; 
volatile int encoderCount = 0; // 인터럽트 안에서 변경되는 변수는 volatile 선언
int lastEncoderCount = 0;

// LED 깜빡임 제어용
unsigned long previousMillis = 0;
bool isLedOn = false;

// --- 추가된 변수 (디바운싱용) ---
unsigned long buttonDebounceTime = 0; // 버튼 디바운싱 타이머
int lastButtonState = HIGH;          // 이전 버튼 상태

void setup() {
  Serial.begin(9600);

  // 핀 모드 설정
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP); // 내부 풀업 저항 사용
  pinMode(SHOCK_PIN, INPUT); // 원상 복구


  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // 로터리 엔코더 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, FALLING);

  Serial.println("=== AGV 시스템 가동 시작 ===");
  Serial.println("[상태 0] 정상 주행 중...");
  setLED(0, 255, 0); // 초기 색상: 녹색
}

void loop() {
  unsigned long currentMillis = millis();

  // --------------------------------------------------
  // [상태 0] 정상 가동 상태
  // --------------------------------------------------
  if (systemState == 0) {
    setLED(0, 255, 0); // 녹색 고정
    Serial.println("STATUS:0"); // <-- 이 줄을 추가하세요! 웹페이지용 신호입니다.

    // [핵심 수정] 충격 감지 신호를 HIGH로 변경!
    if (digitalRead(SHOCK_PIN) == HIGH) { 
      delay(15); // 노이즈 방지
      if (digitalRead(SHOCK_PIN) == HIGH) { // 재확인도 HIGH로 변경!
        systemState = 1; // 상태를 비상 정지로 변경
        Serial.println("\n! 경고 ! 충돌 감지됨. 시스템 비상 정지.");
        setLED(255, 0, 0); // 즉시 빨간색 켜기
        
      }
    }
  }

  // --------------------------------------------------
  // [상태 1] 비상 정지 상태 (충격 감지됨)
  // --------------------------------------------------
  else if (systemState == 1) {
    // 빨간색 LED 깜빡임 (0.5초 간격)
    Serial.println("STATUS:1"); // <-- 이 줄을 추가하세요!

    if (currentMillis - previousMillis >= 500) {
      previousMillis = currentMillis;
      isLedOn = !isLedOn;
      if (isLedOn) setLED(255, 0, 0);
      else setLED(0, 0, 0);
    }

    // --- [수정 1] 엔코더 회전 동작에 의한 버튼 오인 해결 ---
    // 엔코더 손잡이를 돌릴 때 발생하는 노이즈가 버튼 핀을 건드려도,
    // 만약 엔코더가 돌아가는 중(카운트가 변함)이라면 버튼 입력을 무시합니다.
    if (encoderCount == lastEncoderCount) {
      
      // 복구 1단계: 관리자가 엔코더 버튼을 누름
      if (digitalRead(ENCODER_SW) == LOW) {
        // 노이즈가 아닌 진짜 버튼 누름인지 더 오랫동안 확인합니다. (50 -> 100ms)
        delay(100); 
        if (digitalRead(ENCODER_SW) == LOW) {
          systemState = 2; // 복구 대기 상태로 변경
          encoderCount = 0; // 회전 카운트 초기화
          Serial.println("관리자 개입 확인. 안전 인터락 해제 대기 중...");
          Serial.println("-> 다이얼을 시계 방향으로 5칸 돌리세요.");
          setLED(255, 100, 0); // 노란색(주황색)으로 변경
          
          while(digitalRead(ENCODER_SW) == LOW); // 버튼에서 손을 뗄 때까지 대기
        }
      }
    }
  }

  // --------------------------------------------------
  // [상태 2] 복구 대기 상태 (수동 잠금 해제 중)
  // --------------------------------------------------
  else if (systemState == 2) {
    setLED(255, 100, 0); // 노란색 고정
    Serial.println("STATUS:2"); // <-- 이 줄을 추가하세요!

    
    // 카운트가 변했을 때만 시리얼 모니터에 출력
    if (encoderCount != lastEncoderCount) {
      Serial.print("잠금 해제 진행도: ");
      Serial.print(encoderCount);
      Serial.println(" / 5");
      // lastEncoderCount 업데이트는 loop 마지막으로 이동
    }

    // 복구 2단계: 시계 방향으로 5칸 이상 회전하면 정상 상태로 복구
    if (encoderCount >= 5) {
      Serial.println("\n=== 잠금 해제 완료. 시스템 정상 가동 복구 ===");
      setLED(0, 255, 0); // 초록색 켜기
      
      // [핵심 해결책] 진동 무시 대기 시간
      // 조작을 마치고 손을 떼면서 발생하는 브레드보드의 진동,
      // 그리고 KY-002 내부 스프링이 흔들리다가 멈출 때까지 충분히 기다립니다.
      delay(1500); // 1.5초 대기 (이 시간 동안은 센서 입력을 받지 않음)
      
      systemState = 0;  // 대기 시간이 끝난 후 완전한 정상 상태(0)로 전환
      encoderCount = 0; // 다음 비상 상황을 위해 카운트 미리 초기화
    }
    // 실수로 반대로 돌려도 음수로 내려가지 않게 막음
    else if (encoderCount < 0) {
      encoderCount = 0; 
    }
  }
  
  // 다음 루프에서 비교하기 위해 마지막 엔코더 카운트 저장 (루프 맨 마지막으로 이동)
  lastEncoderCount = encoderCount;
}

// ==========================================
// 인터럽트 서비스 루틴 (ISR): 로터리 엔코더 값 읽기
// ==========================================
void readEncoder() {
  if (digitalRead(ENCODER_DT) == HIGH) {
    encoderCount++; // 시계 방향 회전
  } else {
    encoderCount--; // 시계 반대 방향 회전
  }
}

// ==========================================
// RGB LED 제어 편의 함수
// ==========================================
void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}