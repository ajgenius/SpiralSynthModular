AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include $(PORTAUDIO_CFLAGS) $(ALSA_CFLAGS)
lib_LTLIBRARIES = libspiralcore.la
libspiralcore_la_SOURCES = \
    include/Allocator.h \
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
libspiralcore_la_LDFLAGS = -shared -no-undefined -version-info 0:0:0
libspiralcore_la_LIBADD = $(AMIDILIBS) $(SFLIBS) $(PORTAUDIO_LIBS) $(ALSA_LIBS) -lm -lpthread

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
    include/AtomicClock.h \
    include/CommandRingBuffer.h \
    include/Event.h \
    include/EventQueue.h \
    include/JackClient.h \
    include/OSCServer.h \
    include/PortAudioClient.h \
    include/RingBuffer.h \
    include/SearchPaths.h \
    include/SpiralTime.h \
    include/Trace.h \
    include/Tuna.h \
    src/AsyncSampleLoader.cpp \
    src/AtomicClock.cpp \
    src/CommandRingBuffer.cpp \
    src/Event.cpp \
    src/EventQueue.cpp \
    src/JackClient.cpp \
    src/OSCServer.cpp \
    src/PortAudioClient.cpp \
    src/RingBuffer.cpp \
    src/SearchPaths.cpp \
    src/Time.cpp \
    src/Tuna.cpp

EXTRA_DIST += README.ssm

libspiralcore_la_SOURCES += include/AudioClient.h
if HAVE_OUTPUT_PORTAUDIO
libspiralcore_la_SOURCES += src/PortAudioClient.cpp include/PortAudioClient.h
endif
if HAVE_OUTPUT_ALSA
libspiralcore_la_SOURCES += src/AlsaClient.cpp include/AlsaClient.h
endif
if HAVE_OUTPUT_OSS
libspiralcore_la_SOURCES += src/OSSClient.cpp include/OSSClient.h
endif
