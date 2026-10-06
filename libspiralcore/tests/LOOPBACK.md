# Live output alignment

These tests are opt-in and are not run by `make check`. Build `jack-alignment-live`
and `native-loopback` in the audio library directory (`libspiralcore` in public,
`Foundation/Audio` in private), with JACK enabled.

`jack-alignment-live` connects two independent output clients to its own JACK
capture client. It does not connect to speakers. It compares their samples at
three graph rates/periods, including resampling and periods above and below the
server period. A running JACK server is required. `JACK_DEFAULT_SERVER` can select
an isolated server. Passing this test verifies alignment inside JACK, not at a DAC.

`native-loopback` plays quiet, alternating 120 ms chirps through two selected
backends and records the default microphone for about eleven seconds:

```sh
./native-loopback "$HOME/.SSM/TEST-my-branch" jack coreaudio /tmp/ssm-loopback
python3 tests/analyze-loopback.py /tmp/ssm-loopback --tolerance-ms 1
```

The module root contains `audio/CoreAudio/CoreAudio_Audio.so`, etc.; do not pass
the `audio` subdirectory itself. Install modules and their matching core library
from the same branch. The optional final argument chooses a different capture
backend (for example `portaudio`); the default is `coreaudio`. Input and output
devices use each backend's defaults. JACK output connects only the test client's
ports to `system:playback_1` and `system:playback_2`; client teardown removes those
connections. Existing routes are not modified.

Use a cabled loopback for precise hardware validation. Speaker-to-microphone
capture can identify relative offsets when both outputs use the same speaker and
recording path. Start at a comfortable low hardware volume. Do not enable input
monitoring. Multiple physical speakers/devices require controlled path lengths.

The result files are native-endian `.f32` mono samples, `.f64` per-sample capture
timestamps, and `.txt` metadata. The recording includes ambient microphone audio;
keep it local and delete both raw files after extracting measurements. The
analyzer requires Python 3 and NumPy and prints a JSON report. Positive relative
offset means the second backend arrives later, after subtracting the intentional
500 ms separation. It checks cycles 1 through 8, excluding initial startup.

A pass needs at least six pairs with correlation at least 0.6 and every accepted
pair within the requested tolerance. Silence, unrelated noise, invalid timestamps
and incomplete recordings cannot pass. Exit status 1 means the alignment/quality
requirement was not met. Validate the analyzer itself with:

```sh
python3 tests/analyze-loopback.py --self-test
```

The native tool's stream error counters include startup/priming; they are not
steady-state dropout counts. A good relative offset does not establish absolute
ADC/DAC latency, long-term drift across different hardware clocks, or freedom from
dropouts under load. Native driver latency metadata must be correct: a shared
presentation clock cannot compensate for latency omitted by a backend.
