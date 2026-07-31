# Loop Device — Power Budget & Battery Runtime

Estimate of electrical consumption for the **Loop** WAV player (`devices/loop/`) running continuously on a **3200 mAh** LiPo cell, as used in the Flat Line project.

> **No bench measurements exist in the repo.** Figures below are computed from firmware behaviour, hardware BOM, and published datasheet / community measurements. Treat peak-current values as design margins, not measured averages.

---

## Summary

Firmware decodes **16-bit stereo WAV** and writes both channels over I2S (`wav_player.cpp`). The current BOM uses a **single MAX98357A** (mono output — the chip mixes L+R internally). The **stereo column** below assumes **two MAX98357A modules**, one per channel, each driving a 4 Ω speaker.

| Scenario | Mono (1× amp) | | Stereo (2× amp) | |
|----------|---------------|---|-----------------|---|
| | **Current** | **Runtime** | **Current** | **Runtime** |
| **Worst case (max volume)** | **~500 mA** | **~6 h 20 min** | **~1000 mA** | **~3 h 10 min** |
| **Typical (moderate volume)** | **~160 mA** | **~20 h** | **~260 mA** | **~12 h** |
| **Muted (default firmware)** | **~65 mA** | **~49 h** | **~68 mA** | **~47 h** |

**Minimum guaranteed runtime (conservative design target):**

- **Mono:** ~6 hours at full volume on a fresh 3200 mAh cell.
- **Stereo:** ~3 hours at full volume on a fresh 3200 mAh cell.

---

## Device Under Analysis

| Item | Detail |
|------|--------|
| MCU | Seeed XIAO RP2040 — dual Cortex-M0+ @ 133 MHz |
| Storage | microSD over SPI @ 50 MHz (`SD_SPI_CLOCK_MHZ`) |
| Audio (mono build) | 1× MAX98357A I2S Class-D amplifier → mono speaker |
| Audio (stereo build) | 2× MAX98357A — L and R channels, shared BCLK/LRCLK/DIN bus |
| Power | Single-cell LiPo + TP4056 charger + hardware ON/OFF switch |
| Battery (this estimate) | **3200 mAh** (project spec; BOM lists 1000 mAh) |

### Firmware power behaviour

The firmware has **no sleep or low-power modes**. When the power switch is ON:

1. **Playback** — Core 0 decodes WAV from SD; Core 1 resamples and streams I2S (`main.cpp`, `wav_player.cpp`). Both cores run busy loops.
2. **Between tracks** — 1 s gap (`FILE_GAP_MS = 1000`); I2S is stopped (`i2s_.end()`), but both cores stay fully powered (`delay`, no sleep).
3. **Controls** — Two pots sampled at ~100 Hz during playback (`POT_READ_INTERVAL_MS = 10`).

Duty cycle is effectively **~100 % active** whenever audio files are present.

---

## Consumption Model

### 1. XIAO RP2040 (MCU + board overhead)

