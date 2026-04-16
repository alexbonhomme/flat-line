## The Loop – RP2040 Dual‑Core WAV Player

`loop` is a small RP2040-based audio project that plays a folder of `.wav` files from a microSD card through an I2S DAC. It uses both RP2040 cores: one core streams and decodes audio from the SD card, while the other core handles real‑time audio output and smooth, pot‑controlled pitch and volume.

### Features

- **Dual‑core architecture**:  
  - **Core 0**: Scans the SD card, opens WAV files, and keeps the decode buffer filled.  
  - **Core 1**: Runs the audio engine, performs software pitch‑shifting + interpolation, and streams samples to I2S.
- **Smooth controls**:  
  - `PITCH_POT_PIN` and `VOLUME_POT_PIN` are read using `ResponsiveAnalogRead` to avoid zipper noise.  
  - Pitch is continuously variable (about 0.5×–2×) and volume is 0–100%.
- **WAV playback from SD**:  
  - Uses `SdFat` and `Adafruit_WavePlayer` to read 16‑bit stereo WAV files.  
  - Iterates over all `.wav` files in the SD card root directory (skips hidden files).
- **I2S audio output**:  
  - Uses the Arduino `I2S` API and a simple I2S DAC / audio BFF.  
  - Sample‑rate‑preserving playback with software resampling for pitch changes.

### Hardware Overview

The default configuration targets **Seeed Studio XIAO RP2040** with an I2S DAC “audio BFF” and a microSD card.

- **MCU**: Seeed XIAO RP2040 (Arduino core via PlatformIO).  
- **Storage**: microSD card on SPI, chip‑select at `SD_CS_PIN` (see `src/file_reader.cpp`).  
- **Audio**: I2S DAC / amplifier connected to:
  - `pBCLK` = GPIO6 (bit clock)  
  - `pWS`   = GPIO7 (word‑select / LRCLK)  
  - `pDOUT` = A3 (GPIO29, I2S data)
- **Controls**:
  - `VOLUME_POT_PIN` = A0 (12‑bit ADC, 0–4095)  
  - `PITCH_POT_PIN`  = A1 (12‑bit ADC, 0–4095)

For physical PCB compatibility details (especially if you had a custom board for XIAO), see `PCB_PHYSICAL_COMPATIBILITY.md`.

### Repository Layout

- `src/main.cpp` – Core setup; manages dual‑core loops and reads smoothed potentiometers.  
- `src/file_reader.{h,cpp}` – SD card setup and iteration over `.wav` files in the root directory.  
- `src/wav_player.{h,cpp}` – I2S + WavePlayer integration, interpolation‑based pitch shifting, and audio streaming.  
- `platformio.ini` – PlatformIO environment configuration and library dependencies.  
- `PCB_PHYSICAL_COMPATIBILITY.md` – Mechanical and footprint notes for custom PCB work.

### Software Setup (PlatformIO)

1. **Install PlatformIO**  
   - Via VS Code extension or `pip install platformio`.

2. **Open the project**  
   - In PlatformIO/VS Code, open the `loop` folder as a project.

3. **Libraries**  
   Dependencies are declared in `platformio.ini`:

   - `SdFat` (from the Seeed/Adafruit URL in the platform definition)  
   - `Adafruit WavePlayer Library`  
   - `ResponsiveAnalogRead`

   PlatformIO will install these automatically on the first build.

4. **Build & upload (XIAO RP2040)**  
   - Select the `seeed_xiao_rp2040` environment (as defined in `platformio.ini`).  
   - Connect the board via USB, put it in bootloader mode if necessary.  
   - Run **Build** then **Upload** from the PlatformIO sidebar or with:

```bash
pio run
pio run -t upload
```

### Preparing the SD Card

- Format the card as FAT/FAT32 or exFAT (as supported by `SdFat`).  
- Copy one or more **16‑bit PCM WAV** files to the **root directory** (`/`).  
- The firmware will:
  - Open the root directory on startup.  
  - Iterate all files, printing their names when `DEBUG` is enabled.  
  - Play each file whose name ends with `.wav` (case‑insensitive), skipping hidden files and non‑WAVs.

### Using the Device

1. Power the board and insert the prepared SD card.  
2. The firmware initializes the SD card and starts scanning for WAV files.  
3. For each `.wav` file:
   - Core 0 streams and decodes the file.  
   - Core 1 continuously calls `WavPlayer::process()`, which:
     - Reads pot‑driven `pitch` and `volume` values,  
     - Interpolates between consecutive samples to achieve smooth pitch shifts,  
     - Sends stereo samples to I2S.
4. Adjust the **pitch** and **volume** knobs in real time while audio is playing.

### Debugging

- Uncomment `-DDEBUG` in `platformio.ini` `build_flags` to enable serial logging.  
- Serial output (115200 baud) will show SD initialization status and filenames as they are scanned.

