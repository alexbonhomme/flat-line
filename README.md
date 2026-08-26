# Flat Line

Art research project around sound and texture, by Marine Penhouet with Alexandre Bonhomme.

## Devices

Each device is a self-contained PlatformIO firmware project with KiCad hardware in `hardware/`.

- [`devices/beat/`](devices/beat/) — AD8232 ECG on XIAO RP2040. Dual-core: sampling + beat detection on core 0, SH1107 OLED on core 1. Beat-synced audio via PWM to a filtered mono jack. Custom PCB with Lipo Rider Plus power. *Not a medical device.*
- [`devices/loop/`](devices/loop/) — Dual-core WAV player. Core 0 reads 16-bit stereo WAV from SD; core 1 streams I2S with pot-controlled pitch (0.1×–3×) and volume. MAX98357A amp, LiPo power. See `POWER_BUDGET.md` for runtime estimates.
- [`devices/tape/`](devices/tape/) — Work in progress.
- [`devices/harddrive/`](devices/harddrive/) — Work in progress.

## Conventions

- Firmware: `devices/<name>/src/`, config in `app_config.h`, build via `platformio.ini` (`seeed_xiao_rp2040`).
- Hardware: KiCad schematic and PCB under `devices/<name>/hardware/`.
- Per-device docs: `devices/<name>/README.md`.

## Archives

- [`_archives/`](_archives/) — Legacy prototypes (e.g. original `the_loop` Arduino sketch).

## Audio assets

`wav_converter.py` — prepare WAV/MP3 for embedded playback (e.g. beat sample in `include/blackhole.h`).

Quick ffmpeg one-liner for 16-bit mono 44.1 kHz raw:

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -f s16le -acodec pcm_s16le output.raw
```
