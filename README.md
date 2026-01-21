# Flat Line

## Hardward

- Teensy 4.0/4.1 + Audio Shield

## Covert to RAW

Convert a WAV/MP3 file into a 16bit 44100khz RAW file.

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -f s16le -acodec pcm_s16le output.raw
```
