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

noinst_HEADERS += \
	src/mds/mda_dl_api.h \
	src/mds/mda_mem.h \
	src/mds/mda_pvt_api.h \
	src/mds/mds_adm.h \
	src/mds/mds_core.h \
	src/mds/mds_dl_api.h \
	src/mds/mds_dt.h \
	src/mds/mds_dt2c.h \
	src/mds/mds_dt_tcp.h \
	src/mds/mds_dt_tcp_disc.h \
	src/mds/mds_dt_tcp_trans.h \
	src/mds/mds_log.h \
	src/mds/mds_papi.h \
	src/mds/mds_pvt.h

lib_libopensaf_core_la_SOURCES += \
	src/mds/mds_c_api.c \
	src/mds/mds_c_db.c \
	src/mds/mds_c_sndrcv.c \
	src/mds/mds_dt_common.c \
	src/mds/mds_dt_disc.c \
	src/mds/mds_dt_tcp.c \
	src/mds/mds_dt_trans.c \
	src/mds/mds_log.cc \
	src/mds/mds_main.c \
	src/mds/mds_papi.c \
	src/mds/ncs_ada.c \
	src/mds/ncs_mda.c \
	src/mds/ncs_vda.c

if ENABLE_TIPC_TRANSPORT
noinst_HEADERS += src/mds/mds_dt_tipc.h
lib_libopensaf_core_la_SOURCES += src/mds/mds_dt_tipc.c
endif

if ENABLE_TESTS

bin_PROGRAMS += bin/mdstest

noinst_HEADERS += \
	src/mds/apitest/mdstipc.h

bin_mdstest_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_mdstest_SOURCES = \
	src/mds/apitest/mdstest.c \
	src/mds/apitest/mdstipc_api.c \
	src/mds/apitest/mdstipc_conf.c

bin_mdstest_LDADD = \
	lib/libapitest.la \
	lib/libosaf_common.la \
	lib/libSaLog.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

endif
