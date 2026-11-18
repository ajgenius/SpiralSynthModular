AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/Input -I$(srcdir)/Meters \
	-I$(top_srcdir)/libs/libSpiralPanels/include \
	-I$(top_srcdir)/libs/libSpumoni/include \
	-I$(top_srcdir)/GUI/Controls \
	$(FLTK_CXXFLAGS)

lib_LTLIBRARIES = libSpiralPanelsFltk.la
libSpiralPanelsFltk_la_SOURCES = \
	FltkView.h \
	FltkView.cpp \
	FltkSupport.h \
	FltkControls.cpp \
	FltkTheme.h \
	FltkTheme.cpp \
	Basic/LEDButton.cpp \
	Input/EnvelopeCurve.cpp \
	Input/EnvelopeCurveView.cpp \
	Input/EnvelopeCurveView.h \
	Input/ValueTable.cpp \
	Meters/FunctionPlot.cpp \
	Meters/LCD.cpp \
	Meters/LED.cpp \
	Meters/LoopWaveform.cpp \
	Meters/MeterBar.cpp \
	Meters/SampleWaveform.cpp \
	Meters/Scope.cpp \
	Meters/ScopeView.cpp \
	Meters/ScopeView.h \
	Meters/TimingNotation.cpp Meters/TimingNotationDrawing.h \
	Meters/SevenSegment.cpp \
	Patch/MatrixGrid.cpp \
	Patch/PianoKeys.cpp \
	Patch/PianoRoll.cpp \
	Patch/PortList.cpp

libSpiralPanelsFltk_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralPanelsFltk_la_LIBADD = \
	$(FLTK_LIBS) -lm \
	$(top_builddir)/GUI/Controls/libSpiralControls.la \
	$(top_builddir)/libs/libspiralcore/libspiralcore.la

EMBED_MANIFEST = $(top_builddir)/tools/embed-manifest/embed-manifest$(EXEEXT)
nodist_libSpiralPanelsFltk_la_SOURCES = theme.cpp
BUILT_SOURCES = theme.cpp
CLEANFILES = theme.cpp
EXTRA_DIST = plastic.fltk.json

theme.cpp: $(srcdir)/plastic.fltk.json $(EMBED_MANIFEST)
	$(EMBED_MANIFEST) --data $(srcdir)/plastic.fltk.json $@ PlasticTheme
