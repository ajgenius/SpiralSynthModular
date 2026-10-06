#!/usr/bin/env python3
"""Match native-loopback chirps; requires NumPy. Raw recordings stay local."""
import argparse
import json
from pathlib import Path

import numpy as np


def chirp(rate):
    time = np.arange(round(.12 * rate)) / rate
    return np.sin(np.pi * time / .12) ** 2 * np.sin(
        2 * np.pi * (700 * time + 9000 * time * time)
    )


def measure(samples, times, rate, epoch):
    template = chirp(rate)
    if (len(samples) != len(times) or len(samples) < rate * 9
            or not np.all(np.isfinite(samples)) or not np.all(np.isfinite(times))
            or np.any(np.diff(times) <= 0)):
        raise ValueError("Incomplete capture or invalid sample timestamps")

    # Valid cross-correlation lags locate the first sample of each chirp.
    size = 1 << (len(samples) + len(template) - 1).bit_length()
    correlation = np.fft.irfft(
        np.fft.rfft(samples, size) * np.conj(np.fft.rfft(template, size)), size
    )[:len(samples) - len(template) + 1]
    power = np.concatenate(([0.], np.cumsum(samples * samples)))
    windows = power[len(template):] - power[:-len(template)]
    normalized = np.abs(correlation) / np.sqrt(
        np.maximum(windows * np.sum(template * template), 1e-30)
    )

    pulses = []
    pairs = []
    for cycle in range(1, 9):
        pair = []
        for output in range(2):
            expected = epoch + cycle + .5 * output
            lo = max(0, int(np.searchsorted(times, expected - .2)))
            hi = min(len(normalized), int(np.searchsorted(times, expected + .2)))
            if hi <= lo:
                continue

            sample = lo + int(np.argmax(normalized[lo:hi]))
            pulse = dict(cycle=cycle, output=output,
                         delay_ms=float(1000 * (times[sample] - expected)),
                         correlation=float(normalized[sample]))
            pulses.append(pulse)
            pair.append(pulse)

        if len(pair) == 2 and min(p["correlation"] for p in pair) >= .6:
            pairs.append(dict(cycle=cycle,
                              relative_ms=pair[1]["delay_ms"] - pair[0]["delay_ms"]))

    result = dict(rate=rate, frames=len(samples), valid_pairs=len(pairs),
                  peak=float(np.max(np.abs(samples))),
                  rms=float(np.sqrt(np.mean(samples * samples))),
                  pulses=pulses, pairs=pairs)
    if pairs:
        offsets = np.array([p["relative_ms"] for p in pairs])
        result.update(median_relative_ms=float(np.median(offsets)),
                      min_relative_ms=float(offsets.min()),
                      max_relative_ms=float(offsets.max()),
                      stddev_ms=float(offsets.std()))

    return result


def passes(result, tolerance):
    return result["valid_pairs"] >= 6 and all(
        abs(p["relative_ms"]) <= tolerance for p in result["pairs"]
    )


def self_test():
    rate = 48000
    times = np.arange(rate * 11) / rate
    wave = chirp(rate)
    for offset in (0, 928, -928):
        samples = np.random.default_rng(7).normal(0, .0001, len(times))
        for cycle in range(1, 9):
            for output in range(2):
                start = round((1 + cycle + .5 * output) * rate) + output * offset
                samples[start:start + len(wave)] += .015 * wave

        result = measure(samples, times, rate, 1)
        assert result["valid_pairs"] == 8
        assert abs(result["median_relative_ms"] - 1000 * offset / rate) < .001
        assert passes(result, 1) == (offset == 0)

    silent = measure(np.zeros(len(times)), times, rate, 1)
    assert not passes(silent, 1)
    noise = measure(np.random.default_rng(8).normal(0, .01, len(times)), times, rate, 1)
    assert not passes(noise, 1)
    times[rate] = times[rate - 1]
    try:
        measure(samples, times, rate, 1)
    except ValueError:
        print("Loopback analysis: known signed offsets, silence, noise and invalid timing PASS")
        return

    raise AssertionError("Accepted nonmonotonic capture timestamps")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", nargs="?", help="native-loopback result prefix")
    parser.add_argument("--tolerance-ms", type=float, default=1.)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0

    if not args.prefix or not np.isfinite(args.tolerance_ms) or args.tolerance_ms < 0:
        parser.error("provide a result prefix and a finite nonnegative tolerance")

    stem = args.prefix
    info = dict(line.split("=", 1) for line in Path(stem + ".txt").read_text().splitlines())
    if int(info["errors"]):
        raise ValueError("Capture callback reported errors")

    samples = np.fromfile(stem + ".f32", dtype=np.float32).astype(float)
    times = np.fromfile(stem + ".f64", dtype=np.float64)
    if len(samples) != int(info["frames"]):
        raise ValueError("Capture file length differs from metadata")

    result = measure(samples, times, int(info["rate"]), float(info["epoch"]))
    result.update(first=info["first"], second=info["second"],
                  capture=info.get("capture", "coreaudio"),
                  tolerance_ms=args.tolerance_ms, passed=passes(result, args.tolerance_ms))
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
