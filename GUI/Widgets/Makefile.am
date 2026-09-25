AM_CPPFLAGS = -I$(top_srcdir)                                              \
                $(FLTK_CXXFLAGS)                        \
                -I$(top_srcdir)/GUI/Widgets           \
                -I$(top_srcdir)/libspiralcore/include

EXTRA_DIST = 				\
		Fl_DragBar.fld  	\
		Fl_Knob.fld		\
		Fl_LED_Button.fld	\
		Fl_SevenSeg.fld

lib_LTLIBRARIES = libSpiralPlugin.la libSpiralWidgets.la
libSpiralWidgets_la_SOURCES =					\
					Fl_Canvas.h		\
					Fl_Canvas.C		\
					Fl_CommentGUI.h		\
					Fl_CommentGUI.C		\
					Fl_DeviceGUI.h		\
					Fl_DeviceGUI.C		\
					Fl_DragBar.H		\
					Fl_DragBar.cxx		\
					Fl_EventMap.h		\
					Fl_EventMap.C		\
					Fl_Knob.H		\
					Fl_Knob.cxx		\
					Fl_LED_Button.H		\
					Fl_LED_Button.cxx	\
					Fl_Loop.h		\
					Fl_Loop.C		\
					Fl_SEvent.C		\
					Fl_SEvent.h		\
					Fl_SevenSeg.H		\
					Fl_SevenSeg.cxx		\
					Fl_Trigger.h		\
					Fl_Trigger.C		\
					Fl_VU_Meter.h		\
					Fl_VU_Meter.cxx		\
					PawfalInput.h		\
					PawfalInput.C		\
					PawfalYesNo.h		\
					PawfalYesNo.C		\
					PluginManager.h		\
					PluginManager.C		\
			                SpiralGUI.H		\
			                SpiralGUI.C		\
					SpiralPluginGUI.h	\
					SpiralPluginGUI.C	\
					WS_Pulse1.h		\
					WS_Pulse2.h		\
					WS_RevSaw.h		\
					WS_Saw.h		\
					WS_Sine.h		\
					WS_Square.h		\
					WS_Tri.h		\
					WaveChooser.h		\
					WaveChooser.C		\
					WS_InvSine.h		\
					WS_Noise.h
					
libSpiralWidgets_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralWidgets_la_LIBADD = 				\
				$(FLTK_LIBS)		\
				 -lm	\
                $(top_builddir)/libspiralcore/libspiralcore.la

libSpiralPlugin_la_SOURCES = SpiralPlugin.C SpiralPlugin.h
libSpiralPlugin_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la
libSpiralPlugin_la_LDFLAGS = -shared -no-undefined -version-info $(LT_VERSION_INFO)
libSpiralWidgets_la_LIBADD += libSpiralPlugin.la
