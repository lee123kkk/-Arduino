// 차량용 LED 핀
const int car_R = 3;
const int car_G = 5;
const int car_B = 6;

// 보행자용 LED 핀
const int ped_R = 9;
const int ped_G = 10;

void setup() {
  pinMode(car_R, OUTPUT); pinMode(car_G, OUTPUT); pinMode(car_B, OUTPUT);
  pinMode(ped_R, OUTPUT); pinMode(ped_G, OUTPUT);
}

void loop() {
  // --- 1단계: 차량 주행 (차량: 초록 / 보행자: 빨강) ---
  setCarColor(0, 255, 0); // 초록
  setPedColor(255, 0);   // 빨강
  delay(5000);           // 5초 유지

  // --- 2단계: 차량 정지 준비 (차량: 노랑 / 보행자: 빨강) ---
  setCarColor(255, 100, 0); // 노란색 (R+G 조합)
  setPedColor(255, 0);      // 빨강
  delay(2000);              // 2초 유지

  // --- 3단계: 보행자 횡단 (차량: 빨강 / 보행자: 초록) ---
  setCarColor(255, 0, 0); // 빨강
  setPedColor(0, 255);   // 초록
  delay(5000);           // 5초 유지

  // --- 4단계: 보행자 경고 (차량: 빨강 / 보행자: 초록 깜빡임) ---
  for(int i=0; i<5; i++) {
    setPedColor(0, 255); // 초록 켬
    delay(300);
    setPedColor(0, 0);   // 끔
    delay(300);
  }

  // --- 5단계: 전체 정지 (차량: 빨강 / 보행자: 빨강) ---
  setCarColor(255, 0, 0); 
  setPedColor(255, 0);
  delay(1000);           // 안전을 위한 1초 버퍼
}

// 차량용 RGB 제어 함수
void setCarColor(int r, int g, int b) {
  analogWrite(car_R, r);
  analogWrite(car_G, g);
  analogWrite(car_B, b);
}

// 보행자용 RGB 제어 함수 (Blue는 생략)
void setPedColor(int r, int g) {
  analogWrite(ped_R, r);
  analogWrite(ped_G, g);
}