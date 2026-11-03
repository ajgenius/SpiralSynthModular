# BLOP (Mike Rawes' bandlimited oscillators and their companions), LADSPA
# modules bundled beside the swh-plugins until the native ports arrive: see
# README.ssm.md.
#
# Each plugin is its own module, under the file name upstream installs. The
# three oscillators' wavetables are modules of their own too, generated at
# build time by upstream's wavegen with upstream's default settings, and
# found at run time in blop_files/ under a directory on LADSPA_PATH, which
# the host's own plugin directory always is.

AUTOMAKE_OPTIONS = subdir-objects

BLOP_SRC = \
	src/adsr_1653.so.c src/adsr_1680.so.c src/amp_1654.so.c \
	src/branch_1673.so.c src/dahdsr_2021.so.c src/difference_2030.so.c \
	src/fmod_1656.so.c src/interpolator_1660.so.c src/lp4pole_1671.so.c \
	src/lp4pole_filter.c src/product_1668.so.c src/pulse_1645.so.c \
	src/quantiser.so.c src/random_1661.so.c src/ratio_2034.so.c \
	src/sawtooth_1641.so.c src/sequencer.so.c src/square_1643.so.c \
	src/sum_1665.so.c src/sync_pulse_2023.so.c src/sync_square_1678.so.c \
	src/tracker_2025.so.c src/triangle_1649.so.c src/wavedata.c \
	src/wavegen.c src/wdatutil.c

BLOP_HDR = \
	src/include/common.h src/include/float_cast.h src/include/gettext.h \
	src/include/interpolate.h src/include/lp4pole_filter.h \
	src/include/math_func.h src/include/wavedata.h src/include/wdatutil.h

if BUNDLE_LADSPA_PLUGINS

ladspadir = @PLUGINDIR@/ladspa
blopdatadir = $(ladspadir)/blop_files

# Upstream's configure defines PACKAGE and LOCALEDIR, which the plugins hand
# to gettext (a no-op here: NLS is off), WAVEDATA_SUBDIR, and HAVE_LRINT,
# HAVE_LRINTF and HAVE_SINF, which every C library now has: without them
# float_cast.h's stand-in clashes with the real lrintf, and math_func.h's
# double fallback calls pow with one argument. They are given here rather
# than in config.h because float_cast.h is read before config.h. _init and
# _fini are renamed and run by blop-init.c, which says why.
AM_CPPFLAGS = -I$(srcdir)/src/include -I$(top_srcdir)/third-party/ladspa \
	-DPACKAGE=\"blop\" -DLOCALEDIR=\"$(localedir)\" \
	-DWAVEDATA_SUBDIR=\"blop_files\" -DHAVE_LRINT=1 -DHAVE_LRINTF=1 -DHAVE_SINF=1 \
	-D_init=blop_init -D_fini=blop_fini

# As for the swh-plugins: modules, no version suffix, and -module repeated
# per target because automake will not take it from AM_LDFLAGS.
MODULE_LDFLAGS = -avoid-version -shared -rpath $(ladspadir) \
	-export-symbols-regex '^ladspa_descriptor$$'

# The three quantisers (2027-2029) are not built: upstream has to compile
# them with -O0, which a target here cannot force over the user's CFLAGS.
ladspa_LTLIBRARIES = \
	sawtooth_1641.la square_1643.la pulse_1645.la triangle_1649.la \
	adsr_1653.la amp_1654.la fmod_1656.la interpolator_1660.la \
	random_1661.la sum_1665.la product_1668.la lp4pole_1671.la \
	branch_1673.la sequencer64_1675.la sequencer32_1676.la \
	sequencer16_1677.la sync_square_1678.la adsr_1680.la dahdsr_2021.la \
	sync_pulse_2023.la tracker_2025.la difference_2030.la ratio_2034.la

