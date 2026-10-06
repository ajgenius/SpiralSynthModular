AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = ESD
MODULE_DIRS =
DIST_SUBDIRS = ESD PipeWire PortAudio CoreAudio ALSA OSS
if HAVE_OUTPUT_ESD
MODULE_DIRS += ESD
endif
if HAVE_OUTPUT_PIPEWIRE
SUBDIRS += PipeWire
MODULE_DIRS += PipeWire
endif
if HAVE_OUTPUT_PORTAUDIO
SUBDIRS += PortAudio
MODULE_DIRS += PortAudio
endif
if HAVE_CORE_AUDIO_CLIENT
SUBDIRS += CoreAudio
MODULE_DIRS += CoreAudio
endif
if HAVE_OUTPUT_ALSA
SUBDIRS += ALSA
MODULE_DIRS += ALSA
endif
if HAVE_OUTPUT_OSS
SUBDIRS += OSS
MODULE_DIRS += OSS
endif

AM_CPPFLAGS = -I$(top_srcdir)/libspiralcore/include
check_PROGRAMS = audio-modules-test
TESTS = tests/modules.sh
audio_modules_test_SOURCES = tests/modules.cpp
audio_modules_test_LDADD = $(top_builddir)/libspiralcore/libspiralcore.la
TESTS_ENVIRONMENT = AUDIO_MODULE_DIRS='$(MODULE_DIRS)' AUDIO_MODULE_SUFFIX='@plugin_module_ext@'
EXTRA_DIST = tests/modules.sh
