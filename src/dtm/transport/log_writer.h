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

#ifndef DTM_TRANSPORT_LOG_WRITER_H_
#define DTM_TRANSPORT_LOG_WRITER_H_

#include <cstddef>
#include <string>
#include "base/macros.h"

// This class is responsible for writing MDS log messages to disk, and rotating
// the log file when it exceeds the maximum file size limit.
class LogWriter {
 public:
  constexpr static const size_t kMaxMessageSize = 2 * size_t{1024};

  LogWriter();
  virtual ~LogWriter();

  char* current_buffer_position() { return buffer_ + current_buffer_size_; }

  // Write @a size bytes of log message data in the memory pointed to by @a
  // buffer to the MDS log file. After the log message has been written, the
  // file will be rotated if necessary. This method performs blocking file I/O.
  void Write(size_t size);
  void Flush();

 private:
  constexpr static const size_t kBufferSize = 128 * size_t{1024};
  constexpr static const size_t kMaxFileSize = 5000 * size_t{1024};
  static size_t FileSize(int fd);
  void Open();
  void Close();
  void RotateLog();

  const char* mds_log_file() const {
    return mds_log_file_.c_str();
  }

  const char* old_mds_log_file() const {
    return old_mds_log_file_.c_str();
  }

  const std::string mds_log_file_;
  const std::string old_mds_log_file_;
  int fd_;
  size_t current_file_size_;
  size_t current_buffer_size_;
  char* buffer_;

  DELETE_COPY_AND_MOVE_OPERATORS(LogWriter);
};

#endif  // DTM_TRANSPORT_LOG_WRITER_H_
