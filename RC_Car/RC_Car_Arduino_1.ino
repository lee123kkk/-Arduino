// --- 모터 드라이버(L9110S) 핀 설정 ---
// 왼쪽 모터 (Motor A)
const int motorA_IA = 5; // 전진 핀
const int motorA_IB = 6; // 후진 핀

// 오른쪽 모터 (Motor B)
const int motorB_IA = 9; // 전진 핀
const int motorB_IB = 10;// 후진 핀

void setup() {
  // ESP32와의 통신 시작 (속도 9600)
  Serial.begin(9600); 

  // 모터 제어 핀을 출력(OUTPUT) 모드로 설정
  pinMode(motorA_IA, OUTPUT);
  pinMode(motorA_IB, OUTPUT);
  pinMode(motorB_IA, OUTPUT);
  pinMode(motorB_IB, OUTPUT);

  // 시작할 때 모터 정지 상태 유지
  stopMotors();
}

void loop() {
  // ESP32(스마트폰)로부터 명령 데이터가 들어왔는지 확인
  if (Serial.available() > 0) {
    char command = Serial.read(); // 한 글자 읽기

    // 수신된 문자에 따라 동작 수행
    switch (command) {
      case 'F': // Forward (전진)
      case 'f':
        moveForward();
        break;
      case 'B': // Backward (후진)
      case 'b':
        moveBackward();
        break;
      case 'L': // Left (좌회전)
      case 'l':
        turnLeft();
        break;
      case 'R': // Right (우회전)
      case 'r':
        turnRight();
        break;
      case 'S': // Stop (정지)
      case 's':
        stopMotors();
        break;
    }
  }
}

// --- 모터 제어 함수 ---
// 주의: 만약 전진 명령을 내렸는데 뒤로 간다면 HIGH/LOW를 반대로 바꾸거나 모터 배선을 바꾸면 됩니다.

void moveForward() {
  // 양쪽 모터 모두 전진
  digitalWrite(motorA_IA, HIGH); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, HIGH); digitalWrite(motorB_IB, LOW);
}

void moveBackward() {
  // 양쪽 모터 모두 후진
  digitalWrite(motorA_IA, LOW); digitalWrite(motorA_IB, HIGH);
  digitalWrite(motorB_IA, LOW); digitalWrite(motorB_IB, HIGH);
}

void turnLeft() {
  // 왼쪽 모터는 후진, 오른쪽 모터는 전진 (제자리 좌회전)
  digitalWrite(motorA_IA, LOW); digitalWrite(motorA_IB, HIGH);
  digitalWrite(motorB_IA, HIGH); digitalWrite(motorB_IB, LOW);
}

void turnRight() {
  // 왼쪽 모터는 전진, 오른쪽 모터는 후진 (제자리 우회전)
  digitalWrite(motorA_IA, HIGH); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, LOW); digitalWrite(motorB_IB, HIGH);
}

void stopMotors() {
  // 양쪽 모터 정지
  digitalWrite(motorA_IA, LOW); digitalWrite(motorA_IB, LOW);
  digitalWrite(motorB_IA, LOW); digitalWrite(motorB_IB, LOW);
}