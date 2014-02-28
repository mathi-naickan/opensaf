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

  DESCRIPTION: This file contains the utility functions for creating
  messages provided by communication interface module and used by both
  itself and the other modules in the AVD.
  
******************************************************************************
*/

#include <string.h>

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>
#include <amfd.h>

static const SaNameT _amfSvcUsrName = {
	sizeof("safApp=safAmfService"),
	"safApp=safAmfService",
};

const SaNameT *amfSvcUsrName = &_amfSvcUsrName;

const char *avd_adm_state_name[] = {
	"INVALID",
	"UNLOCKED",
	"LOCKED",
	"LOCKED_INSTANTIATION",
	"SHUTTING_DOWN"
};

static const char *avd_adm_op_name[] = {
	"INVALID",
	"UNLOCK",
	"LOCK",
	"LOCK_INSTANTIATION",
	"UNLOCK_INSTANTIATION",
	"SHUTDOWN",
	"RESTART",
	"SI_SWAP",
	"SG_ADJUST",
	"REPAIRED",
	"EAM_START",
	"EAM_STOP"
};

const char *avd_pres_state_name[] = {
	"INVALID",
	"UNINSTANTIATED",
	"INSTANTIATING",
	"INSTANTIATED",
	"TERMINATING",
	"RESTARTING",
	"INSTANTIATION_FAILED",
	"TERMINATION_FAILED"
};

const char *avd_oper_state_name[] = {
	"INVALID",
	"ENABLED",
	"DISABLED"
};

const char *avd_proxy_status_name[] = {
	"INVALID",
	"UNPROXIED",
	"PROXIED"
};

const char *avd_readiness_state_name[] = {
	"INVALID",
	"OUT_OF_SERVICE",
	"IN_SERVICE",
	"STOPPING"
};

const char *avd_ass_state[] = {
	"INVALID",
	"UNASSIGNED",
	"FULLY_ASSIGNED",
	"PARTIALLY_ASSIGNED"
};

const char *avd_ha_state[] = {
	"INVALID",
	"ACTIVE",
	"STANDBY",
	"QUIESCED",
	"QUIESCING"
};

