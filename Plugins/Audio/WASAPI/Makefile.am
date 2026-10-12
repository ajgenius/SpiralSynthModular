# AudioClient.h must not shadow the Windows SDK's audioclient.h on a
# case-insensitive filesystem. Project headers use quoted includes only.
AM_CPPFLAGS = -iquote$(top_srcdir)/libs/libspiralcore/Audio/include \
	-iquote$(top_srcdir)/libs/libspiralcore/Plugin/include -I$(top_builddir)
AM_CXXFLAGS = -std=c++03

plugindir = @PLUGINDIR@/audio/WASAPI
plugin_LTLIBRARIES = WASAPI_Audio.la
WASAPI_Audio_la_SOURCES = WASAPIClient.h WASAPIClient.cpp Module.cpp
WASAPI_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
WASAPI_Audio_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(WASAPI_LIBS)

check_PROGRAMS = wasapi-client-test wasapi-module-test wasapi-stream-test
TESTS = $(check_PROGRAMS)
wasapi_client_test_SOURCES = wasapi-client-test.cpp WASAPIClient.cpp
wasapi_client_test_CPPFLAGS = $(AM_CPPFLAGS)
wasapi_client_test_LDADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(WASAPI_LIBS)

wasapi_module_test_SOURCES = wasapi-module-test.cpp
wasapi_stream_test_SOURCES = wasapi-stream-test.cpp WASAPIClient.cpp
wasapi_stream_test_CPPFLAGS = $(AM_CPPFLAGS)
wasapi_stream_test_LDADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la $(WASAPI_LIBS)

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/WASAPI_Audio.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/WASAPI_Audio@plugin_module_ext@"
