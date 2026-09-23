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
			
LADSPAPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
LADSPAPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin.la"

# The installed .la is removed, so libtool cannot discover the module at uninstall.
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/$(plugin)"
