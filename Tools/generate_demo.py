#!/usr/bin/env python3
"""Create a synthetic stereo WAV for a repeatable meter demonstration."""

import argparse
import math
from pathlib import Path
import struct
import wave


def write_demo(output: Path) -> None:
    sample_rate = 48_000
    amplitudes = (0.03, 0.10, 0.50, 0.90, 0.25, 0.0, 0.05, 0.40)
    cycle = bytearray()
    for second, amplitude in enumerate(amplitudes):
        for index in range(sample_rate):
            phase = 2.0 * math.pi * 440.0 * (second * sample_rate + index) / sample_rate
            left = round(32767 * amplitude * math.sin(phase))
            right = round(32767 * amplitude * 0.5 * math.sin(phase))
            cycle.extend(struct.pack('<hh', left, right))

    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), 'wb') as audio:
        audio.setnchannels(2)
        audio.setsampwidth(2)
        audio.setframerate(sample_rate)
        audio.writeframes(cycle * 4)
    print(output.resolve())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('build/demo/Demo.wav'))
    write_demo(parser.parse_args().output)
