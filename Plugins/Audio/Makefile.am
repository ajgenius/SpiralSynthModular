AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS =
if HAVE_OUTPUT_PORTAUDIO
SUBDIRS += PortAudio
endif
if HAVE_CORE_AUDIO_CLIENT
SUBDIRS += CoreAudio
endif
if HAVE_OUTPUT_ALSA
SUBDIRS += ALSA
endif
if HAVE_OUTPUT_OSS
SUBDIRS += OSS
endif
DIST_SUBDIRS = PortAudio CoreAudio ALSA OSS

AM_CPPFLAGS = -I$(top_srcdir)/libspiralcore/include
check_PROGRAMS = audio-modules-test
TESTS = tests/modules.sh
audio_modules_test_SOURCES = tests/modules.cpp
audio_modules_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la
TESTS_ENVIRONMENT = AUDIO_MODULE_DIRS='$(SUBDIRS)' AUDIO_MODULE_SUFFIX='@plugin_module_ext@'
EXTRA_DIST = tests/modules.sh
