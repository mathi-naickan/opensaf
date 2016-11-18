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

EXTRA_DIST += \
	src/imm/README.2PBE \
	src/imm/README.ACCESS_CONTROL \
	src/imm/README.NO_DANGLING \
	src/imm/README.RESOURCE_DISPLAY \
	src/imm/README.SASTRINGT_API \
	src/imm/README.SC_ABSENCE

noinst_LTLIBRARIES += lib/libimm_common.la

lib_libimm_common_la_CFLAGS = \
	$(OSAF_LIB_FLAGS) $(AM_CFLAGS) @SQLITE3_CFLAGS@

lib_libimm_common_la_CXXFLAGS = \
	$(OSAF_LIB_FLAGS) $(AM_CXXFLAGS) -Wformat=1 @SQLITE3_CFLAGS@

lib_libimm_common_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libimm_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static @SQLITE3_LIBS@

lib_libimm_common_la_SOURCES = \
	src/imm/immpbe_dump.cc \
	src/imm/immsv_evt.c

lib_LTLIBRARIES += lib/libSaImmOm.la lib/libSaImmOi.la

lib_libSaImmOm_la_SOURCES = \
	src/imm/agent/imma_om_api.c \
	src/imm/agent/imma_db.c \
	src/imm/agent/imma_init.c \
	src/imm/agent/imma_mds.c \
	src/imm/agent/imma_proc.c

lib_libSaImmOm_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaImmOm_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaImmOm_la_CPPFLAGS = \
	-DIMMA_OM -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libSaImmOm_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaImmOm_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/imm/saf/libSaImmOm.map
endif

lib_libSaImmOm_la_LDFLAGS += \
	-version-number 0:4:0 -lstdc++

lib_libSaImmOm_la_LIBADD = \
	lib/libimm_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

lib_libSaImmOi_la_SOURCES = \
	src/imm/agent/imma_oi_api.c \
	src/imm/agent/imma_db.c \
	src/imm/agent/imma_init.c \
	src/imm/agent/imma_mds.c \
	src/imm/agent/imma_proc.c

lib_libSaImmOi_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaImmOi_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaImmOi_la_CPPFLAGS = \
	-DIMMA_OI -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libSaImmOi_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaImmOi_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/imm/saf/libSaImmOi.map
endif

lib_libSaImmOi_la_LDFLAGS += \
	-version-number 0:3:0 -lstdc++

lib_libSaImmOi_la_LIBADD = \
	lib/libimm_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/imm/saf/saImm.h \
	src/imm/saf/saImmOi_A_2_11.h \
	src/imm/saf/saImmOi_A_2_15.h \
	src/imm/saf/saImmOi.h \
	src/imm/saf/saImmOm_A_2_11.h \
	src/imm/saf/saImmOm_A_2_12.h \
	src/imm/saf/saImmOm_A_2_13.h \
	src/imm/saf/saImmOm_A_2_14.h \
	src/imm/saf/saImmOm_A_2_15.h \
	src/imm/saf/saImmOm_A_2_16.h \
	src/imm/saf/saImmOm_A_2_17.h \
	src/imm/saf/saImmOm.h

noinst_HEADERS += \
	src/imm/agent/imma.h \
	src/imm/agent/imma_cb.h \
	src/imm/agent/imma_def.h \
	src/imm/agent/imma_mds.h \
	src/imm/agent/imma_proc.h \
	src/imm/immd/immd.h \
	src/imm/immd/immd_cb.h \
	src/imm/immd/immd_mds.h \
	src/imm/immd/immd_proc.h \
	src/imm/immd/immd_red.h \
	src/imm/immd/immd_sbedu.h \
	src/imm/immloadd/imm_loader.h \
	src/imm/immnd/ImmAttrValue.h \
	src/imm/immnd/ImmModel.h \
	src/imm/immnd/ImmSearchOp.h \
	src/imm/immnd/immnd.h \
	src/imm/immnd/immnd_cb.h \
	src/imm/immnd/immnd_init.h \
	src/imm/immpbe_dump.h \
	src/imm/immpbed/immpbe.h \
	src/imm/immsv.h \
	src/imm/immsv_api.h \
	src/imm/immsv_evt.h \
	src/imm/immsv_evt_model.h \
	src/imm/tools/imm_dumper.h

bin_PROGRAMS += bin/immadm bin/immcfg bin/immdump bin/immfind bin/immlist
osaf_execbin_PROGRAMS += bin/osafimmd bin/osafimmloadd bin/osafimmnd bin/osafimmpbed
EXTRA_DIST += src/imm/saf/libSaImmOm.map src/imm/saf/libSaImmOi.map
CORE_INCLUDES += -I$(top_srcdir)/src/imm/saf
pkgconfig_DATA += src/imm/saf/opensaf-imm.pc

nodist_pkgclccli_SCRIPTS += \
	src/imm/immd/osaf-immd \
	src/imm/immnd/osaf-immnd

