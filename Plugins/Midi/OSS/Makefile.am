AM_CPPFLAGS = -I$(top_srcdir)/libspiralcore/include -I$(top_builddir)

plugindir = @PLUGINDIR@/midi/OSS
plugin_LTLIBRARIES = OSS_Midi.la
OSS_Midi_la_SOURCES = OssMidiBackend.cpp
OSS_Midi_la_LDFLAGS = -module -avoid-version -shared -no-undefined
OSS_Midi_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lpthread

# A pipe stands in for the device: bytes written to it come out as packets.
check_PROGRAMS = oss-midi-test
TESTS = $(check_PROGRAMS)
oss_midi_test_SOURCES = oss-midi-test.cpp OssMidiBackend.cpp
oss_midi_test_CPPFLAGS = $(AM_CPPFLAGS)
oss_midi_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la -lpthread

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OSS_Midi.la"

uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OSS_Midi.so"
