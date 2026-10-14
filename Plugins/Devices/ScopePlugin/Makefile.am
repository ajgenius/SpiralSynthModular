plugindir = @PLUGINDIR@/dsp/ScopePlugin
dist_plugin_DATA = info.json SpiralIcon.xpm
EXTRA_DIST =
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ScopePlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/default-ui.am
