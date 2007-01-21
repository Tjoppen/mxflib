# $Id: Makefile.am,v 1.5 2007/01/21 09:58:59 matt-beard Exp $
#

# mxfcrypt compilation is enabled with ./configure --enable-crypt
# OpenSSL provides the libcrypto library for AES routines.
if HAVE_OPENSSL

INCLUDES = -I$(top_builddir)

bin_PROGRAMS = mxfcrypt

mxfcrypt_SOURCES = mxfcrypt.cpp crypto_asdcp.cpp crypto_asdcp.h

LDADD = ../mxflib/libmxf.a $(UUIDLIB) -lcrypto

endif
