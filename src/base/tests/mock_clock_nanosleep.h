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

#ifndef BASE_TESTS_MOCK_CLOCK_NANOSLEEP_H_
#define BASE_TESTS_MOCK_CLOCK_NANOSLEEP_H_

#include <time.h>

struct MockClockNanosleep {
  int return_value;
};

static const int kMockClockNanosleepInstances = 3;
extern int current_nanosleep_index;
extern int number_of_nanosleep_instances;
extern MockClockNanosleep mock_clock_nanosleep[kMockClockNanosleepInstances];

#endif  // BASE_TESTS_MOCK_CLOCK_NANOSLEEP_H_
