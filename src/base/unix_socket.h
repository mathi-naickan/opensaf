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

#ifndef BASE_UNIX_SOCKET_H_
#define BASE_UNIX_SOCKET_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include <string>
#include "base/macros.h"

namespace base {

// A class implementing non-blocking operations on a UNIX domain socket.
class UnixSocket {
 public:
  // Close the socket.
  virtual ~UnixSocket();
  // Send a message in non-blocking mode. This call will open the socket if it
  // was not already open. The EINTR error code from the send() libc function is
  // handled by retrying the send() call in a loop. In case of other errors, the
  // socket will be closed.
  ssize_t Send(const void* buffer, size_t length) {
    if (fd_ < 0) Open();
    ssize_t result = -1;
    if (fd_ >= 0) {
      do {
        result = send(fd_, buffer, length, MSG_NOSIGNAL);
      } while (result < 0 && errno == EINTR);
      if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
    }
    return result;
  }
  // Receive a message in non-blocking mode. This call will open the socket if
  // it was not already open. The EINTR error code from the recv() libc function
  // is handled by retrying the recv() call in a loop. In case of other errors,
  // the socket will be closed.
  ssize_t Recv(void* buffer, size_t length) {
    if (fd_ < 0) Open();
    ssize_t result = -1;
    if (fd_ >= 0) {
      do {
        result = recv(fd_, buffer, length, 0);
      } while (result < 0 && errno == EINTR);
      if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
    }
    return result;
  }
  // Returns the current file descriptor for this UNIX socket, or -1 if the
  // socket is currently not open. Note that the Send() and Recv() methods may
  // open and/or close the socket, and potentially the file descriptor will be
  // different after a call to any of these two methods.
  int fd() const { return fd_; }

 protected:
  explicit UnixSocket(const std::string& path);
  virtual void Open();
  virtual void Close();
  const struct sockaddr* addr() const {
    return reinterpret_cast<const struct sockaddr*>(&addr_);
  }
  static socklen_t addrlen() { return sizeof(addr_); }
  const char* path() const { return addr_.sun_path; }

 private:
  int fd_;
  struct sockaddr_un addr_;

  DELETE_COPY_AND_MOVE_OPERATORS(UnixSocket);
};

}  // namespace base

#endif  // BASE_UNIX_SOCKET_H_
