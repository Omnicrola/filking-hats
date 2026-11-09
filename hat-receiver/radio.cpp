#include "radio.h"
#include "config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t rxLevel = 0;
static volatile uint8_t rxHue = 0;
static volatile uint32_t rxTimeMs = 0;
static volatile uint32_t rxCount = 0;
static volatile uint32_t rxLost = 0;
static volatile uint8_t rxLastSeq = 0;
static volatile bool rxAny = false;

static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(HatPacket)) return;
  HatPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.magic != PACKET_MAGIC || pkt.group != GROUP_ID) return;
  taskENTER_CRITICAL(&rxMux);
  if (rxAny) rxLost += (uint8_t)(pkt.seq - rxLastSeq - 1);
  rxLastSeq = pkt.seq;
  rxLevel = pkt.level;
  rxHue = pkt.hue;
  rxTimeMs = millis();
  rxCount++;
  rxAny = true;
  taskEXIT_CRITICAL(&rxMux);
}

bool radioBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_ADDR, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) return false;
  return esp_now_register_recv_cb(onReceive) == ESP_OK;
}

void radioSendHeartbeat() {
  HeartbeatPacket pkt = {HEARTBEAT_MAGIC, GROUP_ID, FIRMWARE_VERSION};
  esp_now_send(BROADCAST_ADDR, (const uint8_t *)&pkt, sizeof(pkt));
}

String radioHatId() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  char buf[9];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

RadioState radioSnapshot() {
  RadioState s;
  taskENTER_CRITICAL(&rxMux);
  s.any = rxAny;
  s.level = rxLevel;
  s.hue = rxHue;
  s.lastRxMs = rxTimeMs;
  s.count = rxCount;
  s.lost = rxLost;
  taskEXIT_CRITICAL(&rxMux);
  return s;
}
