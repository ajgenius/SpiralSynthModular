AUTOMAKE_OPTIONS = foreign
SUBDIRS = JSON libspiralcore GUI Plugins

AM_CPPFLAGS =                                              \
                $(FLTK_CXXFLAGS)                        \
                -I$(top_srcdir)/GUI/Widgets           \
                -I$(top_srcdir)/libspiralcore/include

SpiralSynthModular_LDADD = 							\
	$(FLTK_LIBS) $(MIDI_LIBS)					\
	-lm				\
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
	$(top_builddir)/GUI/Widgets/libSpiralPlugin.la\
                $(top_builddir)/libspiralcore/libspiralcore.la


EXTRA_DIST = autogen.sh build-aux/macos-bundle.py Examples
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
		GraphSort.h \
		GraphSort.C \
		main.cpp

AM_CPPFLAGS += -DPLUGIN_PATH_LOCATION=\"$(ssm_plugindir)/\"
ssm_plugindir = @PLUGINDIR@

# Widgets and loadable plugins resolve host symbols from the executable.
SpiralSynthModular_LDFLAGS = -export-dynamic

BUNDLE_DIR = $(abs_builddir)/SpiralSynthModular.app
.PHONY: bundle
if MACOS_BUNDLE
bundle: all
	@test "$(BUNDLE_PYTHON)" != no || { echo "Python 2.7 or 3 is required for bundle packaging"; exit 1; }
	+"$(BUNDLE_PYTHON)" "$(srcdir)/build-aux/macos-bundle.py" --make "$(MAKE)" \
		--bindir "$(bindir)" --libdir "$(libdir)" --plugindir "$(ssm_plugindir)" \
		--srcdir "$(srcdir)" --version "$(PACKAGE_VERSION)" --output "$(BUNDLE_DIR)"
else
bundle:
	@echo "Bundle packaging requires macOS"; exit 1
endif
