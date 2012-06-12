#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
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

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafckptd

osafckptd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_CPD=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/cpsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafckptd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	cpd_amf.c \
	cpd_db.c \
	cpd_evt.c \
	cpd_imm.c \
	cpd_init.c \
	cpd_main.c \
	cpd_mbcsv.c \
	cpd_mds.c \
	cpd_proc.c \
	cpd_red.c \
	cpd_sbevt.c \
	cpd_tmr.c

osafckptd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/cpsv/libcpsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la 
