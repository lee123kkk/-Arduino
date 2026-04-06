#include<Servo.h>
#include <U8x8lib.h>

Servo base;
Servo shoulder;
Servo upperarm;
Servo forearm;

int baseAngle = 90;
int shoulderAngle = 90;
int upperarmAngle = 90;
int forearmAngle = 90;

// [수정 포인트 1] 하드웨어 영점 조절용 오프셋 선언
int offset_upperarm = -10; 
int offset_forearm = 10;

String baseAngle_str;
String shoulder_str;
String upperarm_str;
String forearm_str;
String inString;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); 

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

void clear_oled(){
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"                  ");
  u8x8.drawString(0,1,"                  ");
  u8x8.drawString(0,2,"                  ");
  u8x8.drawString(0,3,"                  ");
  delay(100);
}

void setup() {
  Serial.begin(115200);
   // 1. attach()를 하기 전에 목표 각도를 먼저 write() 해줍니다.
  // 이렇게 하면 아두이노가 핀을 연결하자마자 90도가 아닌, 우리가 지정한 오프셋 각도로 신호를 보냅니다.
  base.write(baseAngle);
  base.attach(3);

  shoulder.write(shoulderAngle);
  shoulder.attach(5);
  
  // 2. 핀 번호를 원래 잘 작동했던 번호(upperarm:9, forearm:6)로 원복했습니다.
  upperarm.write(upperarmAngle + offset_upperarm); 
  upperarm.attach(9); 
  
  forearm.write(forearmAngle + offset_forearm);
  forearm.attach(6);  

  u8x8.begin();
  u8x8.setPowerSave(0);
}

void loop() {
  delay(100);
  if (Serial.available() > 0){  
    if(Serial.available()){
      inString = Serial.readStringUntil('\n');
      char cmd = inString[0];
      
      // ===== 명령어 1: 상태 읽기 =====
      if(cmd == '1'){
        clear_oled();
        u8x8.drawString(0,0,"read angles");
        Serial.print('a'); Serial.print(baseAngle);
        Serial.print('b'); Serial.print(shoulderAngle);
        Serial.print('c'); Serial.print(upperarmAngle);
        Serial.print('d'); Serial.print(forearmAngle);
        Serial.println('e');
      }
      
      // ===== 명령어 2: 각도 제어 (파싱) =====
      else if(cmd == '2'){
        int first = inString.indexOf('a');
        int second = inString.indexOf('b');
        baseAngle_str = inString.substring(first+1, second);
        baseAngle = baseAngle_str.toInt();    
        Serial.print(baseAngle); Serial.print(" ");
        
        first = inString.indexOf('b');
        second = inString.indexOf('c');
        shoulder_str = inString.substring(first+1, second);
        shoulderAngle = shoulder_str.toInt();
        Serial.print(shoulderAngle); Serial.print(" ");
        
        first = inString.indexOf('c');
        second = inString.indexOf('d');
        upperarm_str = inString.substring(first+1, second);
        // 방어 코드: 문자열 길이가 0보다 클 때(즉, 숫자가 정상적으로 추출되었을 때)만 적용!
        if(upperarm_str.length() > 0) {
          upperarmAngle = upperarm_str.toInt();
        }
        Serial.print(upperarmAngle); Serial.print(" ");
        
        first = inString.indexOf('d');
        second = inString.indexOf('e');
        forearm_str = inString.substring(first+1, second);
        forearmAngle = forearm_str.toInt();
        Serial.print(forearmAngle); Serial.println(" ");

        int status1 = 0, status2 = 0, status3 = 0, status4 = 0;
        int done = 0 ;    

        while(done == 0){     
          status1 = servoParallelControl(baseAngle, base, 30);         
          status2 = servoParallelControl(shoulderAngle, shoulder, 30);
          // [수정 포인트 3] 명령어 2번의 제어에 오프셋 적용
          status3 = servoParallelControl(upperarmAngle + offset_upperarm, upperarm, 30);      
          status4 = servoParallelControl(forearmAngle + offset_forearm, forearm, 30);  

          if (status1 == 1 && status2 == 1 && status3 == 1 && status4 == 1){
            done = 1; 
          }   
        }
        
        clear_oled();
        u8x8.setFont(u8x8_font_chroma48medium8_r);
        u8x8.drawString(0,0,inString.c_str());
        u8x8.drawString(0,1,baseAngle_str.c_str());
        u8x8.drawString(0,2,shoulder_str.c_str());
        u8x8.drawString(0,3,upperarm_str.c_str());
        delay(100);
      }
      
      // ===== 명령어 3: 초기화 =====
      else if(cmd == '3'){
        int done = 0;
        int status1 = 0, status2 = 0, status3 = 0, status4 = 0;
        
        // 각도 변수들도 90으로 초기화
        baseAngle = 90; shoulderAngle = 90; 
        upperarmAngle = 90; forearmAngle = 90;

        while(done == 0){     
            status1 = servoParallelControl(90, base, 20);         
            status2 = servoParallelControl(90, shoulder, 20);
            // [수정 포인트 4] 명령어 3번의 초기화에 오프셋 적용
            status3 = servoParallelControl(90 + offset_upperarm, upperarm, 20);      
            status4 = servoParallelControl(90 + offset_forearm, forearm, 20);  

            if (status1 == 1 && status2 == 1 && status3 == 1 && status4 == 1){
              done = 1; 
            }   
          }
      }
    }
  }
}