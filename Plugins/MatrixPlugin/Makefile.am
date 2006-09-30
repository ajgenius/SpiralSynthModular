INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MatrixPlugin.la
MatrixPlugin_la_SOURCES =					\
			MatrixPluginGUI.h			\
			MatrixPluginGUI.C			\
			MatrixPlugin.h			\
			MatrixPlugin.C
			
MatrixPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
MatrixPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
