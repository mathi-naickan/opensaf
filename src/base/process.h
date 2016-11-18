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

#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

#include <csignal>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <mutex>
#include "base/macros.h"

namespace base {

class Process {
  DELETE_COPY_AND_MOVE_OPERATORS(Process);
 public:
  using Duration = std::chrono::steady_clock::duration;
  Process();
  virtual ~Process();
  /**
   * @brief Execute a program with a timeout.
   *
   * Execute a program in a new process group, with the parameters specified in
   * @a argc and @a argv (analogous to the parameters taken by the main()
   * function). The first index in argv must be the full path to the executable
   * file. The rest of the parameters shall specify the arguments to the
   * program. In addition, argv[argc] must be a NULL pointer. This function
   * blocks until the called program has exited or until the @a timeout expires,
   * whichever happens first. If the @a timeout expires, all the processes in
   * the new process group of the started program will be killed by calling the
   * Kill() method of this class with the SIGTERM signal. A @a timeout of zero
   * will give an infinite timeout.
   *
   * The program will be executed with an empty environment containing just a
   * PATH variable with the fixed value set to the following string:
   * /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
   *
   * You should not try to execute several programs concurrently using the same
   * instance of this class, since the timeout handling only supports waiting
   * for one program at a time.
   */
  void Execute(int argc, char *argv[], const Duration& timeout);
  /**
   * @brief Check if there is currently an executing process.
   *
   * Returns true if there is currently an executing process that was started
   * using this instance of the Process class. Note that if the process(es) are
   * currently being killed (i.e. because of a call to the Kill() method or
   * because the timeout has expired), this method may return false even though
   * not all processes have yet been killed.
   */
  bool is_executing() const {
    return process_group_id_ > 1;
  }
  /**
   * @brief Kill the currently executing program.
   *
   * This function kills the currently executing program by calling the
   * KillProc() method on the new process group that was created when starting
   * the program.
   */
  void Kill(int sig_no, const Duration& wait_time);
  /**
   * @brief Send a signal to a process (group) and wait for it to terminate.
   *
   * This is a utility function that works in a similar way as the killproc(8)
   * command. It sends the signal @a sig_no to the process @a pid, and waits for
   * @a wait_time or until the process has terminated, whichever happens
   * first. If @a sig_no is SIGTERM and the process does not terminate before
   * the @a wait_time have passed, this function will retry by sending SIGKILL
   * to @a pid and again waiting @a wait_time for it to terminate.  In the worst
   * case, the total wait time can thus be two times @a wait_time when @a sig_no
   * is SIGTERM; however, in practice the process will terminate very quickly
   * after receiving SIGKILL since this signal cannot be caught, and the the
   * total wait time should thus not be much longer than @a wait_time.
   *
   * It is also possible to use this function to send a signal to a process
   * group, by passing a negative value as @a pid (in a similar way as with the
   * kill(2) function). In that case, this function will wait until all
   * processes in the process group have terminated.
   */
  static void KillProc(pid_t pid, int sig_no, const Duration& wait_time);
 private:
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::steady_clock::time_point;
  void StartTimer(const Duration& timeout);
  void StopTimer();
  static void TimerExpirationEvent(sigval notification_data);
  std::mutex mutex_;
  timer_t timer_id_;
  bool is_timer_created_;
  pid_t process_group_id_;
};

} // namespace base

#endif  // BASE_PROCESS_H_
