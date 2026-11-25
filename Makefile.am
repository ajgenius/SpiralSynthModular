AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS = libs/libSpumoni libs/libSpiralPanels tools/embed-manifest libs/libspiralcore GUI/Controls libs/libSpiralPanels/toolkits/fltk GUI src Plugins third-party/swh-plugins third-party/cmt third-party/blop

AM_CPPFLAGS =                                              \
                $(FLTK_CXXFLAGS)                        \
                $(SPIRALGUI_CFLAGS)           \
                $(SPIRALCORE_CFLAGS)                    \
                $(SPIRALPANELS_CFLAGS)                  \
                -I$(top_srcdir)/libs/libSpumoni/include \
                -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CPPFLAGS += -DSSM_INSTALLED_SCHEMA_DIR='"$(pkgdatadir)/schemas"'
AM_CPPFLAGS += -DSPIRALCORE_SCHEMA_DIR='"$(abs_top_srcdir)/libs/libspiralcore/File/schemas"'

HOST_LDADD = 							\
	$(FLTK_LIBS) $(MIDI_LIBS)					\
	-lm				\
	$(top_builddir)/GUI/Panels/libSpiralPanelWindow.la \
	$(top_builddir)/libs/libSpiralPanels/toolkits/fltk/libSpiralPanelsFltk.la \
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
	$(top_builddir)/GUI/Controls/libSpiralControls.la \
                $(top_builddir)/libs/libspiralcore/libspiralcore.la

EXTRA_DIST = autogen.sh build-aux/macos-bundle.sh GUI/SpiralSynthModular.icns Examples
EXTRA_DIST += third-party/ladspa
EXTRA_DIST += tools/native-audio/README.md tools/native-audio/build-macos.sh \
	tools/native-audio/patches/jack2-coreaudio-latency.patch \
	tools/native-audio/patches/portaudio-coreaudio-latency.patch
SpiralPlugins = $(libdir)/SpiralPlugins

AM_CPPFLAGS += -DSSM_EXAMPLES_DIR=\"$(pkgdatadir)/Examples\"
AM_CPPFLAGS += $(LADSPA_CFLAGS)
ssm_plugindir = @PLUGINDIR@

BUNDLE_DIR = $(abs_builddir)/SpiralSynthModular.app
.PHONY: bundle
if MACOS_BUNDLE
bundle: all
	+$(SHELL) "$(srcdir)/build-aux/macos-bundle.sh" "$(MAKE)" \
		"$(bindir)" "$(libdir)" "$(ssm_plugindir)" \
		"$(srcdir)" "$(PACKAGE_VERSION)" "$(BUNDLE_DIR)" "$(pkgdatadir)/schemas"
else
bundle:
	@echo "Bundle packaging requires macOS"; exit 1
endif

check_PROGRAMS = mixer-ports-test
TESTS = $(check_PROGRAMS)
mixer_ports_test_SOURCES = tests/mixer-ports.cpp Plugins/Devices/MixerPlugin/MixerPlugin.C src/SpiralInfo.C
mixer_ports_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/Plugins/Devices/MixerPlugin
mixer_ports_test_LDADD = $(HOST_LDADD)

check_PROGRAMS += panel-handlers-test
panel_handlers_test_SOURCES = tests/panel-handlers.cpp Plugins/Devices/MixerPlugin/MixerPlugin.C src/SpiralInfo.C
panel_handlers_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/Plugins/Devices/MixerPlugin -I$(srcdir)/libs/libSpumoni/include
panel_handlers_test_LDADD = $(HOST_LDADD)

