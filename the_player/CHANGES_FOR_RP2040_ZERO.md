# Code Changes Required for RP2040-Zero

## File-by-File Changes

### 1. `src/wav_player.cpp`

**Line 5 - Change:**
```cpp
// BEFORE (XIAO RP2040):
#define pDOUT A3 // QT Py Audio BFF default DATA

// AFTER (RP2040-Zero):
#define pDOUT 29 // I2S Data Output (GPIO29, was A3 on XIAO)
```

### 2. `src/main.cpp`

**Line 12 - Change:**
```cpp
// BEFORE (XIAO RP2040):
#define PITCH_POT_PIN A1 // Analog pin for pitch potentiometer

// AFTER (RP2040-Zero):
#define PITCH_POT_PIN 27 // Analog pin for pitch potentiometer (GPIO27, was A1 on XIAO)
```

### 3. `platformio.ini`

**Complete replacement:**
```ini
# BEFORE (XIAO RP2040):
[env:seeed_xiao_rp2040]
platform = https://github.com/Seeed-Studio/platform-seeedboards.git
board = seeed-xiao-rp2040
framework = arduino
lib_deps =
  SdFat
  adafruit/Adafruit WavePlayer Library@^1.0.7
build_flags =
  ; -DDEBUG

# AFTER (RP2040-Zero) - Option 1: Using earlephilhower core (recommended):
[env:rp2040_zero]
platform = https://github.com/earlephilhower/arduino-pico/releases/download/4.5.2/package_rp2040_index.json
board = rpipico
framework = arduino
board_build.core = earlephilhower
lib_deps =
  SdFat
  adafruit/Adafruit WavePlayer Library@^1.0.7
build_flags =
  ; -DDEBUG

# AFTER (RP2040-Zero) - Option 2: Using standard Raspberry Pi platform:
[env:rp2040_zero]
platform = raspberrypi
board = pico
framework = arduino
lib_deps =
  SdFat
  adafruit/Adafruit WavePlayer Library@^1.0.7
build_flags =
  ; -DDEBUG
```

## Quick Reference: Pin Mapping

| XIAO RP2040 Pin | GPIO | RP2040-Zero Pin | Function |
|-----------------|------|-----------------|----------|
| D6 / Pin 6 | GPIO6 | GPIO6 | I2S BCLK |
| D7 / Pin 7 | GPIO7 | GPIO7 | I2S WS/LRCLK |
| A3 / D3 | GPIO29 | GPIO29 | I2S DATA |
| D0 / TX | GPIO0 | GPIO0 | SD Card CS |
| A1 / D1 | GPIO27 | GPIO27 | Pitch Potentiometer |

**Key Insight:** The GPIO numbers are identical! Only the pin naming convention differs.
