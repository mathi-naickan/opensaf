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

#ifndef DTM_TRANSPORT_TRANSPORT_MONITOR_H_
#define DTM_TRANSPORT_TRANSPORT_MONITOR_H_

#include <unistd.h>
#include <cstdint>
#include <string>
#include "base/macros.h"
#include "osaf/configmake.h"

// This class is responsible for monitoring the osafdtmd process.
class TransportMonitor {
 public:
  // @a term_fd is a file descriptor that will become readable when the program
  // should exit because it has received the SIGTERM signal.
  explicit TransportMonitor(int term_fd);
  virtual ~TransportMonitor();

  // Wait @a seconds_to_wait seconds for the OpenSAF daemon @daemon_name to
  // start. Return the process id of the daemon process, or pid_t{-1} if the
  // daemon didn't start within @a seconds_to wait or if we have received the
  // SIGTERM signal and should exit.
  pid_t WaitForDaemon(const std::string& daemon_name, int64_t seconds_to_wait);

  // If @a pid_to_watch is non-zero, run in a loop watching the process with
  // that process id. Return if the process dies, or if we have received the
  // SIGTERM signal and should exit. If @a pid_to_watch is zero, return
  // immediately.
  void SuperviseDaemon(pid_t pid_to_watch);

  // Sleep for @a seconds_to_wait seconds, or until we have received the SIGTERM
  // signal and should exit, whichever happens first. Return true if the SIGTERM
  // signal has been received, and false otherwise.
  bool Sleep(int64_t seconds_to_wait);

  // Return true if OpenSAF has been configured to use the TIPC transport, and
  // false if the TCP transport has been configured.
  bool use_tipc() const {
    return use_tipc_;
  }

 private:
  const char* pkgpiddir() const {
    return pkgpiddir_.c_str();
  }

  const char* proc_path() const {
    return proc_path_.c_str();
  }

  const char* mds_log_file() const {
    return mds_log_file_.c_str();
  }

  const char* old_mds_log_file() const {
    return old_mds_log_file_.c_str();
  }

  static bool IsDir(const std::string& path);
  static uint64_t FileSize(const std::string& path);

  int term_fd_;
  const std::string fifo_file_ {PKGLOCALSTATEDIR "/osafdtmd.fifo"};
  const std::string pkgpiddir_;
  const std::string proc_path_;
  const std::string mds_log_file_;
  const std::string old_mds_log_file_;
  const bool use_tipc_;

  DELETE_COPY_AND_MOVE_OPERATORS(TransportMonitor);
};

#endif  // DTM_TRANSPORT_TRANSPORT_MONITOR_H_
