INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_srcdir)/libspiralcore/include
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = EnvFollowerPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = EnvFollowerPlugin.la

EnvFollowerPlugin_la_SOURCES =					\
			EnvFollowerPluginGUI.h			\
			EnvFollowerPluginGUI.C			\
			EnvFollowerPlugin.h			\
			EnvFollowerPlugin.C
			
EnvFollowerPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
EnvFollowerPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvFollowerPlugin.la"

# The installed .la is removed, so libtool cannot discover the module at uninstall.
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/$(plugin)"

AM_CPPFLAGS = -I$(srcdir)
