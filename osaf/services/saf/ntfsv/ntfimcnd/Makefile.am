#      -*- OpenSAF  -*-
#
# (C) Copyright 2013 The OpenSAF Foundation
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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

noinst_HEADERS = \
	ntfimcn_main.h \
	ntfimcn_notifier.h \
	ntfimcn_imm.h

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafntfimcnd

osafntfimcnd_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/ntfsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafntfimcnd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	ntfimcn_main.c \
	ntfimcn_notifier.c \
	ntfimcn_imm.c

osafntfimcnd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la \
	$(top_builddir)/osaf/libs/saf/libSaNtf/libSaNtf.la
