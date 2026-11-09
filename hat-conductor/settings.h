#pragma once

#include <Arduino.h>

struct Settings {
  float gateDbfs;
  float minFreqHz;
  float releasePerS;
  float agcDecayDbPerS;
};

void settingsBegin();
const Settings &settings();
void settingsSave(const Settings &s);
void settingsReset();
