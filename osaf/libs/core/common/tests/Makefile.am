#      -*- OpenSAF  -*-
#
# (C) Copyright 2015 The OpenSAF Foundation
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

TESTS = core_common_test

check_PROGRAMS = $(TESTS)

core_common_test_CXXFLAGS =$(AM_CXXFLAGS)

core_common_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/core/common/include \
	-I$(GTEST_DIR)/include

core_common_test_LDFLAGS = \
	-pthread -lrt \
	$(top_builddir)/osaf/libs/core/common/libopensaf_common_la-osaf_time.o \
	$(top_builddir)/osaf/libs/core/common/libopensaf_common_la-osaf_utility.o

core_common_test_SOURCES = \
	osaf_clock_gettime_test.cc \
	osaf_nanosleep_test.cc \
	osaf_normalize_timespec_test.cc \
	osaf_timespec_add_test.cc \
	osaf_timespec_compare_test.cc \
	osaf_timespec_convert_test.cc \
	osaf_timespec_subtract_test.cc \
	osaf_millis_timeout_test.cc \
	mock_clock_gettime.cc \
	mock_clock_nanosleep.cc \
	mock_syslog.cc

core_common_test_LDADD = \
	$(GTEST_DIR)/lib/libgtest.la \
	$(GTEST_DIR)/lib/libgtest_main.la
