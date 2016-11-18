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
	src/dtm/dtmnd/dtm.h \
	src/dtm/dtmnd/dtm_cb.h \
	src/dtm/dtmnd/dtm_inter.h \
	src/dtm/dtmnd/dtm_inter_disc.h \
	src/dtm/dtmnd/dtm_inter_trans.h \
	src/dtm/dtmnd/dtm_intra.h \
	src/dtm/dtmnd/dtm_intra_disc.h \
	src/dtm/dtmnd/dtm_intra_trans.h \
	src/dtm/dtmnd/dtm_node.h \
	src/dtm/dtmnd/dtm_socket.h \
	src/dtm/transport/log_server.h \
	src/dtm/transport/log_writer.h \
	src/dtm/transport/tests/mock_logtrace.h \
	src/dtm/transport/tests/mock_osaf_poll.h \
	src/dtm/transport/transport_monitor.h

osaf_execbin_PROGRAMS += bin/osafdtmd bin/osaftransportd
nodist_pkgclccli_SCRIPTS += \
	src/dtm/dtmnd/osaf-dtm \
	src/dtm/transport/osaf-transport

dist_pkgsysconf_DATA += \
	src/dtm/dtmnd/dtmd.conf

bin_osaftransportd_CXXFLAGS =$(AM_CXXFLAGS)

bin_osaftransportd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osaftransportd_SOURCES = \
	src/dtm/transport/log_server.cc \
	src/dtm/transport/log_writer.cc \
	src/dtm/transport/main.cc \
	src/dtm/transport/transport_monitor.cc

bin_osaftransportd_LDADD = \
	lib/libopensaf_core.la

bin_osafdtmd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osafdtmd_SOURCES = \
	src/dtm/dtmnd/dtm_intra.c \
	src/dtm/dtmnd/dtm_main.c \
	src/dtm/dtmnd/dtm_read_config.c \
	src/dtm/dtmnd/dtm_node.c \
	src/dtm/dtmnd/dtm_node_db.c \
	src/dtm/dtmnd/dtm_node_sockets.c \
	src/dtm/dtmnd/dtm_inter_svc.c \
	src/dtm/dtmnd/dtm_intra_svc.c \
	src/dtm/dtmnd/dtm_intra_trans.c \
	src/dtm/dtmnd/dtm_inter_trans.c

bin_osafdtmd_LDADD = \
	lib/libopensaf_core.la

TESTS += bin/transport_test

bin_transport_test_CXXFLAGS =$(AM_CXXFLAGS)

bin_transport_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(GTEST_DIR)/include

bin_transport_test_LDFLAGS = \
	$(AM_LDFLAGS) \
	-pthread -lrt \
	src/base/lib_libopensaf_core_la-getenv.o \
	src/dtm/transport/bin_osaftransportd-transport_monitor.o

bin_transport_test_SOURCES = \
	src/dtm/transport/tests/transport_monitor_test.cc \
	src/dtm/transport/tests/mock_logtrace.cc \
	src/dtm/transport/tests/mock_osaf_poll.cc

bin_transport_test_LDADD = \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la \
	lib/libopensaf_core.la
