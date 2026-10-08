AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir)

plugindir = @PLUGINDIR@/midi/CoreMIDI
plugin_LTLIBRARIES = CoreMIDI_Midi.la
CoreMIDI_Midi_la_SOURCES = CoreMidiBackend.cpp
CoreMIDI_Midi_la_LDFLAGS = -module -avoid-version -shared -no-undefined
CoreMIDI_Midi_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(MIDI_LIBS) -lpthread

# Live: the transport's virtual destination receives what a client sends it.
check_PROGRAMS = coremidi-midi-test
TESTS = $(check_PROGRAMS)
coremidi_midi_test_SOURCES = coremidi-midi-test.cpp CoreMidiBackend.cpp
coremidi_midi_test_CPPFLAGS = $(AM_CPPFLAGS)
coremidi_midi_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la $(MIDI_LIBS) -lpthread

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/CoreMIDI_Midi.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/CoreMIDI_Midi.so"
