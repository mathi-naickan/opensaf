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

#include <pthread.h>
#include <sched.h>
#include <syslog.h>
#include <cerrno>
#include <cstdlib>
#include "base/daemon.h"
#include "base/ncssysf_def.h"
#include "dtm/transport/log_server.h"
#include "dtm/transport/transport_monitor.h"

constexpr static const int kDaemonStartWaitTimeInSeconds = 15;

enum Termination {
  kExit,
  kDaemonExit,
  kReboot
};

struct Result {
  Result(Termination t, const char* m, int e) :
      term{t}, msg{m}, error{e} {}
  Termination term;
  const char* msg;
  int error;
};

static void* LogServerStartFunction(void* instance) {
  LogServer* log_server = static_cast<LogServer*>(instance);
  log_server->Run();
  return nullptr;
}

Result MainFunction(int term_fd) {
  pthread_attr_t attr;
  int result = pthread_attr_init(&attr);
  if (result != 0) return Result{kExit, "pthread_attr_init() failed", result};
  result = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  if (result != 0) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "pthread_attr_setscope() failed", result};
  }
  result = pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
  if (result != 0) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "pthread_attr_setschedpolicy() failed", result};
  }
  struct sched_param param{};
  param.sched_priority = sched_get_priority_min(SCHED_OTHER);
  if (param.sched_priority == -1) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "sched_get_priority_min() failed", errno};
  }
  result = pthread_attr_setschedparam(&attr, &param);
  if (result != 0) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "pthread_attr_setschedparam() failed", result};
  }
  result = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  if (result != 0) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "pthread_attr_setinheritsched() failed", result};
  }
  LogServer log_server{term_fd};
  pthread_t thread_id;
  result = pthread_create(&thread_id, &attr,
                          LogServerStartFunction, &log_server);
  if (result != 0) {
    pthread_attr_destroy(&attr);
    return Result{kExit, "pthread_create() failed", result};
  }
  result = pthread_attr_destroy(&attr);
  if (result != 0) return Result{kExit, "pthread_attr_destroy() failed", result};
  TransportMonitor monitor{term_fd};
  if (!monitor.use_tipc()) {
    pid_t pid = monitor.WaitForDaemon("osafdtmd", kDaemonStartWaitTimeInSeconds);
    if (pid == pid_t{-1}) return Result{kReboot, "osafdtmd failed to start", 0};
    monitor.SuperviseDaemon(pid);
    if (!monitor.Sleep(0)) return Result{kReboot,
                                 "osafdtmd Process down, Rebooting the node", 0};
  }
  result = pthread_join(thread_id, nullptr);
  if (result != 0) syslog(LOG_ERR, "pthread_join() failed: %d", result);
  return Result{kDaemonExit, "SIGTERM received, exiting", 0};
}

int main(int argc, char** argv) {
  opensaf_reboot_prepare();
  daemonize(argc, argv);
  int term_fd;
  daemon_sigterm_install(&term_fd);
  Result result = MainFunction(term_fd);
  switch (result.term) {
    case kExit:
      if (result.error != 0) {
        syslog(LOG_ERR, "%s: %d", result.msg, result.error);
      } else {
        syslog(LOG_ERR, "%s", result.msg);
      }
      break;
    case kDaemonExit:
      daemon_exit();
      break;
    case kReboot:
      opensaf_reboot(0, nullptr, result.msg);
      break;
  }
  return EXIT_FAILURE;
}
