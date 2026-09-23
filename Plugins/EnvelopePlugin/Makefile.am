INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = EnvelopePlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = EnvelopePlugin.la

EnvelopePlugin_la_SOURCES =					\
			EnvelopePluginGUI.h			\
			EnvelopePluginGUI.C			\
			EnvelopePlugin.h			\
			EnvelopePlugin.C
			
EnvelopePlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
EnvelopePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
