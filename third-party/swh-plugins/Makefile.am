# swh-plugins, built as LADSPA modules and bundled with the macOS application.
#
# Only built where the platform has no LADSPA packages of its own: see
# README.ssm.md. The C is generated from the XML descriptions by upstream's
# makestub.pl, so the tree carries descriptions rather than generated sources.
#
# These are compiled without $(top_builddir) on the include path, so that the
# config.h they include is this directory's and not SSM's.

AUTOMAKE_OPTIONS = subdir-objects

ALL_PLUGIN_XML = \
	alias_1407.xml allpass_1895.xml am_pitchshift_1433.xml amp_1181.xml \
	analogue_osc_1416.xml bandpass_a_iir_1893.xml bandpass_iir_1892.xml \
	bode_shifter_1431.xml bode_shifter_cv_1432.xml butterworth_1902.xml \
	chebstortion_1430.xml comb_1190.xml comb_1887.xml \
	comb_splitter_1411.xml const_1909.xml crossover_dist_1404.xml \
	dc_remove_1207.xml debug_1184.xml decay_1886.xml decimator_1202.xml \
	declip_1195.xml delay_1898.xml delayorama_1402.xml diode_1185.xml \
	divider_1186.xml dj_eq_1901.xml dj_flanger_1438.xml \
	dyson_compress_1403.xml fad_delay_1192.xml \
	fast_lookahead_limiter_1913.xml flanger_1191.xml fm_osc_1415.xml \
	foldover_1213.xml foverdrive_1196.xml freq_tracker_1418.xml \
	gate_1410.xml gate_1921.xml giant_flange_1437.xml gong_1424.xml \
	gong_beater_1439.xml gsm_1215.xml gverb_1216.xml hard_limiter_1413.xml \
	harmonic_gen_1220.xml hermes_filter_1200.xml highpass_iir_1890.xml \
	hilbert_1440.xml imp_1199.xml impulse_1885.xml inv_1429.xml \
	karaoke_1409.xml latency_1914.xml lcr_delay_1436.xml \
	lowpass_iir_1891.xml ls_filter_1908.xml matrix_ms_st_1421.xml \
	matrix_spatialiser_1422.xml matrix_st_ms_1420.xml mbeq_1197.xml \
	mod_delay_1419.xml multivoice_chorus_1201.xml notch_iir_1894.xml \
	phasers_1217.xml pitch_scale_1193.xml pitch_scale_1194.xml \
	plate_1423.xml pointer_cast_1910.xml rate_shifter_1417.xml \
	retro_flange_1208.xml revdelay_1605.xml ringmod_1188.xml \
	satan_maximiser_1408.xml sc1_1425.xml sc2_1426.xml sc3_1427.xml \
	sc4_1882.xml sc4m_1916.xml se4_1883.xml shaper_1187.xml \
	sifter_1210.xml sin_cos_1881.xml single_para_1203.xml \
	sinus_wavewrapper_1198.xml smooth_decimate_1414.xml split_1406.xml \
	step_muxer_1212.xml surround_encoder_1401.xml svf_1214.xml \
	tape_delay_1211.xml transient_1206.xml triple_para_1204.xml \
	valve_1209.xml valve_rect_1405.xml vocoder_1337.xml vynil_1905.xml \
	wave_terrain_1412.xml xfade_1915.xml zm1_1428.xml

if BUNDLE_LADSPA_PLUGINS

ladspadir = @PLUGINDIR@/ladspa

AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/util -I$(top_srcdir)/third-party/ladspa

# Shared DSP the plugins draw on, as a libtool convenience library so each
# module links only what it references and everything is built PIC. The GSM
# codec is upstream's own copy, so gsm_1215 needs no library from the host.
noinst_LTLIBRARIES = libswhsupport.la
libswhsupport_la_SOURCES = \
	util/blo.c util/buffer.c util/db.c util/iir.c util/rms.c \
	gverb/gverb.c gverb/gverbdsp.c \
	gsm/add.c gsm/code.c gsm/decode.c gsm/gsm_create.c gsm/gsm_decode.c \
	gsm/gsm_destroy.c gsm/gsm_encode.c gsm/gsm_option.c gsm/long_term.c \
	gsm/lpc.c gsm/preprocess.c gsm/rpe.c gsm/short_term.c gsm/table.c

