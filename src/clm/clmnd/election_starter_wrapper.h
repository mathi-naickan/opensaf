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

#ifndef CLM_CLMND_ELECTION_STARTER_WRAPPER_H_
#define CLM_CLMND_ELECTION_STARTER_WRAPPER_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef  __cplusplus
extern "C" {
#endif

enum ElectionStarterServiceType {
  ElectionStarterServiceNode,
  ElectionStarterServiceController
};

void ElectionStarterUpEvent(void* instance, uint32_t node_id,
                            enum ElectionStarterServiceType service);
void ElectionStarterDownEvent(void* instance, uint32_t node_id,
                              enum ElectionStarterServiceType service);
struct timespec ElectionStarterPoll(void* instance);
void* ElectionStarterConstructor(bool is_nid_started, uint32_t own_node_id);

#ifdef  __cplusplus
}
#endif

#endif  // CLM_CLMND_ELECTION_STARTER_WRAPPER_H_
