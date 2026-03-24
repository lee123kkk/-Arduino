/*
 * 프로젝트: ESP32 + VL53L0X 레이저 거리 측정
 * 통신 방식: I2C (SDA=21, SCL=22)
 */

#include "Adafruit_VL53L0X.h"

// 센서 객체 생성
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Serial.begin(115200);
  
  // 시리얼 모니터가 열릴 때까지 잠시 대기
  while (!Serial) { delay(1); }

  Serial.println("VL53L0X 센서 부팅 시작...");

  // lox.begin() 함수가 내부적으로 I2C 통신을 시작하고 센서를 초기화합니다.
  if (!lox.begin()) {
    Serial.println("오류: VL53L0X 센서를 찾을 수 없습니다. 선 연결을 확인하세요!");
    while(1); // 무한 루프에 빠져서 더 이상 진행하지 않음
  }
  
  Serial.println("센서 부팅 성공! 거리 측정을 시작합니다.\n"); 
}

void loop() {
  // 측정 데이터를 담을 구조체(변수 묶음) 생성
  VL53L0X_RangingMeasurementData_t measure;
  
  // 센서에게 거리를 측정해서 measure 변수에 담으라고 명령
  // false: 디버그용 추가 정보 출력 안 함
  lox.rangingTest(&measure, false); 

  // RangeStatus가 4이면 센서 측정 범위를 벗어났다는 뜻입니다. (약 1.2m ~ 2m 이상)
  if (measure.RangeStatus != 4) {  
    Serial.print("현재 거리: "); 
    Serial.print(measure.RangeMilliMeter); // mm 단위 출력
    Serial.println(" mm");
  } else {
    Serial.println("측정 범위 초과 (Out of range)");
  }
  
  delay(100); // 0.1초마다 반복 측정
}