/*****************************************************************************
 * Function: avd_snd_node_ack_msg
 *
 * Purpose:  This function prepares the Message ID ACK message and sends it
 *           to the node from which message is received.. 
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND structure of the node.
 *        msg_id - Message ID for which ACK needs to be sent.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_node_ack_msg(AVD_CL_CB *cb, AVD_AVND *avnd, uint32_t msg_id)
{
	AVD_DND_MSG *d2n_msg;

	d2n_msg = new AVD_DND_MSG();

	/* prepare the message */
	d2n_msg->msg_type = AVSV_D2N_DATA_ACK_MSG;
	d2n_msg->msg_info.d2n_ack_info.node_id = avnd->node_info.nodeId;
	d2n_msg->msg_info.d2n_ack_info.msg_id_ack = msg_id;

	TRACE("Send ack msg to %x", avnd->node_info.nodeId);

	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_snd_node_data_verify_msg
 *
 * Purpose:  This function prepares the node data verify message for the
 * given node and sends the message to the node directors. 
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND structure of the node whose information
 *               needs to be update with all the other nodes.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_node_data_verify_msg(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_DND_MSG *d2n_msg;

	TRACE_ENTER();

	/* Verify if the AvND structure pointer is valid. */
	if (avnd == NULL) {
		/* This is a invalid situation as the node record
		 * needs to be mentioned.
		 */

		/* Log a fatal error that node record can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	d2n_msg = new AVSV_DND_MSG();

	/* prepare the message */
	d2n_msg->msg_type = AVSV_D2N_DATA_VERIFY_MSG;
	d2n_msg->msg_info.d2n_data_verify.node_id = avnd->node_info.nodeId;
	d2n_msg->msg_info.d2n_data_verify.rcv_id_cnt = avnd->rcv_msg_id;
	d2n_msg->msg_info.d2n_data_verify.snd_id_cnt = avnd->snd_msg_id;
	d2n_msg->msg_info.d2n_data_verify.su_failover_prob = avnd->saAmfNodeSuFailOverProb;
	d2n_msg->msg_info.d2n_data_verify.su_failover_max = avnd->saAmfNodeSuFailoverMax;

	TRACE("Sending %u to %x", AVSV_D2N_DATA_VERIFY_MSG, avnd->node_info.nodeId);

	/* Now send the message to the node director */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}


/*****************************************************************************
 * Function: avd_snd_node_up_msg
 *
 * Purpose:  This function prepares the node up message for the
 * given node, by scanning all the nodes present in the cluster and
 * adding their information to the message. It then sends the message to the 
 * node.
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND structure of the node to which node up
 *               message needs to be sent.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_node_up_msg(AVD_CL_CB *cb, AVD_AVND *avnd, uint32_t msg_id_ack)
{
	AVD_DND_MSG *d2n_msg;

	TRACE_ENTER();

	/* Verify if the AvND structure pointer is valid. */
	if (avnd == NULL) {
		/* This is a invalid situation as the node record
		 * needs to be mentioned.
		 */

		/* Log a fatal error that node record can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	d2n_msg = new AVSV_DND_MSG();

	/* prepare the message */
	d2n_msg->msg_type = AVSV_D2N_NODE_UP_MSG;
	d2n_msg->msg_info.d2n_node_up.node_id = avnd->node_info.nodeId;
	d2n_msg->msg_info.d2n_node_up.node_type = avnd->type;
	d2n_msg->msg_info.d2n_node_up.su_failover_max = avnd->saAmfNodeSuFailoverMax;
	d2n_msg->msg_info.d2n_node_up.su_failover_prob = avnd->saAmfNodeSuFailOverProb;

	TRACE("Sending %u to %x", AVSV_D2N_NODE_UP_MSG, avnd->node_info.nodeId);

	/* Now send the message to the node director */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
 * Function: avd_snd_oper_state_msg
 *
 * Purpose:  This function prepares the operation state message which is a
 * acknowledgement message for the node.
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND structure of the node to which this
 *               operation state acknowledgement needs to be sent.
 *        msg_id_ack - The id of the message that needs to be acknowledged.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_oper_state_msg(AVD_CL_CB *cb, AVD_AVND *avnd, uint32_t msg_id_ack)
{
	AVD_DND_MSG *d2n_msg;

	TRACE_ENTER();

	/* Verify if the node structure pointer is valid. */
	if (avnd == NULL) {
		/* This is a invalid situation as the avnd record
		 * needs to be mentioned.
		 */

		/* Log a fatal error that AvND record can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* prepare the node operation state ack message. */
	d2n_msg = new AVSV_DND_MSG();

	d2n_msg->msg_type = AVSV_D2N_DATA_ACK_MSG;
	d2n_msg->msg_info.d2n_ack_info.msg_id_ack = msg_id_ack;
	d2n_msg->msg_info.d2n_ack_info.node_id = avnd->node_info.nodeId;

	TRACE("Sending %u to %x", AVSV_D2N_DATA_ACK_MSG, avnd->node_info.nodeId);

	/* send the message */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_snd_presence_msg
 *
 * Purpose:  This function prepares the presence SU message for the
 * given SU. It then sends the message to the node director to which the
 * SU belongs.
 *
 * Input: cb - Pointer to the AVD control block
 *        su - Pointer to the SU structure for which the presence message needs
 *             to be sent.
 *        term_state - True terminate false instantiate.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_presence_msg(AVD_CL_CB *cb, AVD_SU *su, bool term_state)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_DND_MSG *d2n_msg;
	AVD_AVND *node = NULL;

	m_AVD_GET_SU_NODE_PTR(cb, su, node);

	TRACE_ENTER2("%s '%s'", (term_state == true) ? "Terminate" : "Instantiate", su->name.value);

	/* prepare the node update message. */
	d2n_msg = new AVSV_DND_MSG();

	/* prepare the SU presence state change notification message */
	d2n_msg->msg_type = AVSV_D2N_PRESENCE_SU_MSG;
	d2n_msg->msg_info.d2n_prsc_su.msg_id = ++(node->snd_msg_id);
	d2n_msg->msg_info.d2n_prsc_su.node_id = node->node_info.nodeId;
	d2n_msg->msg_info.d2n_prsc_su.su_name = su->name;
	d2n_msg->msg_info.d2n_prsc_su.term_state = term_state;

	TRACE("Sending %u to %x", AVSV_D2N_PRESENCE_SU_MSG, node->node_info.nodeId);

	/* send the message */
	if (avd_d2n_msg_snd(cb, node, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		--(node->snd_msg_id);
		goto done;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, (node), AVSV_CKPT_AVND_SND_MSG_ID);
	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_snd_op_req_msg
 *
 * Purpose:  This function prepares the operation request 
 * message for the given object id and its corresponding value. It then sends
 * the message to the particular node director mentioned or to all the 
 * node directors in the system.
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND info for the node that needs to be sent
 *               the message. Non NULL if message need to be sent to a
 *               particular node.
 *  param_info - Pointer to the information and operation that need to be done.
 *             
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * 
 **************************************************************************/

uint32_t avd_snd_op_req_msg(AVD_CL_CB *cb, AVD_AVND *avnd, AVSV_PARAM_INFO *param_info)
{
	AVD_DND_MSG *op_req_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (cb->avail_state_avd != SA_AMF_HA_ACTIVE) 
		goto done;

	op_req_msg = new AVSV_DND_MSG();

	/* prepare the operation request message. */
	op_req_msg->msg_type = AVSV_D2N_OPERATION_REQUEST_MSG;
	memcpy(&op_req_msg->msg_info.d2n_op_req.param_info, param_info, sizeof(AVSV_PARAM_INFO));

	if (avnd == NULL) {
		/* This means the message needs to be broadcasted to
		 * all the nodes.
		 */

		/* Broadcast the operation request message to all the nodes. */
		avd_d2n_msg_bcast(cb, op_req_msg);

		delete op_req_msg;
		goto done;
	}

	/* check whether AvND is in good state */
	if ((avnd->node_state == AVD_AVND_STATE_ABSENT) ||
	    (avnd->node_state == AVD_AVND_STATE_GO_DOWN) ||
		(avnd->node_state == AVD_AVND_STATE_SHUTTING_DOWN)) {
		TRACE("Node %x down", avnd->node_info.nodeId);
		delete op_req_msg;
		goto done;
	}

	/* send this operation request to a particular node. */
	op_req_msg->msg_info.d2n_op_req.node_id = avnd->node_info.nodeId;

	op_req_msg->msg_info.d2n_op_req.msg_id = ++(avnd->snd_msg_id);

	/* send the operation request message to the node. */
	if ((rc = avd_d2n_msg_snd(cb, avnd, op_req_msg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		--(avnd->snd_msg_id);
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

done:
	TRACE_LEAVE();
	return rc;
}

 /*****************************************************************************
 * Function: avd_prep_su_info
 *
 * Purpose:  This function prepares the SU
 * information for the given SU and adds it to the message. 
 *
 * Input: cb - Pointer to the AVD control block
 *        su - Pointer to the SU related to which the messages need to be sent.
 *        su_msg - Pointer to the SU message being prepared.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static uint32_t avd_prep_su_info(AVD_CL_CB *cb, AVD_SU *su, AVD_DND_MSG *su_msg)
{
	AVSV_SU_INFO_MSG *su_info;

	TRACE_ENTER();

	su_info = new AVSV_SU_INFO_MSG();

	/* fill and add the SU into
	 * the SU message at the top of the list
	 */
	su_info->name = su->name;
	su_info->comp_restart_max = su->sg_of_su->saAmfSGCompRestartMax;
	su_info->comp_restart_prob = su->sg_of_su->saAmfSGCompRestartProb;
	su_info->su_restart_max = su->sg_of_su->saAmfSGSuRestartMax;
	su_info->su_restart_prob = su->sg_of_su->saAmfSGSuRestartProb;
	su_info->is_ncs = su->sg_of_su->sg_ncs_spec;
	su_info->su_is_external = su->su_is_external;

	su_info->next = su_msg->msg_info.d2n_reg_su.su_list;
	su_msg->msg_info.d2n_reg_su.su_list = su_info;
	su_msg->msg_info.d2n_reg_su.num_su++;

	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
 * Function: avd_snd_su_reg_msg
 *
 * Purpose:  This function prepares the SU message for all
 * the SUs in the node and sends it to the Node director on the node.
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND info for the node.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_su_reg_msg(AVD_CL_CB *cb, AVD_AVND *avnd, bool fail_over)
{
	AVD_SU *i_su = NULL;
	AVD_DND_MSG *su_msg;
	uint32_t i, count = 0;
	SaNameT temp_su_name = {0};
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", avnd->node_name);

	su_msg = new AVSV_DND_MSG();

	/* prepare the SU message. */
	su_msg->msg_type = AVSV_D2N_REG_SU_MSG;
	su_msg->msg_info.d2n_reg_su.nodeid = avnd->node_info.nodeId;
	su_msg->msg_info.d2n_reg_su.msg_on_fover = fail_over;

	/* build the SU message for both the NCS and application SUs. */

	/* Check whether the AvND belongs to ACT controller. If yes, then send all
	   the external SUs/Components to it, otherwise send only cluster 
	   components. */
	if (avnd->node_info.nodeId == cb->node_id_avd) {
		count = 2;
	} else
		count = 1;

	for (i = 0; i <= count; ++i) {
		if (i == 0)
			i_su = avnd->list_of_ncs_su;
		else if (i == 1)
			i_su = avnd->list_of_su;
		else {
			/* For external component, we don't have any node attached to it. 
			   So, get the first external SU. */
			temp_su_name.length = 0;
			while (NULL != (i_su = avd_su_getnext(&temp_su_name))) {
				if (true == i_su->su_is_external)
					break;

				temp_su_name = i_su->name;
			}
		}

		while (i_su != NULL) {
			/* Add information about this SU to the message */
			if (avd_prep_su_info(cb, i_su, su_msg) == NCSCC_RC_FAILURE) {
				/* Free all the messages and return error */
				d2n_msg_free(su_msg);
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			/* get the next SU in the node */
			if ((0 == i) || (1 == i))
				i_su = i_su->avnd_list_su_next;
			else {
				/* Get the next external SU. */
				temp_su_name = i_su->name;
				while (NULL != (i_su = avd_su_getnext(&temp_su_name))) {
					if (true == i_su->su_is_external)
						break;

					temp_su_name = i_su->name;
				}
			}
		}
	}

	/* check if atleast one SU data is being sent. If not 
	 * dont send the messages.
	 */
	if (su_msg->msg_info.d2n_reg_su.su_list == NULL) {
		/* Free all the messages and return success */
		d2n_msg_free(su_msg);
		goto done;
	}

	su_msg->msg_info.d2n_reg_su.msg_id = ++(avnd->snd_msg_id);

	/* send the SU message to the node if return value is failure
	 * free messages and return error.
	 */

	TRACE("Sending AVSV_D2N_REG_SU_MSG to %s", avnd->node_name);

	if (avd_d2n_msg_snd(cb, avnd, su_msg) == NCSCC_RC_FAILURE) {
		--(avnd->snd_msg_id);
		LOG_ER("%s: snd to %s failed", __FUNCTION__, avnd->node_name);
		d2n_msg_free(su_msg);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_snd_su_msg
 *
 * Purpose:  This function prepares the SU a message for the given SU.
 * It then sends the message to Node director on the node to which the SU
 * belongs.
 *
 * Input: cb - Pointer to the AVD control block
 *        su - Pointer to the SU related to which the messages need to be sent.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_su_msg(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_DND_MSG *su_msg;
	AVD_AVND *node = NULL;

	TRACE_ENTER();

	if (su == NULL) {
		/* This is a invalid situation as the SU
		 * needs to be mentioned.
		 */

		/* Log a fatal error that su can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	m_AVD_GET_SU_NODE_PTR(cb, su, node);

	su_msg = new AVSV_DND_MSG();

	/* prepare the SU  message. */

	su_msg->msg_type = AVSV_D2N_REG_SU_MSG;

	su_msg->msg_info.d2n_reg_su.nodeid = node->node_info.nodeId;

	/* Add information about this SU to the message */
	if (avd_prep_su_info(cb, su, su_msg) == NCSCC_RC_FAILURE) {
		/* Free the messages and return error */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, node->node_info.nodeId);
		delete su_msg;
		return NCSCC_RC_FAILURE;
	}

	su_msg->msg_info.d2n_reg_su.msg_id = ++(node->snd_msg_id);

	/* send the SU message to the node if return value is failure
	 * free messages and return error.
	 */

	TRACE("Sending %u to %x", AVSV_D2N_REG_SU_MSG, node->node_info.nodeId);

	if (avd_d2n_msg_snd(cb, node, su_msg) == NCSCC_RC_FAILURE) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
		--(node->snd_msg_id);
		d2n_msg_free(su_msg);
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, (node), AVSV_CKPT_AVND_SND_MSG_ID);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_prep_csi_attr_info
 *
 * Purpose:  This function prepares the csi attribute fields for a
 *           compcsi info. It is used when the SUSI message is prepared
 *           with action new.
 *
 * Input: cb - Pointer to the AVD control block *        
 *        compcsi_info - Pointer to the compcsi element of the message
 *                       That needs to have the attributes added to it.
 *        compcsi - component CSI relationship struct pointer for which the
 *                  the attribute values need to be sent.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static uint32_t avd_prep_csi_attr_info(AVD_CL_CB *cb, AVSV_SUSI_ASGN *compcsi_info, AVD_COMP_CSI_REL *compcsi)
{
	AVSV_ATTR_NAME_VAL *i_ptr;
	AVD_CSI_ATTR *attr_ptr;

	TRACE_ENTER();

	/* Allocate the memory for the array of structures for the attributes. */
	if (compcsi->csi->num_attributes == 0) {
		/* No CSI attribute parameters available for the CSI. Return success. */
		return NCSCC_RC_SUCCESS;
	}

	compcsi_info->attrs.list = new AVSV_ATTR_NAME_VAL[compcsi->csi->num_attributes];

	/* initilize both the message pointer and the database pointer. Also init the
	 * message content. 
	 */
	i_ptr = compcsi_info->attrs.list;
	attr_ptr = compcsi->csi->list_attributes;
	compcsi_info->attrs.number = 0;

	/* Scan the list of attributes for the CSI and add it to the message */
	while ((attr_ptr != NULL) && (compcsi_info->attrs.number < compcsi->csi->num_attributes)) {
		memcpy(i_ptr, &attr_ptr->name_value, sizeof(AVSV_ATTR_NAME_VAL));
		compcsi_info->attrs.number++;
		i_ptr = i_ptr + 1;
		attr_ptr = attr_ptr->attr_next;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_snd_susi_msg
 *
 * Purpose:  This function prepares the SU SI message for the given SU and 
 * its SUSI if any. 
 *
 * Input: cb - Pointer to the AVD control block *        
 *        su - Pointer to the SU related to which the messages need to be sent.
 *      susi - SU SI relationship struct pointer. Is NULL when all the 
 *             SI assignments need to be deleted
 *             or when all the SIs of the SU need to change role.
 *      actn - The action value that needs to be sent in the message.
 *      single_csi - If a single component is to be assigned as csi.
 *      compcsi - Comp CSI assignment(signle comp and associated csi) if single_csi is true else NULL.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_susi_msg(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT actn, bool single_csi, 
		AVD_COMP_CSI_REL *compcsi) {

	AVD_DND_MSG *susi_msg;
	AVD_AVND *avnd;
	AVD_COMP_CSI_REL *l_compcsi;
	AVD_SU_SI_REL *l_susi, *i_susi;
	AVSV_SUSI_ASGN *compcsi_info;
	SaAmfCSITransitionDescriptorT trans_dsc;

	TRACE_ENTER();

	/* Get the node information from the SU */
	m_AVD_GET_SU_NODE_PTR(cb, su, avnd);

	/* Need not proceed further if node is not in proper state */
	if ((avnd->node_state == AVD_AVND_STATE_ABSENT) || (avnd->node_state == AVD_AVND_STATE_GO_DOWN))
		return NCSCC_RC_SUCCESS;

	/* Initialize the local variables to avoid warnings */
	l_susi = susi;
	trans_dsc = SA_AMF_CSI_NEW_ASSIGN;

	/* prepare the SU SI message. */
	susi_msg = new AVSV_DND_MSG();

	susi_msg->msg_type = AVSV_D2N_INFO_SU_SI_ASSIGN_MSG;
	susi_msg->msg_info.d2n_su_si_assign.node_id = avnd->node_info.nodeId;
	susi_msg->msg_info.d2n_su_si_assign.su_name = su->name;
	susi_msg->msg_info.d2n_su_si_assign.msg_act = actn;

	if (true == single_csi) {
		susi_msg->msg_info.d2n_su_si_assign.single_csi = true;
	}
	switch (actn) {
	case AVSV_SUSI_ACT_DEL:
		if (susi != AVD_SU_SI_REL_NULL) {
			/* Means we need to delete only this SU SI assignment for 
			 * this SU.
			 */
			susi_msg->msg_info.d2n_su_si_assign.si_name = susi->si->name;

			/* For only these options fill the comp CSI values. */
			if (true == single_csi) {
				osafassert(compcsi);
				l_compcsi = compcsi;

				compcsi_info = new AVSV_SUSI_ASGN();

				compcsi_info->comp_name = l_compcsi->comp->comp_info.name;
				compcsi_info->csi_name = l_compcsi->csi->name;
				susi_msg->msg_info.d2n_su_si_assign.num_assigns = 1;
				susi_msg->msg_info.d2n_su_si_assign.list = compcsi_info;
			}
		}
		break;		/* case AVSV_SUSI_ACT_DEL */
	case AVSV_SUSI_ACT_MOD:

		if (susi == AVD_SU_SI_REL_NULL) {
			/* Means we need to modify all the SUSI for 
			 * this SU.
			 */

			/* Find the su si that is not unassigned and has the highest number of
			 * CSIs in the SI.
			 */

			i_susi = su->list_of_susi;
			l_susi = AVD_SU_SI_REL_NULL;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if (i_susi->fsm == AVD_SU_SI_STATE_UNASGN) {
					i_susi = i_susi->su_next;
					continue;
				}

				if (l_susi == AVD_SU_SI_REL_NULL) {
					l_susi = i_susi;
					i_susi = i_susi->su_next;
					continue;
				}

				i_susi = i_susi->su_next;
			}

			if (l_susi == AVD_SU_SI_REL_NULL) {
				/* log fatal error. This call for assignments has to be
				 * called with a valid SU SI relationship value.
				 */
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
				/* free the SUSI message */
				d2n_msg_free(susi_msg);
				return NCSCC_RC_FAILURE;
			}

			/* use this SU SI modification information to fill the message.
			 */


			susi_msg->msg_info.d2n_su_si_assign.ha_state = l_susi->state;
		} else {	/* if (susi == AVD_SU_SI_REL_NULL) */

			/* for modifications of a SU SI fill the SI name.
			 */

			susi_msg->msg_info.d2n_su_si_assign.si_name = susi->si->name;
			susi_msg->msg_info.d2n_su_si_assign.ha_state = susi->state;

			l_susi = susi;

		}		/* if (susi == AVD_SU_SI_REL_NULL) */

		break;		/* case AVSV_SUSI_ACT_MOD */
	case AVSV_SUSI_ACT_ASGN:

		if (susi == AVD_SU_SI_REL_NULL) {
			/* log fatal error. This call for assignments has to be
			 * called with a valid SU SI relationship value.
			 */
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
			/* free the SUSI message */
			d2n_msg_free(susi_msg);
			return NCSCC_RC_FAILURE;
		}

		/* for new assignments of a SU SI fill the SI name.
		 */

		susi_msg->msg_info.d2n_su_si_assign.si_name = susi->si->name;
		susi_msg->msg_info.d2n_su_si_assign.ha_state = susi->state;

		/* Fill the SU SI pointer to l_susi which will be used from now
		 * for information related to this SU SI
		 */

		l_susi = susi;

		break;		/* case AVSV_SUSI_ACT_ASGN */

	default:

		/* This is invalid case statement
		 */

		/* Log a fatal error that it is an invalid action */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, actn);
		d2n_msg_free(susi_msg);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(actn) */

	/* The following code will be executed by both modification
	 * and assignment action messages to fill the component CSI
	 * information in the message
	 */

	if ((actn == AVSV_SUSI_ACT_ASGN) || ((actn == AVSV_SUSI_ACT_MOD) &&
					     ((susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_ACTIVE) ||
					      (susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_STANDBY)))) {
		/* Check if the other SU was quiesced w.r.t the SI */
		if ((actn == AVSV_SUSI_ACT_MOD) && (susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_ACTIVE)) {
			if (l_susi->si_next == AVD_SU_SI_REL_NULL) {
				if (l_susi->si->list_of_sisu != AVD_SU_SI_REL_NULL) {
					if ((l_susi->si->list_of_sisu->state == SA_AMF_HA_QUIESCED) &&
					    (l_susi->si->list_of_sisu->fsm == AVD_SU_SI_STATE_ASGND))
						trans_dsc = SA_AMF_CSI_QUIESCED;
					else
						trans_dsc = SA_AMF_CSI_NOT_QUIESCED;
				} else {
					trans_dsc = SA_AMF_CSI_NOT_QUIESCED;
				}
			} else {
				if ((l_susi->si_next->state == SA_AMF_HA_QUIESCED) &&
				    (l_susi->si_next->fsm == AVD_SU_SI_STATE_ASGND))
					trans_dsc = SA_AMF_CSI_QUIESCED;
				else
					trans_dsc = SA_AMF_CSI_NOT_QUIESCED;
			}
		}

		/* For only these options fill the comp CSI values. */
		if (true == single_csi) {
			osafassert(compcsi);
			l_compcsi = compcsi;
		} else
			l_compcsi = l_susi->list_of_csicomp;

		while (l_compcsi != NULL) {
			compcsi_info = new AVSV_SUSI_ASGN();

			compcsi_info->comp_name = l_compcsi->comp->comp_info.name;
			compcsi_info->csi_name = l_compcsi->csi->name;
			compcsi_info->csi_rank = l_compcsi->csi->rank;
			compcsi_info->active_comp_dsc = trans_dsc;

			/* fill the Attributes for the CSI if new. */
			if (actn == AVSV_SUSI_ACT_ASGN) {
				if (avd_prep_csi_attr_info(cb, compcsi_info, l_compcsi)
				    == NCSCC_RC_FAILURE) {
					LOG_EM("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
					d2n_msg_free(susi_msg);
					return NCSCC_RC_FAILURE;
				}
			}

			/* fill the quiesced and active component name info */
			if (!((actn == AVSV_SUSI_ACT_ASGN) &&
			      (susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_ACTIVE))) {
				if (l_compcsi->csi_csicomp_next == NULL) {
					if ((l_compcsi->csi->list_compcsi != NULL) &&
					    (l_compcsi->csi->list_compcsi != l_compcsi)) {
						compcsi_info->active_comp_name =
						    l_compcsi->csi->list_compcsi->comp->comp_info.name;

						if ((trans_dsc == SA_AMF_CSI_QUIESCED) &&
						    (l_compcsi->csi->list_compcsi->comp->saAmfCompOperState
						     == SA_AMF_OPERATIONAL_DISABLED)) {
							compcsi_info->active_comp_dsc = SA_AMF_CSI_NOT_QUIESCED;
						}
					}
				} else {
					compcsi_info->active_comp_name =
					    l_compcsi->csi_csicomp_next->comp->comp_info.name;

					if ((trans_dsc == SA_AMF_CSI_QUIESCED) &&
					    (l_compcsi->csi_csicomp_next->comp->saAmfCompOperState
					     == SA_AMF_OPERATIONAL_DISABLED)) {
						compcsi_info->active_comp_dsc = SA_AMF_CSI_NOT_QUIESCED;
					}
				}
			}

			compcsi_info->next = susi_msg->msg_info.d2n_su_si_assign.list;
			susi_msg->msg_info.d2n_su_si_assign.list = compcsi_info;
			susi_msg->msg_info.d2n_su_si_assign.num_assigns++;

			if (true == single_csi) break;
			l_compcsi = l_compcsi->susi_csicomp_next;
		}		/* while (l_compcsi != AVD_COMP_CSI_REL_NULL) */
	}

	/* if ((actn == AVSV_SUSI_ACT_ASGN) || ((actn == AVSV_SUSI_ACT_MOD) && 
	   ((susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_ACTIVE) || 
	   (susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_STANDBY))) */
	susi_msg->msg_info.d2n_su_si_assign.msg_id = ++(avnd->snd_msg_id);

	/* send the SU SI message */
	TRACE("Sending %u to %x", AVSV_D2N_INFO_SU_SI_ASSIGN_MSG, avnd->node_info.nodeId);
	if (avd_d2n_msg_snd(cb, avnd, susi_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		--(avnd->snd_msg_id);
		d2n_msg_free(susi_msg);
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);
	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: avd_prep_pg_mem_list
 *
 * Purpose:  This function prepares the current members of the PG of the 
 *           specified CSI. 
 *
 * Input: cb       - ptr to the AVD control block
 *        csi      - ptr to the csi
 *        mem_list - ptr to the mem-list
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/
static uint32_t avd_prep_pg_mem_list(AVD_CL_CB *cb, AVD_CSI *csi, SaAmfProtectionGroupNotificationBufferT *mem_list)
{
	AVD_COMP_CSI_REL *curr = 0;
	uint32_t i = 0;

	TRACE_ENTER();

	memset(mem_list, 0, sizeof(SaAmfProtectionGroupNotificationBufferT));

	if (csi->compcsi_cnt) {
		/* alloc the memory for the notify buffer */
		mem_list->notification = new SaAmfProtectionGroupNotificationT[csi->compcsi_cnt]();

		/* copy the contents */
		for (curr = csi->list_compcsi; curr; curr = curr->csi_csicomp_next, i++) {
			mem_list->notification[i].member.compName = curr->comp->comp_info.name;
			mem_list->notification[i].member.haState = curr->susi->state;
			mem_list->notification[i].member.rank = curr->comp->su->saAmfSURank;
			mem_list->notification[i].change = SA_AMF_PROTECTION_GROUP_NO_CHANGE;
			mem_list->numberOfItems++;
		}		/* for */
	}

	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: avd_snd_pg_resp_msg
 *
 * Purpose:  This function sends the response to the PG track start & stop 
 *           request from AvND.
 *
 * Input:    cb - ptr to the AVD control block
 *           node - ptr to the node from which the req is rcvd
 *           csi  - ptr to the csi for which the request is sent
 *           n2d_msg - ptr to the start/stop request message from AvND
 *
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES:    None.
 **************************************************************************/
uint32_t avd_snd_pg_resp_msg(AVD_CL_CB *cb, AVD_AVND *node, AVD_CSI *csi, AVSV_N2D_PG_TRACK_ACT_MSG_INFO *n2d_msg)
{
	AVD_DND_MSG *pg_msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *pg_msg_info = 0;

	TRACE_ENTER();

	/* alloc the response msg */
	pg_msg = new AVSV_DND_MSG();

	pg_msg_info = &pg_msg->msg_info.d2n_pg_track_act_rsp;

	/* fill the common fields */
	pg_msg->msg_type = AVSV_D2N_PG_TRACK_ACT_RSP_MSG;
	pg_msg_info->actn = n2d_msg->actn;
	pg_msg_info->csi_name = n2d_msg->csi_name;
	pg_msg_info->msg_id_ack = n2d_msg->msg_id;
	pg_msg_info->node_id = n2d_msg->node_id;
	pg_msg_info->msg_on_fover = n2d_msg->msg_on_fover;

	/* prepare the msg based on the action */
	switch (n2d_msg->actn) {
	case AVSV_PG_TRACK_ACT_START:
		{
			if (csi != NULL) {
				pg_msg_info->is_csi_exist = true;
				rc = avd_prep_pg_mem_list(cb, csi, &pg_msg_info->mem_list);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			} else
				pg_msg_info->is_csi_exist = false;
		}
		break;

	case AVSV_PG_TRACK_ACT_STOP:
		/* do nothing.. the msg is already prepared */
		break;

	default:
		osafassert(0);
	}			/* switch */

	/* send the msg to avnd */
	TRACE("Sending %u to %x", AVSV_D2N_PG_TRACK_ACT_RSP_MSG, n2d_msg->node_id);

	if (avd_d2n_msg_snd(cb, node, pg_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
	}

 done:
	return rc;
}

 /*****************************************************************************
 * Function: avd_snd_pg_upd_msg
 *
 * Purpose:  This function sends the PG update to the specified AvND. It is 
 *           sent when PG membership change happens or when the CSI (on which
 *           is active) is deleted.
 *
 * Input:    cb           - ptr to the AVD control block
 *           node         - ptr to the node to which the update is to be sent
 *           comp_csi     - ptr to the comp-csi relationship (valid only
 *                          when CSI is not deleted)
 *           change       - change wrt this member
 *           csi_name - ptr to the csi-name (valid only when csi is 
 *                          deleted)
 *
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES:    None.
 **************************************************************************/
uint32_t avd_snd_pg_upd_msg(AVD_CL_CB *cb,
			 AVD_AVND *node,
			 AVD_COMP_CSI_REL *comp_csi, SaAmfProtectionGroupChangesT change, SaNameT *csi_name)
{
	AVD_DND_MSG *pg_msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_D2N_PG_UPD_MSG_INFO *pg_msg_info = 0;

	TRACE_ENTER();

	/* alloc the update msg */
	pg_msg = new AVSV_DND_MSG();

	pg_msg_info = &pg_msg->msg_info.d2n_pg_upd;

	/* populate the msg */
	pg_msg->msg_type = AVSV_D2N_PG_UPD_MSG;
	pg_msg_info->node_id = node->node_info.nodeId;
	pg_msg_info->is_csi_del = (csi_name) ? true : false;
	if (false == pg_msg_info->is_csi_del) {
		pg_msg_info->csi_name = comp_csi->csi->name;
		pg_msg_info->mem.member.compName = comp_csi->comp->comp_info.name;
		pg_msg_info->mem.member.haState = comp_csi->susi->state;
		pg_msg_info->mem.member.rank = comp_csi->comp->su->saAmfSURank;
		pg_msg_info->mem.change = change;
	} else
		pg_msg_info->csi_name = *csi_name;

	/* send the msg to avnd */
	TRACE("Sending %u to %x", AVSV_D2N_PG_UPD_MSG, node->node_info.nodeId);

	if (avd_d2n_msg_snd(cb, node, pg_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
	}

	/* if (pg_msg) d2n_msg_free(pg_msg); */
	TRACE_LEAVE2("(%u)", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_snd_set_leds_msg
 *
 * Purpose:  This function sends a message to AvND to set the leds on 
 *           that node. 
 *
 * Input: cb - Pointer to the AVD control block
 *        avnd - Pointer to the AVND structure of the node.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_set_leds_msg(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_DND_MSG *d2n_msg;

	TRACE_ENTER();

	/* Verify if the node structure pointer is valid. */
	if (avnd == NULL) {
		/* This is a invalid situation as the avnd record
		 * needs to be mentioned.
		 */

		/* Log a fatal error that AvND record can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* If we have wrongly identified the card type of avnd, its time to correct it */
	if ((cb->node_id_avd_other == avnd->node_info.nodeId) && (avnd->type != AVSV_AVND_CARD_SYS_CON)) {
		avnd->type = AVSV_AVND_CARD_SYS_CON;
		/* checkpoint this information */
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVD_NODE_CONFIG);
	}

	/* prepare the message. */
	d2n_msg = new AVSV_DND_MSG();

	d2n_msg->msg_type = AVSV_D2N_SET_LEDS_MSG;
	d2n_msg->msg_info.d2n_set_leds.node_id = avnd->node_info.nodeId;
	d2n_msg->msg_info.d2n_set_leds.msg_id = ++(avnd->snd_msg_id);

	TRACE("Sending %u to %x", AVSV_D2N_SET_LEDS_MSG, avnd->node_info.nodeId);

	/* send the message */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		--(avnd->snd_msg_id);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_snd_comp_validation_resp
 *
 * Purpose:  This function sends a component validation resp to AvND.
 *
 * Input: cb        - Pointer to the AVD control block.
 *        avnd      - Pointer to AVND structure of the node.
 *        comp_ptr  - Pointer to the component. 
 *        n2d_msg   - Pointer to the message from AvND to AvD.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: If the component doesn't exist then node_id will be zero and 
 *        the result will be NCSCC_RC_FAILURE.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_comp_validation_resp(AVD_CL_CB *cb, AVD_AVND *avnd, AVD_COMP *comp_ptr, AVD_DND_MSG *n2d_msg)
{
	AVD_DND_MSG *d2n_msg = NULL;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER();

	/* prepare the component validation message. */
	d2n_msg = new AVSV_DND_MSG();

	/* prepare the componenet validation response message */
	d2n_msg->msg_type = AVSV_D2N_COMP_VALIDATION_RESP_MSG;

	d2n_msg->msg_info.d2n_comp_valid_resp_info.comp_name = n2d_msg->msg_info.n2d_comp_valid_info.comp_name;
	d2n_msg->msg_info.d2n_comp_valid_resp_info.msg_id = n2d_msg->msg_info.n2d_comp_valid_info.msg_id;
	if (NULL == comp_ptr) {
		/* We don't have the component, so node id is unavailable. */
		d2n_msg->msg_info.d2n_comp_valid_resp_info.node_id = 0;
		d2n_msg->msg_info.d2n_comp_valid_resp_info.result = AVSV_VALID_FAILURE;
	} else {
		/* Check whether this is an external or cluster component */
		if (true == comp_ptr->su->su_is_external) {
			/* There would not be any node associated with it. */
			d2n_msg->msg_info.d2n_comp_valid_resp_info.node_id = 0;
			su_node_ptr = cb->ext_comp_info.local_avnd_node;
		} else {
			d2n_msg->msg_info.d2n_comp_valid_resp_info.node_id = comp_ptr->su->su_on_node->node_info.nodeId;
			su_node_ptr = comp_ptr->su->su_on_node;
		}

		if ((AVD_AVND_STATE_PRESENT == su_node_ptr->node_state) ||
		    (AVD_AVND_STATE_NO_CONFIG == su_node_ptr->node_state) ||
		    (AVD_AVND_STATE_NCS_INIT == su_node_ptr->node_state)) {
			d2n_msg->msg_info.d2n_comp_valid_resp_info.result = AVSV_VALID_SUCC_COMP_NODE_UP;
		} else {
			/*  Component is not configured. */
			d2n_msg->msg_info.d2n_comp_valid_resp_info.result = AVSV_VALID_SUCC_COMP_NODE_DOWN;
		}
	}

	TRACE("Sending %u to %x", AVSV_D2N_COMP_VALIDATION_RESP_MSG, avnd->node_info.nodeId);

	/* send the message */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;

}

int avd_admin_state_is_valid(SaAmfAdminStateT state)
{
	return ((state >= SA_AMF_ADMIN_UNLOCKED) && (state <= SA_AMF_ADMIN_SHUTTING_DOWN));
}

/*****************************************************************************
 * Function: avd_object_name_create
 *
 * Purpose: This routine generates obj name using class name and parent name.
 *
 *
 * Input  : Rdn Attribute Value, Parent Name.
 *
 * Output  : Object name.
 *
 * Returns: Ok/Failure.
 *
 * NOTES  : None.
 *
 *
 **************************************************************************/
SaAisErrorT avd_object_name_create(SaNameT *rdn_attr_value, SaNameT *parentName, SaNameT *object_name)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t name_length = 0;

	memset(object_name, 0, sizeof(SaNameT));
	object_name->length = rdn_attr_value->length;

	if (object_name->length > SA_MAX_NAME_LENGTH)
		return SA_AIS_ERR_INVALID_PARAM;

	strcat((char *)&object_name->value[0], (char *)&rdn_attr_value->value[0]);
	strcat((char *)&object_name->value[0], ",");

	name_length = strlen((char *)&parentName->value[0]);
	object_name->length += name_length;

	if (object_name->length > SA_MAX_NAME_LENGTH)
		return SA_AIS_ERR_INVALID_PARAM;

	strncat((char *)&object_name->value[0], (char *)&parentName->value[0], name_length);

	return rc;
}

void avsv_sanamet_init_from_association_dn(const SaNameT *haystack, SaNameT *dn,
	const char *needle, const char *parent)
{
	char *p;
	char *pp;
	int i = 0;

	memset(dn, 0, sizeof(SaNameT));

	/* find what we actually are looking for */
	p = strstr((char*)haystack->value, needle);
	osafassert(p);

	/* find the parent */
	pp = strstr((char*)haystack->value, parent);
	osafassert(pp);

	/* position at parent separtor */
	pp--;

	/* copy the value upto parent but skip escape chars */
	while (p != pp) {
		if (*p != '\\')
			dn->value[i++] = *p;
		p++;
	}

	dn->length = strlen((char*)dn->value);
}

/**
 * Call this function from GDB to dump the AVD state. Example 
 * below.
 * 
 * @param path 
 */
/* gdb --pid=`pgrep ncs_scap` -ex 'call	avd_file_dump("/tmp/avd.dump")'	/usr/local/lib/opensaf/ncs_scap */
void avd_file_dump(const char *path)
{
	SaNameT dn = {0};
	FILE *f = fopen(path, "w");

	if (!f) {
		fprintf(stderr, "Could not open %s - %s\n", path, strerror(errno));
		return;
	}

	AVD_AVND *node;
	SaClmNodeIdT node_id = 0;
	while (NULL != (node = avd_node_getnext_nodeid(node_id))) {
		fprintf(f, "%s\n", node->name.value);
		fprintf(f, "\tsaAmfNodeAdminState=%u\n", node->saAmfNodeAdminState);
		fprintf(f, "\tsaAmfNodeOperState=%u\n", node->saAmfNodeOperState);
		fprintf(f, "\tnode_state=%u\n", node->node_state);
		fprintf(f, "\ttype=%u\n", node->type);
		fprintf(f, "\tadest=%" PRIx64 "\n", node->adest);
		fprintf(f, "\trcv_msg_id=%u\n", node->rcv_msg_id);
		fprintf(f, "\tsnd_msg_id=%u\n", node->snd_msg_id);
		fprintf(f, "\tnodeId=%x\n", node->node_info.nodeId);
		node_id = node->node_info.nodeId;
	}

	AVD_APP *app;
	dn.length = 0;
	for (app = avd_app_getnext(&dn); app != NULL; app = avd_app_getnext(&dn)) {
		fprintf(f, "%s\n", app->name.value);
		fprintf(f, "\tsaAmfApplicationAdminState=%u\n", app->saAmfApplicationAdminState);
		fprintf(f, "\tsaAmfApplicationCurrNumSGs=%u\n", app->saAmfApplicationCurrNumSGs);
		dn = app->name;
	}

	AVD_SG *sg;
	dn.length = 0;
	for (sg = avd_sg_getnext(&dn); sg != NULL; sg = avd_sg_getnext(&dn)) {
		fprintf(f, "%s\n", sg->name.value);
		fprintf(f, "\tsaAmfSGAdminState=%u\n", sg->saAmfSGAdminState);
		fprintf(f, "\tsaAmfSGNumCurrAssignedSUs=%u\n", sg->saAmfSGNumCurrAssignedSUs);
		fprintf(f, "\tsaAmfSGNumCurrInstantiatedSpareSUs=%u\n", sg->saAmfSGNumCurrInstantiatedSpareSUs);
		fprintf(f, "\tsaAmfSGNumCurrNonInstantiatedSpareSUs=%u\n", sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
		fprintf(f, "\tadjust_state=%u\n", sg->adjust_state);
		fprintf(f, "\tsg_fsm_state=%u\n", sg->sg_fsm_state);
		dn = sg->name;
	}

	AVD_SU *su;
	dn.length = 0;
	for (su = avd_su_getnext(&dn); su != NULL; su = avd_su_getnext(&dn)) {
		fprintf(f, "%s\n", su->name.value);
		fprintf(f, "\tsaAmfSUPreInstantiable=%u\n", su->saAmfSUPreInstantiable);
		fprintf(f, "\tsaAmfSUOperState=%u\n", su->saAmfSUOperState);
		fprintf(f, "\tsaAmfSUAdminState=%u\n", su->saAmfSUAdminState);
		fprintf(f, "\tsaAmfSuReadinessState=%u\n", su->saAmfSuReadinessState);
		fprintf(f, "\tsaAmfSUPresenceState=%u\n", su->saAmfSUPresenceState);
		fprintf(f, "\tsaAmfSUHostedByNode=%s\n", su->saAmfSUHostedByNode.value);
		fprintf(f, "\tsaAmfSUNumCurrActiveSIs=%u\n", su->saAmfSUNumCurrActiveSIs);
		fprintf(f, "\tsaAmfSUNumCurrStandbySIs=%u\n", su->saAmfSUNumCurrStandbySIs);
		fprintf(f, "\tsaAmfSURestartCount=%u\n", su->saAmfSURestartCount);
		fprintf(f, "\tterm_state=%u\n", su->term_state);
		fprintf(f, "\tsu_switch=%u\n", su->su_switch);
		fprintf(f, "\tsu_act_state=%u\n", su->su_act_state);
		dn = su->name;
	}

	AVD_COMP *comp;
	dn.length = 0;
	for (comp = avd_comp_getnext(&dn); comp != NULL; comp = avd_comp_getnext(&dn)) {
		fprintf(f, "%s\n", comp->comp_info.name.value);
		fprintf(f, "\tsaAmfCompOperState=%u\n", comp->saAmfCompOperState);
		fprintf(f, "\tsaAmfCompReadinessState=%u\n", comp->saAmfCompReadinessState);
		fprintf(f, "\tsaAmfCompPresenceState=%u\n", comp->saAmfCompPresenceState);
		fprintf(f, "\tsaAmfCompRestartCount=%u\n", comp->saAmfCompRestartCount);
		fprintf(f, "\tsaAmfCompOperState=%s\n", comp->saAmfCompCurrProxyName.value);
		dn = comp->comp_info.name;
	}

	AVD_SI *si;
	dn.length = 0;
	for (si = avd_si_getnext(&dn); si != NULL; si = avd_si_getnext(&dn)) {
		fprintf(f, "%s\n", si->name.value);
		fprintf(f, "\tsaAmfSIAdminState=%u\n", si->saAmfSIAdminState);
		fprintf(f, "\tsaAmfSIAssignmentStatee=%u\n", si->saAmfSIAssignmentState);
		fprintf(f, "\tsaAmfSINumCurrActiveAssignments=%u\n", si->saAmfSINumCurrActiveAssignments);
		fprintf(f, "\tsaAmfSINumCurrStandbyAssignments=%u\n", si->saAmfSINumCurrStandbyAssignments);
		fprintf(f, "\tsi_switch=%u\n", si->si_switch);
		dn = si->name;
	}

	AVD_SU_SI_REL *rel;
	dn.length = 0;
	for (su = avd_su_getnext(&dn); su != NULL; su = avd_su_getnext(&dn)) {
		for (rel = su->list_of_susi; rel != NULL; rel = rel->su_next) {
			fprintf(f, "%s,%s\n", rel->su->name.value, rel->si->name.value);
			fprintf(f, "\thastate=%u\n", rel->state);
			fprintf(f, "\tfsm=%u\n", rel->fsm);
		}
		dn = su->name;
	}

	dn.length = 0;
	AVD_COMPCS_TYPE *compcstype;
	for (compcstype = avd_compcstype_getnext(&dn); compcstype != NULL;
	     compcstype = avd_compcstype_getnext(&dn)) {
		fprintf(f, "%s\n", compcstype->name.value);
		fprintf(f, "\tsaAmfCompNumCurrActiveCSIs=%u\n", compcstype->saAmfCompNumCurrActiveCSIs);
		fprintf(f, "\tsaAmfCompNumCurrStandbyCSIs=%u\n", compcstype->saAmfCompNumCurrStandbyCSIs);
		dn = compcstype->name;
	}

	fclose(f);
}

/**
 * Send an admin op request message to the node director
 * @param dn
 * @param class_id
 * @param opId
 * @param node
 * 
 * @return int
 */
int avd_admin_op_msg_snd(const SaNameT *dn, AVSV_AMF_CLASS_ID class_id,
	SaAmfAdminOperationIdT opId, AVD_AVND *node)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;
	AVD_DND_MSG *d2n_msg;
	unsigned int rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2(" '%s' %u", dn->value, opId);

	d2n_msg = new AVSV_DND_MSG();

	d2n_msg->msg_type = AVSV_D2N_ADMIN_OP_REQ_MSG;
	d2n_msg->msg_info.d2n_admin_op_req_info.msg_id = ++(node->snd_msg_id);
	d2n_msg->msg_info.d2n_admin_op_req_info.dn = *dn;
	d2n_msg->msg_info.d2n_admin_op_req_info.class_id = class_id;
	d2n_msg->msg_info.d2n_admin_op_req_info.oper_id = opId;

	rc = avd_d2n_msg_snd(cb, node, d2n_msg);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		--(node->snd_msg_id);
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, node, AVSV_CKPT_AVND_SND_MSG_ID);

	TRACE_LEAVE2("(%u)", rc);
	return rc;
}

/**
 * returns the parent of a DN (including DNs with escaped RDNs)
 *
 * @param dn
 */
const char* avd_getparent(const char* dn)
{
	const char* parent = dn;
	const char* tmp_parent;

	/* Check if there exist any escaped RDN in the DN */
	tmp_parent = strrchr(dn, '\\');
	if (tmp_parent != NULL) {
		parent = tmp_parent + 2;
	}

	if ((parent = strchr((char*)parent, ',')) != NULL) {
		parent++;
	}

	return parent;
}

/**
 * Verify the existence of an object in IMM
 * @param dn
 * 
 * @return bool
 */
bool object_exist_in_imm(const SaNameT *dn)
{
	bool rc = false;
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAttrNameT attributeNames[] = {const_cast<SaImmAttrNameT>("SaImmAttrClassName"), NULL};

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);

	if (immutil_saImmOmAccessorGet_2(accessorHandle, dn, attributeNames,
									 (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK)
		rc = true;

	immutil_saImmOmAccessorFinalize(accessorHandle);

	return rc;
}

const char *admin_op_name(SaAmfAdminOperationIdT opid)
{
	if (opid <= SA_AMF_ADMIN_EAM_STOP) {
		return avd_adm_op_name[opid];
	} else
		return avd_adm_op_name[0];
}


/*****************************************************************************
 * Function: free_d2n_su_msg_info
 *
 * Purpose:  This function frees the d2n SU message contents.
 *
 * Input: su_msg - Pointer to the SU message contents to be freed.
 *
 * Returns: None
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static void free_d2n_su_msg_info(AVSV_DND_MSG *su_msg)
{
	AVSV_SU_INFO_MSG *su_info;

	while (su_msg->msg_info.d2n_reg_su.su_list != NULL) {
		su_info = su_msg->msg_info.d2n_reg_su.su_list;
		su_msg->msg_info.d2n_reg_su.su_list = su_info->next;
		delete su_info;
	}
}


/*****************************************************************************
 * Function: free_d2n_susi_msg_info
 *
 * Purpose:  This function frees the d2n SU SI message contents.
 *
 * Input: susi_msg - Pointer to the SUSI message contents to be freed.
 *
 * Returns: none
 *
 * NOTES: It also frees the array of attributes, which are sperately
 * allocated and pointed to by AVSV_SUSI_ASGN structure.
 *
 * 
 **************************************************************************/

static void free_d2n_susi_msg_info(AVSV_DND_MSG *susi_msg)
{
	AVSV_SUSI_ASGN *compcsi_info;

	while (susi_msg->msg_info.d2n_su_si_assign.list != NULL) {
		compcsi_info = susi_msg->msg_info.d2n_su_si_assign.list;
		susi_msg->msg_info.d2n_su_si_assign.list = compcsi_info->next;
		if (compcsi_info->attrs.list != NULL) {
			delete [] (compcsi_info->attrs.list);
			compcsi_info->attrs.list = NULL;
		}
		delete compcsi_info;
	}
}

/*****************************************************************************
 * Function: free_d2n_pg_msg_info
 *
 * Purpose:  This function frees the d2n PG track response message contents.
 *
 * Input: pg_msg - Pointer to the PG message contents to be freed.
 *
 * Returns: None
 *
 * NOTES: None
 *
 * 
 **************************************************************************/

static void free_d2n_pg_msg_info(AVSV_DND_MSG *pg_msg)
{
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info = &pg_msg->msg_info.d2n_pg_track_act_rsp;

	if (info->mem_list.numberOfItems)
		delete [] info->mem_list.notification;

	info->mem_list.notification = 0;
	info->mem_list.numberOfItems = 0;
}

/****************************************************************************
  Name          : d2n_msg_free
 
  Description   : This routine frees the Message structures used for
                  communication between AvD and AvND. 
 
  Arguments     : msg - ptr to the DND message that needs to be freed.
 
  Return Values : None
 
  Notes         : For : AVSV_D2N_REG_SU_MSG, AVSV_D2N_INFO_SU_SI_ASSIGN_MSG
                  and AVSV_D2N_PG_TRACK_ACT_RSP_MSG, this procedure calls the
                  corresponding information free function to free the
                  list information in them before freeing the message.
******************************************************************************/
void d2n_msg_free(AVSV_DND_MSG *msg)
{
	if (msg == NULL)
		return;

	/* these messages have information list in them free them
	 * first by calling the corresponding free routine.
	 */
	switch (msg->msg_type) {
	case AVSV_D2N_REG_SU_MSG:
		free_d2n_su_msg_info(msg);
		break;
	case AVSV_D2N_INFO_SU_SI_ASSIGN_MSG:
		free_d2n_susi_msg_info(msg);
		break;
	case AVSV_D2N_PG_TRACK_ACT_RSP_MSG:
		free_d2n_pg_msg_info(msg);
		break;
	default:
		break;
	}

	/* free the message */
	delete msg;
}

/**
 * Sends a reboot command to amfnd
 * @param node
 */
void avd_d2n_reboot_snd(AVD_AVND *node)
{
	TRACE("Sending REBOOT MSG to %x", node->node_info.nodeId);

	AVD_DND_MSG *d2n_msg = new AVD_DND_MSG();

	d2n_msg->msg_type = AVSV_D2N_REBOOT_MSG;
	d2n_msg->msg_info.d2n_reboot_info.node_id = node->node_info.nodeId;
	d2n_msg->msg_info.d2n_reboot_info.msg_id = ++(node->snd_msg_id);

	if (avd_d2n_msg_snd(avd_cb, node, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
		d2n_msg_free(d2n_msg);
	}
}

/**
 * Logs to saflog if active
 * @param priority
 * @param format
 */
void amflog(int priority, const char *format, ...)
{
	if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
		va_list ap;
		char str[256];

		va_start(ap, format);
		vsnprintf(str, sizeof(str), format, ap);
		va_end(ap);
		saflog(priority, amfSvcUsrName, "%s", str);
	}
}
