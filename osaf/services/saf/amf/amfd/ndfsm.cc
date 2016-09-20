/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************

  DESCRIPTION: This file contains the node state machine related functional
  routines. It is part of the Node submodule.

******************************************************************************
*/

#include <logtrace.h>

#include <amfd.h>
#include <cluster.h>
#include <daemon.h>
#include <algorithm>

AmfDb<uint32_t, AVD_FAIL_OVER_NODE> *node_list_db = 0;      /* SaClmNodeIdT index */

/*****************************************************************************
 * Function: avd_process_state_info_queue
 *
 * Purpose:  This function will pull out the queue event and looking for sync
 * info (sisu, compcsi) event to recover the SI/CSI assignment
 *
 * Input: cb - the AVD control block
 *
 * Returns: None.
 *
 * NOTES:
 *
 *
 **************************************************************************/
void avd_process_state_info_queue(AVD_CL_CB *cb)
{
	uint32_t i;
	const auto queue_size = cb->evt_queue.size();
	AVD_EVT_QUEUE *queue_evt = nullptr;
	/* Counter for Act Amfnd node up message.*/
	static int act_amfnd_node_up_count = 0;
	bool found_state_info = false;
	TRACE_ENTER();

	TRACE("queue_size before processing: %lu", (unsigned long) queue_size);
	act_amfnd_node_up_count ++;
	// recover assignments from state info
	for(i=0 ; i<queue_size ; i++) {
		queue_evt = cb->evt_queue.front();
		osafassert(queue_evt->evt);
		cb->evt_queue.pop();

		TRACE("rcv_evt: %u", queue_evt->evt->rcv_evt);

		if (queue_evt->evt->rcv_evt == AVD_EVT_ND_SISU_STATE_INFO_MSG ||
			queue_evt->evt->rcv_evt == AVD_EVT_ND_CSICOMP_STATE_INFO_MSG) {

			AVD_DND_MSG* n2d_msg = queue_evt->evt->info.avnd_msg;

			TRACE("msg_type: %u", n2d_msg->msg_type);
			found_state_info = true;
			switch(n2d_msg->msg_type) {
				case AVSV_N2D_ND_SISU_STATE_INFO_MSG:
					avd_susi_recreate(&n2d_msg->msg_info.n2d_nd_sisu_state_info);
					break;
				case AVSV_N2D_ND_CSICOMP_STATE_INFO_MSG:
					avd_compcsi_recreate(&n2d_msg->msg_info.n2d_nd_csicomp_state_info);
					break;
				default:
					break;
			}

			avsv_dnd_msg_free(n2d_msg);

			delete queue_evt->evt;
			delete queue_evt;
		} else {
			cb->evt_queue.push(queue_evt);
		}
	}

	/* Alarms shouldn't be reset in next subsequent node up message.
	   Because in the previous node up messages queue_size might have
	   been zero. In the subsequent node up messages, this might cause
	   alarm_sent to get reset and this may cause unassigned alarm to
	   exist even those SIs are assigned after some time.*/
	if (act_amfnd_node_up_count > 1) goto done;

	// Once active amfd looks up the state info from queue, that means node sync
	// finishes. Therefore, if the queue is empty, this active amfd is coming
	// from a cluster restart, the alarm state should be reset.
	// Otherwise, amfd is coming from SC recovery from headless, SI alarm state
	// should be re-evalutated and raise the alarm in case it's still unassigned.
	if (queue_size == 0) {
		for (std::map<std::string, AVD_SI*>::const_iterator it = si_db->begin();
				it != si_db->end(); it++) {
			AVD_SI *si = it->second;
			if (si->alarm_sent == true) {
				si->update_alarm_state(false, false);
			}
		}
	}
	else {
		for (std::map<std::string, AVD_SI*>::const_iterator it = si_db->begin();
				it != si_db->end(); it++) {
			AVD_SI *si = it->second;
			if (si->alarm_sent == false &&
					si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_UNASSIGNED) {
				si->update_alarm_state(true);
			}
		}
	}

	// Read cached rta from Imm
	// Reading sg must be after reading susi
	if (found_state_info == true) {
		avd_compcsi_cleanup_imm_object(cb);
		avd_sg_read_headless_fsm_state_cached_rta(cb);
		avd_susi_read_headless_cached_rta(cb);
		avd_sg_read_headless_su_oper_list_cached_rta(cb);
		avd_su_read_headless_cached_rta(cb);
	}
done:
	TRACE("queue_size after processing: %lu", (unsigned long) cb->evt_queue.size());
	TRACE_LEAVE();
}
/*****************************************************************************
 * Function: avd_count_sync_node_size
 *
 * Purpose:  Helper function count the maximum number of node in cluster
 *           to be synced from headless
 *
 * Input: cb - the AVD control block
 *
 * Returns: Number of nd
 *
 * NOTES:
 *
 **************************************************************************/
