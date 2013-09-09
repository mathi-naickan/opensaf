#      -*- OpenSAF  -*-
#
# (C) Copyright 2009 The OpenSAF Foundation
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
# Author(s): Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = ntftest

dist_bin_SCRIPTS = ntf_search_criteria_test.sh

noinst_HEADERS = \
	sa_error.h \
	tet_ntf_common.h \
	tet_ntf.h

ntftest_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/tests/unit_test_fw/inc

ntftest_SOURCES = \
	$(top_srcdir)/tests/unit_test_fw/src/utest.c \
	$(top_srcdir)/tests/unit_test_fw/src/util.c \
	$(top_srcdir)/osaf/libs/core/common/logtrace.c \
	tet_ntf_main.c \
	sa_error.c \
	tet_ntf_common.c \
	tet_saNtfInitialize.c \
	tet_saNtfSelectionObjectGet.c \
	tet_saNtfDispatch.c \
	tet_saNtfFinalize.c \
	tet_saNtfNotificationCallbackT.c \
	tet_saNtfNotificationDiscardedCallbackT.c \
	tet_saNtfMiscellaneousNotificationAllocate.c \
	tet_saNtfAlarmNotificationAllocate.c \
	tet_saNtfAttributeChangeNotificationAllocate.c \
	tet_saNtfStateChangeNotificationAllocate.c \
	tet_saNtfSecurityAlarmNotificationAllocate.c \
	tet_saNtfNotificationSend.c \
	tet_saNtfNotificationSubscribe.c \
	tet_saNtfStateChangeNotificationFilterAllocate.c \
	tet_saNtfSecurityAlarmNotificationFilterAllocate.c \
	tet_saNtfObjectCreateDeleteNotificationFilterAllocate.c \
	tet_saNtfAttributeChangeNotificationFilterAllocate.c \
	tet_saNtfAlarmNotificationFilterAllocate.c \
	tet_saNtfNotificationUnsubscribe.c \
	tet_saNtfObjectCreateDeleteNotificationAllocate.c \
	tet_saNtfNotificationReadInitialize.c \
	tet_saNtfNotificationReadFinalize.c \
	tet_saNtfNotificationReadNext.c \
	tet_NotificationContentVerification.c \
	tet_saNtfArrayValAllocate.c \
	tet_saNtfPtrValAllocate.c \
	tet_saNtfArrayValGet.c \
	tet_saNtfPtrValGet.c \
	test_ntfFilterVerification.c

if ENABLE_NTFIMCN

ntftest_SOURCES += test_ntf_imcn.c
ntftest_SOURCES += $(top_srcdir)/osaf/tools/safimm/src/immutil.c
ntftest_CPPFLAGS += -I$(top_srcdir)/osaf/libs/common/immsv/include

endif

ntftest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaNtf/libSaNtf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la
