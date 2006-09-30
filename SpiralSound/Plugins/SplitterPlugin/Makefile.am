INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound		\
		-II$(top_builddir)/SpiralSound/Plugins

EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SplitterPlugin.la
SplitterPlugin_la_SOURCES =					\
			../../ChannelHandler.h		\
			../../ChannelHandler.C		\
			../../Sample.h			\
			../../Sample.C			\
			../SpiralPlugin.h		\
			../SpiralPlugin.C		\
			../SpiralPluginGUI.h		\
			../SpiralPluginGUI.C		\
			SplitterPluginGUI.h			\
			SplitterPluginGUI.C			\
			SplitterPlugin.h			\
			SplitterPlugin.C
			
SplitterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SplitterPlugin_la_LIBADD = 					\
			$(FLTK_LIBS)			\
			-lm -ldl -lpthread		\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la
