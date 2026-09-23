INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = FilterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = FilterPlugin.la

FilterPlugin_la_SOURCES =					\
			iir_filter.h				\
			iir_filter.C				\
			FilterPluginGUI.h			\
			FilterPluginGUI.C			\
			FilterPlugin.h				\
			FilterPlugin.C
			
FilterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
FilterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
