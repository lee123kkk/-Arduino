int A_IA = 10;
int A_IB = 9;
int ENCODER = 2; // UNO에서는 2번 또는 3번 핀을 사용해야 합니다.
volatile int count = 0;
unsigned long oldTime = 0; // millis()는 unsigned long이므로 타입을 맞추는 것이 좋습니다.
unsigned long newTime = 0;

void ISRencoder(){
  count++;
}

void setup() {
  Serial.begin(115200);
  pinMode(A_IA, OUTPUT);
  pinMode(A_IB, OUTPUT);
  pinMode(ENCODER, INPUT_PULLUP);
  
  // digitalPinToInterrupt를 사용하여 안전하게 설정합니다.
  attachInterrupt(digitalPinToInterrupt(ENCODER), ISRencoder, FALLING);
}

void loop() {
  if(Serial.available() > 0){
    char c = Serial.read();
    if(c == 'f'){
      analogWrite(A_IA, 128);
      analogWrite(A_IB, 0);
    }
    else if (c == 's'){
      analogWrite(A_IA, 0);
      analogWrite(A_IB, 0);
    }
  }

  newTime = millis();
  if(newTime - oldTime > 1000){
    oldTime = newTime;
    
    // count 값을 읽을 때만 잠시 인터럽트를 중지했다가 다시 켭니다.
    noInterrupts();
    int currentCount = count;
    count = 0; // 1초마다 초기화하고 싶다면 여기서 0으로 만듭니다.
    interrupts();
    
    Serial.print("cnt : ");
    Serial.println(currentCount);
  }
}