dist_pkgsysconf_DATA += \
	src/imm/immd/immd.conf \
	src/imm/immnd/immnd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/imm/immsv_sc_template.xml \
	src/imm/immsv_pl_template.xml \
	src/imm/immsv_classes.xml \
	src/imm/immsv_objects.xml \
	src/imm/imm_copyright.xml

dist_pkgimmxml_DATA = \
	src/imm/tools/README

dist_pkgimmxml_SCRIPTS = \
	src/imm/tools/baseimm.py \
	src/imm/tools/immxml-configure \
	src/imm/tools/immxml-modify-config \
	src/imm/tools/immxml-merge \
	src/imm/tools/immxml-nodegen \
	src/imm/tools/immxml-validate \
	src/imm/tools/immxml-clustersize \
	src/imm/tools/verify.sh

endif

bin_immadm_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_immadm_SOURCES = \
	src/imm/tools/imm_admin.c

bin_immadm_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la

bin_immcfg_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	@XML2_CFLAGS@

bin_immcfg_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS) \
	@XML2_CFLAGS@

bin_immcfg_SOURCES = \
	src/imm/tools/imm_import.cc \
	src/imm/tools/imm_cfg.c

bin_immcfg_LDFLAGS = \
	$(AM_LDFLAGS) \
	@XML2_LIBS@ \
	-ldl

bin_immcfg_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la

bin_immdump_CXXFLAGS = $(AM_CXXFLAGS) @XML2_CFLAGS@ @SQLITE3_CFLAGS@

bin_immdump_SOURCES = \
	src/imm/tools/imm_dumper.cc \
	src/imm/tools/imm_xmlw_dump.cc

bin_immdump_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_immdump_LDFLAGS = \
	$(AM_LDFLAGS) \
	@XML2_LIBS@

bin_immdump_LDADD = \
	lib/libimm_common.la \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la

bin_immfind_CXXFLAGS = \
	$(AM_CXXFLAGS)

bin_immfind_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_immfind_SOURCES = \
	src/imm/tools/imm_find.c

bin_immfind_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la

bin_immlist_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_immlist_SOURCES = \
	src/imm/tools/imm_list.c

bin_immlist_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la

bin_osafimmd_CPPFLAGS = \
	-DSA_CLM_B01=1 -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafimmd_SOURCES = \
	src/imm/immd/immd_amf.c \
	src/imm/immd/immd_db.c \
	src/imm/immd/immd_evt.c \
	src/imm/immd/immd_main.c \
	src/imm/immd/immd_mbcsv.c \
	src/imm/immd/immd_mds.c \
	src/imm/immd/immd_proc.c \
	src/imm/immd/immd_sbevt.c

bin_osafimmd_LDADD = \
	lib/libimm_common.la \
	lib/libSaAmf.la \
	lib/libopensaf_core.la

bin_osafimmloadd_CXXFLAGS = $(AM_CXXFLAGS) @XML2_CFLAGS@ @SQLITE3_CFLAGS@

bin_osafimmloadd_SOURCES = \
	src/imm/immloadd/imm_loader.cc \
	src/imm/immloadd/imm_pbe_load.cc

bin_osafimmloadd_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafimmloadd_LDFLAGS = \
	$(AM_LDFLAGS) \
	@XML2_LIBS@ @SQLITE3_LIBS@

bin_osafimmloadd_LDADD = \
	lib/libimm_common.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_osafimmnd_CXXFLAGS =$(AM_CXXFLAGS)

bin_osafimmnd_CPPFLAGS = \
	-DSA_CLM_B01=1 -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafimmnd_SOURCES = \
	src/imm/immnd/immnd_amf.c \
	src/imm/immnd/immnd_db.c \
	src/imm/immnd/immnd_evt.c \
	src/imm/immnd/immnd_main.c \
	src/imm/immnd/immnd_mds.c \
	src/imm/immnd/immnd_proc.c \
	src/imm/immnd/ImmAttrValue.cc \
	src/imm/immnd/ImmSearchOp.cc \
	src/imm/immnd/ImmModel.cc

bin_osafimmnd_LDADD = \
	lib/libimm_common.la \
	lib/libSaAmf.la \
	lib/libopensaf_core.la

bin_osafimmpbed_CXXFLAGS = $(AM_CXXFLAGS) @XML2_CFLAGS@ @SQLITE3_CFLAGS@

bin_osafimmpbed_SOURCES = \
	src/imm/immpbed/immpbe.cc \
	src/imm/immpbed/immpbe_daemon.cc

bin_osafimmpbed_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafimmpbed_LDFLAGS = \
	$(AM_LDFLAGS) \
	@XML2_LIBS@ @SQLITE3_LIBS@ -lpthread

bin_osafimmpbed_LDADD = \
	lib/libimm_common.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

noinst_HEADERS += \
	src/imm/apitest/immtest.h

