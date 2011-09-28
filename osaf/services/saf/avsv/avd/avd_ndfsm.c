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
	NCS_BOOL comp_sent;
	uns32 rc = NCSCC_RC_SUCCESS;

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

	if (avd_snd_su_comp_msg(cb, avnd, &comp_sent, FALSE) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
		/* we are in a bad shape. Restart the node for recovery */

		/* call the routine to failover all the effected nodes
		 * due to restarting this node
		 */

		avd_node_down_func(cb, avnd);
		goto done;
	}

	/* If component message is sent change state to no config or else skip
	 * all the comming up states and jump to present state. This is because
	 * the node doesnt have any components meaning that even the NCS component
	 * is not present.
	 */
	if (comp_sent == TRUE) {
		avd_node_state_set(avnd, AVD_AVND_STATE_NO_CONFIG);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);
	} else {
		avd_nd_reg_comp_evt_hdl(cb, avnd);
	}

	/* checkpoint the node. */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVD_NODE_CONFIG);

done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_nd_reg_comp_evt_hdl
 *
 * Purpose:  This function is the handler for node director event when the comp
 * message acknowledgement is received indicating that all the configuration
 * on the node is done. This function will instantiate all the NCS SUs on
 * the node. If none exists it will change the node FSM state to present and
 * call the AvD state machine.
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

