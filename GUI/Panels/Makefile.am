AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) \
	$(FLTK_CXXFLAGS) \
	$(SPIRALGUI_CFLAGS) $(SPIRALPANELS_CFLAGS) \
	$(SPIRALCORE_CFLAGS) \
	-I$(top_srcdir)/libs/libSpumoni/include

lib_LTLIBRARIES = libSpiralPanels.la
libSpiralPanels_la_SOURCES = \
	Panel.h \
	Panel.cpp \
	PanelControl.h \
	PanelControl.cpp \
	PanelControlRegistry.h \
	PanelControlRegistry.cpp \
	PanelProperty.h \
	PanelProperty.cpp \
	PanelEditor.h \
	PanelEditor.cpp \
	DeviceSource.h \
	DeviceSource.cpp \
	Controls/PanelButton.cpp \
	Controls/PanelButton.h \
	Controls/PanelCheckBox.cpp \
	Controls/PanelCheckBox.h \
	Controls/PanelChoice.cpp \
	Controls/PanelChoice.h \
	Scopes/EnvelopeCurveView.cpp \
	Scopes/EnvelopeCurveView.h \
	Scopes/PanelEnvelopeCurve.cpp \
	Scopes/PanelEnvelopeCurve.h \
	Scopes/PanelFunctionPlot.cpp \
	Scopes/PanelFunctionPlot.h \
	Controls/PanelFileChoosingButton.cpp \
	Controls/PanelFileChoosingButton.h \
	Scopes/PanelPianoKeys.cpp \
	Scopes/PanelPianoKeys.h \
	Controls/PanelKnob.cpp \
	Controls/PanelKnob.h \
	Scopes/PanelLED.cpp \
	Scopes/PanelLED.h \
	Scopes/PanelLEDButton.cpp \
	Scopes/PanelLEDButton.h \
	Controls/PanelLabel.cpp \
	Controls/PanelLabel.h \
	Controls/PanelListChoosingButton.cpp \
	Controls/PanelListChoosingButton.h \
	Spiral/PanelLoopWaveform.cpp \
	Spiral/PanelLoopWaveform.h \
	Scopes/PanelSampleWaveform.cpp \
	Scopes/PanelSampleWaveform.h \
	Scopes/PanelMeterBar.cpp \
	Scopes/PanelMeterBar.h \
	Scopes/PanelLCD.cpp \
	Scopes/PanelLCD.h \
	Spiral/PanelPortList.cpp \
	Spiral/PanelPortList.h \
	Scopes/PanelSevenSegment.cpp \
	Scopes/PanelSevenSegment.h \
	Controls/PanelNotebook.cpp \
	Controls/PanelNotebook.h \
	Controls/PanelRadioButton.cpp \
	Controls/PanelRadioButton.h \
	Controls/PanelSlider.cpp \
	Controls/PanelSlider.h \
	Scopes/PanelScope.cpp \
	Scopes/PanelScope.h \
	Controls/PanelSpinner.cpp \
	Spiral/PanelMatrixGrid.cpp \
	Spiral/PanelMatrixGrid.h \
	Controls/PanelSpinner.h \
	Controls/PanelTextEntry.cpp \
	Controls/PanelTextEntry.h \
	Spiral/PanelValueTable.cpp \
	Spiral/PanelValueTable.h \
	Controls/PanelVirtualScrollArea.cpp \
	Controls/PanelVirtualScrollArea.h \
	Scopes/ScopeView.cpp \
	Scopes/ScopeView.h

libSpiralPanels_la_LDFLAGS = -shared -version-info $(LT_VERSION_INFO)
libSpiralPanels_la_LIBADD = \
	$(FLTK_LIBS) -lm \
	$(top_builddir)/GUI/Widgets/libSpiralWidgets.la \
	$(top_builddir)/GUI/Controls/libSpiralControls.la \
	$(top_builddir)/libs/libspiralcore/libspiralcore.la
