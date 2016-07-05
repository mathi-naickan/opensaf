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

SUBDIRS = include scripts tests

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafamfnd

osafamfd_CXXFLAGS =$(AM_CXXFLAGS)

osafamfnd_CPPFLAGS = \
	-DSA_CLM_B01=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/amf/include \
	-I$(top_srcdir)/osaf/services/saf/amf/amfnd/include \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafamfnd_SOURCES = \
	amfnd.cc \
	cam.cc \
	cbq.cc \
	chc.cc \
	clc.cc \
	clm.cc \
	comp.cc \
	compdb.cc \
	cpm.cc \
	di.cc \
	err.cc \
	evt.cc \
	hcdb.cc \
	main.cc \
	mds.cc \
	mon.cc \
	pg.cc \
	pgdb.cc \
	proxy.cc \
	proxydb.cc \
	sidb.cc \
	su.cc \
	sudb.cc \
	susm.cc \
	term.cc \
	tmr.cc \
	util.cc \
	verify.cc \
	imm.cc

osafamfnd_LDADD = \
	$(top_builddir)/osaf/tools/safimm/src/libimmutil.la \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/amf/libamf_common.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la \
	$(top_builddir)/osaf/libs/saf/libSaClm/libSaClm.la
