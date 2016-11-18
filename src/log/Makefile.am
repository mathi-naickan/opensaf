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

EXTRA_DIST += src/log/saf/libSaLog.map

lib_LTLIBRARIES += lib/libSaLog.la

lib_libSaLog_la_SOURCES = \
	src/log/agent/lga_api.c \
	src/log/agent/lga_util.c \
	src/log/agent/lga_mds.c \
	src/log/agent/lga_state.c

lib_libSaLog_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaLog_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaLog_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libSaLog_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaLog_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/log/saf/libSaLog.map
endif

lib_libSaLog_la_LDFLAGS += \
	-version-number 1:0:2 -lstdc++

lib_libSaLog_la_LIBADD = \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/log/saf/saLog.h

noinst_HEADERS += \
	src/log/agent/lga.h \
	src/log/agent/lga_state.h \
	src/log/lgsv_defs.h \
	src/log/lgsv_msg.h \
	src/log/logd/lgs.h \
	src/log/logd/lgs_cb.h \
	src/log/logd/lgs_clm.h \
	src/log/logd/lgs_config.h \
	src/log/logd/lgs_evt.h \
	src/log/logd/lgs_file.h \
	src/log/logd/lgs_filehdl.h \
	src/log/logd/lgs_fmt.h \
	src/log/logd/lgs_imm_gcfg.h \
	src/log/logd/lgs_mbcsv.h \
	src/log/logd/lgs_mbcsv_v1.h \
	src/log/logd/lgs_mbcsv_v2.h \
	src/log/logd/lgs_mbcsv_v3.h \
	src/log/logd/lgs_mbcsv_v5.h \
	src/log/logd/lgs_recov.h \
	src/log/logd/lgs_stream.h \
	src/log/logd/lgs_util.h

bin_PROGRAMS += bin/saflogger
osaf_execbin_PROGRAMS += bin/osaflogd
CORE_INCLUDES += -I$(top_srcdir)/src/log/saf
pkgconfig_DATA += src/log/saf/opensaf-log.pc

nodist_pkgclccli_SCRIPTS += \
	src/log/logd/osaf-logd

dist_pkgsysconf_DATA += \
	src/log/logd/logd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/log/logsv_sc_template.xml \
	src/log/logsv_classes.xml \
	src/log/logsv_objects.xml

endif

bin_osaflogd_CXXFLAGS =$(AM_CXXFLAGS) -Wformat=1

bin_osaflogd_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osaflogd_SOURCES = \
	src/log/logd/lgs_amf.cc \
	src/log/logd/lgs_clm.cc \
	src/log/logd/lgs_config.cc \
	src/log/logd/lgs_evt.cc \
	src/log/logd/lgs_file.cc \
	src/log/logd/lgs_filehdl.cc \
	src/log/logd/lgs_fmt.cc \
	src/log/logd/lgs_imm.cc \
	src/log/logd/lgs_imm_gcfg.cc \
	src/log/logd/lgs_main.cc \
	src/log/logd/lgs_mbcsv.cc \
	src/log/logd/lgs_mbcsv_v1.cc \
	src/log/logd/lgs_mbcsv_v2.cc \
	src/log/logd/lgs_mbcsv_v3.cc \
	src/log/logd/lgs_mbcsv_v5.cc \
	src/log/logd/lgs_mds.cc \
	src/log/logd/lgs_recov.cc \
	src/log/logd/lgs_stream.cc \
	src/log/logd/lgs_util.cc

bin_osaflogd_LDADD = \
	lib/libosaf_common.la \
	lib/libSaAmf.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libSaClm.la \
	lib/libopensaf_core.la

bin_saflogger_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_saflogger_SOURCES = \
	src/log/tools/saf_logger.c

bin_saflogger_LDADD = \
	lib/libSaLog.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

bin_PROGRAMS += bin/logtest bin/saflogtest bin/logtestfr

noinst_HEADERS += \
	src/log/apitest/logtest.h \
	src/log/apitest/logutil.h

bin_logtest_CFLAGS = $(AM_CFLAGS) -Wformat=1

bin_logtest_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_logtest_SOURCES = \
	src/log/apitest/logtest.c \
	src/log/apitest/logutil.c \
	src/log/apitest/tet_saLogInitialize.c \
	src/log/apitest/tet_saLogSelectionObjectGet.c \
	src/log/apitest/tet_saLogDispatch.c \
	src/log/apitest/tet_saLogFinalize.c \
	src/log/apitest/tet_saLogStreamOpen_2.c \
	src/log/apitest/tet_saLogStreamOpenAsync_2.c \
	src/log/apitest/tet_saLogStreamOpenCallbackT.c \
	src/log/apitest/tet_saLogWriteLog.c \
	src/log/apitest/tet_saLogWriteLogAsync.c \
	src/log/apitest/tet_saLogWriteLogCallbackT.c \
	src/log/apitest/tet_saLogFilterSetCallbackT.c \
	src/log/apitest/tet_saLogStreamClose.c \
	src/log/apitest/tet_saLogLimitGet.c \
	src/log/apitest/tet_LogOiOps.c \
	src/log/apitest/tet_Log_misc.c \
	src/log/apitest/tet_Log_recov.c \
	src/log/apitest/tet_log_runtime_cfgobj.c \
	src/log/apitest/tet_log_longDN.c \
	src/log/apitest/tet_Log_clm.c

bin_logtest_LDADD = \
	lib/libapitest.la \
	lib/libosaf_common.la \
	lib/libSaLog.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_saflogtest_SOURCES = \
	src/log/apitest/saflogtest.c

bin_saflogtest_LDADD = \
	lib/libapitest.la \
	lib/libosaf_common.la \
	lib/libSaLog.la \
	lib/libopensaf_core.la

bin_logtestfr_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_logtestfr_SOURCES = \
	src/log/apitest/logtestfr.c

bin_logtestfr_LDADD = \
	lib/libapitest.la \
	lib/libosaf_common.la \
	lib/libSaLog.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

endif
