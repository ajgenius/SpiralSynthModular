dnl Library Revisions

m4_define([lt_current], [0])
m4_define([lt_revision], [1])
m4_define([lt_age], [0])
          
dnl Process this file with autoconf to produce a configure script.
AC_INIT([SpiralSynthModular], [0.3.1])
AC_CONFIG_SRCDIR([SpiralSynthModular.C])

AM_INIT_AUTOMAKE

dnl Automake (can't use AC_CONFIG_HEADER - get complaints!)
AM_CONFIG_HEADER(config.h)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_C_BIGENDIAN
AC_PROG_INSTALL

AM_PROG_LIBTOOL

AC_PATH_TOOL([PKG_CONFIG], [pkg-config], [no])

dnl ---- OutputPlugin audio backends (in libspiralcore) ----
dnl Blocking device-paced I/O: PortAudio Pa_WriteStream, ALSA writei, OSS
dnl write().  Not the original callback PortAudioClient, not PipeWire/ESD.

AC_ARG_ENABLE([portaudio],
    [AS_HELP_STRING([--disable-portaudio], [Do not build PortAudio output support])],
    [enable_output_portaudio="$enableval"], [enable_output_portaudio="yes"])
have_output_portaudio="no"
portaudio_message="manually disabled"
PORTAUDIO_CFLAGS=""
PORTAUDIO_LIBS=""
if test "x$enable_output_portaudio" = "xyes"; then
    if test "$PKG_CONFIG" != no && $PKG_CONFIG --exists portaudio-2.0; then
        PORTAUDIO_CFLAGS=`$PKG_CONFIG --cflags portaudio-2.0`
        PORTAUDIO_LIBS=`$PKG_CONFIG --libs portaudio-2.0`
        have_output_portaudio=yes
        portaudio_message=found
    fi
    if test "x$have_output_portaudio" != "xyes"; then
        AC_CHECK_HEADER([portaudio.h],
            [save_LIBS="$LIBS"
             AC_CHECK_LIB([portaudio], [Pa_WriteStream],
                [have_output_portaudio="yes"; PORTAUDIO_LIBS="-lportaudio"; portaudio_message="found"],
                [portaudio_message="libportaudio not found"])
             LIBS="$save_LIBS"],
            [portaudio_message="portaudio.h not found"])
    fi
fi
if test "x$have_output_portaudio" = "xyes"; then
    AC_DEFINE([HAVE_OUTPUT_PORTAUDIO], [1], [Build PortAudio output support in libspiralcore])
fi
AC_SUBST([PORTAUDIO_CFLAGS])
AC_SUBST([PORTAUDIO_LIBS])
AM_CONDITIONAL([HAVE_OUTPUT_PORTAUDIO], [test "x$have_output_portaudio" = "xyes"])

AC_ARG_ENABLE([alsa-output],
    [AS_HELP_STRING([--disable-alsa-output], [Do not build ALSA PCM output support])],
    [enable_output_alsa="$enableval"], [enable_output_alsa="yes"])
have_output_alsa="no"
alsa_output_message="manually disabled"
ALSA_CFLAGS=""
ALSA_LIBS=""
if test "x$enable_output_alsa" = "xyes"; then
    if test "$PKG_CONFIG" != no && $PKG_CONFIG --exists alsa; then
        ALSA_CFLAGS=`$PKG_CONFIG --cflags alsa`
        ALSA_LIBS=`$PKG_CONFIG --libs alsa`
        have_output_alsa=yes
        alsa_output_message=found
    fi
    if test "x$have_output_alsa" != "xyes"; then
        AC_CHECK_HEADER([alsa/asoundlib.h],
            [save_LIBS="$LIBS"
             AC_CHECK_LIB([asound], [snd_pcm_open],
                [have_output_alsa="yes"; ALSA_LIBS="-lasound"; alsa_output_message="found"],
                [alsa_output_message="libasound not found"])
             LIBS="$save_LIBS"],
            [alsa_output_message="alsa/asoundlib.h not found"])
    fi
fi
if test "x$have_output_alsa" = "xyes"; then
    AC_DEFINE([HAVE_OUTPUT_ALSA], [1], [Build ALSA PCM output support in libspiralcore])
fi
AC_SUBST([ALSA_CFLAGS])
AC_SUBST([ALSA_LIBS])
AM_CONDITIONAL([HAVE_OUTPUT_ALSA], [test "x$have_output_alsa" = "xyes"])

AC_ARG_ENABLE([oss-output],
    [AS_HELP_STRING([--disable-oss-output], [Do not build legacy OSS output support])],
    [enable_output_oss="$enableval"], [enable_output_oss="yes"])
have_output_oss="no"
oss_output_message="manually disabled"
if test "x$enable_output_oss" = "xyes"; then
    AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h])
    if test "x$ac_cv_header_sys_soundcard_h" = "xyes" -o \
            "x$ac_cv_header_soundcard_h" = "xyes" -o \
            "x$ac_cv_header_machine_soundcard_h" = "xyes"; then
        have_output_oss="yes"
        oss_output_message="found"
    else
        oss_output_message="no OSS soundcard header found"
    fi
