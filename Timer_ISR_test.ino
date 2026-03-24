/*
 * ATmega328P Timer1 Interrupt Flow Analysis Code
 * Target: Arduino Uno (16MHz Clock)
 * Goal: Visualize Main Loop vs ISR execution time using two LEDs.
 */

// 1. 하드웨어 핀 정의
const int main_loop_led = 4;   // loop() 동작 표시용 (LED A)
const int isr_active_led = 5;  // ISR 동작 표시용 (LED B)

// 2. 루프 깜빡임용 글로벌 변수
unsigned long previousLoopMillis = 0;
const long loopBlinkInterval = 500; // 500ms 간격
bool mainLoopState = LOW;

void setup() {
  // 3. LED 핀 모드 설정
  pinMode(main_loop_led, OUTPUT);
  pinMode(isr_active_led, OUTPUT);

  // 4. Timer1 레지스터 설정 (Section 16 - 16-bit Timer/Counter1)
  // CTC 모드 (Clear Timer on Compare Match)로 설정하여 1ms 주기 인터럽트 생성.

  // 설정 중 인터럽트 방지
  noInterrupts();

  // a. TCCR1A/B (Timer/Counter Control Registers) 초기화 (Section 16.11)
  TCCR1A = 0; 
  TCCR1B = 0; 
  TCNT1  = 0; // 카운터 값 초기화 (Section 16.11.2)

  /* b. OCR1A (Output Compare Register A) 값 설정 (Section 16.11.3)
   * 1ms 주기를 원하므로, 1kHz 주파수가 필요합니다.
   * 계산: (16 MHz / 64 prescaler / 1000 Hz) - 1 = 249 ticks.
   */
  OCR1A = 249; // 1ms 주기를 위한 비교값 설정

  // c. WGM (Waveform Generation Mode) 설정 (Section 16.11.1, Table 16-4)
  // Mode 4 = CTC 모드 (OCR1A 매치 시 카운터 초기화)
  // TCCR1B의 WGM12 비트를 1로 설정.
  TCCR1B |= (1 << WGM12);

  // d. CS (Clock Select) 설정 (Section 16.11.1, Table 16-5)
  // Prescaler = 64 (메인 클럭 16MHz를 64분주)
  // TCCR1B의 CS11, CS10 비트를 1로 설정.
  TCCR1B |= (1 << CS11) | (1 << CS10);

  // e. TIMSK1 (Timer/Counter Interrupt Mask Register) 설정 (Section 16.11.7)
  // OCIE1A 비트를 1로 설정하여 Compare A 매치 인터럽트 활성화.
  TIMSK1 |= (1 << OCIE1A);

  // f. 인터럽트 다시 활성화
  interrupts();

  Serial.begin(115200);
  Serial.println("Timer1 Flow Analysis System Ready.");
}

void loop() {
  // 5. 메인 루프 작업 (백그라운드 처리)
  // 이 LED는 500ms마다 깜빡이며 루프가 계속 돌고 있음을 보여줍니다.
  unsigned long currentMillis = millis();
  if (currentMillis - previousLoopMillis >= loopBlinkInterval) {
    previousLoopMillis = currentMillis;
    mainLoopState = !mainLoopState;
    digitalWrite(main_loop_led, mainLoopState); // LED 4 깜빡임
  }

  // 데이터시트 매뉴얼 분석 중이므로 시리얼 모니터로도 출력합니다.
  // Serial.println("Main Loop Running...");
  // delay(10); // 실제 프로젝트에선 delay를 피하지만, 테스트용으로 살짝 둠
}

// 6. 인터럽트 서비스 루틴 (ISR) - Vector 12 (Section 11)
// TCNT1 값이 OCR1A(249) 값과 일치할 때 자동으로 실행됩니다.
ISR(TIMER1_COMPA_vect) {
  // 흐름 분석 포인트:
  // ISR이 시작되자마자 분석용 LED를 켭니다. 
  // 가장 빠르고 정확한 반응을 위해 직접 포트 조작(Direct Port Manipulation)을 사용합니다.
  // 5번 핀(PD5)을 HIGH로 설정.
  PORTD |= (1 << PD5); 

  // --- 여기에 인터럽트에서 처리할 고우선순위 주가 작업을 넣습니다 ---
  // (예: 정밀 센서 읽기, 모터 스텝 계산 등)

  // 시각화 연습: ISR의 실행 시간을 늘려봅니다.
  // (아래 주석을 해제하여 더미 로드를 주면 LED B가 더 밝게 보입니다.)
  // for(volatile int i=0; i<500; i++);

  // 흐름 분석 포인트:
  // ISR의 모든 작업이 끝나고 복귀하기 직전에 분석용 LED를 끕니다.
  // 5번 핀(PD5)을 LOW로 설정.
  PORTD &= ~(1 << PD5); 
}