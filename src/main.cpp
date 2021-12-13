// Include standard library
#include <string>
#include <vector>

#include <Arduino.h>
#include <ESPNowW.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 3600);

/*
  This project uses NTP to get time and broadcast it via ESP Now.
*/

uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void on_receive(const uint8_t *mac, const uint8_t *data, int len) {
  const std::string message(data, data + len);
  Serial.printf("Received: %s\n", message.c_str());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

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
}

void loop() {
  // Update
  timeClient.update();

  // Other stuff
  static unsigned long last_time = 0;
  if (millis() - last_time > 1000) {
    last_time = millis();
    const uint64_t timestamp = timeClient.getBetterEpochTime();
    char buffer[40];
    snprintf(buffer, 40, "T:%" PRIu64 "\n", timestamp);
    ESPNow.send_message(receiver_mac, (uint8_t *)buffer, strlen(buffer));
    Serial.printf("Sending message: %s\n", buffer);
  }
}