#pragma once

#include <Arduino.h>

#define DEBUG 1

constexpr uint16_t FIRMWARE_VERSION = 6;

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

constexpr int8_t MIC_BCLK_PIN = 43;
constexpr int8_t MIC_WS_PIN = 44;
constexpr int8_t MIC_DIN_PIN = 8;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint16_t SAMPLES = 512;
constexpr float FRAME_S = (float)SAMPLES / SAMPLE_RATE;

constexpr float FREQ_MAX_HZ = 4000.0f;
constexpr float MIN_REF_DBFS = -50.0f;
constexpr float DISPLAY_SPAN_DB = 30.0f;
constexpr float LEVEL_ATTACK = 0.6f;
constexpr float HUE_SMOOTHING = 0.3f;
constexpr uint16_t SPECTRUM_SAMPLES = 2048;
constexpr float SPECTRUM_LOW_HZ = 80.0f;
constexpr float SPECTRUM_HIGH_HZ = 500.0f;

constexpr float DEFAULT_GATE_DBFS = -65.0f;
constexpr float DEFAULT_MIN_FREQ_HZ = 100.0f;
constexpr float DEFAULT_LEVEL_RELEASE_PER_S = 400.0f;
constexpr float DEFAULT_AGC_DECAY_DB_PER_S = 1.5f;

constexpr float GATE_DBFS_MIN = -90.0f;
constexpr float GATE_DBFS_MAX = -20.0f;
constexpr float MIN_FREQ_HZ_MIN = 20.0f;
constexpr float MIN_FREQ_HZ_MAX = 1000.0f;
constexpr float LEVEL_RELEASE_MIN = 20.0f;
constexpr float LEVEL_RELEASE_MAX = 2000.0f;
constexpr float AGC_DECAY_MIN = 0.1f;
constexpr float AGC_DECAY_MAX = 20.0f;

constexpr const char *AP_SSID = "conductor-hat";
constexpr const char *AP_PASSWORD = "jinglebells";
constexpr uint8_t AP_MAX_CLIENTS = 2;

constexpr int MAX_RECEIVERS = 32;
constexpr uint32_t RECEIVER_TIMEOUT_MS = 3000;
constexpr uint32_t BELL_ACTIVE_MS = 5000;

constexpr uint8_t DEFAULT_CHANNEL = 2;
constexpr uint8_t CHANNEL_MIN = 1;
constexpr uint8_t CHANNEL_MAX = 11;
constexpr uint32_t CHANNEL_BEACON_MS = 1000;
constexpr uint32_t CHANNEL_ANNOUNCE_MS = 1500;
constexpr uint16_t PACKET_MAGIC = 0x5A48;
constexpr uint16_t HEARTBEAT_MAGIC = 0x5A52;
constexpr uint16_t CHANNEL_MAGIC = 0x5A43;
constexpr uint16_t SPECTRUM_MAGIC = 0x5A53;
constexpr uint16_t BELL_MAGIC = 0x5A42;
constexpr int BELL_MAX_RECEIVERS = 32;
constexpr uint32_t BELL_DECAY_MS = 1000;
constexpr uint8_t FREQ_MIN_BRIGHTNESS = 8;
constexpr uint8_t GROUP_ID = 1;

enum HatMode : uint8_t { MODE_REACTIVE = 0, MODE_SLEEP = 1, MODE_DISCO = 2, MODE_RANGED = 3, MODE_METRONOME = 4, MODE_FREQUENCY = 5, MODE_BELL = 6 };
constexpr uint8_t MODE_COUNT = 7;

constexpr uint16_t BPM_MIN = 40;
constexpr uint16_t BPM_MAX = 300;
constexpr uint16_t DEFAULT_BPM = 120;
constexpr uint32_t METRONOME_FRAME_MS = 10;
constexpr uint32_t METRONOME_FLASH_MS = 150;
constexpr uint8_t METRONOME_BEATS_PER_BAR = 4;
constexpr uint8_t METRONOME_ACCENT_HUE = 255;
constexpr uint8_t METRONOME_BEAT_HUE = 85;

struct __attribute__((packed)) HatPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t seq;
  uint8_t level;
  uint8_t hue;
  uint8_t mode;
};

struct __attribute__((packed)) HeartbeatPacket {
  uint16_t magic;
  uint8_t group;
  uint16_t version;
};

struct __attribute__((packed)) ChannelPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t channel;
};

struct __attribute__((packed)) SpectrumPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t bands[NUM_LEDS];
};

struct __attribute__((packed)) BellPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t count;
  uint8_t winner;
  uint8_t macs[BELL_MAX_RECEIVERS][6];
};
