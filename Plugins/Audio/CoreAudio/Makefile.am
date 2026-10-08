AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir)

plugindir = @PLUGINDIR@/audio/CoreAudio
plugin_LTLIBRARIES = CoreAudio_Audio.la
CoreAudio_Audio_la_SOURCES = CoreAudioClient.h CoreAudioClient.cpp
CoreAudio_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
CoreAudio_Audio_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(CORE_AUDIO_LIBS)

# A module cannot be linked against on macOS; the test builds the client itself.
check_PROGRAMS = coreaudio-client-test
TESTS = $(check_PROGRAMS)
coreaudio_client_test_SOURCES = coreaudio-client-test.cpp CoreAudioClient.cpp
coreaudio_client_test_CPPFLAGS = $(AM_CPPFLAGS)
coreaudio_client_test_LDADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(CORE_AUDIO_LIBS)

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/CoreAudio_Audio.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/CoreAudio_Audio.so"
