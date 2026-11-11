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

static float specRing[SPECTRUM_SAMPLES];
static float sReal[SPECTRUM_SAMPLES];
static float sImag[SPECTRUM_SAMPLES];
static ArduinoFFT<float> specFft(sReal, sImag, SPECTRUM_SAMPLES, (float)SAMPLE_RATE);
static int bandLo[NUM_LEDS];
static int bandHi[NUM_LEDS];
static float bandRefDb = MIN_REF_DBFS;
static float bandF[NUM_LEDS];

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

static void bandBins(int i, int n, int &kLo, int &kHi) {
  float binHz = (float)SAMPLE_RATE / SPECTRUM_SAMPLES;
  float ratio = SPECTRUM_HIGH_HZ / SPECTRUM_LOW_HZ;
  float lo = SPECTRUM_LOW_HZ * powf(ratio, (float)i / n);
  float hi = SPECTRUM_LOW_HZ * powf(ratio, (float)(i + 1) / n);
  kLo = (int)ceilf(lo / binHz);
  kHi = (int)ceilf(hi / binHz) - 1;
  if (kHi < kLo) kLo = kHi = (int)roundf(sqrtf(lo * hi) / binHz);
}

static void bandsInit() {
  for (int i = 0; i < NUM_LEDS; i++) bandBins(i, NUM_LEDS, bandLo[i], bandHi[i]);
}

static void specCompute() {
  memcpy(sReal, specRing, sizeof(sReal));
  memset(sImag, 0, sizeof(sImag));
  specFft.windowing(FFTWindow::Hann, FFTDirection::Forward);
  specFft.compute(FFTDirection::Forward);
}

static float binPower(int kLo, int kHi) {
  float power = 0;
  for (int k = kLo; k <= kHi; k++) power += sReal[k] * sReal[k] + sImag[k] * sImag[k];
  return power;
}

static void pushSpectrumSamples() {
  float mean = 0;
  for (int i = 0; i < SAMPLES; i++) mean += (rawSamples[i] >> 8) / 8388608.0f;
  mean /= SAMPLES;
  memmove(specRing, specRing + SAMPLES, (SPECTRUM_SAMPLES - SAMPLES) * sizeof(float));
  float *dst = specRing + (SPECTRUM_SAMPLES - SAMPLES);
  for (int i = 0; i < SAMPLES; i++) dst[i] = (rawSamples[i] >> 8) / 8388608.0f - mean;
}

static void spectrumBands(const Settings &s, bool gated, uint8_t *out) {
  specCompute();
  float norm = 2.0f / (0.375f * (float)SPECTRUM_SAMPLES * (float)SPECTRUM_SAMPLES);
  float bandDb[NUM_LEDS];
  float maxDb = -200.0f;
  for (int i = 0; i < NUM_LEDS; i++) {
    float power = binPower(bandLo[i], bandHi[i]);
    bandDb[i] = 10.0f * log10f(power * norm + 1e-18f);
    maxDb = max(maxDb, bandDb[i]);
  }
  if (maxDb > bandRefDb) bandRefDb = maxDb;
  else bandRefDb = max(MIN_REF_DBFS, bandRefDb - s.agcDecayDbPerS * FRAME_S);
  float floorDb = bandRefDb - DISPLAY_SPAN_DB;
  for (int i = 0; i < NUM_LEDS; i++) {
    float target = gated ? 0.0f : constrain((bandDb[i] - floorDb) / DISPLAY_SPAN_DB, 0.0f, 1.0f) * 255.0f;
    if (target > bandF[i]) bandF[i] += (target - bandF[i]) * LEVEL_ATTACK;
    else bandF[i] = max(target, bandF[i] - s.releasePerS * FRAME_S);
    out[i] = (uint8_t)roundf(bandF[i]);
  }
}

static uint8_t frequencyToHue(float f, float minHz) {
  f = constrain(f, minHz, FREQ_MAX_HZ);
  return (uint8_t)roundf(255.0f * logf(f / minHz) / logf(FREQ_MAX_HZ / minHz));
}

bool audioBegin() {
  hpfInit(settings().minFreqHz);
  bandsInit();
  i2s.setPins(MIC_BCLK_PIN, MIC_WS_PIN, -1, MIC_DIN_PIN);
  return i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
}

AudioFrame audioProcess(bool spectrum) {
  const Settings &s = settings();
  if (s.minFreqHz != hpfCutoffHz) hpfInit(s.minFreqHz);

  micReadBlock();
  pushSpectrumSamples();

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
  if (spectrum) spectrumBands(s, gated, frame.bands);
  else memset(frame.bands, 0, sizeof(frame.bands));
  return frame;
}

int audioDominantBand(int bands, bool gated) {
  if (gated || bands < 1) return -1;
  specCompute();
  int best = -1;
  float bestPower = 0;
  for (int i = 0; i < bands; i++) {
    int kLo, kHi;
    bandBins(i, bands, kLo, kHi);
    float p = binPower(kLo, kHi);
    if (p > bestPower) {
      bestPower = p;
      best = i;
    }
  }
  return best;
}
