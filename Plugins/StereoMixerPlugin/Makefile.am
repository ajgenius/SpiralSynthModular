INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = StereoMixerPlugin.la
StereoMixerPlugin_la_SOURCES =					\
			StereoMixerPluginGUI.h			\
			StereoMixerPluginGUI.C			\
			StereoMixerPlugin.h			\
			StereoMixerPlugin.C
			
StereoMixerPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
StereoMixerPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
