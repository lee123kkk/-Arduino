#include <Servo.h>

Servo base;
Servo shoulder;
Servo upperarm;
Servo forearm;

int baseAngle = 90;
int shoulderAngle = 90;
int upperarmAngle = 90;
int forearmAngle = 90;

// 이전에 찾으신 영점 조절 오프셋 (필요시 수정)
int offset_forearm = 10;  
int offset_upperarm = -10; 

String inString;

// 서보 동시 제어 함수 (이전과 동일)
int servoParallelControl (int thePos, Servo theServo, int speed){
    int startPos = theServo.read();       
    int newPos = startPos;                
    
    if (startPos < (thePos)){
       newPos = newPos + 1;               
       theServo.write(newPos);
       delay(speed);
       return 0;                     
    }
    else if (newPos > (thePos)){
      newPos = newPos - 1;
      theServo.write(newPos);
      delay(speed);
      return 0;  
    }  
    else {
        return 1;
    }  
}

void setup() {
  Serial.begin(115200);
  
  // 모터 핀 연결
  base.attach(3);
  shoulder.attach(5);
  forearm.attach(6);
  upperarm.attach(9);

  // 초기 위치 90도로 세팅 (오프셋 적용)
  base.write(90);
  shoulder.write(90);
  forearm.write(90 + offset_forearm);
  upperarm.write(90 + offset_upperarm);
}

void loop() {
  // 1. 시리얼 통신으로 데이터가 들어오면 실행
  if(Serial.available()){
    inString = Serial.readStringUntil('\n');
    
    // 2. 문자열 파싱 (a, b, c, d, e 사이의 숫자 추출)
    int first = inString.indexOf('a');
    int second = inString.indexOf('b');
    if(first != -1 && second != -1) baseAngle = inString.substring(first+1, second).toInt();
    
    first = inString.indexOf('b');
    second = inString.indexOf('c');
    if(first != -1 && second != -1) shoulderAngle = inString.substring(first+1, second).toInt();
    
    first = inString.indexOf('c');
    second = inString.indexOf('d');
    if(first != -1 && second != -1) upperarmAngle = inString.substring(first+1, second).toInt();
    
    first = inString.indexOf('d');
    second = inString.indexOf('e');
    if(first != -1 && second != -1) forearmAngle = inString.substring(first+1, second).toInt();

    // 터미널에 확인용 출력
    Serial.print("목표 각도: ");
    Serial.print(baseAngle); Serial.print(" ");
    Serial.print(shoulderAngle); Serial.print(" ");
    Serial.print(upperarmAngle); Serial.print(" ");
    Serial.println(forearmAngle);

    // 3. 파싱된 각도로 모터 이동 시작
    int status1 = 0, status2 = 0, status3 = 0, status4 = 0;
    int done = 0;
    
    // 모든 모터가 목표에 도달할 때까지 반복
    while(done == 0){ 
      status1 = servoParallelControl(baseAngle, base, 20);
      status2 = servoParallelControl(shoulderAngle, shoulder, 20);
      // forearm과 upperarm은 영점 조절 오프셋을 더해서 제어합니다.
      status3 = servoParallelControl(forearmAngle + offset_forearm, forearm, 20);
      status4 = servoParallelControl(upperarmAngle + offset_upperarm, upperarm, 20);        
      
      if (status1 == 1 && status2 == 1 && status3 == 1 && status4 == 1){
        done = 1; // 이동 완료
        Serial.println("이동 완료!");
      }   
    } // end of while
  } // end of if(Serial.available())
}
