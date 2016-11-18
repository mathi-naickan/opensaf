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

EXTRA_DIST += src/lck/saf/libSaLck.map

if ENABLE_AIS_LCK

noinst_LTLIBRARIES += lib/liblck_common.la

lib_liblck_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_liblck_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_liblck_common_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_liblck_common_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

lib_liblck_common_la_SOURCES = \
	src/lck/glsv_edu.c

lib_LTLIBRARIES += lib/libSaLck.la

lib_libSaLck_la_SOURCES = \
	src/lck/agent/gla_api.c \
	src/lck/agent/gla_clbk.c \
	src/lck/agent/gla_init.c \
	src/lck/agent/gla_mds.c \
	src/lck/agent/gla_queue.c \
	src/lck/agent/gla_tmr.c

lib_libSaLck_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaLck_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaLck_la_CPPFLAGS = \
	-DNCS_GLA=1 \
	$(AM_CPPFLAGS)

lib_libSaLck_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaLck_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/lck/saf/libSaLck.map
endif

lib_libSaLck_la_LDFLAGS += \
	-version-number 0:0:0 -lstdc++

lib_libSaLck_la_LIBADD = \
	lib/liblck_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/lck/saf/saLck.h

noinst_HEADERS += \
	src/lck/agent/gla.h \
	src/lck/agent/gla_callbk.h \
	src/lck/agent/gla_cb.h \
	src/lck/agent/gla_dl_api.h \
	src/lck/agent/gla_evt.h \
	src/lck/agent/gla_mds.h \
	src/lck/agent/gla_mem.h \
	src/lck/glsv_defs.h \
	src/lck/glsv_lck.h \
	src/lck/glsv_mem.h \
	src/lck/lckd/gld.h \
	src/lck/lckd/gld_amf.h \
	src/lck/lckd/gld_api.h \
	src/lck/lckd/gld_cb.h \
	src/lck/lckd/gld_dl_api.h \
	src/lck/lckd/gld_edu.h \
	src/lck/lckd/gld_evt.h \
	src/lck/lckd/gld_imm.h \
	src/lck/lckd/gld_mbcsv.h \
	src/lck/lckd/gld_mds.h \
	src/lck/lckd/gld_mem.h \
	src/lck/lckd/gld_tmr.h \
	src/lck/lcknd/glnd.h \
	src/lck/lcknd/glnd_api.h \
	src/lck/lcknd/glnd_cb.h \
	src/lck/lcknd/glnd_client.h \
	src/lck/lcknd/glnd_dl_api.h \
	src/lck/lcknd/glnd_edu.h \
	src/lck/lcknd/glnd_evt.h \
	src/lck/lcknd/glnd_mds.h \
	src/lck/lcknd/glnd_mem.h \
	src/lck/lcknd/glnd_res.h \
	src/lck/lcknd/glnd_restart.h \
	src/lck/lcknd/glnd_tmr.h

osaf_execbin_PROGRAMS += bin/osaflckd bin/osaflcknd
CORE_INCLUDES += -I$(top_srcdir)/src/lck/saf
pkgconfig_DATA += src/lck/saf/opensaf-lck.pc

nodist_pkgclccli_SCRIPTS += \
	src/lck/lckd/osaf-lckd \
	src/lck/lcknd/osaf-lcknd

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/lck/glsv_sc_template.xml \
	src/lck/glsv_pl_template.xml \
	src/lck/glsv_classes.xml \
	src/lck/glsv_objects.xml

endif

dist_pkgsysconf_DATA += \
	src/lck/lckd/lckd.conf \
	src/lck/lcknd/lcknd.conf

bin_osaflcknd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_GLND=1 \
	$(AM_CPPFLAGS)

bin_osaflcknd_SOURCES = \
	src/lck/lcknd/glnd_agent.c \
	src/lck/lcknd/glnd_amf.c \
	src/lck/lcknd/glnd_api.c \
	src/lck/lcknd/glnd_cb.c \
	src/lck/lcknd/glnd_ckpt.c \
	src/lck/lcknd/glnd_client.c \
	src/lck/lcknd/glnd_evt.c \
	src/lck/lcknd/glnd_main.c \
	src/lck/lcknd/glnd_mds.c \
	src/lck/lcknd/glnd_queue.c \
	src/lck/lcknd/glnd_res.c \
	src/lck/lcknd/glnd_res_req.c \
	src/lck/lcknd/glnd_restart.c \
	src/lck/lcknd/glnd_shm.c \
	src/lck/lcknd/glnd_tmr.c

bin_osaflcknd_LDADD = \
	lib/liblck_common.la \
	lib/libSaAmf.la \
	lib/libopensaf_core.la

bin_osaflckd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_GLD=1 \
	$(AM_CPPFLAGS)

bin_osaflckd_SOURCES = \
	src/lck/lckd/gld_amf.c \
	src/lck/lckd/gld_api.c \
	src/lck/lckd/gld_evt.c \
	src/lck/lckd/gld_imm.c \
	src/lck/lckd/gld_main.c \
	src/lck/lckd/gld_mbcsv.c \
	src/lck/lckd/gld_mds.c \
	src/lck/lckd/gld_red.c \
	src/lck/lckd/gld_rsc.c \
	src/lck/lckd/gld_standby.c \
	src/lck/lckd/gld_tmr.c

bin_osaflckd_LDADD = \
	lib/liblck_common.la \
	lib/libSaAmf.la \
	lib/libosaf_common.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

if ENABLE_TESTS

noinst_HEADERS += \
	src/lck/apitest/tet_gla_conf.h \
	src/lck/apitest/tet_glsv.h

endif

endif
