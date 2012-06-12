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
osaf_execbin_PROGRAMS = osafmsgnd

osafmsgnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_MQND=1 -DASAPi_DEBUG=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/mqsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafmsgnd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	mqnd_amf.c \
	mqnd_db.c \
	mqnd_evt.c \
	mqnd_imm.c \
	mqnd_init.c \
	mqnd_main.c \
	mqnd_mds.c \
	mqnd_mq.c \
	mqnd_namedb.c \
	mqnd_proc.c \
	mqnd_restart.c \
	mqnd_shm.c \
	mqnd_tmr.c \
	mqnd_util.c

osafmsgnd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/mqsv/libmqsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la