uint32_t avd_count_sync_node_size(AVD_CL_CB *cb)
{
	uint32_t twon_ncs_su_count = 0;
	uint32_t count = 0;
	TRACE_ENTER();

	for (std::map<std::string, AVD_AVND *>::const_iterator it = node_name_db->begin();
			it != node_name_db->end(); it++) {
		AVD_AVND *avnd = it->second;
		osafassert(avnd);
		for (const auto& su :avnd->list_of_ncs_su) {
			if (su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) {
				twon_ncs_su_count++;
				continue;
			}
		}
	}
	// cluster can have 1 SC or more SCs which hosting 2N Opensaf SU
	// so twon_ncs_su_count at least is 1
	osafassert(twon_ncs_su_count > 0);

	if (twon_ncs_su_count == 1) {
		// 1 SC, the rest of nodes could be in sync from headless
		count = node_name_db->size() - 1;
	} else {
		// >=2 SCs, the rest of nodes could be in sync except active/standby SC
		count = node_name_db->size() - 2;
	}

	TRACE("sync node size:%d", count);
	TRACE_LEAVE();
	return count;
}
/*****************************************************************************
 * Function: avd_count_node_up
 *
 * Purpose:  Helper function count number of nodes that sent node_up msg to
 *           director
 *
 * Input: cb - the AVD control block
 *
 * Returns: Number of node
 *
 * NOTES:
 *
 *
 **************************************************************************/
uint32_t avd_count_node_up(AVD_CL_CB *cb)
{
	uint32_t received_count = 0;
	AVD_AVND *node = nullptr;

	TRACE_ENTER();

	for (std::map<std::string, AVD_AVND *>::const_iterator it = node_name_db->begin();
			it != node_name_db->end(); it++) {
		node = it->second;
		if (node->node_up_msg_count > 0
				&& node->node_info.nodeId != cb->node_id_avd
				&& node->node_info.nodeId != cb->node_id_avd_other)
			++received_count;
	}
	TRACE("Number of node director(s) that director received node_up msg:%u",
			received_count);

	TRACE_LEAVE();
	return received_count;
}

/*****************************************************************************
 * Function: record_node_up_msg_info
 *
 * Purpose:  Update the rcv_msg_id and adest which are sent from amfnd
 *
 * Input: avnd - ptr to the appropriate amfnd
 *        n2d_msg - node_up msg sent from amfnd
 *
 * Returns: None
 *
 * NOTES:
 *
 *
 **************************************************************************/
void record_node_up_msg_info(AVD_AVND *avnd, const AVD_DND_MSG *n2d_msg)
{
	osafassert(avnd != nullptr);

	avnd->adest = n2d_msg->msg_info.n2d_node_up.adest_address;

	if (n2d_msg->msg_info.n2d_node_up.msg_id >= avnd->rcv_msg_id) {
		LOG_NO("Received node_up from %x: msg_id %u",
			n2d_msg->msg_info.n2d_node_up.node_id,
			n2d_msg->msg_info.n2d_node_up.msg_id);

		avnd->rcv_msg_id = n2d_msg->msg_info.n2d_node_up.msg_id;
	} else {
		// This is expected after recovering from a headless state.
		// NODE_UPs will not be processed until all PLs are up.
		// In the mean time, we may get other messages from amfnd
		// that pushes up rcv_msg_id
		LOG_NO("NODE UP from %x: msg_id out of order. rcv_msg_id %u, msg_id %u",
			n2d_msg->msg_info.n2d_node_up.node_id,
			avnd->rcv_msg_id,
			n2d_msg->msg_info.n2d_node_up.msg_id);
	}
}



