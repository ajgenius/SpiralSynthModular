AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir)
# PipeWire's headers require C++11; this override is local to the module.
CXXFLAGS = @CXXFLAGS@ -std=c++11
AM_CPPFLAGS += $(PIPEWIRE_CFLAGS)
plugindir = @PLUGINDIR@/audio/PipeWire
plugin_LTLIBRARIES =
plugin_LTLIBRARIES += PipeWire_Audio.la
PipeWire_Audio_la_SOURCES = PipeWireClient.C PipeWireClient.h PipeWireModule.cpp
PipeWire_Audio_la_CPPFLAGS = $(AM_CPPFLAGS)
PipeWire_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
PipeWire_Audio_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(PIPEWIRE_LIBS) -lpthread

check_PROGRAMS = pipewire-client-test
TESTS = tests/pipewire-client.sh
pipewire_client_test_SOURCES = tests/pipewire-client.cpp PipeWireClient.C PipeWireClient.h
pipewire_client_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
pipewire_client_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la $(PIPEWIRE_LIBS) -lpthread
EXTRA_DIST = tests/pipewire-client.sh tests/pipewire-client.conf tests/pipewire-server.conf

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PipeWire_Audio.la"
