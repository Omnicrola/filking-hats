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

## Versioning

Each sketch's `config.h` has a `FIRMWARE_VERSION` integer. **Increment it in both sketches every time you change either sketch**, even a one-line tweak, and flash every hat you update. The two numbers always match, so the version tells you exactly which firmware a hat runs:

- The conductor shows its own version at the top right of the control page.
- Each receiver sends its version in its heartbeat. The **Hats** view highlights in orange any receiver whose version differs from the conductor's, i.e. one that missed a reflash.
- Both hats print their version on the serial monitor at boot.

## Software setup

- Arduino IDE with **esp32 by Espressif core 3.x**. Earlier cores use a different I2S/ESP-NOW API and will not compile.
- Board: `XIAO_ESP32S3`. Set **USB CDC On Boot: Enabled** to see debug output.
- Libraries: **FastLED ≥ 3.9** (needed for ESP32-S3 RMT on core 3.x) and **arduinoFFT ≥ 2.0**.
- Debug output at 115200 baud. Set `#define DEBUG 0` in either sketch to turn it off.

## Behavior

**Both hats at power-on:** a single white LED chases up the strip at 100 ms per LED, then the strip turns off.

**Conductor:**
1. Samples the mic at 16 kHz, 32-bit slots (24-bit data).
2. Applies a 2nd-order Butterworth high-pass filter at the **minimum frequency** setting (default 100 Hz).
3. Processes 512-sample blocks, about 31 frames per second:
   - Converts RMS to dBFS. Below the **noise floor** setting the bar drops to 0.
   - AGC: the top of the display tracks recent peak loudness. It rises instantly, falls at the **gain recovery decay** setting, and never drops below `MIN_REF_DBFS`. The bar covers the top `DISPLAY_SPAN_DB` dB.
   - Level smoothing: fast attack, then falls at the **bar decay** setting.
   - FFT peak between the minimum frequency and 4 kHz, mapped on a log scale to palette index 0–255, then smoothed. During silence the last color is kept.
4. Broadcasts one packet per frame and renders its own bar.
5. Runs a Wi-Fi access point with a control page (see below), and counts receivers from their heartbeats.

**Receiver:**
- Renders at 100 Hz. It smooths toward the latest level and keeps using the last values between packets.
- Broadcasts a heartbeat once per second so the conductor can count it.
- If nothing valid arrives for **5 s**, the whole strip turns solid red. This includes the first 5 s after boot. Normal display resumes on the next valid packet.
- While red, it scans channels 1–11, staying 250 ms on each (`SCAN_DWELL_MS`), until it hears its conductor. A full sweep takes about 3 s. It then saves that channel to flash and starts there at next boot.
- Follows the conductor's channel beacon: if the conductor says to use a different channel, it switches at once and saves it.

**Display:** the bar fills from LED 0 upward in a single palette color, with a peak dot that holds 300 ms and then falls.

## Phone control page (conductor)

1. On your phone, join the Wi-Fi network **`conductor-hat`** with password **`jinglebells`**. Change both in `config.h` (`AP_SSID`, `AP_PASSWORD`; the password needs at least 8 characters). Your phone may warn that the network has no internet; stay connected anyway.
2. Browse to **http://192.168.4.1**.

The **☰** menu at the top left switches between three views: **Tuning**, **Hats** and **Modes**. You can bookmark any of them directly, e.g. `http://192.168.4.1/#tuning`, `#hats` or `#modes`.

### Tuning view

Refreshes twice a second and shows:
- **Receivers:** hats whose heartbeat arrived in the last 3 s (`RECEIVER_TIMEOUT_MS`).
- **dBFS:** the current mic level after the high-pass filter. 0 is the mic's maximum; quieter is more negative.
- **Hz:** the dominant frequency. It shows "–" while the sound is below the noise floor. **Tap it** to switch to the nearest note in standard 12-tone equal temperament (A4 = 440 Hz), shown with its octave number, e.g. `A4` or `C♯5`. Tap again to switch back. The phone's browser remembers the choice.
- A level bar mirroring the LEDs.

Settings (sliders, then **Save**):

| Setting | Range | Default | Effect |
|---|---|---|---|
| Noise floor | -90 to -20 dBFS | -65 | Below this, the bar shows nothing. Raise it if room noise lights the bar |
| Minimum frequency | 20–1000 Hz | 100 | High-pass filter cutoff and lowest pitch used for color |
| Bar decay | 20–2000 /s | 400 | How fast the bar falls, in level units per second (full bar = 255) |
| Gain recovery decay | 0.1–20 dB/s | 1.5 | How fast the meter regains sensitivity after loud passages |

