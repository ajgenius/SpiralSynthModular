AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_srcdir)/libs/libSpumoni/include -I$(top_srcdir)/libs/libSpiralPanels/include
noinst_PROGRAMS = embed-manifest
embed_manifest_SOURCES = embed-manifest.cpp
embed_manifest_LDADD = $(top_builddir)/libs/libSpiralPanels/libSpiralPanels.la $(top_builddir)/libs/libSpumoni/libSpumoni.la $(ZLIB_LIBS)