/*****************************************************************************
 * Function: avd_node_up_evh
 *
 * Purpose:  This function is the handler for node up event indicating
 * the arrival of the node_up message. Based on the state machine either
 * It will ignore the message or send all the reg messages to the node
 * or order the node reboot if the node_up message arrives after the node
 * sync window has closed.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_node_up_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_AVND *avnd = nullptr;
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t sync_nd_size = avd_count_sync_node_size(cb);
	bool act_nd;

	TRACE_ENTER2("from %x, %s", n2d_msg->msg_info.n2d_node_up.node_id,
				osaf_extended_name_borrow(&n2d_msg->msg_info.n2d_node_up.node_name));

	act_nd = n2d_msg->msg_info.n2d_node_up.node_id == cb->node_id_avd;
	if (cb->scs_absence_max_duration > 0 &&
		cb->all_nodes_synced == false &&
		cb->node_sync_window_closed == false) {
		avnd = avd_node_get(Amf::to_string(&n2d_msg->msg_info.n2d_node_up.node_name));
		if (avnd == nullptr) {
			LOG_ER("Invalid node_name. Check node_id");

			// perhaps this is a node_up from an old version of amfnd without headless support
			// let's check if the node_id is valid
			if ((avnd = avd_node_find_nodeid(n2d_msg->msg_info.n2d_node_up.node_id)) == nullptr) {
				LOG_ER("invalid node ID (%x)", n2d_msg->msg_info.n2d_node_up.node_id);
				goto done;
			}
		}
		uint32_t rc_node_up;
		avnd->node_up_msg_count++;
		rc_node_up = avd_count_node_up(cb);
		if (rc_node_up == sync_nd_size) {
			if (cb->node_sync_tmr.is_active) {
				avd_stop_tmr(cb, &cb->node_sync_tmr);
				TRACE("stop NodeSync timer");
			}
			cb->all_nodes_synced = true;
			LOG_NO("Received node_up_msg from all nodes");
		} else {
			if (avnd->node_up_msg_count == 1 &&
				(act_nd || n2d_msg->msg_info.n2d_node_up.leds_set)) {

				// start (or restart) timer if this is the first message
				// from amfnd-active-SC or amfnd-green-leds-PL
				cb->node_sync_tmr.type = AVD_TMR_NODE_SYNC;
				avd_start_tmr(cb, &(cb->node_sync_tmr), AVSV_DEF_NODE_SYNC_PERIOD);

				TRACE("Received node_up_msg from node:%s. Start/Restart "
						" NodeSync timer waiting for remaining (%d) node(s)",
						osaf_extended_name_borrow(&n2d_msg->msg_info.n2d_node_up.node_name),
						sync_nd_size - rc_node_up);
				goto done;
			}
			if (cb->node_sync_tmr.is_active == true) {
				if (n2d_msg->msg_info.n2d_node_up.leds_set == false) {
					TRACE("NodeSync timer is active, ignore this node_up msg (nodeid:%x)",
						n2d_msg->msg_info.n2d_node_up.node_id);
					goto done;
				}
			}
		}
	}

	/* Cannot use avd_msg_sanity_chk here since this is a special case */
	if ((avnd = avd_node_find_nodeid(n2d_msg->msg_info.n2d_node_up.node_id)) == nullptr) {
		TRACE("invalid node ID (%x)", n2d_msg->msg_info.n2d_node_up.node_id);
		goto done;
	}

	/* Retrieve the information from the message */
	record_node_up_msg_info(avnd, n2d_msg);

	/* Check the AvD FSM state process node up only if AvD is in init done or
	 * APP init state for all nodes except the primary system controller
	 * whose node up is accepted in config done state.
	 */
	if ((n2d_msg->msg_info.n2d_node_up.node_id != cb->node_id_avd) && (cb->init_state < AVD_INIT_DONE)) {
		TRACE("invalid init state (%u), node %x",
			cb->init_state, n2d_msg->msg_info.n2d_node_up.node_id);
		goto done;
	}

	if ((n2d_msg->msg_info.n2d_node_up.node_id == cb->node_id_avd) && (cb->init_state < AVD_INIT_DONE)) {
		// node up from local AVND
		avd_process_state_info_queue(cb);
	}

	if (avnd->node_info.member != SA_TRUE) {
		LOG_WA("Not a Cluster Member dropping the msg");
		goto done;
	}

	/* send the node up message to the node. */
	if (avd_snd_node_up_msg(cb, avnd, avnd->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
		/* free the node up message */

		/* call the routine to failover all the effected nodes
		 * due to restarting this node
		 */

		avd_node_down_func(cb, avnd);
		goto done;
	}

	/* send the Ack message to the node. */
	// note: it's important to ack the msg_id received in n2d_msg, rather than avnd->rcv_msg_id.
	// They will not always be same when headless option is enabled.
	if (avd_snd_node_ack_msg(cb, avnd, n2d_msg->msg_info.n2d_node_up.msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);

		/* call the routine to failover all the effected nodes
		 * due to restarting this node
		 */
		avd_node_down_func(cb, avnd);
		goto done;
	}

	if (n2d_msg->msg_info.n2d_node_up.leds_set == true) {
		TRACE("node %x is already up", avnd->node_info.nodeId);

		if (avnd->reboot) {
			LOG_WA("Sending node reboot order to node:%s, due to nodeFailFast during headless",
				osaf_extended_name_borrow(&n2d_msg->msg_info.n2d_node_up.node_name));
		} else if (cb->node_sync_window_closed == true && avnd->node_up_msg_count == 0) {
			LOG_WA("Sending node reboot order to node:%s, due to first node_up_msg after node sync window",
				osaf_extended_name_borrow(&n2d_msg->msg_info.n2d_node_up.node_name));
			avnd->reboot = true;
		} else if (cb->init_state == AVD_APP_STATE) {
			LOG_WA("Sending node reboot order to node:%s, due to late node_up_msg after cluster startup timeout",
				osaf_extended_name_borrow(&n2d_msg->msg_info.n2d_node_up.node_name));
			avnd->reboot = true;
		}

		if (avnd->reboot) {
			avd_d2n_reboot_snd(avnd);
			avnd->reboot = false;
			goto done;
		} else {
			// this node is already up
			avd_node_state_set(avnd, AVD_AVND_STATE_PRESENT);
			avd_node_oper_state_set(avnd, SA_AMF_OPERATIONAL_ENABLED);
			
			// Update readiness state of all SUs which are waiting for node
			// oper state
			for (const auto& su : avnd->list_of_ncs_su) {
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				if (su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) {
					if (su->sg_of_su->su_insvc(cb, su) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.c_str());
						su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
						goto done;
					}
					avd_node_state_set(avnd, AVD_AVND_STATE_NCS_INIT);
				}
			}
			
			for (const auto& su :avnd->list_of_su) {
				if (su->is_in_service())
					su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
			}
			// Start/Restart cluster init timer to wait for all node becomes PRESENT
			// and all SUs are IN-SERVICE
			// NOTE: start cluster init timer with NODE_SYNC_PERIOD. This NODE_SYNC_PERIOD
			// is defined to sync node_up message of one node. The reason is using
			// NODE_SYNC_PERIOD is that saAmfClusterStartupTimeout can be configured with a
			// too large or even too small, that will cause the presence state synchronization
			// of all nodes unstable.
			cb->amf_init_tmr.type = AVD_TMR_CL_INIT;
			avd_start_tmr(cb,&(cb->amf_init_tmr), AVSV_DEF_NODE_SYNC_PERIOD);
			if (cluster_su_instantiation_done(cb, nullptr) == true) {
				avd_stop_tmr(cb, &cb->amf_init_tmr);
				cluster_startup_expiry_event_generate(cb);
			}
			goto node_joined;
		}
	}

	/* Send role change to this controller AvND */
	if (avnd->node_info.nodeId == cb->node_id_avd) {
		/* Here obviously the role will be ACT. */
		rc = avd_avnd_send_role_change(cb, cb->node_id_avd, cb->avail_state_avd);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s: avd_avnd_send_role_change failed to %x",
				   __FUNCTION__, cb->node_id_avd);
		}
	}
	/* Send role change to this controller AvND */
	if (avnd->node_info.nodeId == cb->node_id_avd_other) {
		/* Here obviously the role will be STDBY. Here we are not picking the
		   state from "avail_state_avd_other" as it may happen that the cold 
		   sync would be in progress so "avail_state_avd_other" might not
		   have been updated. */
		rc = avd_avnd_send_role_change(cb, cb->node_id_avd_other, SA_AMF_HA_STANDBY);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s: avd_avnd_send_role_change failed to peer %x",
				   __FUNCTION__, cb->node_id_avd);
		}
	}

	if (avd_snd_su_reg_msg(cb, avnd, false) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
		/* we are in a bad shape. Restart the node for recovery */

		/* call the routine to failover all the effected nodes
		 * due to restarting this node
		 */

		avd_node_down_func(cb, avnd);
		goto done;
	}

	avd_node_state_set(avnd, AVD_AVND_STATE_NO_CONFIG);

