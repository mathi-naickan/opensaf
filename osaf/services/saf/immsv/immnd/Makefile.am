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
# Author(s): Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = scripts

noinst_HEADERS = \
	immnd_cb.h \
	immnd.h \
	immnd_init.h \
	ImmAttrValue.hh \
	ImmModel.hh \
	ImmSearchOp.hh

osaf_execbindir = $(pkglibdir)
osaf_execbin_PROGRAMS = osafimmnd

osafimmnd_CPPFLAGS = \
	-DSA_CLM_B01 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/immsv/include

osafimmnd_SOURCES =	\
	immnd_amf.c \
	immnd_db.c \
	immnd_evt.c \
	immnd_main.c \
	immnd_mds.c \
	immnd_proc.c \
	ImmAttrValue.cc \
	ImmSearchOp.cc \
	ImmModel.cc

osafimmnd_LDADD = \
	$(top_builddir)/osaf/libs/core/libopensaf_core.la \
	$(top_builddir)/osaf/libs/common/immsv/libimmsv_common.la \
	$(top_builddir)/osaf/libs/saf/libSaAmf/libSaAmf.la
