#include "bell.h"
#include "config.h"
#include "radio.h"
#include "display.h"

static uint8_t macs[BELL_MAX_RECEIVERS][6];
static float brightF = FREQ_MIN_BRIGHTNESS;
static uint32_t lastMs = 0;

void bellFrame(const AudioFrame &frame) {
  int receivers = radioActiveHats(macs, BELL_MAX_RECEIVERS, BELL_ACTIVE_MS);
  int note = audioDominantNote(frame.gated);
  int winner = note < 0 ? -1 : note * (receivers + 1) / BELL_NOTES;
  broadcastBell(macs, receivers, winner);

  uint32_t now = millis();
  float dt = (float)(now - lastMs);
  lastMs = now;
  if (winner == 0) brightF = 255.0f;
  else brightF = max((float)FREQ_MIN_BRIGHTNESS, brightF - 255.0f * dt / BELL_DECAY_MS);
  renderSolid(0, (uint8_t)brightF);
}
