AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) \
	$(FLTK_CXXFLAGS) \
	$(SPIRALGUI_CFLAGS) $(SPIRALPANELS_CFLAGS) \
	$(SPIRALCORE_CFLAGS) \
	-I$(top_srcdir)/libs/libSpumoni/include

lib_LTLIBRARIES = libSpiralPanelWindow.la
libSpiralPanelWindow_la_SOURCES = \
	DeviceSource.h \
	DeviceSource.cpp \
	PanelWindow.h \
	PanelWindow.cpp

libSpiralPanelWindow_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralPanelWindow_la_LIBADD = \
	$(FLTK_LIBS) -lm \
	$(top_builddir)/libs/libSpiralPanels/toolkits/fltk/libSpiralPanelsFltk.la \
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
	$(top_builddir)/GUI/Controls/libSpiralControls.la \
	$(top_builddir)/libs/libspiralcore/libspiralcore.la
