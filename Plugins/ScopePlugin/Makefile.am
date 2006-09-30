INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = ScopePlugin.la
ScopePlugin_la_SOURCES =					\
			ScopePluginGUI.h			\
			ScopePluginGUI.C			\
			ScopePlugin.h			\
			ScopePlugin.C
			
ScopePlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
ScopePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
