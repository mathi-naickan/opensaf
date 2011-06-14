#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
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

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = test_lib.c

SUBDIRS = include

noinst_LTLIBRARIES = libplmc.la

libplmc_la_CPPFLAGS = \
	-DNCS_PLMSV_LOG=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/plmsv/plmc/include \
	-I$(top_srcdir)/contrib/plmc/include
	
libplmc_la_LDFLAGS = -static

libplmc_la_SOURCES = \
	plmc_lib.c \
	plmc_lib_internal.c

libplmc_la_LIBADD = \
	$(top_builddir)/contrib/plmc/lib/utils/libplmc_utils.la
