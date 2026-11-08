#pragma once

#include <Arduino.h>

bool radioBegin();
void broadcast(uint8_t level, uint8_t hue);
