#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(mxflib, beta-1.0.0)
AC_CONFIG_SRCDIR([mxflib/mxffile.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(mxflib, beta-1.0.0)

# Create a .zip package in addition to .tar.gz when running "make dist"
AM_INIT_AUTOMAKE(dist-zip)

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB
if test "$CXX" = "g++"; then
	gcc_version=`$CXX -dumpversion`
	case $gcc_version in
		2.95*)
			AC_MSG_ERROR($CXX version $gcc_version is known to mis-compile MXFlib - use g++ 3.x or newer)
			;;
	esac
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h sys/timeb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor gettimeofday memmove memset strcasecmp strerror strtoul])

# Checks for libraries needed by MXFlib.
UUIDLIB=""
AC_CHECK_HEADER([uuid/uuid.h],
		[AC_CHECK_LIB(uuid, uuid_generate, 
				[AC_DEFINE([HAVE_UUID_GENERATE], 1, [ Have uuid_generate])
				have_libuuid=yes UUIDLIB="-luuid" ], [have_libuuid=no])])
# Under _WIN32 AC_CHECK_LIB is not reliable (you get false negatives)
AC_CHECK_HEADER([windows.h], [UUIDLIB="-lole32"])

# Check for optional features requested by --enable-feature
have_openssl=no
AC_ARG_ENABLE(crypt,
[  --enable-crypt          enable building of mxfcrypt],
[	if test "$enableval" = "yes"; then
		dnl openssl provides the AES routines used by mxfcrypt
		have_openssl=no
		AC_CHECK_HEADER([openssl/aes.h],
			[AC_CHECK_LIB(crypto, AES_set_encrypt_key,
				[ have_openssl=yes] )]
			)
		if test x"$have_openssl" = "xno" ; then
			AC_MSG_WARN([*** OpenSSL not available - mxfcrypt will not be built ***])
		fi
	fi
])
AM_CONDITIONAL(HAVE_OPENSSL, test x"$have_openssl" = "xyes")

# Check for doxygen (needed for building docs)
AC_ARG_WITH(doxygen,
	[  --without-doxygen       don't autogenerate doxygen documentation],
	[if test -n ${with_doxygen}; then HAVE_DOXYGEN="false"; fi],
	[
		AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
		if test $HAVE_DOXYGEN = "false"; then
	             AC_MSG_WARN([*** doxygen not found, docs will not be built])
		fi
		AC_CHECK_PROG(HAVE_DOT, dot, YES, NO)
		if test $HAVE_DOT = "NO"; then
	             AC_MSG_WARN([*** dot not found (part of Graphviz), class graphs will not be built])
		fi
	])
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)

DEFAULT_INCLUDES=""
CXXFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DDEFAULT_DICT_PATH=\\\"\$(datadir)/mxflib\\\" -g -Wall"
AC_SUBST(UUIDLIB)
AC_SUBST(DEFAULT_INCLUDES)
AC_SUBST(CXXFLAGS)

AC_CONFIG_TESTDIR([tests], [tests:mxfdump:mxfwrap:mxfsplit])
AC_CONFIG_FILES([tests/Makefile tests/atlocal])

AC_CONFIG_FILES([Makefile
                 docs/Makefile
                 docs/doxyfile.cfg
                 mxfdump/Makefile
                 mxflib/Makefile
                 mxfsplit/Makefile
                 mxfwrap/Makefile
                 mxfcrypt/Makefile])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
