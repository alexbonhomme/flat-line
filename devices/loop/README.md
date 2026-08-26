## The Loop – RP2040 Dual‑Core WAV Player

RP2040 firmware that plays `.wav` files from a microSD card over I2S. Core 0 decodes from SD; core 1 streams to the DAC with pot‑controlled pitch and volume.

### Features

- **Dual‑core**: Core 0 scans SD and fills decode buffers; core 1 runs pitch interpolation and I2S output.
- **Controls**: `ResponsiveAnalogRead` on pitch (A0) and volume (A1) — pitch 0.1×–3×, volume 0–100%.
- **Playback**: 16‑bit stereo WAV from SD root via `SdFat` + `Adafruit_WavePlayer`.
- **Output**: I2S to MAX98357A (mono) or dual modules (stereo).

### Hardware

Targets **Seeed XIAO RP2040** + microSD on SPI + I2S amp (see `hardware/` for custom PCB).

| Signal | Pin |
|--------|-----|
| I2S BCLK | GPIO6 |
| I2S LRCLK | GPIO7 |
| I2S DATA | A3 (GPIO29) |
| SD CS | A2 |
| Pitch pot | A0 |
| Volume pot | A1 |

Power: LiPo + TP4056 + hardware switch. See `POWER_BUDGET.md` for runtime estimates.

### Layout

- `src/main.cpp` — dual‑core setup and file loop
- `src/file_reader.{h,cpp}` — SD init, root‑dir WAV scan
- `src/wav_player.{h,cpp}` — I2S, pitch shift, streaming
- `src/control_updater.{h,cpp}` — smoothed pot reads
- `src/app_config.h` — pins and constants
- `platformio.ini` — build env and deps
- `hardware/` — KiCad schematic and PCB
- `POWER_BUDGET.md` — battery runtime model

### Build

```bash
pio run
pio run -t upload
```

Environment: `seeed_xiao_rp2040`. Deps install on first build. Uncomment `-DDEBUG` in `platformio.ini` for serial logging (115200 baud).

### SD Card

FAT/FAT32 or exFAT. Copy **16‑bit PCM WAV** files to the root (`/`). Firmware plays all non‑hidden `.wav` files in order, 1 s gap between tracks.
