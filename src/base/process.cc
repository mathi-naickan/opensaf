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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "base/process.h"
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "base/logtrace.h"
#include "base/time.h"

namespace {
char path_environment_variable[] = "PATH=/usr/local/sbin:" \
    "/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
}

namespace base {

Process::Process() :
    mutex_{},
    timer_id_{},
    is_timer_created_{false},
    process_group_id_{0} {
}

Process::~Process() {
  StopTimer();
}

void Process::Kill(int sig_no, const Duration& wait_time) {
  std::unique_lock<std::mutex> lock(mutex_);
  pid_t pgid = process_group_id_;
  process_group_id_ = 0;
  lock.unlock();
  if (pgid > 1) KillProc(-pgid, sig_no, wait_time);
}

void Process::KillProc(pid_t pid, int sig_no, const Duration& wait_time) {
  LOG_NO("Sending signal %d to PID %d", sig_no, (int) pid);
  if (kill(pid, sig_no) == 0) {
    TimePoint timeout = Clock::now() + wait_time;
    for (;;) {
      int status;
      pid_t wait_pid = waitpid(pid, &status, WNOHANG);
      if (wait_pid == (pid_t) -1) {
        if (errno == ECHILD) break;
        if (errno != EINTR) osaf_abort(pid);
      }
      if (Clock::now() >= timeout) {
        if (sig_no == SIGTERM) {
          KillProc(pid, SIGKILL, wait_time);
        } else {
          LOG_WA("Failed to kill %d", (int) pid);
        }
        break;
      }
      if (wait_pid == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  } else if (errno == EPERM) {
    LOG_WA("kill(%d, %d) failed due to EPERM", (int) pid, sig_no);
  } else if (errno != ESRCH) {
    osaf_abort(pid);
  }
}

void Process::Execute(int argc, char *argv[], const Duration& timeout) {
  struct rlimit rlim;
  int nofile = 1024;
  char *const env[] = { path_environment_variable, nullptr };

  TRACE_ENTER();
  osafassert(argc >= 1 && argv[argc] == nullptr);
  LOG_NO("Running '%s' with %d argument(s)", argv[0], argc - 1);

  if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
    if (rlim.rlim_cur != RLIM_INFINITY &&
        rlim.rlim_cur <= INT_MAX && (int) rlim.rlim_cur >= 0) {
      nofile = rlim.rlim_cur;
    } else {
      LOG_ER("RLIMIT_NOFILE is out of bounds: %llu",
             (unsigned long long) rlim.rlim_cur);
    }
  } else {
    LOG_ER("getrlimit(RLIMIT_NOFILE) failed: %s", strerror(errno));
  }

  pid_t child_pid = fork();
  if (child_pid == 0) {
    setpgid(0, 0);
    for (int fd = 3; fd < nofile; ++fd) close(fd);
    execve(argv[0], argv, env);
    _Exit(123);
  } else if (child_pid != (pid_t) -1 && child_pid != 1) {
    if (setpgid(child_pid, 0) != 0) {
      LOG_WA("setpgid(%u, 0) failed: %s",
             (unsigned) child_pid, strerror(errno));
    }
    std::unique_lock<std::mutex> lock(mutex_);
    process_group_id_ = child_pid;
    StartTimer(timeout);
    lock.unlock();
    int status;
    pid_t wait_pid;
    do {
      wait_pid = waitpid(child_pid, &status, 0);
    } while (wait_pid == (pid_t) -1 && errno == EINTR);
    lock.lock();
    StopTimer();
    bool killed = process_group_id_ == 0;
    process_group_id_ = 0;
    lock.unlock();
    bool successful = false;
    if (!killed && wait_pid != (pid_t) -1) {
      if (!WIFEXITED(status)) {
        LOG_ER("'%s' terminated abnormally", argv[0]);
      } else if (WEXITSTATUS(status) != 0) {
        if (WEXITSTATUS(status) == 123) {
          LOG_ER("'%s' could not be executed", argv[0]);
        } else {
          LOG_ER("'%s' failed with exit code %d", argv[0],
                 WEXITSTATUS(status));
        }
      } else {
        LOG_IN("'%s' exited successfully", argv[0]);
        successful = true;
      }
    } else if (!killed) {
      LOG_ER("'%s' failed in waitpid(%u): %s",
             argv[0], (unsigned) child_pid, strerror(errno));
    }
    if (!successful) KillProc(-child_pid, SIGTERM, std::chrono::seconds(2));
  } else {
    LOG_ER("'%s' failed in fork(): %s", argv[0],
           strerror(errno));
  }
  TRACE_LEAVE();
}

void Process::StartTimer(const Duration& timeout) {
  if (timeout != std::chrono::seconds(0)) {
    if (!is_timer_created_) {
      sigevent event;
      event.sigev_notify = SIGEV_THREAD;
      event.sigev_value.sival_ptr = this;
      event.sigev_notify_function = TimerExpirationEvent;
      event.sigev_notify_attributes = nullptr;
      int result;
      do {
        result = timer_create(CLOCK_MONOTONIC, &event, &timer_id_);
        if (result == -1 && errno == EAGAIN) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      } while (result == -1 && errno == EAGAIN);
      if (result != 0) osaf_abort(result);
      is_timer_created_ = true;
    }
    itimerspec tmrspec;
    tmrspec.it_interval = kZeroSeconds;
    tmrspec.it_value = DurationToTimespec(timeout);
    if (timer_settime(timer_id_, 0, &tmrspec, nullptr) != 0) osaf_abort(0);
  } else {
    StopTimer();
  }
}

void Process::StopTimer() {
  if (is_timer_created_) {
    if (timer_delete(timer_id_) != 0) osaf_abort(0);
    is_timer_created_ = false;
  }
}

void Process::TimerExpirationEvent(sigval notification_data) {
  LOG_ER("Process execution timed out; sending SIGTERM to the process group");
  Process* instance = static_cast<Process*>(notification_data.sival_ptr);
  instance->Kill(SIGTERM, std::chrono::seconds(2));
}

} // namespace base

