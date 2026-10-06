# macOS native latency fixes

JACK 1.9.22 omits CoreAudio stream latency from successful port-latency queries.
PortAudio's CoreAudio callback timestamps omit it even though `PaStreamInfo`
includes it. SSM's shared presentation clock uses these native timestamps; both
omissions can cause audible offsets between backends using the same device.

These patches fix the native metadata without adding an SSM compensation value.
They are local dependency patches, not released upstream fixes.

## Isolated build

On macOS, install the compiler tools, Git, make and Aften first. The default
dependency prefix is `/opt/local`; override it with `NATIVE_DEPS_PREFIX` if needed.
JACK's bundled waf needs Python 3.6–3.11 (`WAF_PYTHON`, default `/usr/bin/python3`).

```sh
tools/native-audio/build-macos.sh "$HOME/.SSM/TEST-native-latency"
source "$HOME/.SSM/TEST-native-latency/env.sh"
```

The script fetches exact revisions, applies the patches, builds and runs their
focused regressions, and installs below the new prefix. It refuses an existing
prefix. Logs and source checkouts remain there for inspection. `JOBS` sets build
parallelism (default 4). Network access to GitHub is required.

Pinned bases:

- JACK 1.9.22: `4f58969432339a250ce87fe855fb962c67d00ddb`
- PortAudio: `88ab584e7bf4358599744cd662cfbc978f41efbf`

Source `env.sh` in a fresh shell before configuring and running SSM. It sets
`PKG_CONFIG_PATH`, `CPPFLAGS`, `LDFLAGS`, `DYLD_LIBRARY_PATH`, `PATH` and
`JACK_DRIVER_DIR` for that shell. Add other dependency paths before sourcing it,
so the patched libraries take precedence over an existing installation.
Build the core and audio modules from the same SSM branch. PortAudio must load the
patched library; JACK must run the patched server and CoreAudio driver. Existing
applications and JACK servers do not change when this file is sourced. Select a
separate named JACK server for testing (use a short name such as `ssmn` to avoid
JACK's macOS semaphore-name limit), or stop and replace your own server when
appropriate; the script does neither.

Use the opt-in `native-loopback` and `jack-alignment-live` tests documented in
the audio library's `tests/LOOPBACK.md`. Verify physical alignment as well as
reported latency. Close the test shell and remove the entire test prefix after
stopping applications and test servers using it. System libraries are untouched.

## Scope

The JACK patch maps each port to its physical channel's stream latency, including
interleaved streams and channel remapping. Encoded AC3 ports use the maximum of
their two physical channels. Latencies refresh during the driver's existing
update path; reopen the driver after a hardware stream configuration change.

PortAudio caches stream latency outside its audio callback and listens for
latency and stream-list changes. It retains PortAudio's first-device-stream
convention for the single ADC/DAC timestamp. Heterogeneous aggregate devices with
different per-channel latencies still need a richer native timing contract.

These fixes do not establish absolute converter latency, independent-device
clock drift, or Linux backend behavior. They correct missing CoreAudio metadata.
