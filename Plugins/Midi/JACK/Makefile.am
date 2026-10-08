AM_CPPFLAGS = -I$(top_srcdir)/libspiralcore/include -I$(top_builddir) $(JACK_MIDI_CFLAGS)

plugindir = @PLUGINDIR@/midi/JACK
plugin_LTLIBRARIES = JACK_Midi.la
JACK_Midi_la_SOURCES = JackMidiBackend.cpp
JACK_Midi_la_LDFLAGS = -module -avoid-version -shared -no-undefined
JACK_Midi_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(JACK_MIDI_LIBS) -lpthread

# The transport against a stubbed libjack: no server needed.
check_PROGRAMS = jack-midi-test
TESTS = $(check_PROGRAMS)
jack_midi_test_SOURCES = jack-midi-test.cpp
jack_midi_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_midi_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la -lpthread

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/JACK_Midi.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/JACK_Midi.so"
