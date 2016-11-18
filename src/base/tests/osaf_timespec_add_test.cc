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

static const timespec kTwoDotFourSeconds = { 2, 400000000 };
static const timespec kFourDotEightSeconds = { 4, 800000000 };
static const timespec kNineDotSixSeconds = { 9, 600000000 };
static const timespec number1 = { 576315623, 358743382 };
static const timespec number2 = { 1279394477, 719128783 };
static const timespec sum = { 1855710101,  77872165 };

TEST(OsafTimespecAdd, ZeroPlusZero) {
  timespec result;
  osaf_timespec_add(&kZeroSeconds, &kZeroSeconds, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kZeroSeconds), 0);
}

TEST(OsafTimespecAdd, TwoDotFourAddedWithItself) {
  timespec result = kTwoDotFourSeconds;
  osaf_timespec_add(&result, &result, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kFourDotEightSeconds), 0);
}

TEST(OsafTimespecAdd, FourDotEightAddedWithItself) {
  timespec result = kFourDotEightSeconds;
  osaf_timespec_add(&result, &result, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kNineDotSixSeconds), 0);
}

TEST(OsafTimespecAdd, AddRandomNumbers) {
  timespec result;
  osaf_timespec_add(&number1, &number2, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &sum), 0);
}

TEST(OsafTimespecAdd, FirstParameterAndResultAtSameAddress) {
  timespec result = number1;
  osaf_timespec_add(&result, &number2, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &sum), 0);
}

TEST(OsafTimespecAdd, SecondParameterAndResultAtSameAddress) {
  timespec result = number2;
  osaf_timespec_add(&number1, &result, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &sum), 0);
}
