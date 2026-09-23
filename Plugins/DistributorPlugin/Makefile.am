INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = DistributorPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = DistributorPlugin.la

DistributorPlugin_la_SOURCES =					\
			DistributorPluginGUI.h			\
			DistributorPluginGUI.C			\
			DistributorPlugin.h			\
			DistributorPlugin.C
			
DistributorPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
DistributorPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
