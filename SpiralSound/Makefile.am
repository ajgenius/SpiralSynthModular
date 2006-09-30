lib_LTLIBRARIES = libSpiralCommon.la
libSpiralCommon_la_SOURCES =				\
				ChannelHandler.h	\
				ChannelHandler.C	\
				Midi.h			\
				Midi.C			\
				NoteTable.h		\
				NoteTable.C		\
				RiffWav.h		\
				RiffWav.C		\
				Sample.h		\
				Sample.C

libSpiralCommon_la_LDFLAGS = -shared -no-undefined
libSpiralCommon_la_LIBADD = $(SFLIBS) $(AMIDILIBS) $(LRDFLIBS) -lm -ldl -lpthread
