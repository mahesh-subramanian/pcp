# aclocal.m4 generated automatically by aclocal 1.6.3 -*- Autoconf -*-

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

#
# Generic macro, sets up all of the global packaging variables.
# The following environment variables may be set to override defaults:
#   DEBUG OPTIMIZER MALLOCLIB PLATFORM DISTRIBUTION INSTALL_USER INSTALL_GROUP
#   BUILD_VERSION
#
AC_DEFUN([AC_PACKAGE_GLOBALS],
  [ pkg_name="$1"
    AC_SUBST(pkg_name)

    . ./VERSION
    pkg_version=${PKG_MAJOR}.${PKG_MINOR}.${PKG_REVISION}
    AC_SUBST(pkg_version)
    pkg_release=$PKG_BUILD
    test -z "$BUILD_VERSION" || pkg_release="$BUILD_VERSION"
    AC_SUBST(pkg_release)

    DEBUG=${DEBUG:-'-DDEBUG'}		dnl  -DNDEBUG
    debug_build="$DEBUG"
    AC_SUBST(debug_build)

    OPTIMIZER=${OPTIMIZER:-'-g -O2'}
    opt_build="$OPTIMIZER"
    AC_SUBST(opt_build)

    MALLOCLIB=${MALLOCLIB:-''}		dnl  /usr/lib/libefence.a
    malloc_lib="$MALLOCLIB"
    AC_SUBST(malloc_lib)

    pkg_user=`id -u -n`
    test -z "$INSTALL_USER" || pkg_user="$INSTALL_USER"
    AC_SUBST(pkg_user)

    pkg_group=`id -g -n`
    test -z "$INSTALL_GROUP" || pkg_group="$INSTALL_GROUP"
    AC_SUBST(pkg_group)

    pkg_distribution=`uname -s`
    test -z "$DISTRIBUTION" || pkg_distribution="$DISTRIBUTION"
    AC_SUBST(pkg_distribution)

    pkg_platform=`uname -s | tr 'A-Z' 'a-z' | sed -e 's/irix64/irix/'`
    test -z "$PLATFORM" || pkg_platform="$PLATFORM"
    AC_SUBST(pkg_platform)
  ])

#
# Check for specified utility (env var) - if unset, fail.
#
AC_DEFUN([AC_PACKAGE_NEED_UTILITY],
  [ if test -z "$2"; then
        echo
        echo FATAL ERROR: $3 does not seem to be installed.
        echo $1 cannot be built without a working $4 installation.
        exit 1
    fi
  ])

