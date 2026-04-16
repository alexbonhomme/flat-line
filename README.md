# Flat Line

## Hardware

- XIAO RP2040

## Convert to RAW

Convert a WAV/MP3 file into a 16-bit 44.1 kHz RAW file.

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -f s16le -acodec pcm_s16le output.raw
```
