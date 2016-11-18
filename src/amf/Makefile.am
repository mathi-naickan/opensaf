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

noinst_LTLIBRARIES += lib/libamf_common.la

lib_libamf_common_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libamf_common_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libamf_common_la_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

lib_libamf_common_la_LDFLAGS =
	$(AM_LDFLAGS) \
	-static

lib_libamf_common_la_SOURCES = \
	src/amf/d2nedu.c \
	src/amf/d2nmsg.c \
	src/amf/eduutil.c \
	src/amf/n2avaedu.c \
	src/amf/n2avamsg.c \
	src/amf/nd2ndedu.c \
	src/amf/util.c

lib_LTLIBRARIES += lib/libSaAmf.la

lib_libSaAmf_la_SOURCES = \
	src/amf/agent/amf_agent.cc \
	src/amf/agent/ava_hdl.cc \
	src/amf/agent/ava_init.cc \
	src/amf/agent/ava_mds.cc \
	src/amf/agent/ava_op.cc

lib_libSaAmf_la_CFLAGS = $(OSAF_LIB_FLAGS) $(AM_CFLAGS)

lib_libSaAmf_la_CXXFLAGS = $(OSAF_LIB_FLAGS) $(AM_CXXFLAGS)

lib_libSaAmf_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

lib_libSaAmf_la_LDFLAGS = \
	$(AM_LDFLAGS)

if HAVE_LD_VERSION_SCRIPT
lib_libSaAmf_la_LDFLAGS += \
	-Wl,-version-script=$(top_srcdir)/src/amf/saf/libSaAmf.map
endif

lib_libSaAmf_la_LDFLAGS += \
	-version-number 0:2:1 -lstdc++

lib_libSaAmf_la_LIBADD = \
	lib/libamf_common.la \
	lib/libais.la \
	lib/libopensaf_core.la

include_HEADERS += \
	src/amf/saf/saAmf.h \
	src/amf/saf/saAmf_B_04_02.h

noinst_HEADERS += \
	src/amf/agent/amf_agent.h \
	src/amf/agent/ava.h \
	src/amf/agent/ava_cb.h \
	src/amf/agent/ava_def.h \
	src/amf/agent/ava_dl_api.h \
	src/amf/agent/ava_hdl.h \
	src/amf/agent/ava_mds.h \
	src/amf/amf.h \
	src/amf/amf_amfparam.h \
	src/amf/amf_d2nedu.h \
	src/amf/amf_d2nmsg.h \
	src/amf/amf_db_template.h \
	src/amf/amf_defs.h \
	src/amf/amf_eduutil.h \
	src/amf/amf_n2avaedu.h \
	src/amf/amf_n2avamsg.h \
	src/amf/amf_nd2ndmsg.h \
	src/amf/amf_si_assign.h \
	src/amf/amf_util.h \
	src/amf/amfd/amfd.h \
	src/amf/amfd/app.h \
	src/amf/amfd/apptype.h \
	src/amf/amfd/cb.h \
	src/amf/amfd/ckpt.h \
	src/amf/amfd/ckpt_edu.h \
	src/amf/amfd/ckpt_msg.h \
	src/amf/amfd/ckpt_updt.h \
	src/amf/amfd/clm.h \
	src/amf/amfd/cluster.h \
	src/amf/amfd/comp.h \
	src/amf/amfd/csi.h \
	src/amf/amfd/db_template.h \
	src/amf/amfd/def.h \
	src/amf/amfd/evt.h \
	src/amf/amfd/hlt.h \
	src/amf/amfd/imm.h \
	src/amf/amfd/mds.h \
	src/amf/amfd/msg.h \
	src/amf/amfd/node.h \
	src/amf/amfd/ntf.h \
	src/amf/amfd/pg.h \
	src/amf/amfd/proc.h \
	src/amf/amfd/role.h \
	src/amf/amfd/sg.h \
	src/amf/amfd/sgtype.h \
	src/amf/amfd/si.h \
	src/amf/amfd/si_dep.h \
	src/amf/amfd/sirankedsu.h \
	src/amf/amfd/su.h \
	src/amf/amfd/susi.h \
	src/amf/amfd/sutcomptype.h \
	src/amf/amfd/sutype.h \
	src/amf/amfd/svctype.h \
	src/amf/amfd/svctypecstype.h \
	src/amf/amfd/timer.h \
	src/amf/amfd/util.h \
	src/amf/amfnd/avnd.h \
	src/amf/amfnd/avnd_cb.h \
	src/amf/amfnd/avnd_clm.h \
	src/amf/amfnd/avnd_comp.h \
	src/amf/amfnd/avnd_def.h \
	src/amf/amfnd/avnd_defs.h \
	src/amf/amfnd/avnd_di.h \
	src/amf/amfnd/avnd_dl_api.h \
	src/amf/amfnd/avnd_err.h \
	src/amf/amfnd/avnd_evt.h \
	src/amf/amfnd/avnd_hc.h \
	src/amf/amfnd/avnd_mds.h \
	src/amf/amfnd/avnd_mon.h \
	src/amf/amfnd/avnd_pg.h \
	src/amf/amfnd/avnd_proc.h \
	src/amf/amfnd/avnd_proxy.h \
	src/amf/amfnd/avnd_su.h \
	src/amf/amfnd/avnd_tmr.h \
	src/amf/amfnd/avnd_util.h \
	src/amf/amfnd/imm.h

