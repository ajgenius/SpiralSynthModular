OutputPlugin audio backport
===========================

PortAudio v19, ALSA PCM and OSS provide blocking audio I/O in libspiralcore.
The existing synth thread remains the clock's consumer; no PortAudio callback
or additional processing thread is introduced. JackPlugin remains separate.

Configure builds the available backends, preferring PortAudio, then ALSA,
then OSS. Options -> Audio Client selects among compiled backends. Output
Device accepts default, a PortAudio device index/name, an ALSA PCM name
(such as default or hw:0,0), or an OSS device path. Optional build flags:
--disable-portaudio, --disable-alsa-output, --disable-oss-output.

The AudioClient preference is saved in ~/.spiralmodular. Existing preferences
without it select the configured default; historical /dev/dsp defaults are
translated to default for non-OSS backends. Device names may contain spaces.
The existing .ssm format and plugin ID 0x0000 are retained. Backend settings
remain preferences and are not embedded in .ssm files.

Read, Write and Duplex use the same two input/two output ports and shared
master volume as the original OutputPlugin. All live Output instances mix
into one device stream. A stable live instance services it once per engine
cycle. Adding or deleting another Output does not close the active stream.
Changing audio settings resets the stream and discards old buffered samples.
A failed open or transfer closes the mode and releases the blocking flag;
select Read, Write or Duplex again after correcting the device settings.

Rebuild the host, widget library and all plugins together: HostInfo gained
an audio-client field. Do not mix old plugin binaries with the rebuilt host.

Source: private upstream_patching commit 033d3dd53c852f6b6cbad5358c192257d24693b8,
with file paths mapped to the public layout. That source extraction preserves
its original author, committer, timestamps and message. A separate adaptation
commit handles this tree's build, preferences and includes; also ports the
stable representative idea from private master and corrects partial transfers,
ALSA recovery, stale buffers and error-state handling. The source message's
reference to .ssm version 5 describes the private branch; this backport
intentionally retains the public branch's existing patch format.

Validation: C++03 host/core/OutputPlugin build with all three backends; OSS-only
build; noninteractive ALSA null-device lifecycle and failed-open tests;
ALSA short-transfer/recovery tests; PortAudio mocked failure/cleanup tests;
sanitized mixer/buffer tests; preference round-trip with spaced device names.
No GUI or physical audio device testing. GCC 3.4/4.1 were not available.

Adaptation code and this document generated or modified by ChatGPT.