fi
if test "x$have_output_oss" = "xyes"; then
    AC_DEFINE([HAVE_OUTPUT_OSS], [1], [Build OSS output support in libspiralcore])
fi
AM_CONDITIONAL([HAVE_OUTPUT_OSS], [test "x$have_output_oss" = "xyes"])

default_output_audio_client="none"
if test "x$have_output_portaudio" = "xyes"; then
    default_output_audio_client="portaudio"
elif test "x$have_output_alsa" = "xyes"; then
    default_output_audio_client="alsa"
elif test "x$have_output_oss" = "xyes"; then
    default_output_audio_client="oss"
fi
AC_DEFINE_UNQUOTED([DEFAULT_OUTPUT_AUDIO_CLIENT], ["$default_output_audio_client"],
    [Default OutputPlugin audio client])
if test "x$default_output_audio_client" = "xnone"; then
    AC_MSG_WARN([No OutputPlugin audio backend found (PortAudio, ALSA, or OSS)])
fi


dnl Checks for header files.
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

LT_VERSION_INFO="lt_current:lt_revision:lt_age"
AC_SUBST(LT_VERSION_INFO)

dnl Check plugin directory
AC_ARG_WITH(plugindir,
  [  --with-plugindir=dir    give the plugin directory],
  plugindir="$withval",
  plugindir="/usr/lib/SpiralPlugins")
AC_SUBST(PLUGINDIR, $plugindir)

dnl Check whether to enable debugging
AC_ARG_ENABLE(debug,  [  --enable-debug          enable debugging ],
        ac_arg_debug="Y", ac_arg_debug="N")

if test $ac_arg_debug = "Y" ; then
	CFLAGS="-pipe -Wall -O0 -ggdb "
	CXXFLAGS="-pipe -Wall -O0 -ggdb "
else
	CFLAGS="-pipe -Wall -O3 -ffast-math -DNO_DEBUG -Wno-unused "
	CXXFLAGS="-pipe -Wall -O3 -ffast-math -DNO_DEBUG -Wno-unused "
fi

dnl build plugin list - plugins that do not require an optional component
dnl (i.e. Jack, and LADSPA)

