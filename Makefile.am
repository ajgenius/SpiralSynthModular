AUTOMAKE_OPTIONS = foreign
SUBDIRS = JSON libspiralcore GUI Plugins

INCLUDES =                                              \
                $(FLTK_CXXFLAGS)                        \
                $(FLTK_CFLAGS)                          \
                -I$(top_builddir)/GUI/Widgets           \
                -I$(top_srcdir)/libspiralcore/include

LIBS = 							\
	$(FLTK_LIBS)					\
	-lm -ldl -lpthread				\
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
	$(top_builddir)/GUI/Widgets/libSpiralPlugin.la\
                $(top_builddir)/libspiralcore/libspiralcore.la


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
