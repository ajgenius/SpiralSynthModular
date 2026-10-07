AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir)
plugindir = @PLUGINDIR@/audio/ESD
plugin_LTLIBRARIES =
if HAVE_OUTPUT_ESD
plugin_LTLIBRARIES += ESD_Audio.la
ESD_Audio_la_SOURCES = ESDClient.C ESDClient.h ESDModule.cpp
ESD_Audio_la_CPPFLAGS = $(AM_CPPFLAGS) $(ESOUND_CFLAGS)
ESD_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ESD_Audio_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(ESOUND_LIBS) -lpthread
endif

check_PROGRAMS = esd-client-test
TESTS = $(check_PROGRAMS)
esd_client_test_SOURCES = tests/esd-client.cpp ESDClient.C ESDClient.h
esd_client_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/tests/esd
esd_client_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la
EXTRA_DIST = tests/esd/esd.h

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ESD_Audio.la"
