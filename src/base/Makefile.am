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

lib_LTLIBRARIES += lib/libopensaf_core.la

lib_libopensaf_core_la_CFLAGS = $(OSAF_LIB_FLAGS) -fno-strict-aliasing $(AM_CFLAGS)

lib_libopensaf_core_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libopensaf_core_la_CPPFLAGS += \
	-DUSE_LITTLE_ENDIAN=1 -DNCSL_ENV_INIT_HM=1 \
	-DNCSL_ENV_INIT_MTM=1 -DNCSL_ENV_INIT_TMR=1 \
	$(AM_CPPFLAGS)

lib_libopensaf_core_la_LDFLAGS += \
	$(AM_LDFLAGS) \
	-pthread -ldl -lrt -lstdc++ \
	-version-number 0:2:0

lib_libopensaf_core_la_SOURCES += \
	src/base/daemon.c \
	src/base/file_notify.cc \
	src/base/getenv.cc \
	src/base/hj_dec.c \
	src/base/hj_edp.c \
	src/base/hj_edu.c \
	src/base/hj_enc.c \
	src/base/hj_hdl.c \
	src/base/hj_queue.c \
	src/base/hj_tmr.c \
	src/base/hj_ubaid.c \
	src/base/log_message.cc \
	src/base/logtrace.c \
	src/base/ncs_main_pub.c \
	src/base/ncs_sprr.c \
	src/base/ncsdlib.c \
	src/base/os_defs.c \
	src/base/osaf_extended_name.c \
	src/base/osaf_poll.c \
	src/base/osaf_secutil.c \
	src/base/osaf_time.c \
	src/base/osaf_timerfd.c \
	src/base/osaf_unicode.c \
	src/base/osaf_utility.c \
	src/base/patricia.c \
	src/base/process.cc \
	src/base/saf_edu.c \
	src/base/saf_error.c \
	src/base/sysf_def.c \
	src/base/sysf_exc_scr.c \
	src/base/sysf_ipc.c \
	src/base/sysf_mem.c \
	src/base/sysf_tmr.c \
	src/base/sysf_tsk.c \
	src/base/unix_client_socket.cc \
	src/base/unix_server_socket.cc \
	src/base/unix_socket.cc

noinst_HEADERS += \
	src/base/buffer.h \
	src/base/daemon.h \
	src/base/file_notify.h \
	src/base/getenv.h \
	src/base/log_message.h \
	src/base/logtrace.h \
	src/base/macros.h \
	src/base/ncs_edu.h \
	src/base/ncs_edu_pub.h \
	src/base/ncs_hdl.h \
	src/base/ncs_hdl_pub.h \
	src/base/ncs_lib.h \
	src/base/ncs_main_papi.h \
	src/base/ncs_main_pub.h \
	src/base/ncs_mda_papi.h \
	src/base/ncs_mda_pvt.h \
	src/base/ncs_osprm.h \
	src/base/ncs_queue.h \
	src/base/ncs_saf.h \
	src/base/ncs_saf_edu.h \
	src/base/ncs_sprr_papi.h \
	src/base/ncs_svd.h \
	src/base/ncs_tmr.h \
	src/base/ncs_ubaid.h \
	src/base/ncs_util.h \
	src/base/ncsdlib.h \
	src/base/ncsencdec_pub.h \
	src/base/ncsgl_defs.h \
	src/base/ncspatricia.h \
	src/base/ncssysf_def.h \
	src/base/ncssysf_ipc.h \
	src/base/ncssysf_lck.h \
	src/base/ncssysf_mem.h \
	src/base/ncssysf_tmr.h \
	src/base/ncssysf_tsk.h \
	src/base/ncssysfpool.h \
	src/base/ncsusrbuf.h \
	src/base/os_defs.h \
	src/base/osaf_extended_name.h \
	src/base/osaf_poll.h \
	src/base/osaf_secutil.h \
	src/base/osaf_time.h \
	src/base/osaf_timerfd.h \
	src/base/osaf_unicode.h \
	src/base/osaf_utility.h \
	src/base/process.h \
	src/base/saf_def.h \
	src/base/saf_error.h \
	src/base/saf_mem.h \
	src/base/sprr_dl_api.h \
	src/base/sysf_exc_scr.h \
	src/base/sysf_ipc.h \
	src/base/tests/mock_clock_gettime.h \
	src/base/tests/mock_clock_nanosleep.h \
	src/base/tests/mock_logtrace.h \
	src/base/tests/mock_osaf_abort.h \
	src/base/tests/mock_osafassert.h \
	src/base/tests/mock_syslog.h \
	src/base/time.h \
	src/base/unix_client_socket.h \
	src/base/unix_server_socket.h \
	src/base/unix_socket.h \
	src/base/usrbuf.h

TESTS += bin/testleap bin/libbase_test bin/core_common_test

bin_testleap_CXXFLAGS =$(AM_CXXFLAGS)

bin_testleap_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(GTEST_DIR)/include

bin_testleap_LDFLAGS = \
	$(AM_LDFLAGS) \
	-lpthread

bin_testleap_SOURCES = \
	src/base/tests/sysf_ipc_test.cc \
	src/base/tests/sysf_tmr_test.cc

bin_testleap_LDADD = \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la \
	lib/libosaf_common.la \
	lib/libosaf_common.la \
	lib/libopensaf_core.la \
	lib/libamf_common.la \
	lib/libSaNtf.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libSaClm.la \
	lib/libSaLog.la

bin_libbase_test_CXXFLAGS =$(AM_CXXFLAGS)

bin_libbase_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(GTEST_DIR)/include

bin_libbase_test_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pthread -lrt \
	src/base/lib_libopensaf_core_la-getenv.o \
	src/base/lib_libopensaf_core_la-log_message.o \
	src/base/lib_libopensaf_core_la-process.o \
	src/base/lib_libopensaf_core_la-file_notify.o

bin_libbase_test_SOURCES = \
	src/base/tests/getenv_test.cc \
	src/base/tests/log_message_test.cc \
	src/base/tests/mock_logtrace.cc \
	src/base/tests/mock_osaf_abort.cc \
	src/base/tests/mock_osafassert.cc \
	src/base/tests/time_add_test.cc \
	src/base/tests/time_compare_test.cc \
	src/base/tests/time_convert_test.cc \
	src/base/tests/file_notify_test.cc \
	src/base/tests/time_subtract_test.cc

bin_libbase_test_LDADD = \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la \
	lib/libopensaf_core.la

bin_core_common_test_CXXFLAGS =$(AM_CXXFLAGS)

bin_core_common_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(GTEST_DIR)/include

bin_core_common_test_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pthread -lrt \
	src/base/lib_libopensaf_core_la-osaf_time.o \
	src/base/lib_libopensaf_core_la-osaf_utility.o

bin_core_common_test_SOURCES = \
	src/base/tests/osaf_clock_gettime_test.cc \
	src/base/tests/osaf_nanosleep_test.cc \
	src/base/tests/osaf_normalize_timespec_test.cc \
	src/base/tests/osaf_timespec_add_test.cc \
	src/base/tests/osaf_timespec_compare_test.cc \
	src/base/tests/osaf_timespec_convert_test.cc \
	src/base/tests/osaf_timespec_subtract_test.cc \
	src/base/tests/osaf_millis_timeout_test.cc \
	src/base/tests/mock_clock_gettime.cc \
	src/base/tests/mock_clock_nanosleep.cc \
	src/base/tests/mock_syslog.cc

bin_core_common_test_LDADD = \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la
