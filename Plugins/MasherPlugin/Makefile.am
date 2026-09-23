INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = MasherPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MasherPlugin.la

MasherPlugin_la_SOURCES =				\
			MasherPluginGUI.h		\
			MasherPluginGUI.C		\
			MasherPlugin.h			\
			MasherPlugin.C
			
MasherPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
MasherPlugin_la_LIBADD = 						\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
