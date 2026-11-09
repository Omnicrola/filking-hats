#include "audio.h"
#include "config.h"
#include "settings.h"
#include <ESP_I2S.h>
#include <arduinoFFT.h>

static I2SClass i2s;
static int32_t rawSamples[SAMPLES];
static float vReal[SAMPLES];
static float vImag[SAMPLES];
static ArduinoFFT<float> fft(vReal, vImag, SAMPLES, (float)SAMPLE_RATE);

static float hpfCutoffHz = 0;
static float hb0, hb1, hb2, ha1, ha2;
static float hx1 = 0, hx2 = 0, hy1 = 0, hy2 = 0;

static float agcRefDb = MIN_REF_DBFS;
static float levelF = 0;
static float hueF = 0;

static void hpfInit(float cutoffHz) {
  hpfCutoffHz = cutoffHz;
  float w0 = 2.0f * PI * cutoffHz / SAMPLE_RATE;
  float cw = cosf(w0);
  float alpha = sinf(w0) / (2.0f * 0.70710678f);
  float a0 = 1.0f + alpha;
  hb0 = ((1.0f + cw) / 2.0f) / a0;
  hb1 = -(1.0f + cw) / a0;
  hb2 = hb0;
  ha1 = (-2.0f * cw) / a0;
  ha2 = (1.0f - alpha) / a0;
  hx1 = hx2 = hy1 = hy2 = 0;
}

static inline float hpf(float x) {
  float y = hb0 * x + hb1 * hx1 + hb2 * hx2 - ha1 * hy1 - ha2 * hy2;
  hx2 = hx1;
  hx1 = x;
  hy2 = hy1;
  hy1 = y;
  return y;
}

static void micReadBlock() {
  size_t want = sizeof(rawSamples);
  size_t got = 0;
  while (got < want) {
    size_t n = i2s.readBytes((char *)rawSamples + got, want - got);
    if (n == 0) delay(1);
    got += n;
  }
}

static float peakFrequency(float minHz) {
  fft.windowing(FFTWindow::Hann, FFTDirection::Forward);
  fft.compute(FFTDirection::Forward);
  fft.complexToMagnitude();
  float binHz = (float)SAMPLE_RATE / SAMPLES;
  int kMin = max(1, (int)ceilf(minHz / binHz));
  int kMax = min(SAMPLES / 2 - 2, (int)floorf(FREQ_MAX_HZ / binHz));
  int k = kMin;
  for (int i = kMin + 1; i <= kMax; i++) {
    if (vReal[i] > vReal[k]) k = i;
  }
  float a = vReal[k - 1], b = vReal[k], c = vReal[k + 1];
  float denom = a - 2.0f * b + c;
  float d = (fabsf(denom) > 1e-12f) ? 0.5f * (a - c) / denom : 0.0f;
  return (k + d) * binHz;
}

static uint8_t frequencyToHue(float f, float minHz) {
  f = constrain(f, minHz, FREQ_MAX_HZ);
  return (uint8_t)roundf(255.0f * logf(f / minHz) / logf(FREQ_MAX_HZ / minHz));
}

bool audioBegin() {
  hpfInit(settings().minFreqHz);
  i2s.setPins(MIC_BCLK_PIN, MIC_WS_PIN, -1, MIC_DIN_PIN);
  return i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
}

AudioFrame audioProcess() {
  const Settings &s = settings();
  if (s.minFreqHz != hpfCutoffHz) hpfInit(s.minFreqHz);

  micReadBlock();

  double sumSq = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float x = (rawSamples[i] >> 8) / 8388608.0f;
    float y = hpf(x);
    vReal[i] = y;
    vImag[i] = 0;
    sumSq += (double)y * y;
  }
  float rms = sqrtf(sumSq / SAMPLES);
  float db = 20.0f * log10f(rms + 1e-9f);

  if (db > agcRefDb) agcRefDb = db;
  else agcRefDb = max(MIN_REF_DBFS, agcRefDb - s.agcDecayDbPerS * FRAME_S);

  bool gated = db < s.gateDbfs;
  float target = 0;
  if (!gated) {
    float floorDb = agcRefDb - DISPLAY_SPAN_DB;
    target = constrain((db - floorDb) / DISPLAY_SPAN_DB, 0.0f, 1.0f) * 255.0f;
  }
  if (target > levelF) levelF += (target - levelF) * LEVEL_ATTACK;
  else levelF = max(target, levelF - s.releasePerS * FRAME_S);

  float freq = 0;
  if (!gated) {
    freq = peakFrequency(s.minFreqHz);
    hueF += (frequencyToHue(freq, s.minFreqHz) - hueF) * HUE_SMOOTHING;
  }

  AudioFrame frame;
  frame.db = db;
  frame.refDb = agcRefDb;
  frame.freq = freq;
  frame.level = (uint8_t)roundf(levelF);
  frame.hue = (uint8_t)roundf(hueF);
  frame.gated = gated;
  return frame;
}
