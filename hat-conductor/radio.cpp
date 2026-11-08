#include "radio.h"
#include "config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t seq = 0;

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
  return esp_now_add_peer(&peer) == ESP_OK;
}

void broadcast(uint8_t level, uint8_t hue) {
  HatPacket pkt = {PACKET_MAGIC, GROUP_ID, seq++, level, hue};
  esp_now_send(BROADCAST_ADDR, (const uint8_t *)&pkt, sizeof(pkt));
}
