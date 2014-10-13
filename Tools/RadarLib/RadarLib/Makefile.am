bin_PROGRAMS = CRDFTool
lib_LTLIBRARIES = libcrdf.la
CRDFTool_SOURCES = main.c


libcrdf_la_SOURCES = radarlib.c radarlib.h
libcrdf_la_LIBADD = -lbz2 -lz


CRDFTool_LDADD = libcrdf.la


pkginclude_HEADERS = radarlib.h
