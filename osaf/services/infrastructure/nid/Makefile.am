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

SUBDIRS = config scripts

noinst_HEADERS = \
	nodeinit.h

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = opensafd

opensafd_CPPFLAGS = \
	-DNCS_SAF=1 \
	$(AM_CPPFLAGS)

opensafd_SOURCES = \
	nodeinit.c

opensafd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la
