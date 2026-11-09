#pragma once

#include <Arduino.h>

uint8_t modeCurrent();
bool modeSet(uint8_t mode);
uint16_t modeBpm();
bool modeSetBpm(uint16_t bpm);
void metronomeFrame(uint8_t &level, uint8_t &hue);
