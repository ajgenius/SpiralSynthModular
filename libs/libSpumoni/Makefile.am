AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(srcdir)/include
AM_CXXFLAGS = -std=c++03 -fPIC

# A libtool convenience library, not a plain archive: libspiralcore lists it in
# LIBADD and libtool then links every object in, in the place a library belongs.
noinst_LTLIBRARIES = libSpumoni.la
libSpumoni_la_SOURCES = \
	include/JSON.h include/detail/JSONReader.h src/JSON.cpp \
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

check_PROGRAMS = json-test store-test package-test
TESTS = $(check_PROGRAMS)
json_test_SOURCES = tests/json.cpp
json_test_LDADD = libSpumoni.la
store_test_SOURCES = tests/store.cpp
store_test_LDADD = libSpumoni.la
package_test_SOURCES = tests/package.cpp
package_test_LDADD = libSpumoni.la $(ZLIB_LIBS)

EXTRA_DIST = README.md

EXTRA_DIST += tests/json-conformance.h
