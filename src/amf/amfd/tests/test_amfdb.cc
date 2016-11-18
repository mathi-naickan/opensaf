/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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

#include "gtest/gtest.h"
#include "amf/amf_db_template.h"

class TEST_APP {
};

// The fixture for testing class AmfDb
class AmfDbTest : public ::testing::Test {

 protected:

  virtual void SetUp() {

  }

  virtual void TearDown() {
  }

  AmfDb<std::string, TEST_APP> db_;

};

TEST_F(AmfDbTest, IsEmptyInitially) {
  std::map<std::string, TEST_APP*>::const_iterator it = db_.begin();
  EXPECT_EQ(it, db_.end());
}

TEST_F(AmfDbTest, InsertWorks) {
  unsigned int rc = 0;
  TEST_APP *app = new TEST_APP;
  std::string str("app1");
  rc = db_.insert(str, app);
  EXPECT_EQ(1U, rc);
}
