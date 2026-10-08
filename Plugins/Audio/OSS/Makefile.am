AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(top_builddir)
plugindir = @PLUGINDIR@/audio/OSS
plugin_LTLIBRARIES = OSS_Audio.la
OSS_Audio_la_SOURCES = OSSClient.h OSSClient.cpp Module.cpp
OSS_Audio_la_LDFLAGS = -module -avoid-version -shared -no-undefined
OSS_Audio_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la

install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OSS_Audio.la"
