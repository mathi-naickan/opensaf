#      -*- OpenSAF  -*-
#
# (C) Copyright 2008 The OpenSAF Foundation
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

AC_PREREQ([2.61])
AC_INIT([opensaf], [4.2.3], [opensaf-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR([.])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([INSTALL])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([$PACKAGE_NAME], [$PACKAGE_VERSION])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_USE_SYSTEM_EXTENSIONS

# (major version) = (first number) - (third number)
# (minor version) = (third number)
# (patch version) = (second number)
OPENSAF_LIB_VERSION=0:0:0
AC_SUBST([OPENSAF_LIB_VERSION])

# FIXME: Until the m4 macro gets cleaned for DSO symbol tests and not executable
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "yes" = "yes")
#m4_include([m4/linker-script.m4])
#gl_LINKER_SCRIPT

OPENSAF_RELEASE=$PACKAGE_VERSION
OPENSAF_BUGREPORT=$PACKAGE_BUGREPORT
AC_SUBST([OPENSAF_RELEASE])
AC_SUBST([OPENSAF_BUGREPORT])
AC_SUBST([ac_configure_args])


#
# Check if rpath is enabled
#
AC_MSG_CHECKING([whether to build with rpath enabled])
AC_ARG_ENABLE([rpath],
	[AC_HELP_STRING([--disable-rpath],
		[patches libtool to not use rpath in the libraries produced.
		[Default=no]])],
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
# The RPM spec file has a container for the package release number (default to
# one), it is possible to customize that value with this configure option.
#
AC_ARG_WITH([rpm-release],
        [AS_HELP_STRING([--with-rpm-release=@<:@ARG@:>@],
                [set the RPM release value to be current timestamp (ARG=timestamp);
                set the RPM release value to be the global revision (ARG=global-rev);
                set the RPM release value to be the local revision (ARG=local-rev);
                set the RPM release value to be <val> (ARG=<val>, [default=1])])],
        [
        if test "$withval" = timestamp; then
                with_rpm_release=`date +%Y%m%d%H%M`
        elif test "$withval" = global-rev; then
                with_rpm_release=`hg id -i`
        elif test "$withval" = local-rev; then
                with_rpm_release=c`hg id -n`
        else
                with_rpm_release=$withval
        fi
        ],
        [with_rpm_release=1])

AC_SUBST([OPENSAF_RPM_RELEASE], ["$with_rpm_release"])


#
# OpenSAF supports linking with various SAHPI interface version, by default it
# will autodetect the version by looking at the SaHpi.h used at compilation,
# when cross-compiling this is not possible, it's possible to provide the
# version manually
#
AC_ARG_WITH([hpi-interface],
        [AS_HELP_STRING([--with-hpi-interface=@<:@ARG@:>@],
                [autodetect the SAHPI_INTERFACE_VERSION (ARG=check [default]);
                set the SAHPI_INTERFACE_VERSION to A.01.01 (ARG=A01);
                set the SAHPI_INTERFACE_VERSION to B.01.01 (ARG=B01);
                set the SAHPI_INTERFACE_VERSION to B.02.01 (ARG=B02);
                set the SAHPI_INTERFACE_VERSION to B.03.01 (ARG=B03)])],
        [
        if test "$withval" = A01; then
                with_hpi_interface=check
        elif test "$withval" = A01; then
                with_hpi_interface=A01
                AC_DEFINE([HAVE_HPI_A01], 1)
        elif test "$withval" = B01; then
                with_hpi_interface=B01
                AC_DEFINE([HAVE_HPI_B01], 1)
        elif test "$withval" = B02; then
                with_hpi_interface=B02
                AC_DEFINE([HAVE_HPI_B02], 1)
        elif test "$withval" = B03; then
                with_hpi_interface=B03
                AC_DEFINE([HAVE_HPI_B03], 1)
        else
                AC_ERROR([Invalid SAHPI_INTERFACE_VERSION specified])
        fi
        ],
        [with_hpi_interface=check])

AC_SUBST([WITH_HPI_INTERFACE], ["$with_hpi_interface"])


#
# Some build systems support their own 'make rpm' rule, offer the possibility to
# disable the one provided with OpenSAF
#
AC_MSG_CHECKING([whether to enable the make rpm target])
AC_ARG_ENABLE([rpm-target],
        [AS_HELP_STRING([--disable-rpm-target],
                [disable support for the "make rpm" target [default=no]])],
        [],
        [enable_rpm_target=yes])
AC_MSG_RESULT([$enable_rpm_target])

AM_CONDITIONAL([ENABLE_RPM_TARGET], [test "$enable_rpm_target" = yes])

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
# Enable/disable the Python AIS bindings
#
AC_MSG_CHECKING([whether to enable the Python AIS bindings])
AC_ARG_ENABLE([python],
        [AS_HELP_STRING([--enable-python],
                [enable the Python AIS bindings [default=no]])],
        [],
        [enable_python=no])
AC_MSG_RESULT([$enable_python])

AM_CONDITIONAL([ENABLE_PYTHON], [test "$enable_python" = yes])
AC_SUBST([PYTHON_ENABLED], ["$enable_python"])

#
# If the AM4J API library isn't in the standard CLASSPATH, it can be configured
# with a parameter instead
AC_ARG_WITH([am4j-api-jar],
        [AS_HELP_STRING([--with-am4j-api-jar=@<:@FILE@:>@],
                [use FILE as the AM4J API JAR])],
        [
        if test -f $withval; then
                with_am4j_api_jar=$withval
        else
                AC_ERROR([$withval is an invalid filename])
        fi
        ],
        [with_am4j_api_jar=no])

#
# Enable/disable the Java AIS interface mapping components
#
m4_include([m4/java.m4])

AC_MSG_CHECKING([whether to enable the Java AIS interface mapping])
AC_ARG_ENABLE([java],
        [AS_HELP_STRING([--enable-java],
                [enable the Java AIS interface mapping [default=no]])],
        [],
        [enable_java=no])
AC_MSG_RESULT([$enable_java])

AM_CONDITIONAL([ENABLE_JAVA], [test "$enable_java" = yes])
AC_SUBST([JAVA_ENABLED], ["$enable_java"])

#
# Enable/disable the AM4J agent
#
AC_MSG_CHECKING([whether to enable the AM4J agent])
AC_ARG_ENABLE([am4j],
        [AS_HELP_STRING([--enable-am4j],
                [enable the AM4J agent [default=no]])],
        [],
        [enable_am4j=no])

AC_MSG_RESULT([$enable_am4j])
AM_CONDITIONAL([ENABLE_AM4J], [test "$enable_am4j" = yes])
AC_SUBST([AM4J_ENABLED], ["$enable_am4j"])

#
# Enable/disable TIPC transport
#
AC_MSG_CHECKING([whether to use TIPC as the transport])
AC_ARG_ENABLE([tipc],
        [AS_HELP_STRING([--disable-tipc],
                [disable building the TIPC [default=no]])],
        [],
        [enable_tipc=yes])
AC_MSG_RESULT([$enable_tipc])
 
if test "$enable_tipc" = yes; then
        AC_DEFINE([ENABLE_TIPC_TRANSPORT], 1, [Define if TIPC is enabled])
fi
 
AM_CONDITIONAL([ENABLE_TIPC_TRANSPORT], [test "$enable_tipc" = yes])
AC_SUBST([TIPC_TRANSPORT_ENABLED], ["$enable_tipc"])


#
# Enable/disable building the OpenSAF tests
#
AC_MSG_CHECKING([whether to enable building the OpenSAF tests])
AC_ARG_ENABLE([tests],
        [AS_HELP_STRING([--enable-tests],
                [enable building the OpenSAF testing programs [default=no]])],
        [],
        [enable_tests=no])
AC_MSG_RESULT([$enable_tests])

#
# If tests are enabled, then compile not just test programs but any code 
# anywhere bracketed by "HAVE_TESTS"
#
if test "$enable_tests" = yes; then
	AC_DEFINE([HAVE_TESTS], 1, [Define if we built OpenSAF tests])
fi

AM_CONDITIONAL([ENABLE_TESTS], [test "$enable_tests" = yes])
AC_SUBST([TESTS_ENABLED], ["$enable_tests"])

#
# Enable/disable IMM Persistent Backend
#
AC_MSG_CHECKING([whether to use the IMM persistent backend])
AC_ARG_ENABLE([imm-pbe],
	[AS_HELP_STRING([--enable-imm-pbe],
		[enable the IMM Persistent Backend Interface [default=yes]])],
	[],
	[enable_imm_pbe=no])
AC_MSG_RESULT([$enable_imm_pbe])

if test "$enable_imm_pbe" = yes; then
	AC_DEFINE([HAVE_IMM_PBE], 1, [Define if we have IMM_PBE enabled])
fi

AM_CONDITIONAL([ENABLE_IMM_PBE], [test "$enable_imm_pbe" = yes])
AC_SUBST([IMM_PBE_ENABLED], ["$enable_imm_pbe"])


#
# Enable/disable building some of the OpenSAF AIS Services
#
AC_MSG_CHECKING([whether to build the SAI-AIS-CKPT service])
AC_ARG_ENABLE([ais-ckpt],
	[AS_HELP_STRING([--disable-ais-ckpt],
		[disable building the SAI-AIS-CKPT service [default=no]])],
	[],
	[enable_ais_ckpt=yes])
AC_MSG_RESULT([$enable_ais_ckpt])

AM_CONDITIONAL([ENABLE_AIS_CKPT], [test "$enable_ais_ckpt" = yes])
AC_SUBST([AIS_CKPT_ENABLED], ["$enable_ais_ckpt"])


AC_MSG_CHECKING([whether to build the SAI-AIS-EVT service])
AC_ARG_ENABLE([ais-evt],
	[AS_HELP_STRING([--disable-ais-evt],
		[disable building the SAI-AIS-EVT service [default=no]])],
	[],
	[enable_ais_evt=yes])
AC_MSG_RESULT([$enable_ais_evt])

AM_CONDITIONAL([ENABLE_AIS_EVT], [test "$enable_ais_evt" = yes])
AC_SUBST([AIS_EVT_ENABLED], ["$enable_ais_evt"])


AC_MSG_CHECKING([whether to build the SAI-AIS-LCK service])
AC_ARG_ENABLE([ais-lck],
	[AS_HELP_STRING([--disable-ais-lck],
		[disable building the SAI-AIS-LCK service [default=no]])],
	[],
	[enable_ais_lck=yes])
AC_MSG_RESULT([$enable_ais_lck])

AM_CONDITIONAL([ENABLE_AIS_LCK], [test "$enable_ais_lck" = yes])
AC_SUBST([AIS_LCK_ENABLED], ["$enable_ais_lck"])


AC_MSG_CHECKING([whether to build the SAI-AIS-MSG service])
AC_ARG_ENABLE([ais-msg],
	[AS_HELP_STRING([--disable-ais-msg],
		[disable building the SAI-AIS-MSG service [default=no]])],
	[],
	[enable_ais_msg=yes])
AC_MSG_RESULT([$enable_ais_msg])

AM_CONDITIONAL([ENABLE_AIS_MSG], [test "$enable_ais_msg" = yes])
AC_SUBST([AIS_MSG_ENABLED], ["$enable_ais_msg"])


AC_MSG_CHECKING([whether to build the SAI-AIS-SMF service])
AC_ARG_ENABLE([ais-smf],
	[AS_HELP_STRING([--disable-ais-smf],
		[disable building the SAI-AIS-SMF service [default=no]])],
	[],
	[enable_ais_smf=yes])
AC_MSG_RESULT([$enable_ais_smf])

AM_CONDITIONAL([ENABLE_AIS_SMF], [test "$enable_ais_smf" = yes])
AC_SUBST([AIS_SMF_ENABLED], ["$enable_ais_smf"])

AC_MSG_CHECKING([whether to build the SAI-AIS-PLM service])
AC_ARG_ENABLE([ais-plm],
	[AS_HELP_STRING([--disable-ais-plm],
		[disable building the SAI-AIS-PLM service [default=yes]])],
	[],
	[enable_ais_plm=no])
AC_MSG_RESULT([$enable_ais_plm])

if test "$enable_ais_plm" = yes; then
	AC_DEFINE([ENABLE_AIS_PLM], 1, [Define if PLM is enabled])
fi

AM_CONDITIONAL([ENABLE_AIS_PLM], [test "$enable_ais_plm" = yes])
AC_SUBST([AIS_PLM_ENABLED], ["$enable_ais_plm"])


AC_MSG_CHECKING([whether to install the immxml tools])
AC_ARG_ENABLE([immxml],
	[AS_HELP_STRING([--disable-immxml],
		[disable installing the immxml tools [default=no]])],
	[],
	[enable_immxml=yes])
AC_MSG_RESULT([$enable_immxml])

AM_CONDITIONAL([ENABLE_IMMXML], [test "$enable_immxml" = yes])
AC_SUBST([IMMXML_ENABLED], ["$enable_immxml"])

#
# unless explicitly set, drop previx from sysconfdir
# and localstatedir
#
if test "$sysconfdir" = '${prefix}/etc' ; then
        if test "$prefix" = 'NONE' ; then
                sysconfdir="/etc"
        else
                sysconfdir="$prefix"/etc
        fi
fi

AC_SUBST([sysconfdir])

if test "$localstatedir" = '${prefix}/var' ; then
        if test "$prefix" = 'NONE' ; then
                localstatedir="/var"
        else
                localstatedir="$prefix"/var
        fi
fi

AC_SUBST([localstatedir])


#############################################
# Checks for programs.
#############################################
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_GCC_TRADITIONAL
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC

if test "$enable_python" = yes; then
        AM_PATH_PYTHON([2.5])
fi

if test "$enable_java" = yes; then
        AC_JAVA_WITH_JDK

        if test "$ac_java_jvm_dir" != ""; then
                AC_SUBST([JAVAC], [$ac_java_jvm_dir"bin/javac"])
                AC_SUBST([JAVADOC], [$ac_java_jvm_dir"bin/javadoc"])
        else
                AC_PROG_JAVAC
                AC_PROG_JAVADOC
        fi

        AC_JAVA_ANT
        AC_SUBST([JAVA_HOME], [$ac_java_jvm_dir])

        # Only handle AM4J if it's enabled explicitly or by configuring the AM4J API jar
        if test "$enable_am4j" = yes || test "$with_am4j_api_jar" != no ; then
                enable_am4j=yes
                AC_PROG_AM4J_API
                AC_SUBST([AM4J_API_JAR], ["$with_am4j_api_jar"])
                AM_CONDITIONAL([ENABLE_AM4J], [test "$enable_am4j" = yes])
                AC_SUBST([AM4J_ENABLED], ["$enable_am4j"])
        fi
fi

if test "$enable_java" = no; then
        if test "$with_am4j_api_jar" != no || test "$enable_am4j" != no; then
                AC_ERROR([Can't build AM4J support without enabling Java])
        fi
fi

#############################################
# MDS ARCH TYPE: MDS advises message encode/decode
# optimization to its clients if the sender and receiver
# are of "compatible-type". For this optimization to work, 
# the sender's platform and receiver's platform should be 
# similar (for e.g. both PowerPC) and use an libncs_core built 
# with the same value for "mds_arch". An "mds_arch" value of 0 
# is the default value and stands for a platform which is NOT
# compatible with any platform. 
#############################################

AC_ARG_VAR([mds_arch], [The arch-type input to MDS - valid values 1-7])

if test "$mds_arch" = ""; then
        mdsarchtype=0
else
        mdsarchtype=$mds_arch
fi

AC_SUBST([MDS_WORD_ARCH_FLAGS], [" -DMDS_ARCH_TYPE=$mdsarchtype"])

#############################################
# Checks for libraries.
#############################################
PKG_CHECK_MODULES([XML2], [libxml-2.0])

if test "$enable_ais_plm" = yes; then
    PKG_CHECK_MODULES([HPI], [openhpi])

    if test "$with_hpi_interface" = check; then
       if test "$cross_compiling" = no; then
          # Test for the SAHPI_INTERFACE_VERSION
          AC_RUN_IFELSE(
             [AC_LANG_PROGRAM(
                [#include <openhpi/SaHpi.h>],
                [if (SAHPI_INTERFACE_VERSION != 0x010101) return 1;]
             )],
             AC_DEFINE(
                [HAVE_HPI_A01], 1, [SAHPI_INTERFACE_VERSION == A01]
             )
          )
          AC_RUN_IFELSE(
             [AC_LANG_PROGRAM(
                [#include <openhpi/SaHpi.h>],
                [if (SAHPI_INTERFACE_VERSION != 0x020101) return 1;]
             )],
             AC_DEFINE(
                [HAVE_HPI_B01], 1, [SAHPI_INTERFACE_VERSION == B01]
             )
          )
          AC_RUN_IFELSE(
             [AC_LANG_PROGRAM(
                [#include <openhpi/SaHpi.h>],
                [if (SAHPI_INTERFACE_VERSION != 0x020201) return 1;]
             )],
             AC_DEFINE(
                [HAVE_HPI_B02], 1, [SAHPI_INTERFACE_VERSION == B02]
             )
          )
          AC_RUN_IFELSE(
             [AC_LANG_PROGRAM(
                [#include <openhpi/SaHpi.h>],
                [if (SAHPI_INTERFACE_VERSION != 0x020301) return 1;]
             )],
             AC_DEFINE(
                [HAVE_HPI_B03], 1, [SAHPI_INTERFACE_VERSION == B03]
             )
          )
       else
         AC_ERROR([Autodecting HPI interface version isn't supported while
         cross-compiling, force --with-hpi-version])
       fi
    fi
fi

if test "$enable_imm_pbe" = yes; then
	PKG_CHECK_MODULES([SQLITE3], [sqlite3])
fi

#############################################
# Checks for header files.
#############################################
AC_HEADER_DIRENT
AC_HEADER_MAJOR
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

AC_CHECK_HEADERS([ \
        arpa/inet.h \
        fcntl.h \
        limits.h \
        malloc.h \
        netdb.h \
        netinet/in.h \
        stddef.h \
        stdlib.h \
        string.h \
        sys/file.h \
        sys/ioctl.h \
        sys/param.h \
        sys/socket.h \
        sys/time.h \
        syslog.h \
        termios.h \
        unistd.h \
        utmp.h])


#############################################
# Checks for typedefs, structures, 
# and compiler characteristics.
#############################################
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_STRUCT_TM
AC_TYPE_OFF_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_INT16_T
AC_TYPE_UINT32_T
AC_TYPE_INT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T


#############################################
# Checks for library functions.
#############################################
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([ \
        bzero \
        clock_gettime \
        dup2 \
        ftruncate \
        gethostname \
        gettimeofday \
        inet_ntoa \
        isascii \
        localtime_r \
        memmove \
        memset \
        mkfifo \
        munmap \
        pow \
        select \
        setenv \
        socket \
        strcasecmp \
        strchr \
        strdup \
        strerror \
        strndup \
        strpbrk \
        strrchr \
        strspn \
        strstr \
        strtol \
        strtoul \
        strtoull])


#############################################
# List the output Makefiles
#############################################
AC_CONFIG_FILES([
        Makefile
        opensaf.spec
        Doxyfile
        java/Makefile
        java/ais_api/Makefile
        java/ais_api_impl/Makefile
        java/ais_api_impl_native/Makefile
        java/amf_agent/Makefile
        python/Makefile
        python/pyosaf/Makefile
        osaf/Makefile
        osaf/libs/Makefile
        osaf/libs/agents/Makefile
        osaf/libs/agents/infrastructure/Makefile
        osaf/libs/agents/infrastructure/rda/Makefile
        osaf/libs/agents/saf/Makefile
        osaf/libs/agents/saf/ava/Makefile
        osaf/libs/agents/saf/ava/include/Makefile
        osaf/libs/agents/saf/imma/Makefile
        osaf/libs/agents/saf/plma/Makefile
        osaf/libs/agents/saf/lga/Makefile
        osaf/libs/agents/saf/ntfa/Makefile
        osaf/libs/agents/saf/clma/Makefile
        osaf/libs/agents/saf/cpa/Makefile
        osaf/libs/agents/saf/eda/Makefile
        osaf/libs/agents/saf/gla/Makefile
        osaf/libs/agents/saf/mqa/Makefile
        osaf/libs/agents/saf/smfa/Makefile
        osaf/libs/core/Makefile
        osaf/libs/core/include/Makefile
        osaf/libs/core/common/Makefile
        osaf/libs/core/common/include/Makefile
        osaf/libs/core/leap/Makefile
        osaf/libs/core/leap/include/Makefile
        osaf/libs/core/mbcsv/Makefile
        osaf/libs/core/mbcsv/include/Makefile
        osaf/libs/core/mds/Makefile
        osaf/libs/core/mds/include/Makefile
        osaf/libs/common/Makefile
        osaf/libs/common/avsv/Makefile
        osaf/libs/common/avsv/include/Makefile
        osaf/libs/common/cpsv/Makefile
        osaf/libs/common/cpsv/include/Makefile
        osaf/libs/common/edsv/Makefile
        osaf/libs/common/edsv/include/Makefile
        osaf/libs/common/glsv/Makefile
        osaf/libs/common/glsv/include/Makefile
        osaf/libs/common/immsv/Makefile
        osaf/libs/common/immsv/include/Makefile
        osaf/libs/common/plmsv/Makefile
        osaf/libs/common/plmsv/include/Makefile
        osaf/libs/common/plmsv/plmc/Makefile
        osaf/libs/common/plmsv/plmc/include/Makefile
        osaf/libs/common/logsv/Makefile
        osaf/libs/common/logsv/include/Makefile
        osaf/libs/common/mqsv/Makefile
        osaf/libs/common/mqsv/include/Makefile
        osaf/libs/common/ntfsv/Makefile
        osaf/libs/common/ntfsv/include/Makefile
        osaf/libs/common/smfsv/Makefile
        osaf/libs/common/smfsv/include/Makefile
        osaf/libs/common/clmsv/Makefile
        osaf/libs/common/clmsv/include/Makefile
        osaf/libs/saf/Makefile
        osaf/libs/saf/include/Makefile
        osaf/libs/saf/libSaAmf/Makefile
        osaf/libs/saf/libSaAmf/pkgconfig/Makefile
        osaf/libs/saf/libSaAmf/pkgconfig/opensaf-amf.pc
        osaf/libs/saf/libSaImm/Makefile
        osaf/libs/saf/libSaImm/pkgconfig/Makefile
        osaf/libs/saf/libSaImm/pkgconfig/opensaf-imm.pc
        osaf/libs/saf/libSaPlm/Makefile
        osaf/libs/saf/libSaPlm/pkgconfig/Makefile
        osaf/libs/saf/libSaPlm/pkgconfig/opensaf-plm.pc	
        osaf/libs/saf/libSaLog/Makefile
        osaf/libs/saf/libSaLog/pkgconfig/Makefile
        osaf/libs/saf/libSaLog/pkgconfig/opensaf-log.pc
        osaf/libs/saf/libSaNtf/Makefile
        osaf/libs/saf/libSaNtf/pkgconfig/Makefile
        osaf/libs/saf/libSaNtf/pkgconfig/opensaf-ntf.pc
        osaf/libs/saf/libSaClm/Makefile
        osaf/libs/saf/libSaClm/pkgconfig/Makefile
        osaf/libs/saf/libSaClm/pkgconfig/opensaf-clm.pc
        osaf/libs/saf/libSaCkpt/Makefile
        osaf/libs/saf/libSaCkpt/pkgconfig/Makefile
        osaf/libs/saf/libSaCkpt/pkgconfig/opensaf-ckpt.pc
        osaf/libs/saf/libSaEvt/Makefile
        osaf/libs/saf/libSaEvt/pkgconfig/Makefile
        osaf/libs/saf/libSaEvt/pkgconfig/opensaf-evt.pc
        osaf/libs/saf/libSaLck/Makefile
        osaf/libs/saf/libSaLck/pkgconfig/Makefile
        osaf/libs/saf/libSaLck/pkgconfig/opensaf-lck.pc
        osaf/libs/saf/libSaMsg/Makefile
        osaf/libs/saf/libSaMsg/pkgconfig/Makefile
        osaf/libs/saf/libSaMsg/pkgconfig/opensaf-msg.pc
        osaf/libs/saf/libSaSmf/Makefile
        osaf/libs/saf/libSaSmf/pkgconfig/Makefile
        osaf/libs/saf/libSaSmf/pkgconfig/opensaf-smf.pc
        osaf/services/Makefile
        osaf/services/infrastructure/Makefile
        osaf/services/infrastructure/dtms/Makefile
        osaf/services/infrastructure/dtms/dtm/Makefile
        osaf/services/infrastructure/dtms/scripts/Makefile
        osaf/services/infrastructure/dtms/scripts/osaf-dtm
        osaf/services/infrastructure/dtms/scripts/dh_monitoring
        osaf/services/infrastructure/dtms/config/Makefile
        osaf/services/infrastructure/dtms/include/Makefile
        osaf/services/infrastructure/fm/Makefile
        osaf/services/infrastructure/fm/fms/Makefile
        osaf/services/infrastructure/fm/config/Makefile
        osaf/services/infrastructure/fm/fms/scripts/Makefile
        osaf/services/infrastructure/fm/fms/scripts/osaf-fmd
        osaf/services/infrastructure/nid/Makefile
        osaf/services/infrastructure/nid/config/Makefile
        osaf/services/infrastructure/nid/scripts/Makefile
        osaf/services/infrastructure/nid/scripts/nid_tipc
        osaf/services/infrastructure/nid/scripts/opensafd
        osaf/services/infrastructure/rde/Makefile
        osaf/services/infrastructure/rde/include/Makefile
        osaf/services/infrastructure/rde/scripts/Makefile
        osaf/services/infrastructure/rde/config/Makefile
        osaf/services/infrastructure/rde/scripts/osaf-rded
        osaf/services/saf/Makefile
        osaf/services/saf/avsv/Makefile
        osaf/services/saf/avsv/config/Makefile
        osaf/services/saf/avsv/avd/Makefile
        osaf/services/saf/avsv/avd/include/Makefile
        osaf/services/saf/avsv/avd/scripts/Makefile
        osaf/services/saf/avsv/avd/scripts/osaf-amfd
        osaf/services/saf/avsv/avnd/Makefile
        osaf/services/saf/avsv/avnd/include/Makefile
        osaf/services/saf/avsv/avnd/scripts/Makefile
        osaf/services/saf/avsv/avnd/scripts/osaf-amfnd
        osaf/services/saf/avsv/amfwdog/Makefile
        osaf/services/saf/avsv/amfwdog/scripts/Makefile
        osaf/services/saf/avsv/amfwdog/scripts/osaf-amfwd
        osaf/services/saf/immsv/Makefile
        osaf/services/saf/immsv/config/Makefile
        osaf/services/saf/immsv/immd/Makefile
        osaf/services/saf/immsv/immnd/Makefile
        osaf/services/saf/immsv/immd/scripts/Makefile
        osaf/services/saf/immsv/immd/scripts/osaf-immd
        osaf/services/saf/immsv/immnd/scripts/Makefile
        osaf/services/saf/immsv/immnd/scripts/osaf-immnd
        osaf/services/saf/plmsv/Makefile
        osaf/services/saf/plmsv/config/Makefile
        osaf/services/saf/plmsv/plms/Makefile
        osaf/services/saf/plmsv/plms/hpi_intf/Makefile
        osaf/services/saf/plmsv/plms/scripts/Makefile
        osaf/services/saf/plmsv/plms/scripts/osaf-plmd
        osaf/services/saf/logsv/Makefile
        osaf/services/saf/logsv/config/Makefile
        osaf/services/saf/logsv/lgs/Makefile
        osaf/services/saf/logsv/lgs/scripts/Makefile
        osaf/services/saf/logsv/lgs/scripts/osaf-logd
        osaf/services/saf/ntfsv/Makefile
        osaf/services/saf/ntfsv/config/Makefile
        osaf/services/saf/ntfsv/ntfs/Makefile
        osaf/services/saf/ntfsv/ntfs/scripts/Makefile
        osaf/services/saf/ntfsv/ntfs/scripts/osaf-ntfd
        osaf/services/saf/clmsv/Makefile
        osaf/services/saf/clmsv/config/Makefile
        osaf/services/saf/clmsv/nodeagent/Makefile
        osaf/services/saf/clmsv/clms/Makefile
        osaf/services/saf/clmsv/clms/scripts/Makefile
        osaf/services/saf/clmsv/clms/scripts/osaf-clmd
        osaf/services/saf/clmsv/nodeagent/scripts/Makefile
        osaf/services/saf/clmsv/nodeagent/scripts/osaf-noded
        osaf/services/saf/cpsv/Makefile
        osaf/services/saf/cpsv/config/Makefile
        osaf/services/saf/cpsv/cpd/Makefile
        osaf/services/saf/cpsv/cpnd/Makefile
        osaf/services/saf/cpsv/cpd/scripts/Makefile
        osaf/services/saf/cpsv/cpd/scripts/osaf-ckptd
        osaf/services/saf/cpsv/cpnd/scripts/Makefile
        osaf/services/saf/cpsv/cpnd/scripts/osaf-ckptnd
        osaf/services/saf/edsv/Makefile
        osaf/services/saf/edsv/config/Makefile
        osaf/services/saf/edsv/eds/Makefile
        osaf/services/saf/edsv/eds/scripts/Makefile
        osaf/services/saf/edsv/eds/scripts/osaf-evtd
        osaf/services/saf/glsv/Makefile
        osaf/services/saf/glsv/config/Makefile
        osaf/services/saf/glsv/gld/Makefile
        osaf/services/saf/glsv/glnd/Makefile
        osaf/services/saf/glsv/gld/scripts/Makefile
        osaf/services/saf/glsv/gld/scripts/osaf-lckd
        osaf/services/saf/glsv/glnd/scripts/Makefile
        osaf/services/saf/glsv/glnd/scripts/osaf-lcknd
        osaf/services/saf/mqsv/Makefile
        osaf/services/saf/mqsv/config/Makefile
        osaf/services/saf/mqsv/mqd/Makefile
        osaf/services/saf/mqsv/mqnd/Makefile
        osaf/services/saf/mqsv/mqd/scripts/Makefile
        osaf/services/saf/mqsv/mqd/scripts/osaf-msgd
        osaf/services/saf/mqsv/mqnd/scripts/Makefile
        osaf/services/saf/mqsv/mqnd/scripts/osaf-msgnd
        osaf/services/saf/smfsv/Makefile
        osaf/services/saf/smfsv/config/Makefile
        osaf/services/saf/smfsv/scripts/Makefile
        osaf/services/saf/smfsv/smfd/Makefile
        osaf/services/saf/smfsv/smfnd/Makefile
        osaf/services/saf/smfsv/smfd/scripts/Makefile
        osaf/services/saf/smfsv/smfd/scripts/osaf-smfd
        osaf/services/saf/smfsv/smfnd/scripts/Makefile
        osaf/services/saf/smfsv/smfnd/scripts/osaf-smfnd
        osaf/tools/Makefile
        osaf/tools/safamf/Makefile
        osaf/tools/safamf/amfpm/Makefile
        osaf/tools/safimm/Makefile
        osaf/tools/safimm/immadm/Makefile
        osaf/tools/safimm/immcfg/Makefile
        osaf/tools/safimm/immdump/Makefile
        osaf/tools/safimm/immfind/Makefile
        osaf/tools/safimm/immlist/Makefile
        osaf/tools/safimm/immload/Makefile
        osaf/tools/safimm/immxml/Makefile
        osaf/tools/safimm/include/Makefile
        osaf/tools/saflog/Makefile
        osaf/tools/saflog/include/Makefile
        osaf/tools/saflog/saflogger/Makefile
        osaf/tools/safntf/Makefile
        osaf/tools/safntf/include/Makefile
        osaf/tools/safntf/ntfsend/Makefile
        osaf/tools/safntf/ntfsubscribe/Makefile
        osaf/tools/safntf/ntfread/Makefile
        osaf/tools/rde/Makefile
        osaf/tools/scripts/Makefile
        pkgconfig/Makefile
        pkgconfig/opensaf.pc
        tests/immsv/Makefile
        tests/immsv/common/Makefile
        tests/immsv/implementer/Makefile
        tests/immsv/management/Makefile
        tests/logsv/Makefile
        tests/clmsv/Makefile
        tests/ntfsv/Makefile
        tests/plmsv/plms/Makefile
        tests/plmsv/Makefile
        tests/unit_test_fw/Makefile
        tests/unit_test_fw/inc/Makefile])

AC_CONFIG_SUBDIRS([contrib/plmc])

AC_OUTPUT

echo ""
echo "======================================================="
echo "OpenSAF Build Configuration Summary"
echo "======================================================="
echo " Version: $VERSION"
echo " Release: $OPENSAF_RPM_RELEASE"
echo ""
echo " Installation Directories:"
echo "${ECHO_T}  prefix: ${prefix}"
echo "${ECHO_T}  exec_prefix: ${exec_prefix}"
echo "${ECHO_T}  bindir: ${bindir}"
echo "${ECHO_T}  sbindir: ${sbindir}"
echo "${ECHO_T}  datadir: ${datadir}"
echo "${ECHO_T}  sysconfdir: ${sysconfdir}"
echo "${ECHO_T}  docdir: ${docdir}"
echo "${ECHO_T}  localstatedir: ${localstatedir}"
echo "${ECHO_T}  includedir: ${includedir}"
echo "${ECHO_T}  libdir: ${libdir}"
echo ""
echo " Compiling Options:"
echo "${ECHO_T}  C Compiler: ${CC}"
echo "${ECHO_T}  C++ Compiler: ${CXX}"
echo "${ECHO_T}  CFLAGS: ${CFLAGS} ${AM_CFLAGS}"
echo "${ECHO_T}  CXXFLAGS: ${CXXFLAGS} ${AM_CXXFLAGS}"
echo "${ECHO_T}  LDFLAGS: ${LDFLAGS}"
echo "${ECHO_T}  Enable RPATH: ${enable_rpath}"

if test "$srcdir" == "."; then
        echo "${ECHO_T}  Using VPATH: no"
else
        echo "${ECHO_T}  Using VPATH: yes"
fi

AM_CONDITIONAL([WITH_VPATH], [test "$srcdir" != "."])

echo ""
echo " Dependencies Options:"

if test "$enable_ais_plm" = yes; then
        echo "${ECHO_T}  HPI_CFLAGS: ${HPI_CFLAGS}"
        echo "${ECHO_T}  HPI_LIBS: ${HPI_LIBS}"
fi

if test "$enable_imm_pbe" = yes; then
        echo "${ECHO_T}  SQLITE3_CFLAGS: ${SQLITE3_CFLAGS}"
        echo "${ECHO_T}  SQLITE3_LIBS: ${SQLITE3_LIBS}"
fi

echo "${ECHO_T}  XML2_CFLAGS: ${XML2_CFLAGS}"
echo "${ECHO_T}  XML2_LIBS: ${XML2_LIBS}"
echo ""
echo " OpenSAF Options:"
echo "${ECHO_T}  Enable Python AIS Bindings: ${enable_python}"
echo "${ECHO_T}  Enable Java AIS Mapping: ${enable_java}"
echo "${ECHO_T}  Enable AM4J: ${enable_am4j}"
echo "${ECHO_T}  Enable PLM support: ${enable_ais_plm}"
echo "${ECHO_T}  Enable TIPC transport: ${enable_tipc}"
echo "======================================================="
echo ""
