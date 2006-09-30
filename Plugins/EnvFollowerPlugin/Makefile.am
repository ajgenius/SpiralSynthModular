INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = EnvFollowerPlugin.la
EnvFollowerPlugin_la_SOURCES =					\
			EnvFollowerPluginGUI.h			\
			EnvFollowerPluginGUI.C			\
			EnvFollowerPlugin.h			\
			EnvFollowerPlugin.C
			
EnvFollowerPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
EnvFollowerPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
