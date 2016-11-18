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

#ifndef DTM_TRANSPORT_LOG_SERVER_H_
#define DTM_TRANSPORT_LOG_SERVER_H_

#include <cstddef>
#include <string>
#include "base/macros.h"
#include "base/unix_server_socket.h"
#include "dtm/transport/log_writer.h"

// This class implements a loop that receives log messages over a UNIX socket
// and sends them to a LogWriter instance.
class LogServer {
 public:
  // @a term_fd is a file descriptor that will become readable when the program
  // should exit because it has received the SIGTERM signal.
  explicit LogServer(int term_fd);
  virtual ~LogServer();

  // Run in a loop, receiving log messages over the UNIX socket until the
  // process has received the SIGTERM signal, which is indicated by the caller
  // by making the term_fd (provided in the constructor) readable.
  void Run();

 private:
  int term_fd_;
  base::UnixServerSocket log_socket_;
  LogWriter log_writer_;

  DELETE_COPY_AND_MOVE_OPERATORS(LogServer);
};

#endif  // DTM_TRANSPORT_LOG_SERVER_H_