check_PROGRAMS += mixer-signal-test stereo-mixer-signal-test
mixer_signal_test_SOURCES = tests/mixer-signal.cpp Plugins/Devices/MixerPlugin/MixerPlugin.C
mixer_signal_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/Devices/MixerPlugin
mixer_signal_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += moog-filter-test
moog_filter_test_SOURCES = tests/moog-filter.cpp Plugins/Devices/MoogFilterPlugin/MoogFilterPlugin.C
moog_filter_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/Devices/MoogFilterPlugin
moog_filter_test_LDADD = $(mixer_signal_test_LDADD)
stereo_mixer_signal_test_SOURCES = tests/mixer-signal.cpp Plugins/Devices/StereoMixerPlugin/StereoMixerPlugin.C
stereo_mixer_signal_test_CPPFLAGS = $(AM_CPPFLAGS) -DTEST_STEREO -I$(srcdir)/Plugins/Devices/StereoMixerPlugin
stereo_mixer_signal_test_LDADD = $(mixer_signal_test_LDADD)

check_PROGRAMS += musical-clock-test
musical_clock_test_SOURCES = tests/musical-clock.cpp Plugins/Devices/ClockPlugin/ClockPlugin.C
musical_clock_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/Devices/ClockPlugin
musical_clock_test_LDADD = libs/libspiralcore/libspiralcore.la -ldl

check_PROGRAMS += sampler-voices-test
sampler_voices_test_SOURCES = tests/sampler-voices.cpp Plugins/Devices/PoshSamplerPlugin/PoshSamplerPlugin.C
sampler_voices_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/Devices/PoshSamplerPlugin
sampler_voices_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += event-voices-test
event_voices_test_SOURCES = tests/event-voices.cpp
event_voices_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += piano-roll-test
piano_roll_test_SOURCES = tests/piano-roll.cpp Plugins/Devices/PianoRollPlugin/PianoRollPlugin.C
piano_roll_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/Plugins/Devices/PianoRollPlugin
piano_roll_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += plugin-classes-test
plugin_classes_test_SOURCES = tests/plugin-classes.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
plugin_classes_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
plugin_classes_test_LDADD = $(HOST_LDADD)
plugin_classes_test_LDFLAGS = -export-dynamic

check_PROGRAMS += waveshaper-editor-test
waveshaper_editor_test_SOURCES = tests/waveshaper-editor.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
waveshaper_editor_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
waveshaper_editor_test_LDADD = $(HOST_LDADD)
waveshaper_editor_test_LDFLAGS = -export-dynamic

check_PROGRAMS += menu-chrome-test
menu_chrome_test_SOURCES = tests/menu-chrome.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
menu_chrome_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
menu_chrome_test_LDADD = $(HOST_LDADD)
menu_chrome_test_LDFLAGS = -export-dynamic

check_PROGRAMS += center-patch-test
center_patch_test_SOURCES = tests/center-patch.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
center_patch_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
center_patch_test_LDADD = $(HOST_LDADD)
center_patch_test_LDFLAGS = -export-dynamic

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-gui-test jack-plugin-test
jack_gui_test_SOURCES = tests/jack-gui.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
jack_gui_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_gui_test_LDADD = $(HOST_LDADD) $(JACK_LIBS)
jack_gui_test_LDFLAGS = -export-dynamic
jack_plugin_test_SOURCES = tests/jack-plugin.cpp
jack_plugin_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/libs/libspiralcore/Devices/Jack
jack_plugin_test_LDADD = libs/libspiralcore/libspiralcore.la $(JACK_LIBS) -ldl
endif

if HAVE_CORE_AUDIO_CLIENT
check_PROGRAMS += coreaudio-output-test
coreaudio_output_test_SOURCES = tests/coreaudio-output.cpp
coreaudio_output_test_LDADD = libs/libspiralcore/libspiralcore.la
endif

check_PROGRAMS += midi-bindings-test
midi_bindings_test_SOURCES = tests/midi-bindings.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
midi_bindings_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
midi_bindings_test_LDADD = $(HOST_LDADD) $(MIDI_LIBS)
midi_bindings_test_LDFLAGS = -export-dynamic

surface_modules = tests/surface-modules/dsp/FilterPlugin tests/surface-modules/dsp/PianoRollPlugin tests/surface-modules/dsp/MidiPlugin \
	tests/surface-modules/dsp/EnvelopePlugin tests/surface-modules/dsp/ComplexEnvelopePlugin
