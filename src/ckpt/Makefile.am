#      -*- OpenSAF  -*-
#
# (C) Copyright 2016 The OpenSAF Foundation
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
# Author(s): Ericsson AB
#

EXTRA_DIST += src/ckpt/saf/libSaCkpt.map

if ENABLE_AIS_CKPT

noinst_LTLIBRARIES += lib/libckpt_common.la

lib_libckpt_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libckpt_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libckpt_common_la_CPPFLAGS = \
	-DNCS_CPND=1 -DNCS_CPD=1 \
	$(AM_CPPFLAGS)

lib_libckpt_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_libckpt_common_la_SOURCES = \
	src/ckpt/cpsv_edu.c \
	src/ckpt/cpsv_evt.c \
	src/ckpt/cpsv_mbedu.c

lib_LTLIBRARIES += lib/libSaCkpt.la

lib_libSaCkpt_la_SOURCES = \
	src/ckpt/agent/cpa_api.c \
	src/ckpt/agent/cpa_db.c \
	src/ckpt/agent/cpa_init.c \
	src/ckpt/agent/cpa_mds.c \
	src/ckpt/agent/cpa_proc.c \
	src/ckpt/agent/cpa_tmr.c

lib_libSaCkpt_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaCkpt_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaCkpt_la_CPPFLAGS = \
	-DNCS_CPA=1 \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libSaCkpt_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaCkpt_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/ckpt/saf/libSaCkpt.map
endif

lib_libSaCkpt_la_LDFLAGS += \
	-version-number 1:0:2 -lstdc++

lib_libSaCkpt_la_LIBADD = \
	lib/libckpt_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/ckpt/saf/saCkpt_B_02_03.h \
	src/ckpt/saf/saCkpt.h

pkginclude_HEADERS += \
	src/ckpt/saf/cpsv_papi.h

noinst_HEADERS += \
	src/ckpt/agent/cpa.h \
	src/ckpt/agent/cpa_cb.h \
	src/ckpt/agent/cpa_def.h \
	src/ckpt/agent/cpa_dl_api.h \
	src/ckpt/agent/cpa_mds.h \
	src/ckpt/agent/cpa_mem.h \
	src/ckpt/agent/cpa_proc.h \
	src/ckpt/agent/cpa_tmr.h \
	src/ckpt/ckptd/cpd.h \
	src/ckpt/ckptd/cpd_cb.h \
	src/ckpt/ckptd/cpd_dl_api.h \
	src/ckpt/ckptd/cpd_imm.h \
	src/ckpt/ckptd/cpd_init.h \
	src/ckpt/ckptd/cpd_mds.h \
	src/ckpt/ckptd/cpd_mem.h \
	src/ckpt/ckptd/cpd_proc.h \
	src/ckpt/ckptd/cpd_red.h \
	src/ckpt/ckptd/cpd_sbedu.h \
	src/ckpt/ckptd/cpd_tmr.h \
	src/ckpt/ckptnd/cpnd.h \
	src/ckpt/ckptnd/cpnd_cb.h \
	src/ckpt/ckptnd/cpnd_dl_api.h \
	src/ckpt/ckptnd/cpnd_init.h \
	src/ckpt/ckptnd/cpnd_mem.h \
	src/ckpt/ckptnd/cpnd_sec.h \
	src/ckpt/ckptnd/cpnd_tmr.h \
	src/ckpt/cpsv.h \
	src/ckpt/cpsv_evt.h \
	src/ckpt/cpsv_mem.h \
	src/ckpt/cpsv_shm.h

osaf_execbin_PROGRAMS += bin/osafckptd bin/osafckptnd
CORE_INCLUDES += -I$(top_srcdir)/src/ckpt/saf
pkgconfig_DATA += src/ckpt/saf/opensaf-ckpt.pc

nodist_pkgclccli_SCRIPTS += \
	src/ckpt/ckptd/osaf-ckptd \
	src/ckpt/ckptnd/osaf-ckptnd

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/ckpt/cpsv_classes.xml \
	src/ckpt/cpsv_objects.xml \
	src/ckpt/cpsv_pl_template.xml \
	src/ckpt/cpsv_sc_template.xml

endif

dist_pkgsysconf_DATA += \
	src/ckpt/ckptd/ckptd.conf \
	src/ckpt/ckptnd/ckptnd.conf

bin_osafckptnd_CXXFLAGS =$(AM_CXXFLAGS)

bin_osafckptnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_CPND=1  \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafckptnd_SOURCES = \
	src/ckpt/ckptnd/cpnd_amf.c \
	src/ckpt/ckptnd/cpnd_db.c \
	src/ckpt/ckptnd/cpnd_evt.c \
	src/ckpt/ckptnd/cpnd_init.c \
	src/ckpt/ckptnd/cpnd_main.c \
	src/ckpt/ckptnd/cpnd_mds.c \
	src/ckpt/ckptnd/cpnd_proc.c \
	src/ckpt/ckptnd/cpnd_res.c \
	src/ckpt/ckptnd/cpnd_sec.cc \
	src/ckpt/ckptnd/cpnd_tmr.c

bin_osafckptnd_LDADD = \
	lib/libckpt_common.la \
	lib/libSaAmf.la \
	lib/libSaClm.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_osafckptd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_CPD=1 \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafckptd_SOURCES = \
	src/ckpt/ckptd/cpd_amf.c \
	src/ckpt/ckptd/cpd_db.c \
	src/ckpt/ckptd/cpd_evt.c \
	src/ckpt/ckptd/cpd_imm.c \
	src/ckpt/ckptd/cpd_init.c \
	src/ckpt/ckptd/cpd_main.c \
	src/ckpt/ckptd/cpd_mbcsv.c \
	src/ckpt/ckptd/cpd_mds.c \
	src/ckpt/ckptd/cpd_proc.c \
	src/ckpt/ckptd/cpd_red.c \
	src/ckpt/ckptd/cpd_sbevt.c \
	src/ckpt/ckptd/cpd_tmr.c

bin_osafckptd_LDADD = \
	lib/libSaAmf.la \
	lib/libSaClm.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libckpt_common.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

noinst_HEADERS += \
	src/ckpt/apitest/test_cpsv.h \
	src/ckpt/apitest/test_cpsv_conf.h

bin_PROGRAMS += bin/ckpttest

bin_ckpttest_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

# Uncomment the line below to print full log
#bin_ckpttest_CPPFLAGS += -DFULL_LOG

bin_ckpttest_SOURCES = \
	src/ckpt/apitest/ckpttest.c \
	src/ckpt/apitest/test_cpa.c \
	src/ckpt/apitest/test_cpa_util.c

bin_ckpttest_LDADD = \
	lib/libapitest.la \
	lib/libSaCkpt.la \
	lib/libopensaf_core.la

endif

endif
