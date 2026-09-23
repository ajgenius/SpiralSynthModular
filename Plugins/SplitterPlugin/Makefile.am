INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = SplitterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SplitterPlugin.la

SplitterPlugin_la_SOURCES =					\
			SplitterPluginGUI.h			\
			SplitterPluginGUI.C			\
			SplitterPlugin.h			\
			SplitterPlugin.C
			
SplitterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SplitterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