sbin_PROGRAMS += bin/amfpm
osaf_execbin_PROGRAMS += bin/osafamfd bin/osafamfnd bin/osafamfwd
EXTRA_DIST += src/amf/saf/libSaAmf.map
CORE_INCLUDES += -I$(top_srcdir)/src/amf/saf
TESTS += bin/testamfd
pkgconfig_DATA += src/amf/saf/opensaf-amf.pc

nodist_pkgclccli_SCRIPTS += \
	src/amf/amfnd/osaf-amfnd \
	src/amf/amfwd/osaf-amfwd \
	src/amf/amfd/osaf-amfd

dist_bin_SCRIPTS += \
	src/amf/tools/amf-adm \
	src/amf/tools/amf-find \
	src/amf/tools/amf-state

dist_pkgsysconf_DATA += \
	src/amf/amfnd/amfnd.conf \
	src/amf/amfwd/amfwdog.conf \
	src/amf/amfd/amfd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/amf/common_pl_template.xml \
	src/amf/common_sc_template.xml \
	src/amf/amf_classes.xml \
	src/amf/amf_objects.xml \
	src/amf/amfwdog_sc_template.xml \
	src/amf/amfwdog_pl_template.xml \
	src/amf/amfwdog_objects.xml

endif

bin_testamfd_CXXFLAGS =$(AM_CXXFLAGS)

bin_testamfd_CPPFLAGS = \
	-DSA_CLM_B01=1 -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS) \
	-I$(GTEST_DIR)/include