EXTRA_midi_bindings_test_DEPENDENCIES = $(surface_modules)
$(surface_modules):
	$(MKDIR_P) tests/surface-modules/dsp
	ln -s "$(abs_builddir)/Plugins/Devices/$(@F)/.libs" "$@"
CLEANFILES = $(surface_modules)

check_PROGRAMS += midi-plugin-test
midi_plugin_test_SOURCES = tests/midi-plugin.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
midi_plugin_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
midi_plugin_test_LDADD = $(HOST_LDADD) $(MIDI_LIBS)
midi_plugin_test_LDFLAGS = -export-dynamic
check_PROGRAMS += dummy-host-test
dummy_host_test_SOURCES = tests/dummy-host.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
dummy_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
dummy_host_test_LDADD = $(HOST_LDADD)
dummy_host_test_LDFLAGS = -export-dynamic

if HAVE_CORE_AUDIO_CLIENT
check_PROGRAMS += coreaudio-host-test
coreaudio_host_test_SOURCES = tests/coreaudio-host.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
coreaudio_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
coreaudio_host_test_LDADD = $(HOST_LDADD)
coreaudio_host_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-output-host-test
jack_output_host_test_SOURCES = tests/jack-output-host.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
jack_output_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_output_host_test_LDADD = $(HOST_LDADD) $(JACK_LIBS)
jack_output_host_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-output-timing-test
jack_output_timing_test_SOURCES = tests/jack-output-timing.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
jack_output_timing_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
jack_output_timing_test_LDADD = $(HOST_LDADD) $(JACK_LIBS)
jack_output_timing_test_LDFLAGS = -export-dynamic
endif

if HAVE_JACK_CLIENT
check_PROGRAMS += jack-periods-test
TESTS += jack-periods-test
jack_periods_test_SOURCES = tests/jack-periods.cpp libs/libspiralcore/Devices/Jack/JackPlugin.cpp libs/libspiralcore/Audio/src/JackClient.cpp
jack_periods_test_SOURCES += libs/libspiralcore/Audio/src/OutputAudioClient.cpp \
	libs/libspiralcore/Audio/src/AudioTransportHub.cpp libs/libspiralcore/Plugin/src/OutputPlugin.cpp
jack_periods_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/libs/libspiralcore/Devices/Jack
jack_periods_test_LDADD = libs/libspiralcore/libspiralcore.la
endif

check_PROGRAMS += hub-clock-test
TESTS += hub-clock-test
hub_clock_test_SOURCES = tests/hub-clock.cpp
hub_clock_test_LDADD = libs/libspiralcore/libspiralcore.la

# Example patches, including the sampler's sibling folder of waves.
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(pkgdatadir)/Examples"
	cp -R "$(srcdir)/Examples/." "$(DESTDIR)$(pkgdatadir)/Examples"

uninstall-local:
	rm -rf "$(DESTDIR)$(pkgdatadir)/Examples"

check_PROGRAMS += package-project-test
package_project_test_SOURCES = tests/package-project.cpp
package_project_test_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(srcdir)/libs/libSpumoni/include \
	-DPUBLIC_TEST_SOURCE='"$(abs_srcdir)/Examples/FourOsc.ssm"'
package_project_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += package-sidecars-host-test
package_sidecars_host_test_SOURCES = tests/package-sidecars-host.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
package_sidecars_host_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) \
	-DPUBLIC_SAMPLER_SOURCE='"$(abs_srcdir)/Examples/Tutorial5-MatrixSampler.ssm"'
package_sidecars_host_test_LDADD = $(HOST_LDADD)
package_sidecars_host_test_LDFLAGS = -export-dynamic

check_PROGRAMS += import-branch-test
import_branch_test_SOURCES = tests/import-branch.cpp
import_branch_test_CPPFLAGS = $(SPIRALCORE_CFLAGS) -I$(srcdir)/libs/libSpumoni/include
import_branch_test_LDADD = libs/libspiralcore/libspiralcore.la

