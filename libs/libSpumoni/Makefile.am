AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(srcdir)/include
AM_CXXFLAGS = -std=c++03 -fPIC

noinst_LIBRARIES = libSpumoni.a
libSpumoni_a_SOURCES = \
	include/JSON.h src/JSON.cpp \
	include/Container.h src/Container.cpp src/Containers.cpp \
	include/Directory.h src/Directory.cpp \
	include/Folder.h src/Folder.cpp \
	include/Identity.h src/Identity.cpp \
	include/MemoryFolder.h src/MemoryFolder.cpp \
	include/Package.h src/Package.cpp src/PackageVersion.cpp \
	include/Project.h src/Project.cpp \
	include/SourcePart.h src/SourcePart.cpp \
	include/Tar.h src/Tar.cpp \
	include/Zip.h src/Zip.cpp

EXTRA_DIST = README.md
