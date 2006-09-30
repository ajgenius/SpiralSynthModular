INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MidiPlugin.la
MidiPlugin_la_SOURCES =					\
			MidiPluginGUI.h			\
			MidiPluginGUI.C			\
			MidiPlugin.h			\
			MidiPlugin.C
			
MidiPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
MidiPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
