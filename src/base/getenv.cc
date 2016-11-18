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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "base/getenv.h"
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <cinttypes>
#include <climits>
#include <string>
#include "base/logtrace.h"

namespace {

uint64_t GetEnvUnsigned(const char* environment_variable,
                        uint64_t default_value, uint64_t max_value);
int64_t GetEnvSigned(const char* environment_variable,
                     int64_t default_value, int64_t max_value,
                     int64_t min_value);
char* RemoveLeadingWhitespace(char* str);

} // namespace

namespace base {

template<>
const char* GetEnv<const char*>(const char* environment_variable,
    const char* default_value) {
  const char* str = getenv(environment_variable);
  if (str == nullptr) {
    TRACE("%s is not set; using default value '%s'", environment_variable,
          default_value);
    str = default_value;
  } else {
    TRACE("%s = '%s'", environment_variable, str);
  }
  return str;
}

template<>
std::string GetEnv<std::string>(const char* environment_variable,
    std::string default_value) {
  return GetEnv(environment_variable, default_value.c_str());
}

template<>
uint64_t GetEnv<uint64_t>(const char* environment_variable,
                          uint64_t default_value) {
  return GetEnvUnsigned(environment_variable, default_value, UINT64_MAX);
}

template<>
uint32_t GetEnv<uint32_t>(const char* environment_variable,
                          uint32_t default_value) {
  return GetEnvUnsigned(environment_variable, default_value, UINT32_MAX);
}

template<>
uint16_t GetEnv<uint16_t>(const char* environment_variable,
                          uint16_t default_value) {
  return GetEnvUnsigned(environment_variable, default_value, UINT16_MAX);
}

template<>
int64_t GetEnv<int64_t>(const char* environment_variable,
                        int64_t default_value) {
  return GetEnvSigned(environment_variable, default_value, INT64_MAX,
                       INT64_MIN);
}

template<>
int32_t GetEnv<int32_t>(const char* environment_variable,
                        int32_t default_value) {
  return GetEnvSigned(environment_variable, default_value, INT32_MAX,
                       INT32_MIN);
}

template<>
int16_t GetEnv<int16_t>(const char* environment_variable,
                        int16_t default_value) {
  return GetEnvSigned(environment_variable, default_value, INT16_MAX,
                       INT16_MIN);
}

} // namespace base

namespace {

uint64_t GetEnvUnsigned(const char* environment_variable,
                        uint64_t default_value, uint64_t max_value) {
  char* str = RemoveLeadingWhitespace(getenv(environment_variable));
  uint64_t retval = default_value;
  if (str != nullptr && *str != '\0') {
    errno = 0;
    char* endptr;
    unsigned long long val = strtoull(str, &endptr, 0);
    endptr = RemoveLeadingWhitespace(endptr);
    if (errno == 0 && val <= max_value && *endptr == '\0') {
      retval = val;
      TRACE("%s = %" PRIu64, environment_variable, retval);
    } else {
      LOG_ER("%s has invalid value '%s'; using default value %" PRIu64,
             environment_variable, str, default_value);
    }
  } else {
    TRACE("%s is %s; using default value %" PRIu64, environment_variable,
          str == nullptr ? "not set" : "empty", default_value);

  }
  return retval;
}

int64_t GetEnvSigned(const char* environment_variable, int64_t default_value,
                     int64_t max_value, int64_t min_value) {
  char* str = RemoveLeadingWhitespace(getenv(environment_variable));
  int64_t retval = default_value;
  if (str != nullptr && *str != '\0') {
    errno = 0;
    char* endptr;
    long long val = strtoll(str, &endptr, 0);
    endptr = RemoveLeadingWhitespace(endptr);
    if (errno == 0 && val >= min_value && val <= max_value && *endptr == '\0') {
      retval = val;
      TRACE("%s = %" PRId64, environment_variable, retval);
    } else {
      LOG_ER("%s has invalid value '%s'; using default value %" PRId64,
             environment_variable, str, default_value);
    }
  } else {
    TRACE("%s is %s; using default value %" PRId64, environment_variable,
          str == nullptr ? "not set" : "empty", default_value);
  }
  return retval;
}

char* RemoveLeadingWhitespace(char* str) {
  if (str != nullptr) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') ++str;
  }
  return str;
}

} // namespace
