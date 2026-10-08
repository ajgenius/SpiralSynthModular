AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(srcdir) $(YAJL_CFLAGS)

if HAVE_YAJL
check_PROGRAMS = json-reader-test
TESTS = $(check_PROGRAMS)
json_reader_test_SOURCES = JSONParser.h JSONParser.cpp tests/reader.cpp
json_reader_test_LDADD = $(YAJL_LIBS)
endif
