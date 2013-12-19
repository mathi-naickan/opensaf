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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = javah.xml

lib_LTLIBRARIES = libjava_ais_api_native.la

noinst_HEADERS = \
	j_ais_amf_compRegistry.h \
	j_ais_amf_csiManager.h \
	j_ais_amf_errReporting.h \
	j_ais_amf.h \
	j_ais_amf_healthcheck.h \
	j_ais_amf_libHandle.h \
	j_ais_amf_pgManager.h \
	j_ais_amf_pm.h \
	j_ais_clm.h \
	j_ais_clm_libHandle.h \
	j_ais_clm_manager.h \
	j_ais.h \
	j_ais_libHandle.h \
	jni_ais_amf.h \
	jni_ais_clm.h \
	jni_ais.h \
	j_utils.h \
	j_utilsPrint.h \
	tracer.h

libjava_ais_api_native_la_CPPFLAGS = \
	-I$(top_srcdir)/osaf/libs/saf/include \
	-I$(top_srcdir)/osaf/libs/core/common/include \
	-I $(JAVA_HOME)/include \
	-I $(JAVA_HOME)/include/linux \
	-DSA_CLM_B01

libjava_ais_api_native_la_SOURCES = \
	$(top_srcdir)/java/ais_api_impl_native/j_ais.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_compRegistry.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_csiManager.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_errReporting.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_healthcheck.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_libHandle.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_pgManager.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_amf_pm.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_clm.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_clm_libHandle.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_clm_manager.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_libHandle.c \
	$(top_srcdir)/java/ais_api_impl_native/j_ais_socketUtil.c \
	$(top_srcdir)/java/ais_api_impl_native/j_utils.c \
	$(top_srcdir)/java/ais_api_impl_native/j_utilsPrint.c \
	$(top_srcdir)/java/ais_api_impl_native/tracer.c

libjava_ais_api_native_la_LIBADD = \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la

libjava_ais_api_native_la_LDFLAGS = -version-info @OPENSAF_LIB_VERSION@

