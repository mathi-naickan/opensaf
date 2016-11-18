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
#include "election_starter_wrapper.h"
#include "election_starter.h"

void ElectionStarterUpEvent(void* instance, uint32_t node_id,
                            enum ElectionStarterServiceType service) {
  static_cast<ElectionStarter*>(instance)->UpEvent(
      node_id,
      service == ElectionStarterServiceNode ?
          ElectionStarter::Node : ElectionStarter::Controller);
}

void ElectionStarterDownEvent(void* instance, uint32_t node_id,
                              enum ElectionStarterServiceType service) {
  static_cast<ElectionStarter*>(instance)->DownEvent(
      node_id,
      service == ElectionStarterServiceNode ?
          ElectionStarter::Node : ElectionStarter::Controller);
}

struct timespec ElectionStarterPoll(void* instance) {
  return static_cast<ElectionStarter*>(instance)->Poll();
}

void* ElectionStarterConstructor(bool is_nid_started, uint32_t own_node_id) {
  return new ElectionStarter(is_nid_started, own_node_id);
}