PLUGIN_IDS = \
	alias_1407 allpass_1895 am_pitchshift_1433 amp_1181 analogue_osc_1416 \
	bandpass_a_iir_1893 bandpass_iir_1892 bode_shifter_1431 \
	bode_shifter_cv_1432 butterworth_1902 chebstortion_1430 comb_1190 \
	comb_1887 comb_splitter_1411 const_1909 crossover_dist_1404 \
	dc_remove_1207 debug_1184 decay_1886 decimator_1202 declip_1195 \
	delay_1898 delayorama_1402 diode_1185 divider_1186 dj_eq_1901 \
	dj_flanger_1438 dyson_compress_1403 fad_delay_1192 \
	fast_lookahead_limiter_1913 flanger_1191 fm_osc_1415 foldover_1213 \
	foverdrive_1196 freq_tracker_1418 gate_1410 gate_1921 \
	giant_flange_1437 gong_1424 gong_beater_1439 gsm_1215 gverb_1216 \
	hard_limiter_1413 harmonic_gen_1220 hermes_filter_1200 \
	highpass_iir_1890 hilbert_1440 impulse_1885 inv_1429 karaoke_1409 \
	latency_1914 lcr_delay_1436 lowpass_iir_1891 ls_filter_1908 \
	matrix_ms_st_1421 matrix_spatialiser_1422 matrix_st_ms_1420 \
	mod_delay_1419 multivoice_chorus_1201 notch_iir_1894 phasers_1217 \
	plate_1423 pointer_cast_1910 rate_shifter_1417 retro_flange_1208 \
	revdelay_1605 ringmod_1188 satan_maximiser_1408 sc1_1425 sc2_1426 \
	sc3_1427 sc4_1882 sc4m_1916 se4_1883 shaper_1187 sifter_1210 \
	sin_cos_1881 single_para_1203 sinus_wavewrapper_1198 \
	smooth_decimate_1414 split_1406 step_muxer_1212 surround_encoder_1401 \
	svf_1214 tape_delay_1211 transient_1206 triple_para_1204 valve_1209 \
	valve_rect_1405 vocoder_1337 vynil_1905 wave_terrain_1412 xfade_1915 \
	zm1_1428

ladspa_LTLIBRARIES = \
	alias_1407.la allpass_1895.la am_pitchshift_1433.la amp_1181.la \
	analogue_osc_1416.la bandpass_a_iir_1893.la bandpass_iir_1892.la \
	bode_shifter_1431.la bode_shifter_cv_1432.la butterworth_1902.la \
	chebstortion_1430.la comb_1190.la comb_1887.la comb_splitter_1411.la \
	const_1909.la crossover_dist_1404.la dc_remove_1207.la debug_1184.la \
	decay_1886.la decimator_1202.la declip_1195.la delay_1898.la \
	delayorama_1402.la diode_1185.la divider_1186.la dj_eq_1901.la \
	dj_flanger_1438.la dyson_compress_1403.la fad_delay_1192.la \
	fast_lookahead_limiter_1913.la flanger_1191.la fm_osc_1415.la \
	foldover_1213.la foverdrive_1196.la freq_tracker_1418.la gate_1410.la \
	gate_1921.la giant_flange_1437.la gong_1424.la gong_beater_1439.la \
	gsm_1215.la gverb_1216.la hard_limiter_1413.la harmonic_gen_1220.la \
	hermes_filter_1200.la highpass_iir_1890.la hilbert_1440.la \
	impulse_1885.la inv_1429.la karaoke_1409.la latency_1914.la \
	lcr_delay_1436.la lowpass_iir_1891.la ls_filter_1908.la \
	matrix_ms_st_1421.la matrix_spatialiser_1422.la matrix_st_ms_1420.la \
	mod_delay_1419.la multivoice_chorus_1201.la notch_iir_1894.la \
	phasers_1217.la plate_1423.la pointer_cast_1910.la \
	rate_shifter_1417.la retro_flange_1208.la revdelay_1605.la \
	ringmod_1188.la satan_maximiser_1408.la sc1_1425.la sc2_1426.la \
	sc3_1427.la sc4_1882.la sc4m_1916.la se4_1883.la shaper_1187.la \
	sifter_1210.la sin_cos_1881.la single_para_1203.la \
	sinus_wavewrapper_1198.la smooth_decimate_1414.la split_1406.la \
	step_muxer_1212.la surround_encoder_1401.la svf_1214.la \
	tape_delay_1211.la transient_1206.la triple_para_1204.la valve_1209.la \
	valve_rect_1405.la vocoder_1337.la vynil_1905.la wave_terrain_1412.la \
	xfade_1915.la zm1_1428.la

