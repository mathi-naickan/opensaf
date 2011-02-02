#      -*- OpenSAF  -*-
#
# (C) Copyright 2011 The OpenSAF Foundation
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
# Author(s): Goahead Software
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = rdegetrole

rdegetrole_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/core/include

rdegetrole_SOURCES = \
	rde_get_role.c

rdegetrole_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/agents/infrastructure/rda/librda.la