PLUGINLIST="AmpPlugin AnotherFilterPlugin BeatMatchPlugin ControllerPlugin \
CounterPlugin DelayPlugin DiskWriterPlugin DistributorPlugin EchoPlugin \
EnvFollowerPlugin EnvelopePlugin FilterPlugin FlipflopPlugin FormantFilterPlugin \
KeyboardPlugin LFOPlugin LogicPlugin MasherPlugin MatrixPlugin \
MeterPlugin MidiPlugin MixSwitchPlugin MixerPlugin MoogFilterPlugin NoisePlugin \
NoteSnapPlugin OperatorPlugin OscillatorPlugin OutputPlugin PoshSamplerPlugin \
RingModPlugin SVFilterPlugin SampleHoldPlugin ScopePlugin SeqSelectorPlugin \
SmoothPlugin SpiralLoopPlugin SplitSwitchPlugin SplitterPlugin StereoMixerPlugin \
StreamPlugin SwitchPlugin TranslatePlugin TransposePlugin TrigPlugin WaveShaperPlugin \
WaveTablePlugin XFadePlugin"

dnl Check whether jack is present or has been disabled
jack_message=""
AC_ARG_ENABLE(
    jack,
    [  --disable-jack          Do not build JackPlugin],
    [build_jack="n" ; jack_message="manually disabled"],
    [build_jack="y"]
)
if test "$build_jack" = "y"; then
   AC_CHECK_HEADER(jack/jack.h, [build_jack="y"], [build_jack="n" ; jack_message="No jack/jack.h found"])
fi
if test "$build_jack" = "y"; then
   AC_CHECK_LIB(jack, jack_client_new, [build_jack="y"], [build_jack="n" ; jack_message="No libjack found"])
fi
if test "$build_jack" = "y" ; then
   AC_CHECK_LIB(jack, jack_client_open,
      [AC_DEFINE([HAVE_JACK_CLIENT_OPEN], [1], [Define if JACK provides jack_client_open])])
   PLUGINLIST="JackPlugin $PLUGINLIST"
fi

dnl Check whether LADSPA is present or has been disabled
ladspa_message=""
AC_ARG_ENABLE(
    ladspa,
    [  --disable-ladspa        Do not build LADSPAPlugin],
    [build_ladspa="n" ; ladspa_message="manually disabled"],
    [build_ladspa="y"]
)
if test "$build_ladspa" = "y"; then
   AC_CHECK_HEADER(ladspa.h, [build_ladspa="y"], [build_ladspa="n" ; ladspa_message="No ladspa.h found"])
fi
if test "$build_ladspa" = "y" ; then
   PLUGINLIST="LADSPAPlugin $PLUGINLIST"
fi

dnl Check whether liblrdf is present or has been disabled
lrdf_message=""
AC_ARG_ENABLE(
    lrdf,
    [  --disable-lrdf          Disable liblrdf for LADSPA Plugin],
    [use_liblrdf="n" ; lrdf_message="manually disabled"],
    [use_liblrdf="y"]
)
if test "$build_ladspa" = "n" ; then
   use_liblrdf="n"
   lrdf_message="LADSPAPlugin not used"
fi
if test "$use_liblrdf" = "y"; then
   AC_CHECK_HEADER(lrdf.h, [use_liblrdf="y"], [use_liblrdf="n" ; lrdf_message="No lrdf.h found"])
fi
if test "$use_liblrdf" = "y"; then
   AC_CHECK_LIB(lrdf, lrdf_init, [use_liblrdf="y"], [use_liblrdf="n" ; lrdf_message="No liblrdf found"])
fi
if test "$use_liblrdf" = "y"; then
   AC_DEFINE(HAVE_LIBLRDF, , [Enable liblrdf support for LADSPA Plugin])
   LRDFLIBS="-llrdf"
   AC_SUBST(LRDFLIBS)
fi

dnl Check whether alsa-midi is present or has been disabled
amidi_message=""
AC_ARG_ENABLE(
    alsa_midi,
    [  --disable-alsa-midi     Disable alsa midi (use OSS)],
    [use_alsa_midi="n"; amidi_message="manually disabled"],
    [use_alsa_midi="y"]
)
if test "$use_alsa_midi" = "y"; then
   AC_CHECK_HEADER(alsa/asoundlib.h, [use_alsa_midi="y"], [use_alsa_midi="n" ; amidi_message="No alsa/asoundlib.h found"])
