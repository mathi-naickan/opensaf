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

static const timespec kZeroDotNineSeconds = { 0, 900000000 };
static const timespec kOneDotOneSeconds = { 1, 100000000 };

TEST(BaseTimeCompare, ZeroWithZero) {
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(base::kZeroSeconds < base::kZeroSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(base::kZeroSeconds <= base::kZeroSeconds);
  EXPECT_TRUE(base::kZeroSeconds == base::kZeroSeconds);
  EXPECT_FALSE(base::kZeroSeconds != base::kZeroSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(base::kZeroSeconds >= base::kZeroSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(base::kZeroSeconds > base::kZeroSeconds);
}

TEST(BaseTimeCompare, ZeroWithZeroDotOne) {
  EXPECT_TRUE(base::kZeroSeconds < base::kOneHundredMilliseconds);
  EXPECT_TRUE(base::kZeroSeconds <= base::kOneHundredMilliseconds);
  EXPECT_FALSE(base::kZeroSeconds == base::kOneHundredMilliseconds);
  EXPECT_TRUE(base::kZeroSeconds != base::kOneHundredMilliseconds);
  EXPECT_FALSE(base::kZeroSeconds >= base::kOneHundredMilliseconds);
  EXPECT_FALSE(base::kZeroSeconds > base::kOneHundredMilliseconds);
}

TEST(BaseTimeCompare, ZeroDotOneWithZero) {
  EXPECT_FALSE(base::kOneHundredMilliseconds < base::kZeroSeconds);
  EXPECT_FALSE(base::kOneHundredMilliseconds <= base::kZeroSeconds);
  EXPECT_FALSE(base::kOneHundredMilliseconds == base::kZeroSeconds);
  EXPECT_TRUE(base::kOneHundredMilliseconds != base::kZeroSeconds);
  EXPECT_TRUE(base::kOneHundredMilliseconds >= base::kZeroSeconds);
  EXPECT_TRUE(base::kOneHundredMilliseconds > base::kZeroSeconds);
}

TEST(BaseTimeCompare, ZeroWithOne) {
  EXPECT_TRUE(base::kZeroSeconds < base::kOneSecond);
  EXPECT_TRUE(base::kZeroSeconds <= base::kOneSecond);
  EXPECT_FALSE(base::kZeroSeconds == base::kOneSecond);
  EXPECT_TRUE(base::kZeroSeconds != base::kOneSecond);
  EXPECT_FALSE(base::kZeroSeconds >= base::kOneSecond);
  EXPECT_FALSE(base::kZeroSeconds > base::kOneSecond);
}

TEST(BaseTimeCompare, OneWithZero) {
  EXPECT_FALSE(base::kOneSecond < base::kZeroSeconds);
  EXPECT_FALSE(base::kOneSecond <= base::kZeroSeconds);
  EXPECT_FALSE(base::kOneSecond == base::kZeroSeconds);
  EXPECT_TRUE(base::kOneSecond != base::kZeroSeconds);
  EXPECT_TRUE(base::kOneSecond >= base::kZeroSeconds);
  EXPECT_TRUE(base::kOneSecond > base::kZeroSeconds);
}

TEST(BaseTimeCompare, OneWithOne) {
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(base::kOneSecond < base::kOneSecond);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(base::kOneSecond <= base::kOneSecond);
  EXPECT_TRUE(base::kOneSecond == base::kOneSecond);
  EXPECT_FALSE(base::kOneSecond != base::kOneSecond);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(base::kOneSecond >= base::kOneSecond);
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(base::kOneSecond > base::kOneSecond);
}

TEST(BaseTimeCompare, OneWithOneDotOne) {
  EXPECT_TRUE(base::kOneSecond < kOneDotOneSeconds);
  EXPECT_TRUE(base::kOneSecond <= kOneDotOneSeconds);
  EXPECT_FALSE(base::kOneSecond == kOneDotOneSeconds);
  EXPECT_TRUE(base::kOneSecond != kOneDotOneSeconds);
  EXPECT_FALSE(base::kOneSecond >= kOneDotOneSeconds);
  EXPECT_FALSE(base::kOneSecond > kOneDotOneSeconds);
}

TEST(BaseTimeCompare, OneDotOneWithOne) {
  EXPECT_FALSE(kOneDotOneSeconds < base::kOneSecond);
  EXPECT_FALSE(kOneDotOneSeconds <= base::kOneSecond);
  EXPECT_FALSE(kOneDotOneSeconds == base::kOneSecond);
  EXPECT_TRUE(kOneDotOneSeconds != base::kOneSecond);
  EXPECT_TRUE(kOneDotOneSeconds >= base::kOneSecond);
  EXPECT_TRUE(kOneDotOneSeconds > base::kOneSecond);
}

TEST(BaseTimeCompare, ZeroDotNineWithZeroDotNine) {
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(kZeroDotNineSeconds < kZeroDotNineSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(kZeroDotNineSeconds <= kZeroDotNineSeconds);
  EXPECT_TRUE(kZeroDotNineSeconds == kZeroDotNineSeconds);
  EXPECT_FALSE(kZeroDotNineSeconds != kZeroDotNineSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_TRUE(kZeroDotNineSeconds >= kZeroDotNineSeconds);
  // cppcheck-suppress duplicateExpression
  EXPECT_FALSE(kZeroDotNineSeconds > kZeroDotNineSeconds);
}

TEST(BaseTimeCompare, ZeroDotNineWithOneDotOne) {
  EXPECT_TRUE(kZeroDotNineSeconds < kOneDotOneSeconds);
  EXPECT_TRUE(kZeroDotNineSeconds <= kOneDotOneSeconds);
  EXPECT_FALSE(kZeroDotNineSeconds == kOneDotOneSeconds);
  EXPECT_TRUE(kZeroDotNineSeconds != kOneDotOneSeconds);
  EXPECT_FALSE(kZeroDotNineSeconds >= kOneDotOneSeconds);
  EXPECT_FALSE(kZeroDotNineSeconds > kOneDotOneSeconds);
}

TEST(BaseTimeCompare, ZeroDotNineWithZeroDotOne) {
  EXPECT_FALSE(kZeroDotNineSeconds < base::kOneHundredMilliseconds);
  EXPECT_FALSE(kZeroDotNineSeconds <= base::kOneHundredMilliseconds);
  EXPECT_FALSE(kZeroDotNineSeconds == base::kOneHundredMilliseconds);
  EXPECT_TRUE(kZeroDotNineSeconds != base::kOneHundredMilliseconds);
  EXPECT_TRUE(kZeroDotNineSeconds >= base::kOneHundredMilliseconds);
  EXPECT_TRUE(kZeroDotNineSeconds > base::kOneHundredMilliseconds);
}

TEST(BaseTimeCompare, OneDotOneWithZeroDotNine) {
  EXPECT_FALSE(kOneDotOneSeconds < kZeroDotNineSeconds);
  EXPECT_FALSE(kOneDotOneSeconds <= kZeroDotNineSeconds);
  EXPECT_FALSE(kOneDotOneSeconds == kZeroDotNineSeconds);
  EXPECT_TRUE(kOneDotOneSeconds != kZeroDotNineSeconds);
  EXPECT_TRUE(kOneDotOneSeconds >= kZeroDotNineSeconds);
  EXPECT_TRUE(kOneDotOneSeconds > kZeroDotNineSeconds);
}

TEST(BaseTimeCompare, ZeroDotOneWithZeroDotNine) {
  EXPECT_TRUE(base::kOneHundredMilliseconds < kZeroDotNineSeconds);
  EXPECT_TRUE(base::kOneHundredMilliseconds <= kZeroDotNineSeconds);
  EXPECT_FALSE(base::kOneHundredMilliseconds == kZeroDotNineSeconds);
  EXPECT_TRUE(base::kOneHundredMilliseconds != kZeroDotNineSeconds);
  EXPECT_FALSE(base::kOneHundredMilliseconds >= kZeroDotNineSeconds);
  EXPECT_FALSE(base::kOneHundredMilliseconds > kZeroDotNineSeconds);
}

TEST(BaseTimeCompare, MaxOfTwoWithLargestLast) {
  EXPECT_EQ(base::Max(kZeroDotNineSeconds, kOneDotOneSeconds),
            kOneDotOneSeconds);
  EXPECT_EQ(base::Max(base::kOneHundredMilliseconds,
                      base::kTwoHundredMilliseconds),
            base::kTwoHundredMilliseconds);
}

TEST(BaseTimeCompare, MaxOfTwoWithLargestFirst) {
  EXPECT_EQ(base::Max(kOneDotOneSeconds, kZeroDotNineSeconds),
            kOneDotOneSeconds);
  EXPECT_EQ(base::Max(base::kTwoHundredMilliseconds,
                      base::kOneHundredMilliseconds),
            base::kTwoHundredMilliseconds);
}

TEST(BaseTimeCompare, MaxOfTwoEqual) {
  EXPECT_EQ(base::Max(kOneDotOneSeconds, kOneDotOneSeconds),
            kOneDotOneSeconds);
}

TEST(BaseTimeCompare, MaxOfThreeWithLargestLast) {
  EXPECT_EQ(base::Max(kZeroDotNineSeconds, base::kOneSecond,
                      kOneDotOneSeconds),
            kOneDotOneSeconds);
  EXPECT_EQ(base::Max(base::kOneHundredMilliseconds,
                      base::kTwoHundredMilliseconds,
                      base::kThreeHundredMilliseconds),
            base::kThreeHundredMilliseconds);
}

TEST(BaseTimeCompare, MaxOfThreeWithLargestInTheMiddle) {
  EXPECT_EQ(base::Max(kZeroDotNineSeconds, kOneDotOneSeconds,
                      base::kOneSecond),
            kOneDotOneSeconds);
  EXPECT_EQ(base::Max(base::kOneHundredMilliseconds,
                      base::kThreeHundredMilliseconds,
                      base::kTwoHundredMilliseconds),
            base::kThreeHundredMilliseconds);
}

TEST(BaseTimeCompare, MaxOfThreeWithLargestFirst) {
  EXPECT_EQ(base::Max(kOneDotOneSeconds, base::kOneSecond,
                      kZeroDotNineSeconds),
            kOneDotOneSeconds);
  EXPECT_EQ(base::Max(base::kThreeHundredMilliseconds,
                      base::kOneHundredMilliseconds,
                      base::kTwoHundredMilliseconds),
            base::kThreeHundredMilliseconds);
}

TEST(BaseTimeCompare, MaxOfThreeEqual) {
  EXPECT_EQ(base::Max(kOneDotOneSeconds, kOneDotOneSeconds, kOneDotOneSeconds),
            kOneDotOneSeconds);
}
