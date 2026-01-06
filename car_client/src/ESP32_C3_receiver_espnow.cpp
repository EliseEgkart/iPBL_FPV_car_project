#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

Servo myservo;
const int servoPin = 7;
const int LED = 8;


const int freqServo = 50;  // 서보 모터 PWM주파수 (50Hz)
const int freqMOTOR = 1000; // BLDC 모터 PWM주파수 (2000Hz)
const int PWMChannel1 = 4; // BRAKE PWM 채널 (타이머 2 사용)
const int PWMChannel2 = 5;  // BLDC PWM 채널 (타이머 2 사용)
const int resolution = 8;  // PWM 해상도 (8비트)
const int EN1_2 = 1; // MOTOR_A EN1/2핀
const int EN3_4 = 6; // MOTOR_B EN3/4핀
boolean DirA = HIGH;
boolean DirB = HIGH;

// L293D 핀
const int IN1 = 5;   // DirA
const int IN2 = 4;   // DirB

int steeringValue = 0;
int motorspeedValue = 0;
bool motorStarted = false;

// 수신 콜백
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  char packet[50];
  memcpy(packet, data, len);
  packet[len] = 0;

  sscanf(packet, "steering:%d,speed:%d",&steeringValue,&motorspeedValue);
  Serial.printf("Recv: steer=%d,speed=%d\n",steeringValue,motorspeedValue);

    // 서보 조향
    int angle = map(steeringValue, -113, 113, 60, 120);
    myservo.write(angle);

    // 모터 제어
    if (abs(motorspeedValue) > 20) {  // 데드존
      motorStarted = true;
    }

    if (motorStarted) {
      if (motorspeedValue > 0) {  // 전진
      DirA = LOW;
      DirB = HIGH;
 
      } else if(motorspeedValue < 0){                    // 후진
      DirA = HIGH;
      DirB = LOW;
      }
       digitalWrite(IN1, DirA);
       digitalWrite(IN2, DirB);
      int pwm1 = map(abs(motorspeedValue), 0, 114, 0, 255);
      int pwm2 = map(abs(motorspeedValue), 0, 114, 0, 255);
      ledcWrite(EN1_2, pwm1);
      ledcWrite(EN3_4, pwm2);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());  // 조종기 코드에 이 MAC 입력!

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  // 연결 전 LED OFF
    myservo.setPeriodHertz(freqServo); // 서보 모터의 PWM 주파수 설정 (50Hz)
    myservo.attach(servoPin, 500, 2400); // 서보 모터 핀 및 최소/최대 펄스 폭 설정 (0~180')각도
    
    ledcAttachChannel(EN1_2,freqMOTOR,resolution,PWMChannel1);   
    ledcAttachChannel(EN3_4,freqMOTOR,resolution,PWMChannel2);  
  myservo.write(90);


  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, DirA);
  digitalWrite(IN2, DirB);

  WiFi.mode(WIFI_STA);
  WiFi.channel(11);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW Receiver Ready");
}

void loop() {
  // 비어있음 (비동기 수신)
}