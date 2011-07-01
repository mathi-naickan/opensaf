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
# Author(s): Wind River Systems
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = libSaAmf.map

LIB_VERSION = 0:1:2

SUBDIRS = pkgconfig

lib_LTLIBRARIES = libSaAmf.la

libSaAmf_la_SOURCES =

libSaAmf_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DNCS_AVA=1 -DNCS_AVSV_LOG=1

if HAVE_LD_VERSION_SCRIPT
libSaAmf_la_LDFLAGS = \
	-Wl,-version-script=$(top_srcdir)/osaf/libs/saf/libSaAmf/libSaAmf.map \
	-version-number $(LIB_VERSION)
else
libSaAmf_la_LDFLAGS = \
	-version-number $(LIB_VERSION)
endif

libSaAmf_la_LIBADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/agents/saf/ava/libava.la \
	$(top_builddir)/osaf/libs/common/avsv/libavsv_common.la
