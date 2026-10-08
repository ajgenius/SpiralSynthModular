AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir) $(RTMIDI_CFLAGS)

plugindir = @PLUGINDIR@/midi/RtMidi
plugin_LTLIBRARIES = RtMidi_Midi.la
RtMidi_Midi_la_SOURCES = RtMidiBackend.cpp
RtMidi_Midi_la_CXXFLAGS = -std=c++11
RtMidi_Midi_la_LDFLAGS = -module -avoid-version -shared -no-undefined
RtMidi_Midi_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(RTMIDI_LIBS) -lpthread

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/RtMidi_Midi.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/RtMidi_Midi.so"
