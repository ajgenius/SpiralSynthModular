INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = OperatorPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = OperatorPlugin.la

OperatorPlugin_la_SOURCES =					\
			OperatorPluginGUI.h			\
			OperatorPluginGUI.C			\
			OperatorPlugin.h			\
			OperatorPlugin.C
			
OperatorPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
OperatorPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