check_PROGRAMS += canvas-menu-test
check_LTLIBRARIES =
if BUILD_LADSPA
check_PROGRAMS += ladspa-reset-test
ladspa_reset_test_SOURCES = tests/ladspa-reset.cpp
ladspa_reset_test_CPPFLAGS = $(AM_CPPFLAGS) $(LADSPA_CFLAGS)
ladspa_reset_test_LDADD = libs/libspiralcore/libspiralcore.la
check_LTLIBRARIES += tests/ladspa/ports.la
tests_ladspa_ports_la_CPPFLAGS = $(AM_CPPFLAGS) $(LADSPA_CFLAGS)
tests_ladspa_ports_la_SOURCES = tests/ladspa/ports.cpp
tests_ladspa_ports_la_LDFLAGS = -module -avoid-version -rpath /unused
TESTS_ENVIRONMENT = LADSPA_PATH='$(abs_builddir)/tests/ladspa/.libs'

if BUNDLE_LADSPA_PLUGINS
check_PROGRAMS += ladspa-gverb-test
ladspa_gverb_test_SOURCES = tests/ladspa-gverb.cpp
ladspa_gverb_test_CPPFLAGS = $(AM_CPPFLAGS) $(LADSPA_CFLAGS)
ladspa_gverb_test_LDADD = libs/libspiralcore/libspiralcore.la
TESTS_ENVIRONMENT += LADSPA_BUNDLE_DIR='$(abs_builddir)/third-party/swh-plugins/.libs'
endif
endif
canvas_menu_test_SOURCES = tests/canvas-menu.cpp src/SpiralInfo.C
canvas_menu_test_LDADD = $(HOST_LDADD)

check_PROGRAMS += drawer-test
drawer_test_SOURCES = tests/drawer.cpp src/SpiralInfo.C
drawer_test_LDADD = $(HOST_LDADD)

check_PROGRAMS += stream-paste-test
stream_paste_test_SOURCES = Plugins/Devices/StreamPlugin/StreamPlugin.C tests/stream-paste.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
stream_paste_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/Plugins/Devices/StreamPlugin
stream_paste_test_LDADD = $(HOST_LDADD)
stream_paste_test_LDFLAGS = -export-dynamic

check_PROGRAMS += schema-deployment-test plugin-abi-test
schema_deployment_test_SOURCES = tests/schema-deployment.cpp src/SpiralSynthModular.C src/UnavailablePlugin.C src/SpiralInfo.C src/SettingsWindow.C
schema_deployment_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
schema_deployment_test_LDADD = $(HOST_LDADD)
schema_deployment_test_LDFLAGS = -export-dynamic
plugin_abi_test_SOURCES = tests/plugin-abi.cpp
plugin_abi_test_LDADD = libs/libspiralcore/libspiralcore.la
abi9_module = tests/abi9-modules/dsp/OldDevice_DSP$(plugin_module_ext)
EXTRA_plugin_abi_test_DEPENDENCIES = $(abi9_module) $(surface_modules)
$(abi9_module): tests/old-device.la
	$(MKDIR_P) tests/abi9-modules/dsp
	ln -sf "$(abs_builddir)/tests/.libs/old-device$(plugin_module_ext)" "$@"
CLEANFILES += $(abi9_module)
check_LTLIBRARIES += tests/old-device.la
tests_old_device_la_SOURCES = tests/old-device.cpp
tests_old_device_la_LDFLAGS = -module -avoid-version -rpath /unused
EXTRA_DIST += tests/fixtures/controller-v4.ssm

if HAVE_JACK_CLIENT
check_PROGRAMS += module-ports-test
module_ports_test_SOURCES = tests/module-ports.cpp src/SpiralInfo.C
module_ports_test_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir) -I$(srcdir)/Plugins/Devices/MixerPlugin -I$(srcdir)/libs/libspiralcore/Devices/Jack
module_ports_test_LDADD = $(HOST_LDADD) $(JACK_LIBS) -ldl
endif

