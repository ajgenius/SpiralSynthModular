#############################################################################
# Makefile for building SamplerPlugin.so
#############################################################################

####### Compiler, tools and options

CC	=	gcc
CXX	=	g++
CFLAGS	=       @CFLAGS@ @FLTK_CFLAGS@
CXXFLAGS=       @CXXFLAGS@ @FLTK_CXXFLAGS@
INCPATH	=	-I/usr/X11R6/include
LINK	=	g++ -shared
LFLAGS	=
LIBS	=   @FLTK_LIBS@
MOC     = moc
UIC	=

INSTALL	= @INSTALL@

###### Autoconf variables

prefix = @prefix@
exec_prefix = @exec_prefix@

bindir = @bindir@
sbindir = @sbindir@
libexecdir = @libexecdir@
datadir = @datadir@
sysconfdir = @sysconfdir@
sharedstatedir = @sharedstatedir@
localstatedir = @localstatedir@
libdir = @libdir@
infodir = @infodir@
mandir = @mandir@

####### Files

HEADERS =	SamplerPlugin.h \
		SamplerPluginGUI.h \
		../SpiralPlugin.h \
		../SpiralPluginGUI.h \
		../../ChannelHandler.h \
		../../Sample.h \
		../../RiffWav.h \
		../Widgets/Fl_Knob.H \
		../../../GUI/Widgets/SpiralGUI.H
SOURCES =	SamplerPlugin.C \
		SamplerPluginGUI.C \
		../SpiralPlugin.C \
		../SpiralPluginGUI.C \
		../../ChannelHandler.C \
		../../Sample.C \
		../../RiffWav.C \
		../Widgets/Fl_Knob.cxx \
		../../../GUI/Widgets/SpiralGUI.C
OBJECTS =	SamplerPlugin.o \
		SamplerPluginGUI.o \
		../SpiralPlugin.o \
		../SpiralPluginGUI.o \
		../../ChannelHandler.o \
		../../Sample.o \
		../../RiffWav.o \
		../Widgets/Fl_Knob.o \
		../../../GUI/Widgets/SpiralGUI.o
INTERFACES =
UICDECLS =
UICIMPLS =
SRCMOC	=
OBJMOC	=
DIST	=
TARGET	=	SamplerPlugin.so

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules


all: $(TARGET)

$(TARGET): $(UICDECLS) $(OBJECTS) $(OBJMOC)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(LIBS)

moc: $(SRCMOC)

clean:
	-rm -f $(OBJECTS) $(OBJMOC) $(SRCMOC) $(UICIMPLS) $(UICDECLS) $(TARGET)
	-rm -f *~ core

install:
	$(INSTALL) $(TARGET) $(libdir)/SpiralPlugins

####### Sub-libraries


###### Combined headers


####### Compile

../SpiralPlugin.o: ../SpiralPlugin.C \
		../SpiralPlugin.h \
		../../ChannelHandler.h \
		../../Sample.h

../SpiralPluginGUI.o: ../SpiralPluginGUI.C \
		../SpiralPluginGUI.h \
		../SpiralPlugin.h \
		../../ChannelHandler.h \
		../../../GUI/Widgets/SpiralGUI.H

../Widgets/Fl_Knob.o: ../Widgets/Fl_Knob.cxx \
		../Widgets/Fl_Knob.H

../../../GUI/Widgets/SpiralGUI.o: ../../../GUI/Widgets/SpiralGUI.C \
		../../../GUI/Widgets/SpiralGUI.H

../../Sample.o: ../../Sample.C \
		../../Sample.h

../../RiffWav.o: ../../RiffWav.C \
		../../RiffWav.h \
		../../Sample.h

SamplerPlugin.o: SamplerPlugin.C \
		SamplerPlugin.h \
		../SpiralPlugin.h \
		../../Sample.h \
		../../Sample.h \
		SamplerPluginGUI.h \
		../SpiralPluginGUI.h \
		SpiralIcon.xpm \
		../../RiffWav.h \
		../../NoteTable.h

SamplerPluginGUI.o: SamplerPluginGUI.C \
		SamplerPluginGUI.h \
		SamplerPlugin.h \
		../SpiralPlugin.h \
		../../Sample.h \
		../../Sample.h \
		../SpiralPluginGUI.h \
		../../../GUI/Widgets/SpiralGUI.H \
		../Widgets/Fl_Knob.H

../../ChannelHandler.o: ../../ChannelHandler.C \
		../../ChannelHandler.h
