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

noinst_LTLIBRARIES += lib/libclm_common.la

lib_libclm_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libclm_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libclm_common_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_libclm_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_libclm_common_la_SOURCES = \
	src/clm/clmsv_enc_dec.c

lib_libSaClm_la_SOURCES = \
	src/clm/agent/clma_api.c \
	src/clm/agent/clma_mds.c \
	src/clm/agent/clma_util.c

lib_LTLIBRARIES += lib/libSaClm.la

lib_libSaClm_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaClm_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaClm_la_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS)

lib_libSaClm_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaClm_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/clm/saf/libSaClm.map
endif

lib_libSaClm_la_LDFLAGS += \
	-version-number 0:0:0 -lstdc++

lib_libSaClm_la_LIBADD = \
	lib/libclm_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/clm/saf/saClm.h

noinst_HEADERS += \
	src/clm/agent/clma.h \
	src/clm/clmd/clms.h \
	src/clm/clmd/clms_cb.h \
	src/clm/clmd/clms_evt.h \
	src/clm/clmd/clms_imm.h \
	src/clm/clmd/clms_mbcsv.h \
	src/clm/clmd/clms_ntf.h \
	src/clm/clmnd/cb.h \
	src/clm/clmnd/clmna.h \
	src/clm/clmnd/election_starter.h \
	src/clm/clmnd/election_starter_wrapper.h \
	src/clm/clmnd/evt.h \
	src/clm/clmsv_defs.h \
	src/clm/clmsv_enc_dec.h \
	src/clm/clmsv_msg.h

osaf_execbin_PROGRAMS += bin/osafclmd bin/osafclmna
EXTRA_DIST += src/clm/saf/libSaClm.map
CORE_INCLUDES += -I$(top_srcdir)/src/clm/saf
pkgconfig_DATA += src/clm/saf/opensaf-clm.pc

nodist_pkgclccli_SCRIPTS += \
	src/clm/clmd/osaf-clmd \
	src/clm/clmnd/osaf-clmna

dist_pkgsysconf_DATA += \
	src/clm/clmd/clmd.conf \
	src/clm/clmnd/clmna.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/clm/clmsv_classes.xml \
	src/clm/clmsv_objects.xml

if ENABLE_AIS_PLM
dist_pkgimmxml_svc_DATA += \
	src/clm/clmsv_plm_sc_template.xml \
	src/clm/clmsv_plm_pl_template.xml
else
dist_pkgimmxml_svc_DATA += \
	src/clm/clmsv_sc_template.xml \
	src/clm/clmsv_pl_template.xml
endif

endif

bin_osafclmd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osafclmd_SOURCES = \
	src/clm/clmd/clms_amf.c \
	src/clm/clmd/clms_evt.c \
	src/clm/clmd/clms_imm.c \
	src/clm/clmd/clms_main.c \
	src/clm/clmd/clms_util.c \
	src/clm/clmd/clms_mbcsv.c \
	src/clm/clmd/clms_ntf.c \
	src/clm/clmd/clms_mds.c

bin_osafclmd_LDADD = \
	lib/libclm_common.la \
	lib/libosaf_common.la \
	lib/libosaf_common.la \
	lib/libSaAmf.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libSaLog.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la

if ENABLE_AIS_PLM

bin_osafclmd_SOURCES += src/clm/clmd/clms_plm.c

bin_osafclmd_LDADD += \
	lib/libSaPlm.la

endif

bin_osafclmna_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osafclmna_SOURCES = \
	src/clm/clmnd/main.c \
	src/clm/clmnd/amf.c \
	src/clm/clmnd/election_starter.cc \
	src/clm/clmnd/election_starter_wrapper.cc

bin_osafclmna_LDADD = \
	lib/libclm_common.la \
	lib/libSaAmf.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

bin_PROGRAMS += bin/clmtest

noinst_HEADERS += \
	src/clm/apitest/clmtest.h

bin_clmtest_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS)

bin_clmtest_SOURCES = \
	src/clm/apitest/clmtest.c \
	src/clm/apitest/tet_saClmInitialize.c \
	src/clm/apitest/tet_saClmClusterNodeGetAsync.c \
	src/clm/apitest/tet_saClmClusterTrack.c \
	src/clm/apitest/tet_saClmClusterTrackStop.c \
	src/clm/apitest/tet_saClmClusterNotificationFree.c \
	src/clm/apitest/tet_saClmClusterNodeGet.c \
	src/clm/apitest/tet_saClmResponse.c \
	src/clm/apitest/tet_ClmOiOps.c \
	src/clm/apitest/tet_saClmDispatch.c \
	src/clm/apitest/tet_saClmSelectionObjectGet.c \
	src/clm/apitest/tet_saClmFinalize.c \
	src/clm/apitest/tet_ClmLongRdn.c

bin_clmtest_LDADD = \
	lib/libSaClm.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la \
	lib/libapitest.la

endif
