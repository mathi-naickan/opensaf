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
# Author(s): GoAhead Software
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafdtmd

osafdtmd_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/services/infrastructure/dtms/include 

osafdtmd_SOURCES = \
	dtm_intra.c \
	dtm_main.c \
	dtm_read_config.c \
	dtm_node.c \
        dtm_node_db.c \
	dtm_node_sockets.c \
	dtm_inter_svc.c \
	dtm_intra_svc.c \
	dtm_intra_trans.c \
	dtm_inter_trans.c  

osafdtmd_LDFLAGS = \
	$(AM_LDFLAGS) \
	-Wl,--export-dynamic

osafdtmd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la 
