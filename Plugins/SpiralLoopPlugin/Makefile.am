INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = SpiralLoopPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SpiralLoopPlugin.la

SpiralLoopPlugin_la_SOURCES =					\
			SpiralLoopPluginGUI.h			\
			SpiralLoopPluginGUI.C			\
			SpiralLoopPlugin.h			\
			SpiralLoopPlugin.C
			
SpiralLoopPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SpiralLoopPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
