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

  DESCRIPTION: This file contains the node related events processing.
  Some of these node realted events might effect the AvNDs state machine. Some
  messages might trigger the configuration module processing. It is part of
  the node submodule.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <immutil.h>
#include <logtrace.h>

#include <avd.h>
#include <avd_imm.h>
#include <avd_cluster.h>
#include <logtrace.h>

/**
 * This function does a sanity check w.r.t the message received and returns the node structure
 * related to the node_id.
 * 
 * @param evt
 * @param node_id
 * @param msg_typ
 * @param msg_id
 * 
 * @return AVD_AVND*
 */
AVD_AVND *avd_msg_sanity_chk(AVD_EVT* evt, SaClmNodeIdT node_id, AVSV_DND_MSG_TYPE msg_typ,
	uint32_t msg_id)
{
	AVD_AVND *node;

	if (avd_cluster->saAmfClusterAdminState != SA_AMF_ADMIN_UNLOCKED) {
		LOG_WA("%s: cluster admin state down", __FUNCTION__);
		return NULL;
	}

	if (evt->info.avnd_msg->msg_type != msg_typ) {
		LOG_WA("%s: wrong message type (%u)", __FUNCTION__,evt->info.avnd_msg->msg_type);
		return NULL;
	}

	if ((node = avd_node_find_nodeid(node_id)) == NULL) {
		LOG_WA("%s: invalid node ID (%x)", __FUNCTION__, node_id);
		return NULL;
	}

	if ((node->rcv_msg_id + 1) != msg_id) {
		LOG_WA("%s: invalid msg id %u, from %x should be %u",
			__FUNCTION__, msg_id, node_id, node->rcv_msg_id + 1);
		return NULL;
 	}

	return node;
}

/*****************************************************************************
 * Function: avd_reg_su_func
 *
 * Purpose:  This function is the handler for reg_su event
 * indicating the arrival of the reg_su message response. Based on the state
 * machine and the error value returned, it will processes the message
 * accordingly.
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

void avd_reg_su_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;

	TRACE_ENTER2("from %x, '%s'", n2d_msg->msg_info.n2d_reg_su.node_id, n2d_msg->msg_info.n2d_reg_su.su_name.value);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_reg_su.node_id, AVSV_N2D_REG_SU_MSG,
		n2d_msg->msg_info.n2d_reg_su.msg_id)) == NULL) {
		/* sanity failed return */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	/* Update the receive id for the node */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_reg_su.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, node->node_info.nodeId);
	}

	/* check if this is a operator initiated update */
	if (node->node_state == AVD_AVND_STATE_PRESENT) {
		if (n2d_msg->msg_info.n2d_reg_su.error != NCSCC_RC_SUCCESS) {
			/* Cannot do much, let the operator clean up the mess */
			LOG_ER("%s: '%s' node=%x FAILED to register", __FUNCTION__,
			       n2d_msg->msg_info.n2d_reg_su.su_name.value, n2d_msg->msg_info.n2d_reg_su.node_id);
		}

		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	/* check the ack message for errors. If error stop and free all the
	 * messages in the list and issue HPI restart of the node
	 */

	if (n2d_msg->msg_info.n2d_reg_su.error == NCSCC_RC_SUCCESS) {
		AVD_SU *su;

		/* the node has been successfully updated with SU information */
		avd_node_state_set(node, AVD_AVND_STATE_NCS_INIT);

		/* Instantiate all OpenSAF SUs on this node */
		for (su = node->list_of_ncs_su; su != NULL; su = su->avnd_list_su_next) {
			if ((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) ||
			    (su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)) {
				avd_snd_presence_msg(cb, su, false);
			}
		}

		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	/* log an error since this shouldn't happen */

	LOG_EM("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_reg_su.error);

	/* call the routine to failover all the effected nodes
	 * due to restarting this node
	 */

	avd_node_down_func(cb, node);
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;

	/* checkpoint the AVND structure */
 done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_node_down_func
 *
 * Purpose:  This function is called to abruptly reset a node. It issues the
 * request to reset the node, stop the timers if any.
 *
 * Input: cb - the AVD control block
 *        avnd - The AVND pointer of the node that needs to be shutdown.
 *
 * Returns: None.
 *
 * NOTES: None. 
 **************************************************************************/

void avd_node_down_func(AVD_CL_CB *cb, AVD_AVND *avnd)
{

	TRACE_ENTER();

	/*TODO*/
//	opensaf_reboot(avnd->node_info.nodeId,
//		avnd->node_info.executionEnvironment.value,
//		"Making the node down");

	if (avnd->node_state != AVD_AVND_STATE_SHUTTING_DOWN) {
		avd_node_state_set(avnd, AVD_AVND_STATE_GO_DOWN);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_NODE_STATE);
	}
}

/*****************************************************************************
 * Function: avd_oper_req_func
 *
 * Purpose:  This function is the handler for operation requests response  
 * event indicating the arrival of the response message to the operation . 
 * request message. This function will log and ignore the operation based 
 * on the response in case of error. If success no action.
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

void avd_oper_req_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;

	TRACE_ENTER2("from %x", n2d_msg->msg_info.n2d_op_req.node_id);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_op_req.node_id, AVSV_N2D_OPERATION_REQUEST_MSG,
		n2d_msg->msg_info.n2d_op_req.msg_id)) == NULL) {
		/* sanity failed return */
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* Update the receive id for the node */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_op_req.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, node->node_info.nodeId);
	}

	/* check the ack message for errors. If error stop and free all the
	 * messages in the list and issue HPI restart of the node
	 */

	if (n2d_msg->msg_info.n2d_op_req.error == NCSCC_RC_SUCCESS) {
		/* the AVND has been successfully updated with the operation
		 */
		goto done;
	}

	LOG_ER("Operation request FAILED, sender %x, '%s'",
		n2d_msg->msg_info.n2d_op_req.node_id, n2d_msg->msg_info.n2d_op_req.param_info.name.value);

 done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

