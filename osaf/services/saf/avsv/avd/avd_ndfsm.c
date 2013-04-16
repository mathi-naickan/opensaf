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

#include <avd.h>
#include <avd_cluster.h>

/*****************************************************************************
 * Function: avd_node_up_func
 *
 * Purpose:  This function is the handler for node up event indicating
 * the arrival of the node_up message. Based on the state machine either
 * It will ignore the message or send all the reg messages to the node. 
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
	AVD_AVND *avnd = NULL;
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("from %x", n2d_msg->msg_info.n2d_node_up.node_id);

	/* Cannot use avd_msg_sanity_chk here since this is a special case */
	if ((avnd = avd_node_find_nodeid(n2d_msg->msg_info.n2d_node_up.node_id)) == NULL) {
		TRACE("invalid node ID (%x)", n2d_msg->msg_info.n2d_node_up.node_id);
		goto done;
	}

	/* Check the AvD FSM state process node up only if AvD is in init done or
	 * APP init state for all nodes except the primary system controller
	 * whose node up is accepted in config done state.
	 */
	if ((n2d_msg->msg_info.n2d_node_up.node_id != cb->node_id_avd) && (cb->init_state < AVD_INIT_DONE)) {
		TRACE("invalid init state (%u), node %x",
			cb->init_state, n2d_msg->msg_info.n2d_node_up.node_id);
		goto done;
	}

	if (avnd->node_state != AVD_AVND_STATE_ABSENT) {
		LOG_WA("invalid node state %u for node %x",
			avnd->node_state, n2d_msg->msg_info.n2d_node_up.node_id);
		goto done;
	}

	/* Retrive the information from the message */
	avnd->adest = n2d_msg->msg_info.n2d_node_up.adest_address;
	avnd->rcv_msg_id = n2d_msg->msg_info.n2d_node_up.msg_id;

	if (avnd->node_info.member != SA_TRUE) {
		LOG_WA("Not a Cluster Member dropping the msg");
		goto done;
	}

	/* Identify if this AVND is running on the same node as AVD */
	if ((avnd->node_info.nodeId == cb->node_id_avd) || (avnd->node_info.nodeId == cb->node_id_avd_other)) {
		avnd->type = AVSV_AVND_CARD_SYS_CON;
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
	if (avd_snd_node_ack_msg(cb, avnd, avnd->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);

		/* call the routine to failover all the effected nodes
		 * due to restarting this node
		 */
		avd_node_down_func(cb, avnd);
		goto done;
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

	LOG_NO("Node '%s' joined the cluster", avnd->node_name);

	avd_node_state_set(avnd, AVD_AVND_STATE_NO_CONFIG);

	/* checkpoint the node. */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVD_NODE_CONFIG);

done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_nd_ncs_su_assigned
 *
 * Purpose:  This function is the handler for node director event when a
 *           NCS SU is assigned with a SI. It verifies that all the
 *           NCS SUs are assigned and calls the SG module instantiation
 *           function for each of the SUs on the node. It will also change the 
 *           node FSM state to present and call the AvD state machine.
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
	AVD_SU *ncs_su, *su;

	TRACE_ENTER();

	ncs_su = avnd->list_of_ncs_su;

	while (ncs_su != NULL) {
		if ((ncs_su->list_of_susi == AVD_SU_SI_REL_NULL) ||
		    (ncs_su->list_of_susi->fsm != AVD_SU_SI_STATE_ASGND)) {
			/* this is an unassigned SU so no need to scan further return here. */
			return;
		}

		ncs_su = ncs_su->avnd_list_su_next;
	}

	/* All the NCS SUs are assigned now change the state to present */
	if (avnd->node_state != AVD_AVND_STATE_PRESENT) {
		avd_node_state_set(avnd, AVD_AVND_STATE_PRESENT);
		avd_node_oper_state_set(avnd, SA_AMF_OPERATIONAL_ENABLED);

		/* Make application SUs operational state ENABLED */
		for (su = avnd->list_of_su; su != NULL; su = su->avnd_list_su_next)
			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_ENABLED);

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

		/* Instantiate the application SUs. */
		avd_sg_app_node_su_inst_func(cb, avnd);

	}

	/* if (avnd->node_state != AVD_AVND_STATE_PRESENT) */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, cb, AVSV_CKPT_AVD_CB_CONFIG);
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
		opensaf_reboot(avnd->node_info.nodeId, (char *)avnd->node_info.executionEnvironment.value,
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

	if (node != NULL) {
		// Do nothing if the local node goes down. Most likely due to system shutdown.
		// If node director goes down due to a bug, the AMF watchdog will restart the node.
		if (node->node_info.nodeId == cb->node_id_avd) {
			TRACE("Ignoring down event for local node director");
			goto done;
		}

		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			avd_node_failover(node);
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
		AVD_FAIL_OVER_NODE *node_fovr;
		node_fovr = (AVD_FAIL_OVER_NODE *)ncs_patricia_tree_get(&cb->node_list,
					(uint8_t *)&evt->info.node_id);

		if (NULL != node_fovr) {
			ncs_patricia_tree_del(&cb->node_list, &node_fovr->tree_node_id_node);
			free(node_fovr);
		}
	}
done:
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
	AVD_AVND *avnd;
	SaClmNodeIdT node_id = 0;
	AVD_FAIL_OVER_NODE *node_to_add;

	/* Mark this AVD as one recovering from fail-over */
	cb->avd_fover_state = true;

	/* Walk through all the nodes and send verify message to them. */
	while (NULL != (avnd = avd_node_getnext_nodeid(node_id))) {
		node_id = avnd->node_info.nodeId;

		/*
		 * If AVND state machine is in Absent state then just return.
		 */
		if ((AVD_AVND_STATE_ABSENT == avnd->node_state) ||
		    ((node_id == cb->node_id_avd_other) && (cb->swap_switch == false))) {
			continue;
		}

		/*
		 * Check if we are in present state. If yes then send DATA verify 
		 * message to all the AVND's.
		 */
		if (AVD_AVND_STATE_PRESENT == avnd->node_state) {
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
			if (NULL == (node_to_add = calloc(1, sizeof(AVD_FAIL_OVER_NODE)))) {
				/* Log Error */
				return;
			}

			node_to_add->node_id = avnd->node_info.nodeId;
			node_to_add->tree_node_id_node.key_info = (uint8_t *)&(node_to_add->node_id);
			node_to_add->tree_node_id_node.bit = 0;
			node_to_add->tree_node_id_node.left = NCS_PATRICIA_NODE_NULL;
			node_to_add->tree_node_id_node.right = NCS_PATRICIA_NODE_NULL;

			if (ncs_patricia_tree_add(&cb->node_list, &node_to_add->tree_node_id_node) != NCSCC_RC_SUCCESS) {
				/* log an error */
				free(node_to_add);
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
	AVD_SU *su_ptr;
	AVD_SU_SI_REL *rel_ptr;
	AVD_FAIL_OVER_NODE *node_fovr;
	AVD_DND_MSG *n2d_msg;

	TRACE_ENTER();

	n2d_msg = evt->info.avnd_msg;
	/* Find if node is there in the f-over node list. If yes then remove entry
	 * and process the message. Else just return.
	 */
	if (NULL != (node_fovr =
		     (AVD_FAIL_OVER_NODE *)ncs_patricia_tree_get(&cb->node_list,
								 (uint8_t *)&evt->info.avnd_msg->msg_info.
								 n2d_ack_nack_info.node_id))) {
		ncs_patricia_tree_del(&cb->node_list, &node_fovr->tree_node_id_node);
		free(node_fovr);
	} else {
		/* do i need to log an error */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	/*
	 * Check if AVND is in the present state. If No then drop this event.
	 */
	if (NULL == (avnd = avd_node_find_nodeid(evt->info.avnd_msg->msg_info.n2d_ack_nack_info.node_id))) {
		/* Not an error? Log information will be helpful */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if (AVD_AVND_STATE_PRESENT != avnd->node_state) {
		/* Not an error? Log information will be helpful */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if (true == evt->info.avnd_msg->msg_info.n2d_ack_nack_info.ack) {
		/* Wow great!! We are in sync with this node...Log inforamtion */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
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
			evt->info.avnd_msg = NULL;
			goto done;
		}

		/*
		 * Send SU_SI relationship which are in ASSIGN, MODIFY and  
		 * UNASSIGN state.for this node.
		 */
		for (su_ptr = avnd->list_of_ncs_su; su_ptr != NULL; su_ptr = su_ptr->avnd_list_su_next) {
			for (rel_ptr = su_ptr->list_of_susi; rel_ptr != NULL; rel_ptr = rel_ptr->su_next) {
				if ((AVD_SU_SI_STATE_ASGND == rel_ptr->fsm) || (AVD_SU_SI_STATE_ABSENT == rel_ptr->fsm))
					continue;

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, rel_ptr->fsm, false, NULL) != NCSCC_RC_SUCCESS) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su_ptr->name.value,
									     su_ptr->name.length);
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
		for (su_ptr = avnd->list_of_su; su_ptr != NULL; su_ptr = su_ptr->avnd_list_su_next) {
			/* check if susi. If not continue */
			if (!su_ptr->list_of_susi)
				continue;

			/* if SG is 2n,N+M,N-way active and susi is modifying send susi relation */
			if ((su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) ||
			    (su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL) ||
			    (su_ptr->sg_of_su->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)) {
				if (AVD_SU_SI_STATE_MODIFY == su_ptr->list_of_susi->fsm) {
					avd_snd_susi_msg(cb, su_ptr, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_MOD, false, NULL);
					continue;
				}
			}

			for (rel_ptr = su_ptr->list_of_susi; rel_ptr != NULL; rel_ptr = rel_ptr->su_next) {
				if ((AVD_SU_SI_STATE_ASGND == rel_ptr->fsm) || (AVD_SU_SI_STATE_ABSENT == rel_ptr->fsm))
					continue;

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, rel_ptr->fsm, false, NULL) != NCSCC_RC_SUCCESS) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su_ptr->name.value,
									     su_ptr->name.length);
				}
			}
		}
	}

	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
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

	if ((avnd = avd_node_find_nodeid(node_id)) == NULL) {
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

	LOG_NO("Node '%s' left the cluster", node->node_name);

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
	AVD_DND_MSG msg = {0};
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

