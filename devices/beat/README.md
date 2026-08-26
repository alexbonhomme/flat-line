# AD8232 ECG + SH1107 + PWM Beat Prototype

ECG from AD8232 on Seeed XIAO RP2040, scrolling waveform on SH1107 I2C OLED, heartbeat-synced audio on PWM. Core 0 samples and detects beats; core 1 renders the display.

**Not a medical device** — experimentation only.

## Features

- 12-bit ECG sampling (`A0`, 500 Hz)
- Scrolling waveform + BPM on SH1107 128×64 I2C OLED
- R-peak detection with refractory period and rolling BPM
- Beat-triggered audio: embedded sample by default (`SamplePlayer`), optional sine PWM (`PwmBeeper`)

## Hardware

Schematic in `hardware/beat.kicad_sch` (see `beat.pdf`).

| Part | Value |
|------|-------|
| MCU | Seeed XIAO RP2040 |
| ECG | SparkFun AD8232 |
| Display | SH1107 128×64 I2C OLED (RST/CS/DC NC — SH1106-compatible driver) |
| Power | Seeed Lipo Rider Plus |
| Audio out | Mono jack (J2), PWM through onboard filter |

### XIAO connections

| Signal | Pin |
|--------|-----|
| ECG data | A0 |
| LO+ | D2 |
| LO− | D3 |
| I2C SDA / SCL | SDA / SCL |
| Audio PWM | D10 |

Power: LiPo → Lipo Rider Plus → 5 V to XIAO, 3.3 V to AD8232 and display.

Audio path: D10 → 1 kΩ + 220 nF low-pass (+ BAT43 clamp) → J2.

For breadboard bring-up, wire the AD8232 breakout and OLED to the same pins.

## Layout

- `src/main.cpp` — dual-core loop, beat trigger
- `src/ecg_processing.{h,cpp}` — ADC sampling and filtering
- `src/beat_detector.{h,cpp}` — R-peak detection, BPM
- `src/ecg_display.{h,cpp}` — SH1106 driver, waveform render
- `src/sample_player.{h,cpp}` — embedded WAV playback (default)
- `src/pwm_beeper.{h,cpp}` — sine PWM beep (optional)
- `src/app_config.h` — pins and tuning constants
- `include/blackhole.h` — embedded beat sample
- `hardware/` — KiCad schematic
- `platformio.ini` — build env and deps

## Build

```bash
pio run
pio run -t upload
```

Environment: `seeed_xiao_rp2040`. Uncomment `-DDEBUG` in `platformio.ini` for serial logging (115200 baud).

### Tuning (`src/app_config.h`)

- **ECG**: `ECG_SAMPLE_HZ`, `HP_BASELINE_ALPHA`, `SIGNAL_SMOOTH_ALPHA`
- **Detection**: `MIN_PEAK_THRESHOLD`, `THRESHOLD_GAIN`, `MIN_RISE_SLOPE`, `REFRACTORY_MS`
- **PWM beep**: `BEEP_FREQ_HZ`, `BEEP_DURATION_MS`, `BEEP_ATTACK_MS`, `BEEP_DECAY_MS`
- **Sample playback**: `SAMPLE_ATTACK_MS`, `SAMPLE_RELEASE_MS`, `SAMPLE_RETRIGGER_RELEASE_MS`

### Debug flags (`platformio.ini`)

- `-DDEBUG_FAKE_BPM=80` — fixed BPM, skips real detection
- `-DDEBUG_USE_PWM_BEEPER` — use `PwmBeeper` instead of `SamplePlayer`

## Troubleshooting

- **No display** — check I2C wiring and 3.3 V power. Driver: `U8G2_SH1106_128X64_NONAME_F_HW_I2C` (works with SH1107).
- **Flat/noisy ECG** — electrode contact, cable motion, LO+/LO− wiring. Remap `LO_P_PIN`/`LO_N_PIN` in `app_config.h` if needed.
- **Wrong BPM** — reduce motion; tune `MIN_PEAK_THRESHOLD`, `THRESHOLD_GAIN`, `MIN_RISE_SLOPE`.
- **No audio** — verify D10 and J2 wiring; plug into filtered jack output on the PCB.

## Reference

- [SparkFun AD8232 Hookup Guide](https://learn.sparkfun.com/tutorials/ad8232-heart-rate-monitor-hookup-guide/all)