node_joined:
	LOG_NO("Node '%s' joined the cluster", avnd->node_name.c_str());

	/* checkpoint the node. */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVD_NODE_CONFIG);

done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = nullptr;
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_nd_ncs_su_assigned
 *
 * Purpose:  This function is the handler for node director event when a
 *           NCS SU is assigned with a SI or when a spare NCS 2N SU is
 *           instantiated. It verifies that all the NCS SUs are assigned
 *           and all the spare SUs are instantiated then calls the SG module
 *           instantiation function for each of the SUs on the node. It will also
 *           change the node FSM state to present and call the AvD state machine.
 *
 * Input: cb - the AVD control block
 *        avnd - The AvND which has sent the ack for all the component additions.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_nd_ncs_su_assigned(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	TRACE_ENTER();

	for (const auto& ncs_su : avnd->list_of_ncs_su) {
		if (ncs_su->list_of_susi == AVD_SU_SI_REL_NULL ||
			ncs_su->list_of_susi->fsm != AVD_SU_SI_STATE_ASGND) {
			if (ncs_su->sg_of_su->sg_redundancy_model == SA_AMF_NO_REDUNDANCY_MODEL) {
				/* This is an unassigned nored ncs SU so no need to scan further, return here. */
				TRACE_LEAVE();
				return;
			}
			else if (ncs_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL &&
					(ncs_su->sg_of_su->curr_assigned_sus() < 2 ||
					ncs_su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATED)) {
				/* This is an unassigned ncs 2N SU or not yet instantiated ncs spare 2N SU
				 * so no need to scan further, return here.
				 */
				TRACE_LEAVE();
				return;
			}
		}
	}

	/* All the NCS SUs are assigned now change the state to present */
	if (avnd->node_state != AVD_AVND_STATE_PRESENT) {
		avd_node_state_set(avnd, AVD_AVND_STATE_PRESENT);
		avd_node_oper_state_set(avnd, SA_AMF_OPERATIONAL_ENABLED);

		/* Make application SUs operational state ENABLED */
		for (const auto& su : avnd->list_of_su) {
			su->set_oper_state(SA_AMF_OPERATIONAL_ENABLED);
			std::for_each (su->list_of_comp.begin(), su->list_of_comp.end(),
				[] (AVD_COMP *comp) {comp->avd_comp_oper_state_set(SA_AMF_OPERATIONAL_ENABLED);});
		}
		/* We can now set the LEDS */
		avd_snd_set_leds_msg(cb, avnd);

		/* check if this is the primary system controller if yes change the
		 * state of the AvD to init done state.
		 */
		if (avnd->node_info.nodeId == cb->node_id_avd) {
			cb->init_state = AVD_INIT_DONE;

			/* start the cluster init timer. */
			m_AVD_CLINIT_TMR_START(cb);
		}

		/* Check if all SUs are in 'in-service' cluster-wide, if so start assignments */
		if ((cb->amf_init_tmr.is_active == true) && 
				(cluster_su_instantiation_done(cb, nullptr) == true)) {
			avd_stop_tmr(cb, &cb->amf_init_tmr);
			cluster_startup_expiry_event_generate(cb);
		}

		/* Instantiate the application SUs. */
		avd_sg_app_node_su_inst_func(cb, avnd);

	}

	/* if (avnd->node_state != AVD_AVND_STATE_PRESENT) */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, cb, AVSV_CKPT_AVD_CB_CONFIG);
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_nd_ncs_su_failed
 *
 * Purpose:  This function is the handler for node director event when a
 *           NCS SUs operation state goes to disabled.
 *
 * Input: cb - the AVD control block
 *        avnd - The AvND which has sent the ack for all the component additions.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_nd_ncs_su_failed(AVD_CL_CB *cb, AVD_AVND *avnd)
{

	TRACE_ENTER();

	/* call the function to down this node */
	avd_node_down_func(cb, avnd);

	// opensaf_reboot(avnd->node_info.nodeID, 
	//	avnd->node_info.executionEnvironment.value, 
	//	"NCS SU failed default recovery NODE_FAILFAST");
	/* reboot the AvD if the AvND matches self and AVD is quiesced. */
	if ((avnd->node_info.nodeId == cb->node_id_avd) && (cb->avail_state_avd == SA_AMF_HA_QUIESCED)) {
		opensaf_reboot(avnd->node_info.nodeId, osaf_extended_name_borrow(&avnd->node_info.executionEnvironment),
				"Node failed in qsd state");
	}

	return;

}

