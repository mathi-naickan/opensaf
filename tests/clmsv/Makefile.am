#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
# file and program are licensed under High-Availability Operating
# Environment Software License Version 1.4.
# Complete License can be accesseble from below location.
# http://www.opensaf.org/license
# See the Copying file included with the OpenSAF distribution for
# full licensing terms.
#
# Author(s): Emerson Network Power
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = clmtest

noinst_HEADERS = \
	$(top_srcdir)/tests/clmsv/src/clmtest.h

clmtest_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/tests/unit_test_fw/inc

clmtest_SOURCES = \
	$(top_srcdir)/osaf/libs/core/common/logtrace.c \
	$(top_srcdir)/tests/unit_test_fw/src/utest.c \
	$(top_srcdir)/tests/unit_test_fw/src/util.c \
	$(top_srcdir)/tests/clmsv/src/clmtest.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmInitialize.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmClusterNodeGetAsync.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmClusterTrack.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmClusterTrackStop.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmClusterNotificationFree.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmClusterNodeGet.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmResponse.c \
	$(top_srcdir)/tests/clmsv/src/tet_ClmOiOps.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmDispatch.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmSelectionObjectGet.c \
	$(top_srcdir)/tests/clmsv/src/tet_saClmFinalize.c

clmtest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la