# Modules, not libraries: no version suffix, and libtool wants -rpath for a
# destination that is not a library search path. Automake will not take
# -module from AM_LDFLAGS for a target whose name does not begin with lib, so
# every module repeats it; the rest of the flags stay in one place.
MODULE_LDFLAGS = -avoid-version -shared -rpath $(ladspadir)

alias_1407_la_LDFLAGS = -module $(MODULE_LDFLAGS)
alias_1407_la_LIBADD = libswhsupport.la -lm
allpass_1895_la_LDFLAGS = -module $(MODULE_LDFLAGS)
allpass_1895_la_LIBADD = libswhsupport.la -lm
am_pitchshift_1433_la_LDFLAGS = -module $(MODULE_LDFLAGS)
am_pitchshift_1433_la_LIBADD = libswhsupport.la -lm
amp_1181_la_LDFLAGS = -module $(MODULE_LDFLAGS)
amp_1181_la_LIBADD = libswhsupport.la -lm
analogue_osc_1416_la_LDFLAGS = -module $(MODULE_LDFLAGS)
analogue_osc_1416_la_LIBADD = libswhsupport.la -lm
bandpass_a_iir_1893_la_LDFLAGS = -module $(MODULE_LDFLAGS)
bandpass_a_iir_1893_la_LIBADD = libswhsupport.la -lm
bandpass_iir_1892_la_LDFLAGS = -module $(MODULE_LDFLAGS)
bandpass_iir_1892_la_LIBADD = libswhsupport.la -lm
bode_shifter_1431_la_LDFLAGS = -module $(MODULE_LDFLAGS)
bode_shifter_1431_la_LIBADD = libswhsupport.la -lm
bode_shifter_cv_1432_la_LDFLAGS = -module $(MODULE_LDFLAGS)
bode_shifter_cv_1432_la_LIBADD = libswhsupport.la -lm
butterworth_1902_la_LDFLAGS = -module $(MODULE_LDFLAGS)
butterworth_1902_la_LIBADD = libswhsupport.la -lm
chebstortion_1430_la_LDFLAGS = -module $(MODULE_LDFLAGS)
chebstortion_1430_la_LIBADD = libswhsupport.la -lm
comb_1190_la_LDFLAGS = -module $(MODULE_LDFLAGS)
comb_1190_la_LIBADD = libswhsupport.la -lm
comb_1887_la_LDFLAGS = -module $(MODULE_LDFLAGS)
comb_1887_la_LIBADD = libswhsupport.la -lm
comb_splitter_1411_la_LDFLAGS = -module $(MODULE_LDFLAGS)
comb_splitter_1411_la_LIBADD = libswhsupport.la -lm
const_1909_la_LDFLAGS = -module $(MODULE_LDFLAGS)
const_1909_la_LIBADD = libswhsupport.la -lm
crossover_dist_1404_la_LDFLAGS = -module $(MODULE_LDFLAGS)
crossover_dist_1404_la_LIBADD = libswhsupport.la -lm
dc_remove_1207_la_LDFLAGS = -module $(MODULE_LDFLAGS)
dc_remove_1207_la_LIBADD = libswhsupport.la -lm
debug_1184_la_LDFLAGS = -module $(MODULE_LDFLAGS)
debug_1184_la_LIBADD = libswhsupport.la -lm
decay_1886_la_LDFLAGS = -module $(MODULE_LDFLAGS)
decay_1886_la_LIBADD = libswhsupport.la -lm
decimator_1202_la_LDFLAGS = -module $(MODULE_LDFLAGS)
decimator_1202_la_LIBADD = libswhsupport.la -lm
declip_1195_la_LDFLAGS = -module $(MODULE_LDFLAGS)
declip_1195_la_LIBADD = libswhsupport.la -lm
delay_1898_la_LDFLAGS = -module $(MODULE_LDFLAGS)
delay_1898_la_LIBADD = libswhsupport.la -lm
delayorama_1402_la_LDFLAGS = -module $(MODULE_LDFLAGS)
delayorama_1402_la_LIBADD = libswhsupport.la -lm
diode_1185_la_LDFLAGS = -module $(MODULE_LDFLAGS)
diode_1185_la_LIBADD = libswhsupport.la -lm
divider_1186_la_LDFLAGS = -module $(MODULE_LDFLAGS)
divider_1186_la_LIBADD = libswhsupport.la -lm
dj_eq_1901_la_LDFLAGS = -module $(MODULE_LDFLAGS)
dj_eq_1901_la_LIBADD = libswhsupport.la -lm
dj_flanger_1438_la_LDFLAGS = -module $(MODULE_LDFLAGS)
dj_flanger_1438_la_LIBADD = libswhsupport.la -lm
dyson_compress_1403_la_LDFLAGS = -module $(MODULE_LDFLAGS)
dyson_compress_1403_la_LIBADD = libswhsupport.la -lm
fad_delay_1192_la_LDFLAGS = -module $(MODULE_LDFLAGS)
fad_delay_1192_la_LIBADD = libswhsupport.la -lm
fast_lookahead_limiter_1913_la_LDFLAGS = -module $(MODULE_LDFLAGS)
fast_lookahead_limiter_1913_la_LIBADD = libswhsupport.la -lm
flanger_1191_la_LDFLAGS = -module $(MODULE_LDFLAGS)
flanger_1191_la_LIBADD = libswhsupport.la -lm
fm_osc_1415_la_LDFLAGS = -module $(MODULE_LDFLAGS)
fm_osc_1415_la_LIBADD = libswhsupport.la -lm
foldover_1213_la_LDFLAGS = -module $(MODULE_LDFLAGS)
foldover_1213_la_LIBADD = libswhsupport.la -lm
foverdrive_1196_la_LDFLAGS = -module $(MODULE_LDFLAGS)
foverdrive_1196_la_LIBADD = libswhsupport.la -lm
freq_tracker_1418_la_LDFLAGS = -module $(MODULE_LDFLAGS)
freq_tracker_1418_la_LIBADD = libswhsupport.la -lm
gate_1410_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gate_1410_la_LIBADD = libswhsupport.la -lm
gate_1921_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gate_1921_la_LIBADD = libswhsupport.la -lm
giant_flange_1437_la_LDFLAGS = -module $(MODULE_LDFLAGS)
giant_flange_1437_la_LIBADD = libswhsupport.la -lm
gong_1424_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gong_1424_la_LIBADD = libswhsupport.la -lm
gong_beater_1439_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gong_beater_1439_la_LIBADD = libswhsupport.la -lm
gsm_1215_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gsm_1215_la_LIBADD = libswhsupport.la -lm
gverb_1216_la_LDFLAGS = -module $(MODULE_LDFLAGS)
gverb_1216_la_LIBADD = libswhsupport.la -lm
hard_limiter_1413_la_LDFLAGS = -module $(MODULE_LDFLAGS)
hard_limiter_1413_la_LIBADD = libswhsupport.la -lm
harmonic_gen_1220_la_LDFLAGS = -module $(MODULE_LDFLAGS)
harmonic_gen_1220_la_LIBADD = libswhsupport.la -lm
hermes_filter_1200_la_LDFLAGS = -module $(MODULE_LDFLAGS)
hermes_filter_1200_la_LIBADD = libswhsupport.la -lm
highpass_iir_1890_la_LDFLAGS = -module $(MODULE_LDFLAGS)
highpass_iir_1890_la_LIBADD = libswhsupport.la -lm
hilbert_1440_la_LDFLAGS = -module $(MODULE_LDFLAGS)
hilbert_1440_la_LIBADD = libswhsupport.la -lm
impulse_1885_la_LDFLAGS = -module $(MODULE_LDFLAGS)
impulse_1885_la_LIBADD = libswhsupport.la -lm
inv_1429_la_LDFLAGS = -module $(MODULE_LDFLAGS)
inv_1429_la_LIBADD = libswhsupport.la -lm
karaoke_1409_la_LDFLAGS = -module $(MODULE_LDFLAGS)
karaoke_1409_la_LIBADD = libswhsupport.la -lm
latency_1914_la_LDFLAGS = -module $(MODULE_LDFLAGS)
latency_1914_la_LIBADD = libswhsupport.la -lm
lcr_delay_1436_la_LDFLAGS = -module $(MODULE_LDFLAGS)
lcr_delay_1436_la_LIBADD = libswhsupport.la -lm
lowpass_iir_1891_la_LDFLAGS = -module $(MODULE_LDFLAGS)
lowpass_iir_1891_la_LIBADD = libswhsupport.la -lm
ls_filter_1908_la_LDFLAGS = -module $(MODULE_LDFLAGS)
ls_filter_1908_la_LIBADD = libswhsupport.la -lm
matrix_ms_st_1421_la_LDFLAGS = -module $(MODULE_LDFLAGS)
matrix_ms_st_1421_la_LIBADD = libswhsupport.la -lm
matrix_spatialiser_1422_la_LDFLAGS = -module $(MODULE_LDFLAGS)
matrix_spatialiser_1422_la_LIBADD = libswhsupport.la -lm
matrix_st_ms_1420_la_LDFLAGS = -module $(MODULE_LDFLAGS)
matrix_st_ms_1420_la_LIBADD = libswhsupport.la -lm
mod_delay_1419_la_LDFLAGS = -module $(MODULE_LDFLAGS)
mod_delay_1419_la_LIBADD = libswhsupport.la -lm
multivoice_chorus_1201_la_LDFLAGS = -module $(MODULE_LDFLAGS)
multivoice_chorus_1201_la_LIBADD = libswhsupport.la -lm
notch_iir_1894_la_LDFLAGS = -module $(MODULE_LDFLAGS)
notch_iir_1894_la_LIBADD = libswhsupport.la -lm
phasers_1217_la_LDFLAGS = -module $(MODULE_LDFLAGS)
phasers_1217_la_LIBADD = libswhsupport.la -lm
plate_1423_la_LDFLAGS = -module $(MODULE_LDFLAGS)
plate_1423_la_LIBADD = libswhsupport.la -lm
pointer_cast_1910_la_LDFLAGS = -module $(MODULE_LDFLAGS)
pointer_cast_1910_la_LIBADD = libswhsupport.la -lm
rate_shifter_1417_la_LDFLAGS = -module $(MODULE_LDFLAGS)
rate_shifter_1417_la_LIBADD = libswhsupport.la -lm
retro_flange_1208_la_LDFLAGS = -module $(MODULE_LDFLAGS)
retro_flange_1208_la_LIBADD = libswhsupport.la -lm
revdelay_1605_la_LDFLAGS = -module $(MODULE_LDFLAGS)
revdelay_1605_la_LIBADD = libswhsupport.la -lm
ringmod_1188_la_LDFLAGS = -module $(MODULE_LDFLAGS)
ringmod_1188_la_LIBADD = libswhsupport.la -lm
satan_maximiser_1408_la_LDFLAGS = -module $(MODULE_LDFLAGS)
satan_maximiser_1408_la_LIBADD = libswhsupport.la -lm
sc1_1425_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sc1_1425_la_LIBADD = libswhsupport.la -lm
sc2_1426_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sc2_1426_la_LIBADD = libswhsupport.la -lm
sc3_1427_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sc3_1427_la_LIBADD = libswhsupport.la -lm
sc4_1882_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sc4_1882_la_LIBADD = libswhsupport.la -lm
sc4m_1916_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sc4m_1916_la_LIBADD = libswhsupport.la -lm
se4_1883_la_LDFLAGS = -module $(MODULE_LDFLAGS)
se4_1883_la_LIBADD = libswhsupport.la -lm
shaper_1187_la_LDFLAGS = -module $(MODULE_LDFLAGS)
shaper_1187_la_LIBADD = libswhsupport.la -lm
sifter_1210_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sifter_1210_la_LIBADD = libswhsupport.la -lm
sin_cos_1881_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sin_cos_1881_la_LIBADD = libswhsupport.la -lm
single_para_1203_la_LDFLAGS = -module $(MODULE_LDFLAGS)
single_para_1203_la_LIBADD = libswhsupport.la -lm
sinus_wavewrapper_1198_la_LDFLAGS = -module $(MODULE_LDFLAGS)
sinus_wavewrapper_1198_la_LIBADD = libswhsupport.la -lm
smooth_decimate_1414_la_LDFLAGS = -module $(MODULE_LDFLAGS)
smooth_decimate_1414_la_LIBADD = libswhsupport.la -lm
split_1406_la_LDFLAGS = -module $(MODULE_LDFLAGS)
split_1406_la_LIBADD = libswhsupport.la -lm
step_muxer_1212_la_LDFLAGS = -module $(MODULE_LDFLAGS)
step_muxer_1212_la_LIBADD = libswhsupport.la -lm
surround_encoder_1401_la_LDFLAGS = -module $(MODULE_LDFLAGS)
surround_encoder_1401_la_LIBADD = libswhsupport.la -lm
svf_1214_la_LDFLAGS = -module $(MODULE_LDFLAGS)
svf_1214_la_LIBADD = libswhsupport.la -lm
tape_delay_1211_la_LDFLAGS = -module $(MODULE_LDFLAGS)
tape_delay_1211_la_LIBADD = libswhsupport.la -lm
transient_1206_la_LDFLAGS = -module $(MODULE_LDFLAGS)
transient_1206_la_LIBADD = libswhsupport.la -lm
triple_para_1204_la_LDFLAGS = -module $(MODULE_LDFLAGS)
triple_para_1204_la_LIBADD = libswhsupport.la -lm
valve_1209_la_LDFLAGS = -module $(MODULE_LDFLAGS)
valve_1209_la_LIBADD = libswhsupport.la -lm
valve_rect_1405_la_LDFLAGS = -module $(MODULE_LDFLAGS)
valve_rect_1405_la_LIBADD = libswhsupport.la -lm
vocoder_1337_la_LDFLAGS = -module $(MODULE_LDFLAGS)
vocoder_1337_la_LIBADD = libswhsupport.la -lm
vynil_1905_la_LDFLAGS = -module $(MODULE_LDFLAGS)
vynil_1905_la_LIBADD = libswhsupport.la -lm
wave_terrain_1412_la_LDFLAGS = -module $(MODULE_LDFLAGS)
wave_terrain_1412_la_LIBADD = libswhsupport.la -lm
xfade_1915_la_LDFLAGS = -module $(MODULE_LDFLAGS)
xfade_1915_la_LIBADD = libswhsupport.la -lm
zm1_1428_la_LDFLAGS = -module $(MODULE_LDFLAGS)
zm1_1428_la_LIBADD = libswhsupport.la -lm

