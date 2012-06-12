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

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafclmna

osafclmna_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/clmsv/include

osafclmna_SOURCES = \
	clms_nodeagent.c

osafclmna_LDADD = \
	$(top_builddir)/osaf/libs/common/clmsv/libclmsv_common.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la
