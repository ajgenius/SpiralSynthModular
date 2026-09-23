INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = LADSPAPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = LADSPAPlugin.la

LADSPAPlugin_la_SOURCES =					\
			LADSPAInfo.h				\
			LADSPAInfo.C				\
			LADSPAPluginGUI.h			\
			LADSPAPluginGUI.C			\
			LADSPAPlugin.h				\
			LADSPAPlugin.C
			
LADSPAPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
LADSPAPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
