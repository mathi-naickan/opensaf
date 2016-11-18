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

#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/inotify.h>
#include <string>
#include <vector>
#include "base/macros.h"

#ifndef BASE_FILE_NOTIFY_H_
#define BASE_FILE_NOTIFY_H_

namespace base {

class FileNotify {
 public:
  enum class FileNotifyErrors {
    kOK = 0,
    kTimeOut,
    kError,
    kUserFD
  };

  FileNotify();
  virtual ~FileNotify();

  /**
   * @brief Wait for a file to be created with a timeout.
   *
   * @a file name in format /path/file.
   * This function blocks until the file has been created or until the @a timeout expires,
   * whichever happens first.
   *
   */
  FileNotifyErrors
  WaitForFileCreation(const std::string &file_name,
                      const std::vector<int>& user_fds, int timeout);

  FileNotifyErrors
  WaitForFileCreation(const std::string &file_name, int timeout) {
    std::vector<int> empty_user_fds;
    return WaitForFileCreation(file_name, empty_user_fds, timeout);
  }

  /**
   * @brief Wait for a file to be deleted.
   *
   * @a file name in format /path/file.
   * This function blocks until the file has been deleted or until the @a timeout expires,
   * whichever happens first.
   *
   */
  FileNotifyErrors
  WaitForFileDeletion(const std::string &file_name,
                      const std::vector<int>& user_fds, int timeout);

  FileNotifyErrors
    WaitForFileDeletion(const std::string &file_name, int timeout) {
    std::vector<int> empty_user_fds;
    return WaitForFileDeletion(file_name, empty_user_fds, timeout);
  }

 private:
  static const int kBufferSize = 10 * (sizeof (inotify_event) + NAME_MAX + 1);

  FileNotifyErrors ProcessEvents(const std::vector<int>& user_fds, int timeout);

  bool FileExists(const std::string& file_name) const {
    struct stat buffer;
    return stat(file_name.c_str(), &buffer) == 0;
  }

  void SplitFileName(const std::string &file_name);

  char buf_[kBufferSize] __attribute__((aligned(8))) = {};
  std::string file_path_;
  std::string file_name_;

  int inotify_fd_{-1};
  int inotify_wd_{-1};
  DELETE_COPY_AND_MOVE_OPERATORS(FileNotify);
};

}  // namespace base

#endif  // BASE_FILE_NOTIFY_H_
