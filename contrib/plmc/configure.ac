#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
# under the GNU Lesser General Public License Version 2.1, February 1999.
# The complete license can be accessed from the following location:
# http://opensource.org/licenses/lgpl-license.php
# See the Copying file included with the OpenSAF distribution for full
# licensing terms.
#
# Author(s): Wind River Systems
#

AC_PREREQ([2.59])
AC_INIT([plmc], [0.9], [opensaf-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR([.])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([INSTALL])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([$PACKAGE_NAME], [$PACKAGE_VERSION])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#
# Check if rpath is enabled
#
AC_MSG_CHECKING([whether to build with rpath enabled])
AC_ARG_ENABLE([rpath],
	[AC_HELP_STRING([--enable-rpath],
		[Patches libtool to not use rpath in the libraries produced.])],
	[],
	[enable_rpath=yes])
AC_MSG_RESULT([$enable_rpath])

AC_PROG_LIBTOOL
AC_CONFIG_COMMANDS([libtool-rpath-patch],
	[if test "$libtool_patch_use_rpath" = "no"; then
		sed < libtool > libtool-2 's/^hardcode_libdir_flag_spec.*$'/'hardcode_libdir_flag_spec=""/'
		mv libtool-2 libtool
		sed < libtool > libtool-2 's/^runpath_var=LD_RUN_PATH$'/'runpath_var=DIE_RPATH_DIE/'
		mv libtool-2 libtool
		chmod 755 libtool
	fi],
[libtool_patch_use_rpath=$enable_rpath])


#
# Many distros use different init.d directory depending on their LSB compliance
# support. Use this option to point to the init.d directory used by your distro.
# Default to /etc/init.d
#
AC_ARG_WITH([initscriptdir],
        [AS_HELP_STRING([--with-initscriptdir=@<:@DIR@:>@],
                [use DIR as the directory containing the init.d scripts.
		 [default=/etc/init.d]])],
        [
        if test -d $withval; then
                with_initscriptdir=$withval
        else
                AC_ERROR([$withval is an invalid path])
        fi
        ],
        [with_initscriptdir=/etc/init.d])

AC_SUBST([initscriptdir], ["$with_initscriptdir"])


#
# Unconditionaly drop previx from sysconfdir
# and localstatedir
#
if test "$sysconfdir" = '${prefix}/etc' ; then
        sysconfdir="/etc"
fi

AC_SUBST([sysconfdir])

if test "$localstatedir" = '${prefix}/var' ; then
        localstatedir="/var"
fi

AC_SUBST([localstatedir])


#
# Check for configure deps
#
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

AC_CHECK_FUNCS([ \
        bzero \
        inet_ntoa \
        memset \
        select \
        socket \
        strerror \
        strstr])

AC_CHECK_HEADERS([ \
        arpa/inet.h \
        fcntl.h \
        limits.h \
        netdb.h \
        netinet/in.h \
        sys/file.h \
        sys/ioctl.h \
        sys/socket.h \
        syslog.h])

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_HEADER_STDBOOL
AC_PROG_CXX
AC_TYPE_PID_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_INT16_T
AC_TYPE_UINT32_T
AC_TYPE_INT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T


#
# List the output Makefiles
#
AC_CONFIG_FILES([ \
        Makefile \
        config/Makefile \
        include/Makefile \
        lib/Makefile \
        lib/utils/Makefile \
        plmcd/Makefile \
        scripts/Makefile \
        scripts/plmcboot \
        scripts/plmcd])

AC_OUTPUT
