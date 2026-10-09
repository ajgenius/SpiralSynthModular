AM_CPPFLAGS = -I$(top_srcdir)                                              \
                $(FLTK_CXXFLAGS)                        \
                -I$(top_srcdir)/GUI/Widgets           \
                -I$(top_srcdir)/GUI/Controls          \
                $(SPIRALCORE_CFLAGS) \
                -I$(top_srcdir)/libs/libSpumoni/include \
                $(PORTAUDIO_CFLAGS) $(ALSA_CFLAGS)

lib_LTLIBRARIES = libSpiralWidgets.la
libSpiralWidgets_la_SOURCES =					\
					Fl_BoundaryDrawer.h	\
					Fl_BoundaryDrawer.cpp	\
					Fl_Canvas.h		\
					Fl_Canvas.C		\
					Fl_CommentGUI.h		\
					Fl_CommentGUI.C		\
					Fl_DeviceGUI.h		\
					Fl_DeviceGUI.C		\
					OutputPluginGUI.h	\
					OutputPluginGUI.cpp	\
			                SpiralGUI.H		\
			                SpiralGUI.C		\
					SpiralPluginGUI.h	\
					SpiralPluginGUI.C

libSpiralWidgets_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralWidgets_la_LIBADD = 				\
				$(FLTK_LIBS)		\
				 -lm	\
                $(top_builddir)/GUI/Controls/libSpiralControls.la \
                $(top_builddir)/libs/libspiralcore/libspiralcore.la

