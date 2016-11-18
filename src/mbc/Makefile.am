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
	src/mbc/mbcsv.h \
	src/mbc/mbcsv_dl_api.h \
	src/mbc/mbcsv_env.h \
	src/mbc/mbcsv_evt_msg.h \
	src/mbc/mbcsv_evts.h \
	src/mbc/mbcsv_mbx.h \
	src/mbc/mbcsv_mds.h \
	src/mbc/mbcsv_mem.h \
	src/mbc/mbcsv_papi.h \
	src/mbc/mbcsv_pwe_anc.h \
	src/mbc/mbcsv_tmr.h

lib_libopensaf_core_la_CPPFLAGS += \
	-DNCS_MBCSV=1

lib_libopensaf_core_la_SOURCES += \
	src/mbc/mbcsv_act.c \
	src/mbc/mbcsv_api.c \
	src/mbc/mbcsv_dl_api.c \
	src/mbc/mbcsv_fsm.c \
	src/mbc/mbcsv_mbx.c \
	src/mbc/mbcsv_mds.c \
	src/mbc/mbcsv_peer.c \
	src/mbc/mbcsv_pr_evts.c \
	src/mbc/mbcsv_pwe_anc.c \
	src/mbc/mbcsv_queue.c \
	src/mbc/mbcsv_tmr.c \
	src/mbc/mbcsv_util.c

if ENABLE_TESTS

noinst_HEADERS += \
	src/mbc/apitest/mbcsv_api.h \
	src/mbc/apitest/mbcsv_purpose.h

endif
