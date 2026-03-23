// 핀 설정 (이미지 기준)
const int motor_A = 10; 
const int motor_B = 9;

void setup() {
  pinMode(motor_A, OUTPUT);
  pinMode(motor_B, OUTPUT);
  Serial.begin(115200);
  Serial.println("Motor Test Start!");
}

void loop() {
  // 1. 전진 (속도 150/255)
  Serial.println("Forward...");
  analogWrite(motor_A, 150);
  analogWrite(motor_B, 0);
  delay(2000);

  // 2. 정지
  Serial.println("Stop");
  analogWrite(motor_A, 0);
  analogWrite(motor_B, 0);
  delay(1000);

  // 3. 후진 (속도 150/255)
  Serial.println("Backward...");
  analogWrite(motor_A, 0);
  analogWrite(motor_B, 150);
  delay(2000);

  // 4. 정지
  Serial.println("Stop");
  analogWrite(motor_A, 0);
  analogWrite(motor_B, 0);
  delay(1000);
}