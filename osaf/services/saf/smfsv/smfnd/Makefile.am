#      OpenSAF 
#
# (C) Copyright 2009-2010 The OpenSAF Foundation
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

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafsmfnd

noinst_HEADERS = \
	smfnd.h \
	smfnd_evt.h

osafsmfnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/smfsv/include

osafsmfnd_SOURCES = \
	smfnd_amf.c \
	smfnd_evt.c \
	smfnd_main.c \
	smfnd_mds.c

osafsmfnd_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/smfsv/libsmfsv_common.la
