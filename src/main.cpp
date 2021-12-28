// Include standard library
#include <string>

#include <Arduino.h>
#include <ESPNowW.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <analogWrite.h>

#define PIN_R 19
#define PIN_G 18
#define PIN_B 17

uint32_t ledtimer = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 3600);

/*
  : _Bit_alloc_type(), _M_start(), _M_finish(), _M_end_of_storage()
  This project uses NTP to get time and broadcast it via ESP Now.
*/

uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void on_receive(const uint8_t *mac, const uint8_t *data, int len) {
  const std::string message(data, data + len);
  Serial.printf("Received: %s\n", message.c_str());
  
  // Parse the message (msg format: "msg_type:msg_value:msg_token") via find and npos
  const size_t pos = message.find(":");
  if (pos == std::string::npos) {
    Serial.println("Invalid message format");
    return;
  }
  const size_t pos2 = message.find(":", pos + 1);
  if (pos2 == std::string::npos) {
    Serial.println("Invalid message format");
    return;
  }
  const std::string msg_type = message.substr(0, pos);
  const std::string msg_value = message.substr(pos + 1, pos2 - pos - 1);
  std::string msg_token{};
  const size_t pos3 = message.find(":", pos2 + 1);
  if (pos3 != std::string::npos) {
    message.substr(pos2 + 1, pos3 - pos2 - 1);
  }

  if (msg_type == "BOBBYOPEN") {
    Serial.println("Bob is opening the door");
    digitalWrite(PIN_R, HIGH);
    digitalWrite(PIN_G, LOW);
    digitalWrite(PIN_B, LOW);

    ledtimer = millis();
  }
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  digitalWrite(PIN_R, HIGH);
  digitalWrite(PIN_G, HIGH);
  digitalWrite(PIN_B, HIGH);

  // Set hostname
  WiFi.hostname("ESP32-Garagentor");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // NTP
  timeClient.begin();

  // ESPNow
  ESPNow.init();
  ESPNow.add_peer(receiver_mac);
  ESPNow.reg_recv_cb(on_receive);

  delay(100);
  digitalWrite(PIN_R, LOW);
  digitalWrite(PIN_G, LOW);
  digitalWrite(PIN_B, LOW);
}

void loop() {
  // Update
  timeClient.update();

  // Other stuff
  static unsigned long last_time = 0;
  if (millis() - last_time > 1000) {
    Serial.printf("Current WiFi Channel: %d\n", WiFi.channel());
    last_time = millis();
    const uint64_t timestamp = timeClient.getBetterEpochTime();
    char buffer[40];
    snprintf(buffer, 40, "T:%" PRIu64 "\n", timestamp);
    ESPNow.send_message(receiver_mac, (uint8_t *)buffer, strlen(buffer));
    Serial.printf("Sending message: %s\n", buffer);
  }

  if (millis() - ledtimer > 1000) {
    digitalWrite(PIN_R, LOW);
    digitalWrite(PIN_G, LOW);
    digitalWrite(PIN_B, LOW);
    ledtimer = 0;
  }
}