#pragma once

#include <Arduino.h>

struct AudioFrame {
  float db;
  float refDb;
  float freq;
  uint8_t level;
  uint8_t hue;
  bool gated;
};

bool audioBegin();
AudioFrame audioProcess();
