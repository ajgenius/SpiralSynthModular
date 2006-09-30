INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MousePlugin.la
MousePlugin_la_SOURCES =					\
			scratch.h				\
			scratch.C				\
			MousePluginGUI.h			\
			MousePluginGUI.C			\
			MousePlugin.h				\
			MousePlugin.C
			
MousePlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
MousePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
