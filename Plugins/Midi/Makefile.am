AM_CPPFLAGS = $(DEBUG_FLAGS) $(SSM_CFLAGS) -I$(top_srcdir)/Foundation/libSSMCore
AM_CXXFLAGS = -std=c++11
plugindir = $(SSM_PLUGIN_ROOT)/midi
plugin_LTLIBRARIES = DummyMidi.la
DummyMidi_la_SOURCES = DummyModule.cpp
DummyMidi_la_LDFLAGS = -module -avoid-version -no-undefined
DummyMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la
if HAVE_RTMIDI
plugin_LTLIBRARIES += RtMidi.la
RtMidi_la_SOURCES = RtMidiModule.cpp
RtMidi_la_CPPFLAGS = $(AM_CPPFLAGS) $(RTMIDI_CFLAGS)
RtMidi_la_LDFLAGS = -module -avoid-version -no-undefined
RtMidi_la_LIBADD = $(top_builddir)/Foundation/libSSMCore/libSSMCore.la $(RTMIDI_LIBS) -lpthread
endif
