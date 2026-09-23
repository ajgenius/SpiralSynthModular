INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = AnotherFilterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = AnotherFilterPlugin.la

AnotherFilterPlugin_la_SOURCES =					\
			AnotherFilterPluginGUI.h			\
			AnotherFilterPluginGUI.C			\
			AnotherFilterPlugin.h			\
			AnotherFilterPlugin.C
			
AnotherFilterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
AnotherFilterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
