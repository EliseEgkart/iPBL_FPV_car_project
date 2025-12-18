#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUID를 BLEUUID 객체로 정의 (서비스와 특성의 고유 식별자)
static BLEUUID serviceUUID("a5944480-9116-4bc8-addb-5b4bcb7dc698");
static BLEUUID charUUID("3665520f-7d6c-4a8d-8c43-667dc712cdd8");
BLECharacteristic *pCharacteristic;    // BLE 특성을 나타내는 포인터 변수
bool deviceConnected = false;          // 클라이언트 연결 여부를 저장하는 변수

const int LED = 8;             // 상태 표시용 LED 핀 번호
const int joystick_Y = 3;   // 아날로그 입력 핀 3 조이스틱 X축
const int joystick_X = 4;   // 아날로그 입력 핀 4 조이스틱 Y축

int steering=0; // 조향값 변수
int speed=0;    // DC모터 속도값 변수
unsigned long previousMillis = 0;      // 이전 시간 저장 변수

// 연결 및 연결 해제 시 호출되는 콜백 클래스 정의
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;         // 클라이언트가 연결되면 deviceConnected를 true로 설정
        digitalWrite(LED, HIGH);         // LED를 HIGH 로 설정 (LED 끄기)
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;        // 클라이언트가 연결 해제되면 deviceConnected를 false로 설정
        digitalWrite(LED, LOW);        // LED를 LOW 로 설정 (LED 켜기)
         BLEDevice::startAdvertising(); 
    }
};

// 클라이언트에게 알림을 전송하는 함수
void sendNotification() {
    if (deviceConnected) {  // 클라이언트가 연결되어 있는 경우에만 실행
       // 전송할 메시지를 형식화하여 문자열로 생성
        char message[75];
        snprintf(message,sizeof(message),"steering:%d,speed:%d",steering,speed);

        pCharacteristic->setValue(message); // 특성에 메시지 값 설정
        pCharacteristic->notify();          // 클라이언트에 알림 전송

        // 디버깅용으로 메시지 출력
        Serial.print("Notification sent: ");
        Serial.println(message);
    }
}



void setup() {
    pinMode(LED, OUTPUT);       // LED 핀을 출력 모드로 설정
    digitalWrite(LED, LOW);     // 초기 상태를 LOW로 설정 (LED 켜기)
    analogReadResolution(8);   // ADC를 8비트 분해능으로 설정 0~255

    Serial.begin(115200);       // 시리얼 통신 시작
    Serial.println("Starting BLE work!"); // BLE 작업 시작 메시지 출력
    BLEDevice::init("ESP32 BLE Controller");   // BLE 장치 초기화 및 장치 이름 설정

    BLEServer *pServer = BLEDevice::createServer(); // BLE 서버 생성
    pServer->setCallbacks(new MyServerCallbacks()); // 연결/해제 시 실행될 콜백 설정
    BLEService *pService = pServer->createService(serviceUUID); // 서버에 서비스 생성
    pCharacteristic = pService->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY // 읽기와 알림 권한 설정
    );
    pCharacteristic->setValue("Initial Value"); // 초기 특성 값 설정
    pService->start();                          // 서비스 시작
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // 광고 객체 생성
    pAdvertising->addServiceUUID(serviceUUID);                  // 광고에 서비스 UUID 추가
    pAdvertising->setScanResponse(true);                        // 스캔 응답 활성화
    pAdvertising->setMinPreferred(0x06);                       
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();                              // BLE 광고 시작
    Serial.println("Characteristic defined!");
}

void loop() {
unsigned long currentMillis = millis(); // 현재 시간 저장

  int rawX = analogRead(joystick_Y);
  int rawY = analogRead(joystick_X);

    // 아날로그 입력값을 읽어 변수에 저장
    steering = constrain(rawX - 118, -118, 118);
    speed = constrain(rawY - 114, -114, 114);

    // 50ms마다 BLE 알림을 통해 데이터 전송
    if (currentMillis - previousMillis >= 50) {
        previousMillis = currentMillis; // 이전 시간 갱신
        // 디버깅용 시리얼 출력
        Serial.print("steering: ");
        Serial.print(steering);
        Serial.print(", speed: ");
        Serial.println(speed);
        sendNotification();   // 알림 전송
    }
}