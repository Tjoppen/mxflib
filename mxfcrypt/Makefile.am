# $Id: Makefile.am,v 1.4 2004/12/18 20:19:36 matt-beard Exp $
#

# mxfcrypt compilation is enabled with ./configure --enable-crypt
# OpenSSL provides the libcrypto library for AES routines.
if HAVE_OPENSSL

INCLUDES = -I$(top_builddir)

bin_PROGRAMS = mxfcrypt

mxfcrypt_SOURCES = mxfcrypt.cpp crypto_asdcp.cpp

LDADD = ../mxflib/libmxf.a $(UUIDLIB) -lcrypto

endif
