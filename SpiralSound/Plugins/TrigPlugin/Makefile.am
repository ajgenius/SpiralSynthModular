INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound		\
		-II$(top_builddir)/SpiralSound/Plugins

EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = TrigPlugin.la
TrigPlugin_la_SOURCES =					\
			../../ChannelHandler.h		\
			../../ChannelHandler.C		\
			../../Sample.h			\
			../../Sample.C			\
			../SpiralPlugin.h		\
			../SpiralPlugin.C		\
			../SpiralPluginGUI.h		\
			../SpiralPluginGUI.C		\
			TrigPluginGUI.h			\
			TrigPluginGUI.C			\
			TrigPlugin.h			\
			TrigPlugin.C
			
TrigPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
TrigPlugin_la_LIBADD = 					\
			$(FLTK_LIBS)			\
			-lm -ldl -lpthread		\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la
