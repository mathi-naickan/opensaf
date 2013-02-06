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

bin_PROGRAMS = immomtest immpopulate

immomtest_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/tests/immsv/common \
	-I$(top_srcdir)/tests/unit_test_fw/inc

immomtest_SOURCES = \
	$(top_srcdir)/tests/unit_test_fw/src/utest.c \
	$(top_srcdir)/tests/unit_test_fw/src/util.c \
	$(top_srcdir)/tests/immsv/common/immtest.c \
	test_saImmOmInitialize.c \
	test_saImmOmSelectionObjectGet.c \
	test_saImmOmDispatch.c \
	test_saImmOmFinalize.c \
	test_saImmOmAccessorInitialize.c \
	test_saImmOmAccessorGet_2.c \
	test_saImmOmAccessorFinalize.c \
	test_saImmOmClassCreate_2.c \
	test_saImmOmClassDescriptionGet_2.c \
	test_saImmOmClassDescriptionMemoryFree_2.c \
	test_saImmOmClassDelete.c \
	test_saImmOmSearchInitialize_2.c \
	test_saImmOmSearchNext_2.c \
	test_saImmOmSearchFinalize.c \
	test_saImmOmAdminOwnerInitialize.c \
	test_saImmOmAdminOwnerSet.c \
	test_saImmOmAdminOwnerRelease.c \
	test_saImmOmAdminOwnerFinalize.c \
	test_saImmOmAdminOwnerClear.c \
	test_saImmOmCcbInitialize.c \
	test_saImmOmCcbObjectCreate_2.c \
	test_saImmOmCcbObjectDelete.c \
	test_saImmOmCcbObjectModify_2.c \
	test_saImmOmCcbApply.c \
	test_saImmOmCcbFinalize.c \
	test_saImmOmAdminOperationContinue.c

immomtest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la


immpopulate_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/tools/safimm/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

immpopulate_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	$(top_srcdir)/osaf/tools/safimm/src/saf_error.c \
	populate.c

immpopulate_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la