/**
 * handler to report error response to imm for any pending admin operation on comp 
 *
 * @param comp
 * @param pres
 */
static void comp_admin_op_report_to_imm(AVD_COMP *comp, SaAmfPresenceStateT pres)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;
	TRACE_ENTER2("%u", pres);

	if (comp->admin_pend_cbk.admin_oper == SA_AMF_ADMIN_RESTART) {
		if ((comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATED) &&
		   (pres != SA_AMF_PRESENCE_RESTARTING)) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle,comp->admin_pend_cbk.invocation, SA_AIS_ERR_BAD_OPERATION);
			comp->admin_pend_cbk.admin_oper = 0;
			comp->admin_pend_cbk.invocation = 0;
		}
		else if (comp->saAmfCompPresenceState == SA_AMF_PRESENCE_RESTARTING) {
			if (pres == SA_AMF_PRESENCE_INSTANTIATED)
				avd_saImmOiAdminOperationResult(cb->immOiHandle,comp->admin_pend_cbk.invocation, SA_AIS_OK);
			else
				avd_saImmOiAdminOperationResult(cb->immOiHandle,comp->admin_pend_cbk.invocation, SA_AIS_ERR_REPAIR_PENDING);

			comp->admin_pend_cbk.admin_oper = 0;
			comp->admin_pend_cbk.invocation = 0;
		}
	}
	TRACE_LEAVE2("(%llu)", comp->admin_pend_cbk.invocation);
}

/**
 * handler to report error response to imm for any pending admin operation on su 
 *
 * @param su
 * @param pres
 */
static void su_admin_op_report_to_imm(AVD_SU *su, SaAmfPresenceStateT pres)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;

	TRACE_ENTER2("(%u) (%llu)", pres, su->pend_cbk.invocation);

	switch (su->pend_cbk.admin_oper) {
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		if (pres == SA_AMF_PRESENCE_UNINSTANTIATED) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation, SA_AIS_OK);
			su->pend_cbk.invocation = 0;
			su->pend_cbk.admin_oper = 0;
		} else if (pres == SA_AMF_PRESENCE_TERMINATION_FAILED) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
							    SA_AIS_ERR_REPAIR_PENDING);
			su->pend_cbk.invocation = 0;
			su->pend_cbk.admin_oper = 0;
		}
		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		if (pres == SA_AMF_PRESENCE_INSTANTIATED) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation, SA_AIS_OK);
			su->pend_cbk.invocation = 0;
			su->pend_cbk.admin_oper = 0;
		} else if ((pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
			   (pres == SA_AMF_PRESENCE_TERMINATION_FAILED)) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
							    SA_AIS_ERR_REPAIR_PENDING);
			su->pend_cbk.invocation = 0;
			su->pend_cbk.admin_oper = 0;
		}
		break;
	case SA_AMF_ADMIN_REPAIRED:
		if (pres == SA_AMF_PRESENCE_UNINSTANTIATED) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle,
				su->pend_cbk.invocation, SA_AIS_OK);
			
			/* Hand over to SG and let it do the rest */
			(void) avd_sg_app_su_inst_func(cb, su->sg_of_su);
		} else {
			LOG_WA("Bad presence state %u after '%s' adm repaired", pres, su->name.value);
			avd_saImmOiAdminOperationResult(cb->immOiHandle,
				su->pend_cbk.invocation, SA_AIS_ERR_BAD_OPERATION);
		}
		su->pend_cbk.invocation = 0;
		su->pend_cbk.admin_oper = 0;
		break;
	default:
		break;
	}

	TRACE_LEAVE2("(%llu)", su->pend_cbk.invocation);
}

