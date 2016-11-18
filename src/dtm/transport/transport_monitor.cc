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

#include "dtm/transport/transport_monitor.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include "base/file_notify.h"
#include "base/getenv.h"
#include "base/logtrace.h"
#include "base/osaf_poll.h"
#include "base/osaf_time.h"

TransportMonitor::TransportMonitor(int term_fd)
    : term_fd_{term_fd},
      pkgpiddir_{base::GetEnv<std::string>("pkgpiddir", PKGPIDDIR)},
      proc_path_{"/proc/"},
      use_tipc_{base::GetEnv<std::string>("MDS_TRANSPORT", "TCP") == "TIPC"} {
}

TransportMonitor::~TransportMonitor() {
}

pid_t TransportMonitor::WaitForDaemon(const std::string& daemon_name,
                                      int64_t seconds_to_wait) {
  std::vector<int> user_fds {term_fd_};
  std::string pidfile = pkgpiddir_ + "/" + daemon_name + ".pid";
  pid_t pid = pid_t{-1};
  base::FileNotify file_notify;
  base::FileNotify::FileNotifyErrors rc =
      file_notify.WaitForFileCreation(pidfile, user_fds,
                                      seconds_to_wait * kMillisPerSec);

  if (rc == base::FileNotify::FileNotifyErrors::kOK) {
    if (!(std::ifstream{pidfile} >> pid).fail()) {
      if (IsDir(proc_path_ + std::to_string(pid))) {
        return pid;
      }
    }
  }
  return pid_t{-1};
}

bool TransportMonitor::Sleep(int64_t seconds_to_wait) {
  return osaf_poll_one_fd(term_fd_, seconds_to_wait * kMillisPerSec) != 0;
}

bool TransportMonitor::IsDir(const std::string& path) {
  struct stat stat_buf;
  int stat_result = stat(path.c_str(), &stat_buf);
  return stat_result == 0 && S_ISDIR(stat_buf.st_mode) != 0;
}

void TransportMonitor::SuperviseDaemon(pid_t pid_to_watch) {
  if (pid_to_watch == pid_t{0}) return;
  int fifo_fd{-1};
  std::vector<int> user_fds {term_fd_};
  base::FileNotify file_notify;
  std::string pid_path{proc_path_ + std::to_string(pid_to_watch)};

  do {
    fifo_fd = open(fifo_file_.c_str(), O_WRONLY|O_NONBLOCK);
  } while (fifo_fd == -1 && errno == EINTR);

  if (fifo_fd == -1) {
    LOG_WA("open fifo file failed");
  } else {
    user_fds.emplace_back(fifo_fd);
  }

  file_notify.WaitForFileDeletion(pid_path, user_fds, -1);
  close(fifo_fd);
}
