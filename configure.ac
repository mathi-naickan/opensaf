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

AC_PREREQ([2.64])
AC_INIT([opensaf], [5.2.M0], [opensaf-users@lists.sourceforge.net], [opensaf], [http://opensaf.sourceforge.net/])
AC_CONFIG_AUX_DIR([.])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([opensaf.spec.in])
AC_CONFIG_HEADER([src/osaf/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1 subdir-objects])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_USE_SYSTEM_EXTENSIONS

# (major version) = (first number) - (third number)
# (minor version) = (third number)
# (patch version) = (second number)
OPENSAF_LIB_VERSION=0:0:0
AC_SUBST([OPENSAF_LIB_VERSION])

INTERNAL_VERSION_ID=0:000000000000
AC_SUBST([INTERNAL_VERSION_ID])

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

LT_PREREQ([2.2.6])
LT_INIT([disable-static pic-only])
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
                with_rpm_release=`git rev-parse HEAD`
        elif test "$withval" = local-rev; then
                with_rpm_release=`git rev-parse HEAD`
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
        [AS_HELP_STRING([--enable-tipc],
                [enable building the TIPC [default=no]])],
        [],
        [enable_tipc=no])
AC_MSG_RESULT([$enable_tipc])
 
if test "$enable_tipc" = yes; then
        AC_DEFINE([ENABLE_TIPC_TRANSPORT], 1, [Define if TIPC is enabled])
fi
 
AM_CONDITIONAL([ENABLE_TIPC_TRANSPORT], [test "$enable_tipc" = yes])
AC_SUBST([TIPC_TRANSPORT_ENABLED], ["$enable_tipc"])

#
# Enable/disable use of systemd
#
PKG_CHECK_MODULES([SYSTEMD], [systemd], [enable_systemd=yes], [enable_systemd=no])
 
if test "$enable_systemd" = yes; then
       PKG_CHECK_MODULES([SYSTEMD], [systemd])
       AC_DEFINE([ENABLE_SYSTEMD], 1, [Define if systemd is enabled])
       systemdunitdir=$(pkg-config --variable=systemdsystemunitdir systemd)
       systemdversion=$(pkg-config --modversion systemd)
fi
 
AM_CONDITIONAL([ENABLE_SYSTEMD], [test "$enable_systemd" = yes])
AC_SUBST([SYSTEMD_ENABLED], ["$enable_systemd"])
AC_SUBST([systemdsystemunitdir], ["$systemdunitdir"])
AC_SUBST([systemdmodversion], ["$systemdversion"])

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
		[enable the IMM Persistent Backend Interface [default=no]])],
	[],
	[enable_imm_pbe=no])
AC_MSG_RESULT([$enable_imm_pbe])

if test "$enable_imm_pbe" = yes; then
	AC_DEFINE([HAVE_IMM_PBE], 1, [Define if we have IMM_PBE enabled])
fi

AM_CONDITIONAL([ENABLE_IMM_PBE], [test "$enable_imm_pbe" = yes])
AC_SUBST([IMM_PBE_ENABLED], ["$enable_imm_pbe"])


#
# Enable/disable NTF Information Model Change Notifier (ntf-imcn)
#
AC_MSG_CHECKING([whether to use the Information Model Change Notifier])
AC_ARG_ENABLE([ntf-imcn],
	[AS_HELP_STRING([--enable-ntf-imcn],
		[enable the NTF Information Model Notification producer [default=no]])],
	[],
	[enable_ntf_imcn=no])
AC_MSG_RESULT([$enable_ntf_imcn])

if test "$enable_ntf_imcn" = yes; then
	AC_DEFINE([HAVE_NTFIMCN], 1, [Define if we have NTFIMCN enabled])
fi

AM_CONDITIONAL([ENABLE_NTFIMCN], [test "$enable_ntf_imcn" = yes])
AC_SUBST([NTFIMCN_ENABLED], ["$enable_ntf_imcn"])


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
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

if test "$enable_python" = yes; then
        AM_PATH_PYTHON([2.6])
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
# Checks for libraries.
#############################################
PKG_CHECK_MODULES([XML2], [libxml-2.0])

