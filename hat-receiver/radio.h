#pragma once

#include <Arduino.h>
#include "config.h"

struct RadioState {
  bool any;
  uint8_t level;
  uint8_t hue;
  uint8_t mode;
  int8_t rssi;
  uint8_t bands[NUM_LEDS];
  uint32_t lastRxMs;
  uint32_t count;
  uint32_t lost;
};

bool radioBegin();
RadioState radioSnapshot();
void radioSendHeartbeat();
String radioHatId();
void radioUpdate(bool lost);
uint8_t radioChannel();
