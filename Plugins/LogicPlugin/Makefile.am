INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = LogicPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = LogicPlugin.la

LogicPlugin_la_SOURCES =					\
			LogicPluginGUI.h			\
			LogicPluginGUI.C			\
			LogicPlugin.h			\
			LogicPlugin.C
			
LogicPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
LogicPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
