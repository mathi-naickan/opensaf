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

EXTRA_DIST += src/msg/saf/libSaMsg.map

if ENABLE_AIS_MSG

noinst_LTLIBRARIES += lib/libmsg_common.la

lib_libmsg_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libmsg_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libmsg_common_la_CPPFLAGS = \
	-DNCS_MQA=1 \
	$(AM_CPPFLAGS)

lib_libmsg_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_libmsg_common_la_SOURCES = \
	src/msg/mqsv_common.c \
	src/msg/mqsv_asapi_dec.c \
	src/msg/mqsv_asapi_enc.c \
	src/msg/mqsv_asapi.c \
	src/msg/mqsv_edu.c \
	src/msg/posix.c

lib_LTLIBRARIES += lib/libSaMsg.la

lib_libSaMsg_la_SOURCES = \
	src/msg/agent/mqa_api.c \
	src/msg/agent/mqa_mds.c \
	src/msg/agent/mqa_clbk.c \
	src/msg/agent/mqa_queue.c \
	src/msg/agent/mqa_init.c

lib_libSaMsg_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaMsg_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaMsg_la_CPPFLAGS = \
	-DNCS_MQA=1 \
	$(AM_CPPFLAGS)

lib_libSaMsg_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaMsg_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/msg/saf/libSaMsg.map
endif

lib_libSaMsg_la_LDFLAGS += \
	-version-number 1:0:2 -lstdc++

lib_libSaMsg_la_LIBADD = \
	lib/libmsg_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/msg/saf/saMsg.h

noinst_HEADERS += \
	src/msg/agent/mqa.h \
	src/msg/agent/mqa_db.h \
	src/msg/agent/mqa_def.h \
	src/msg/agent/mqa_dl_api.h \
	src/msg/agent/mqa_mem.h \
	src/msg/mqsv.h \
	src/msg/mqsv_asapi.h \
	src/msg/mqsv_asapi_mem.h \
	src/msg/mqsv_common.h \
	src/msg/mqsv_def.h \
	src/msg/mqsv_edu.h \
	src/msg/mqsv_evt.h \
	src/msg/mqsv_init.h \
	src/msg/mqsv_mbedu.h \
	src/msg/mqsv_mem.h \
	src/msg/msgd/mqd.h \
	src/msg/msgd/mqd_api.h \
	src/msg/msgd/mqd_clm.h \
	src/msg/msgd/mqd_db.h \
	src/msg/msgd/mqd_dl_api.h \
	src/msg/msgd/mqd_imm.h \
	src/msg/msgd/mqd_mem.h \
	src/msg/msgd/mqd_red.h \
	src/msg/msgd/mqd_saf.h \
	src/msg/msgd/mqd_tmr.h \
	src/msg/msgnd/mqnd.h \
	src/msg/msgnd/mqnd_db.h \
	src/msg/msgnd/mqnd_dl_api.h \
	src/msg/msgnd/mqnd_evt.h \
	src/msg/msgnd/mqnd_imm.h \
	src/msg/msgnd/mqnd_mds.h \
	src/msg/msgnd/mqnd_mem.h \
	src/msg/msgnd/mqnd_proc.h \
	src/msg/msgnd/mqnd_restart.h \
	src/msg/msgnd/mqnd_saf.h \
	src/msg/msgnd/mqnd_shm.h \
	src/msg/msgnd/mqnd_tmr.h

osaf_execbin_PROGRAMS += bin/osafmsgd bin/osafmsgnd
CORE_INCLUDES += -I$(top_srcdir)/src/msg/saf
pkgconfig_DATA += src/msg/saf/opensaf-msg.pc

nodist_pkgclccli_SCRIPTS += \
	src/msg/msgd/osaf-msgd \
	src/msg/msgnd/osaf-msgnd

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/msg/mqsv_sc_template.xml \
	src/msg/mqsv_pl_template.xml \
	src/msg/mqsv_classes.xml \
	src/msg/mqsv_objects.xml

endif

dist_pkgsysconf_DATA += \
	src/msg/msgd/msgd.conf \
	src/msg/msgnd/msgnd.conf

bin_osafmsgnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_MQND=1 -DASAPi_DEBUG=1 \
	$(AM_CPPFLAGS)

bin_osafmsgnd_SOURCES = \
	src/msg/msgnd/mqnd_amf.c \
	src/msg/msgnd/mqnd_db.c \
	src/msg/msgnd/mqnd_evt.c \
	src/msg/msgnd/mqnd_imm.c \
	src/msg/msgnd/mqnd_init.c \
	src/msg/msgnd/mqnd_main.c \
	src/msg/msgnd/mqnd_mds.c \
	src/msg/msgnd/mqnd_mq.c \
	src/msg/msgnd/mqnd_namedb.c \
	src/msg/msgnd/mqnd_proc.c \
	src/msg/msgnd/mqnd_restart.c \
	src/msg/msgnd/mqnd_shm.c \
	src/msg/msgnd/mqnd_tmr.c \
	src/msg/msgnd/mqnd_util.c

bin_osafmsgnd_LDADD = \
	lib/libmsg_common.la \
	lib/libSaAmf.la \
	lib/libSaClm.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_osafmsgd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_MQD=1 -DASAPi_DEBUG=1 \
	$(AM_CPPFLAGS)

bin_osafmsgd_SOURCES = \
	src/msg/msgd/mqd_sbevt.c \
	src/msg/msgd/mqd_mbcsv.c \
	src/msg/msgd/mqd_mbedu.c \
	src/msg/msgd/mqd_util.c \
	src/msg/msgd/mqd_asapi.c \
	src/msg/msgd/mqd_api.c \
	src/msg/msgd/mqd_clm.c \
	src/msg/msgd/mqd_evt.c \
	src/msg/msgd/mqd_imm.c \
	src/msg/msgd/mqd_main.c \
	src/msg/msgd/mqd_mds.c \
	src/msg/msgd/mqd_red.c \
	src/msg/msgd/mqd_saf.c \
	src/msg/msgd/mqd_tmr.c

bin_osafmsgd_LDADD = \
	lib/libmsg_common.la \
	lib/libSaAmf.la \
	lib/libSaClm.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

noinst_HEADERS += \
	src/msg/apitest/tet_mqa_conf.h \
	src/msg/apitest/tet_mqsv.h

endif

endif
