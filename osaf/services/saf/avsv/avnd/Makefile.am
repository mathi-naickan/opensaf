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

SUBDIRS = include scripts

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafamfnd

osafamfnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/avsv/include \
	-I$(top_srcdir)/osaf/services/saf/avsv/avnd/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafamfnd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	amfnd_main.c \
	avnd.c \
	avnd_cam.c \
	avnd_cbq.c \
	avnd_chc.c \
	avnd_ckpt_dec.c \
	avnd_ckpt_edu.c \
	avnd_ckpt_enc.c \
	avnd_ckpt_updt.c \
	avnd_clc.c \
	avnd_clm.c \
	avnd_comp.c \
	avnd_compdb.c \
	avnd_cpm.c \
	avnd_di.c \
	avnd_err.c \
	avnd_evt.c \
	avnd_hcdb.c \
	avnd_mds.c \
	avnd_mbcsv.c \
	avnd_mon.c \
	avnd_pg.c \
	avnd_pgdb.c \
	avnd_proc.c \
	avnd_sidb.c \
	avnd_su.c \
	avnd_sudb.c \
	avnd_susm.c \
	avnd_term.c \
	avnd_tmr.c \
	avnd_util.c \
	avnd_verify.c \
	avnd_avnd.c \
	avnd_proxy.c \
	avnd_proxydb.c

osafamfnd_LDADD = \
        $(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/avsv/libavsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la
