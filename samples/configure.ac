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
AC_INIT([opensafdemo], [0.9], [opensaf-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR([.])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([INSTALL])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([$PACKAGE_NAME], [$PACKAGE_VERSION])

#
# Check for configure deps
#
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_CHECK_FUNCS([ \
        memset \
        strerror])

AC_CHECK_HEADERS([ \
        stdlib.h \
        string.h \
        syslog.h \
        unistd.h])

AC_FUNC_MALLOC

AC_TYPE_PID_T
AC_TYPE_SIZE_T

#
# Check for SAF libraries
#
PKG_CHECK_MODULES([SAF_AIS_AMF], [opensaf-amf])
PKG_CHECK_MODULES([SAF_AIS_CKPT], [opensaf-ckpt])
PKG_CHECK_MODULES([SAF_AIS_EVT], [opensaf-evt])
PKG_CHECK_MODULES([SAF_AIS_LCK], [opensaf-lck])
PKG_CHECK_MODULES([SAF_AIS_MSG], [opensaf-msg])
PKG_CHECK_MODULES([SAF_AIS_SMF], [opensaf-smf])

#
# List the output Makefiles
#
AC_CONFIG_FILES([ \
        Makefile \
        amf/Makefile \
        amf/non_sa_aware/Makefile \
        amf/sa_aware/Makefile \
        amf/wrapper/Makefile \
        cpsv/Makefile \
        edsv/Makefile \
	glsv/Makefile \
        mqsv/Makefile \
        smfsv/Makefile
        smfsv/campaigns/Makefile])

AC_OUTPUT
