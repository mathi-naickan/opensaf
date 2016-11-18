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

#include "base/log_message.h"
#include <time.h>
#include <cstdint>
#include "base/buffer.h"
#include "base/osaf_time.h"
#include "base/time.h"

namespace base {

const struct timespec LogMessage::kNullTime{0, -1};

void LogMessage::Write(Facility facility, Severity severity,
                       const struct timespec& time_stamp,
                       const HostName& host_name,
                       const AppName& app_name,
                       const ProcId& proc_id,
                       const MsgId& msg_id,
                       const StructuredElements& structured_elements,
                       const std::string& message,
                       Buffer* buffer) {
  uint32_t priority = static_cast<uint32_t>(facility) * uint32_t{8}
      + static_cast<uint32_t>(severity);
  buffer->AppendChar('<');
  buffer->AppendNumber(priority, 100);
  buffer->AppendString(">1 ", 3);
  WriteTime(time_stamp, buffer);
  buffer->AppendChar(' ');
  buffer->AppendString(host_name.data(), host_name.size());
  buffer->AppendChar(' ');
  buffer->AppendString(app_name.data(), app_name.size());
  buffer->AppendChar(' ');
  buffer->AppendString(proc_id.data(), proc_id.size());
  buffer->AppendChar(' ');
  buffer->AppendString(msg_id.data(), msg_id.size());
  buffer->AppendChar(' ');
  if (structured_elements.empty()) {
    buffer->AppendChar('-');
  } else {
    for (const auto& elem : structured_elements) elem.Write(buffer);
  }
  if (!message.empty()) {
    buffer->AppendChar(' ');
    buffer->AppendString(message.data(), message.size());
  }
}

void LogMessage::WriteTime(const struct timespec& ts, Buffer* buffer) {
  struct tm local_time;
  struct tm* local_ptr = localtime_r(&ts.tv_sec, &local_time);
  if (local_ptr != nullptr && local_ptr->tm_year >= -1900
      && local_ptr->tm_year <= (9999 - 1900)
      && ts.tv_nsec >= 0 && ts.tv_nsec < kNanosPerSec) {
    buffer->AppendFixedWidthNumber(local_ptr->tm_year + 1900, 1000);
    buffer->AppendChar('-');
    buffer->AppendFixedWidthNumber(local_ptr->tm_mon + 1, 10);
    buffer->AppendChar('-');
    buffer->AppendFixedWidthNumber(local_ptr->tm_mday, 10);
    buffer->AppendChar('T');
    buffer->AppendFixedWidthNumber(local_ptr->tm_hour, 10);
    buffer->AppendChar(':');
    buffer->AppendFixedWidthNumber(local_ptr->tm_min, 10);
    buffer->AppendChar(':');
    buffer->AppendFixedWidthNumber(local_ptr->tm_sec, 10);
    uint32_t decimals = ts.tv_nsec / 1000;
    if (decimals != 0) {
      uint32_t power = 100000;
      while (decimals % 10 == 0) {
        decimals /= 10;
        power /= 10;
      }
      buffer->AppendChar('.');
      buffer->AppendFixedWidthNumber(decimals, power);
    }
    char* buf = buffer->end();
    if ((buffer->capacity() - buffer->size()) >= 6 &&
        strftime(buf, 6, "%z", local_ptr) == 5) {
      if (buf[1] != '0' || buf[2] != '0' ||
          buf[3] != '0' || buf[4] != '0') {
        buf[5] = buf[4];
        buf[4] = buf[3];
        buf[3] = ':';
        buffer->set_size(buffer->size() + 6);
      } else {
        buffer->AppendChar('Z');
      }
    }
  } else {
    buffer->AppendChar('-');
  }
}

void LogMessage::Element::Write(Buffer* buffer) const {
  buffer->AppendChar('[');
  buffer->AppendString(id_.data(), id_.size());
  for (const Parameter& param : parameter_list_) {
    buffer->AppendChar(' ');
    param.Write(buffer);
  }
  buffer->AppendChar(']');
}

void LogMessage::Parameter::Write(Buffer* buffer) const {
  buffer->AppendString(name_.data(), name_.size());
  buffer->AppendChar('=');
  buffer->AppendChar('"');
  for (const char& c : value_) {
    if (c == '"' || c == '\\' || c == ']') buffer->AppendChar('\\');
    buffer->AppendChar(c);
  }
  buffer->AppendChar('"');
}

}  // namespace base
