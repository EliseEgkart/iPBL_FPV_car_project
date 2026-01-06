#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/* =========================
   BLE UUID 정의
   ========================= */
static BLEUUID serviceUUID("d193b1a0-7afd-4b78-b104-c3922271fc76");
static BLEUUID charUUID("ad61ea0d-26eb-4dc5-a0f3-7a63ad1cebd3");

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

/* =========================
   핀 정의
   ========================= */
const int LED = 8;               // 상태 표시 LED

const int joystick_Y = 3;        // 조이스틱 Y축 → steering 유지
const int joystick_X = 4;        // (미사용, 남겨둠)

const int BTN_BACKWARD = 2;      // 낮은 번호 핀 → 후진
const int BTN_FORWARD  = 7;      // 높은 번호 핀 → 전진

/* =========================
   제어 변수
   ========================= */
int steering = 0;
int speed = 0;

const int SPEED_VALUE = 114;      // 전진/후진 시 고정 속도

unsigned long previousMillis = 0;

/* =========================
   BLE 서버 콜백
   ========================= */
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        digitalWrite(LED, HIGH);
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        digitalWrite(LED, LOW);
        BLEDevice::startAdvertising();
    }
};

/* =========================
   BLE 알림 전송 함수
   ========================= */
void sendNotification() {
    if (deviceConnected) {
        char message[75];
        snprintf(message, sizeof(message),
                 "steering:%d,speed:%d",
                 steering, speed);

        pCharacteristic->setValue(message);
        pCharacteristic->notify();

        Serial.print("Notification sent: ");
        Serial.println(message);
    }
}

/* =========================
   setup()
   ========================= */
void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    // 버튼 내부 풀업 저항 사용
    pinMode(BTN_FORWARD, INPUT_PULLUP);
    pinMode(BTN_BACKWARD, INPUT_PULLUP);

    analogReadResolution(8);   // 0~255

    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    BLEDevice::init("ESP32 BLE Controller");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(serviceUUID);

    pCharacteristic = pService->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->setValue("Initial Value");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined!");
}

/* =========================
   loop()
   ========================= */
void loop() {
    unsigned long currentMillis = millis();

    /* -------- 조향: 조이스틱 Y축 유지 -------- */
    int rawSteer = analogRead(joystick_Y);
    steering = constrain(rawSteer - 118, -118, 118);

    /* -------- 속도: 버튼 기반 -------- */
    bool forwardPressed  = (digitalRead(BTN_FORWARD)  == LOW);
    bool backwardPressed = (digitalRead(BTN_BACKWARD) == LOW);

    if (forwardPressed && !backwardPressed) {
        speed = SPEED_VALUE;          // 전진
    }
    else if (!forwardPressed && backwardPressed) {
        speed = -SPEED_VALUE;         // 후진
    }
    else {
        speed = 0;                    // 정지 또는 동시 입력
    }

    /* -------- BLE 전송 -------- */
    if (currentMillis - previousMillis >= 50) {
        previousMillis = currentMillis;

        Serial.print("steering: ");
        Serial.print(steering);
        Serial.print(", speed: ");
        Serial.println(speed);

        sendNotification();
    }
}
