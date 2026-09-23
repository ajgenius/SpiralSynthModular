INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm

plugin = ControllerPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = ControllerPlugin.la

ControllerPlugin_la_SOURCES =					\
			ControllerPluginGUI.h			\
			ControllerPluginGUI.C			\
			ControllerPlugin.h			\
			ControllerPlugin.C
			
ControllerPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
ControllerPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
