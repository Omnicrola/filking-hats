#include "radio.h"
#include "config.h"
#include "settings.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t seq = 0;
static uint8_t channel = DEFAULT_CHANNEL;
static uint8_t pendingChannel = 0;
static uint32_t switchAtMs = 0;
static uint32_t lastBeaconMs = 0;

static portMUX_TYPE hatsMux = portMUX_INITIALIZER_UNLOCKED;
static HatInfo hats[MAX_RECEIVERS];
static int hatsUsed = 0;

static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(HeartbeatPacket)) return;
  HeartbeatPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.magic != HEARTBEAT_MAGIC || pkt.group != GROUP_ID) return;
  int8_t rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : 0;
  uint32_t now = millis();
  taskENTER_CRITICAL(&hatsMux);
  int slot = -1;
  for (int i = 0; i < hatsUsed; i++) {
    if (memcmp(hats[i].mac, info->src_addr, 6) == 0) {
      slot = i;
      break;
    }
  }
  if (slot < 0 && hatsUsed < MAX_RECEIVERS) {
    slot = hatsUsed++;
    memcpy(hats[slot].mac, info->src_addr, 6);
    hats[slot].firstMs = now;
  }
  if (slot >= 0) {
    hats[slot].rssi = rssi;
    hats[slot].version = pkt.version;
    hats[slot].lastMs = now;
  }
  taskEXIT_CRITICAL(&hatsMux);
}

bool radioBegin() {
  channel = settingsChannel();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, channel, 0, AP_MAX_CLIENTS)) return false;
  esp_wifi_set_ps(WIFI_PS_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_ADDR, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) return false;
  return esp_now_register_recv_cb(onReceive) == ESP_OK;
}

static void sendChannel(uint8_t ch) {
  ChannelPacket pkt = {CHANNEL_MAGIC, GROUP_ID, ch};
  esp_now_send(BROADCAST_ADDR, (const uint8_t *)&pkt, sizeof(pkt));
}

static void applyPendingChannel() {
  channel = pendingChannel;
  pendingChannel = 0;
  settingsSaveChannel(channel);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, channel, 0, AP_MAX_CLIENTS);
  DBG("channel -> %u %s (wifi reports %d)\n", channel, ok ? "ok" : "FAIL", WiFi.channel());
}

void broadcast(uint8_t level, uint8_t hue, uint8_t mode) {
  HatPacket pkt = {PACKET_MAGIC, GROUP_ID, seq++, level, hue, mode};
  esp_now_send(BROADCAST_ADDR, (const uint8_t *)&pkt, sizeof(pkt));
  uint32_t now = millis();
  if (pendingChannel) {
    sendChannel(pendingChannel);
    if ((int32_t)(now - switchAtMs) >= 0) applyPendingChannel();
  } else if (now - lastBeaconMs >= CHANNEL_BEACON_MS) {
    lastBeaconMs = now;
    sendChannel(channel);
  }
}

uint8_t radioChannel() {
  return channel;
}

uint8_t radioPendingChannel() {
  return pendingChannel;
}

bool radioRequestChannel(uint8_t ch) {
  if (ch < CHANNEL_MIN || ch > CHANNEL_MAX) return false;
  if (ch == channel) {
    pendingChannel = 0;
    return true;
  }
  pendingChannel = ch;
  switchAtMs = millis() + CHANNEL_ANNOUNCE_MS;
  return true;
}

int radioReceiverCount() {
  int n = 0;
  taskENTER_CRITICAL(&hatsMux);
  uint32_t now = millis();
  for (int i = 0; i < hatsUsed; i++) {
    if (now - hats[i].lastMs <= RECEIVER_TIMEOUT_MS) n++;
  }
  taskEXIT_CRITICAL(&hatsMux);
  return n;
}

int radioHats(HatInfo *out, int max) {
  taskENTER_CRITICAL(&hatsMux);
  int n = min(hatsUsed, max);
  memcpy(out, hats, n * sizeof(HatInfo));
  taskEXIT_CRITICAL(&hatsMux);
  return n;
}
