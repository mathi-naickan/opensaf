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
osaf_execbin_PROGRAMS = osafckptnd

osafckptnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_CPND=1  \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/cpsv/include

osafckptnd_SOURCES = \
	cpnd_amf.c \
	cpnd_db.c \
	cpnd_evt.c \
	cpnd_init.c \
	cpnd_main.c \
	cpnd_mds.c \
	cpnd_proc.c \
	cpnd_res.c \
	cpnd_tmr.c

osafckptnd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/cpsv/libcpsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la
