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
	src/plm/plmsv_objects.xml.atca \
	src/plm/plmsv_objects.xml.hp \
	src/plm/plmsv_objects.xml.pc \
	src/plm/plmcd/README \
	src/plm/plmcd/test_lib.c \
	src/plm/saf/libSaPlm.map

if ENABLE_AIS_PLM

lib_LTLIBRARIES += lib/libplmc_utils.la

lib_libplmc_utils_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libplmc_utils_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libplmc_utils_la_CPPFLAGS = \
	-DNCS_PLMSV_LOG=1 \
	$(AM_CPPFLAGS)

lib_libplmc_utils_la_SOURCES = \
	src/plm/plmcd/plmc_cmd_string_to_enum.c \
	src/plm/plmcd/plmc_get_listening_ip_addr.c \
	src/plm/plmcd/plmc_read_config.c

noinst_LTLIBRARIES += lib/libplm_common.la

lib_libplm_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS) @HPI_CFLAGS@

lib_libplm_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS) @HPI_CFLAGS@

lib_libplm_common_la_CPPFLAGS = \
	-DNCS_PLMSV_LOG=1 \
	$(AM_CPPFLAGS)

lib_libplm_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_libplm_common_la_SOURCES = \
	src/plm/plms_edu.c \
	src/plm/plms_common_mds.c \
	src/plm/plms_common_utils.c

noinst_LTLIBRARIES += lib/libplmc.la

lib_libplmc_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libplmc_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libplmc_la_CPPFLAGS = \
	-DNCS_PLMSV_LOG=1 \
	$(AM_CPPFLAGS)

lib_libplmc_la_LDFLAGS = $(AM_LDFLAGS) -static

lib_libplmc_la_SOURCES = \
	src/plm/plmcd/plmc_lib.c \
	src/plm/plmcd/plmc_lib_internal.c

lib_libplmc_la_LIBADD = \
	lib/libplmc_utils.la

pkglib_LTLIBRARIES += lib/libplms_hpi.la

lib_libplms_hpi_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_libplms_hpi_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS) @HPI_CFLAGS@

lib_libplms_hpi_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS) @HPI_CFLAGS@

lib_libplms_hpi_la_SOURCES = \
	src/plm/plms_hsm.c \
	src/plm/plms_hrb.c \
	src/plm/plms_hrb_mds.c \
	src/plm/plms_epath_util.c

lib_libplms_hpi_la_LIBADD = \
	lib/libplmc.la \
	lib/libplmc_utils.la

lib_LTLIBRARIES += lib/libSaPlm.la

lib_libSaPlm_la_SOURCES = \
	src/plm/agent/plma_init.c \
	src/plm/agent/plma_api.c \
	src/plm/agent/plma_comm.c \
	src/plm/agent/plma_mds.c

lib_libSaPlm_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS) @HPI_CFLAGS@

lib_libSaPlm_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS) @HPI_CFLAGS@

lib_libSaPlm_la_CPPFLAGS = \
	-DNCS_PLMA=1 -DNCS_PLMSV_LOG=1 \
	$(AM_CPPFLAGS)

lib_libSaPlm_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaPlm_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/plm/saf/libSaPlm.map
endif

lib_libSaPlm_la_LDFLAGS += \
	-version-number 0:0:0 -lstdc++

lib_libSaPlm_la_LIBADD = \
	lib/libplm_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/plm/saf/saPlm.h

noinst_HEADERS += \
	src/plm/agent/plma.h \
	src/plm/plmcd/plmc.h \
	src/plm/plmcd/plmc_cmds.h \
	src/plm/plmcd/plmc_lib.h \
	src/plm/plmcd/plmc_lib_internal.h \
	src/plm/plms.h \
	src/plm/plms_evt.h \
	src/plm/plms_hpi.h \
	src/plm/plms_hrb.h \
	src/plm/plms_hsm.h \
	src/plm/plms_mbcsv.h \
	src/plm/plms_notifications.h \
	src/plm/plms_utils.h \
	src/plm/plms_virt.h