/*****************************************************************************
 * Function: avd_mds_avnd_up_func
 *
 * Purpose:  This function is the handler for the local AvND up event from
 * MDS. 
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_mds_avnd_up_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	TRACE("Local node director is up, start sending heart beats to %" PRIx64, cb->local_avnd_adest);
	avd_tmr_snd_hb_evh(cb, evt);
}

/*****************************************************************************
 * Function: avd_mds_avnd_down_func
 *
 * Purpose:  This function is the handler for the AvND down event from
 * mds. The function right now is just a place holder.
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_mds_avnd_down_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_AVND *node = avd_node_find_nodeid(evt->info.node_id);

	TRACE_ENTER2("%x, %p", evt->info.node_id, node);
	
	//update MDS version db.
	nds_mds_ver_db.erase(evt->info.node_id);

	if (node != nullptr) {
		// Do nothing if the local node goes down. Most likely due to system shutdown.
		// If node director goes down due to a bug, the AMF watchdog will restart the node.
		if (node->node_info.nodeId == cb->node_id_avd) {
			daemon_exit();
		}

		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			avd_node_failover(node);
			// Update standby out of sync if standby sc goes down
			if (avd_cb->node_id_avd_other == node->node_info.nodeId) {
				cb->stby_sync_state = AVD_STBY_OUT_OF_SYNC;
			}
		} else {
			/* Remove dynamic info for node but keep in nodeid tree.
			 * Possibly used at the end of controller failover to
			 * to failover payload nodes.
			 */
			node->node_state = AVD_AVND_STATE_ABSENT;
			node->saAmfNodeOperState = SA_AMF_OPERATIONAL_DISABLED;
			node->adest = 0;
			node->rcv_msg_id = 0;
			node->snd_msg_id = 0;
			node->recvr_fail_sw = false;
			node->node_info.initialViewNumber = 0;
			node->node_info.member = SA_FALSE;
		}
	}

	if (cb->avd_fover_state) {
		/* Find if node is there in the f-over node list.
		 * If yes then remove entry
		 */
		for (std::map<uint32_t, AVD_FAIL_OVER_NODE *>::const_iterator it = node_list_db->begin();
				it != node_list_db->end(); it++) {
			AVD_FAIL_OVER_NODE *node_fovr = it->second;
			if (node_fovr->node_id == evt->info.node_id) {
				node_list_db->erase(node_fovr->node_id);
				delete node_fovr;
				break;
			}
		}
	}
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_fail_over_event
 *
 * Purpose:  This function is the handler for the AVD fail-over event. This 
 *           function will be called on receiving fail-over callback at standby.
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/
void avd_fail_over_event(AVD_CL_CB *cb)
{
	SaClmNodeIdT node_id = 0;
	AVD_FAIL_OVER_NODE *node_to_add;

	/* Mark this AVD as one recovering from fail-over */
	cb->avd_fover_state = true;

	/* Walk through all the nodes and send verify message to them. */
	for (std::map<uint32_t, AVD_AVND *>::const_iterator it = node_id_db->begin();
			it != node_id_db->end(); it++) {
		AVD_AVND *avnd = it->second;
		node_id = avnd->node_info.nodeId;

		/*
		 * If AVND state machine is in Absent state then just return.
		 */
		if ((AVD_AVND_STATE_ABSENT == avnd->node_state) ||
		    ((node_id == cb->node_id_avd_other) && (cb->swap_switch == false))) {
			continue;
		}

		/*
		 * Check if we are in present state and if yes then send DATA verify 
		 * message to all the AVND's.
		 */
		if ((AVD_AVND_STATE_PRESENT == avnd->node_state) ||
				(AVD_AVND_STATE_NO_CONFIG == avnd->node_state) ||
				(AVD_AVND_STATE_NCS_INIT == avnd->node_state)) {
			/*
			 * Send verify message to this node.
			 */
			if (NCSCC_RC_SUCCESS != avd_snd_node_data_verify_msg(cb, avnd)) {
				/* Log Error */
				return;
			}

			/* we should send the above data verify msg right now */
			avd_d2n_msg_dequeue(cb);

			/* remove the PG record */
			avd_pg_node_csi_del_all(cb, avnd);

			/* Add this node in our fail-over node list */
			node_to_add = new AVD_FAIL_OVER_NODE();

			node_to_add->node_id = avnd->node_info.nodeId;

			if (node_list_db->insert(avnd->node_info.nodeId, node_to_add) != NCSCC_RC_SUCCESS) {
				/* log an error */
				delete node_to_add;
				return;
			}
		} else {
			/*
			 * In case of all other states, send HPI event to restart the node.
			 */
			avd_node_down_func(cb, avnd);
		}

		/*
		 * Since we are sending verify message, this is the time to reset
		 * our send ID counter so that next time onwards messages will 
		 * be sent with new send ID.
		 */
		avnd->snd_msg_id = 0;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);
	}
}

