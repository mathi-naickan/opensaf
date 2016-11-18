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

#ifndef CLM_CLMND_ELECTION_STARTER_H_
#define CLM_CLMND_ELECTION_STARTER_H_

#include <stdint.h>
#include <set>
#include "base/macros.h"
#include "base/time.h"

class ElectionStarter {
  DELETE_COPY_AND_MOVE_OPERATORS(ElectionStarter);
 public:
  enum ServiceType {
    /**
     *  A service that runs on all nodes in the cluster, including all system
     *  controller nodes.
     */
    Node,
    /**
     *  A service that only runs on ACTIVE and STANDBY (i.e. not spare) system
     *  controller nodes.
     */
    Controller
  };

  /**
   *  Report the service up event of a service which of type @a service_type on
   *  the node @a node_id. Note that you have to call Poll() after calling this
   *  method.
   */
  ElectionStarter(bool is_nid_started, uint32_t own_node_id);
  /**
   *  Report the service up event of a service which of type @a service_type on
   *  the node @a node_id. Note that you have to call Poll() after calling this
   *  method.
   */
  void UpEvent(uint32_t node_id, ServiceType service_type);
  /**
   *  Report the service down event of a service which of type @a service_type
   *  on the node @a node_id. Note that you have to call Poll() after calling
   *  this method.
   */
  void DownEvent(uint32_t node_id, ServiceType service_type);
  /**
   *  Do the work and return the time you can wait until you need to call Poll()
   *  again Note that you always have to call Poll() immediately after any
   *  UpEvent() or DownEvent(), regardless of the return value from any previous
   *  call to Poll().
   */
  timespec Poll();

 private:
  static const uint64_t kDefaultElectionDelayTime = 200;

  struct NodeCollection {
    NodeCollection();
    std::set<uint32_t> tree;
    timespec last_change;
  };

  void StartElection();
  NodeCollection& GetNodeCollection(uint32_t node_id, ServiceType service);
  timespec CalculateTimeRemainingUntilNextEvent() const;
  static timespec CalculateTimeRemainingUntil(const timespec& time_stamp);

  timespec election_delay_time_;
  bool is_nid_started_;
  bool starting_election_;
  timespec last_election_start_attempt_;
  uint32_t own_node_id_;

  NodeCollection controller_nodes_;
  NodeCollection nodes_with_lower_node_id_;
  NodeCollection nodes_with_greater_or_equal_node_id_;
  static const char* const service_name_[2];
};

#endif  // CLM_CLMND_ELECTION_STARTER_H_
