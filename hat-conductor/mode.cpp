#include "mode.h"
#include "config.h"

static uint8_t current = MODE_REACTIVE;

uint8_t modeCurrent() {
  return current;
}

bool modeSet(uint8_t mode) {
  if (mode >= MODE_COUNT) return false;
  current = mode;
  DBG("mode -> %u\n", current);
  return true;
}