void avd_nd_reg_comp_evt_hdl(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *ncs_su;

	TRACE_ENTER();

	/* Check the AvND structure to see if any NCS SUs exist on the node.
	 * If none exist change the FSM state present*/
	if ((ncs_su = avnd->list_of_ncs_su) == NULL) {
		/* now change the state to present */
		avd_node_state_set(avnd, AVD_AVND_STATE_PRESENT);
		avd_node_oper_state_set(avnd, SA_AMF_OPERATIONAL_ENABLED);

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

		/* Instantiate the application SUs */
		avd_sg_app_node_su_inst_func(cb, avnd);

		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, cb, AVSV_CKPT_AVD_CB_CONFIG);

		return;
	}

	avd_node_state_set(avnd, AVD_AVND_STATE_NCS_INIT);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);

	/* Instantiate all the NCS SUs on this node by sending the presence state
	 * message for each of the SUs whose components have all been configured.
	 */
	while (ncs_su != NULL) {
		if (ncs_su->num_of_comp != ncs_su->curr_num_comp) {
			/* skip these incomplete SUs. */
			ncs_su = ncs_su->avnd_list_su_next;
			continue;
		}

		/* Check the admin state before instantiation. */
		if ((ncs_su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) ||
			(ncs_su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)) {
			avd_snd_presence_msg(cb, ncs_su, FALSE);
		}
		ncs_su = ncs_su->avnd_list_su_next;
	}

	return;
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
		if (ncs_su->num_of_comp != ncs_su->curr_num_comp) {
			/* skip these incomplete SUs. */
			ncs_su = ncs_su->avnd_list_su_next;
			continue;
		}

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
	TRACE("Local node director is up, start sending heart beats to %llx", cb->local_avnd_adest);
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
		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			/* failover this node */
			avd_node_mark_absent(node);
			avd_node_susi_fail_func(avd_cb, node);
			avd_node_delete_nodeid(node);
		} else {
			/* Remove dynamic info for node but keep in nodeid tree.
			 * Possibly used at the end of controller failover to
			 * to failover payload nodes.
			 */
			node->node_state = AVD_AVND_STATE_ABSENT;
			node->saAmfNodeOperState = SA_AMF_OPERATIONAL_DISABLED;
			node->adest = 0;
			node->node_info.member = SA_FALSE;
		}
	}

	if (cb->avd_fover_state) {
		/* Find if node is there in the f-over node list.
		 * If yes then remove entry
		 */
		AVD_FAIL_OVER_NODE *node_fovr;
		node_fovr = (AVD_FAIL_OVER_NODE *)ncs_patricia_tree_get(&cb->node_list,
					(uns8 *)&evt->info.node_id);

		if (NULL != node_fovr) {
			ncs_patricia_tree_del(&cb->node_list, &node_fovr->tree_node_id_node);
			free(node_fovr);
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
	AVD_AVND *avnd;
	SaClmNodeIdT node_id = 0;
	AVD_FAIL_OVER_NODE *node_to_add;

	/* Mark this AVD as one recovering from fail-over */
	cb->avd_fover_state = TRUE;

	/* Walk through all the nodes and send verify message to them. */
	while (NULL != (avnd = avd_node_getnext_nodeid(node_id))) {
		node_id = avnd->node_info.nodeId;

		/*
		 * If AVND state machine is in Absent state then just return.
		 */
		if ((AVD_AVND_STATE_ABSENT == avnd->node_state) ||
		    ((node_id == cb->node_id_avd_other) && (cb->swap_switch == FALSE))) {
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
			node_to_add->tree_node_id_node.key_info = (uns8 *)&(node_to_add->node_id);
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
	NCS_BOOL comp_sent;
	AVD_FAIL_OVER_NODE *node_fovr;
	AVD_DND_MSG *n2d_msg;

	TRACE_ENTER();

	n2d_msg = evt->info.avnd_msg;
	/* Find if node is there in the f-over node list. If yes then remove entry
	 * and process the message. Else just return.
	 */
	if (NULL != (node_fovr =
		     (AVD_FAIL_OVER_NODE *)ncs_patricia_tree_get(&cb->node_list,
								 (uns8 *)&evt->info.avnd_msg->msg_info.
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

	if (TRUE == evt->info.avnd_msg->msg_info.n2d_ack_nack_info.ack) {
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
	if (FALSE == evt->info.avnd_msg->msg_info.n2d_ack_nack_info.ack) {
		if (avd_snd_su_comp_msg(cb, avnd, &comp_sent, TRUE) != NCSCC_RC_SUCCESS) {
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

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, rel_ptr->fsm) != NCSCC_RC_SUCCESS) {
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
					avd_snd_susi_msg(cb, su_ptr, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_MOD);
					continue;
				}
			}

			for (rel_ptr = su_ptr->list_of_susi; rel_ptr != NULL; rel_ptr = rel_ptr->su_next) {
				if ((AVD_SU_SI_STATE_ASGND == rel_ptr->fsm) || (AVD_SU_SI_STATE_ABSENT == rel_ptr->fsm))
					continue;

				if (avd_snd_susi_msg(cb, su_ptr, rel_ptr, rel_ptr->fsm) != NCSCC_RC_SUCCESS) {
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
uns32 avd_node_down(AVD_CL_CB *cb, SaClmNodeIdT node_id)
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

	node->adest = 0;
	node->rcv_msg_id = 0;
	node->snd_msg_id = 0;
	node->recvr_fail_sw = 0;

	node->node_info.initialViewNumber = 0;
	node->node_info.member = SA_FALSE;

	/* Increment node failfast counter */
	avd_cb->nodes_exit_cnt++;

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, avd_cb, AVSV_CKPT_AVD_CB_CONFIG);

	TRACE_LEAVE();
}

/****************************************************************************
 *  Name          : avd_handle_nd_failover_shutdown
 * 
 *  Description   : This routine is invoked by AvD to handle the shutdown
 *                  request. 
 * 
 *  Arguments     : cb         -  AvD cb .
 *                  avnd       -  ptr to AVD_AVND structure
 * 
 *  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * 
 *  Notes         : 
 ***************************************************************************/

static void avd_handle_nd_failover_shutdown(AVD_CL_CB *cb, AVD_AVND *avnd, SaBoolT for_ncs)
{
	AVD_SU *i_su = NULL;
	NCS_BOOL assign_list_empty = TRUE;

	if (avnd == NULL) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, 0);
		return;
	}

	/* Mark the node operational state as disable and make all the
	 * application SUs in the node as O.O.S. Also call the SG FSM
	 * to do the reallignment of SIs for assigned SUs.
	 */
	if (for_ncs == SA_TRUE)
		i_su = avnd->list_of_ncs_su;
	else
		i_su = avnd->list_of_su;

	while (i_su != NULL) {
		avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
		if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
			/* Since assignments exists call the SG FSM.
			 */
			assign_list_empty = FALSE;

			switch (i_su->sg_of_su->sg_redundancy_model) {
			case SA_AMF_2N_REDUNDANCY_MODEL:
				if (avd_sg_2n_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
					/* log error about the failure */
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value,
									     i_su->name.length);
					return;
				}
				break;

			case SA_AMF_N_WAY_REDUNDANCY_MODEL:
				if (avd_sg_nway_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
					/* log error about the failure */
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value,
									     i_su->name.length);
					return;
				}
				break;

			case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
				if (avd_sg_nacvred_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
					/* log error about the failure */
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value,
									     i_su->name.length);
					return;
				}
				break;

			case SA_AMF_NPM_REDUNDANCY_MODEL:
				if (avd_sg_npm_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
					/* log error about the failure */
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value,
									     i_su->name.length);
					return;
				}
				break;

			case SA_AMF_NO_REDUNDANCY_MODEL:
			default:
				if (avd_sg_nored_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
					/* log error about the failure */
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value,
									     i_su->name.length);
					return;
				}
				break;
			}

		}

		i_su = i_su->avnd_list_su_next;
	}			/* while(i_su != AVD_SU_NULL) */

	if (assign_list_empty == TRUE) {
		/* move to next state */
		avd_chk_failover_shutdown_cxt(cb, avnd, for_ncs);
	}
}
/****************************************************************************
 *  Name          : avd_chk_failover_shutdown_cxt
 * 
 *  Description   : This is called in the context when the SUSI is 
 *                  deleted to check the context if node failover or
 *                  node shutdown is in progress if yes, we check for
 *                  the emptiness and then do the required processing.
 * 
 *  Arguments     : cb         -  AvD cb .
 *                  avnd       -  ptr to AVD_AVND structure
 *                  is_ncs     -  flag to indicate if in cxt of NCS SUs
 * 
 *  Return Values : None
 * 
 *  Notes         : 
 ***************************************************************************/

