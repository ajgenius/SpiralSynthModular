AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include -I$(srcdir)/File/include -I$(srcdir)/Plugin/include -I$(srcdir)/Audio/include -I$(srcdir)/Midi/include -I$(top_srcdir)/libs/libSpumoni/include
lib_LTLIBRARIES = libspiralcore.la
libspiralcore_la_SOURCES = \
    include/Allocator.h \
    include/Finite.h \
    include/ChannelHandler.h \
    Midi/include/Midi.h \
    include/NoteTable.h \
    include/Sample.h \
    include/Types.h \
    include/NETypes.h \
    src/Allocator.cpp \
    src/ChannelHandler.cpp \
    Midi/src/Midi.cpp \
    src/Sample.cpp \
    include/RiffWav.h \
    src/RiffWav.cpp \
    include/GraphSort.h \
    src/GraphSort.cpp
libspiralcore_la_LDFLAGS = -shared -no-undefined -version-info 3:0:0 \
	$(SPUMONI_WHOLE_ARCHIVE)
libspiralcore_la_DEPENDENCIES = $(top_builddir)/libs/libSpumoni/libSpumoni.a
# libSpumoni.a is linked in full for the core and its consumers.
libspiralcore_la_LIBADD = $(SFLIBS) $(JACK_LIBS) -lm \
	$(ZLIB_LIBS)

check_PROGRAMS = compatibility-test
TESTS = $(check_PROGRAMS)
compatibility_test_SOURCES = tests/compatibility.cpp
compatibility_test_LDADD = libspiralcore.la

EXTRA_DIST = \
    COPYING \
    include/AsyncSampleLoader.h \
    include/Event.h \
    include/EventQueue.h \
    include/SpiralTime.h \
    src/AsyncSampleLoader.cpp \
    src/Event.cpp \
    src/EventQueue.cpp \
    src/Time.cpp

EXTRA_DIST += Audio/AUDIO-TRANSPORT.md

libspiralcore_la_SOURCES += Audio/include/AudioClient.h

libspiralcore_la_SOURCES += Plugin/include/PluginManager.h Plugin/src/PluginManager.cpp
libspiralcore_la_SOURCES += Plugin/include/DeviceClassRegistry.h Plugin/src/DeviceClassRegistry.cpp Plugin/include/EditorClassRegistry.h Plugin/src/EditorClassRegistry.cpp
libspiralcore_la_SOURCES += Plugin/include/SpiralPlugin.h Plugin/src/SpiralPlugin.cpp Audio/include/OutputAudioClient.h Audio/src/OutputAudioClient.cpp Audio/include/AudioTransportHub.h Audio/src/AudioTransportHub.cpp Plugin/include/OutputPlugin.h Plugin/src/OutputPlugin.cpp Plugin/include/OutputIcon.xpm
libspiralcore_la_SOURCES += Plugin/include/BackendModule.h Audio/include/AudioBackend.h Audio/src/AudioBackend.cpp Audio/src/AudioBackends.cpp Audio/include/DummyClient.h Audio/src/DummyClient.cpp
libspiralcore_la_SOURCES += Midi/include/MidiBackend.h Midi/src/MidiBackend.cpp Midi/include/NativeMidi.h Midi/src/NativeMidi.cpp include/ThreadCompatibility.h include/Compatibility.h
check_PROGRAMS += midi-backend-test native-midi-test midi-device-test
midi_device_test_SOURCES = Midi/tests/midi-device.cpp
midi_device_test_LDADD = libspiralcore.la
midi_backend_test_SOURCES = Midi/tests/midi-backend.cpp
midi_backend_test_LDADD = libspiralcore.la
native_midi_test_SOURCES = Midi/tests/native-midi.cpp
native_midi_test_LDADD = libspiralcore.la
check_PROGRAMS += audio-backend-test
audio_backend_test_SOURCES = Audio/tests/audio-backend.cpp
audio_backend_test_LDADD = libspiralcore.la
check_PROGRAMS += plugin-manager-test
plugin_manager_test_SOURCES = Plugin/tests/plugin-manager.cpp
plugin_manager_test_LDADD = libspiralcore.la
check_LTLIBRARIES = probe_Probe.la sub_Probe.la stranger_Probe.la probe_Audio.la probe_Midi.la
probe_Midi_la_SOURCES = Plugin/tests/plugin-manager/midi.cpp
probe_Midi_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
probe_Audio_la_SOURCES = Plugin/tests/plugin-manager/audio.cpp
probe_Audio_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
probe_Probe_la_SOURCES = Plugin/tests/plugin-manager/probe.cpp
probe_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
sub_Probe_la_SOURCES = Plugin/tests/plugin-manager/probe.cpp
sub_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
stranger_Probe_la_SOURCES = Plugin/tests/plugin-manager/stranger.cpp
stranger_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
# The test root: probe/probe_Probe.so, probe/sub/sub_Probe.so, probe/stranger_Probe.so
# The plugin manifest the host reads before opening a module.
check_PROGRAMS += manifest-test
manifest_test_SOURCES = Plugin/tests/manifest.cpp Plugin/include/PluginManifest.h Plugin/src/PluginManifest.cpp
manifest_test_LDADD = libspiralcore.la

