AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include $(ALSA_CFLAGS)
lib_LTLIBRARIES = libspiralcore.la
libspiralcore_la_SOURCES = \
    include/Allocator.h \
    include/Finite.h \
    include/ChannelHandler.h \
    include/Midi.h \
    include/NoteTable.h \
    include/Sample.h \
    include/Types.h \
    include/NETypes.h \
    src/Allocator.cpp \
    src/ChannelHandler.cpp \
    src/Midi.cpp \
    src/Sample.cpp \
    include/RiffWav.h \
    src/RiffWav.cpp
libspiralcore_la_LDFLAGS = -shared -no-undefined -version-info 2:0:0
libspiralcore_la_LIBADD = $(MIDI_LIBS) $(AMIDILIBS) $(SFLIBS) $(ALSA_LIBS) $(JACK_LIBS) -lm

check_PROGRAMS = compatibility-test
TESTS = $(check_PROGRAMS)
compatibility_test_SOURCES = tests/compatibility.cpp
compatibility_test_LDADD = libspiralcore.la

EXTRA_DIST = \
    CHANGES \
    COPYING \
    README \
    SConstruct \
    include/AsyncSampleLoader.h \
    include/Event.h \
    include/EventQueue.h \
    include/OSCServer.h \
    include/SearchPaths.h \
    include/SpiralTime.h \
    include/Trace.h \
    include/Tuna.h \
    src/AsyncSampleLoader.cpp \
    src/Event.cpp \
    src/EventQueue.cpp \
    src/OSCServer.cpp \
    src/SearchPaths.cpp \
    src/Time.cpp \
    src/Tuna.cpp

EXTRA_DIST += README.ssm AUDIO-TRANSPORT.md

libspiralcore_la_SOURCES += include/AudioClient.h

libspiralcore_la_SOURCES += include/PluginLoader.h src/PluginLoader.cpp
libspiralcore_la_SOURCES += include/AudioBackend.h src/AudioBackend.cpp src/AudioBackends.cpp include/DummyClient.h src/DummyClient.cpp
check_PROGRAMS += audio-backend-test
audio_backend_test_SOURCES = tests/audio-backend.cpp
audio_backend_test_LDADD = libspiralcore.la
check_PROGRAMS += plugin-loader-test
plugin_loader_test_SOURCES = tests/plugin-loader.cpp
plugin_loader_test_LDADD = libspiralcore.la
check_LTLIBRARIES = probe_Probe.la sub_Probe.la stranger_Probe.la probe_Audio.la
probe_Audio_la_SOURCES = tests/plugin-loader/audio.cpp
probe_Audio_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
probe_Probe_la_SOURCES = tests/plugin-loader/probe.cpp
probe_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
sub_Probe_la_SOURCES = tests/plugin-loader/probe.cpp
sub_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
stranger_Probe_la_SOURCES = tests/plugin-loader/stranger.cpp
stranger_Probe_la_LDFLAGS = -module -avoid-version -shared -rpath /nowhere
# The test root: probe/probe_Probe.so, probe/sub/sub_Probe.so, probe/stranger_Probe.so
plugin-loader-root: $(check_LTLIBRARIES)
	rm -rf plugin-loader-root
	$(MKDIR_P) plugin-loader-root/probe/sub plugin-loader-root/audio
	cp .libs/probe_Audio.so plugin-loader-root/audio/
	cp .libs/probe_Probe.so plugin-loader-root/probe/
	cp .libs/sub_Probe.so plugin-loader-root/probe/sub/
	cp .libs/stranger_Probe.so plugin-loader-root/probe/
check_DATA = plugin-loader-root
TESTS_ENVIRONMENT = PLUGIN_LOADER_ROOT=$(abs_builddir)/plugin-loader-root
clean-local:
	rm -rf plugin-loader-root

libspiralcore_la_SOURCES += src/RingBuffer.cpp include/RingBuffer.h src/CommandRingBuffer.cpp include/CommandRingBuffer.h
check_PROGRAMS += ring-buffer-test
ring_buffer_test_SOURCES = tests/ring-buffer.cpp
ring_buffer_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += include/AtomicClock.h
if ATOMIC_CLOCK_MACH
libspiralcore_la_SOURCES += src/MachAtomicClock.cpp
endif
if ATOMIC_CLOCK_ALSA
libspiralcore_la_SOURCES += src/AlsaAtomicClock.cpp include/AlsaAtomicClock.h
endif
if HAVE_ATOMIC_CLOCK
check_PROGRAMS += atomic-clock-test
atomic_clock_test_SOURCES = tests/atomic-clock.cpp
atomic_clock_test_LDADD = libspiralcore.la
endif
EXTRA_DIST += src/AlsaAtomicClock.cpp include/AlsaAtomicClock.h src/MachAtomicClock.cpp
if HAVE_OUTPUT_ALSA
libspiralcore_la_SOURCES += src/AlsaClient.cpp include/AlsaClient.h
check_PROGRAMS += alsa-client-test
alsa_client_test_SOURCES = tests/alsa-client.cpp
alsa_client_test_LDADD = libspiralcore.la
endif
if HAVE_OUTPUT_OSS
libspiralcore_la_SOURCES += src/OSSClient.cpp include/OSSClient.h
endif

if HAVE_JACK_CLIENT
libspiralcore_la_SOURCES += src/JackClient.cpp include/JackClient.h
check_PROGRAMS += jack-client-test
jack_client_test_SOURCES = tests/jack-client.cpp src/JackClient.cpp src/AudioTiming.cpp
jack_client_test_CPPFLAGS = $(AM_CPPFLAGS)
endif

EXTRA_DIST += tests/jack-live.cpp


libspiralcore_la_SOURCES += include/AudioTiming.h src/AudioTiming.cpp include/TimedAudioBuffer.h src/TimedAudioBuffer.cpp
check_PROGRAMS += audio-timeline-test
audio_timeline_test_SOURCES = tests/audio-timeline.cpp
audio_timeline_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += include/AudioStream.h src/AudioStream.cpp include/PresentationClock.h src/PresentationClock.cpp

check_PROGRAMS += audio-stream-test
audio_stream_test_SOURCES = tests/audio-stream.cpp
audio_stream_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += include/AudioTimeline.h src/AudioTimeline.cpp
check_PROGRAMS += audio-coordinator-test
audio_coordinator_test_SOURCES = tests/audio-coordinator.cpp
audio_coordinator_test_LDADD = libspiralcore.la
