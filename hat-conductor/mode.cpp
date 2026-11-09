#include "mode.h"
#include "config.h"

static uint8_t current = MODE_REACTIVE;
static uint16_t bpm = DEFAULT_BPM;
static uint32_t beatStartMs = 0;

uint8_t modeCurrent() {
  return current;
}

bool modeSet(uint8_t mode) {
  if (mode >= MODE_COUNT) return false;
  if (mode == MODE_METRONOME && current != MODE_METRONOME) beatStartMs = millis();
  current = mode;
  DBG("mode -> %u\n", current);
  return true;
}

uint16_t modeBpm() {
  return bpm;
}

bool modeSetBpm(uint16_t value) {
  if (value < BPM_MIN || value > BPM_MAX) return false;
  if (value != bpm) beatStartMs = millis();
  bpm = value;
  return true;
}

void metronomeFrame(uint8_t &level, uint8_t &hue) {
  uint32_t beatMs = 60000UL / bpm;
  uint32_t elapsed = millis() - beatStartMs;
  uint32_t beat = elapsed / beatMs;
  uint32_t phase = elapsed % beatMs;
  uint32_t flashMs = min(METRONOME_FLASH_MS, beatMs / 2);
  level = phase < flashMs ? (uint8_t)(255 - phase * 255 / flashMs) : 0;
  hue = beat % METRONOME_BEATS_PER_BAR == 0 ? METRONOME_ACCENT_HUE : METRONOME_BEAT_HUE;
}
