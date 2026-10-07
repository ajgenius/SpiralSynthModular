AUTOMAKE_OPTIONS = foreign
SUBDIRS = JSON libspiralcore GUI Plugins

AM_CPPFLAGS =                                              \
                $(FLTK_CXXFLAGS)                        \
                -I$(top_srcdir)/GUI/Widgets           \
                $(SPIRALCORE_CFLAGS)

SpiralSynthModular_LDADD = 							\
	$(FLTK_LIBS) $(MIDI_LIBS)					\
	-lm				\
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
                $(top_builddir)/libspiralcore/libspiralcore.la


EXTRA_DIST = autogen.sh build-aux/macos-bundle.sh GUI/SpiralSynthModular.icns Examples
EXTRA_DIST += tools/native-audio/README.md tools/native-audio/build-macos.sh \
	tools/native-audio/patches/jack2-coreaudio-latency.patch \
	tools/native-audio/patches/portaudio-coreaudio-latency.patch
SpiralPlugins = $(libdir)/SpiralPlugins

bin_PROGRAMS = SpiralSynthModular
SpiralSynthModular_SOURCES = MacBundle.h \
		SpiralSynthPluginLocation.h \
		SpiralInfo.h		\
		SpiralInfo.C		\
		SpiralSynthModular.h \
		SpiralSynthModular.C \
		SettingsWindow.h \
		SettingsWindow.C \
		main.cpp

AM_CPPFLAGS += -DPLUGIN_PATH_LOCATION=\"$(ssm_plugindir)/\"
ssm_plugindir = @PLUGINDIR@

# Widgets and loadable plugins resolve host symbols from the executable.
SpiralSynthModular_LDFLAGS = -export-dynamic

BUNDLE_DIR = $(abs_builddir)/SpiralSynthModular.app
.PHONY: bundle
if MACOS_BUNDLE
bundle: all
	+$(SHELL) "$(srcdir)/build-aux/macos-bundle.sh" "$(MAKE)" \
		"$(bindir)" "$(libdir)" "$(ssm_plugindir)" \
		"$(srcdir)" "$(PACKAGE_VERSION)" "$(BUNDLE_DIR)"
else
bundle:
	@echo "Bundle packaging requires macOS"; exit 1
endif

check_PROGRAMS = mixer-ports-test
TESTS = $(check_PROGRAMS)
mixer_ports_test_SOURCES = tests/mixer-ports.cpp Plugins/DSP/MixerPlugin/MixerPlugin.C SpiralInfo.C
mixer_ports_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/Plugins/DSP/MixerPlugin
mixer_ports_test_LDADD = $(SpiralSynthModular_LDADD)

check_PROGRAMS += mixer-signal-test stereo-mixer-signal-test
mixer_signal_test_SOURCES = tests/mixer-signal.cpp Plugins/DSP/MixerPlugin/MixerPlugin.C
mixer_signal_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/DSP/MixerPlugin
mixer_signal_test_LDADD = libspiralcore/libspiralcore.la
stereo_mixer_signal_test_SOURCES = tests/mixer-signal.cpp Plugins/DSP/StereoMixerPlugin/StereoMixerPlugin.C
stereo_mixer_signal_test_CPPFLAGS = $(AM_CPPFLAGS) -DTEST_STEREO -I$(srcdir)/Plugins/DSP/StereoMixerPlugin
stereo_mixer_signal_test_LDADD = $(mixer_signal_test_LDADD)

check_PROGRAMS += plugin-classes-test
plugin_classes_test_SOURCES = tests/plugin-classes.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
plugin_classes_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
plugin_classes_test_LDADD = $(SpiralSynthModular_LDADD)
plugin_classes_test_LDFLAGS = -export-dynamic

check_PROGRAMS += waveshaper-editor-test
waveshaper_editor_test_SOURCES = tests/waveshaper-editor.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
waveshaper_editor_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
waveshaper_editor_test_LDADD = $(SpiralSynthModular_LDADD)
waveshaper_editor_test_LDFLAGS = -export-dynamic

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-gui-test jack-plugin-test
jack_gui_test_SOURCES = tests/jack-gui.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
jack_gui_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_gui_test_LDADD = $(SpiralSynthModular_LDADD) $(JACK_LIBS)
jack_gui_test_LDFLAGS = -export-dynamic
jack_plugin_test_SOURCES = tests/jack-plugin.cpp
jack_plugin_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/DSP/JackPlugin
jack_plugin_test_LDADD = libspiralcore/libspiralcore.la $(JACK_LIBS) -ldl
endif

if HAVE_CORE_AUDIO_CLIENT
check_PROGRAMS += coreaudio-output-test
coreaudio_output_test_SOURCES = tests/coreaudio-output.cpp
coreaudio_output_test_LDADD = libspiralcore/libspiralcore.la
endif

check_PROGRAMS += midi-plugin-test
midi_plugin_test_SOURCES = tests/midi-plugin.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
midi_plugin_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
midi_plugin_test_LDADD = $(SpiralSynthModular_LDADD) $(MIDI_LIBS)
midi_plugin_test_LDFLAGS = -export-dynamic
check_PROGRAMS += dummy-host-test
dummy_host_test_SOURCES = tests/dummy-host.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
dummy_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
dummy_host_test_LDADD = $(SpiralSynthModular_LDADD)
dummy_host_test_LDFLAGS = -export-dynamic

if HAVE_CORE_AUDIO_CLIENT
check_PROGRAMS += coreaudio-host-test
coreaudio_host_test_SOURCES = tests/coreaudio-host.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
coreaudio_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
coreaudio_host_test_LDADD = $(SpiralSynthModular_LDADD)
coreaudio_host_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-output-host-test
jack_output_host_test_SOURCES = tests/jack-output-host.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
jack_output_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_output_host_test_LDADD = $(SpiralSynthModular_LDADD) $(JACK_LIBS)
jack_output_host_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-output-timing-test
jack_output_timing_test_SOURCES = tests/jack-output-timing.cpp SpiralSynthModular.C SpiralInfo.C SettingsWindow.C
jack_output_timing_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_output_timing_test_LDADD = $(SpiralSynthModular_LDADD) $(JACK_LIBS)
jack_output_timing_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-periods-test
TESTS += jack-periods-test
jack_periods_test_SOURCES = tests/jack-periods.cpp Plugins/DSP/JackPlugin/JackPlugin.C libspiralcore/Audio/src/JackClient.cpp
jack_periods_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/DSP/JackPlugin
jack_periods_test_LDADD = libspiralcore/libspiralcore.la
endif

check_PROGRAMS += hub-clock-test
TESTS += hub-clock-test
hub_clock_test_SOURCES = tests/hub-clock.cpp
hub_clock_test_LDADD = libspiralcore/libspiralcore.la
