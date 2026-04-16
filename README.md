# Flat Line

Flat Line is an art research project around sound and texture, initiated by Marine Penhouet with the assistance of Alexandre Bonhomme.

## Repository Architecture

- `devices/beat/`
  - AD8232 ECG monitor prototype for XIAO RP2040.
  - Displays waveform on SH1116 OLED and generates beat-synced audio.
  - Contains firmware (`src/`, `include/`, `platformio.ini`) and hardware design files (`hardware/`).
- `devices/loop/`
  - Dual-core WAV player for RP2040 with SD card + I2S output.
  - Includes pitch/volume control pipeline and board migration notes.
  - Contains firmware (`src/`, `platformio.ini`) and hardware docs/assets (`hardware/`, compatibility notes).
- `devices/tape/`
  - Work in progress.
- `devices/harddrive/`
  - Work in progress.
- `_archives/`
  - Legacy prototype sketches and older standalone experiments kept for reference.
- `wav_converter.py`
  - Utility script used to prepare audio assets for embedded playback workflows.

## Project Conventions

- Each active firmware project keeps its own `README.md` and `platformio.ini`.
- Firmware code lives inside each device folder (`devices/<name>/src`).
- Hardware design assets live under each device's `hardware/` folder.

## Quick Audio Conversion

Convert a WAV/MP3 file into a 16-bit 44.1 kHz RAW file:

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -f s16le -acodec pcm_s16le output.raw
```
