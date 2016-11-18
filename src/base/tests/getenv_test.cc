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

#include <cstdlib>
#include <cstring>
#include "base/getenv.h"
#include "gtest/gtest.h"

TEST(BaseGetEnv, StringDefault) {
  unsetenv("HELLO");
  std::string default_value{"DEFAULT_VALUE"};
  std::string value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}

TEST(BaseGetEnv, StringValue) {
  setenv("HELLO", "THERE", 1);
  std::string default_value{"DEFAULT_VALUE"};
  std::string value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, std::string{"THERE"});
}

TEST(BaseGetEnv, CharDefault) {
  unsetenv("HELLO");
  const char* default_value{"DEFAULT_VALUE"};
  const char* value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(strcmp(value, default_value), 0);
}

TEST(BaseGetEnv, CharValue) {
  setenv("HELLO", "THERE", 1);
  const char* default_value{"DEFAULT_VALUE"};
  const char* value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(strcmp(value, "THERE"), 0);
}

TEST(BaseGetEnv, NegativeSignedIntegerDefault) {
  unsetenv("HELLO");
  int64_t default_value{-1234567890123456789ll};
  int64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}

TEST(BaseGetEnv, NegativeSignedIntegerValue) {
  setenv("HELLO", "-2345678901234567890", 1);
  int64_t default_value{-1234567890123456789ll};
  int64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, -2345678901234567890ll);
}

TEST(BaseGetEnv, PositiveSignedIntegerDefault) {
  unsetenv("HELLO");
  int64_t default_value{1234567890123456789ll};
  int64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}

TEST(BaseGetEnv, PositiveSignedIntegerValue) {
  setenv("HELLO", "2345678901234567890", 1);
  int64_t default_value{1234567890123456789ll};
  int64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, 2345678901234567890ll);
}

TEST(BaseGetEnv, HexadecimalUnsignedIntegerDefault) {
  unsetenv("HELLO");
  uint64_t default_value{0xfefefefefefefefeull};
  uint64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}

TEST(BaseGetEnv, HexadecimalUnsignedIntegerValue) {
  setenv("HELLO", "0xabababababababab", 1);
  uint64_t default_value{0xfefefefefefefefeull};
  uint64_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, 0xababababababababull);
}

TEST(BaseGetEnv, UnparsableInteger) {
  setenv("HELLO", "THERE", 1);
  uint32_t default_value{0xfefefefeul};
  uint32_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}

TEST(BaseGetEnv, IntegerWithTrailingGarbage) {
  setenv("HELLO", "12ab", 1);
  uint32_t default_value{0xcdcdcdcdul};
  uint32_t value = base::GetEnv("HELLO", default_value);
  EXPECT_EQ(value, default_value);
}
