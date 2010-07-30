#      -*- OpenSAF  -*-
#
# (C) Copyright 2008 The OpenSAF Foundation
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
# Author(s): Wind River Systems
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

pkglib_PROGRAMS = osafdtd

osafdtd_CPPFLAGS = \
	-DNCS_DTS=1 -DDTS_LOG=1 -DNCS_DTA=1 -DDTS_FLOW=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/services/infrastructure/dtsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafdtd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	dts_amf.c \
	dts_api.c \
	dts_cbop.c \
	dts_chkop.c \
	dts_ckpt_dec.c \
	dts_ckpt_edu.c \
	dts_ckpt_enc.c \
	dts_ckpt_updt.c \
	dts_def.c \
	dts_dl_api.c \
	dts_imm.c \
	dts_log.c \
	dts_main.c \
	dts_mds.c \
	dts_logpolc.c \
	dts_pvt.c \
	dts_seq.c \
	dts_sysf_log.c \
	dts_tgt.c

osafdtd_LDFLAGS = \
	$(AM_LDFLAGS) \
	-Wl,--export-dynamic

osafdtd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la