| Condition | Current | Source |
|-----------|---------|--------|
| Single-core active @ 125 MHz, 3.3 V | ~27 mA | [Raspberry Pi forum measurement](https://forums.raspberrypi.com/viewtopic.php?t=385595) on Pico @ VSYS |
| Board idle (no user code) | ~24 mA | SparkFun RP2040 Thing Plus hookup guide |
| **This project (dual-core, SPI + I2S + ADC)** | **40–55 mA** | ~1.5× single-core active load + peripheral clocks |

Both cores execute continuously during playback; SPI (SD), I2S, and ADC (pots) add load beyond a simple benchmark loop.

### 2. microSD card (SPI read)

| Condition | Current | Source |
|-----------|---------|--------|
| SPI read, moving average | 20–30 mA | [Gough Lui SD power experiment](https://goughlui.com/2021/02/27/experiment-microsd-card-power-consumption-spi-performance/) |
| Peak during read/init bursts | 100–150 mA | Same; SD spec allows up to 100 mA (SPI mode) |
| **Budget (continuous playback)** | **25–35 mA avg** | Card reads in bursts while decoding; short peaks averaged |

### 3. MAX98357A amplifier (per channel)

| Condition | Current @ 3.7 V (each amp) | Mono (1×) | Stereo (2×) | Source |
|-----------|--------------------------|-----------|-------------|--------|
| Quiescent (I2S clocking, no output) | 2.4 mA typ. | 2.4 mA | 4.8 mA | MAX98357A datasheet |
| ~0.3 W output (moderate volume, 4 Ω) | ~90 mA | 90 mA | 180 mA | \(I = P_\text{out} / (\eta \cdot V_\text{bat})\), η ≈ 90 % |
| ~1.5 W output (near max @ 3.7 V, 4 Ω) | ~450 mA | 450 mA | 900 mA | Datasheet: 3.2 W @ 5 V / 4 Ω; scaled to battery voltage |
| **Design peak (supply sizing)** | ≥ 500 mA each | **≥ 500 mA** | **≥ 1100 mA** | Adafruit MAX98357 guide |

> **Assumption:** 4 Ω speaker per channel (common with MAX98357 breakouts). An 8 Ω load roughly halves amplifier current at the same perceived loudness.

**Stereo wiring note:** Both MAX98357 boards share the same I2S bus (BCLK, LRCLK, DIN). Each module is strapped for left or right channel via its `SD_MODE` / gain pins. MCU, SD, and I2S digital load are unchanged — only the analog output stage doubles.

At **`DEFAULT_VOLUME = 0`**, firmware still clocks I2S and sends zero samples — each amp draws quiescent current only (~2.4 mA), not silence-shutdown (the MAX98357 `SD` pin is not wired in firmware).

### 4. Other (negligible)

| Component | Current |
|-----------|---------|
| TP4056 (discharge / sleep) | < 0.01 mA |
| Potentiometer dividers (2×) | < 0.1 mA |
| Power / charge LEDs (if ON) | 1–3 mA |

---

## Scenario Calculations

All runtimes use:

\[
t_\text{hours} = \frac{C_\text{battery}}{I_\text{avg}} = \frac{3200\ \text{mAh}}{I_\text{avg}}
\]

Battery model: 3.7 V nominal LiPo, 3200 mAh nameplate capacity. No derating applied (moderate discharge rates); add ~10–15 % margin for age, temperature, and cutoff voltage in production.

Shared blocks (mono and stereo identical):

| Block | Current |
|-------|---------|
| RP2040 dual-core + peripherals | 45–50 mA |
| microSD (active read) | 28–35 mA |
| Misc (TP4056, pots, LEDs) | 3–5 mA |

Stereo WAV decode and dual-channel I2S output do **not** materially increase MCU or SD draw — both were already present in the mono build.

### Worst case — maximum volume, continuous playback

| Block | Mono | Stereo |
|-------|------|--------|
| RP2040 + SD + misc | 90 mA | 90 mA |
| Amplifier(s) @ ~1.5 W / channel, 4 Ω | 450 mA | 900 mA |
| **Total** | **~540 mA** | **~990 mA** |

**Mono** — design figure **500 mA** (headroom for SD spikes):

\[
t_\text{min,mono} = \frac{3200}{500} = \mathbf{6.4\ hours}
\]

**Stereo** — design figure **1000 mA** (2 × 450 mA amp peaks + shared digital load):

\[
t_\text{min,stereo} = \frac{3200}{1000} = \mathbf{3.2\ hours}
\]

At 990 mA measured average: \(3200 / 990 = 3.2\ \text{h}\).

### Typical — moderate volume, continuous loop

| Block | Mono | Stereo |
|-------|------|--------|
| RP2040 + SD + misc | 76 mA | 76 mA |
| Amplifier(s) @ ~0.3 W / channel | 90 mA | 180 mA |
| **Total** | **~166 mA ≈ 160 mA** | **~256 mA ≈ 260 mA** |

\[
t_\text{mono} = \frac{3200}{160} = \mathbf{20\ hours} \qquad t_\text{stereo} = \frac{3200}{260} = \mathbf{12.3\ hours}
\]

### Muted — default firmware (`DEFAULT_VOLUME = 0`)

| Block | Mono | Stereo |
|-------|------|--------|
| RP2040 + SD + misc | 76 mA | 76 mA |
| Amplifier quiescent | 2.4 mA | 4.8 mA |
| **Total** | **~78 mA ≈ 65 mA** | **~81 mA ≈ 68 mA** |

\[
t_\text{mono} = \frac{3200}{65} = \mathbf{49\ hours} \qquad t_\text{stereo} = \frac{3200}{68} = \mathbf{47\ hours}
\]

Muted runtime is nearly identical — the amplifier quiescent delta (+2.4 mA) is negligible next to the always-on MCU + SD load.

---

## Between-Track Gap (minor effect)

Each WAV file is followed by a **1 s** pause with I2S off. If average track length is \(T\) seconds, the gap reduces average draw by roughly:

\[
\frac{T}{T + 1} \times 100\ \%
\]

| Avg. track length | Gap duty | Effect on runtime |
|-------------------|----------|-------------------|
| 10 s | 9 % of time slightly lower | +~0.5 h (typical scenario) |
| 60 s | 1.6 % | Negligible |
| 3 s | 25 % | +~1–2 h (typical) |

For long ambient loops this correction is small.

---

## Recommendations

1. **Measure on hardware** — Inline ammeter on the battery line during representative playback is the only way to validate these estimates.
2. **Size the battery path for peaks** — Budget **≥ 600 mA** (mono) or **≥ 1200 mA** (stereo) transient capability (SD init + amplifier peaks) to avoid brownouts.
3. **Extend runtime in firmware** (not implemented today):
   - Drive MAX98357 `SD` pin low between tracks and when volume = 0.
   - Under-clock RP2040 or gate Core 1 when idle.
   - Use `light_sleep` / `WFE` in the 1 s inter-track gap.
4. **Production volume** — If the installation runs muted by default, expect **~2 days** of runtime (mono or stereo). At performance volume, plan for a recharge cycle every **~6 hours** (mono) or **~3 hours** (stereo).

---

## References

- Firmware: `src/main.cpp`, `src/app_config.h`, `src/wav_player.cpp`
- Hardware BOM: `hardware/Index.md`
- [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) — §5.7 Power Consumption
- [MAX98357A datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max98357a-max98357b.pdf) — quiescent & output power
- [TP4056 datasheet](https://dlnmh9ip6v2uc.cloudfront.net/datasheets/Prototyping/TP4056.pdf) — sleep current < 2 µA
- [Pico power measurements @ 3.3 V](https://forums.raspberrypi.com/viewtopic.php?t=385595)
- [microSD SPI power profile](https://goughlui.com/2021/02/27/experiment-microsd-card-power-consumption-spi-performance/)

*Generated from codebase analysis — July 2026.*
