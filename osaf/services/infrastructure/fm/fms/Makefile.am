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
# Author(s): Emerson Network Power 
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osaffmd

noinst_HEADERS = \
	fm_amf.h \
	fm_cb.h \
	fm_evt.h \
	fm.h \
	fm_mds.h \
	fm_mem.h

osaffmd_CPPFLAGS= \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/services/infrastructure/fm/include

osaffmd_CFLAGS = $(AM_CFLAGS)

osaffmd_SOURCES = \
	fm_main.c \
	fm_mds.c \
	fm_rda.c \
	fm_amf.c

osaffmd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la