bin_testamfd_LDFLAGS = \
	$(AM_LDFLAGS) \
	-lpthread \
	src/amf/amfd/bin_osafamfd-app.o \
	src/amf/amfd/bin_osafamfd-apptype.o \
	src/amf/amfd/bin_osafamfd-chkop.o \
	src/amf/amfd/bin_osafamfd-ckpt_dec.o \
	src/amf/amfd/bin_osafamfd-ckpt_edu.o \
	src/amf/amfd/bin_osafamfd-ckpt_enc.o \
	src/amf/amfd/bin_osafamfd-ckpt_updt.o \
	src/amf/amfd/bin_osafamfd-clm.o \
	src/amf/amfd/bin_osafamfd-cluster.o \
	src/amf/amfd/bin_osafamfd-comp.o \
	src/amf/amfd/bin_osafamfd-compcstype.o \
	src/amf/amfd/bin_osafamfd-comptype.o \
	src/amf/amfd/bin_osafamfd-csi.o \
	src/amf/amfd/bin_osafamfd-csiattr.o \
	src/amf/amfd/bin_osafamfd-cstype.o \
	src/amf/amfd/bin_osafamfd-ctcstype.o \
	src/amf/amfd/bin_osafamfd-dmsg.o \
	src/amf/amfd/bin_osafamfd-hlt.o \
	src/amf/amfd/bin_osafamfd-hlttype.o \
	src/amf/amfd/bin_osafamfd-imm.o \
	src/amf/amfd/bin_osafamfd-mds.o \
	src/amf/amfd/bin_osafamfd-ndfsm.o \
	src/amf/amfd/bin_osafamfd-ndmsg.o \
	src/amf/amfd/bin_osafamfd-ndproc.o \
	src/amf/amfd/bin_osafamfd-node.o \
	src/amf/amfd/bin_osafamfd-nodegroup.o \
	src/amf/amfd/bin_osafamfd-nodeswbundle.o \
	src/amf/amfd/bin_osafamfd-ntf.o \
	src/amf/amfd/bin_osafamfd-pg.o \
	src/amf/amfd/bin_osafamfd-role.o \
	src/amf/amfd/bin_osafamfd-sg.o \
	src/amf/amfd/bin_osafamfd-sg_2n_fsm.o \
	src/amf/amfd/bin_osafamfd-sg_nored_fsm.o \
	src/amf/amfd/bin_osafamfd-sg_npm_fsm.o \
	src/amf/amfd/bin_osafamfd-sg_nway_fsm.o \
	src/amf/amfd/bin_osafamfd-sg_nwayact_fsm.o \
	src/amf/amfd/bin_osafamfd-sgproc.o \
	src/amf/amfd/bin_osafamfd-sgtype.o \
	src/amf/amfd/bin_osafamfd-si.o \
	src/amf/amfd/bin_osafamfd-si_dep.o \
	src/amf/amfd/bin_osafamfd-siass.o \
	src/amf/amfd/bin_osafamfd-sirankedsu.o \
	src/amf/amfd/bin_osafamfd-su.o \
	src/amf/amfd/bin_osafamfd-sutcomptype.o \
	src/amf/amfd/bin_osafamfd-sutype.o \
	src/amf/amfd/bin_osafamfd-svctype.o \
	src/amf/amfd/bin_osafamfd-svctypecstypes.o \
	src/amf/amfd/bin_osafamfd-timer.o \
	src/amf/amfd/bin_osafamfd-util.o

bin_testamfd_SOURCES = \
	src/amf/amfd/tests/test_amfdb.cc \
	src/amf/amfd/tests/test_ckpt_enc_dec.cc

bin_testamfd_LDADD = \
	lib/libamf_common.la \
	lib/libosaf_common.la \
	lib/libosaf_common.la \
	lib/libSaClm.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libSaLog.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la

bin_amfpm_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_amfpm_SOURCES = \
	src/amf/tools/amf_pm.c

bin_amfpm_LDADD = \
	lib/libSaAmf.la

bin_osafamfd_CXXFLAGS = -fno-strict-aliasing $(AM_CXXFLAGS)

