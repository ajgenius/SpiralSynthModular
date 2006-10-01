INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = FormantFilterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = FormantFilterPlugin.la

FormantFilterPlugin_la_SOURCES =					\
			FormantFilterPluginGUI.h			\
			FormantFilterPluginGUI.C			\
			FormantFilterPlugin.h			\
			FormantFilterPlugin.C
			
FormantFilterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
FormantFilterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
