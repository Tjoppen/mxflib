#!/bin/sh

DIE=0
package=mxflib
srcfile=mxflib/mxflib.h

export AUTOCONF=autoconf
export AUTOHEADER=autoheader
export AUTOMAKE=automake
export ACLOCAL=aclocal
export LIBTOOLIZE=libtoolize
export LIBTOOL=libtool

autoconf_version_msg () {
    echo
    echo "You must have autoconf 2.50 or greater to bootstrap $package."
    echo "Get the latest version from ftp://ftp.gnu.org/gnu/autoconf/"
    DIE=1
}

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
    autoconf_version_msg
}

autoconf_major=`$AUTOCONF --version | head -n 1 | sed 's/^[^0-9]*//' | sed 's/\([0-9]*\).\([0-9]*\)\([a-z]*\)/\1/'`
autoconf_minor=`$AUTOCONF --version | head -n 1 | sed 's/^[^0-9]*//' | sed 's/\([0-9]*\).\([0-9]*\)\([a-z]*\)/\2/'`

if [ $autoconf_major -le 2 ]; then
	if [ $autoconf_major -lt 2 ]; then
		autoconf_version_msg
	elif [ $autoconf_minor -lt 50 ]; then
		autoconf_version_msg
	fi
fi

automake_version_msg () { 
    echo
    echo "You must have automake 1.5 or greater to bootstrap $package."
    echo "Get the latest version from ftp://ftp.gnu.org/gnu/automake/"
    DIE=1
}
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    automake_version_msg
}


automake_major=`$AUTOMAKE --version | head -n 1 | sed 's/^.*\([0-9][0-9]*\)\.\([0-9][0-9]*\)\(-[^-]*\)*/\1/'`
automake_minor=`$AUTOMAKE --version | head -n 1 | sed 's/^.*\([0-9][0-9]*\)\.\([0-9][0-9]*\)\(-[^-]*\)*/\2/'`

if [ $automake_major -le 1 ]; then
	if [ $automake_major -lt 1 ]; then
		automake_version_msg
	elif [ $automake_minor -lt 5 ]; then
	    automake_version_msg
	fi
fi

if test "$DIE" -eq 1; then
	exit 1
fi

test -f $srcfile || {
	echo "You must run this script in the top-level $package directory"
	exit 1
}

rm -f depcomp missing install-sh mkinstalldirs ltmain.sh
rm -f aclocal.m4 configure config.h.in Makefile.in config.guess config.sub
rm -rf autom4te.cache

set -x
$ACLOCAL 
$AUTOHEADER
$AUTOMAKE --foreign --copy --add-missing
$AUTOCONF