# Four plugins are FFT based. FFTW is not something to make a Linux build
# depend on, but a macOS bundle carries its own copy, so they are built when
# it is there and quietly left out when it is not.
if BUNDLE_LADSPA_FFTW
AM_CPPFLAGS += -DFFTW3 $(FFTW3F_CFLAGS)
# Kept out of libswhsupport: a libtool convenience library hands every one of
# its objects to every module that links it, so pitchscale's FFTW references
# would land in all 98 while only four of them link FFTW. On macOS libtool
# links modules with -undefined dynamic_lookup, so that resolves at build time
# and fails at load time instead.
noinst_LTLIBRARIES += libswhfftw.la
libswhfftw_la_SOURCES = util/pitchscale.c
FFTW_PLUGIN_IDS = imp_1199 mbeq_1197 pitch_scale_1193 pitch_scale_1194
ladspa_LTLIBRARIES += imp_1199.la mbeq_1197.la pitch_scale_1193.la pitch_scale_1194.la

imp_1199_la_LDFLAGS = -module $(MODULE_LDFLAGS)
imp_1199_la_LIBADD = libswhsupport.la libswhfftw.la $(FFTW3F_LIBS) -lm
mbeq_1197_la_LDFLAGS = -module $(MODULE_LDFLAGS)
mbeq_1197_la_LIBADD = libswhsupport.la libswhfftw.la $(FFTW3F_LIBS) -lm
pitch_scale_1193_la_LDFLAGS = -module $(MODULE_LDFLAGS)
pitch_scale_1193_la_LIBADD = libswhsupport.la libswhfftw.la $(FFTW3F_LIBS) -lm
pitch_scale_1194_la_LDFLAGS = -module $(MODULE_LDFLAGS)
pitch_scale_1194_la_LIBADD = libswhsupport.la libswhfftw.la $(FFTW3F_LIBS) -lm
else
FFTW_PLUGIN_IDS =
endif

