# Output audio backends

Output uses the shared audio registry and presentation timeline. PortAudio, CoreAudio, ALSA, OSS, PipeWire and ESD are optional modules under `audio/<Backend>/`. JACK and Dummy remain built in; the named JACK device keeps its existing UI. Native callbacks transfer already-rendered samples and never run the synth graph.

Configure builds the modules whose dependencies are available. `--disable-portaudio`, `--disable-coreaudio`, `--disable-alsa-output`, `--disable-oss-output`, `--disable-pipewire` and `--disable-esd` opt out individually. Only the PipeWire module requires C++11. Device selection accepts the backend's device name, index or path. The AudioClient preference remains in `~/.spiralmodular`; backend settings are not added to legacy `.ssm` files.

All Output instances in the public host share its selected session. Each registry creation owns a separate native client, so independent engines cannot close each other's stream. The common presentation timeline aligns Output and named JACK endpoints through timestamped queues and rate conversion. ESD and PipeWire capture timing are estimated; physical alignment still needs loopback testing.

Rebuild the host, libraries and plugins together. See [audio transport and module ownership](../../../libspiralcore/AUDIO-TRANSPORT.md) for the lifetime contract and tests.

Source: private upstream_patching commit 033d3dd53c852f6b6cbad5358c192257d24693b8,
with file paths mapped to the public layout. That source extraction preserves
its original author, committer, timestamps and message. A separate adaptation
commit handles this tree's build, preferences and includes; also ports the
stable representative idea from private master and corrects partial transfers,
ALSA recovery, stale buffers and error-state handling. The source message's
reference to .ssm version 5 describes the private branch; this backport
intentionally retains the public branch's existing patch format.