#
# Generic macro, sets up all of the global build variables.
# The following environment variables may be set to override defaults:
#  CC MAKE LIBTOOL TAR ZIP MAKEDEPEND AWK SED ECHO SORT
#  MSGFMT MSGMERGE XGETTEXT RPM
#
AC_DEFUN([AC_PACKAGE_UTILITIES],
  [ AC_PROG_CC
    cc="$CC"
    AC_SUBST(cc)
    AC_PACKAGE_NEED_UTILITY($1, "$cc", cc, [C compiler])

    if test -z "$MAKE"; then
        AC_PATH_PROG(MAKE, gmake,, /usr/bin:/usr/local/bin:/usr/freeware/bin)
    fi
    if test -z "$MAKE"; then
        AC_PATH_PROG(MAKE, make,, /usr/bin)
    fi
    make=$MAKE
    AC_SUBST(make)
    AC_PACKAGE_NEED_UTILITY($1, "$make", make, [GNU make])

    if test -z "$LIBTOOL"; then
	AC_PATH_PROG(LIBTOOL, glibtool,, /usr/bin)
    fi
    if test -z "$LIBTOOL"; then
	AC_PATH_PROG(LIBTOOL, libtool,, /usr/bin:/usr/local/bin:/usr/freeware/bin)
    fi
    libtool=$LIBTOOL
    AC_SUBST(libtool)
    AC_PACKAGE_NEED_UTILITY($1, "$libtool", libtool, [GNU libtool])

    if test -z "$TAR"; then
        AC_PATH_PROG(TAR, tar,, /usr/freeware/bin:/bin:/usr/local/bin:/usr/bin)
    fi
    tar=$TAR
    AC_SUBST(tar)
    if test -z "$ZIP"; then
        AC_PATH_PROG(ZIP, gzip,, /bin:/usr/bin:/usr/local/bin:/usr/freeware/bin)
    fi

    zip=$ZIP
    AC_SUBST(zip)

    bzip2=$BZIP2
    AC_SUBST(bzip2)

    if test -z "$MAKEDEPEND"; then
        AC_PATH_PROG(MAKEDEPEND, makedepend, /bin/true)
    fi
    makedepend=$MAKEDEPEND
    AC_SUBST(makedepend)

    if test -z "$AWK"; then
        AC_PATH_PROG(AWK, awk,, /bin:/usr/bin)
    fi
    awk=$AWK
    AC_SUBST(awk)

    if test -z "$SED"; then
        AC_PATH_PROG(SED, sed,, /bin:/usr/bin)
    fi
    sed=$SED
    AC_SUBST(sed)

    if test -z "$ECHO"; then
        AC_PATH_PROG(ECHO, echo,, /bin:/usr/bin)
    fi
    echo=$ECHO
    AC_SUBST(echo)

    if test -z "$SORT"; then
        AC_PATH_PROG(SORT, sort,, /bin:/usr/bin)
    fi
    sort=$SORT
    AC_SUBST(sort)

    dnl check if symbolic links are supported
    AC_PROG_LN_S

    if test "$enable_gettext" = yes; then
        if test -z "$MSGFMT"; then
                AC_PATH_PROG(MSGFMT, msgfmt,, /usr/bin:/usr/local/bin:/usr/freeware/bin)
        fi
        msgfmt=$MSGFMT
        AC_SUBST(msgfmt)
        AC_PACKAGE_NEED_UTILITY($1, "$msgfmt", msgfmt, gettext)

        if test -z "$MSGMERGE"; then
                AC_PATH_PROG(MSGMERGE, msgmerge,, /usr/bin:/usr/local/bin:/usr/freeware/bin)
        fi
        msgmerge=$MSGMERGE
        AC_SUBST(msgmerge)
        AC_PACKAGE_NEED_UTILITY($1, "$msgmerge", msgmerge, gettext)

        if test -z "$XGETTEXT"; then
                AC_PATH_PROG(XGETTEXT, xgettext,, /usr/bin:/usr/local/bin:/usr/freeware/bin)
        fi
        xgettext=$XGETTEXT
        AC_SUBST(xgettext)
        AC_PACKAGE_NEED_UTILITY($1, "$xgettext", xgettext, gettext)
    fi

    if test -z "$RPM"; then
        AC_PATH_PROG(RPM, rpm,, /bin:/usr/bin:/usr/freeware/bin)
    fi
    rpm=$RPM
    AC_SUBST(rpm)

    dnl .. and what version is rpm
    rpm_version=0
    test -n "$RPM" && test -x "$RPM" && rpm_version=`$RPM --version \
                        | awk '{print $NF}' | awk -F. '{V=1; print $V}'`
    AC_SUBST(rpm_version)
    dnl At some point in rpm 4.0, rpm can no longer build rpms, and
    dnl rpmbuild is needed (rpmbuild may go way back; not sure)
    dnl So, if rpm version >= 4.0, look for rpmbuild.  Otherwise build w/ rpm
    if test $rpm_version -ge 4; then
        AC_PATH_PROG(RPMBUILD, rpmbuild)
        rpmbuild=$RPMBUILD
    else
        rpmbuild=$RPM
    fi
    AC_SUBST(rpmbuild)

    dnl check if the dpkg program is available
    if test -z "$DPKG"
    then
	AC_PATH_PROG(DPKG, dpkg)
    fi
    dpkg=$DKPG
    AC_SUBST(dpkg)

    dnl Check for mac PackageMaker
    AC_MSG_CHECKING([for PackageMaker])
    if test -z "$PACKAGE_MAKER"
    then
	if test -x /Developer/Applications/PackageMaker.app/Contents/MacOS/PackageMaker       
	then # Darwin 6.x
	    package_maker=/Developer/Applications/PackageMaker.app/Contents/MacOS/PackageMaker
	    AC_MSG_RESULT([ yes (darwin 6.x)])
	elif test -x /Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
	then # Darwin 7.x
	    AC_MSG_RESULT([ yes (darwin 7.x)])
	    package_maker=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
	else
	    AC_MSG_RESULT([ no])
	fi
    else
	package_maker="$PACKAGE_MAKER"
    fi
    AC_SUBST(package_maker)

    dnl check if the hdiutil program is available
    test -z "$HDIUTIL" && AC_PATH_PROG(HDIUTIL, hdiutil)
    hdiutil=$HDIUTIL
    AC_SUBST(hdiutil)
  ])

