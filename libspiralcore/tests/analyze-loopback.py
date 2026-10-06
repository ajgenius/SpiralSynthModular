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


def measure(samples, times, rate, epoch, pair_count=8):
    if not 8 <= pair_count <= 300:
        raise ValueError("Expected 8 through 300 chirp pairs")

    template = chirp(rate)
    if (len(samples) != len(times) or len(samples) < rate * (pair_count + 1)
            or not np.all(np.isfinite(samples)) or not np.all(np.isfinite(times))
            or np.any(np.diff(times) <= 0)):
        raise ValueError("Incomplete capture or invalid sample timestamps")

    if times[0] > epoch + .8 or times[-1] < epoch + pair_count + .7:
        raise ValueError("Capture does not cover the requested chirp interval")

    # Valid cross-correlation lags locate the first sample of each chirp.
    size = 1 << (len(samples) + len(template) - 1).bit_length()
    correlation = np.fft.irfft(
        np.fft.rfft(samples, size) * np.conj(np.fft.rfft(template, size)), size
    )[:len(samples) - len(template) + 1]
    power = np.concatenate(([0.], np.cumsum(samples * samples)))
    windows = power[len(template):] - power[:-len(template)]
    # FFT/cumulative-sum roundoff in a silent window is not a detected chirp.
    # Without an energy floor, dividing two tiny residuals can exceed unity.
    audible = windows > max(float(windows.max()) * 1e-12, 1e-30)
    normalized = np.abs(correlation) / np.sqrt(
        np.maximum(windows * np.sum(template * template), 1e-30)
    )
    normalized[~audible] = 0

    pulses = []
    pairs = []
    for cycle in range(1, pair_count + 1):
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

    result = dict(rate=rate, frames=len(samples), requested_pairs=pair_count,
                  required_pairs=int(np.ceil(.75 * pair_count)), valid_pairs=len(pairs),
                  peak=float(np.max(np.abs(samples))),
                  rms=float(np.sqrt(np.mean(samples * samples))),
                  pulses=pulses, pairs=pairs)
    if pairs:
        offsets = np.array([p["relative_ms"] for p in pairs])
        result.update(median_relative_ms=float(np.median(offsets)),
                      min_relative_ms=float(offsets.min()),
                      max_relative_ms=float(offsets.max()),
                      stddev_ms=float(offsets.std()))
        if len(pairs) >= 2:
            cycles = np.array([p["cycle"] for p in pairs])
            # One cycle is one second: milliseconds/second * 1000 gives ppm.
            slope = np.polyfit(cycles, offsets, 1)[0]
            result["relative_slope_ppm"] = float(slope * 1000)

    return result


def passes(result, tolerance):
    return result["valid_pairs"] >= result["required_pairs"] and all(
        abs(p["relative_ms"]) <= tolerance for p in result["pairs"]
    )


def startup_errors(info, epoch):
    errors = int(info["errors"])
    # Keep startup diagnostics, but reject any failure in a measured search window.
    # Older recordings with errors have no timing evidence and remain failures.
    if errors:
        last = float(info.get("last_error_time", "inf"))
        if errors < 0 or not np.isfinite(last) or last >= epoch + .8:
            raise ValueError("Capture callback reported errors during measurement")
    return errors


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

    # A longer recording must expose drift and require proportional coverage.
    long_times = np.arange(rate * 33) / rate
    drifting = np.zeros(len(long_times))
    for cycle in range(1, 31):
        for output in range(2):
            start = round((1 + cycle + .5 * output + output * cycle * .0001) * rate)
            drifting[start:start + len(wave)] += .015 * wave

    result = measure(drifting, long_times, rate, 1, 30)
    assert result["valid_pairs"] == 30 and result["required_pairs"] == 23
    assert abs(result["relative_slope_ppm"] - 100) < 1
    assert not passes(result, 1)
    try:
        measure(drifting[:rate * 20], long_times[:rate * 20], rate, 1, 30)
    except ValueError:
        pass
    else:
        raise AssertionError("Accepted truncated long capture")

    assert startup_errors(dict(errors="3", last_error_time="1.7"), 1) == 3
    for info in (dict(errors="1"), dict(errors="1", last_error_time="1.8"),
                 dict(errors="1", last_error_time="nan")):
        try:
            startup_errors(info, 1)
        except ValueError:
            pass
        else:
            raise AssertionError("Accepted capture errors without a clean measurement interval")

    times[rate] = times[rate - 1]
    try:
        measure(samples, times, rate, 1)
    except ValueError:
        print("Loopback analysis: signed offsets, drift, silence, noise and incomplete/invalid timing PASS")
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
    epoch = float(info["epoch"])
    warmup_errors = startup_errors(info, epoch)

    samples = np.fromfile(stem + ".f32", dtype=np.float32).astype(float)
    times = np.fromfile(stem + ".f64", dtype=np.float64)
    if len(samples) != int(info["frames"]):
        raise ValueError("Capture file length differs from metadata")

    result = measure(samples, times, int(info["rate"]), epoch, int(info.get("pairs", 8)))
    result.update(first=info["first"], second=info["second"],
                  capture=info.get("capture", "coreaudio"),
                  startup_capture_errors=warmup_errors,
                  tolerance_ms=args.tolerance_ms, passed=passes(result, args.tolerance_ms))
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