bin_osafamfd_CPPFLAGS = \
	-DSA_CLM_B01=1 -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafamfd_SOURCES = \
	src/amf/amfd/app.cc \
	src/amf/amfd/apptype.cc \
	src/amf/amfd/chkop.cc \
	src/amf/amfd/ckpt_dec.cc \
	src/amf/amfd/ckpt_edu.cc \
	src/amf/amfd/ckpt_enc.cc \
	src/amf/amfd/ckpt_updt.cc \
	src/amf/amfd/clm.cc \
	src/amf/amfd/cluster.cc \
	src/amf/amfd/comp.cc \
	src/amf/amfd/compcstype.cc \
	src/amf/amfd/comptype.cc \
	src/amf/amfd/csi.cc \
	src/amf/amfd/csiattr.cc \
	src/amf/amfd/cstype.cc \
	src/amf/amfd/ctcstype.cc \
	src/amf/amfd/dmsg.cc \
	src/amf/amfd/hlt.cc \
	src/amf/amfd/hlttype.cc \
	src/amf/amfd/imm.cc \
	src/amf/amfd/main.cc \
	src/amf/amfd/mds.cc \
	src/amf/amfd/ndfsm.cc \
	src/amf/amfd/ndmsg.cc \
	src/amf/amfd/ndproc.cc \
	src/amf/amfd/node.cc \
	src/amf/amfd/nodegroup.cc \
	src/amf/amfd/nodeswbundle.cc \
	src/amf/amfd/ntf.cc \
	src/amf/amfd/pg.cc \
	src/amf/amfd/role.cc \
	src/amf/amfd/sg.cc \
	src/amf/amfd/sg_2n_fsm.cc \
	src/amf/amfd/sg_nored_fsm.cc \
	src/amf/amfd/sg_npm_fsm.cc \
	src/amf/amfd/sg_nway_fsm.cc \
	src/amf/amfd/sg_nwayact_fsm.cc \
	src/amf/amfd/sgproc.cc \
	src/amf/amfd/sgtype.cc \
	src/amf/amfd/si.cc \
	src/amf/amfd/si_dep.cc \
	src/amf/amfd/siass.cc \
	src/amf/amfd/sirankedsu.cc \
	src/amf/amfd/su.cc \
	src/amf/amfd/sutcomptype.cc \
	src/amf/amfd/sutype.cc \
	src/amf/amfd/svctype.cc \
	src/amf/amfd/svctypecstypes.cc \
	src/amf/amfd/timer.cc \
	src/amf/amfd/util.cc

bin_osafamfd_LDADD = \
	lib/libamf_common.la \
	lib/libosaf_common.la \
	lib/libosaf_common.la \
	lib/libSaClm.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libSaLog.la \
	lib/libSaNtf.la \
	lib/libopensaf_core.la

bin_osafamfnd_CXXFLAGS = -fno-strict-aliasing $(AM_CXXFLAGS)

bin_osafamfnd_CPPFLAGS = \
	-DSA_CLM_B01=1 -DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafamfnd_SOURCES = \
	src/amf/amfnd/amfnd.cc \
	src/amf/amfnd/cam.cc \
	src/amf/amfnd/cbq.cc \
	src/amf/amfnd/chc.cc \
	src/amf/amfnd/clc.cc \
	src/amf/amfnd/clm.cc \
	src/amf/amfnd/comp.cc \
	src/amf/amfnd/compdb.cc \
	src/amf/amfnd/cpm.cc \
	src/amf/amfnd/di.cc \
	src/amf/amfnd/err.cc \
	src/amf/amfnd/evt.cc \
	src/amf/amfnd/hcdb.cc \
	src/amf/amfnd/imm.cc \
	src/amf/amfnd/main.cc \
	src/amf/amfnd/mds.cc \
	src/amf/amfnd/mon.cc \
	src/amf/amfnd/pg.cc \
	src/amf/amfnd/pgdb.cc \
	src/amf/amfnd/proxy.cc \
	src/amf/amfnd/proxydb.cc \
	src/amf/amfnd/sidb.cc \
	src/amf/amfnd/su.cc \
	src/amf/amfnd/sudb.cc \
	src/amf/amfnd/susm.cc \
	src/amf/amfnd/term.cc \
	src/amf/amfnd/tmr.cc \
	src/amf/amfnd/util.cc \
	src/amf/amfnd/verify.cc

bin_osafamfnd_LDADD = \
	lib/libamf_common.la \
	lib/libosaf_common.la \
	lib/libSaClm.la \
	lib/libSaImmOi.la \
	lib/libSaImmOm.la \
	lib/libopensaf_core.la

bin_osafamfwd_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafamfwd_SOURCES = \
	src/amf/amfwd/amf_wdog.c

bin_osafamfwd_LDADD = \
	lib/libSaAmf.la \
	lib/libopensaf_core.la
