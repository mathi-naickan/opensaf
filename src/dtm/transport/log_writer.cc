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

#include "dtm/transport/log_writer.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include "base/getenv.h"
#include "osaf/configmake.h"

LogWriter::LogWriter()
    : mds_log_file_{base::GetEnv<std::string>("pkglogdir", PKGLOGDIR)
          + "/mds.log"},
      old_mds_log_file_{mds_log_file_ + ".old"},
      fd_{-1},
      current_file_size_{0},
      current_buffer_size_{0},
      buffer_{new char[kBufferSize]} {
}

LogWriter::~LogWriter() {
  Flush();
  Close();
  delete [] buffer_;
}

void LogWriter::Open() {
  if (fd_ < 0) {
    int fd;
    do {
      fd = open(mds_log_file(), O_WRONLY | O_CLOEXEC | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    } while (fd == -1 && errno == EINTR);
    if (fd >= 0) {
      fd_ = fd;
      current_file_size_ = FileSize(fd);
    }
  }
}

void LogWriter::Close() {
  int fd = fd_;
  if (fd >= 0) {
    close(fd);
    fd_ = -1;
    current_file_size_ = 0;
  }
}

void LogWriter::RotateLog() {
  Close();
  unlink(old_mds_log_file());
  if (rename(mds_log_file(), old_mds_log_file()) != 0) {
    unlink(mds_log_file());
  }
}

void LogWriter::Write(size_t size) {
  current_buffer_size_ += size;
  if (current_buffer_size_ > kBufferSize - kMaxMessageSize) Flush();
}

void LogWriter::Flush() {
  size_t size = current_buffer_size_;
  if (size == 0) return;
  if (fd_ < 0) Open();
  if (fd_ < 0) return;
  if (current_file_size_ >= kMaxFileSize) {
    RotateLog();
    if (fd_ < 0) Open();
    if (fd_ < 0) return;
  }
  size_t bytes_written = 0;
  while (bytes_written < size) {
    ssize_t result = write(fd_, buffer_ + bytes_written, size - bytes_written);
    if (result < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
      break;
    }
    bytes_written += result;
  }
  current_buffer_size_ = 0;
  current_file_size_ += bytes_written;
}

size_t LogWriter::FileSize(int fd) {
  struct stat stat_buf;
  size_t file_size;
  if (fstat(fd, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode) != 0) {
    file_size = static_cast<size_t>(stat_buf.st_blocks) * 512;
  } else {
    file_size = 0;
  }
  return file_size;
}