void avd_chk_failover_shutdown_cxt(AVD_CL_CB *cb, AVD_AVND *avnd, SaBoolT is_ncs)
{
	AVD_SU *i_su = NULL;

	/* check for oper_state if its disable we are in context of 
	 ** failover or shutdown 
	 */
	if (avnd->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED)
		return;

	if (is_ncs == SA_FALSE) {

		i_su = avnd->list_of_su;

		while (i_su != NULL) {
			if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
				/* More susi removal pending on this SU nothing to be
				 ** done now.
				 */
				return;
			}
			i_su = i_su->avnd_list_su_next;
		}
		/* If we are here we walked thru the entire list and found it empty 
		 ** All the APP_sus are now unassigned. 
		 ** 
		 ** Check for the context failover/shutdown
		 ** if shutdown, send terminate message to AvND and process NCS_SUs
		 */
		if (avnd->node_state == AVD_AVND_STATE_SHUTTING_DOWN) {
			avd_snd_shutdown_app_su_msg(cb, avnd);
			return;
		}

	} else {		/*if(is_ncs == SA_TRUE ) */

		/* make sure this happens only in the shutdown context */
		if (avnd->node_state != AVD_AVND_STATE_SHUTTING_DOWN)
			return;	/* Should log an error ? */

		/* Now check if all the susi's of NCS SUs on the entire node 
		 ** are deleted
		 */
		i_su = avnd->list_of_ncs_su;

		while (i_su != NULL) {
			if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
				/* More susi removal pending on this SU nothing to  be
				 ** done now.
				 */
				return;
			}
			i_su = i_su->avnd_list_su_next;
		}
		/* If we are here we walked thru the entire list and found it empty 
		 ** All the ncs_sus are now unassigned.
		 */
		avd_node_mark_absent(avnd);
	}			/* End is_ncs == TRUE */

}

/*****************************************************************************
 * Function: avd_shutdown_app_su_resp_evh
 *
 * Purpose:  This function is the handler for response  
 * event indicating that all the Application SUs have been terminated.
 * Now the AVD will continue processing the NCS SUs.
 *
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_shutdown_app_su_resp_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *avnd;

	TRACE_ENTER2("%x", n2d_msg->msg_info.n2d_shutdown_app_su.node_id);

	if ((avnd = avd_node_find_nodeid(n2d_msg->msg_info.n2d_shutdown_app_su.node_id)) == NULL) {
		/* log error that the node id is invalid */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_shutdown_app_su.node_id);
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		return;
	}

	/* update the receive id count */
	/* checkpoint the AVND structure : receive id */
	m_AVD_SET_AVND_RCV_ID(cb, avnd, (n2d_msg->msg_info.n2d_shutdown_app_su.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, avnd, avnd->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
	}

	avd_handle_nd_failover_shutdown(cb, avnd, SA_TRUE);

	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
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
	static uns32 seq_id;

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

