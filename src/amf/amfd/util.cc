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

#include <algorithm>
#include <vector>
#include <string.h>

#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"
#include "amf/amfd/amfd.h"

SaNameT _amfSvcUsrName;
const SaNameT* amfSvcUsrName = &_amfSvcUsrName;

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

const char *amf_recovery[] = {
    "UNKNOWN_RECOVERY",
    "NO_RECOMMENDATION" ,
    "COMPONENT_RESTART",
    "COMPONENT_FAILOVER",
    "NODE_SWITCHOVER",
    "NODE_FAILOVER",
    "NODE_FAILFAST",
    "CLUSTER_RESET",
    "APPLICATION_RESTART",
    "CONTAINER_RESTART" 
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
	if (avnd == nullptr) {
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
	if (avnd == nullptr) {
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
	d2n_msg->msg_info.d2n_node_up.node_type = AVSV_AVND_CARD_SYS_CON;
	d2n_msg->msg_info.d2n_node_up.su_failover_max = avnd->saAmfNodeSuFailoverMax;
	d2n_msg->msg_info.d2n_node_up.su_failover_prob = avnd->saAmfNodeSuFailOverProb;

	TRACE("Sending %u to %x", AVSV_D2N_NODE_UP_MSG, avnd->node_info.nodeId);

	/* Now send the message to the node director */
	if (avd_d2n_msg_snd(cb, avnd, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, avnd->node_info.nodeId);
		d2n_msg_free(d2n_msg);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
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
	if (avnd == nullptr) {
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
	AVD_AVND *node = su->get_node_ptr();

	TRACE_ENTER2("%s '%s'", (term_state == true) ? "Terminate" : "Instantiate",
		su->name.c_str());

	/* prepare the node update message. */
	d2n_msg = new AVSV_DND_MSG();
	SaNameT su_name;
	osaf_extended_name_alloc(su->name.c_str(), &su_name);

	/* prepare the SU presence state change notification message */
	d2n_msg->msg_type = AVSV_D2N_PRESENCE_SU_MSG;
	d2n_msg->msg_info.d2n_prsc_su.msg_id = ++(node->snd_msg_id);
	d2n_msg->msg_info.d2n_prsc_su.node_id = node->node_info.nodeId;
	d2n_msg->msg_info.d2n_prsc_su.su_name = su_name;
	d2n_msg->msg_info.d2n_prsc_su.term_state = term_state;

	TRACE("Sending %u to %x", AVSV_D2N_PRESENCE_SU_MSG, node->node_info.nodeId);

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
 *               the message. Non nullptr if message need to be sent to a
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

	if (avnd == nullptr) {
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

	osaf_extended_name_alloc(osaf_extended_name_borrow(&param_info->name),
		&op_req_msg->msg_info.d2n_op_req.param_info.name);

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

/**
 * Creates and initializes the su_info part of the REG_SU message
 * @param su_msg
 * @param su
 */
static void reg_su_msg_init_su_info(AVD_DND_MSG *su_msg, const AVD_SU *su)
{
	AVSV_SU_INFO_MSG *su_info = new AVSV_SU_INFO_MSG();

	SaNameT su_name;
	osaf_extended_name_alloc(su->name.c_str(), &su_name);

	su_info->name = su_name;
	su_info->comp_restart_max = su->sg_of_su->saAmfSGCompRestartMax;
	su_info->comp_restart_prob = su->sg_of_su->saAmfSGCompRestartProb;
	su_info->su_restart_max = su->sg_of_su->saAmfSGSuRestartMax;
	su_info->su_restart_prob = su->sg_of_su->saAmfSGSuRestartProb;
	su_info->is_ncs = su->sg_of_su->sg_ncs_spec;
	su_info->su_is_external = su->su_is_external;
	su_info->su_failover = su->saAmfSUFailover;

	su_info->next = su_msg->msg_info.d2n_reg_su.su_list;
	su_msg->msg_info.d2n_reg_su.su_list = su_info;
	su_msg->msg_info.d2n_reg_su.num_su++;
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
	AVD_SU *su = nullptr;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", avnd->node_name.c_str());

	AVD_DND_MSG *su_msg = new AVSV_DND_MSG();
	su_msg->msg_type = AVSV_D2N_REG_SU_MSG;
	su_msg->msg_info.d2n_reg_su.nodeid = avnd->node_info.nodeId;
	su_msg->msg_info.d2n_reg_su.msg_on_fover = fail_over;

	// Add osaf SUs
	for (const auto& su : avnd->list_of_ncs_su)
		reg_su_msg_init_su_info(su_msg, su);

	// Add app SUs
	for (const auto& su : avnd->list_of_su)
		reg_su_msg_init_su_info(su_msg, su);

	// Add external SUs but only if node belongs to ACT controller
	if (avnd->node_info.nodeId == cb->node_id_avd) {
		// filter out external SUs from all SUs
		std::vector<AVD_SU*> ext_su_vec;
		for (std::map<std::string, AVD_SU*>::const_iterator it = su_db->begin();
				it != su_db->end(); it++) {
			su = it->second;
			if (su->su_is_external == true)
				ext_su_vec.push_back(su);
		}

		// And add them
		for (std::vector<AVD_SU*>::iterator it = ext_su_vec.begin();
				it != ext_su_vec.end(); it++) {
			su = *it;
			reg_su_msg_init_su_info(su_msg, su);
		}
	}

	/* check if atleast one SU data is being sent. If not 
	 * dont send the messages.
	 */
	if (su_msg->msg_info.d2n_reg_su.su_list == nullptr) {
		/* Free all the messages and return success */
		d2n_msg_free(su_msg);
		goto done;
	}

	su_msg->msg_info.d2n_reg_su.msg_id = ++(avnd->snd_msg_id);

	/* send the SU message to the node if return value is failure
	 * free messages and return error.
	 */

	TRACE("Sending AVSV_D2N_REG_SU_MSG to %s", avnd->node_name.c_str());

	if (avd_d2n_msg_snd(cb, avnd, su_msg) == NCSCC_RC_FAILURE) {
		--(avnd->snd_msg_id);
		LOG_ER("%s: snd to %s failed", __FUNCTION__, avnd->node_name.c_str());
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
	TRACE_ENTER();

	AVD_AVND *node = su->get_node_ptr();
	AVD_DND_MSG *su_msg = new AVSV_DND_MSG();
	su_msg->msg_type = AVSV_D2N_REG_SU_MSG;
	su_msg->msg_info.d2n_reg_su.nodeid = node->node_info.nodeId;
	reg_su_msg_init_su_info(su_msg, su);
	su_msg->msg_info.d2n_reg_su.msg_id = ++(node->snd_msg_id);

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
	while ((attr_ptr != nullptr) && (compcsi_info->attrs.number < compcsi->csi->num_attributes)) {
		memcpy(i_ptr, &attr_ptr->name_value, sizeof(AVSV_ATTR_NAME_VAL));
		osaf_extended_name_alloc(osaf_extended_name_borrow(&attr_ptr->name_value.name),
				&i_ptr->name);
		osaf_extended_name_alloc(osaf_extended_name_borrow(&attr_ptr->name_value.value),
				&i_ptr->value);
		if (attr_ptr->name_value.string_ptr != nullptr) {
			i_ptr->string_ptr = new char[strlen(attr_ptr->name_value.string_ptr)+1];
			memcpy(i_ptr->string_ptr, attr_ptr->name_value.string_ptr,
					strlen(attr_ptr->name_value.string_ptr)+1);
		} else {
			i_ptr->string_ptr = nullptr;
		}
		compcsi_info->attrs.number++;
		i_ptr = i_ptr + 1;
		attr_ptr = attr_ptr->attr_next;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Get saAmfCtCompCapability from the SaAmfCtCsType instance associated with the
 * types of csi and comp.
 * @param csi
 * @param comp
 * @return SaAmfCompCapabilityModelT
 */
static SaAmfCompCapabilityModelT get_comp_capability(const AVD_CSI *csi,
		const AVD_COMP *comp)
{
	SaNameT dn;
	const SaNameTWrapper cstype_name(csi->cstype->name);
	const SaNameTWrapper comp_type_name(comp->comp_type->name);
	avsv_create_association_class_dn(cstype_name,
		comp_type_name,	"safSupportedCsType", &dn);
	AVD_CTCS_TYPE *ctcs_type = ctcstype_db->find(Amf::to_string(&dn));
	osaf_extended_name_free(&dn);
	return ctcs_type->saAmfCtCompCapability;
}

/*****************************************************************************
 * Function: avd_snd_susi_msg
 *
 * Purpose:  This function prepares the SU SI message for the given SU and 
 * its SUSI if any. 
 *
 * Input: cb - Pointer to the AVD control block *        
 *        su - Pointer to the SU related to which the messages need to be sent.
 *      susi - SU SI relationship struct pointer. Is nullptr when all the 
 *             SI assignments need to be deleted
 *             or when all the SIs of the SU need to change role.
 *      actn - The action value that needs to be sent in the message.
 *      single_csi - If a single component is to be assigned as csi.
 *      compcsi - Comp CSI assignment(signle comp and associated csi) if single_csi is true else nullptr.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_snd_susi_msg(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
		AVSV_SUSI_ACT actn, bool single_csi, AVD_COMP_CSI_REL *compcsi) {
	AVD_DND_MSG *susi_msg;
	AVD_COMP_CSI_REL *l_compcsi;
	AVD_SU_SI_REL *l_susi, *i_susi;
	AVSV_SUSI_ASGN *compcsi_info;
	SaAmfCSITransitionDescriptorT trans_dsc;

	TRACE_ENTER();

	AVD_AVND *avnd = su->get_node_ptr();

	/* Need not proceed further if node is not in proper state */
	if ((avnd->node_state == AVD_AVND_STATE_ABSENT) ||
			(avnd->node_state == AVD_AVND_STATE_GO_DOWN)) {
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}

	/* Initialize the local variables to avoid warnings */
	l_susi = susi;
	trans_dsc = SA_AMF_CSI_NEW_ASSIGN;

	/* prepare the SU SI message. */
	susi_msg = new AVSV_DND_MSG();

	// this needs to be freed later
	SaNameT su_name;
	osaf_extended_name_alloc(su->name.c_str(), &su_name);

	// this needs to be freed later
	SaNameT csi_name;

	// this needs to be freed later
	SaNameT si_name;
	
	if (susi != AVD_SU_SI_REL_NULL) {
		osaf_extended_name_alloc(susi->si->name.c_str(), &si_name);
		susi_msg->msg_info.d2n_su_si_assign.si_name = si_name;
	}

	susi_msg->msg_type = AVSV_D2N_INFO_SU_SI_ASSIGN_MSG;
	susi_msg->msg_info.d2n_su_si_assign.node_id = avnd->node_info.nodeId;
	susi_msg->msg_info.d2n_su_si_assign.su_name = su_name;
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
			/* For only these options fill the comp CSI values. */
			if (true == single_csi) {
				osafassert(compcsi);
				l_compcsi = compcsi;

				compcsi_info = new AVSV_SUSI_ASGN();

				SaNameT comp_name;
				osaf_extended_name_alloc(osaf_extended_name_borrow(&l_compcsi->comp->comp_info.name),
					&comp_name);
				compcsi_info->comp_name = comp_name;
				osaf_extended_name_alloc(l_compcsi->csi->name.c_str(), &csi_name);
				compcsi_info->csi_name = csi_name;
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
				TRACE_LEAVE();
				return NCSCC_RC_FAILURE;
			}

			/* use this SU SI modification information to fill the message.
			 */

			susi_msg->msg_info.d2n_su_si_assign.ha_state = l_susi->state;
		} else {	/* if (susi == AVD_SU_SI_REL_NULL) */

			/* for modifications of a SU SI fill the SI name.
			 */

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
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		/* for new assignments of a SU SI fill the SI name.
		 */

		susi_msg->msg_info.d2n_su_si_assign.ha_state = susi->state;
		susi_msg->msg_info.d2n_su_si_assign.si_rank = susi->si->saAmfSIRank;

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
		TRACE_LEAVE();
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

		while (l_compcsi != nullptr) {
			compcsi_info = new AVSV_SUSI_ASGN();

			SaNameT comp_name;
			osaf_extended_name_alloc(osaf_extended_name_borrow(&l_compcsi->comp->comp_info.name),
					&comp_name);
			compcsi_info->comp_name = comp_name;
			osaf_extended_name_alloc(l_compcsi->csi->name.c_str(), &csi_name);
				
			compcsi_info->csi_name = csi_name;
			compcsi_info->csi_rank = l_compcsi->csi->rank;
			compcsi_info->active_comp_dsc = trans_dsc;
			compcsi_info->capability =
				get_comp_capability(l_compcsi->csi, l_compcsi->comp);

			/* fill the Attributes for the CSI if new. */
			if (actn == AVSV_SUSI_ACT_ASGN) {
				if (avd_prep_csi_attr_info(cb, compcsi_info, l_compcsi)
				    == NCSCC_RC_FAILURE) {
					LOG_EM("%s:%u: %u", __FILE__, __LINE__, avnd->node_info.nodeId);
					d2n_msg_free(susi_msg);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}

			/* fill the quiesced and active component name info */
			if (!((actn == AVSV_SUSI_ACT_ASGN) &&
			      (susi_msg->msg_info.d2n_su_si_assign.ha_state == SA_AMF_HA_ACTIVE))) {
				if (l_compcsi->csi_csicomp_next == nullptr) {
					if ((l_compcsi->csi->list_compcsi != nullptr) &&
					    (l_compcsi->csi->list_compcsi != l_compcsi)) {
						osaf_extended_name_alloc(osaf_extended_name_borrow(&l_compcsi->csi->list_compcsi->comp->comp_info.name),
							&compcsi_info->active_comp_name);

						if ((trans_dsc == SA_AMF_CSI_QUIESCED) &&
						    (l_compcsi->csi->list_compcsi->comp->saAmfCompOperState
						     == SA_AMF_OPERATIONAL_DISABLED)) {
							compcsi_info->active_comp_dsc = SA_AMF_CSI_NOT_QUIESCED;
						}
					}
				} else {
					osaf_extended_name_alloc(osaf_extended_name_borrow(&l_compcsi->csi_csicomp_next->comp->comp_info.name),
						&compcsi_info->active_comp_name);

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
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

	TRACE_LEAVE();
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
			SaNameT comp_name;
			osaf_extended_name_alloc(osaf_extended_name_borrow(&curr->comp->comp_info.name),
				&comp_name);
			mem_list->notification[i].member.compName = comp_name;
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
	SaNameT csi_name;
	osaf_extended_name_alloc(osaf_extended_name_borrow(&n2d_msg->csi_name), &csi_name);
	pg_msg_info->csi_name = csi_name;
	pg_msg_info->msg_id_ack = n2d_msg->msg_id;
	pg_msg_info->node_id = n2d_msg->node_id;
	pg_msg_info->msg_on_fover = n2d_msg->msg_on_fover;

	/* prepare the msg based on the action */
	switch (n2d_msg->actn) {
	case AVSV_PG_TRACK_ACT_START:
		{
			if (csi != nullptr) {
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
			 AVD_COMP_CSI_REL *comp_csi, SaAmfProtectionGroupChangesT change, const std::string& csi_name)
{
	AVD_DND_MSG *pg_msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_D2N_PG_UPD_MSG_INFO *pg_msg_info = 0;
	SaNameT temp_csi_name;
	SaNameT comp_name;
	TRACE_ENTER();

	/* alloc the update msg */
	pg_msg = new AVSV_DND_MSG();

	pg_msg_info = &pg_msg->msg_info.d2n_pg_upd;

	/* populate the msg */
	pg_msg->msg_type = AVSV_D2N_PG_UPD_MSG;
	pg_msg_info->node_id = node->node_info.nodeId;
	pg_msg_info->is_csi_del = (csi_name.empty() == false) ? true : false;
	if (false == pg_msg_info->is_csi_del) {
		osaf_extended_name_alloc(comp_csi->csi->name.c_str(), &temp_csi_name);
		pg_msg_info->csi_name = temp_csi_name;
		osaf_extended_name_alloc(osaf_extended_name_borrow(&comp_csi->comp->comp_info.name),
			&comp_name);
		pg_msg_info->mem.member.compName = comp_name;
		pg_msg_info->mem.member.haState = comp_csi->susi->state;
		pg_msg_info->mem.member.rank = comp_csi->comp->su->saAmfSURank;
		pg_msg_info->mem.change = change;
	} else {
		osaf_extended_name_alloc(csi_name.c_str(), &temp_csi_name);
		pg_msg_info->csi_name = temp_csi_name;
	}
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
	if (avnd == nullptr) {
		/* This is a invalid situation as the avnd record
		 * needs to be mentioned.
		 */

		/* Log a fatal error that AvND record can't be null */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, 0);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
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
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

	TRACE_LEAVE();

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
	AVD_DND_MSG *d2n_msg = nullptr;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER();

	/* prepare the component validation message. */
	d2n_msg = new AVSV_DND_MSG();

	/* prepare the componenet validation response message */
	d2n_msg->msg_type = AVSV_D2N_COMP_VALIDATION_RESP_MSG;

	d2n_msg->msg_info.d2n_comp_valid_resp_info.comp_name = n2d_msg->msg_info.n2d_comp_valid_info.comp_name;
	d2n_msg->msg_info.d2n_comp_valid_resp_info.msg_id = n2d_msg->msg_info.n2d_comp_valid_info.msg_id;
	if (nullptr == comp_ptr) {
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

int avd_admin_state_is_valid(SaAmfAdminStateT state, const CcbUtilOperationData_t *opdata)
{
	if (opdata == nullptr) {
		return ((state >= SA_AMF_ADMIN_UNLOCKED) && (state <= SA_AMF_ADMIN_SHUTTING_DOWN));
	} else {
		return ((state >= SA_AMF_ADMIN_UNLOCKED) && (state < SA_AMF_ADMIN_SHUTTING_DOWN));
	}
}

/**
 * Dumps the director state to file
 * This can be done using an admin command:
 * immadm -o 99 <DN of AMF Cluster>
 * @param filename path to file or nullptr if a filename should be generated
 * @return 0 at OK, errno otherwise
 */
int amfd_file_dump(const char *filename)
{
	const int PATH_LEN = 128;
	const AVD_SU_SI_REL *susi;
	const AVD_SI *si;
	const AVD_CSI *csi;
	const char *path = filename;
	char _path[PATH_LEN];

	if (filename == nullptr) {
		// add unique suffix to state file
		snprintf(_path, PATH_LEN, "/tmp/amfd.state.%d", getpid());
		path = _path;
	}

	FILE *f = fopen(path, "w");

	if (!f)
		return errno;

	LOG_NO("dumping state to file %s", path);

	fprintf(f, "control_block:\n");
	fprintf(f, "  avail_state_avd: %d\n", avd_cb->avail_state_avd);
	fprintf(f, "  avd_fover_state: %d\n", avd_cb->avd_fover_state);
	fprintf(f, "  init_state: %d\n", avd_cb->init_state);
	fprintf(f, "  other_avd_adest: %" PRIx64 "\n", avd_cb->other_avd_adest);
	fprintf(f, "  local_avnd_adest: %" PRIx64 "\n", avd_cb->local_avnd_adest);
	fprintf(f, "  stby_sync_state: %d\n", avd_cb->stby_sync_state);
	fprintf(f, "  synced_reo_type: %d\n", avd_cb->synced_reo_type);
	fprintf(f, "  cluster_init_time: %llu\n", avd_cb->cluster_init_time);
	fprintf(f, "  peer_msg_fmt_ver: %d\n", avd_cb->peer_msg_fmt_ver);
	fprintf(f, "  avd_peer_ver: %d\n", avd_cb->avd_peer_ver);

	fprintf(f,"CKPT update counts:\n");
	fprintf(f, "  cb_updt:%d\n",avd_cb->async_updt_cnt.cb_updt);
	fprintf(f, "  node_updt:%d\n",avd_cb->async_updt_cnt.node_updt);
	fprintf(f, "  app_updt:%d\n",avd_cb->async_updt_cnt.app_updt);
	fprintf(f, "  sg_updt:%d\n",avd_cb->async_updt_cnt.sg_updt);
	fprintf(f, "  su_updt:%d\n",avd_cb->async_updt_cnt.su_updt);
	fprintf(f, "  si_updt:%d\n",avd_cb->async_updt_cnt.si_updt);
	fprintf(f, "  sg_su_oprlist_updt:%d\n",avd_cb->async_updt_cnt.sg_su_oprlist_updt);
	fprintf(f, "  sg_admin_si_updt:%d\n",avd_cb->async_updt_cnt.sg_admin_si_updt);
	fprintf(f, "  siass_updt:%d\n",avd_cb->async_updt_cnt.siass_updt);
	fprintf(f, "  comp_updt:%d\n",avd_cb->async_updt_cnt.comp_updt);
	fprintf(f, "  csi_updt:%d\n",avd_cb->async_updt_cnt.csi_updt);
	fprintf(f, "  compcstype_updt:%d\n",avd_cb->async_updt_cnt.compcstype_updt);
	fprintf(f, "  si_trans_updt:%d\n",avd_cb->async_updt_cnt.si_trans_updt);
	fprintf(f, "  ng_updt:%d\n",avd_cb->async_updt_cnt.ng_updt);


	fprintf(f, "nodes:\n");
	for (std::map<uint32_t, AVD_AVND *>::const_iterator it = node_id_db->begin();
			it != node_id_db->end(); it++) {
		AVD_AVND *node = it->second;
		fprintf(f, "  dn: %s\n", node->name.c_str());
		fprintf(f, "    saAmfNodeAdminState: %s\n",
				avd_adm_state_name[node->saAmfNodeAdminState]);
		fprintf(f, "    saAmfNodeOperState: %s\n",
				avd_oper_state_name[node->saAmfNodeOperState]);
		fprintf(f, "    node_state: %u\n", node->node_state);
		fprintf(f, "    adest:%" PRIx64 "\n", node->adest);
		fprintf(f, "    rcv_msg_id: %u\n", node->rcv_msg_id);
		fprintf(f, "    snd_msg_id: %u\n", node->snd_msg_id);
		fprintf(f, "    nodeId: %x\n", node->node_info.nodeId);
	}

	fprintf(f, "applications:\n");
	for (std::map<std::string, AVD_APP*>::const_iterator it = app_db->begin();
			it != app_db->end(); it++) {
		const AVD_APP *app = it->second;
		fprintf(f, "  dn: %s\n", app->name.c_str());
		fprintf(f, "    saAmfApplicationAdminState: %s\n",
				avd_adm_state_name[app->saAmfApplicationAdminState]);
		fprintf(f, "    saAmfApplicationCurrNumSGs: %u\n",
				app->saAmfApplicationCurrNumSGs);
	}

	fprintf(f, "service_instances:\n");
	for (std::map<std::string, AVD_SI*>::const_iterator it = si_db->begin();
			it != si_db->end(); it++) {
		si = it->second;
		fprintf(f, "  dn: %s\n", si->name.c_str());
		fprintf(f, "    saAmfSIProtectedbySG: %s\n",
				si->saAmfSIProtectedbySG.c_str());
		fprintf(f, "    saAmfSIAdminState: %s\n",
				avd_adm_state_name[si->saAmfSIAdminState]);
		fprintf(f, "    saAmfSIAssignmentState: %s\n",
				avd_ass_state[si->saAmfSIAssignmentState]);
		fprintf(f, "    saAmfSINumCurrActiveAssignments: %u\n",
				si->saAmfSINumCurrActiveAssignments);
		fprintf(f, "    saAmfSINumCurrStandbyAssignments: %u\n",
				si->saAmfSINumCurrStandbyAssignments);
		fprintf(f, "    si_switch: %u\n", si->si_switch);
		fprintf(f, "    si_dep_state: %u\n", si->si_dep_state);
		fprintf(f, "    num_dependents: %u\n", si->num_dependents);
		fprintf(f, "    alarm_sent: %u\n", si->alarm_sent);
		fprintf(f, "    assigned_to_sus:\n");
		for (susi = si->list_of_sisu; susi; susi = susi->si_next) {
			fprintf(f, "      dn: %s\n", susi->su->name.c_str());
			fprintf(f, "        hastate: %s\n", avd_ha_state[susi->state]);
			fprintf(f, "        fsm: %u\n", susi->fsm);
		}
	}

	fprintf(f, "component_service_instances:\n");
	for (std::map<std::string, AVD_CSI*>::const_iterator it = csi_db->begin();
			it != csi_db->end(); it++) {
		csi = it->second;
		fprintf(f, "  dn: %s\n", csi->name.c_str());
		fprintf(f, "    rank: %u\n", csi->rank);
		fprintf(f, "    depends:\n");
		AVD_CSI_DEPS *dep;
		for (dep = csi->saAmfCSIDependencies; dep; dep = dep->csi_dep_next)
			fprintf(f, "      %s", dep->csi_dep_name_value.c_str());
		if (csi->saAmfCSIDependencies)
			fprintf(f, "\n");
		fprintf(f, "    assigned_to_components:\n");
		AVD_COMP_CSI_REL *compcsi;
		for (compcsi = csi->list_compcsi; compcsi; compcsi = compcsi->csi_csicomp_next) {
			fprintf(f, "      dn: %s\n", osaf_extended_name_borrow(&compcsi->comp->comp_info.name));
		}
	}

	fprintf(f, "service_groups:\n");
	for (std::map<std::string, AVD_SG*>::const_iterator it = sg_db->begin();
			it != sg_db->end(); it++) {
		const AVD_SG *sg = it->second;
		fprintf(f, "  dn: %s\n", sg->name.c_str());
		fprintf(f, "    saAmfSGAdminState: %s\n",
				avd_adm_state_name[sg->saAmfSGAdminState]);
		fprintf(f, "    saAmfSGNumCurrAssignedSUs: %u\n",
				sg->saAmfSGNumCurrAssignedSUs);
		fprintf(f, "    saAmfSGNumCurrInstantiatedSpareSUs: %u\n",
				sg->saAmfSGNumCurrInstantiatedSpareSUs);
		fprintf(f, "    saAmfSGNumCurrNonInstantiatedSpareSUs: %u\n",
				sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
		fprintf(f, "    adjust_state: %u\n", sg->adjust_state);
		fprintf(f, "    sg_fsm_state: %u\n", sg->sg_fsm_state);
	}

	fprintf(f, "service_units:\n");
	for (std::map<std::string, AVD_SU*>::const_iterator it = su_db->begin();
			it != su_db->end(); it++) {
		const AVD_SU *su = it->second;
		fprintf(f, "  dn: %s\n", su->name.c_str());
		fprintf(f, "    saAmfSUPreInstantiable: %u\n", su->saAmfSUPreInstantiable);
		fprintf(f, "    saAmfSUOperState: %s\n",
				avd_oper_state_name[su->saAmfSUOperState]);
		fprintf(f, "    saAmfSUAdminState: %s\n",
				avd_adm_state_name[su->saAmfSUAdminState]);
		fprintf(f, "    saAmfSuReadinessState: %s\n",
				avd_readiness_state_name[su->saAmfSuReadinessState]);
		fprintf(f, "    saAmfSUPresenceState: %s\n",
				avd_pres_state_name[su->saAmfSUPresenceState]);
		fprintf(f, "    saAmfSUHostedByNode: %s\n", su->saAmfSUHostedByNode.c_str());
		fprintf(f, "    saAmfSUNumCurrActiveSIs: %u\n", su->saAmfSUNumCurrActiveSIs);
		fprintf(f, "    saAmfSUNumCurrStandbySIs: %u\n", su->saAmfSUNumCurrStandbySIs);
		fprintf(f, "    saAmfSURestartCount: %u\n", su->saAmfSURestartCount);
		fprintf(f, "    term_state: %u\n", su->term_state);
		fprintf(f, "    su_switch: %u\n", su->su_switch);
		fprintf(f, "    assigned_SIs:\n");
		for (susi = su->list_of_susi; susi != nullptr; susi = susi->su_next) {
			fprintf(f, "      dn: %s\n", susi->si->name.c_str());
			fprintf(f, "      hastate: %s\n", avd_ha_state[susi->state]);
			fprintf(f, "      fsm: %u\n", susi->fsm);
		}
	}

	fprintf(f, "components:\n");
	for (std::map<std::string, AVD_COMP*>::const_iterator it = comp_db->begin();
			it != comp_db->end(); it++) {
		const AVD_COMP *comp  = it->second;
		fprintf(f, "  dn: %s\n", osaf_extended_name_borrow(&comp->comp_info.name));
		fprintf(f, "    saAmfCompOperState: %s\n",
				avd_oper_state_name[comp->saAmfCompOperState]);
		fprintf(f, "    saAmfCompReadinessState: %s\n",
				avd_readiness_state_name[comp->saAmfCompReadinessState]);
		fprintf(f, "    saAmfCompPresenceState: %s\n",
				avd_pres_state_name[comp->saAmfCompPresenceState]);
		fprintf(f, "    saAmfCompRestartCount: %u\n", comp->saAmfCompRestartCount);
		if (comp->saAmfCompCurrProxyName.empty() == false)
			fprintf(f, "    saAmfCompCurrProxyName: %s\n", comp->saAmfCompCurrProxyName.c_str());
	}

        fprintf(f, "COMPCS_TYPE:\n");
        for (std::map<std::string, AVD_COMPCS_TYPE*>::const_iterator it = compcstype_db->begin();
                        it != compcstype_db->end(); it++) {
                const AVD_COMPCS_TYPE *compcs_type  = it->second;
                fprintf(f, "  dn: %s\n", compcs_type->name.c_str());
                fprintf(f, "    saAmfCompNumMaxActiveCSIs: %u\n",
                                compcs_type->saAmfCompNumMaxActiveCSIs);
                fprintf(f, "    saAmfCompNumMaxStandbyCSIs: %u\n",
                                compcs_type->saAmfCompNumMaxStandbyCSIs);
                fprintf(f, "    saAmfCompNumCurrActiveCSIs: %u\n",
                                compcs_type->saAmfCompNumCurrActiveCSIs);
                fprintf(f, "    saAmfCompNumCurrStandbyCSIs: %u\n",
                                compcs_type->saAmfCompNumCurrStandbyCSIs);
        }


	fprintf(f, "Node Groups:\n");
	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
                        it != nodegroup_db->end(); it++) {
		AVD_AMF_NG *ng = it->second;
		fprintf(f, "  dn: %s\n", ng->name.c_str());
		fprintf(f, "    saAmfNGAdminState: %s\n",avd_adm_state_name[ng->saAmfNGAdminState]);
	}

	fclose(f);
	return 0;
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
int avd_admin_op_msg_snd(const std::string& dn, AVSV_AMF_CLASS_ID class_id,
	SaAmfAdminOperationIdT opId, AVD_AVND *node)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;
	AVD_DND_MSG *d2n_msg;
	unsigned int rc = NCSCC_RC_SUCCESS;

	SaNameT temp_dn;
	osaf_extended_name_alloc(dn.c_str(), &temp_dn);

	TRACE_ENTER2(" '%s' %u", dn.c_str(), opId);

	d2n_msg = new AVSV_DND_MSG();

	d2n_msg->msg_type = AVSV_D2N_ADMIN_OP_REQ_MSG;
	d2n_msg->msg_info.d2n_admin_op_req_info.msg_id = ++(node->snd_msg_id);
	d2n_msg->msg_info.d2n_admin_op_req_info.dn = temp_dn;
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
std::string avd_getparent(const std::string& dn)
{
	std::string::size_type start_pos;
	std::string::size_type parent_pos;

	/* Check if there exist any escaped RDN in the DN */
	start_pos = dn.find_last_of('\\');
	if (start_pos == std::string::npos) {
		start_pos = 0;
	} else {
		++start_pos;
		if (dn[start_pos] == ',') {
			++start_pos;
		}
	}
	
	parent_pos = dn.find(',', start_pos);
	return dn.substr(parent_pos + 1);
}

/**
 * Verify the existence of an object in IMM
 * @param dn
 * 
 * @return bool
 */
bool object_exist_in_imm(const std::string& dn)
{
	bool rc = false;
	const SaNameTWrapper temp_dn(dn);
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAttrNameT attributeNames[] = {const_cast<SaImmAttrNameT>("SaImmAttrClassName"), nullptr};

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);

	if (immutil_saImmOmAccessorGet_2(accessorHandle, temp_dn, attributeNames,
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

	while (su_msg->msg_info.d2n_reg_su.su_list != nullptr) {
		su_info = su_msg->msg_info.d2n_reg_su.su_list;
		su_msg->msg_info.d2n_reg_su.su_list = su_info->next;
		osaf_extended_name_free(&su_info->name);
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
	TRACE_ENTER();
	AVSV_SUSI_ASGN *compcsi_info;

	osaf_extended_name_free(&susi_msg->msg_info.d2n_su_si_assign.si_name);
	osaf_extended_name_free(&susi_msg->msg_info.d2n_su_si_assign.su_name);
	
	while (susi_msg->msg_info.d2n_su_si_assign.list != nullptr) {
		compcsi_info = susi_msg->msg_info.d2n_su_si_assign.list;
		susi_msg->msg_info.d2n_su_si_assign.list = compcsi_info->next;
		if (compcsi_info->attrs.list != nullptr) {
                        for (uint16_t i = 0; i < compcsi_info->attrs.number; i++) {
                                osaf_extended_name_free(&compcsi_info->attrs.list[i].name);
                                osaf_extended_name_free(&compcsi_info->attrs.list[i].value);
                                delete [] compcsi_info->attrs.list[i].string_ptr;
                                compcsi_info->attrs.list[i].string_ptr = nullptr;
                        }
			delete [] (compcsi_info->attrs.list);
			compcsi_info->attrs.list = nullptr;
		}
		osaf_extended_name_free(&compcsi_info->active_comp_name);
		osaf_extended_name_free(&compcsi_info->comp_name);
		osaf_extended_name_free(&compcsi_info->csi_name);
		delete compcsi_info;
	}
	TRACE_LEAVE();
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

	if (info->mem_list.numberOfItems) {
		osaf_extended_name_free(&info->mem_list.notification->member.compName);
		delete [] info->mem_list.notification;
	}

	info->mem_list.notification = 0;
	info->mem_list.numberOfItems = 0;
}

static void free_d2n_compcsi_info(AVSV_DND_MSG *compcsi_msg) {
  AVSV_D2N_COMPCSI_ASSIGN_MSG_INFO *compcsi = &compcsi_msg->msg_info.d2n_compcsi_assign_msg_info;

  osaf_extended_name_free(&compcsi->comp_name);
  osaf_extended_name_free(&compcsi->csi_name);

  if (compcsi->info.attrs.list != nullptr) {
    for (uint16_t i = 0; i < compcsi->info.attrs.number; i++) {
      osaf_extended_name_free(&compcsi->info.attrs.list[i].name);
      osaf_extended_name_free(&compcsi->info.attrs.list[i].value);
      delete [] compcsi->info.attrs.list[i].string_ptr;
      compcsi->info.attrs.list[i].string_ptr = nullptr;
    }
    delete [] (compcsi->info.attrs.list);
    compcsi->info.attrs.list = nullptr;
  }
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
	if (msg == nullptr)
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
		osaf_extended_name_free(&msg->msg_info.d2n_pg_track_act_rsp.csi_name);
		break;
	case AVSV_D2N_PG_UPD_MSG:
		osaf_extended_name_free(&msg->msg_info.d2n_pg_upd.csi_name);
		osaf_extended_name_free(&msg->msg_info.d2n_pg_upd.mem.member.compName);
		break;
	case AVSV_D2N_OPERATION_REQUEST_MSG:
		osaf_extended_name_free(&msg->msg_info.d2n_op_req.param_info.name);
		osaf_extended_name_free(&msg->msg_info.d2n_op_req.param_info.name_sec);
		break;
	case AVSV_D2N_ADMIN_OP_REQ_MSG:
		osaf_extended_name_free(&msg->msg_info.d2n_admin_op_req_info.dn);
		break;
	case AVSV_D2N_PRESENCE_SU_MSG:
		osaf_extended_name_free(&msg->msg_info.d2n_prsc_su.su_name);
		break;
	case AVSV_D2N_COMPCSI_ASSIGN_MSG:
		free_d2n_compcsi_info(msg);
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

/**
 * Validates whether the amfd state is apt to process the admin command.
 * @param: admin operation id.
 * @param: class id related to objects on which admin command has been issued.
 */
bool admin_op_is_valid(SaImmAdminOperationIdT opId, AVSV_AMF_CLASS_ID class_id)
{
	bool valid = false;
	TRACE_ENTER2("%llu, %u", opId, class_id);

	if (avd_cb->init_state == AVD_APP_STATE) {
		TRACE_LEAVE();
		return true;
	}

	switch (class_id) {
		case AVSV_SA_AMF_SU:
			/* Support for admin op on su before cluster timer expires.*/
			if ((avd_cb->init_state == AVD_INIT_DONE) &&
					((opId == SA_AMF_ADMIN_LOCK) || (opId == SA_AMF_ADMIN_UNLOCK) ||
					 (opId == SA_AMF_ADMIN_REPAIRED) ||
					 (opId == SA_AMF_ADMIN_UNLOCK_INSTANTIATION)))
				valid = true;
			break;
		case AVSV_SA_AMF_NODE:
		case AVSV_SA_AMF_SG:
			/* Support for admin op on node/sg before cluster timer expires.*/
			if ((avd_cb->init_state == AVD_INIT_DONE) &&
					((opId == SA_AMF_ADMIN_LOCK) || (opId == SA_AMF_ADMIN_UNLOCK) ||
					 (opId == SA_AMF_ADMIN_UNLOCK_INSTANTIATION)))
				valid = true;
			break;

		case AVSV_SA_AMF_SI:
			/* Support for admin op on si before cluster timer expires. */
			if ((avd_cb->init_state == AVD_INIT_DONE) &&
					((opId == SA_AMF_ADMIN_LOCK) || (opId == SA_AMF_ADMIN_UNLOCK)))
				valid = true;
			break;
		case AVSV_SA_AMF_COMP:
		default:
			break;
	}

	TRACE_LEAVE();
	return valid;
}

 
 /**
  * Gets child DN from an association DN
  *   "safDepend=safSi=SC2-NoRed\\,safApp=OpenSAF,safSi=SC-2N,safApp=OpenSAF"
  *   child DN is "safSi=SC2-NoRed,safApp=OpenSAF"
  * @param ass_dn association DN [in]
  * @param child_dn [out]
  * @return 0 at success
  */
 int get_child_dn_from_ass_dn(const std::string& ass_dn, std::string& child_dn)
 {
	std::string::size_type comma_pos;
	std::string::size_type equal_pos;

 	/* find first comma and step past it */
	comma_pos = ass_dn.find(',');
 	if (comma_pos == std::string::npos)
 		return -1;

 	/* find second comma, an error if not found */
	comma_pos = ass_dn.find(',', comma_pos + 1);
 	if (comma_pos == std::string::npos)
 		return -1;
 
 	/* Skip past the RDN tag */
	equal_pos = ass_dn.find('=');
 	if (equal_pos == std::string::npos)
 		return -1;
 
 	/* copy and skip back slash */
	child_dn = ass_dn.substr(equal_pos + 1, comma_pos - equal_pos - 1);
	child_dn.erase(std::remove(child_dn.begin(), child_dn.end(), '\\'), child_dn.end());

	return 0;
 }

int get_parent_dn_from_ass_dn(const std::string& ass_dn, std::string& parent_dn)
{
	std::string::size_type comma_pos;

 	/* find first comma and step past it */
	comma_pos = ass_dn.find(',');
 	if (comma_pos == std::string::npos)
 		return -1;

 	/* find second comma and step past it */
	comma_pos = ass_dn.find(',', comma_pos + 1);
 	if (comma_pos == std::string::npos)
 		return -1;
	
	parent_dn = ass_dn.substr(comma_pos + 1);

	return 0;
}

/**
 * Initialize a DN by searching for needle in haystack
 * @param haystack
 * @param dn
 * @param needle
 */
void avsv_sanamet_init(const std::string& haystack, std::string& dn, const char *needle)
{
	TRACE_ENTER();

	std::string::size_type pos = haystack.find(needle);
	dn = haystack.substr(pos);
	TRACE("dn %s", dn.c_str());
	
	TRACE_LEAVE();
}

// this function emulates the behaviour of m_CMP_NORDER_SANAMET
int compare_sanamet(const std::string& lhs, const std::string& rhs)
{
	if (lhs.length() > rhs.length())
		return 1;
	else if (lhs.length() < rhs.length())
		return -1;
	else
		return lhs.compare(rhs);
}

/**
 * @brief    Sends a message to AMFND for a COMPCSI.
 *           As of now sends modified list of CSI's attributes to
 *           the AMFND which host assigned component(compcsi).
 * @param    ptr to comp 
 * @param    ptr to csi 
 * @param    ptr to compcsi 
 * @param    act(action of type AVSV_COMPCSI_ACT) 
 * @return   NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
 */
uint32_t avd_snd_compcsi_msg(AVD_COMP *comp, AVD_CSI *csi, AVD_COMP_CSI_REL *compcsi, AVSV_COMPCSI_ACT act) {
  AVD_DND_MSG *compcsi_msg = nullptr;
  AVSV_CSI_ATTRS *ptr_csiattr_msg = nullptr;
  AVSV_ATTR_NAME_VAL *i_ptr_msg = nullptr;
  AVD_CSI_ATTR *attr_ptr_db = nullptr;
  AVD_AVND *avnd = nullptr;

  TRACE_ENTER2("'%s', '%s', act:%u",
    osaf_extended_name_borrow(&compcsi->comp->comp_info.name),
    csi->name.c_str(), act);

  //Depending upon the message sub type retrieve node from eligible entity.
  if (act == AVSV_COMPCSI_ATTR_CHANGE_AND_NO_ACK) {
    avnd = comp->su->get_node_ptr();
  }
  if ((avnd->node_state == AVD_AVND_STATE_ABSENT) ||
    (avnd->node_state == AVD_AVND_STATE_GO_DOWN)) {
    TRACE_LEAVE();
    return NCSCC_RC_SUCCESS;
  }

  //Will be freed in free_d2n_compcsi_info().
  SaNameT comp_name;
  osaf_extended_name_alloc(osaf_extended_name_borrow(&compcsi->comp->comp_info.name),
    &comp_name);
  //Will be freed in free_d2n_compcsi_info().
  SaNameT csi_name;
  osaf_extended_name_alloc(csi->name.c_str(), &csi_name);

  /* prepare the COMP CSI message. */
  compcsi_msg = new AVSV_DND_MSG();
  compcsi_msg->msg_type = AVSV_D2N_COMPCSI_ASSIGN_MSG;
  compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.node_id = avnd->node_info.nodeId;
  compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.msg_act = act;
  compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.comp_name = comp_name;
  compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.csi_name = csi_name;
  switch (act) {
    case AVSV_COMPCSI_ATTR_CHANGE_AND_NO_ACK: {
      ptr_csiattr_msg = &compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.info.attrs;
      ptr_csiattr_msg->list = new AVSV_ATTR_NAME_VAL[compcsi->csi->num_attributes];
      /* initilize both the message pointer and the database pointer. Also init the
       * message content.
       */
      i_ptr_msg = ptr_csiattr_msg->list;
      attr_ptr_db = compcsi->csi->list_attributes;
      ptr_csiattr_msg->number = 0;

      /* Scan the list of attributes for the CSI and add it to the message */
      while ((attr_ptr_db != nullptr) &&
        (ptr_csiattr_msg->number < compcsi->csi->num_attributes)) {
	osaf_extended_name_alloc(osaf_extended_name_borrow(&attr_ptr_db->name_value.name),
          &i_ptr_msg->name);
	osaf_extended_name_alloc(osaf_extended_name_borrow(&attr_ptr_db->name_value.value),
          &i_ptr_msg->value);
	if (attr_ptr_db->name_value.string_ptr != nullptr) {
	  i_ptr_msg->string_ptr = new char[strlen(attr_ptr_db->name_value.string_ptr)+1];
	  memcpy(i_ptr_msg->string_ptr, attr_ptr_db->name_value.string_ptr,
	    strlen(attr_ptr_db->name_value.string_ptr)+1);
	} else {
	    i_ptr_msg->string_ptr = nullptr;
	}
	ptr_csiattr_msg->number++;
	i_ptr_msg = i_ptr_msg + 1;
	attr_ptr_db = attr_ptr_db->attr_next;
      }
      break;
    }
    default: {
      d2n_msg_free(compcsi_msg);
      TRACE_LEAVE();
      return NCSCC_RC_FAILURE;
      break;
    }
  }

  //Generate new msg_id.
  compcsi_msg->msg_info.d2n_compcsi_assign_msg_info.msg_id = ++(avnd->snd_msg_id);

  //Send COMP CSI message*/
  TRACE("Sending %u to %x", AVSV_D2N_COMPCSI_ASSIGN_MSG, avnd->node_info.nodeId);
  if (avd_d2n_msg_snd(avd_cb, avnd, compcsi_msg) != NCSCC_RC_SUCCESS) {
    LOG_ER("Send to %x failed",avnd->node_info.nodeId);
    --(avnd->snd_msg_id);
    d2n_msg_free(compcsi_msg);
    TRACE_LEAVE();
    return NCSCC_RC_FAILURE;
  }
  //Checkpoint to standby AMFD.
  m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, avnd, AVSV_CKPT_AVND_SND_MSG_ID);

  TRACE_LEAVE();
  return NCSCC_RC_SUCCESS;
}

void avd_association_namet_init(const std::string& associate_dn, std::string& child,
		std::string& parent, AVSV_AMF_CLASS_ID parent_class_id) {
  std::string::size_type pos;
  std::string::size_type equal_pos;

  //Example dns
  //dn:safCSIComp=safComp=AmfDemo\,safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safCsi=AmfDemo,safSi=AmfDemo,safApp=AmfDemo1
  // child: safComp=AmfDemo\,safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1 
  //parent: safCsi=AmfDemo,safSi=AmfDemo,safApp=AmfDemo1
  //dn:safSISU=safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safSi=AmfDemo,safApp=AmfDemo1
  if (parent_class_id  == AVSV_SA_AMF_SI) {
    pos = associate_dn.find("safSi=");
  } else if (parent_class_id  == AVSV_SA_AMF_CSI) {
    pos = associate_dn.find("safCsi=");
  } else {
    parent = "";
    child  = "";
    return;
  }
  // set parent name 
  parent = associate_dn.substr(pos);

  // set child name 
  equal_pos = associate_dn.find('=');
  child = associate_dn.substr(equal_pos + 1, pos - equal_pos - 2);
  child.erase(std::remove(child.begin(), child.end(), '\\'), child.end());
}

