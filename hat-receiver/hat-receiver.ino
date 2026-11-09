#include "config.h"
#include "display.h"
#include "radio.h"

float levelF = 0;
uint32_t timeoutStartMs = 0;
uint32_t lastFrameMs = 0;
uint32_t lastDebugMs = 0;
uint32_t lastHeartbeatMs = 0;

void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  displayBegin();
  startupChase();
  bool radioOk = radioBegin();
  DBG("receiver v%u id %s radio %s ch %u\n", FIRMWARE_VERSION, radioHatId().c_str(), radioOk ? "ok" : "FAIL",
      radioChannel());
  timeoutStartMs = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrameMs < FRAME_MS) return;
  lastFrameMs = now;

  RadioState rx = radioSnapshot();
  now = millis();
  uint32_t since = now - (rx.any ? rx.lastRxMs : timeoutStartMs);
  bool timedOut = since > SIGNAL_TIMEOUT_MS;

  if (timedOut) {
    renderTimeout();
    levelF = 0;
  } else if (rx.any) {
    levelF += (rx.level - levelF) * LEVEL_SMOOTHING;
    renderBar((uint8_t)roundf(levelF), rx.hue);
  }

  radioUpdate(timedOut);

  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMs = now;
    radioSendHeartbeat();
  }

  if (now - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = now;
    DBG("ch %u rx %lu lost %lu lvl %3u hue %3u age %lums%s\n", radioChannel(), (unsigned long)rx.count,
        (unsigned long)rx.lost, rx.level, rx.hue, (unsigned long)since, timedOut ? " SCANNING" : "");
  }
}
