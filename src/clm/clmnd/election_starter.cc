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
#include "election_starter.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <set>
#include "base/ncsgl_defs.h"
#include "rde/agent/rda_papi.h"
#include "nid/agent/nid_api.h"
#include "base/logtrace.h"
#include "base/getenv.h"

const char* const ElectionStarter::service_name_[2] = {
  "Node",
  "Controller"
};

ElectionStarter::NodeCollection::NodeCollection()
    : last_change(base::ReadMonotonicClock()) {
}

ElectionStarter::ElectionStarter(bool is_nid_started, uint32_t own_node_id)
    : election_delay_time_(
          base::MillisToTimespec(
              base::GetEnv("CLMNA_ELECTION_DELAY_TIME",
                           kDefaultElectionDelayTime))),
      is_nid_started_{is_nid_started},
      starting_election_{false},
      last_election_start_attempt_(base::ReadMonotonicClock()),
      own_node_id_{own_node_id},
      controller_nodes_{},
      nodes_with_lower_node_id_{},
      nodes_with_greater_or_equal_node_id_{} {
}

void ElectionStarter::StartElection() {
  TRACE_ENTER();
  if (starting_election_ == false) {
    LOG_NO("Starting to promote this node to a system controller");
    starting_election_ = true;
    if (is_nid_started_ && nid_notify(const_cast<char*>("CLMNA"),
                                      NCSCC_RC_SUCCESS, nullptr) !=
        NCSCC_RC_SUCCESS) {
      LOG_ER("nid notify failed");
    }
  }
  SaAmfHAStateT ha_state;
  if (rda_get_role(&ha_state) == NCSCC_RC_SUCCESS &&
      ha_state == SA_AMF_HA_QUIESCED) {
    PCS_RDA_REQ rda_req;
    rda_req.req_type = PCS_RDA_SET_ROLE;
    rda_req.info.io_role = PCS_RDA_ACTIVE;
    if (pcs_rda_request(&rda_req) == PCSRDA_RC_SUCCESS) {
      starting_election_ = false;
    }
  }
  last_election_start_attempt_ = base::ReadMonotonicClock();
  TRACE_LEAVE();
}

ElectionStarter::NodeCollection& ElectionStarter::GetNodeCollection(
    uint32_t node_id, ServiceType service_type) {
  assert(service_type == Controller || service_type == Node);
  if (service_type == Controller) {
    return controller_nodes_;
  } else if (node_id < own_node_id_) {
    return nodes_with_lower_node_id_;
  } else {
    return nodes_with_greater_or_equal_node_id_;
  }
}

void ElectionStarter::UpEvent(uint32_t node_id, ServiceType service_type) {
  TRACE_ENTER2("%s up event on node %" PRIx32, service_name_[service_type],
               node_id);
  NodeCollection& nodes = GetNodeCollection(node_id, service_type);
  std::pair<std::set<uint32_t>::iterator, bool> result = nodes.tree.insert(
      node_id);
  if (result.second) nodes.last_change = base::ReadMonotonicClock();
  TRACE_LEAVE();
}

void ElectionStarter::DownEvent(uint32_t node_id, ServiceType service_type) {
  TRACE_ENTER2("%s down event on node %" PRIx32, service_name_[service_type],
               node_id);
  NodeCollection& nodes = GetNodeCollection(node_id, service_type);
  std::set<uint32_t>::size_type result = nodes.tree.erase(node_id);
  if (result != 0) nodes.last_change = base::ReadMonotonicClock();
  TRACE_LEAVE();
}

timespec ElectionStarter::Poll() {
  TRACE_ENTER();
  timespec timeout = CalculateTimeRemainingUntilNextEvent();
  if (timeout == base::kZeroSeconds) {
    StartElection();
    timeout = base::kOneHundredMilliseconds;
  }
  TRACE_LEAVE2("timeout = %f", base::TimespecToDouble(timeout));
  return timeout;
}

timespec ElectionStarter::CalculateTimeRemainingUntilNextEvent() const {
  timespec timeout;
  if (controller_nodes_.tree.empty()) {
    timespec delay = election_delay_time_;
    // The node with the lowest node-id will be given priority to start an
    // election. After twenty seconds (plus the election delay time) without any
    // controller node, all nodes in the cluster will be allowed to start an
    // election.
    if (!nodes_with_lower_node_id_.tree.empty()) delay += base::kTwentySeconds;
    timeout =
        base::Max(controller_nodes_.last_change + delay,
                  nodes_with_lower_node_id_.last_change + delay,
                  last_election_start_attempt_ + base::kOneHundredMilliseconds);
  } else {
    timeout = base::kTimespecMax;
  }
  return CalculateTimeRemainingUntil(timeout);
}

timespec ElectionStarter::CalculateTimeRemainingUntil(
    const timespec& time_stamp) {
  timespec current_time = base::ReadMonotonicClock();
  if (time_stamp >= current_time) return time_stamp - current_time;
  else return base::kZeroSeconds;
}
