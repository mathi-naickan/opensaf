#      -*- OpenSAF  -*-
#
# (C) Copyright 2016 The OpenSAF Foundation
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

noinst_HEADERS += \
	src/fm/fmd/fm.h \
	src/fm/fmd/fm_amf.h \
	src/fm/fmd/fm_cb.h \
	src/fm/fmd/fm_evt.h \
	src/fm/fmd/fm_mds.h \
	src/fm/fmd/fm_mem.h

osaf_execbin_PROGRAMS += bin/osaffmd
nodist_pkgclccli_SCRIPTS += \
	src/fm/fmd/osaf-fmd

dist_pkgsysconf_DATA += \
	src/fm/fmd/fmd.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/fm/fm_sc_template.xml \
	src/fm/fm_objects.xml

endif

bin_osaffmd_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_osaffmd_SOURCES = \
	src/fm/fmd/fm_amf.c \
	src/fm/fmd/fm_main.c \
	src/fm/fmd/fm_mds.c \
	src/fm/fmd/fm_rda.c

bin_osaffmd_LDADD = \
	lib/libSaAmf.la \
	lib/libSaClm.la \
	lib/libopensaf_core.la