sawtooth_1641_la_SOURCES = src/sawtooth_1641.so.c src/wavedata.c blop-init.c
sawtooth_1641_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sawtooth_1641_la_LIBADD = -lm
square_1643_la_SOURCES = src/square_1643.so.c src/wavedata.c blop-init.c
square_1643_la_LDFLAGS = -module $(MODULE_LDFLAGS)
square_1643_la_LIBADD = -lm
pulse_1645_la_SOURCES = src/pulse_1645.so.c src/wavedata.c blop-init.c
pulse_1645_la_LDFLAGS = -module $(MODULE_LDFLAGS)
pulse_1645_la_LIBADD = -lm
triangle_1649_la_SOURCES = src/triangle_1649.so.c src/wavedata.c blop-init.c
triangle_1649_la_LDFLAGS = -module $(MODULE_LDFLAGS)
triangle_1649_la_LIBADD = -lm
adsr_1653_la_SOURCES = src/adsr_1653.so.c blop-init.c
adsr_1653_la_LDFLAGS = -module $(MODULE_LDFLAGS)
adsr_1653_la_LIBADD = -lm
amp_1654_la_SOURCES = src/amp_1654.so.c blop-init.c
amp_1654_la_LDFLAGS = -module $(MODULE_LDFLAGS)
amp_1654_la_LIBADD = -lm
fmod_1656_la_SOURCES = src/fmod_1656.so.c blop-init.c
fmod_1656_la_LDFLAGS = -module $(MODULE_LDFLAGS)
fmod_1656_la_LIBADD = -lm
interpolator_1660_la_SOURCES = src/interpolator_1660.so.c blop-init.c
interpolator_1660_la_LDFLAGS = -module $(MODULE_LDFLAGS)
interpolator_1660_la_LIBADD = -lm
random_1661_la_SOURCES = src/random_1661.so.c blop-init.c
random_1661_la_LDFLAGS = -module $(MODULE_LDFLAGS)
random_1661_la_LIBADD = -lm
sum_1665_la_SOURCES = src/sum_1665.so.c blop-init.c
sum_1665_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sum_1665_la_LIBADD = -lm
product_1668_la_SOURCES = src/product_1668.so.c blop-init.c
product_1668_la_LDFLAGS = -module $(MODULE_LDFLAGS)
product_1668_la_LIBADD = -lm
lp4pole_1671_la_SOURCES = src/lp4pole_1671.so.c src/lp4pole_filter.c blop-init.c
lp4pole_1671_la_LDFLAGS = -module $(MODULE_LDFLAGS)
lp4pole_1671_la_LIBADD = -lm
branch_1673_la_SOURCES = src/branch_1673.so.c blop-init.c
branch_1673_la_LDFLAGS = -module $(MODULE_LDFLAGS)
branch_1673_la_LIBADD = -lm
sequencer64_1675_la_SOURCES = src/sequencer.so.c blop-init.c
sequencer64_1675_la_CPPFLAGS = $(AM_CPPFLAGS) -DSEQUENCER_MAX_INPUTS=64 -DSEQUENCER_BASE_ID=1675
sequencer64_1675_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sequencer64_1675_la_LIBADD = -lm
sequencer32_1676_la_SOURCES = src/sequencer.so.c blop-init.c
sequencer32_1676_la_CPPFLAGS = $(AM_CPPFLAGS) -DSEQUENCER_MAX_INPUTS=32 -DSEQUENCER_BASE_ID=1676
sequencer32_1676_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sequencer32_1676_la_LIBADD = -lm
sequencer16_1677_la_SOURCES = src/sequencer.so.c blop-init.c
sequencer16_1677_la_CPPFLAGS = $(AM_CPPFLAGS) -DSEQUENCER_MAX_INPUTS=16 -DSEQUENCER_BASE_ID=1677
sequencer16_1677_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sequencer16_1677_la_LIBADD = -lm
sync_square_1678_la_SOURCES = src/sync_square_1678.so.c blop-init.c
sync_square_1678_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sync_square_1678_la_LIBADD = -lm
adsr_1680_la_SOURCES = src/adsr_1680.so.c blop-init.c
adsr_1680_la_LDFLAGS = -module $(MODULE_LDFLAGS)
adsr_1680_la_LIBADD = -lm
dahdsr_2021_la_SOURCES = src/dahdsr_2021.so.c blop-init.c
dahdsr_2021_la_LDFLAGS = -module $(MODULE_LDFLAGS)
dahdsr_2021_la_LIBADD = -lm
sync_pulse_2023_la_SOURCES = src/sync_pulse_2023.so.c blop-init.c
sync_pulse_2023_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sync_pulse_2023_la_LIBADD = -lm
tracker_2025_la_SOURCES = src/tracker_2025.so.c blop-init.c
tracker_2025_la_LDFLAGS = -module $(MODULE_LDFLAGS)
tracker_2025_la_LIBADD = -lm
difference_2030_la_SOURCES = src/difference_2030.so.c blop-init.c
difference_2030_la_LDFLAGS = -module $(MODULE_LDFLAGS)
difference_2030_la_LIBADD = -lm
ratio_2034_la_SOURCES = src/ratio_2034.so.c blop-init.c
ratio_2034_la_LDFLAGS = -module $(MODULE_LDFLAGS)
ratio_2034_la_LIBADD = -lm

