#include "config.h"
#include "display.h"
#include "radio.h"

float levelF = 0;
uint32_t timeoutStartMs = 0;
uint32_t lastFrameMs = 0;
uint32_t lastDebugMs = 0;
uint32_t lastHeartbeatMs = 0;
uint32_t lastDiscoMs = 0;
uint8_t discoHue = 0;
float rssiF = RANGE_RSSI_LOW;
float bandsF[NUM_LEDS];
float bellF = FREQ_MIN_BRIGHTNESS;

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
    if (rx.mode == MODE_SLEEP) renderOff();
    else renderTimeout();
    levelF = 0;
  } else if (rx.any) {
    rssiF += (rx.rssi - rssiF) * RSSI_SMOOTHING;
    switch (rx.mode) {
      case MODE_BELL: {
        if (rx.bellIndex >= 0 && rx.bellWinner == rx.bellIndex) bellF = 255.0f;
        else bellF = max((float)FREQ_MIN_BRIGHTNESS, bellF - 255.0f * FRAME_MS / BELL_DECAY_MS);
        uint8_t hue = (rx.bellIndex > 0 && rx.bellCount > 1) ? rx.bellIndex * 255 / (rx.bellCount - 1) : 0;
        renderSolid(hue, (uint8_t)bellF);
        levelF = 0;
        break;
      }
      case MODE_FREQUENCY: {
        uint8_t out[NUM_LEDS];
        for (int i = 0; i < NUM_LEDS; i++) {
          bandsF[i] += (rx.bands[i] - bandsF[i]) * LEVEL_SMOOTHING;
          out[i] = (uint8_t)roundf(bandsF[i]);
        }
        renderSpectrum(out);
        levelF = 0;
        break;
      }
      case MODE_METRONOME:
        levelF = rx.level;
        renderBar(rx.level, rx.hue);
        break;
      case MODE_RANGED: {
        float t = constrain((rssiF - RANGE_RSSI_LOW) / (RANGE_RSSI_HIGH - RANGE_RSSI_LOW), 0.0f, 1.0f);
        renderRanged((uint8_t)(t * 255));
        levelF = 0;
        break;
      }
      case MODE_SLEEP:
        renderOff();
        levelF = 0;
        break;
      case MODE_DISCO:
        if (now - lastDiscoMs >= DISCO_COLOR_MS) {
          lastDiscoMs = now;
          discoHue = esp_random() & 0xFF;
        }
        levelF += (rx.level - levelF) * LEVEL_SMOOTHING;
        renderBar((uint8_t)roundf(levelF), discoHue);
        break;
      default:
        levelF += (rx.level - levelF) * LEVEL_SMOOTHING;
        renderBar((uint8_t)roundf(levelF), rx.hue);
        break;
    }
  }

  radioUpdate(timedOut);

  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMs = now;
    radioSendHeartbeat();
  }

  if (now - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = now;
    DBG("ch %u mode %u rssi %.0f rx %lu lost %lu lvl %3u hue %3u age %lums%s\n", radioChannel(), rx.mode, rssiF,
        (unsigned long)rx.count, (unsigned long)rx.lost, rx.level, rx.hue, (unsigned long)since,
        timedOut ? " SCANNING" : "");
  }
}
