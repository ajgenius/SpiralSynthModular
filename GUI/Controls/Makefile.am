# Toolkit controls with nothing of the synth in them: knobs, LEDs, meters,
# drag bars, loop and event displays, the Pawfal dialogs, the wave chooser.
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(FLTK_CXXFLAGS)

EXTRA_DIST = Fl_DragBar.fld Fl_Knob.fld Fl_LED_Button.fld Fl_SevenSeg.fld

lib_LTLIBRARIES = libSpiralControls.la
libSpiralControls_la_SOURCES = \
	Fl_DragBar.H \
	Fl_DragBar.cxx \
	Fl_Knob.H \
	Fl_Knob.cxx \
	Fl_LED_Button.H \
	Fl_LED_Button.cxx \
	Fl_SevenSeg.H \
	Fl_SevenSeg.cxx \
	Fl_VU_Meter.h \
	Fl_VU_Meter.cxx \
	Fl_Loop.h \
	Fl_Loop.C \
	Fl_Trigger.h \
	Fl_Trigger.C \
	Fl_EventMap.h \
	Fl_EventMap.C \
	Fl_SEvent.h \
	Fl_SEvent.C \
	PawfalInput.h \
	PawfalInput.C \
	PawfalYesNo.h \
	PawfalYesNo.C \
	WaveChooser.h \
	WaveChooser.C \
	WS_InvSine.h \
	WS_Noise.h \
	WS_Pulse1.h \
	WS_Pulse2.h \
	WS_RevSaw.h \
	WS_Saw.h \
	WS_Sine.h \
	WS_Square.h \
	WS_Tri.h
libSpiralControls_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralControls_la_LIBADD = $(FLTK_LIBS) -lm
