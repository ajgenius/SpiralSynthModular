lib_LTLIBRARIES = libSpiralCommon.la
libSpiralCommon_la_SOURCES = \
    ChannelHandler.h Midi.h NoteTable.h Sample.h RiffWav.h RiffWav.C

# The legacy headers forward to libspiralcore; only RiffWav remains here.
libSpiralCommon_la_LDFLAGS = -shared -no-undefined -version-info 1:0:0
libSpiralCommon_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la $(SFLIBS) $(LRDFLIBS) -lm -ldl -lpthread
