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

SUBDIRS = scripts

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osaflcknd

osaflcknd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_GLND=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/glsv/include

osaflcknd_SOURCES = \
	glnd_agent.c \
	glnd_amf.c \
	glnd_api.c \
	glnd_cb.c \
	glnd_ckpt.c \
	glnd_client.c \
	glnd_evt.c \
	glnd_main.c \
	glnd_mds.c \
	glnd_queue.c \
	glnd_res.c \
	glnd_res_req.c \
	glnd_restart.c \
	glnd_shm.c \
	glnd_tmr.c

osaflcknd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/glsv/libglsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la