- Saved settings are stored in flash and survive power cycles. **Reset to defaults** restores the `DEFAULT_*` values in `config.h`. The slider ranges come from the `*_MIN`/`*_MAX` constants there.
### Hats view

Refreshes once a second and lists every receiver heard since the conductor powered on. The list lives in RAM only and is cleared at power-off. Online hats are listed first, then strongest signal first. Each row shows:

| Column | Meaning |
|---|---|
| ID | Last 3 bytes of the receiver's MAC address, e.g. `A1:B2:C3`. Each receiver prints its own ID on the serial monitor at boot, so you can label the hats |
| Signal | Received signal strength of that hat's latest heartbeat, in dBm. Green ≥ -60 (strong), yellow -60 to -75 (fair), red < -75 (weak; expect lost packets) |
| Version | The receiver's `FIRMWARE_VERSION`. Orange if it differs from the conductor's |
| Last seen | Time since the last heartbeat. A green dot means online (heard within 3 s) |

The list holds up to 32 hats (`MAX_RECEIVERS`); hats beyond that are not listed.

**Broadcast channel.** The dropdown at the top of the Hats view moves every hat to another Wi-Fi channel without reflashing. After you confirm:
1. The conductor tells all receivers the new channel, repeatedly for 1.5 s (`CHANNEL_ANNOUNCE_MS`), on the old channel.
2. It then moves its access point and ESP-NOW to the new channel and saves it to flash. Receivers have already moved and resume within a moment.
3. Your phone is dropped and must rejoin `conductor-hat`. Most phones rejoin by themselves within a few seconds; reload the page if it stops updating.

Receivers that were off or out of range find the new channel by scanning (see Behavior), so they don't need to be present during the change. Channels go from 1 to 11 (`CHANNEL_MIN`/`CHANNEL_MAX`); 12–13 are left out because US phones can't see them. **Reset to defaults** does not change the channel.

### Modes view

One button per display mode; the active one has a gold outline. Tapping a button switches every receiver within one packet (about 30 ms).

The mode is **not saved**. The conductor always boots into **Reactive**, and a receiver boots into Reactive and stays there until it hears the conductor's first packet.

| Mode | Receivers | Conductor's own strip |
|---|---|---|
| Reactive | Mirror the conductor's level and pitch color (the normal behavior) | Normal bar |
| Sleep | All LEDs off; level and color in the packets are ignored. If the conductor goes silent while they sleep, they stay dark instead of turning red (they still scan for it) | Off |
| Disco | Bar height follows the music, but each receiver picks its own random palette color every second (`DISCO_COLOR_MS`) | Normal bar |
| Ranged | Only the first LED lights, colored by the signal strength of the conductor's packets: red at -90 dBm or weaker (`RANGE_RSSI_LOW`), through yellow, to green at -40 dBm or stronger (`RANGE_RSSI_HIGH`). Smoothed over about 0.2 s. Use it to walk the room and find where hats lose the conductor | Normal bar |

### Access limits

- At most 2 devices can join at once (`AP_MAX_CLIENTS`).
- The access point shares the radio with ESP-NOW, so they are always on the same channel. The Hats view's channel dropdown moves both.

## File layout

Each sketch folder is self-contained. The Arduino IDE opens the extra files as tabs and compiles them automatically.

| File | Conductor | Receiver |
|---|---|---|
| `hat-*.ino` | `setup()` and `loop()` only | `setup()` and `loop()`, including the timeout logic and receive-side smoothing |
| `config.h` | Pins, tuning constants, setting defaults and limits, access point name/password, debug macro, packet structs | Pins, timing constants, debug macro, packet structs |
| `display.h/.cpp` | LED setup, startup chase, bar and peak-dot rendering, palette | Same, plus `renderTimeout()` (solid red) |
| `audio.h/.cpp` | I2S mic, high-pass filter, RMS/AGC level, FFT hue | — |
| `radio.h/.cpp` | Access point and ESP-NOW setup, `broadcast()` (including the channel beacon and switch), the seen-hats list (ID, signal, version, last seen), `radioReceiverCount()` and `radioHats()` | ESP-NOW setup, receive callback, thread-safe `radioSnapshot()`, `radioSendHeartbeat()`, `radioHatId()`, `radioUpdate()` (channel following and scanning, saved to flash) |
| `settings.h/.cpp` | Web-adjustable settings and the broadcast channel, saved to flash | — |
| `mode.h/.cpp` | The current display mode (RAM only) | — |
| `web.h/.cpp` | Web server, the control page with its Tuning, Hats and Modes views (embedded HTML), and the JSON API (`/api/status`, `/api/hats`, `/api/settings`, `/api/reset`, `/api/channel`, `/api/mode`) | — |

