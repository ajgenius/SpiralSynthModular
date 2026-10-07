AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir) $(ALSA_CFLAGS)
plugindir = @PLUGINDIR@/audio/ALSA
plugin_LTLIBRARIES = ALSA_Audio.la
ALSA_Audio_la_SOURCES = AlsaClient.h AlsaClient.cpp Module.cpp AlsaAtomicClock.cpp AlsaAtomicClock.h
ALSA_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ALSA_Audio_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(ALSA_LIBS)

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ALSA_Audio.la"

check_PROGRAMS = alsa-client-test
TESTS = $(check_PROGRAMS)
alsa_client_test_SOURCES = alsa-client.cpp AlsaClient.cpp
alsa_client_test_CPPFLAGS = $(AM_CPPFLAGS)
alsa_client_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la $(ALSA_LIBS)

check_PROGRAMS += alsa-clock-test
alsa_clock_test_SOURCES = $(top_srcdir)/libspiralcore/tests/atomic-clock.cpp AlsaAtomicClock.cpp
alsa_clock_test_CPPFLAGS = $(AM_CPPFLAGS)
alsa_clock_test_LDADD = $(ALSA_LIBS)
