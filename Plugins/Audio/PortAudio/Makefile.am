AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir) $(PORTAUDIO_CFLAGS)

plugindir = @PLUGINDIR@/audio/PortAudio
plugin_LTLIBRARIES = PortAudio_Audio.la
PortAudio_Audio_la_SOURCES = PortAudioClient.h PortAudioClient.cpp
PortAudio_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
PortAudio_Audio_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(PORTAUDIO_LIBS)

# A module cannot be linked against on macOS; the test builds the client itself.
check_PROGRAMS = portaudio-client-test
TESTS = $(check_PROGRAMS)
portaudio_client_test_SOURCES = portaudio-client-test.cpp PortAudioClient.cpp
portaudio_client_test_CPPFLAGS = $(AM_CPPFLAGS)
portaudio_client_test_LDADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(PORTAUDIO_LIBS)

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PortAudio_Audio.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/PortAudio_Audio.so"
