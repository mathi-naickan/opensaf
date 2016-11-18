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

#include "base/unix_client_socket.h"
#include <sys/socket.h>
#include <time.h>
#include <cerrno>

namespace base {

UnixClientSocket::UnixClientSocket(const std::string& path) :
    UnixSocket{path} {
}

UnixClientSocket::~UnixClientSocket() {
}

void UnixClientSocket::Open() {
  if (fd() < 0) {
    UnixSocket::Open();
    if (fd() >= 0) {
      int result;
      int e;
      do {
        result = connect(fd(), addr(), addrlen());
        e = errno;
        if (result != 0 && (e == EALREADY || e == EINPROGRESS)) {
          struct timespec delay{0, 10000000};
          clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, nullptr);
        }
      } while (result != 0 && (e == EINTR || e == EALREADY
                               || e == EINPROGRESS));
      if (result != 0) Close();
    }
  }
}

}  // namespace base