osaf_execbin_PROGRAMS += bin/osafplmd
CORE_INCLUDES += -I$(top_srcdir)/src/plm/saf
pkgconfig_DATA += src/plm/saf/opensaf-plm.pc

nodist_pkgclccli_SCRIPTS += \
	src/plm/plmd/osaf-plmd

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/plm/plmsv_classes.xml \
	src/plm/plmsv_objects.xml \
	src/plm/plmsv_pl_template.xml \
	src/plm/plmsv_sc_template.xml

endif

dist_pkgsysconf_DATA += \
	src/plm/plmd/plmd.conf

bin_osafplmd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osafplmd_CFLAGS = $(AM_CFLAGS) @HPI_CFLAGS@

bin_osafplmd_CXXFLAGS = $(AM_CXXFLAGS) @HPI_CFLAGS@

bin_osafplmd_SOURCES = \
	src/plm/plmd/plms_main.c \
	src/plm/plmd/plms_amf.c \
	src/plm/plmd/plms_mds.c \
	src/plm/plmd/plms_notifications.c \
	src/plm/plmd/plms_mbcsv.c \
	src/plm/plmd/plms_imm.c \
	src/plm/plmd/plms_he_pres_fsm.c \
	src/plm/plmd/plms_adm_fsm.c \
	src/plm/plmd/plms_utils.c \
	src/plm/plmd/plms_plmc.c \
	src/plm/plmd/plms_proc.c \
	src/plm/plmd/plms_stdby.c \
	src/plm/plmd/plms_dbg_utils.c \
	src/plm/plmd/plms_virt.cc

bin_osafplmd_LDADD = \
	@HPI_LIBS@ \
	@LIBVIRT_LIBS@ \
	lib/libosaf_common.la \
	lib/libopensaf_core.la \
	lib/libplmc.la \
	lib/libplm_common.la \
	lib/libSaAmf.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libSaNtf.la

if ENABLE_TESTS

noinst_HEADERS += \
	src/plm/apitest/plmtest.h

noinst_LTLIBRARIES += lib/libplmtest.la

lib_libplmtest_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libplmtest_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libplmtest_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_libplmtest_la_SOURCES = \
	src/plm/apitest/plmtest.c

bin_PROGRAMS += bin/plmtest

bin_plmtest_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_plmtest_SOURCES = \
	src/plm/apitest/test_saPlmInitialize.c  \
	src/plm/apitest/test_saPlmSelectionObjectGet.c \
	src/plm/apitest/test_saPlmDispatch.c  \
	src/plm/apitest/test_saPlmFinalize.c \
	src/plm/apitest/test_saPlmEntityGroupCreate.c \
	src/plm/apitest/test_saPlmEntityGroupAdd.c \
	src/plm/apitest/test_saPlmEntityGroupDelete.c \
	src/plm/apitest/test_saPlmEntityGroupRemove.c \
	src/plm/apitest/test_saPlmReadinessTrack.c

bin_plmtest_LDADD = \
	lib/libSaPlm.la \
	lib/libopensaf_core.la \
	lib/libapitest.la \
	lib/libplmtest.la

endif

dist_sysconf_DATA += \
	src/plm/plmcd/plmcd.conf

sbin_PROGRAMS += bin/plmcd

bin_plmcd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_plmcd_CFLAGS = $(AM_CFLAGS)

bin_plmcd_SOURCES = \
	src/plm/plmcd/plmcd.c

bin_plmcd_LDFLAGS = $(AM_LDFLAGS) -lpthread

bin_plmcd_LDADD = \
	lib/libplmc_utils.la

if ENABLE_SYSTEMD
dist_systemd_DATA += \
	src/plm/plmcd/plmcboot.service \
	src/plm/plmcd/plmcd.service

bin_plmcd_LDFLAGS += -lsystemd-daemon
endif

endif
