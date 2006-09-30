INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound		\
		-II$(top_builddir)/SpiralSound/Plugins

EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = TransposePlugin.la
TransposePlugin_la_SOURCES =					\
			../../ChannelHandler.h		\
			../../ChannelHandler.C		\
			../../Sample.h			\
			../../Sample.C			\
			../SpiralPlugin.h		\
			../SpiralPlugin.C		\
			../SpiralPluginGUI.h		\
			../SpiralPluginGUI.C		\
			TransposePluginGUI.h			\
			TransposePluginGUI.C			\
			TransposePlugin.h			\
			TransposePlugin.C
			
TransposePlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
TransposePlugin_la_LIBADD = 					\
			$(FLTK_LIBS)			\
			-lm -ldl -lpthread		\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la
