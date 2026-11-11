AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir)/src -I$(top_srcdir)/libs/libSpumoni/include
AM_CXXFLAGS = -fPIC

noinst_LTLIBRARIES = libSpiralPanels.la
libSpiralPanels_la_SOURCES = \
	include/DataSource.h src/DataSource.cpp \
	include/Binding.h src/Binding.cpp \
	include/MemorySource.h src/MemorySource.cpp \
	include/RowSource.h src/RowSource.cpp \
	include/ValueRule.h src/ValueRule.cpp \
	include/Handlers.h src/Handlers.cpp \
	include/PanelNode.h src/PanelNode.cpp \
	include/PanelDocument.h src/PanelDocument.cpp \
	include/PanelContracts.h src/PanelContracts.cpp \
	include/PanelRepeat.h src/PanelRepeat.cpp \
	include/ControlView.h include/PanelSource.h include/BreakpointCurve.h \
	include/PanelProperty.h src/PanelProperty.cpp \
	include/PanelControl.h src/PanelControl.cpp \
	include/PanelControlRegistry.h src/PanelControlRegistry.cpp \
	include/Panel.h src/Panel.cpp \
	include/Controls.h src/Controls.cpp src/ControlSupport.h \
	include/NoteGrid.h src/NoteGrid.cpp \
	src/Basic/LEDButton.cpp \
	include/Input.h src/Input/EnvelopeCurve.cpp src/Input/ValueTable.cpp \
	include/Meters.h src/Meters/FunctionPlot.cpp src/Meters/LCD.cpp src/Meters/LED.cpp src/Meters/LoopWaveform.cpp src/Meters/MeterBar.cpp src/Meters/SampleWaveform.cpp src/Meters/Scope.cpp src/Meters/SevenSegment.cpp \
	include/Patch.h src/Patch/MatrixGrid.cpp src/Patch/PianoKeys.cpp src/Patch/PianoRoll.cpp src/Patch/PortList.cpp

contractsdir = $(pkgdatadir)/panel-contracts
dist_contracts_DATA = contracts/controls-1.json contracts/basic-1.json contracts/input-1.json contracts/meters-1.json contracts/patch-1.json

TEST_LIBS = libSpiralPanels.la $(top_builddir)/libs/libSpumoni/libSpumoni.la
check_PROGRAMS = value-rule-test handlers-test panel-document-test panel-contracts-test panel-repeat-test note-grid-test
TESTS = $(check_PROGRAMS)
TESTS_ENVIRONMENT = SRCDIR=$(abs_srcdir) TOP_SRCDIR=$(abs_top_srcdir)
value_rule_test_SOURCES = tests/value-rule.cpp
value_rule_test_LDADD = $(TEST_LIBS)
handlers_test_SOURCES = tests/handlers.cpp
handlers_test_LDADD = $(TEST_LIBS)
panel_document_test_SOURCES = tests/panel-document.cpp tests/PanelCorpus.h
panel_document_test_LDADD = $(TEST_LIBS)
panel_contracts_test_SOURCES = tests/panel-contracts.cpp tests/PanelCorpus.h
panel_contracts_test_LDADD = $(TEST_LIBS)
panel_repeat_test_SOURCES = tests/panel-repeat.cpp tests/PanelCorpus.h
panel_repeat_test_LDADD = $(TEST_LIBS)
note_grid_test_SOURCES = tests/note-grid.cpp
note_grid_test_LDADD = $(TEST_LIBS)
EXTRA_DIST = tests/bindings.json tests/handlers.json
