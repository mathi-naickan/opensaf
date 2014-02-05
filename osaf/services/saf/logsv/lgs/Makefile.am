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
#            Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

noinst_HEADERS = \
	lgs_cb.h \
	lgs_fmt.h \
	lgs_mbcsv.h \
	lgs_mbcsv_v1.h \
	lgs_mbcsv_v2.h \
	lgs_util.h \
	lgs_evt.h \
	lgs.h \
	lgs_stream.h \
	lgs_file.h \
	lgs_filehdl.h

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osaflogd

osaflogd_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/logsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osaflogd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	lgs_amf.c \
	lgs_evt.c \
	lgs_fmt.c \
	lgs_imm.c \
	lgs_main.c \
	lgs_mbcsv.c \
	lgs_mbcsv_v1.c \
	lgs_mbcsv_v2.c \
	lgs_mds.c \
	lgs_stream.c \
	lgs_util.c \
	lgs_file.c \
	lgs_filehdl.c

osaflogd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la