/**
 * handler to report error response to imm for any pending admin operation on node 
 * depending on the SU presence state updates received
 *
 * @param su
 * @param pres
 */
static void node_admin_op_report_to_imm(AVD_SU *su, SaAmfPresenceStateT pres)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;

	TRACE_ENTER2("(%u) (%llu)", pres, su->su_on_node->admin_node_pend_cbk.invocation);

	switch (su->su_on_node->admin_node_pend_cbk.admin_oper) {
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		if (pres == SA_AMF_PRESENCE_UNINSTANTIATED) {
			su->su_on_node->su_cnt_admin_oper--;
			if (su->su_on_node->su_cnt_admin_oper == 0) {
				/* if this is the last SU then send out the pending callback */
				avd_saImmOiAdminOperationResult(cb->immOiHandle,
								    su->su_on_node->admin_node_pend_cbk.invocation,
								    SA_AIS_OK);
				su->su_on_node->admin_node_pend_cbk.invocation = 0;
				su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
			}
		} else if (pres == SA_AMF_PRESENCE_TERMINATION_FAILED) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle,
							    su->su_on_node->admin_node_pend_cbk.invocation,
							    SA_AIS_ERR_REPAIR_PENDING);
			su->su_on_node->admin_node_pend_cbk.invocation = 0;
			su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
			su->su_on_node->su_cnt_admin_oper = 0;
		}		/* else do nothing ::SA_AMF_PRESENCE_TERMINATING update is valid */
		break;

	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		if (pres == SA_AMF_PRESENCE_INSTANTIATED) {
			su->su_on_node->su_cnt_admin_oper--;
			if (su->su_on_node->su_cnt_admin_oper == 0) {
				/* if this is the last SU then send out the pending callback */
				avd_saImmOiAdminOperationResult(cb->immOiHandle,
								    su->su_on_node->admin_node_pend_cbk.invocation,
								    SA_AIS_OK);
				su->su_on_node->admin_node_pend_cbk.invocation = 0;
				su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
			}
		} else if ((pres == SA_AMF_PRESENCE_TERMINATION_FAILED) ||
			   (pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle,
							    su->su_on_node->admin_node_pend_cbk.invocation,
							    SA_AIS_ERR_REPAIR_PENDING);
			su->su_on_node->admin_node_pend_cbk.invocation = 0;
			su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
			su->su_on_node->su_cnt_admin_oper = 0;
		}		/* else do nothing :: SA_AMF_PRESENCE_INSTANTIATING update is valid */
		break;

	default:
		/* present state updates may be received as part of other admin ops also */
		TRACE_LEAVE2("Present state update admin op %d", su->su_on_node->admin_node_pend_cbk.admin_oper);
		break;
	}

	TRACE_LEAVE2("(%llu)(%u)", su->su_on_node->admin_node_pend_cbk.invocation, su->su_on_node->su_cnt_admin_oper);
}

