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

#ifndef DTM_TRANSPORT_TESTS_MOCK_OSAF_POLL_H_
#define DTM_TRANSPORT_TESTS_MOCK_OSAF_POLL_H_

#include "base/osaf_poll.h"

struct MockOsafPoll {
  MockOsafPoll() :
      return_value{0},
      invocations{0} {
  }
  void reset() {
    return_value = 0;
    invocations = 0;
  }
  int return_value;
  int invocations;
};

extern MockOsafPoll mock_osaf_poll;

#endif  // DTM_TRANSPORT_TESTS_MOCK_OSAF_POLL_H_
