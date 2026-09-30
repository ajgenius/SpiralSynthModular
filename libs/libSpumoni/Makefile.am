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
	include/Store.h include/StoreSHA256.h src/Store.cpp src/StoreSHA256.cpp \
	include/Tar.h src/Tar.cpp \
	include/Zip.h src/Zip.cpp

check_PROGRAMS = json-test store-test
TESTS = $(check_PROGRAMS)
json_test_SOURCES = tests/json.cpp
json_test_LDADD = libSpumoni.a
store_test_SOURCES = tests/store.cpp
store_test_LDADD = libSpumoni.a

EXTRA_DIST = README.md
