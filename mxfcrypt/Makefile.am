# $Id: Makefile.am,v 1.3 2004/11/15 15:26:21 matt-beard Exp $
#

# mxfcrypt compilation is enabled with ./configure --enable-crypt
# OpenSSL provides the libcrypto library for AES routines.
if HAVE_OPENSSL

INCLUDES = -I$(top_builddir)

bin_PROGRAMS = mxfcrypt

mxfcrypt_SOURCES = mxfcrypt.cpp

LDADD = ../mxflib/libmxf.a $(UUIDLIB) -lcrypto

endif
