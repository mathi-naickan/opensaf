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

EXTRA_DIST += src/ntf/saf/libSaNtf.map

noinst_LTLIBRARIES += lib/libntf_common.la lib/libntfclient.la

lib_libntf_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libntf_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libntf_common_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libntf_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_libntf_common_la_SOURCES = \
	src/ntf/ntfsv_enc_dec.c \
	src/ntf/ntfsv_mem.c

lib_libntfclient_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libntfclient_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libntfclient_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libntfclient_la_SOURCES = \
	src/ntf/tools/ntfclient.c

lib_LTLIBRARIES += lib/libSaNtf.la

lib_libSaNtf_la_SOURCES = \
	src/ntf/agent/ntfa_api.c \
	src/ntf/agent/ntfa_util.c \
	src/ntf/agent/ntfa_mds.c

lib_libSaNtf_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaNtf_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaNtf_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libSaNtf_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaNtf_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/ntf/saf/libSaNtf.map
endif

lib_libSaNtf_la_LDFLAGS += \
	-version-number 0:1:2 -lstdc++

lib_libSaNtf_la_LIBADD = \
	lib/libntf_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/ntf/saf/saNtf.h

noinst_HEADERS += \
	src/ntf/agent/ntfa.h \
	src/ntf/ntfd/NtfAdmin.h \
	src/ntf/ntfd/NtfClient.h \
	src/ntf/ntfd/NtfFilter.h \
	src/ntf/ntfd/NtfLogger.h \
	src/ntf/ntfd/NtfNotification.h \
	src/ntf/ntfd/NtfReader.h \
	src/ntf/ntfd/NtfSubscription.h \
	src/ntf/ntfd/ntfs.h \
	src/ntf/ntfd/ntfs_cb.h \
	src/ntf/ntfd/ntfs_com.h \
	src/ntf/ntfd/ntfs_evt.h \
	src/ntf/ntfd/ntfs_imcnutil.h \
	src/ntf/ntfd/ntfs_mbcsv.h \
	src/ntf/ntfimcnd/ntfimcn_imm.h \
	src/ntf/ntfimcnd/ntfimcn_main.h \
	src/ntf/ntfimcnd/ntfimcn_notifier.h \
	src/ntf/ntfsv_defs.h \
	src/ntf/ntfsv_enc_dec.h \
	src/ntf/ntfsv_mem.h \
	src/ntf/ntfsv_msg.h \
	src/ntf/tools/ntfclient.h \
	src/ntf/tools/ntfconsumer.h

bin_PROGRAMS += bin/ntfread bin/ntfsend bin/ntfsubscribe
osaf_execbin_PROGRAMS += bin/osafntfd
CORE_INCLUDES += -I$(top_srcdir)/src/ntf/saf
pkgconfig_DATA += src/ntf/saf/opensaf-ntf.pc

nodist_pkgclccli_SCRIPTS += \
	src/ntf/ntfd/osaf-ntfd

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/ntf/ntfsv_sc_template.xml \
	src/ntf/ntfsv_objects.xml

endif

dist_pkgsysconf_DATA += \
	src/ntf/ntfd/ntfd.conf

bin_osafntfd_CXXFLAGS = $(AM_CXXFLAGS)

bin_osafntfd_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafntfd_SOURCES = \
	src/ntf/ntfd/ntfs_amf.c \
	src/ntf/ntfd/ntfs_clm.c \
	src/ntf/ntfd/ntfs_evt.c \
	src/ntf/ntfd/ntfs_mbcsv.c \
	src/ntf/ntfd/ntfs_main.c \
	src/ntf/ntfd/ntfs_mds.c \
	src/ntf/ntfd/ntfs_com.c \
	src/ntf/ntfd/NtfNotification.cc \
	src/ntf/ntfd/NtfFilter.cc \
	src/ntf/ntfd/NtfSubscription.cc \
	src/ntf/ntfd/NtfLogger.cc \
	src/ntf/ntfd/NtfReader.cc \
	src/ntf/ntfd/NtfClient.cc \
	src/ntf/ntfd/NtfAdmin.cc

bin_osafntfd_LDADD = \
	lib/libntf_common.la \
	lib/libosaf_common.la \
	lib/libSaAmf.la \
	lib/libSaLog.la \
	lib/libSaClm.la \
	lib/libopensaf_core.la

if ENABLE_NTFIMCN

osaf_execbin_PROGRAMS += bin/osafntfimcnd

