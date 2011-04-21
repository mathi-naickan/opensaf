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
# Author(s): Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = libSaImmOi.map libSaImmOm.map

OILIB_VERSION = 0:2:1
OMLIB_VERSION = 0:2:1

SUBDIRS = pkgconfig

lib_LTLIBRARIES = libSaImmOi.la libSaImmOm.la

libSaImmOi_la_SOURCES = 

libSaImmOi_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

if HAVE_LD_VERSION_SCRIPT
libSaImmOi_la_LDFLAGS = \
	-Wl,-version-script=$(top_srcdir)/osaf/libs/saf/libSaImm/libSaImmOi.map \
	-version-number $(OILIB_VERSION)
else
libSaImmOi_la_LDFLAGS = \
	-version-number $(OILIB_VERSION)
endif

libSaImmOi_la_LIBADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/agents/saf/imma/libimmaOi.la \
	$(top_builddir)/osaf/libs/common/immsv/libimmsv_common.la


libSaImmOm_la_SOURCES = 

libSaImmOm_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

if HAVE_LD_VERSION_SCRIPT
libSaImmOm_la_LDFLAGS = \
	-Wl,-version-script=$(top_srcdir)/osaf/libs/saf/libSaImm/libSaImmOm.map \
	-version-number $(OMLIB_VERSION)
else
libSaImmOm_la_LDFLAGS = \
	-version-number $(OMLIB_VERSION)
endif

libSaImmOm_la_LIBADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/agents/saf/imma/libimmaOm.la \
	$(top_builddir)/osaf/libs/common/immsv/libimmsv_common.la
