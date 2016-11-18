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

EXTRA_DIST += \
	src/libjava/javah.xml

if ENABLE_JAVA

lib_LTLIBRARIES += lib/libjava_ais_api_native.la

lib_libjava_ais_api_native_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libjava_ais_api_native_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libjava_ais_api_native_la_CPPFLAGS = \
	-I $(JAVA_HOME)/include \
	-I $(JAVA_HOME)/include/linux \
	-DSA_CLM_B01 \
	$(AM_CPPFLAGS)

lib_libjava_ais_api_native_la_SOURCES = \
	src/libjava/j_ais.c \
	src/libjava/j_ais_amf.c \
	src/libjava/j_ais_amf_compRegistry.c \
	src/libjava/j_ais_amf_csiManager.c \
	src/libjava/j_ais_amf_errReporting.c \
	src/libjava/j_ais_amf_healthcheck.c \
	src/libjava/j_ais_amf_libHandle.c \
	src/libjava/j_ais_amf_pgManager.c \
	src/libjava/j_ais_amf_pm.c \
	src/libjava/j_ais_clm.c \
	src/libjava/j_ais_clm_libHandle.c \
	src/libjava/j_ais_clm_manager.c \
	src/libjava/j_ais_libHandle.c \
	src/libjava/j_ais_socketUtil.c \
	src/libjava/j_utils.c \
	src/libjava/j_utilsPrint.c \
	src/libjava/tracer.c

lib_libjava_ais_api_native_la_LIBADD = \
	lib/libSaAmf.la \
	lib/libSaClm.la

lib_libjava_ais_api_native_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(OPENSAF_LIB_VERSION)

noinst_HEADERS += \
	src/libjava/j_ais.h \
	src/libjava/j_ais_amf.h \
	src/libjava/j_ais_amf_compRegistry.h \
	src/libjava/j_ais_amf_csiManager.h \
	src/libjava/j_ais_amf_errReporting.h \
	src/libjava/j_ais_amf_healthcheck.h \
	src/libjava/j_ais_amf_libHandle.h \
	src/libjava/j_ais_amf_pgManager.h \
	src/libjava/j_ais_amf_pm.h \
	src/libjava/j_ais_clm.h \
	src/libjava/j_ais_clm_libHandle.h \
	src/libjava/j_ais_clm_manager.h \
	src/libjava/j_ais_libHandle.h \
	src/libjava/j_utils.h \
	src/libjava/j_utilsPrint.h \
	src/libjava/jni_ais.h \
	src/libjava/jni_ais_amf.h \
	src/libjava/jni_ais_clm.h \
	src/libjava/tracer.h

endif
