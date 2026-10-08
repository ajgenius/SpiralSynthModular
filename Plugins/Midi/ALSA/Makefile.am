AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir) $(ALSA_MIDI_CFLAGS)

plugindir = @PLUGINDIR@/midi/ALSA
plugin_LTLIBRARIES = ALSA_Midi.la
ALSA_Midi_la_SOURCES = AlsaMidiBackend.cpp
ALSA_Midi_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ALSA_Midi_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(ALSA_MIDI_LIBS) -lpthread

# The libasound codec round trip; no sequencer device needed.
check_PROGRAMS = alsa-midi-codec-test
TESTS = $(check_PROGRAMS)
alsa_midi_codec_test_SOURCES = alsa-midi-codec-test.cpp
alsa_midi_codec_test_LDADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(ALSA_MIDI_LIBS)

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ALSA_Midi.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/ALSA_Midi.so"
