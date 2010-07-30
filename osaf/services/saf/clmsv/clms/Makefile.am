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
# Author(s):  Emerson Network Power
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

noinst_HEADERS = \
	clms_cb.h \
	clms_mbcsv.h \
	clms_evt.h \
	clms_imm.h \
	clms_ntf.h \
	clms.h

pkglib_PROGRAMS = osafclmd

osafclmd_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/clmsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include \
	-I$(top_srcdir)/osaf/tools/saflog/include

osafclmd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	$(top_srcdir)/osaf/tools/saflog/src/saflog.c \
	clms_amf.c \
	clms_evt.c \
	clms_imm.c \
	clms_main.c \
	clms_util.c \
	clms_mbcsv.c \
	clms_ntf.c \
	clms_mds.c

osafclmd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/clmsv/libclmsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/saf/libSaNtf/libSaNtf.la \
	$(top_builddir)/osaf/libs/saf/libSaLog/libSaLog.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la

if ENABLE_AIS_PLM

osafclmd_SOURCES += clms_plm.c

osafclmd_LDADD += \
	$(top_builddir)/osaf/libs/saf/libSaPlm/libSaPlm.la

endif
