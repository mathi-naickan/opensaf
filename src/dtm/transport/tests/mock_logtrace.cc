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

#include "dtm/transport/tests/mock_logtrace.h"

void _logtrace_log(const char *file, unsigned int line, int priority,
                   const char *format, ...) {
  (void) file;
  (void) line;
  (void) priority;
  (void) format;
}

void _logtrace_trace(const char *file, unsigned int line, unsigned int category,
                     const char *format, ...) {
  (void) file;
  (void) line;
  (void) category;
  (void) format;
}