noinst_LTLIBRARIES += lib/libimmtest.la

lib_libimmtest_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libimmtest_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libimmtest_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_libimmtest_la_SOURCES = \
	src/imm/apitest/immtest.c

bin_PROGRAMS += bin/immoitest bin/immapplier bin/immomtest bin/immpopulate

bin_immoitest_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_immoitest_SOURCES = \
	src/imm/apitest/implementer/test_saImmOiInitialize_2.c \
	src/imm/apitest/implementer/test_saImmOiSelectionObjectGet.c \
	src/imm/apitest/implementer/test_saImmOiDispatch.c \
	src/imm/apitest/implementer/test_saImmOiFinalize.c \
	src/imm/apitest/implementer/test_saImmOiImplementerSet.c \
	src/imm/apitest/implementer/test_saImmOiImplementerClear.c \
	src/imm/apitest/implementer/test_saImmOiClassImplementerSet.c \
	src/imm/apitest/implementer/test_saImmOiClassImplementerRelease.c \
	src/imm/apitest/implementer/test_saImmOiObjectImplementerSet.c \
	src/imm/apitest/implementer/test_saImmOiObjectImplementerRelease.c \
	src/imm/apitest/implementer/test_saImmOiRtObjectCreate_2.c \
	src/imm/apitest/implementer/test_saImmOiRtObjectDelete.c \
	src/imm/apitest/implementer/test_saImmOiRtObjectUpdate_2.c \
	src/imm/apitest/implementer/test_SaImmOiRtAttrUpdateCallbackT.c \
	src/imm/apitest/implementer/test_SaImmOiCcb.c \
	src/imm/apitest/implementer/test_SaImmOiAdminOperation.c \
	src/imm/apitest/implementer/test_saImmOiAugmentCcbInitialize.c \
	src/imm/apitest/implementer/test_saImmOiLongDn.c \
	src/imm/apitest/implementer/test_saImmOiSaStringT.c \
	src/imm/apitest/implementer/test_cleanup.c

bin_immoitest_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la \
	lib/libapitest.la \
	lib/libimmtest.la

bin_immapplier_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_immapplier_SOURCES = \
	src/imm/apitest/implementer/applier.c

bin_immapplier_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_immomtest_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_immomtest_SOURCES = \
	src/imm/apitest/management/test_saImmOmInitialize.c \
	src/imm/apitest/management/test_saImmOmSelectionObjectGet.c \
	src/imm/apitest/management/test_saImmOmDispatch.c \
	src/imm/apitest/management/test_saImmOmFinalize.c \
	src/imm/apitest/management/test_saImmOmAccessorInitialize.c \
	src/imm/apitest/management/test_saImmOmAccessorGet_2.c \
	src/imm/apitest/management/test_saImmOmAccessorFinalize.c \
	src/imm/apitest/management/test_saImmOmClassCreate_2.c \
	src/imm/apitest/management/test_saImmOmClassDescriptionGet_2.c \
	src/imm/apitest/management/test_saImmOmClassDescriptionMemoryFree_2.c \
	src/imm/apitest/management/test_saImmOmClassDelete.c \
	src/imm/apitest/management/test_saImmOmSearchInitialize_2.c \
	src/imm/apitest/management/test_saImmOmSearchNext_2.c \
	src/imm/apitest/management/test_saImmOmSearchFinalize.c \
	src/imm/apitest/management/test_saImmOmAdminOwnerInitialize.c \
	src/imm/apitest/management/test_saImmOmAdminOwnerSet.c \
	src/imm/apitest/management/test_saImmOmAdminOwnerRelease.c \
	src/imm/apitest/management/test_saImmOmAdminOwnerFinalize.c \
	src/imm/apitest/management/test_saImmOmAdminOwnerClear.c \
	src/imm/apitest/management/test_saImmOmCcbInitialize.c \
	src/imm/apitest/management/test_saImmOmCcbObjectCreate_2.c \
	src/imm/apitest/management/test_saImmOmCcbObjectDelete.c \
	src/imm/apitest/management/test_saImmOmCcbObjectModify_2.c \
	src/imm/apitest/management/test_saImmOmCcbApply.c \
	src/imm/apitest/management/test_saImmOmCcbFinalize.c \
	src/imm/apitest/management/test_saImmOmAdminOperationContinue.c \
	src/imm/apitest/management/test_saImmOmThreadInterference.c \
	src/imm/apitest/management/test_saImmOmLongDn.c \
	src/imm/apitest/management/test_saImmOmSaStringT.c \
	src/imm/apitest/management/test_cleanup.c

bin_immomtest_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOm.la \
	lib/libSaImmOi.la \
	lib/libopensaf_core.la \
	lib/libapitest.la \
	lib/libimmtest.la

bin_immpopulate_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_immpopulate_SOURCES = \
	src/imm/apitest/management/populate.c

bin_immpopulate_LDADD = \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

endif
