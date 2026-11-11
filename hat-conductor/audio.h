#pragma once

#include <Arduino.h>
#include "config.h"

struct AudioFrame {
  float db;
  float refDb;
  float freq;
  uint8_t level;
  uint8_t hue;
  bool gated;
  uint8_t bands[NUM_LEDS];
};

bool audioBegin();
AudioFrame audioProcess(bool spectrum);
int audioDominantBand(int bands, bool gated);