/*****************************************************************************
 * Function: avd_ack_nack_event
 *
 * Purpose:  This function is the handler for the AVD ACK_NACK event message 
 *           received from the AVND in response to DATA_VERIFY message.
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/
void avd_ack_nack_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_AVND *avnd;
	AVD_SU_SI_REL *rel_ptr;
	AVD_DND_MSG *n2d_msg;
	bool node_found = false;

	TRACE_ENTER();

	n2d_msg = evt->info.avnd_msg;
	/* Find if node is there in the f-over node list. If yes then remove entry
	 * and process the message. Else just return.
	 */
	for (std::map<uint32_t, AVD_FAIL_OVER_NODE *>::const_iterator it = node_list_db->begin();
			it != node_list_db->end(); it++) {
		AVD_FAIL_OVER_NODE *node_fovr = it->second;
		if (node_fovr->node_id == evt->info.avnd_msg->msg_info.
				n2d_ack_nack_info.node_id) {
			node_found = true;
			node_list_db->erase(node_fovr->node_id);
			delete node_fovr;
			break;
		}
	}

	if (node_found == false) {
		/* do i need to log an error */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = nullptr;
		goto done;
	}

	/*
	 * Check if AVND is in the present state. If No then drop this event.
	 */
	if (nullptr == (avnd = avd_node_find_nodeid(evt->info.avnd_msg->msg_info.n2d_ack_nack_info.node_id))) {
		/* Not an error? Log information will be helpful */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = nullptr;
		goto done;
	}

	if (AVD_AVND_STATE_PRESENT != avnd->node_state) {
		/* Not an error? Log information will be helpful */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = nullptr;
		goto done;
	}

	if (true == evt->info.avnd_msg->msg_info.n2d_ack_nack_info.ack) {
		/* Wow great!! We are in sync with this node...Log inforamtion */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = nullptr;
		goto done;
	}

	/*
	 * In case AVND is in present state and if NACK is received then 
	 * Send entire configuration to AVND. Seems like we received NACK :(,
	 * Log information that we received NACK.
	 */
	if (false == evt->info.avnd_msg->msg_info.n2d_ack_nack_info.ack) {
		if (avd_snd_su_reg_msg(cb, avnd, true) != NCSCC_RC_SUCCESS) {
			LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
			/* we are in a bad shape. Restart the node for recovery */

			/* call the routine to failover all the effected nodes
			 * due to restarting this node
			 */

			avd_node_down_func(cb, avnd);
			avsv_dnd_msg_free(n2d_msg);
			evt->info.avnd_msg = nullptr;
			goto done;
		}

		/*
		 * Send SU_SI relationship which are in ASSIGN, MODIFY and  
		 * UNASSIGN state.for this node.
		 */
		for (const auto& su_ptr : avnd->list_of_ncs_su) {
			for (rel_ptr = su_ptr->list_of_susi; rel_ptr != nullptr; rel_ptr = rel_ptr->su_next) {
				if ((AVD_SU_SI_STATE_ASGND == rel_ptr->fsm) || (AVD_SU_SI_STATE_ABSENT == rel_ptr->fsm))
					continue;

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, static_cast<AVSV_SUSI_ACT>(rel_ptr->fsm), false, nullptr) != NCSCC_RC_SUCCESS) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su_ptr->name.c_str(),
									     su_ptr->name.length());
				}
			}
		}

		/* check the LED status and send a msg if required */
		if (avnd->node_state == AVD_AVND_STATE_PRESENT
		    && avnd->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) {
			/* We can now set the LEDS */
			avd_snd_set_leds_msg(cb, avnd);
		}

		/*
		 * We have take care of NCS SU's, now do the same for normal SU's.
		 */
		for (const auto& su_ptr : avnd->list_of_su) {
			/* check if susi. If not continue */
			if (!su_ptr->list_of_susi)
				continue;

			/* if SG is 2n,N+M,N-way active and susi is modifying send susi relation */
			if ((su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) ||
			    (su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL) ||
			    (su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)) {
				if (AVD_SU_SI_STATE_MODIFY == su_ptr->list_of_susi->fsm) {
					avd_snd_susi_msg(cb, su_ptr, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_MOD, false, nullptr);
					continue;
				}
			}

			for (rel_ptr = su_ptr->list_of_susi; rel_ptr != nullptr; rel_ptr = rel_ptr->su_next) {
				if ((AVD_SU_SI_STATE_ASGND == rel_ptr->fsm) || (AVD_SU_SI_STATE_ABSENT == rel_ptr->fsm))
					continue;

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, static_cast<AVSV_SUSI_ACT>(rel_ptr->fsm), false, nullptr) != NCSCC_RC_SUCCESS) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su_ptr->name.c_str(),
									     su_ptr->name.length());
				}
			}
		}
	}

	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = nullptr;
