# CMT (Richard Furse's Computer Music Toolkit), built as one LADSPA module
# and bundled beside the swh-plugins: see README.ssm.md.
#
# Upstream's own Makefile hides everything but ladspa_descriptor with a GNU
# linker version script; -export-symbols-regex does the same on every
# platform libtool knows, so CMT's classes cannot meet another plugin's.

AUTOMAKE_OPTIONS = subdir-objects

CMT_SRC = \
	src/am.cpp src/ambisonic.cpp src/amp.cpp src/analogue.cpp \
	src/canyondelay.cpp src/cmt.cpp src/delay.cpp src/descriptor.cpp \
	src/disintegrator.cpp src/dynamic.cpp src/filter.cpp src/grain.cpp \
	src/hardgate.cpp src/init.cpp src/lofi.cpp src/logistic.cpp \
	src/mixer.cpp src/noise.cpp src/null.cpp src/organ.cpp src/peak.cpp \
	src/phasemod.cpp src/pink.cpp src/pink_full.cpp src/pink_sh.cpp \
	src/sine.cpp src/sledgehammer.cpp src/syndrum.cpp src/vcf303.cpp \
	src/wshape_sine.cpp \
	src/freeverb/freeverb.cpp src/freeverb/Components/allpass.cpp \
	src/freeverb/Components/comb.cpp src/freeverb/Components/revmodel.cpp

CMT_HDR = \
	src/cmt.h src/ladspa_types.h src/pinknoise.h src/run_adding.h \
	src/utils.h \
	src/freeverb/Components/allpass.h src/freeverb/Components/comb.h \
	src/freeverb/Components/denormals.h src/freeverb/Components/revmodel.h \
	src/freeverb/Components/tuning.h

if BUNDLE_LADSPA_PLUGINS

ladspadir = @PLUGINDIR@/ladspa
ladspa_LTLIBRARIES = cmt.la

cmt_la_SOURCES = $(CMT_SRC) $(CMT_HDR)
cmt_la_CPPFLAGS = -I$(srcdir)/src -I$(top_srcdir)/third-party/ladspa
cmt_la_LDFLAGS = -module -avoid-version -shared -rpath $(ladspadir) \
	-export-symbols-regex '^ladspa_descriptor$$'

# Only the module is of any use once installed, as with the swh-plugins.
install-data-hook:
	-rm -f $(DESTDIR)$(ladspadir)/*.la

endif

EXTRA_DIST = \
	README.ssm.md README doc src/freeverb/readme.txt \
	$(CMT_SRC) $(CMT_HDR)
