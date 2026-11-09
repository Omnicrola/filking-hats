#include "config.h"
#include "settings.h"
#include "display.h"
#include "audio.h"
#include "radio.h"
#include "web.h"
#include <WiFi.h>

void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  settingsBegin();
  displayBegin();
  startupChase();
  bool micOk = audioBegin();
  bool radioOk = radioBegin();
  webBegin();
  DBG("conductor v%u mic %s, radio %s ch %u, web http://%s\n", FIRMWARE_VERSION, micOk ? "ok" : "FAIL",
      radioOk ? "ok" : "FAIL", radioChannel(), WiFi.softAPIP().toString().c_str());
}

void loop() {
  AudioFrame f = audioProcess();
  broadcast(f.level, f.hue);
  renderBar(f.level, f.hue);
  webUpdate(f);
  webHandle();
  DBG("db %6.1f ref %6.1f lvl %3u f %6.0f hue %3u rx %d%s\n", f.db, f.refDb, f.level, f.freq, f.hue,
      radioReceiverCount(), f.gated ? " gated" : "");
}
