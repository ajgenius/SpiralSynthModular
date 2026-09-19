INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_srcdir)/libspiralcore/include
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = XFadePlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = XFadePlugin.la

XFadePlugin_la_SOURCES =					\
			../../GUI/XFadePlugin/XFadePluginGUI.h			\
			../../GUI/XFadePlugin/XFadePluginGUI.cpp			\
			XFadePlugin.h			\
			XFadePlugin.C
			
XFadePlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
XFadePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/XFadePlugin.la"

# The installed .la is removed, so libtool cannot discover the module at uninstall.
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/$(plugin)"

AM_CPPFLAGS = -I$(srcdir)

AM_CPPFLAGS += -I$(srcdir) -I$(top_srcdir)/Plugins/GUI/XFadePlugin
