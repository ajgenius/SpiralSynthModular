AUTOMAKE_OPTIONS = foreign
SUBDIRS = SpiralSound GUI Plugins

INCLUDES =                                              \
                $(FLTK_CXXFLAGS)                        \
                $(FLTK_CFLAGS)                          \
                -I$(top_builddir)/GUI/Widgets           \
                -I$(top_builddir)/SpiralSound

LIBS = 							\
	$(FLTK_LIBS)					\
	-lm -ldl -lpthread				\
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la\
	$(top_builddir)/SpiralSound/libSpiralCommon.la


EXTRA_DIST = autogen.sh	
SpiralPlugins = $(libdir)/SpiralPlugins

bin_PROGRAMS = SpiralSynthModular
SpiralSynthModular_SOURCES = \
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
