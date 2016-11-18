/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

#include "base/osaf_time.h"
#include "gtest/gtest.h"

static const timespec kZeroDotOneSeconds = { 0, 100000000 };
static const timespec kZeroDotNineSeconds = { 0, 900000000 };
static const timespec kOneDotOneSeconds = { 1, 100000000 };

TEST(OsafTimespecCompare, ZeroWithZero) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroSeconds, &kZeroSeconds), 0);
}

TEST(OsafTimespecCompare, ZeroWithZeroDotOne) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroSeconds, &kZeroDotOneSeconds), -1);
}

TEST(OsafTimespecCompare, ZeroDotOneWithZero) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroDotOneSeconds, &kZeroSeconds), 1);
}

TEST(OsafTimespecCompare, ZeroWithOne) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroSeconds, &kOneSecond), -1);
}

TEST(OsafTimespecCompare, OneWithZero) {
  EXPECT_EQ(osaf_timespec_compare(&kOneSecond, &kZeroSeconds), 1);
}

TEST(OsafTimespecCompare, OneWithOne) {
  EXPECT_EQ(osaf_timespec_compare(&kOneSecond, &kOneSecond), 0);
}

TEST(OsafTimespecCompare, OneWithOneDotOne) {
  EXPECT_EQ(osaf_timespec_compare(&kOneSecond, &kOneDotOneSeconds), -1);
}

TEST(OsafTimespecCompare, OneDotOneWithOne) {
  EXPECT_EQ(osaf_timespec_compare(&kOneDotOneSeconds, &kOneSecond), 1);
}

TEST(OsafTimespecCompare, ZeroDotNineWithZeroDotNine) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroDotNineSeconds, &kZeroDotNineSeconds), 0);
}

TEST(OsafTimespecCompare, ZeroDotNineWithOneDotOne) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroDotNineSeconds, &kOneDotOneSeconds), -1);
}

TEST(OsafTimespecCompare, ZeroDotNineWithZeroDotOne) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroDotNineSeconds, &kZeroDotOneSeconds), 1);
}

TEST(OsafTimespecCompare, OneDotOneWithZeroDotNine) {
  EXPECT_EQ(osaf_timespec_compare(&kOneDotOneSeconds, &kZeroDotNineSeconds), 1);
}

TEST(OsafTimespecCompare, ZeroDotOneWithZeroDotNine) {
  EXPECT_EQ(osaf_timespec_compare(&kZeroDotOneSeconds, &kZeroDotNineSeconds), -1);
}