AC_DEFUN([AC_PACKAGE_NEED_QT_QMAKE],
  [ if test -z "$QMAKE"; then
	AC_PATH_PROG(QMAKE, qmake,, /usr/bin)
    fi
    qmake=$QMAKE
    AC_SUBST(qmake)
    AC_PACKAGE_NEED_UTILITY($1, "$qmake", qmake, [Qt make])
  ])

AC_DEFUN([AC_PACKAGE_NEED_QT_UIC],
  [ if test -z "$UIC"; then
	AC_PATH_PROG(UIC, uic,, /usr/bin)
    fi
    uic=$UIC
    AC_SUBST(uic)
    AC_PACKAGE_NEED_UTILITY($1, "$uic", uic, [Qt User Interface Compiler])
  ])

AC_DEFUN([AC_PACKAGE_NEED_QT_MOC],
  [ if test -z "$MOC"; then
	AC_PATH_PROG(MOC, moc,, /usr/bin)
    fi
    moc=$MOC
    AC_SUBST(moc)
    AC_PACKAGE_NEED_UTILITY($1, "$uic", uic, [Qt Meta-Object Compiler])
  ])

#
# Check if we have a pcp/pmapi.h installed
#
AC_DEFUN([AC_PACKAGE_NEED_PMAPI_H],
  [ AC_CHECK_HEADERS(pcp/pmapi.h)
    if test $ac_cv_header_pcp_pmapi_h = no; then
	echo
	echo 'FATAL ERROR: could not find a valid <pcp/pmapi.h> header.'
	exit 1
    fi
  ])

#
# Check if we have a pcp/pmda.h installed
#
AC_DEFUN([AC_PACKAGE_NEED_PMDA_H],
  [ AC_CHECK_HEADERS([pcp/pmda.h], [], [],
[[#include <pcp/pmapi.h>
#include <pcp/impl.h>
]])
    if test $ac_cv_header_pcp_pmda_h = no; then
	echo
	echo 'FATAL ERROR: could not find a valid <pcp/pmda.h> header.'
	exit 1
    fi
  ])

#
# Check if we have the pmNewContext routine in libpcp
#
AC_DEFUN([AC_PACKAGE_NEED_LIBPCP],
  [ AC_CHECK_LIB(pcp, pmNewContext,, [
	echo
	echo 'FATAL ERROR: could not find a PCP library (libpcp).'
	exit 1
    ])
    libpcp=-lpcp
    AC_SUBST(libpcp)
  ])

#
# Check if we have the pmdaMain routine in libpcp_pmda
#
AC_DEFUN([AC_PACKAGE_NEED_LIBPCP_PMDA],
  [ AC_CHECK_LIB(pcp_pmda, pmdaMain,, [
	echo
	echo 'FATAL ERROR: could not find a PCP PMDA library (libpcp_pmda).'
	exit 1
    ])
    libpcp_pmda=-lpcp_pmda
    AC_SUBST(libpcp_pmda)
  ])

AC_DEFUN([AC_PACKAGE_NEED_PTHREAD_H],
  [ AC_CHECK_HEADERS(pthread.h)
    if test $ac_cv_header_pthread_h = no; then
	AC_CHECK_HEADERS(pthread.h,, [
	echo
	echo 'FATAL ERROR: could not find a valid pthread header.'
	exit 1])
    fi
  ])

AC_DEFUN([AC_PACKAGE_NEED_PTHREADMUTEXINIT],
  [ AC_CHECK_LIB(pthread, pthread_mutex_init,, [
	echo
	echo 'FATAL ERROR: could not find a valid pthread library.'
	exit 1
    ])
    libpthread=-lpthread
    AC_SUBST(libpthread)
  ])

# 
# Find format of installed man pages.
# Always gzipped on Debian, but not Redhat pre-7.0.
# We don't deal with bzip2'd man pages, which Mandrake uses,
# someone will send us a patch sometime hopefully. :-)
# 
AC_DEFUN([AC_MANUAL_FORMAT],
  [ have_zipped_manpages=false
    for d in ${prefix}/share/man ${prefix}/man ; do
        if test -f $d/man1/man.1.gz
        then
            have_zipped_manpages=true
            break
        fi
    done
    AC_SUBST(have_zipped_manpages)
  ])

