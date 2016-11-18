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

#include "base/unix_socket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace base {

UnixSocket::UnixSocket(const std::string& path) :
    fd_{-1},
    addr_{AF_UNIX, {}} {
  if (path.size() < sizeof(addr_.sun_path)) {
    memcpy(addr_.sun_path, path.c_str(), path.size() + 1);
  } else {
    addr_.sun_path[0] = '\0';
  }
}

void UnixSocket::Open() {
  if (fd_ < 0) {
    if (addr_.sun_path[0] != '\0') {
      fd_ = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    } else {
      errno = ENAMETOOLONG;
    }
  }
}

UnixSocket::~UnixSocket() {
  Close();
}

void UnixSocket::Close() {
  if (fd_ >= 0) {
    int e = errno;
    close(fd_);
    errno = e;
    fd_ = -1;
  }
}

}  // namespace base
