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

#include "base/file_notify.h"
#include <libgen.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include "time.h"
#include "osaf_poll.h"
#include "base/logtrace.h"

namespace base {

FileNotify::FileNotify() {
  if ((inotify_fd_ = inotify_init()) == -1) {
    LOG_NO("inotify_init failed: %s", strerror(errno));
  }
}

FileNotify::~FileNotify() {
  close(inotify_fd_);
}

void FileNotify::SplitFileName(const std::string &file_name) {
  char *tmp1 = strdup(file_name.c_str());
  char *tmp2 = strdup(file_name.c_str());
  file_path_ = dirname(tmp1);
  file_name_ = basename(tmp2);
  free(tmp1);
  free(tmp2);
}

FileNotify::FileNotifyErrors
FileNotify::WaitForFileCreation(const std::string &file_name,
                                const std::vector<int>& user_fds, int timeout) {
  FileNotify::FileNotifyErrors rc {FileNotifyErrors::kOK};
  SplitFileName(file_name);

  if ((inotify_wd_ =
       inotify_add_watch(inotify_fd_, file_path_.c_str(), IN_CREATE)) == -1) {
    LOG_NO("inotify_add_watch failed: %s", strerror(errno));
    return FileNotifyErrors::kError;
  }

  if (FileExists(file_name)) {
    TRACE("File already created: %s", file_name.c_str());
    inotify_rm_watch(inotify_fd_, inotify_wd_);
    return FileNotifyErrors::kOK;
  }

  rc = ProcessEvents(user_fds, timeout);
  inotify_rm_watch(inotify_fd_, inotify_wd_);
  return rc;
}

FileNotify::FileNotifyErrors
FileNotify::WaitForFileDeletion(const std::string &file_name,
                                const std::vector<int>& user_fds,
                                int timeout) {
  FileNotify::FileNotifyErrors rc {FileNotifyErrors::kOK};

  if ((inotify_wd_ = inotify_add_watch(inotify_fd_, file_name.c_str(),
                                       IN_DELETE_SELF)) == -1) {
    if (errno == ENOENT) {
      TRACE("File already deleted: %s", file_name.c_str());
      return FileNotifyErrors::kOK;
    } else {
      LOG_NO("inotify_add_watch failed: %s", strerror(errno));
      return FileNotifyErrors::kError;
    }
  }

  rc = ProcessEvents(user_fds, timeout);
  inotify_rm_watch(inotify_fd_, inotify_wd_);
  return rc;
}

FileNotify::FileNotifyErrors
FileNotify::ProcessEvents(const std::vector<int>& user_fds, int timeout) {
  enum {
    FD_INOTIFY = 0,
  };

  timespec start_time {0};
  timespec time_left_ts {0};
  timespec timeout_ts {0};

  int num_of_fds = user_fds.size() + 1;
  pollfd* fds = new pollfd[num_of_fds];

  fds[FD_INOTIFY].fd = inotify_fd_;
  fds[FD_INOTIFY].events = POLLIN;
  for (int i = 1; i < num_of_fds; ++i) {
    fds[i].fd = user_fds[i - 1];
    fds[i].events = POLLIN;
  }

  timeout_ts = base::MillisToTimespec(timeout);
  start_time = base::ReadMonotonicClock();

  while (true) {
    TRACE("remaining timeout: %d", timeout);
    unsigned rc = osaf_poll(fds, num_of_fds, timeout);

    if (rc > 0) {
      if (fds[FD_INOTIFY].revents & POLLIN) {
        ssize_t num_read = read(inotify_fd_, buf_, kBufferSize);

        if (num_read == 0) {
          LOG_WA("read returned zero");
          delete [] fds;
          return FileNotifyErrors::kError;
        } else if (num_read == -1) {
          if (errno == EINTR) {
            continue;
          } else {
            LOG_WA("read error: %s", strerror(errno));
            delete [] fds;
            return FileNotifyErrors::kError;
          }
        } else {
          for (char *p = buf_; p < buf_ + num_read;) {
            inotify_event *event = reinterpret_cast<inotify_event*> (p);
            if (event->mask & IN_DELETE_SELF) {
              delete [] fds;
              return FileNotifyErrors::kOK;
            }
            if (event->mask & IN_CREATE) {
              if (file_name_ == event->name) {
                TRACE("file name: %s created", file_name_.c_str());
                delete [] fds;
                return FileNotifyErrors::kOK;
              }
            }
            if (event->mask & IN_IGNORED) {
              TRACE("IN_IGNORE received, (ignored)");
            }
            p += sizeof(inotify_event) + event->len;
          }
          if (timeout != -1) {
            // calculate remaining timeout only if timeout is not infinite (-1)
            timespec current_time {0};
            timespec elapsed_time {0};

            current_time = base::ReadMonotonicClock();

            if (current_time >= start_time) {
              elapsed_time = current_time - start_time;
            }
            if (elapsed_time >= timeout_ts) {
              return FileNotifyErrors::kTimeOut;
            }
            time_left_ts = timeout_ts - elapsed_time;

            timeout = base::TimespecToMillis(time_left_ts);
          }
        }
      }
      // Check user file descriptors
      for (int i = 1; i < num_of_fds; i++) {
        if ((fds[i].revents & POLLIN) ||
            (fds[i].revents & POLLHUP) ||
            (fds[i].revents & POLLERR)) {
          delete [] fds;
          return FileNotifyErrors::kUserFD;
        }
      }
    } else if (rc == 0) {
        TRACE("timeout");
        delete [] fds;
        return FileNotifyErrors::kTimeOut;
    }
  }
}
}  // namespace base
