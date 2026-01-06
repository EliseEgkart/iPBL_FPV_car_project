#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

const int joystick_X = 3;
const int joystick_Y = 4;
uint8_t receiverMAC[] = {0xa8, 0x46, 0x74, 0x5c, 0x11, 0x60};  // 수신기 MAC (아래 코드에서 출력)

// 콜백 (ACK 확인)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(8);

  WiFi.mode(WIFI_STA);  // ESP-NOW 필요
  WiFi.channel(11);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);  // ACK 콜백

  // 수신기 등록 (MAC 주소 입력)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;  // 채널 0 (자동)
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;   // 이 줄 추가 필수!!!
  esp_now_add_peer(&peerInfo);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP-NOW Transmitter Ready");
}

void loop() {
  int rawX = analogRead(joystick_X);
  int rawY = analogRead(joystick_Y);

  // 중립 보정 (이전처럼)
  int steering = constrain(rawX - 118, -118, 118);
  int speed = constrain(rawY - 114, -114, 114);

  char packet[50];
  snprintf(packet, sizeof(packet),"steering:%d,speed:%d",steering,speed);

  esp_now_send(receiverMAC, (uint8_t *)packet, strlen(packet));
  Serial.printf("Sent: %s\n", packet);

  delay(20);  // 50Hz (지연 최소화)
}