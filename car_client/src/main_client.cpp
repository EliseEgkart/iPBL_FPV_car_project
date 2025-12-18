#include <Arduino.h>
#include "BLEDevice.h"
#include <ESP32Servo.h>

// UUID를 BLEUUID 객체로 정의 (서비스와 특성의 고유 식별자)
static BLEUUID serviceUUID("a5944480-9116-4bc8-addb-5b4bcb7dc698");
static BLEUUID charUUID("3665520f-7d6c-4a8d-8c43-667dc712cdd8");
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

Servo myservo;  // 서보 모터 객체 생성
const int servoPin = 7; // 서보 모터 제어에 사용할 핀 번호
const int LED = 8; // 상태 표시용 LED 핀 번호

// L293D 핀
const int IN1 = 5;   // DirA // L293D input_pin 1/3
const int IN2 = 4;   // DirB // L293D input_pin 2/4
int speed = 0;

static int steeringValue=0;    // 조향값 변수
static int motorspeedValue=0;  // DC모터 속도값 변수
bool motorStarted = false;

const int freqServo = 50;  // 서보 모터 PWM주파수 (50Hz)
const int freqMOTOR = 1000; // BLDC 모터 PWM주파수 (1000Hz)
const int PWMChannel1 = 4; // BRAKE PWM 채널 (타이머 2 사용)
const int PWMChannel2 = 5;  // BLDC PWM 채널 (타이머 2 사용)
const int resolution = 8;  // PWM 해상도 (8비트)
const int EN1_2 = 1; // MOTOR_A EN1/2핀
const int EN3_4 = 6; // MOTOR_B EN3/4핀
boolean DirA = HIGH;
boolean DirB = HIGH;

 // BLE에서 수신되는 알림을 처리하는 콜백 함수 정의
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // 수신 데이터 유효성 검증 및 디버깅
  Serial.print("Received Raw Data: ");
  for (size_t i = 0; i < length; i++) {
     Serial.print((char)pData[i]);
  }
  Serial.println();
  //수신된 데이터를 문자열로 변환
  String receivedData;
  for (size_t i = 0; i < length; i++) {
    receivedData += (char)pData[i];
  }
  // 디버깅용 데이터 확인 출력
  Serial.print("Parsed Data: ");
  Serial.println(receivedData);
  // 문자열에서 필요한 데이터를 파싱하여 변수에 저장
  sscanf(receivedData.c_str(),"steering:%d,speed:%d",&steeringValue,&motorspeedValue);
}
  // BLE 클라이언트의 연결 상태를 처리하는 콜백 클래스 정의
  class MyClientCallback : public BLEClientCallbacks {

    void onConnect(BLEClient *pclient) {
      connected = true;        // 연결 성공 시 상태를 true로 설정
      digitalWrite(LED, LOW);  // LED 켜기 (연결 표시)
      Serial.println("onconnect");
    }

    void onDisconnect(BLEClient *pclient) {
      connected = false;       // 연결 해제 시 상태를 false로 설정
      digitalWrite(LED, HIGH); // LED 끄기 (연결 표시)
      Serial.println("onDisconnect");
      doScan = true;
    }
};

// BLE 서버에 연결 시도
bool connectToServer() {
  if (!myDevice) {
    Serial.println("No device found, skipping connection.");
    return false;
  }
    Serial.print("Connecting to: ");
    Serial.println(myDevice->getAddress().toString().c_str());
    BLEClient *pClient = BLEDevice::createClient(); // BLE 클라이언트 생성
    Serial.println(" - Client created");
    pClient->setClientCallbacks(new MyClientCallback()); // 클라이언트 콜백 설정

  if (!pClient->connect(myDevice)) {
    Serial.println(" - Failed to connect to server");
    return false;
 }
  Serial.println(" - Connected to server");
  pClient->setMTU(256); // MTU 크기 설정
// 원격 서비스 검색
BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
  Serial.println("Service UUID not found, disconnecting.");
  pClient->disconnect();
  return false;
}
// 원격 특성 검색 및 콜백 등록
pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Characteristic UUID not found, disconnecting.");
    pClient->disconnect();
    return false;
  }
  connected = true;
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  return true;
}
// 광고된 BLE 장치를 발견할 때 호출되는 콜백 클래스 정의
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

// 설정 함수 - 초기화 및 BLE 스캔 시작
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  BLEDevice::init(""); // BLE 장치 초기화
  Serial.println("initialize BLE Device");
  BLEScan *pBLEScan = BLEDevice::getScan(); // BLE 스캔 객체 생성
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); // 스캔 콜백 설정
  pBLEScan->setInterval(1349); // 스캔 간격 설정
  pBLEScan->setWindow(449); // 스캔 윈도우 설정
  pBLEScan->setActiveScan(true); // 액티브 스캔 활성화
  doScan = true; // 리셋 후에도 BLE 스캔이 자동으로 시작되도록 설정
  Serial.println("BLE Scan started.");

  myservo.setPeriodHertz(freqServo); // 서보 모터의 PWM 주파수 설정 (50Hz)
  myservo.attach(servoPin, 500, 2500); // 서보 모터 핀 및 최소/최대 펄스 폭 설정 (0~180')각도

// DC모터 PWM 설정
ledcAttachChannel(EN1_2,freqMOTOR,resolution,PWMChannel1);   //  PWM 설정 (1000Hz, 8비트 해상도) 
ledcAttachChannel(EN3_4,freqMOTOR,resolution,PWMChannel2);   //  PWM 설정 (1000Hz, 8비트 해상도) 

// 핀 모드 설정
pinMode(LED, OUTPUT);    // LED 핀 모드 설정
pinMode(IN1, OUTPUT); // INPUT_1 DirA
pinMode(IN2, OUTPUT); // INPUT_2 DirB

digitalWrite(LED, HIGH); // LED OFF
digitalWrite(IN1, HIGH); // L293D 입력 초기 입력신호
digitalWrite(IN2, HIGH); 
}

// 루프 함수 - BLE 서버 연결 및 모터 상태 업데이트
void loop() {
  if (doScan) {
    Serial.println("Scanning for devices...");
    BLEDevice::getScan()->start(20, false); // 스캔 실행
    doScan = false; // 스캔 완료 후 false로 설정
  }
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } 
    else {
      Serial.println("Failed to connect, retrying...");
      doScan = true; // 실패하면 다시 스캔 시작
    }
  doConnect = false;
  }

  if (connected) {
   // 서보 각도 매핑 (중립 90도 기준 대칭: 60~120도)
   int angle = map(steeringValue, -113, 113, 60, 120);
    myservo.write(angle); // 서보 각도 출력
     
    // 모터 제어
 if (motorspeedValue > 10) {
  motorStarted = true;
 }
   if (motorStarted) {
      if (motorspeedValue > 0) {   // 전진
        DirA = LOW;
        DirB = HIGH;
      } 
      else if(motorspeedValue < 0){// 후진
        DirA = HIGH;
        DirB = LOW;
      }
       digitalWrite(IN1, DirA);
       digitalWrite(IN2, DirB);

        int pwm1 = map(abs(motorspeedValue), 0, 114, 0, 255);
        int pwm2 = map(abs(motorspeedValue), 0, 114, 0, 255);
        ledcWrite(EN1_2, pwm1); // DC모터 속도 출력
        ledcWrite(EN3_4, pwm2);
    }
  }
}