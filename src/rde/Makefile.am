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
	src/rde/agent/rda.h \
	src/rde/agent/rda_papi.h \
	src/rde/rde_rda_common.h \
	src/rde/rded/rde_amf.h \
	src/rde/rded/rde_cb.h \
	src/rde/rded/rde_rda.h \
	src/rde/rded/role.h

bin_PROGRAMS += bin/rdegetrole
osaf_execbin_PROGRAMS += bin/osafrded

nodist_pkgclccli_SCRIPTS += \
	src/rde/rded/osaf-rded

lib_libopensaf_core_la_SOURCES += \
	src/rde/agent/rda_papi.cc

dist_pkgsysconf_DATA += \
	src/rde/rded/rde.conf

if ENABLE_IMMXML

dist_pkgimmxml_svc_DATA += \
	src/rde/rde_sc_template.xml \
	src/rde/rde_objects.xml

endif

bin_osafrded_CXXFLAGS = $(AM_CXXFLAGS)

bin_osafrded_CPPFLAGS = \
	-DSA_EXTENDED_NAME_SOURCE \
	$(AM_CPPFLAGS)

bin_osafrded_SOURCES = \
	src/rde/rded/rde_amf.cc \
	src/rde/rded/rde_main.cc \
	src/rde/rded/rde_mds.cc \
	src/rde/rded/rde_rda.cc \
	src/rde/rded/role.cc

bin_osafrded_LDADD = \
	lib/libSaAmf.la \
	lib/libopensaf_core.la

bin_rdegetrole_CPPFLAGS = \
	$(AM_CPPFLAGS)

bin_rdegetrole_SOURCES = \
	src/rde/tools/rde_get_role.c

bin_rdegetrole_LDADD = \
	lib/libopensaf_core.la
