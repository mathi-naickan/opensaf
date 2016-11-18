/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#include "base/time.h"
#include "gtest/gtest.h"

namespace {
const timespec kOneDotOneSeconds = { 1, 100000000 };
const timespec kZeroDotTwoSeconds = { 0, 200000000 };
const timespec kZeroDotNineSeconds = { 0, 900000000 };
const timespec number1 = { 1576315623, 358743382 };
const timespec number2 = { 279394477, 639614599 };
const timespec difference = { 1296921145, 719128783 };
}

TEST(BaseTimeSubtract, ZeroMinusZero) {
  timespec result = base::kZeroSeconds - base::kZeroSeconds;
  EXPECT_TRUE(result == base::kZeroSeconds);
}

TEST(BaseTimeSubtract, FifteenMinusFive) {
  timespec result = base::kFifteenSeconds - base::kFiveSeconds;
  EXPECT_TRUE(result == base::kTenSeconds);
}

TEST(BaseTimeSubtract, OneDotOneMinusZeroDotTwo) {
  timespec result = kOneDotOneSeconds - kZeroDotTwoSeconds;
  EXPECT_TRUE(result == kZeroDotNineSeconds);
}

TEST(BaseTimeSubtract, SubtractRandomNumbers) {
  timespec result = number1 - number2;
  EXPECT_TRUE(result == difference);
}

TEST(BaseTimeSubtract, FirstParameterAndResultAtSameAddress) {
  timespec result = number1;
  result -= number2;
  EXPECT_TRUE(result == difference);
}
