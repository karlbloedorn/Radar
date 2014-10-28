bin_PROGRAMS = CRDFTool
lib_LTLIBRARIES = libcrdf.la
CRDFTool_SOURCES = main.c


libcrdf_la_SOURCES = radarlib.c radarlib.h radarclientlib.c radarclientlib.h
libcrdf_la_LIBADD = -lbz2 -lz -lm -lpthread


CRDFTool_LDADD = libcrdf.la


pkginclude_HEADERS = radarlib.h
