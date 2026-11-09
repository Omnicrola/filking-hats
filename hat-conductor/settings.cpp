#include "settings.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static Settings current;

static Settings defaults() {
  return {DEFAULT_GATE_DBFS, DEFAULT_MIN_FREQ_HZ, DEFAULT_LEVEL_RELEASE_PER_S, DEFAULT_AGC_DECAY_DB_PER_S};
}

static Settings clamped(Settings s) {
  s.gateDbfs = constrain(s.gateDbfs, GATE_DBFS_MIN, GATE_DBFS_MAX);
  s.minFreqHz = constrain(s.minFreqHz, MIN_FREQ_HZ_MIN, MIN_FREQ_HZ_MAX);
  s.releasePerS = constrain(s.releasePerS, LEVEL_RELEASE_MIN, LEVEL_RELEASE_MAX);
  s.agcDecayDbPerS = constrain(s.agcDecayDbPerS, AGC_DECAY_MIN, AGC_DECAY_MAX);
  return s;
}

void settingsBegin() {
  prefs.begin("hat", false);
  Settings d = defaults();
  Settings s;
  s.gateDbfs = prefs.getFloat("gate", d.gateDbfs);
  s.minFreqHz = prefs.getFloat("minFreq", d.minFreqHz);
  s.releasePerS = prefs.getFloat("release", d.releasePerS);
  s.agcDecayDbPerS = prefs.getFloat("agcDecay", d.agcDecayDbPerS);
  current = clamped(s);
}

const Settings &settings() {
  return current;
}

void settingsSave(const Settings &s) {
  current = clamped(s);
  prefs.putFloat("gate", current.gateDbfs);
  prefs.putFloat("minFreq", current.minFreqHz);
  prefs.putFloat("release", current.releasePerS);
  prefs.putFloat("agcDecay", current.agcDecayDbPerS);
}

void settingsReset() {
  prefs.clear();
  current = defaults();
}
