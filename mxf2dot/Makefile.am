# $Id: Makefile.am,v 1.1 2007/01/21 10:44:21 matt-beard Exp $
#

INCLUDES = -I$(top_builddir)

bin_PROGRAMS = mxf2dot

mxfcrypt_SOURCES = mxf2dot.cpp DotFile.cpp DotFile.h

LDADD = ../mxflib/libmxf.a $(UUIDLIB)