done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_node_down
 *
 * Purpose:  This function is set this node to down.
 *
 * Input: cb - the AVD control block
 *        node_id - Node ID.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/
uint32_t avd_node_down(AVD_CL_CB *cb, SaClmNodeIdT node_id)
{
	AVD_AVND *avnd;

	TRACE_ENTER();

	if ((avnd = avd_node_find_nodeid(node_id)) == nullptr) {
		/* log error that the node id is invalid */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, node_id);
		return NCSCC_RC_FAILURE;
	}

	if ((avnd->node_state == AVD_AVND_STATE_ABSENT) || (avnd->node_state == AVD_AVND_STATE_GO_DOWN)) {
		/* log information error that the node is in invalid state */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_state);
		return NCSCC_RC_FAILURE;
	}

	/* call the routine to failover all the effected nodes
	 * due to restarting this node
	 */

	/* call the function to down this node */
	avd_node_down_func(cb, avnd);

	return NCSCC_RC_SUCCESS;
}

void avd_node_mark_absent(AVD_AVND *node)
{
	TRACE_ENTER();

	avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
	avd_node_state_set(node, AVD_AVND_STATE_ABSENT);

	LOG_NO("Node '%s' left the cluster", node->node_name.c_str());

	node->adest = 0;
	node->rcv_msg_id = 0;
	node->snd_msg_id = 0;
	node->recvr_fail_sw = false;

	node->node_info.initialViewNumber = 0;
	node->node_info.member = SA_FALSE;

	/* Increment node failfast counter */
	avd_cb->nodes_exit_cnt++;

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, avd_cb, AVSV_CKPT_AVD_CB_CONFIG);

	TRACE_LEAVE();
}

