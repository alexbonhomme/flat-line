# AD8232 ECG + SH1116 + PWM Beat Prototype

This project reads ECG data from an AD8232 module using a Seeed XIAO RP2040, shows a scrolling waveform on an SH1116 I2C OLED, and plays a heartbeat-synced beep using PWM output.

This is a prototype for experimentation only. It is not a medical device and must not be used for diagnosis or treatment.

## Features

- 12-bit ECG sampling from AD8232 (`A0`)
- Real-time scrolling waveform display on SH1116-compatible 128x64 OLED
- Basic beat detection (R-peak style) with refractory period
- Rolling BPM estimate
- Non-blocking PWM beep envelope on each detected beat

## Hardware

- Seeed XIAO RP2040
- AD8232 ECG module with electrode leads
- SH1116 I2C 128x64 OLED (SH1106 command-compatible modules also work)
- Passive buzzer / small amp input for audio
- Optional RC filter on PWM output

## Wiring

### AD8232 to XIAO RP2040

| AD8232 | XIAO RP2040 |
|---|---|
| 3.3V | 3V3 |
| GND | GND |
| OUTPUT | A0 |
| LO+ | D2 |
| LO- | D3 |

### SH1116 OLED (I2C) to XIAO RP2040

Use your board's hardware I2C pins:

| OLED | XIAO RP2040 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | SDA |
| SCL | SCL |

### PWM audio output

| Signal | XIAO RP2040 |
|---|---|
| PWM out | D10 |
| GND | GND |

For cleaner analog audio, use a simple RC low-pass filter on `D10` before the amplifier input.

Example starter values:

- `R = 2.2k ohm`
- `C = 10nF` to GND

## Build and upload

From the `the_beat` folder:

```bash
pio run
pio run -t upload
```

## Runtime tuning knobs

You can adjust constants in `src/app_config.h`:

- `ECG_SAMPLE_HZ`: ADC sampling rate
- `HP_BASELINE_ALPHA`, `SIGNAL_SMOOTH_ALPHA`: filtering behavior
- `MIN_PEAK_THRESHOLD`, `THRESHOLD_GAIN`, `MIN_RISE_SLOPE`, `REFRACTORY_MS`: beat detection sensitivity
- `BEEP_FREQ_HZ`, `BEEP_DURATION_MS`, `BEEP_ATTACK_MS`, `BEEP_DECAY_MS`: beep timbre and shape

Debug build flags in `platformio.ini`:

- `DEBUG_FAKE_BPM`: force a fixed BPM for testing visuals/audio.
- `DEBUG_USE_PWM_BEEPER`: `0` uses `SamplePlayer`, `1` uses `PwmBeeper`.

## Notes

- Display driver used is `U8G2_SH1106_128X64_NONAME_F_HW_I2C`, which matches many SH1116 modules in practice.
- If your display does not show anything, check:
  - I2C wiring (`SDA`, `SCL`, GND, 3V3)
  - Power voltage (3.3V logic-safe module)
  - Module controller/address compatibility
- If lead-off detection conflicts with your wiring, remap `LO_P_PIN` / `LO_N_PIN` in `src/app_config.h`.

## Troubleshooting

- **Noisy waveform**
  - Keep still and minimize cable movement.
  - Place electrodes as close to recommended positions as possible.
  - Use fresh electrode pads.
- **Flat line or unstable signal**
  - Check electrode contact and AD8232 wiring.
  - Verify `LO+`/`LO-` are connected and not permanently reporting leads off.
- **BPM seems incorrect**
  - Reduce motion artifacts.
  - Adjust detection constants (`MIN_PEAK_THRESHOLD`, `THRESHOLD_GAIN`, `MIN_RISE_SLOPE`).
- **No audio**
  - Confirm output on `D10`.
  - Verify buzzer/amp wiring and ground.
  - For line-level listening, ensure RC filter and amplifier are connected correctly.

## Reference

- [SparkFun AD8232 Hookup Guide](https://learn.sparkfun.com/tutorials/ad8232-heart-rate-monitor-hookup-guide/all)
