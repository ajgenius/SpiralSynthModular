AM_CPPFLAGS = $(FLTK_CXXFLAGS) $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) $(SPIRALPANELS_CFLAGS) -I$(top_srcdir)/libs/libSpumoni/include
AM_CPPFLAGS += -DSSM_INSTALLED_SCHEMA_DIR='"$(pkgdatadir)/schemas"'
AM_CPPFLAGS += -DSPIRALCORE_SCHEMA_DIR='"$(abs_top_srcdir)/libs/libspiralcore/File/schemas"'
AM_CPPFLAGS += -DSSM_EXAMPLES_DIR=\"$(pkgdatadir)/Examples\"
AM_CPPFLAGS += $(LADSPA_CFLAGS)

bin_PROGRAMS = SpiralSynthModular
SpiralSynthModular_SOURCES = MacBundle.h \
		SpiralSynthPluginLocation.h \
		SpiralInfo.h \
		SpiralInfo.C \
		SpiralSynthModular.h \
		SpiralSynthModular.C \
		SettingsWindow.h \
		SettingsWindow.C \
		UnavailablePlugin.h \
		UnavailablePlugin.C \
		main.cpp

SpiralSynthModular_LDADD = $(FLTK_LIBS) $(MIDI_LIBS) -lm \
		$(top_builddir)/GUI/Panels/libSpiralPanels.la \
		$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
		$(top_builddir)/GUI/Controls/libSpiralControls.la \
		$(top_builddir)/libs/libspiralcore/libspiralcore.la

SpiralSynthModular_LDFLAGS = -export-dynamic
