INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SmoothPlugin.la
SmoothPlugin_la_SOURCES =					\
			SmoothPluginGUI.h			\
			SmoothPluginGUI.C			\
			SmoothPlugin.h			\
			SmoothPlugin.C
			
SmoothPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SmoothPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