bin_osafntfd_SOURCES += src/ntf/ntfd/ntfs_imcnutil.c

bin_osafntfimcnd_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafntfimcnd_SOURCES = \
	src/ntf/ntfimcnd/ntfimcn_main.c \
	src/ntf/ntfimcnd/ntfimcn_notifier.c \
	src/ntf/ntfimcnd/ntfimcn_imm.c

bin_osafntfimcnd_LDADD = \
	lib/libntf_common.la \
	lib/libosaf_common.la \
	lib/libSaAmf.la \
	lib/libSaNtf.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

endif

bin_ntfread_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_ntfread_SOURCES = \
	src/ntf/tools/ntfread.c

bin_ntfread_LDADD = \
	lib/libntfclient.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la

bin_ntfsend_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_ntfsend_SOURCES = \
	src/ntf/tools/ntfsend.c

bin_ntfsend_LDADD = \
	lib/libntfclient.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la

bin_ntfsubscribe_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_ntfsubscribe_SOURCES = \
	src/ntf/tools/ntfsubscribe.c

bin_ntfsubscribe_LDADD = \
	lib/libntfclient.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

bin_PROGRAMS += bin/ntftest

dist_bin_SCRIPTS += src/ntf/apitest/ntf_search_criteria_test.sh

noinst_HEADERS += \
	src/ntf/apitest/sa_error.h \
	src/ntf/apitest/test_ntf_imcn.h \
	src/ntf/apitest/tet_ntf.h \
	src/ntf/apitest/tet_ntf_common.h

bin_ntftest_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_ntftest_SOURCES = \
	src/ntf/apitest/tet_ntf_main.c \
	src/ntf/apitest/sa_error.c \
	src/ntf/apitest/tet_ntf_common.c \
	src/ntf/apitest/tet_ntf_api_wrapper.c \
	src/ntf/apitest/tet_saNtfInitialize.c \
	src/ntf/apitest/tet_saNtfSelectionObjectGet.c \
	src/ntf/apitest/tet_saNtfDispatch.c \
	src/ntf/apitest/tet_saNtfFinalize.c \
	src/ntf/apitest/tet_saNtfNotificationCallbackT.c \
	src/ntf/apitest/tet_saNtfNotificationDiscardedCallbackT.c \
	src/ntf/apitest/tet_saNtfMiscellaneousNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfAlarmNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfAttributeChangeNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfStateChangeNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfSecurityAlarmNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfNotificationSend.c \
	src/ntf/apitest/tet_saNtfNotificationSubscribe.c \
	src/ntf/apitest/tet_saNtfStateChangeNotificationFilterAllocate.c \
	src/ntf/apitest/tet_saNtfSecurityAlarmNotificationFilterAllocate.c \
	src/ntf/apitest/tet_saNtfObjectCreateDeleteNotificationFilterAllocate.c \
	src/ntf/apitest/tet_saNtfAttributeChangeNotificationFilterAllocate.c \
	src/ntf/apitest/tet_saNtfAlarmNotificationFilterAllocate.c \
	src/ntf/apitest/tet_saNtfNotificationUnsubscribe.c \
	src/ntf/apitest/tet_saNtfObjectCreateDeleteNotificationAllocate.c \
	src/ntf/apitest/tet_saNtfNotificationReadInitialize.c \
	src/ntf/apitest/tet_saNtfNotificationReadFinalize.c \
	src/ntf/apitest/tet_saNtfNotificationReadNext.c \
	src/ntf/apitest/tet_NotificationContentVerification.c \
	src/ntf/apitest/tet_saNtfArrayValAllocate.c \
	src/ntf/apitest/tet_saNtfPtrValAllocate.c \
	src/ntf/apitest/tet_saNtfArrayValGet.c \
	src/ntf/apitest/tet_saNtfPtrValGet.c \
	src/ntf/apitest/test_ntfFilterVerification.c \
	src/ntf/apitest/tet_longDnObject_notification.c \
	src/ntf/apitest/tet_scOutage_reinitializeHandle.c \
	src/ntf/apitest/tet_ntf_clm.c

bin_ntftest_LDADD = \
	lib/libapitest.la

if ENABLE_NTFIMCN

bin_ntftest_SOURCES += src/ntf/apitest/test_ntf_imcn.c
bin_ntftest_LDADD += lib/libosaf_common.la

endif

bin_ntftest_LDADD += \
	lib/libSaNtf.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

endif
