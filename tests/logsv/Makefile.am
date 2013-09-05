#      -*- OpenSAF  -*-
#
# (C) Copyright 2009 The OpenSAF Foundation
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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = logtest saflogtest

noinst_HEADERS = \
	logtest.h

logtest_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/tests/unit_test_fw/inc \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

logtest_SOURCES = \
	$(top_srcdir)/tests/unit_test_fw/src/utest.c \
	$(top_srcdir)/tests/unit_test_fw/src/util.c \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	logtest.c \
	tet_saLogInitialize.c \
	tet_saLogSelectionObjectGet.c \
	tet_saLogDispatch.c \
	tet_saLogFinalize.c \
	tet_saLogStreamOpen_2.c \
	tet_saLogStreamOpenAsync_2.c \
	tet_saLogStreamOpenCallbackT.c \
	tet_saLogWriteLog.c \
	tet_saLogWriteLogAsync.c \
	tet_saLogWriteLogCallbackT.c \
	tet_saLogFilterSetCallbackT.c \
	tet_saLogStreamClose.c \
	tet_saLogLimitGet.c \
	tet_LogOiOps.c

logtest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaLog/libSaLog.la

saflogtest_SOURCES = \
	saflogtest.c

saflogtest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaLog/libSaLog.la

