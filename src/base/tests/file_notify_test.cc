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
#include "gtest/gtest.h"

class FileNotifyTest : public ::testing::Test {
 protected:
  FileNotifyTest() {
    // Setup work can be done here for each test.
  }

  virtual ~FileNotifyTest() {
    // Cleanup work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  // cppcheck-suppress unusedFunction
  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  // cppcheck-suppress unusedFunction
  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case.
  base::FileNotify::FileNotifyErrors rc{base::FileNotify::FileNotifyErrors::kOK};
};

//
TEST_F(FileNotifyTest, TestNonExistingPathCreation) {
  base::FileNotify file_notify;
  std::string non_existing_file = "/a/b/c";

  rc = file_notify.WaitForFileCreation(non_existing_file, 10);
  ASSERT_EQ(rc, base::FileNotify::FileNotifyErrors::kError);
}

//
TEST_F(FileNotifyTest, TestExistingFileCreation) {
  base::FileNotify file_notify;
  std::string existing_file = __FILE__;

  rc = file_notify.WaitForFileCreation(existing_file, 10);
  ASSERT_EQ(rc, base::FileNotify::FileNotifyErrors::kOK);
}

//
TEST_F(FileNotifyTest, TestNonExistingPathDeletion) {
  base::FileNotify file_notify;
  std::string non_existing_path = "/a/b/c";

  rc = file_notify.WaitForFileDeletion(non_existing_path, 10);
  ASSERT_EQ(rc, base::FileNotify::FileNotifyErrors::kOK);
}

TEST_F(FileNotifyTest, TestExistingFileDeletion) {
  base::FileNotify file_notify;
  std::string existing_file = __FILE__;

  rc = file_notify.WaitForFileDeletion(existing_file, 10);
  ASSERT_EQ(rc, base::FileNotify::FileNotifyErrors::kTimeOut);
}
