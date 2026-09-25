AM_CPPFLAGS = -I$(top_srcdir)/Headers $(DEBUG_FLAGS) $(SSM_CFLAGS) -I$(top_srcdir)/Foundation/libSSMCore
AM_CXXFLAGS = $(SSM_CXX03_FLAGS)
plugindir = $(SSM_PLUGIN_ROOT)/midi
plugin_LTLIBRARIES = DummyMidi.la
DummyMidi_la_SOURCES = DummyModule.cpp
DummyMidi_la_LDFLAGS = -module -avoid-version -no-undefined
DummyMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la
if HAVE_RTMIDI
plugin_LTLIBRARIES += RtMidi.la
RtMidi_la_CXXFLAGS = -std=c++11
RtMidi_la_SOURCES = RtMidiModule.cpp
RtMidi_la_CPPFLAGS = $(AM_CPPFLAGS) $(RTMIDI_CFLAGS)
RtMidi_la_LDFLAGS = -module -avoid-version -no-undefined
RtMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la $(RTMIDI_LIBS) -lpthread
endif

if HAVE_ALSA_MIDI
plugin_LTLIBRARIES += AlsaMidi.la
AlsaMidi_la_SOURCES = AlsaModule.cpp NativeMidi.cpp NativeMidi.h
AlsaMidi_la_CPPFLAGS = $(AM_CPPFLAGS) $(ALSA_MIDI_CFLAGS)
AlsaMidi_la_LDFLAGS = -module -avoid-version -no-undefined
AlsaMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la $(ALSA_MIDI_LIBS) -lpthread
endif
if HAVE_JACK_MIDI
plugin_LTLIBRARIES += JackMidi.la
JackMidi_la_SOURCES = JackModule.cpp NativeMidi.cpp NativeMidi.h
JackMidi_la_CPPFLAGS = $(AM_CPPFLAGS) $(JACK_MIDI_CFLAGS)
JackMidi_la_LDFLAGS = -module -avoid-version -no-undefined
JackMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la $(JACK_MIDI_LIBS) -lpthread
endif

check_PROGRAMS = native-midi-test
TESTS = $(check_PROGRAMS)
native_midi_test_SOURCES = tests/native.cpp NativeMidi.cpp NativeMidi.h
native_midi_test_LDADD = -lpthread
AM_CPPFLAGS += -I$(srcdir)
AUTOMAKE_OPTIONS = subdir-objects

if HAVE_ALSA_MIDI
check_PROGRAMS += alsa-midi-codec-test
alsa_midi_codec_test_SOURCES = tests/alsa.cpp
alsa_midi_codec_test_CPPFLAGS = $(AM_CPPFLAGS) $(ALSA_MIDI_CFLAGS)
alsa_midi_codec_test_LDADD = $(ALSA_MIDI_LIBS)
endif
if HAVE_JACK_MIDI
check_PROGRAMS += jack-midi-callback-test
jack_midi_callback_test_SOURCES = tests/jack.cpp NativeMidi.cpp NativeMidi.h
jack_midi_callback_test_CPPFLAGS = $(AM_CPPFLAGS) $(JACK_MIDI_CFLAGS)
jack_midi_callback_test_LDADD = -lpthread
endif
