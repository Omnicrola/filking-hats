#pragma once

#include <Arduino.h>

void displayBegin();
void startupChase();
void renderBar(uint8_t level, uint8_t hue);
void renderOff();
void renderSpectrum(const uint8_t *bands);
void renderSolid(uint8_t hue, uint8_t brightness);
void renderTimeout();
void renderRanged(uint8_t strength);