static void clm_pend_response(AVD_SU *su, SaAmfPresenceStateT pres)
{
	TRACE_ENTER();
	if (pres == SA_AMF_PRESENCE_UNINSTANTIATED) {
		su->su_on_node->su_cnt_admin_oper--;
		if (su->su_on_node->su_cnt_admin_oper == 0) {
			/* if this is the last SU then send out the pending callback */
			saClmResponse_4(avd_cb->clmHandle, su->su_on_node->clm_pend_inv,
							   SA_CLM_CALLBACK_RESPONSE_OK);
			su->su_on_node->clm_pend_inv = 0;
		}
	} else if (pres == SA_AMF_PRESENCE_TERMINATION_FAILED) {
		saClmResponse_4(avd_cb->clmHandle, su->su_on_node->clm_pend_inv,
							   SA_CLM_CALLBACK_RESPONSE_ERROR);
		su->su_on_node->clm_pend_inv = 0;
		su->su_on_node->su_cnt_admin_oper = 0;
	}		/* else do nothing ::SA_AMF_PRESENCE_TERMINATING update is valid */

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_data_update_req_func
 *
 * Purpose:  This function is the handler for update request message
 * event indicating the arrival of the data update request message.
 * This function will update the local structures with the information
 * provided by this message and send response to any admin callbacks from IMM
 * for which response was not sent.
 * It is used for supporting some of the objects whose values are
 * are managed by AvND.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 *
 * NOTES: The table id object id list handled in the routine from the AvND are
 *
 * 
 **************************************************************************/

void avd_data_update_req_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;
	AVD_COMP *comp;
	AVD_SU *su;
	uint32_t l_val = 0;

	TRACE_ENTER2("from %x", n2d_msg->msg_info.n2d_data_req.node_id);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_data_req.node_id, AVSV_N2D_DATA_REQUEST_MSG,
		n2d_msg->msg_info.n2d_data_req.msg_id)) == NULL) {
		/* sanity failed return */
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* Update the receive message id. */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_data_req.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		/* log error that the director is not able to send the message */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, node->node_info.nodeId);
	}

	/* Verify that operation is only modification. */
	if (n2d_msg->msg_info.n2d_data_req.param_info.act != AVSV_OBJ_OPR_MOD) {
		/* log error that a the table value is invalid */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.act);
		goto done;
	}

	switch (n2d_msg->msg_info.n2d_data_req.param_info.class_id) {
	case AVSV_SA_AMF_COMP:{
			/* Find the component record in the database, specified in the message. */
			if ((comp = avd_comp_get(&n2d_msg->msg_info.n2d_data_req.param_info.name)) == NULL) {
				LOG_ER("%s: Invalid Comp '%s' (%u)", __FUNCTION__,
					n2d_msg->msg_info.n2d_data_req.param_info.name.value,
					n2d_msg->msg_info.n2d_data_req.param_info.name.length);
				goto done;
			}

			switch (n2d_msg->msg_info.n2d_data_req.param_info.attr_id) {
			case saAmfCompOperState_ID:
				TRACE("comp oper state");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					SaAmfReadinessStateT saAmfCompReadinessState;

					l_val = *((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]);
					avd_comp_oper_state_set(comp, ntohl(l_val));

					/* We need to update saAmfCompReadinessState */
					if ((comp->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
						saAmfCompReadinessState = SA_AMF_READINESS_IN_SERVICE;
					} else if ((comp->su->saAmfSuReadinessState == SA_AMF_READINESS_STOPPING) &&
						   (comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
						saAmfCompReadinessState = SA_AMF_READINESS_STOPPING;
					} else
						saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;

					avd_comp_readiness_state_set(comp, saAmfCompReadinessState);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			case saAmfCompPresenceState_ID:
				TRACE("comp pres state");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = ntohl(*((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]));
					/* Now check whether this presence state change was due to 
					   adminstrative operation invocation on the component */
					if (comp->admin_pend_cbk.invocation != 0)
						comp_admin_op_report_to_imm(comp, l_val);
					avd_comp_pres_state_set(comp, l_val);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			case saAmfCompRestartCount_ID:
				TRACE("comp restart count");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = *((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]);
					comp->saAmfCompRestartCount = ntohl(l_val);
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVSV_CKPT_COMP_RESTART_COUNT);
					avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
								  "saAmfCompRestartCount", SA_IMM_ATTR_SAUINT32T,
								  &comp->saAmfCompRestartCount);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			case saAmfCompCurrProxyName_ID:
				TRACE("comp curr proxy name");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len ==
				    strlen((char *)n2d_msg->msg_info.n2d_data_req.param_info.value)) {
					l_val = n2d_msg->msg_info.n2d_data_req.param_info.value_len;
					comp->saAmfCompCurrProxyName.length = l_val;

					strncpy((char *)comp->saAmfCompCurrProxyName.value,
						(char *)n2d_msg->msg_info.n2d_data_req.param_info.value,
						SA_MAX_NAME_LENGTH - 1);
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVSV_CKPT_COMP_CURR_PROXY_NAME);
					avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
								  "saAmfCompCurrProxyName", SA_IMM_ATTR_SANAMET,
								  &comp->saAmfCompCurrProxyName);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			case saAmfCompProxyStatus_ID:
				TRACE("comp proxy status");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = ntohl(*((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]));
					avd_comp_proxy_status_change(comp, l_val);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
									value_len);
				}
				break;
			default:
				/* log error that a the object value is invalid */
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.attr_id);
				break;
			}	/* switch(n2d_msg->msg_info.n2d_data_req.param_info.obj_id) */

			break;
		}
	case AVSV_SA_AMF_SU:{
			/* Find the component record in the database, specified in the message. */
			if ((su = avd_su_get(&n2d_msg->msg_info.n2d_data_req.param_info.name)) == NULL) {
				LOG_ER("%s: Invalid SU '%s' (%u)", __FUNCTION__,
					n2d_msg->msg_info.n2d_data_req.param_info.name.value,
					n2d_msg->msg_info.n2d_data_req.param_info.name.length);
				goto done;
			}

			switch (n2d_msg->msg_info.n2d_data_req.param_info.attr_id) {
			case saAmfSUOperState_ID:
				TRACE("oper pres state");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = ntohl(*((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]));
					avd_su_oper_state_set(su, l_val);
				}
				break;
			case saAmfSUPresenceState_ID:
				TRACE("su pres state");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = ntohl(*((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]));
					avd_su_pres_state_set(su, l_val);

					/* Send response to any admin callbacks delivered by IMM if not sent already. */
					if (su->su_on_node->admin_node_pend_cbk.invocation != 0) {
						node_admin_op_report_to_imm(su, l_val);
					} else if (su->pend_cbk.invocation != 0) {
						su_admin_op_report_to_imm(su, l_val);
					}
					/* send response to pending clm callback */
					if (su->su_on_node->clm_pend_inv != 0)
						clm_pend_response(su, l_val);
				} else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			case saAmfSURestartCount_ID:
				TRACE("su restart count");
				if (n2d_msg->msg_info.n2d_data_req.param_info.value_len == sizeof(uint32_t)) {
					l_val = ntohl(*((uint32_t *)&n2d_msg->msg_info.n2d_data_req.param_info.value[0]));
					su->saAmfSURestartCount = l_val;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_SU_RESTART_COUNT);
				}
				else {
					/* log error that a the  value len is invalid */
					LOG_ER("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.
								    value_len);
				}
				break;
			default:
				/* log error that a the object value is invalid */
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.attr_id);
				break;
			}	/* switch(n2d_msg->msg_info.n2d_data_req.param_info.obj_id) */

			break;	/* case AVSV_SA_AMF_SU */
		}
	default:
		/* log error that a the table value is invalid */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, n2d_msg->msg_info.n2d_data_req.param_info.class_id);
		goto done;
		break;
	}			/* switch(n2d_msg->msg_info.n2d_data_req.param_info.table_id) */

 done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_comp_validation_func
 *
 * Purpose:  This function responds to componenet validation query from AvND. 
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_comp_validation_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_COMP *comp_ptr = NULL;
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	uint32_t res = NCSCC_RC_SUCCESS;
	AVD_AVND *node;
	AVSV_N2D_COMP_VALIDATION_INFO *valid_info = NULL;

	valid_info = &n2d_msg->msg_info.n2d_comp_valid_info;

	TRACE_ENTER2("%s", valid_info->comp_name.value);

	if ((node = avd_msg_sanity_chk(evt, valid_info->node_id, AVSV_N2D_COMP_VALIDATION_MSG,
		valid_info->msg_id)) == NULL) {
		/* sanity failed return */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	/* Update the receive id for the node */
	m_AVD_SET_AVND_RCV_ID(cb, node, (valid_info->msg_id));

	comp_ptr = avd_comp_get(&valid_info->comp_name);

	if (NULL != comp_ptr) {
		/* We found the component, reply to AvND. */
		res = avd_snd_comp_validation_resp(cb, node, comp_ptr, n2d_msg);

	} else {
		/* We couldn't find the component, this is not a configured component. 
		   Reply to AvND. */
		res = avd_snd_comp_validation_resp(cb, node, NULL, n2d_msg);
	}

	if (NCSCC_RC_FAILURE == res) {
		LOG_ER("%s: avd_snd_comp_validation_resp to %x failed", __FUNCTION__, valid_info->node_id);
	}

	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
done:
	TRACE_LEAVE();
}


/**
 * An AMF node has left the cluster. Cleanup the node state and fail over services to other nodes.
 * @param node
 */
void avd_node_failover(AVD_AVND *node)
{
	TRACE_ENTER2("'%s'", node->name.value);
	avd_node_mark_absent(node);
	avd_pg_node_csi_del_all(avd_cb, node);
	avd_node_down_mw_susi_failover(avd_cb, node);
	avd_node_down_appl_susi_failover(avd_cb, node);
	avd_node_delete_nodeid(node);
	TRACE_LEAVE();
}

