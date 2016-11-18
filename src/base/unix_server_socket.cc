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

#include "base/unix_server_socket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

namespace base {

UnixServerSocket::UnixServerSocket(const std::string& path) :
    UnixSocket{path} {
}

UnixServerSocket::~UnixServerSocket() {
}

void UnixServerSocket::Open() {
  if (fd() < 0) {
    UnixSocket::Open();
    if (fd() >= 0) {
      int result = bind(fd(), addr(), addrlen());
      if (result != 0) Close();
    }
  }
}

void UnixServerSocket::Close() {
  if (fd() >= 0) {
    int e = errno;
    UnixSocket::Close();
    unlink(path());
    errno = e;
  }
}

}  // namespace base
