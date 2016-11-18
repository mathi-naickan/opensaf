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

static const timespec kOneDotOneSeconds = { 1, 100000000 };
static const timespec kZeroDotTwoSeconds = { 0, 200000000 };
static const timespec kZeroDotNineSeconds = { 0, 900000000 };
static const timespec number1 = { 1576315623, 358743382 };
static const timespec number2 = { 279394477, 639614599 };
static const timespec difference = { 1296921145, 719128783 };

TEST(OsafTimespecSubtract, ZeroMinusZero) {
  timespec result;
  osaf_timespec_subtract(&kZeroSeconds, &kZeroSeconds, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kZeroSeconds), 0);
}

TEST(OsafTimespecSubtract, FifteenMinusFive) {
  timespec result = kFifteenSeconds;
  osaf_timespec_subtract(&result, &kFiveSeconds, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kTenSeconds), 0);
}

TEST(OsafTimespecSubtract, OneDotOneMinusZeroDotTwo) {
  timespec result;
  osaf_timespec_subtract(&kOneDotOneSeconds, &kZeroDotTwoSeconds, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kZeroDotNineSeconds), 0);
}

TEST(OsafTimespecSubtract, SubtractRandomNumbers) {
  timespec result;
  osaf_timespec_subtract(&number1, &number2, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &difference), 0);
}

TEST(OsafTimespecSubtract, FirstParameterAndResultAtSameAddress) {
  timespec result = number1;
  osaf_timespec_subtract(&result, &number2, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &difference), 0);
}

TEST(OsafTimespecSubtract, SecondParameterAndResultAtSameAddress) {
  timespec result = number2;
  osaf_timespec_subtract(&number1, &result, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &difference), 0);
}