plugin-manager-root: $(check_LTLIBRARIES)
	rm -rf plugin-manager-root
	$(MKDIR_P) plugin-manager-root/probe/sub plugin-manager-root/audio plugin-manager-root/midi
	cp .libs/probe_Audio.so plugin-manager-root/audio/
	cp .libs/probe_Midi.so plugin-manager-root/midi/
	cp .libs/probe_Probe.so plugin-manager-root/probe/
	cp .libs/sub_Probe.so plugin-manager-root/probe/sub/
	cp .libs/stranger_Probe.so plugin-manager-root/probe/
check_DATA = plugin-manager-root
TESTS_ENVIRONMENT = PLUGIN_MANAGER_ROOT=$(abs_builddir)/plugin-manager-root
clean-local:
	rm -rf plugin-manager-root

libspiralcore_la_SOURCES += src/RingBuffer.cpp include/RingBuffer.h src/CommandRingBuffer.cpp include/CommandRingBuffer.h
check_PROGRAMS += ring-buffer-test
ring_buffer_test_SOURCES = tests/ring-buffer.cpp
ring_buffer_test_LDADD = libspiralcore.la

# The description: a device's state as positional tokens, the form the
# files have always been written in.
libspiralcore_la_SOURCES += File/include/Description.h File/src/Description.cpp
libspiralcore_la_SOURCES += File/include/PatchProject.h File/src/PatchProject.cpp
check_PROGRAMS += description-test description-reader-test
description_test_SOURCES = File/tests/description.cpp
description_test_LDADD = libspiralcore.la
description_reader_test_SOURCES = File/tests/description-reader.cpp
description_reader_test_LDADD = libspiralcore.la

# The positional contract and its reader. The contract is data; Spumoni's
# JSON reader loads it.
schemadir = $(pkgdatadir)/schemas
dist_schema_DATA = \
    File/schemas/SpiralPositionalText-0.2.x.json \
    File/schemas/SpiralPositionalText-0.2.x.AUTHORS \
    File/schemas/SpiralPositionalText.history.json
AM_CPPFLAGS += -DSPIRALCORE_SCHEMA_DIR='"$(abs_srcdir)/File/schemas"'
libspiralcore_la_SOURCES += File/include/PositionalReader.h File/src/PositionalReader.cpp
libspiralcore_la_SOURCES += File/src/DescriptionJSON.cpp
check_PROGRAMS += positional-reader-test description-json-test
positional_reader_test_SOURCES = File/tests/positional-reader.cpp
positional_reader_test_LDADD = libspiralcore.la
description_json_test_SOURCES = File/tests/description-json.cpp
description_json_test_LDADD = libspiralcore.la
noinst_PROGRAMS = positional-tokens
positional_tokens_SOURCES = File/tools/positional-tokens.cpp
positional_tokens_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += Audio/include/AtomicClock.h
if ATOMIC_CLOCK_MACH
libspiralcore_la_SOURCES += Audio/src/MachAtomicClock.cpp
endif
if ATOMIC_CLOCK_MACH
check_PROGRAMS += atomic-clock-test
atomic_clock_test_SOURCES = Audio/tests/atomic-clock.cpp
atomic_clock_test_LDADD = libspiralcore.la
endif
EXTRA_DIST += Audio/src/MachAtomicClock.cpp

if HAVE_JACK_CLIENT
libspiralcore_la_SOURCES += Audio/src/JackClient.cpp Audio/include/JackClient.h
check_PROGRAMS += jack-client-test
jack_client_test_SOURCES = Audio/tests/jack-client.cpp Audio/src/JackClient.cpp Audio/src/AudioTiming.cpp
jack_client_test_CPPFLAGS = $(AM_CPPFLAGS)
endif

EXTRA_DIST += Audio/tests/jack-live.cpp Audio/tests/analyze-loopback.py Audio/tests/LOOPBACK.md

# Build these explicitly; live routing and microphone tests are opt-in.
if HAVE_JACK_CLIENT
EXTRA_PROGRAMS = jack-alignment-live native-loopback
jack_alignment_live_SOURCES = Audio/tests/jack-alignment-live.cpp
jack_alignment_live_LDADD = libspiralcore.la $(JACK_LIBS)
native_loopback_SOURCES = Audio/tests/native-loopback.cpp
native_loopback_LDADD = libspiralcore.la $(JACK_LIBS)
endif


libspiralcore_la_SOURCES += Audio/include/AudioTiming.h Audio/src/AudioTiming.cpp Audio/include/TimedAudioBuffer.h Audio/src/TimedAudioBuffer.cpp
check_PROGRAMS += audio-timeline-test
audio_timeline_test_SOURCES = Audio/tests/audio-timeline.cpp
audio_timeline_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += Audio/include/AudioStream.h Audio/src/AudioStream.cpp Audio/include/PresentationClock.h Audio/src/PresentationClock.cpp

check_PROGRAMS += audio-stream-test
audio_stream_test_SOURCES = Audio/tests/audio-stream.cpp
audio_stream_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += Audio/include/AudioTimeline.h Audio/src/AudioTimeline.cpp
check_PROGRAMS += audio-coordinator-test
audio_coordinator_test_SOURCES = Audio/tests/audio-coordinator.cpp
audio_coordinator_test_LDADD = libspiralcore.la
