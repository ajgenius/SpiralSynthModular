INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = MixerPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MixerPlugin.la

MixerPlugin_la_SOURCES =					\
			MixerPluginGUI.h			\
			MixerPluginGUI.C			\
			MixerPlugin.h			\
			MixerPlugin.C
			
MixerPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
MixerPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
