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

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osaflckd

# TODO: Fix the cpsv include dep
osaflckd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_GLD=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/glsv/include \
	-I$(top_srcdir)/osaf/libs/common/cpsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osaflckd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	gld_amf.c \
	gld_api.c \
	gld_evt.c \
	gld_imm.c \
	gld_main.c \
	gld_mbcsv.c \
	gld_mds.c \
	gld_red.c \
	gld_rsc.c \
	gld_standby.c \
	gld_tmr.c

osaflckd_LDADD =  \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/glsv/libglsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la 
