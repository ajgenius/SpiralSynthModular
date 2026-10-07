# Audio transport contract

`AudioClient` is a native I/O adapter. `AudioStream` moves already rendered audio
between its native callback (or a bounded polling worker) and the engine.
`PresentationClock` supplies one engine timeline. These classes have no GUI or
Spicy dependency and remain C++03 compatible.

## Time and ownership

`AudioCycleTiming` describes the first native sample's ADC and DAC times, in
seconds on `AudioMonotonicTime()`. `GetChannelTime` accounts for different port
latencies. Callback arrival, musical transport position, and physical playback
time are distinct. Drivers without hardware timestamps identify their queue-depth
estimate with `Estimated`; a native timestamp source takes precedence as master.

`AudioStamp` gives a graph block's monotonically increasing frame, generation,
first presentation time, and seconds per sample. A musical locate never rewinds
this frame counter. A new stream, clock switch, format change, native clock
discontinuity, or long engine stall establishes a new generation. Readers reject
expired generations and preserve future ones until the engine selects them.

The host cycle gate protects stream registration, graph execution, control,
configuration, and destruction. The host copies only a scalar sleep duration
before releasing that gate. A native callback touches its own `AudioStream` and
native buffers; it never enters the graph or host gate. Each audio queue has one
producer and one consumer. A bounded triple-buffer mailbox publishes clock
snapshots without retry loops or locks.

`Attach` prepares a client. Configure queues and install the callback before
`Start`. Stop the polling worker, then detach the native client before freeing or
reconfiguring its queues. Callback I/O allocates nothing. Non-callback clients
must bound `WaitForCycle` and use nonblocking reads/writes; short I/O fails the
stream so control can reopen it. No callback closes its own device.

## Alignment and rate conversion

The host renders ahead of the maximum attached output latency, native/engine
periods, and resampler lookahead. All outputs receive the same presentation stamp.
A callback reads the samples for its own DAC time, including per-channel latency,
using a windowed-sinc converter. Different rates and callback phases therefore do
not acquire separate FIFO delays. Native timestamps estimate sample-clock drift;
phase correction is bounded to 500 ppm, while a discontinuity starts a new epoch.

Capture uses the same timeline with a common explicit input-to-output delay. Its
lookback includes render-ahead, maximum input latency, native periods and filter
lookahead. This favors coherent routing over minimum latency for one endpoint.

An underrun produces silence for missing time. Late audio is discarded, rather
than played late forever. Queues retain bounded filter history and reject
repeated/reordered frames. Storage is fixed while running; configuration rejects
formats exceeding a 128 MiB queue/scratch budget per stream.

JACK supplies cycle time and connected-port latency. CoreAudio supplies HAL host
time plus hardware/stream latency. PortAudio supplies ADC/DAC callback times
mapped from its stream clock. ALSA uses monotonic status timestamps and reported
PCM delay when available; older ALSA and OSS use query-time delay estimates. OSS
drivers without playback-delay reporting cannot attach to this transport.
PipeWire supplies monotonic playback timestamps; capture timing remains estimated. ESD is a compatibility backend with estimated timing because it exposes no server/DAC clock. These two clients share their implementation with private. PipeWire alone requires C++11; its headers and flags stay inside its optional module.

Driver-reported latency cannot account for unreported external converters or
acoustic paths; mixed hardware still needs loopback measurement.

## Backend extraction boundary

`PluginManager` owns library handles and discovery. `AudioBackendRegistry` validates descriptors, selects clients and tracks their lifetime; it refuses unload while clients remain live and removes module descriptors before `dlclose`. PortAudio, CoreAudio, ALSA, OSS, PipeWire and ESD are loadable modules. JACK and Dummy currently register through the same interface as built-ins. The legacy ALSA timer and its focused test live with the ALSA module; the presentation scheduler does not use that timer. Each factory creates an independent native client.

`AudioTimeline` coordinates one engine's presentation clock, stream membership, capture lookback and pacing. The public hub supplies host/session policy; the private host supplies Spicy ownership and control scheduling. `AudioClient`, `AudioTiming`, `AudioStream`, `TimedAudioBuffer`, `PresentationClock`, `AudioTimeline`, loader and registry sources can therefore remain identical between hosts. Named JACK devices and Output use this same transport contract.

Automated regressions exercise unequal rates and periods, fractional callback
phases, per-channel latency, positive/negative clock drift, capture alignment,
queue overflow and recovery, generation changes, native frame wrap, allocation
freedom inside transfer callbacks, and bounded worker shutdown. Physical loopback
and listening tests remain separate from these deterministic checks.

Module sources and tests live in `Plugins/Audio/<Backend>`. Configure builds every supported module it detects; `--disable-portaudio`, `--disable-coreaudio`, `--disable-alsa-output`, `--disable-oss-output`, `--disable-pipewire` and `--disable-esd` opt out individually. ESD's socket-pair fixture runs even when libesd is absent. The module loader regression covers independent instances, unload refusal, descriptor removal and reload for every configured module. PipeWire's runtime test starts an isolated server with no hardware nodes.
