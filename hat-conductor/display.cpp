#include "display.h"
#include "config.h"
#include <FastLED.h>

DEFINE_GRADIENT_PALETTE(hatPalette_gp){
    0,   255, 0,   0,
    85,  255, 140, 0,
    170, 0,   255, 0,
    255, 255, 255, 255};

static CRGB leds[NUM_LEDS];
static CRGBPalette256 palette = hatPalette_gp;
static float peakPos = 0;
static uint32_t peakHoldUntil = 0;
static uint32_t lastRenderMs = 0;

void displayBegin() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_POWER_MA);
  lastRenderMs = millis();
}

void startupChase() {
  for (int i = 0; i < NUM_LEDS; i++) {
    FastLED.clear();
    leds[i] = CRGB::White;
    FastLED.show();
    delay(CHASE_STEP_MS);
  }
  FastLED.clear();
  FastLED.show();
  lastRenderMs = millis();
}

void renderBar(uint8_t level, uint8_t hue) {
  uint32_t now = millis();
  float dt = (now - lastRenderMs) / 1000.0f;
  lastRenderMs = now;
  int lit = (level * NUM_LEDS + 127) / 255;
  if (lit >= peakPos) {
    peakPos = lit;
    peakHoldUntil = now + PEAK_HOLD_MS;
  } else if (now > peakHoldUntil) {
    peakPos = max(0.0f, peakPos - PEAK_FALL_LEDS_PER_S * dt);
  }
  CRGB c = palette[hue];
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < lit; i++) leds[i] = c;
  int p = (int)ceilf(peakPos) - 1;
  if (p >= lit && p < NUM_LEDS) leds[p] = c;
  FastLED.show();
}

void renderOff() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  peakPos = 0;
}

void renderSpectrum(const uint8_t *bands) {
  for (int i = 0; i < NUM_LEDS; i++) {
    CRGB c = palette[(uint8_t)(i * 255 / (NUM_LEDS - 1))];
    c.nscale8_video(max(FREQ_MIN_BRIGHTNESS, bands[i]));
    leds[i] = c;
  }
  FastLED.show();
  peakPos = 0;
}

void renderSolid(uint8_t hue, uint8_t brightness) {
  CRGB c = palette[hue];
  c.nscale8_video(brightness);
  fill_solid(leds, NUM_LEDS, c);
  FastLED.show();
  peakPos = 0;
}
