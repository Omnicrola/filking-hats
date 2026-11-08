#pragma once

#include <Arduino.h>

#define DEBUG 1

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

constexpr float HPF_CUTOFF_HZ = 100.0f;
constexpr float FREQ_MIN_HZ = 100.0f;
constexpr float FREQ_MAX_HZ = 4000.0f;

constexpr float GATE_DBFS = -65.0f;
constexpr float MIN_REF_DBFS = -50.0f;
constexpr float DISPLAY_SPAN_DB = 30.0f;
constexpr float AGC_DECAY_DB_PER_S = 1.5f;
constexpr float LEVEL_ATTACK = 0.6f;
constexpr float LEVEL_RELEASE_PER_S = 400.0f;
constexpr float HUE_SMOOTHING = 0.3f;

constexpr uint8_t ESPNOW_CHANNEL = 2;
constexpr uint16_t PACKET_MAGIC = 0x5A48;
constexpr uint8_t GROUP_ID = 1;

struct __attribute__((packed)) HatPacket {
  uint16_t magic;
  uint8_t group;
  uint8_t seq;
  uint8_t level;
  uint8_t hue;
};
