#include "BluetoothSerial.h"

// 블루투스 객체 생성
BluetoothSerial SerialBT;

// ESP32와 아두이노 간의 통신을 위해 Serial2(하드웨어 시리얼) 사용 권장
// (주의: ESP32의 RX 핀을 GPIO 16, TX 핀을 GPIO 17에 연결해야 합니다.)
#define RXD2 16
#define TXD2 17

void setup() {
  // PC 시리얼 모니터용 (디버깅)
  Serial.begin(115200);
  
  // 아두이노와의 통신용 (속도 9600)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // 블루투스 이름 설정 (스마트폰에서 이 이름으로 검색됩니다)
  SerialBT.begin("RC_CAR_ESP32"); 
  Serial.println("블루투스가 시작되었습니다. 스마트폰에서 페어링해주세요!");
}

void loop() {
  // 1. 스마트폰(블루투스)에서 명령이 들어오면
  if (SerialBT.available()) {
    char cmd = SerialBT.read(); // 명령 읽기
    Serial2.write(cmd);         // 아두이노로 전달
    
    // 스마트폰에서 어떤 명령이 왔는지 PC 시리얼 모니터로 확인
    Serial.print("받은 명령: "); 
    Serial.println(cmd);
  }

  // 2. (선택사항) 아두이노에서 ESP32로 보내는 응답이 있다면 스마트폰으로 전달
  if (Serial2.available()) {
    char response = Serial2.read();
    SerialBT.write(response);
  }
}