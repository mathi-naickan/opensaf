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

pkglib_PROGRAMS = osafmsgd

osafmsgd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	-DNCS_MQD=1 -DASAPi_DEBUG=1 -DNCS_MQSV_LOG=1 -DNCS_DTA=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/mqsv/include \
	-I$(top_srcdir)/osaf/services/infrastructure/dtsv/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafmsgd_SOURCES = \
	$(top_srcdir)/osaf/tools/safimm/src/immutil.c \
	mqd_sbevt.c \
	mqd_mbcsv.c \
	mqd_mbedu.c \
	mqd_util.c \
	mqd_asapi.c \
	mqd_api.c \
	mqd_clm.c \
	mqd_evt.c \
	mqd_log.c \
	mqd_imm.c \
	mqd_main.c \
	mqd_mds.c \
	mqd_red.c \
	mqd_saf.c \
	mqd_tmr.c

osafmsgd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/mqsv/libmqsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la	
