INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = SwitchPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SwitchPlugin.la

SwitchPlugin_la_SOURCES =					\
			SwitchPluginGUI.h			\
			SwitchPluginGUI.C			\
			SwitchPlugin.h			\
			SwitchPlugin.C
			
SwitchPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SwitchPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
