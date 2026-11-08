# Acoustic Santa Filking Hats

One **conductor** hat listens through an I2S microphone and drives a VU-style LED bar. It broadcasts the level and color over ESP-NOW to any number of **receiver** hats, which mirror its display.

## Hardware

Board: Seeed XIAO ESP32S3 (113991114) on every hat.

| Signal | XIAO pad | GPIO | Notes |
|---|---|---|---|
| WS2812B data | D8 | 7 | Both hats. 330 Ω series resistor at the strip end recommended |
| Mic DOUT | D9 | 8 | Conductor only |
| Mic BCLK | D6 (TX) | 43 | Conductor only |
| Mic LRCL / WS | D7 (RX) | 44 | Conductor only |
| Mic SEL | GND | — | Selects the left channel; the code reads left |
| Mic 3V | 3V3 | — | |

- D6/D7 are UART0, but the sketches use the native USB port for `Serial`, so the pins are free. At reset, the boot ROM briefly prints on GPIO43; this is harmless to the mic.
- Strips: 15 × WS2812B, GRB order.

## Power

Supply: **1000 mAh single-cell LiPo** (about 4.2 V full, 3.7 V nominal, about 3.5 V near empty).

- **Runtime.** The radio stays on continuously (about 80–100 mA), plus up to `LED_POWER_MA` (250 mA) for the LEDs. Expect roughly **3–5 hours** per charge.
- **Discharge rate.** Peak draw is about 0.5 A, well within the 1C (1 A) rating of a typical 1000 mAh cell.
- **Low voltage.** WS2812B parts are specified down to 3.5 V. Near the end of a charge, blue and white fade first, then flicker appears. That's the cue to recharge.
- **Brightness.** `LED_POWER_MA` = 250 allows full brightness on a full bar at `LED_BRIGHTNESS` 64. Lower it to extend runtime.

## Software setup

- Arduino IDE with **esp32 by Espressif core 3.x**. Earlier cores use a different I2S/ESP-NOW API and will not compile.
- Board: `XIAO_ESP32S3`. Set **USB CDC On Boot: Enabled** to see debug output.
- Libraries: **FastLED ≥ 3.9** (needed for ESP32-S3 RMT on core 3.x) and **arduinoFFT ≥ 2.0**.
- Debug output at 115200 baud. Set `#define DEBUG 0` in either sketch to turn it off.

## Behavior

**Both hats at power-on:** a single white LED chases up the strip at 100 ms per LED, then the strip turns off.

**Conductor:**
1. Samples the mic at 16 kHz, 32-bit slots (24-bit data).
2. Applies a 2nd-order Butterworth high-pass filter at 100 Hz.
3. Processes 512-sample blocks, about 31 frames per second:
   - Converts RMS to dBFS. Below `GATE_DBFS` the bar drops to 0.
   - AGC: the top of the display tracks recent peak loudness. It rises instantly, falls at `AGC_DECAY_DB_PER_S`, and never drops below `MIN_REF_DBFS`. The bar covers the top `DISPLAY_SPAN_DB` dB.
   - Level smoothing: fast attack, linear release.
   - FFT peak between 100 Hz and 4 kHz, mapped on a log scale to palette index 0–255, then smoothed. During silence the last color is kept.
4. Broadcasts one packet per frame and renders its own bar.

**Receiver:**
- Renders at 100 Hz. It smooths toward the latest level and keeps using the last values between packets.
- If nothing valid arrives for **5 s**, the whole strip turns solid red. This includes the first 5 s after boot. Normal display resumes on the next valid packet.

**Display:** the bar fills from LED 0 upward in a single palette color, with a peak dot that holds 300 ms and then falls.

## File layout

Each sketch folder is self-contained. The Arduino IDE opens the extra files as tabs and compiles them automatically.

| File | Conductor | Receiver |
|---|---|---|
| `hat-*.ino` | `setup()` and `loop()` only | `setup()` and `loop()`, including the timeout logic and receive-side smoothing |
| `config.h` | Pins, tuning constants, debug macro, `HatPacket` | Same, plus receiver timing constants |
| `display.h/.cpp` | LED setup, startup chase, bar and peak-dot rendering, palette | Same, plus `renderTimeout()` (solid red) |
| `audio.h/.cpp` | I2S mic, high-pass filter, RMS/AGC level, FFT hue | — |
| `radio.h/.cpp` | ESP-NOW setup and `broadcast()` | ESP-NOW setup, receive callback, thread-safe `radioSnapshot()` |

## Must stay in sync between the two sketches

There is no shared library. These pieces are duplicated and must match exactly:

- In `config.h`: the `HatPacket` struct layout, `PACKET_MAGIC`, `GROUP_ID` and `ESPNOW_CHANNEL`. For matching looks, also `NUM_LEDS`, `LED_BRIGHTNESS`, `LED_POWER_MA` and the peak-dot constants.
- In `display.cpp`: the receiver's copy is the conductor's plus `renderTimeout()`. Any change to `hatPalette_gp` or `renderBar()` must be made in both.

## Radio protocol

- ESP-NOW broadcast to `FF:FF:FF:FF:FF:FF` on Wi-Fi channel 2, with no access point.
- 6-byte packed little-endian packet:

| Byte | Field | Meaning |
|---|---|---|
| 0–1 | magic | `0x5A48` |
| 2 | group | Hat group ID. Receivers ignore other groups, so two conductor sets can share a room |
| 3 | seq | Increments per packet. The receiver uses it to count lost packets |
| 4 | level | 0–255 bar height |
| 5 | hue | 0–255 palette index |

## Palette

The palette is a 256-entry gradient (FastLED `CRGBPalette256`): red (bass) → orange/gold → green → white (treble). To change the colors, edit `hatPalette_gp` in both sketches.

## Tuning (conductor)

| Constant | Effect |
|---|---|
| `GATE_DBFS` | Raise it if room noise lights the bar; lower it for more sensitivity |
| `MIN_REF_DBFS` | Lowest the AGC adapts to. Raise it so quiet rooms don't look "full" |
| `DISPLAY_SPAN_DB` | Dynamic range of the bar. Smaller means jumpier |
| `AGC_DECAY_DB_PER_S` | How fast it re-adapts after loud passages |
| `LEVEL_RELEASE_PER_S` | How fast the bar falls |
| `HUE_SMOOTHING` | 0–1. Lower means calmer color changes |

## Troubleshooting

- **Conductor bar never moves and the debug output shows about -180 dB.** The mic channel or wiring is wrong. Check that SEL is on GND, and that BCLK and WS are not swapped.
- **Receivers stay red.** Check that the conductor prints `radio ok`, that both sketches have the same magic, group and channel, and that the receiver's `rx` count increases.
