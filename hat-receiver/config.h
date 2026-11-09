#pragma once

#include <Arduino.h>

#define DEBUG 1

constexpr uint16_t FIRMWARE_VERSION = 4;

#if DEBUG
#define DBG(...) Serial.printf(__VA_ARGS__)
#else
#define DBG(...)
#endif

constexpr uint8_t LED_PIN = 7;
constexpr int NUM_LEDS = 15;
constexpr uint8_t LED_BRIGHTNESS = 64;
constexpr uint32_t LED_POWER_MA = 250;
constexpr uint32_t CHASE_STEP_MS = 100;
constexpr uint32_t PEAK_HOLD_MS = 300;
constexpr float PEAK_FALL_LEDS_PER_S = 12.0f;

constexpr uint32_t FRAME_MS = 10;
constexpr float LEVEL_SMOOTHING = 0.35f;
constexpr uint32_t SIGNAL_TIMEOUT_MS = 5000;
constexpr uint32_t DEBUG_INTERVAL_MS = 1000;
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 1000;
constexpr uint32_t DISCO_COLOR_MS = 1000;
constexpr float RANGE_RSSI_LOW = -90.0f;
constexpr float RANGE_RSSI_HIGH = -40.0f;
constexpr float RSSI_SMOOTHING = 0.05f;

constexpr uint8_t DEFAULT_CHANNEL = 2;
constexpr uint8_t CHANNEL_MIN = 1;
constexpr uint8_t CHANNEL_MAX = 11;
constexpr uint32_t SCAN_DWELL_MS = 250;
constexpr uint16_t PACKET_MAGIC = 0x5A48;
constexpr uint16_t HEARTBEAT_MAGIC = 0x5A52;
constexpr uint16_t CHANNEL_MAGIC = 0x5A43;
constexpr uint8_t GROUP_ID = 1;

enum HatMode : uint8_t { MODE_REACTIVE = 0, MODE_SLEEP = 1, MODE_DISCO = 2, MODE_RANGED = 3, MODE_METRONOME = 4 };
constexpr uint8_t MODE_COUNT = 5;

struct __attribute__((packed)) HeartbeatPacket {
  uint16_t magic;
  uint8_t group;
  uint16_t version;
};

struct __attribute__((packed)) HatPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t seq;
  uint8_t level;
  uint8_t hue;
  uint8_t mode;
};

struct __attribute__((packed)) ChannelPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t channel;
};