noinst_PROGRAMS = wavegen
wavegen_SOURCES = src/wavegen.c src/wdatutil.c
wavegen_LDADD = -lm

blopdata_LTLIBRARIES = sawtooth_1641_data.la square_1643_data.la parabola_1649_data.la
WAVEDATA_LDFLAGS = -module -avoid-version -shared -rpath $(blopdatadir)
sawtooth_1641_data_la_SOURCES = blop-data-init.c
nodist_sawtooth_1641_data_la_SOURCES = sawtooth_1641_data.so.c
sawtooth_1641_data_la_LDFLAGS = $(WAVEDATA_LDFLAGS)
square_1643_data_la_SOURCES = blop-data-init.c
nodist_square_1643_data_la_SOURCES = square_1643_data.so.c
square_1643_data_la_LDFLAGS = $(WAVEDATA_LDFLAGS)
parabola_1649_data_la_SOURCES = blop-data-init.c
nodist_parabola_1649_data_la_SOURCES = parabola_1649_data.so.c
parabola_1649_data_la_LDFLAGS = $(WAVEDATA_LDFLAGS)

# Upstream's defaults: tables for 48 kHz from note 12, every note, at least
# 128 samples, full Gibbs compensation.
WAVEGEN = ./wavegen$(EXEEXT) -r 48000 -f 12 -s 1 -m 128 -g 1.0
sawtooth_1641_data.so.c: wavegen$(EXEEXT)
	$(AM_V_GEN)$(WAVEGEN) -w saw -p sawtooth -o $@
square_1643_data.so.c: wavegen$(EXEEXT)
	$(AM_V_GEN)$(WAVEGEN) -w square -p square -o $@
parabola_1649_data.so.c: wavegen$(EXEEXT)
	$(AM_V_GEN)$(WAVEGEN) -w parabola -p parabola -o $@

CLEANFILES = sawtooth_1641_data.so.c square_1643_data.so.c parabola_1649_data.so.c

# Only the modules are of any use once installed, as with the swh-plugins.
install-data-hook:
	-rm -f $(DESTDIR)$(ladspadir)/*.la $(DESTDIR)$(blopdatadir)/*.la

endif

EXTRA_DIST = \
	README.ssm.md COPYING AUTHORS README ChangeLog NEWS THANKS TODO \
	doc/about.txt doc/blop.rdf doc/plugins_list.txt \
	config.h blop-init.c blop-data-init.c $(BLOP_SRC) $(BLOP_HDR)
