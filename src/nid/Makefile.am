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
	src/nid/agent/nid_api.h \
	src/nid/agent/nid_start_util.h \
	src/nid/nodeinit.h

osaf_execbin_PROGRAMS += bin/opensafd

nodist_osaf_execbin_SCRIPTS += \
	src/nid/configure_tipc

dist_lsbinit_SCRIPTS += \
	src/nid/opensafd

lib_libopensaf_core_la_SOURCES += \
	src/nid/agent/nid_api.c \
	src/nid/agent/nid_ipc.c \
	src/nid/agent/nid_start_util.c

bin_opensafd_CPPFLAGS = \
	-DNCS_SAF=1 \
	$(AM_CPPFLAGS)

bin_opensafd_SOURCES = \
	src/nid/nodeinit.c

bin_opensafd_LDADD = \
	lib/libopensaf_core.la

dist_pkgsysconf_DATA += \
	src/nid/nodeinit.conf.controller \
	src/nid/nodeinit.conf.payload \
	src/nid/nid.conf

if ENABLE_SYSTEMD
dist_systemd_DATA += \
	src/nid/opensafd.service
endif
