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

#include <limits.h>
#include <time.h>
#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"
#include "base/buffer.h"
#include "base/log_message.h"

static const struct timespec time_stamp_1 = { 1475749599, 497243812 };
static const struct timespec time_stamp_2 = { 1458469599, 97240812 };

static const char hello_world_text_1[] = "<133>1 2016-10-06T12:26:39.497243"
    "+02:00 PL-42 log_message_test 24744 4711 "
    "[elem1 param1=\"value1\" param2=\"value2\"][elem2] hello, world!";

static const char hello_world_text_2[] = "<191>1 2016-03-20T11:26:39.09724"
    "+01:00 dark_star - _ 12345678901234567890123456789012 "
    "[- -=\"\" a_b_c=\"d=e\\]f\"][_]  test ";

static const char null_text[] = "<0>1 - - - - - -";

TEST(LogMessageTest, WriteHelloWorld1) {
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf{256};
  base::LogMessage::Write(
      base::LogMessage::Facility::kLocal0,
      base::LogMessage::Severity::kNotice,
      time_stamp_1,
      base::LogMessage::HostName{"PL-42"},
      base::LogMessage::AppName{"log_message_test"},
      base::LogMessage::ProcId{"24744"},
      base::LogMessage::MsgId{"4711"},
      {{base::LogMessage::SdName{"elem1"},
          {base::LogMessage::Parameter{base::LogMessage::SdName{"param1"},
                  "value1"},
                base::LogMessage::Parameter{base::LogMessage::SdName{"param2"},
                      "value2"}}},
        {base::LogMessage::SdName{"elem2"},
          {}}},
      "hello, world!",
      &buf);
  EXPECT_EQ(buf.size(), sizeof(hello_world_text_1) - 1);
  EXPECT_EQ(memcmp(buf.data(),
                   hello_world_text_1,
                   sizeof(hello_world_text_1) - 1), 0);
}

TEST(LogMessageTest, WriteHelloWorld2) {
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf{256};
  base::LogMessage::Write(
      base::LogMessage::Facility::kLocal7,
      base::LogMessage::Severity::kDebug,
      time_stamp_2,
      base::LogMessage::HostName{"dark star"},
      base::LogMessage::AppName{""},
      base::LogMessage::ProcId{"_"},
      base::LogMessage::MsgId{"123456789012345678901234567890123"},
      {{base::LogMessage::SdName{""},
          {base::LogMessage::Parameter{base::LogMessage::SdName{""},
                  ""},
                base::LogMessage::Parameter{base::LogMessage::SdName{"a=b]c"},
                      "d=e]f"}}},
        {base::LogMessage::SdName{"_"},
          {}}},
      " test ",
      &buf);
  EXPECT_EQ(buf.size(), sizeof(hello_world_text_2) - 1);
  EXPECT_EQ(memcmp(buf.data(),
                   hello_world_text_2,
                   sizeof(hello_world_text_2) - 1), 0);
}

TEST(LogMessageTest, WriteNullMessage) {
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf(256);
  base::LogMessage::Write(
      base::LogMessage::Facility::kKern,
      base::LogMessage::Severity::kEmerg,
      base::LogMessage::kNullTime,
      base::LogMessage::HostName{""},
      base::LogMessage::AppName{""},
      base::LogMessage::ProcId{""},
      base::LogMessage::MsgId{""},
      {},
      "",
      &buf);

  EXPECT_EQ(buf.size(), sizeof(null_text) - 1);
  EXPECT_EQ(memcmp(buf.data(),
                   null_text,
                   sizeof(null_text) - 1), 0);
}

#if __WORDSIZE >= 64
TEST(LogMessageTimeTest, WriteBCTime) {
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf(256);
  base::LogMessage::WriteTime({-62188992000, 0}, &buf);
  EXPECT_EQ(buf.size(), 1);
  EXPECT_EQ(memcmp(buf.data(), "-", 1), 0);
}

TEST(LogMessageTimeTest, WriteFarFutureTime) {
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf(256);
  base::LogMessage::WriteTime({253423296000, 0}, &buf);
  EXPECT_EQ(buf.size(), 1);
  EXPECT_EQ(memcmp(buf.data(), "-", 1), 0);
}
#endif

TEST(LogMessageTimeTest, WriteTimeInSmallBuffer) {
  static const char expected_result[] = "2016-10-10T16:24:02.505489";
  setenv("TZ", "Europe/Stockholm", 1);
  tzset();
  base::Buffer buf(30);
  base::LogMessage::WriteTime({1476109442, 505489184}, &buf);
  EXPECT_EQ(buf.size(), sizeof(expected_result) - 1);
  EXPECT_EQ(memcmp(buf.data(), expected_result, buf.size()), 0);
}

TEST(LogMessageTimeTest, WriteTimeWithUtcTimeZone) {
  static const char expected_result[] = "2016-10-10T14:33:32.82328Z";
  setenv("TZ", "UTC", 1);
  tzset();
  base::Buffer buf(256);
  base::LogMessage::WriteTime({1476110012, 823280288}, &buf);
  EXPECT_EQ(buf.size(), sizeof(expected_result) - 1);
  EXPECT_EQ(memcmp(buf.data(), expected_result, buf.size()), 0);
}
