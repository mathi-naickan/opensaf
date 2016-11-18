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

TEST(OsafNormalizeTimespec, Zero) {
  const timespec zero_seconds = kZeroSeconds;
  timespec result;
  osaf_normalize_timespec(&zero_seconds, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kZeroSeconds), 0);
}

TEST(OsafNormalizeTimespec, AlreadyNormalized) {
  const timespec already_normalized = { 123456789, 987654321 };
  timespec result;
  osaf_normalize_timespec(&already_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &already_normalized), 0);
}

TEST(OsafNormalizeTimespec, NanoseconsdGreaterThanOneBillion) {
  const timespec not_normalized = { 123456789, 1987654321 };
  const timespec normalized = { 123456790, 987654321 };
  timespec result;
  osaf_normalize_timespec(&not_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}

TEST(OsafNormalizeTimespec, NanoseconsdLessThanZero) {
  const timespec not_normalized = { 123456789, -100 };
  const timespec normalized = { 123456788, 1000000000 - 100 };
  timespec result;
  osaf_normalize_timespec(&not_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}

TEST(OsafNormalizeTimespec, NanoseconsGreaterThan2Billions) {
  const timespec not_normalized = { 123456789, 2101234567 };
  const timespec normalized = { 123456789 + 2, 101234567 };
  timespec result;
  osaf_normalize_timespec(&not_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}

TEST(OsafNormalizeTimespec, NanoseconsLessThanMinus2Billions) {
  const timespec not_normalized = { 123456789, -2000000567 };
  const timespec normalized = { 123456789 - 2 - 1, 1000000000 - 567 };
  timespec result;
  osaf_normalize_timespec(&not_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}

TEST(OsafNormalizeTimespec, BothParametersAtSameAddress) {
  const timespec not_normalized = { 123456789, -2000000567 };
  const timespec normalized = { 123456789 - 2 - 1, 1000000000 - 567 };
  timespec result = not_normalized;
  osaf_normalize_timespec(&result, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}

TEST(OsafNormalizeTimespec, NegativeSecondsAndNanoseconds) {
  const timespec not_normalized = { -123, -1 };
  const timespec normalized = { -124, 999999999 };
  timespec result;
  osaf_normalize_timespec(&not_normalized, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &normalized), 0);
}
