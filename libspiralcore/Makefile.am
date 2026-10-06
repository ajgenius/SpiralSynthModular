AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include $(PORTAUDIO_CFLAGS) $(ALSA_CFLAGS)
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
libspiralcore_la_LIBADD = $(MIDI_LIBS) $(AMIDILIBS) $(SFLIBS) $(PORTAUDIO_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(CORE_AUDIO_LIBS) -lm

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
    include/PortAudioClient.h \
    include/SearchPaths.h \
    include/SpiralTime.h \
    include/Trace.h \
    include/Tuna.h \
    src/AsyncSampleLoader.cpp \
    src/Event.cpp \
    src/EventQueue.cpp \
    src/OSCServer.cpp \
    src/PortAudioClient.cpp \
    src/SearchPaths.cpp \
    src/Time.cpp \
    src/Tuna.cpp

EXTRA_DIST += README.ssm

libspiralcore_la_SOURCES += include/AudioClient.h

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
if HAVE_OUTPUT_PORTAUDIO
libspiralcore_la_SOURCES += src/PortAudioClient.cpp include/PortAudioClient.h
check_PROGRAMS += portaudio-client-test
portaudio_client_test_CPPFLAGS = $(AM_CPPFLAGS)
portaudio_client_test_SOURCES = tests/portaudio-client.cpp src/PortAudioClient.cpp src/AudioTiming.cpp
endif
if HAVE_OUTPUT_ALSA
libspiralcore_la_SOURCES += src/AlsaClient.cpp include/AlsaClient.h
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

if HAVE_CORE_AUDIO_CLIENT
libspiralcore_la_SOURCES += src/CoreAudioClient.cpp include/CoreAudioClient.h
check_PROGRAMS += coreaudio-client-test
coreaudio_client_test_SOURCES = tests/coreaudio-client.cpp
coreaudio_client_test_LDADD = libspiralcore.la
endif

libspiralcore_la_SOURCES += include/AudioTiming.h src/AudioTiming.cpp include/TimedAudioBuffer.h src/TimedAudioBuffer.cpp
check_PROGRAMS += audio-timeline-test
audio_timeline_test_SOURCES = tests/audio-timeline.cpp
audio_timeline_test_LDADD = libspiralcore.la

libspiralcore_la_SOURCES += include/AudioStream.h src/AudioStream.cpp include/PresentationClock.h src/PresentationClock.cpp

check_PROGRAMS += audio-stream-test
audio_stream_test_SOURCES = tests/audio-stream.cpp
audio_stream_test_LDADD = libspiralcore.la