%.c: %.xml $(srcdir)/makestub.pl
	$(AM_V_GEN)$(PERL) $(srcdir)/makestub.pl $< > $@ || { rm -f $@; exit 1; }

BUILT_SOURCES = $(PLUGIN_IDS:%=%.c) $(FFTW_PLUGIN_IDS:%=%.c)
CLEANFILES = $(BUILT_SOURCES)

# Only the modules are of any use once installed, as the plugin directories do.
install-data-hook:
	-rm -f $(DESTDIR)$(ladspadir)/*.la

endif

EXTRA_DIST = \
	README.ssm.md COPYING AUTHORS README ChangeLog IDs NEWS TODO \
	ladspa-swh.dtd ladspa.css metadata \
	config.h ladspa-util.h \
	makestub.pl mkspec.pl bozosoity-checker.pl \
	$(ALL_PLUGIN_XML) \
	util/blo.c util/blo.h util/buffer.c util/buffer.h util/db.c util/db.h \
	util/iir.c util/iir.h util/pitchscale.c util/pitchscale.h \
	util/rms.c util/rms.h util/biquad.h util/ls_filter.h util/waveguide_nl.h \
	gverb/gverb.c gverb/gverb.h gverb/gverbdsp.c gverb/gverbdsp.h \
	gsm impulses
