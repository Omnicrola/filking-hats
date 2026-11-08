#include "config.h"
#include "display.h"
#include "audio.h"
#include "radio.h"

void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  displayBegin();
  startupChase();
  bool micOk = audioBegin();
  bool radioOk = radioBegin();
  DBG("mic %s, radio %s\n", micOk ? "ok" : "FAIL", radioOk ? "ok" : "FAIL");
}

void loop() {
  AudioFrame f = audioProcess();
  broadcast(f.level, f.hue);
  renderBar(f.level, f.hue);
  DBG("db %6.1f ref %6.1f lvl %3u f %6.0f hue %3u%s\n", f.db, f.refDb, f.level, f.freq, f.hue, f.gated ? " gated" : "");
}
