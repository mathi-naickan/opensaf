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

#include "dtm/transport/log_server.h"
#include "base/getenv.h"
#include "base/osaf_poll.h"
#include "base/time.h"
#include "osaf/configmake.h"

LogServer::LogServer(int term_fd)
    : term_fd_{term_fd},
      log_socket_{base::GetEnv<std::string>("pkglocalstatedir",
          PKGLOCALSTATEDIR) + "/mds_log.sock"},
      log_writer_{} {
}

LogServer::~LogServer() {
}

void LogServer::Run() {
  struct pollfd pfd[2] = {
    {term_fd_, POLLIN, 0},
    {log_socket_.fd(), POLLIN, 0}
  };
  struct timespec last_flush = base::ReadMonotonicClock();
  do {
    for (int i = 0; i < 256; ++i) {
      char* buffer = log_writer_.current_buffer_position();
      ssize_t result = log_socket_.Recv(buffer, LogWriter::kMaxMessageSize);
      if (result < 0) break;
      while (result != 0 && buffer[result - 1] == '\n') --result;
      if (static_cast<size_t>(result) != LogWriter::kMaxMessageSize) {
        buffer[result++] = '\n';
      } else {
        buffer[result - 1] = '\n';
      }
      log_writer_.Write(result);
    }
    struct timespec current = base::ReadMonotonicClock();
    if ((current - last_flush) >= base::kFifteenSeconds) {
      log_writer_.Flush();
      last_flush = current;
    }
    struct timespec timeout = (last_flush + base::kFifteenSeconds) - current;
    pfd[1].fd = log_socket_.fd();
    osaf_ppoll(pfd, 2, &timeout, nullptr);
  } while ((pfd[0].revents & POLLIN) == 0);
}