/**
 * Send heart beat to node local node director
 * @param cb
 * @param evt
 */
void avd_tmr_snd_hb_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG msg = {};
	static uint32_t seq_id;

	TRACE_ENTER2("seq_id=%u", seq_id);

	msg.msg_type = AVSV_D2N_HEARTBEAT_MSG;
	msg.msg_info.d2n_hb_info.seq_id = seq_id++;
	if (avd_mds_send(NCSMDS_SVC_ID_AVND, cb->local_avnd_adest,(NCSCONTEXT)(&msg)) != NCSCC_RC_SUCCESS) {
		LOG_WA("%s failed to send HB msg", __FUNCTION__);
	}

	avd_stop_tmr(cb, &cb->heartbeat_tmr);
	avd_start_tmr(cb, &cb->heartbeat_tmr, cb->heartbeat_tmr_period);
	TRACE_LEAVE();
}

/**
 * @brief  This event is for remembering MDS version for AMFND.
 * @param  ptr to AVD_CL_CB 
 * @param  ptr to AVD_EVT
 */
void avd_avnd_mds_info_evh(AVD_CL_CB *cb, AVD_EVT *evt) {
  TRACE_ENTER2("MDS version:%d, nodeid:%x",
    evt->info.nd_mds_info.mds_version, evt->info.nd_mds_info.node_id);
  nds_mds_ver_db[evt->info.nd_mds_info.node_id] = evt->info.nd_mds_info.mds_version;
  TRACE_LEAVE();
}