fi
if test "$use_alsa_midi" = "y"; then
   AC_CHECK_LIB(asound, snd_seq_open, [use_alsa_midi="y"], [use_alsa_midi="n" ; amidi_message="No libasound found"])
fi
if test "$use_alsa_midi" = "y"; then
   AC_DEFINE(USE_ALSA_MIDI, , [Use ALSA MIDI not OSS])
   AMIDILIBS="-lasound"
   AC_SUBST(AMIDILIBS)
else
   AC_DEFINE(USE_OSS_MIDI, , [Use OSS MIDI not ALSA])
fi

dnl Check whether libsndfile is present or has been disabled
sndfile_message=""
AC_ARG_ENABLE(
    sndfile,
    [  --disable-sndfile       Disable libsndfile for waveform loading],
    [use_sndfile="n" ; sndfile_message="manually disabled"],
    [use_sndfile="y"]
)
if test "$use_sndfile" = "y"; then
   AC_CHECK_HEADER(sndfile.h, [use_sndfile="y"], [use_sndfile="n" ; sndfile_message="No sndfile.h found"])
fi
if test "$use_sndfile" = "y"; then
   AC_CHECK_LIB(sndfile, sf_open, [use_sndfile="y"], [use_sndfile="n" ; sndfile_message="No libsndfile found"])
fi
if test "$use_sndfile" = "y"; then
   AC_DEFINE(USE_LIBSNDFILE, , [Enable libsndfile for waveform loading])
   SFLIBS="-lsndfile"
   AC_SUBST(SFLIBS)
fi

dnl Checks for library functions.

AC_CHECK_HEADERS(math.h)
AC_CHECK_LIB(m, sin)

dnl AC_CHECK_HEADERS(X11/Xlib.h)
dnl AC_CHECK_LIB(X11, XDisplayName)
dnl AC_CHECK_LIB(Xext, main)

AC_PATH_PROG(FLTK_CONFIG, fltk-config, no)
if test "$FLTK_CONFIG" = "no"; then
  AC_PATH_XTRA
  LIBGL=""
  CPPFLAGS="$CPPFLAGS $X_CFLAGS"
  AC_CHECK_HEADERS(GL/gl.h)
  if test $ac_cv_header_GL_gl_h = "yes" ; then
    AC_CHECK_LIB(GL, glBegin, LIBGL="-lGL")
  fi
  AC_CHECK_HEADERS(FL/Fl.h)
  if test $ac_cv_header_FL_Fl_h = "no" ; then
    echo "*** FLTK headers missing."
    exit 1
  fi
  AC_CHECK_LIB(fltk, main)
  FLTK_CXXFLAGS="$X_CFLAGS"
  FLTK_CFLAGS="$FLTK_CXXFLAGS"
  FLTK_LIBS="$X_LIBS -lfltk $LIBGL"
else
  FLTK_CXXFLAGS=`$FLTK_CONFIG $fltk_config_args --cxxflags`
  FLTK_CFLAGS=`$FLTK_CONFIG $fltk_config_args --cxxflags`
  FLTK_LIBS=`$FLTK_CONFIG $fltk_config_args --ldflags`
dnl This caused problems for debian users,
dnl It would be nice to find a more robust way of doing it.
dnl  if test "${FLTK_LIBS/*-rpath*/yes}" != "yes"; then
dnl    echo "*** FLTK needs to be configured with --enable-shared"
dnl   exit 1
dnl  fi
fi
AC_SUBST(FLTK_CXXFLAGS)
AC_SUBST(FLTK_CFLAGS)
AC_SUBST(FLTK_LIBS)

AC_CHECK_HEADERS(dlfcn.h)
AC_CHECK_LIB(dl, dlopen)

dnl Do we need png?????
dnl AC_CHECK_HEADERS(png.h)
dnl AC_CHECK_LIB(png, png_get_copyright)