if test "$enable_ais_plm" = yes; then
    PKG_CHECK_MODULES([HPI], [openhpi])
    PKG_CHECK_MODULES([LIBVIRT], [libvirt])

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

if test -z "$OSAF_HARDEN_FLAGS"; then
	# _FORTIFY_SOURCE requires optimization, so only enable it in optimized
	# builds, i.e. when -O is present in both CFLAGS and CXXFLAGS.
	if echo "${CFLAGS}" | grep -q -- -O; then
		if echo "${CXXFLAGS}" | grep -q -- -O; then
			OSAF_HARDEN_FLAGS="-D_FORTIFY_SOURCE=2"
		fi
	fi
	# Also check for -O0 (which explicitly disables optimisation)
	if echo "${CFLAGS} ${CXXFLAGS}" | grep -q -- -O0; then
		OSAF_HARDEN_FLAGS=""
	fi
	OSAF_HARDEN_FLAGS="${OSAF_HARDEN_FLAGS} -fstack-protector --param ssp-buffer-size=4 -fPIE -pie -zrelro -znow"
fi
AC_SUBST(OSAF_HARDEN_FLAGS)

#############################################
# List the output Makefiles
#############################################
AC_CONFIG_FILES([
        Makefile
        opensaf.spec
        Doxyfile
	src/amf/amfd/osaf-amfd
	src/amf/amfnd/osaf-amfnd
	src/amf/amfwd/osaf-amfwd
	src/amf/saf/opensaf-amf.pc
	src/ckpt/ckptd/osaf-ckptd
	src/ckpt/ckptnd/osaf-ckptnd
	src/ckpt/saf/opensaf-ckpt.pc
	src/clm/clmd/osaf-clmd
	src/clm/clmnd/osaf-clmna
	src/clm/saf/opensaf-clm.pc
	src/dtm/dtmnd/osaf-dtm
	src/evt/evtd/osaf-evtd
	src/evt/saf/opensaf-evt.pc
	src/fm/fmd/osaf-fmd
	src/imm/immd/osaf-immd
	src/imm/immnd/osaf-immnd
	src/imm/saf/opensaf-imm.pc
	src/lck/lckd/osaf-lckd
	src/lck/lcknd/osaf-lcknd
	src/lck/saf/opensaf-lck.pc
	src/log/logd/osaf-logd
	src/log/saf/opensaf-log.pc
	src/msg/msgd/osaf-msgd
	src/msg/msgnd/osaf-msgnd
	src/msg/saf/opensaf-msg.pc
	src/nid/configure_tipc
	src/nid/opensafd
	src/nid/opensafd.service
	src/ntf/ntfd/osaf-ntfd
	src/ntf/saf/opensaf-ntf.pc
	src/plm/plmcd/plmcboot
	src/plm/plmcd/plmcboot.service
	src/plm/plmcd/plmcd
	src/plm/plmcd/plmcd.service
	src/plm/plmd/osaf-plmd
	src/plm/saf/opensaf-plm.pc
	src/rde/rded/osaf-rded
	src/smf/saf/opensaf-smf.pc
	src/smf/smfd/osaf-smfd
	src/smf/smfnd/osaf-smfnd
	src/dtm/transport/osaf-transport
        pkgconfig/opensaf.pc])

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
echo "${ECHO_T}  CPPFLAGS: ${CPPFLAGS} ${AM_CPPFLAGS}"
echo "${ECHO_T}  CFLAGS: ${CFLAGS} ${AM_CFLAGS}"
echo "${ECHO_T}  CXXFLAGS: ${CXXFLAGS} ${AM_CXXFLAGS}"
echo "${ECHO_T}  OSAF_HARDEN_FLAGS: ${OSAF_HARDEN_FLAGS}"
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
        echo "${ECHO_T}  LIBVIRT_CFLAGS: ${LIBVIRT_CFLAGS}"
        echo "${ECHO_T}  LIBVIRT_LIBS: ${LIBVIRT_LIBS}"
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
echo "${ECHO_T}  Enable systemd: ${enable_systemd}"
echo "======================================================="
echo ""
