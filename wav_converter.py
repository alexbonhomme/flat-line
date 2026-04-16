#!/usr/bin/env python3
import wave
import struct
import sys
import os
import numpy as np

# === CONFIG ===
TARGET_SAMPLE_RATE = 44100   # Hz, to match Arduino playback rate

def usage():
    print("Usage: ./wav_converter [--16bit] [--no-normalize] <input.wav> [output.h]")
    print("  --16bit        Output 16-bit signed samples (default: 8-bit unsigned)")
    print("  --no-normalize Skip peak normalization")
    print("  input.wav      Path to WAV file to convert")
    print("  output.h       Optional output header path (default: <input_basename>.h)")
    sys.exit(1)

args = [a for a in sys.argv[1:] if a not in ("--16bit", "-2", "--no-normalize")]
OUTPUT_16BIT = "--16bit" in sys.argv or "-2" in sys.argv
NO_NORMALIZE = "--no-normalize" in sys.argv

if len(args) < 1:
    usage()
INPUT_FILE = args[0]
if not os.path.isfile(INPUT_FILE):
    print(f"Error: input file not found: {INPUT_FILE}", file=sys.stderr)
    sys.exit(1)
if len(args) >= 2:
    OUTPUT_FILE = args[1]
else:
    OUTPUT_FILE = os.path.splitext(os.path.basename(INPUT_FILE))[0] + ".h"

# === Helper functions ===
def read_wav(filename):
    with wave.open(filename, 'rb') as wf:
        n_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        framerate = wf.getframerate()
        n_frames = wf.getnframes()
        data = wf.readframes(n_frames)
    return data, n_channels, sample_width, framerate, n_frames

def convert_to_mono(samples, n_channels):
    if n_channels == 1:
        return samples
    if n_channels == 2:
        return samples.reshape(-1, 2).mean(axis=1)
    raise ValueError(f"Unsupported channel count: {n_channels} (only mono and stereo are supported)")

# === Read and process WAV ===
data, n_channels, width, rate, n_frames = read_wav(INPUT_FILE)

# Decode based on bit depth
if width == 2:
    fmt = "<{}h".format(n_frames * n_channels)
    samples = np.array(struct.unpack(fmt, data), dtype=np.float32)
    samples /= 32768.0
elif width == 1:
    fmt = "<{}B".format(n_frames * n_channels)
    samples = np.array(struct.unpack(fmt, data), dtype=np.float32)
    samples = (samples - 128.0) / 128.0
elif width == 3:
    # 24-bit little-endian: 3 bytes per sample, sign-extend to 32-bit then normalize
    n_samples = n_frames * n_channels
    raw = np.frombuffer(data, dtype=np.uint8)
    raw = raw.reshape(-1, 3)
    lo, mid, hi = raw[:, 0], raw[:, 1], raw[:, 2]
    sample_int = lo.astype(np.int32) | (mid.astype(np.int32) << 8) | (hi.astype(np.int32) << 16)
    sample_int = np.where(sample_int >= 0x800000, sample_int - 0x1000000, sample_int)
    samples = sample_int.astype(np.float32) / 8388608.0
else:
    raise ValueError("Unsupported bit depth: {} bytes per sample".format(width))

samples = convert_to_mono(samples, n_channels)

# Resample if needed
if rate != TARGET_SAMPLE_RATE:
    import scipy.signal
    samples = scipy.signal.resample_poly(samples, TARGET_SAMPLE_RATE, rate)

# Remove DC offset (true constant bias, estimated from the full signal median)
dc = np.median(samples)
samples = samples - dc
if abs(dc) > 0.0001:
    print(f"ℹ️  DC offset removed: {dc:+.6f}")


# Peak normalize: scale so max absolute value is 1.0
if not NO_NORMALIZE:
    peak = np.max(np.abs(samples))
    if peak > 0:
        samples = samples / peak

samples = np.clip(samples, -1.0, 1.0)

# Symbol names from output filename (e.g. sample_a.h → sampleDataA, SAMPLE_A_LEN)
out_basename = os.path.splitext(os.path.basename(OUTPUT_FILE))[0].lower()
if out_basename == "sample_a":
    array_name, len_name, bits_name = "sampleDataA", "SAMPLE_A_LEN", "SAMPLE_A_BITS"
elif out_basename == "sample_b":
    array_name, len_name, bits_name = "sampleDataB", "SAMPLE_B_LEN", "SAMPLE_B_BITS"
elif out_basename == "sample_c":
    array_name, len_name, bits_name = "sampleDataC", "SAMPLE_C_LEN", "SAMPLE_C_BITS"
else:
    array_name, len_name, bits_name = "sampleData", "SAMPLE_LEN", "SAMPLE_BITS"

# === Write C header ===
with open(OUTPUT_FILE, "w") as f:
    f.write("#pragma once\n")
    f.write("// Converted from {}\n".format(INPUT_FILE))
    if OUTPUT_16BIT:
        # 16-bit signed: scale to -32767..32767 to avoid overflow
        samples_i16 = (samples * 32767.0).astype(np.int16)
        f.write("#define {} 16\n".format(bits_name))
        f.write("const int16_t {}[] = {{\n  ".format(array_name))
        for i, s in enumerate(samples_i16):
            f.write("{:d},".format(int(s)))
            if (i + 1) % 16 == 0:
                f.write("\n  ")
        f.write("\n};\n")
        f.write("const int {} = {};\n".format(len_name, len(samples_i16)))
        n_samples = len(samples_i16)
    else:
        # 8-bit unsigned
        samples_u8 = ((samples + 1.0) * 127.5).astype(np.uint8)
        f.write("#define {} 8\n".format(bits_name))
        f.write("const uint8_t {}[] = {{\n  ".format(array_name))
        for i, s in enumerate(samples_u8):
            f.write("{:d},".format(s))
            if (i + 1) % 16 == 0:
                f.write("\n  ")
        f.write("\n};\n")
        f.write("const int {} = {};\n".format(len_name, len(samples_u8)))
        n_samples = len(samples_u8)

print(f"✅ Conversion complete: {n_samples} samples ({16 if OUTPUT_16BIT else 8}-bit) written to {OUTPUT_FILE}")