dnl This is normally done with libtool but since we aren't using libtool
dnl yet to it here.

dnl FIXME: forced to add -fPIC here..
CFLAGS="$CFLAGS -fPIC"
CXXFLAGS="$CFLAGS -fPIC"

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
GUI_PLUGINLIST=""
for plugin in $PLUGINLIST; do
    case "$plugin" in
        SampleHoldPlugin) ;;
        *) GUI_PLUGINLIST="$GUI_PLUGINLIST $plugin" ;;
    esac
done
AC_SUBST(GUI_PLUGINLIST)
AC_SUBST(PLUGINLIST)

dnl Modify the output files.
AC_OUTPUT(
SpiralSynthPluginLocation.h
Makefile
GUI/Makefile
GUI/Widgets/Makefile
libspiralcore/Makefile
Plugins/Makefile
Plugins/DSP/Makefile
Plugins/GUI/Makefile
Plugins/GUI/AmpPlugin/Makefile
Plugins/GUI/AnotherFilterPlugin/Makefile
Plugins/GUI/BeatMatchPlugin/Makefile
Plugins/GUI/ControllerPlugin/Makefile
Plugins/GUI/CounterPlugin/Makefile
Plugins/GUI/DelayPlugin/Makefile
Plugins/GUI/DiskWriterPlugin/Makefile
Plugins/GUI/DistributorPlugin/Makefile
Plugins/GUI/EchoPlugin/Makefile
Plugins/GUI/EnvFollowerPlugin/Makefile
Plugins/GUI/EnvelopePlugin/Makefile
Plugins/GUI/FilterPlugin/Makefile
Plugins/GUI/FlipflopPlugin/Makefile
Plugins/GUI/FormantFilterPlugin/Makefile
Plugins/GUI/JackPlugin/Makefile
Plugins/GUI/KeyboardPlugin/Makefile
Plugins/GUI/LADSPAPlugin/Makefile
Plugins/GUI/LFOPlugin/Makefile
Plugins/GUI/LogicPlugin/Makefile
Plugins/GUI/MasherPlugin/Makefile
Plugins/GUI/MatrixPlugin/Makefile
Plugins/GUI/MeterPlugin/Makefile
Plugins/GUI/MidiPlugin/Makefile
Plugins/GUI/MixSwitchPlugin/Makefile
Plugins/GUI/MixerPlugin/Makefile
Plugins/GUI/MoogFilterPlugin/Makefile
Plugins/GUI/NoisePlugin/Makefile
Plugins/GUI/NoteSnapPlugin/Makefile
Plugins/GUI/OperatorPlugin/Makefile
Plugins/GUI/OscillatorPlugin/Makefile
Plugins/GUI/OutputPlugin/Makefile
Plugins/GUI/PoshSamplerPlugin/Makefile
Plugins/GUI/RingModPlugin/Makefile
Plugins/GUI/SVFilterPlugin/Makefile
Plugins/GUI/ScopePlugin/Makefile
Plugins/GUI/SeqSelectorPlugin/Makefile
Plugins/GUI/SmoothPlugin/Makefile
Plugins/GUI/SpiralLoopPlugin/Makefile
Plugins/GUI/SplitSwitchPlugin/Makefile
Plugins/GUI/SplitterPlugin/Makefile
Plugins/GUI/StereoMixerPlugin/Makefile
Plugins/GUI/StreamPlugin/Makefile
Plugins/GUI/SwitchPlugin/Makefile
Plugins/GUI/TranslatePlugin/Makefile
Plugins/GUI/TransposePlugin/Makefile
Plugins/GUI/TrigPlugin/Makefile
Plugins/GUI/WaveShaperPlugin/Makefile
Plugins/GUI/WaveTablePlugin/Makefile
Plugins/GUI/XFadePlugin/Makefile
Plugins/DSP/AmpPlugin/Makefile
Plugins/DSP/AnotherFilterPlugin/Makefile
Plugins/DSP/BeatMatchPlugin/Makefile
Plugins/DSP/ControllerPlugin/Makefile
Plugins/DSP/CounterPlugin/Makefile
Plugins/DSP/DelayPlugin/Makefile
Plugins/DSP/DiskWriterPlugin/Makefile
Plugins/DSP/DistributorPlugin/Makefile
Plugins/DSP/EchoPlugin/Makefile
Plugins/DSP/EnvFollowerPlugin/Makefile
Plugins/DSP/EnvelopePlugin/Makefile
Plugins/DSP/FilterPlugin/Makefile
Plugins/DSP/FlipflopPlugin/Makefile
Plugins/DSP/FormantFilterPlugin/Makefile
Plugins/DSP/JackPlugin/Makefile
Plugins/DSP/KeyboardPlugin/Makefile
Plugins/DSP/LADSPAPlugin/Makefile
Plugins/DSP/LFOPlugin/Makefile
Plugins/DSP/LogicPlugin/Makefile
Plugins/DSP/MasherPlugin/Makefile
Plugins/DSP/MatrixPlugin/Makefile
Plugins/DSP/MeterPlugin/Makefile
Plugins/DSP/MidiPlugin/Makefile
Plugins/DSP/MixSwitchPlugin/Makefile
Plugins/DSP/MixerPlugin/Makefile
Plugins/DSP/MoogFilterPlugin/Makefile
Plugins/Broken/MousePlugin/Makefile
Plugins/DSP/NoisePlugin/Makefile
Plugins/DSP/NoteSnapPlugin/Makefile
Plugins/DSP/OperatorPlugin/Makefile
Plugins/DSP/OscillatorPlugin/Makefile
Plugins/DSP/OutputPlugin/Makefile
Plugins/DSP/PoshSamplerPlugin/Makefile
Plugins/DSP/RingModPlugin/Makefile
Plugins/DSP/SVFilterPlugin/Makefile
Plugins/DSP/SampleHoldPlugin/Makefile
Plugins/DSP/ScopePlugin/Makefile
Plugins/DSP/SeqSelectorPlugin/Makefile
Plugins/DSP/SmoothPlugin/Makefile
Plugins/DSP/SpiralLoopPlugin/Makefile
Plugins/DSP/SplitSwitchPlugin/Makefile
Plugins/DSP/SplitterPlugin/Makefile
Plugins/DSP/StereoMixerPlugin/Makefile
Plugins/DSP/StreamPlugin/Makefile
Plugins/DSP/SwitchPlugin/Makefile
Plugins/DSP/TranslatePlugin/Makefile
Plugins/DSP/TransposePlugin/Makefile
Plugins/DSP/TrigPlugin/Makefile
Plugins/DSP/WaveShaperPlugin/Makefile
Plugins/DSP/WaveTablePlugin/Makefile
Plugins/DSP/XFadePlugin/Makefile
)

echo -e "\nConfig options:"
echo -n "libsndfile support for wave loading - "
if test "$use_sndfile" = "y"; then
   echo "Enabled"
else
   echo "Disabled - $sndfile_message"
fi
echo -n "        ALSA support for MidiPlugin - "
if test "$use_alsa_midi" = "y"; then
   echo "Enabled"
else
   echo "Disabled - $amidi_message - Using OSS instead"
fi
echo -n "      Jack support (via JackPlugin) - "
if test "$build_jack" = "y"; then
   echo "Enabled"
else
   echo "Disabled - $jack_message"
fi
echo -n "  LADSPA support (via LADSPAPlugin) - "
if test "$build_ladspa" = "y"; then
   echo "Enabled"
else
   echo "Disabled - $ladspa_message"
fi
echo -n "   liblrdf support for LADSPAPlugin - "
if test "$use_liblrdf" = "y"; then
   echo "Enabled"
else
   echo "Disabled - $lrdf_message"
fi
echo ""
