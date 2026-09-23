INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = SeqSelectorPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SeqSelectorPlugin.la

SeqSelectorPlugin_la_SOURCES =					\
			SeqSelectorPluginGUI.h			\
			SeqSelectorPluginGUI.C			\
			SeqSelectorPlugin.h			\
			SeqSelectorPlugin.C
			
SeqSelectorPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SeqSelectorPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