## Must stay in sync between the two sketches

There is no shared library. These pieces are duplicated and must match exactly:

- In `config.h`: `FIRMWARE_VERSION`, the `HatPacket`, `HeartbeatPacket` and `ChannelPacket` struct layouts, `PACKET_MAGIC`, `HEARTBEAT_MAGIC`, `CHANNEL_MAGIC`, `GROUP_ID`, `DEFAULT_CHANNEL`, `CHANNEL_MIN`, `CHANNEL_MAX`, the `HatMode` values and `MODE_COUNT`. For matching looks, also `NUM_LEDS`, `LED_BRIGHTNESS`, `LED_POWER_MA` and the peak-dot constants.
- In `display.cpp`: the receiver's copy is the conductor's plus `renderTimeout()`. Any change to `hatPalette_gp` or `renderBar()` must be made in both.

## Radio protocol

- ESP-NOW broadcast to `FF:FF:FF:FF:FF:FF` on the broadcast channel: `DEFAULT_CHANNEL` (2) until changed from the Hats view, then whatever was last chosen, saved in flash on every hat. The conductor sends from its access point interface; receivers don't join the access point.
- Conductor → receivers, about 31 per second, 7-byte packed little-endian packet:

| Byte | Field | Meaning |
|---|---|---|
| 0–1 | magic | `0x5A48` |
| 2 | group | Hat group ID. Receivers ignore other groups, so two conductor sets can share a room |
| 3 | seq | Increments per packet. The receiver uses it to count lost packets |
| 4 | level | 0–255 bar height |
| 5 | hue | 0–255 palette index |
| 6 | mode | Display mode (see Modes view): 0 Reactive, 1 Sleep, 2 Disco, 3 Ranged. Receivers treat unknown values as Reactive |

- Receiver → broadcast, once per second, 5-byte heartbeat: `magic` = `0x5A52` (bytes 0–1), `group` (byte 2) and `version` (bytes 3–4, the receiver's `FIRMWARE_VERSION`). The conductor identifies each receiver by the sender's MAC address and records the signal strength (RSSI) of each heartbeat. Hats drop out of the online count after 3 s without a heartbeat but stay in the Hats list. Receivers ignore each other's heartbeats because the length differs.
- Conductor → receivers, 4-byte channel packet: `magic` = `0x5A43` (bytes 0–1), `group` (byte 2), `channel` (byte 3). Sent once per second with the current channel (`CHANNEL_BEACON_MS`), and with every frame for 1.5 s before a channel change, carrying the new channel. A receiver that hears a different channel than its own switches to it. This also corrects a receiver that locked onto a neighboring channel by picking up overlapping signal while scanning.

## Palette

The palette is a 256-entry gradient (FastLED `CRGBPalette256`): red (bass) → orange/gold → green → white (treble). To change the colors, edit `hatPalette_gp` in both sketches.

## Tuning (conductor)

Noise floor, minimum frequency, bar decay and gain recovery decay are set from the phone page. These remain compile-time constants in `config.h`:

| Constant | Effect |
|---|---|
| `MIN_REF_DBFS` | Lowest the AGC adapts to. Raise it so quiet rooms don't look "full" |
| `DISPLAY_SPAN_DB` | Dynamic range of the bar. Smaller means jumpier |
| `HUE_SMOOTHING` | 0–1. Lower means calmer color changes |

## Troubleshooting

- **Conductor bar never moves and the debug output shows about -180 dB.** The mic channel or wiring is wrong. Check that SEL is on GND, and that BCLK and WS are not swapped.
- **Receivers stay red.** Check that the conductor prints `radio ok`, that both sketches have the same magics and group, and that the receiver's `rx` count increases. While red, the receiver's debug line shows `SCANNING` and `ch` should cycle 1–11; if the conductor is on, it should lock on within about 3 s.
- **Receivers on v1 firmware after a channel change.** They don't understand channel packets or scanning, and stay on channel 2. Reflash them.
- **Page shows 0 receivers but receivers are lit.** The heartbeat isn't arriving. Check that `HEARTBEAT_MAGIC` and `GROUP_ID` match, and that the receivers were reflashed with the heartbeat version.
- **Page won't load.** Make sure the phone is still on `conductor-hat`; some phones drop networks with no internet. Turn off mobile data, or tell the phone to stay connected.
