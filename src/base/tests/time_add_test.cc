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

static const timespec kTwoDotFourSeconds = { 2, 400000000 };
static const timespec kFourDotEightSeconds = { 4, 800000000 };
static const timespec kNineDotSixSeconds = { 9, 600000000 };
static const timespec number1 = { 576315623, 358743382 };
static const timespec number2 = { 1279394477, 719128783 };
static const timespec sum = { 1855710101,  77872165 };

TEST(BaseTimeAdd, ZeroPlusZero) {
  timespec result = base::kZeroSeconds + base::kZeroSeconds;
  EXPECT_TRUE(result == base::kZeroSeconds);
}

TEST(BaseTimeAdd, TwoDotFourAddedWithItself) {
  timespec result = kTwoDotFourSeconds;
  result += result;
  EXPECT_TRUE(result == kFourDotEightSeconds);
}

TEST(BaseTimeAdd, FourDotEightAddedWithItself) {
  timespec result = kFourDotEightSeconds;
  result += result;
  EXPECT_TRUE(result == kNineDotSixSeconds);
}

TEST(BaseTimeAdd, AddRandomNumbers) {
  timespec result = number1 + number2;
  EXPECT_TRUE(result == sum);
}

TEST(BaseTimeAdd, FirstParameterAndResultAtSameAddress) {
  timespec result = number1;
  result += number2;
  EXPECT_TRUE(result == sum);
}
