/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#include "usrbuf.h"
#include "dtm.h"
#include "dtm_cb.h"
#include "dtm_inter.h"
#include "dtm_inter_disc.h"
#include "dtm_inter_trans.h"

DTM_SVC_DISTRIBUTION_LIST *dtm_svc_dist_list = NULL;

static uint32_t dtm_prepare_and_send_svc_up_msg_for_node_up(NODE_ID node_id);


uint32_t dtm_prepare_svc_up_msg(uint8_t *buffer, uint32_t server_type, uint32_t server_inst, uint32_t pid);
uint32_t dtm_prepare_svc_down_msg(uint8_t *buffer, uint32_t server_type, uint32_t server_inst, uint32_t pid);


/**
 * Function to process rcv up message
 *
 * @param buffer len node_id
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_process_rcv_up_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	/* Post the event to the mailbox of the intra_thread */
	DTM_RCV_MSG_ELEM *dtm_msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (dtm_msg_elem = calloc(1, sizeof(DTM_RCV_MSG_ELEM)))) {
		TRACE("DTM : SVC UP EVENT : Calloc FAILED");
		return NCSCC_RC_FAILURE;
	}
	dtm_msg_elem->type = DTM_MBX_UP_TYPE;
	dtm_msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	dtm_msg_elem->info.svc_event.len = len;
	dtm_msg_elem->info.svc_event.node_id = node_id;
	dtm_msg_elem->info.svc_event.buffer = buffer;
	if ((m_NCS_IPC_SEND(&dtm_intranode_cb->mbx, dtm_msg_elem, dtm_msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(dtm_msg_elem);
		TRACE("DTM :Intranode IPC_SEND : SVC UP EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM :Intranode IPC_SEND : SVC UP EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to process rcv down message
 *
 * @param buffer len node_id
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_process_rcv_down_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	/* Post the event to the mailbox of the intra_thread */
	DTM_RCV_MSG_ELEM *dtm_msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (dtm_msg_elem = calloc(1, sizeof(DTM_RCV_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	dtm_msg_elem->type = DTM_MBX_DOWN_TYPE;
	dtm_msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	dtm_msg_elem->info.svc_event.len = len;
	dtm_msg_elem->info.svc_event.node_id = node_id;
	dtm_msg_elem->info.svc_event.buffer = buffer;
	if ((m_NCS_IPC_SEND(&dtm_intranode_cb->mbx, dtm_msg_elem, dtm_msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(dtm_msg_elem);
		TRACE("DTM : Intranode IPC_SEND : SVC DOWN EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Intranode IPC_SEND : SVC DOWN EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to process node up
 *
 * @param node_id node_name mbx
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_node_up(NODE_ID node_id, char *node_name, SYSF_MBX mbx)
{
	/* Function call from inter thread */
	/* Post the event to the mailbox of the intra_thread */
	DTM_RCV_MSG_ELEM *dtm_msg_elem = NULL;
	uint32_t status = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	LOG_NO("Established contact with '%s'", node_name);
	if (NULL == (dtm_msg_elem = calloc(1, sizeof(DTM_RCV_MSG_ELEM)))) {
		TRACE("DTM :Calloc failed for DTM_RCV_MSG_ELEM, dtm_node_up");
		return status;
	}
	dtm_msg_elem->type = DTM_MBX_NODE_UP_TYPE;
	dtm_msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	dtm_msg_elem->info.node.node_id = node_id;
	dtm_msg_elem->info.node.mbx = mbx;
	strcpy(dtm_msg_elem->info.node.node_name, node_name);

	/* Do a mailbox post */
	if ((m_NCS_IPC_SEND(&dtm_intranode_cb->mbx, dtm_msg_elem, dtm_msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(dtm_msg_elem);
		TRACE("DTM : Intranode IPC_SEND : NODE UP EVENT : FAILED");
		status = NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Intranode IPC_SEND : NODE UP EVENT : SUCC");
		status = NCSCC_RC_SUCCESS;
	}

	/* Now send the distribution list */
	dtm_prepare_and_send_svc_up_msg_for_node_up(node_id);
	TRACE_LEAVE();
	return status;
}

/**
 * Function to process node down
 *
 * @param node_id node_name mbx
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_node_down(NODE_ID node_id, char *node_name, SYSF_MBX mbx)
{
	/* Function call from inter thread */
	/* Do a mailbox post */
	DTM_RCV_MSG_ELEM *dtm_msg_elem = NULL;
	TRACE_ENTER();
	LOG_NO("Lost contact with '%s'", node_name);
	if (NULL == (dtm_msg_elem = calloc(1, sizeof(DTM_RCV_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	dtm_msg_elem->type = DTM_MBX_NODE_DOWN_TYPE;
	dtm_msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	dtm_msg_elem->info.node.node_id = node_id;
	dtm_msg_elem->info.node.mbx = mbx;
	strcpy(dtm_msg_elem->info.node.node_name, node_name);
	if ((m_NCS_IPC_SEND(&dtm_intranode_cb->mbx, dtm_msg_elem, dtm_msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(dtm_msg_elem);
		TRACE("DTM : Intranode IPC_SEND : NODE DOWN EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Intranode IPC_SEND : NODE DOWN EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to add svc into the svclist
 *
 * @param server_type server_inst pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_add_to_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	/* Post the event to the mailbox of the inter_thread */
	DTM_SND_MSG_ELEM *msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (msg_elem = calloc(1, sizeof(DTM_SND_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	msg_elem->type = DTM_MBX_ADD_DISTR_TYPE;
	msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	msg_elem->info.svc_event.server_type = server_type;
	msg_elem->info.svc_event.server_inst = server_inst;
	msg_elem->info.svc_event.pid = pid;
	if ((m_NCS_IPC_SEND(&dtms_gl_cb->mbx, msg_elem, msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(msg_elem);
		TRACE("DTM : Internode IPC_SEND : SVC UP EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Internode IPC_SEND : SVC UP EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to del from svc into the svclist
 *
 * @param server_type server_inst pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_del_from_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	/* Post the event to the mailbox of the inter_thread */
	DTM_SND_MSG_ELEM *msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (msg_elem = calloc(1, sizeof(DTM_SND_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	msg_elem->type = DTM_MBX_DEL_DISTR_TYPE;
	msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	msg_elem->info.svc_event.server_type = server_type;
	msg_elem->info.svc_event.server_inst = server_inst;
	msg_elem->info.svc_event.pid = pid;
	if ((m_NCS_IPC_SEND(&dtms_gl_cb->mbx, msg_elem, msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(msg_elem);
		TRACE("DTM : Internode IPC_SEND : SVC UP EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Internode IPC_SEND : SVC UP EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to init the svclist
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_internode_init_svc_dist_list(void)
{
	if (NULL == (dtm_svc_dist_list = calloc(1, sizeof(DTM_SVC_DISTRIBUTION_LIST)))) {
		LOG_ER("calloc failure for dtm_svc_dist_list");
		assert(0);
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to add to svc dist list
 *
 * @param server_type server_inst pid 
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_add_to_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	uint8_t *buffer = NULL;
	DTM_SVC_DATA *add_ptr = NULL, *hdr = NULL, *tail = NULL;

	TRACE_ENTER();
	if (NULL != dtm_svc_dist_list) {
		hdr = dtm_svc_dist_list->data_ptr_hdr;
		tail = dtm_svc_dist_list->data_ptr_tail;
	} else {
		dtm_internode_init_svc_dist_list();
	}

	if (NULL == (add_ptr = calloc(1, sizeof(DTM_SVC_DATA)))) {
		TRACE("DTM :CALLOC FAILED FOR DTM_SVC_DATA");
		return NCSCC_RC_FAILURE;
	}
	if (NULL == (buffer = calloc(1, DTM_UP_MSG_SIZE_FULL))) {
		TRACE("DTM :CALLOC FAILED FOR buffer");
		free(add_ptr);
		return NCSCC_RC_FAILURE;
	}
	add_ptr->type = server_type;
	add_ptr->inst = server_inst;
	add_ptr->pid = pid;
	add_ptr->next = NULL;

	if (NULL == hdr) {
		dtm_svc_dist_list->data_ptr_hdr = add_ptr;
		dtm_svc_dist_list->data_ptr_tail = add_ptr;
	} else {
		tail->next = add_ptr;
		dtm_svc_dist_list->data_ptr_tail = add_ptr;
	}
	dtm_svc_dist_list->num_elem++;

	/* Now send this info to all the connected nodes */
	dtm_prepare_svc_up_msg(buffer, server_type, server_inst, pid);
	dtm_internode_snd_msg_to_all_nodes(buffer, (uint16_t)DTM_UP_MSG_SIZE_FULL);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to del from svc dist list
 *
 * @param server_type server_inst pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_del_from_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	uint8_t *buffer = NULL;
	DTM_SVC_DATA *back = NULL, *mov_ptr = NULL;
	int check_val = 0;
	TRACE_ENTER();
	if (NULL == dtm_svc_dist_list) {
		TRACE("DTM :NULL svc dist list ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	mov_ptr = dtm_svc_dist_list->data_ptr_hdr;

	if (NULL == mov_ptr) {
		TRACE("DTM :NULL svc dist list ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	for (back = NULL, mov_ptr = dtm_svc_dist_list->data_ptr_hdr; mov_ptr != NULL;
	     back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((server_type == mov_ptr->type) && (server_inst == mov_ptr->inst) && (pid == mov_ptr->pid)) {
			if (back == NULL) {
				dtm_svc_dist_list->data_ptr_hdr = mov_ptr->next;
				if (NULL == mov_ptr->next) {
					dtm_svc_dist_list->data_ptr_tail = mov_ptr->next;
				}
			} else {
				if (NULL == mov_ptr->next) {
					dtm_svc_dist_list->data_ptr_tail = back;
				}
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			dtm_svc_dist_list->num_elem--;
			TRACE("DTM :Successfully deleted the node from svc dist list ");
			check_val = 1;
			break;
		}
	}

	if (1 == check_val) {
		if (NULL == (buffer = calloc(1, DTM_DOWN_MSG_SIZE_FULL))) {
			TRACE("DTM :CALLOC FAILED FOR buffer, unable to send down to all nodes");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		/* Now send this info to all the connected nodes */
		dtm_prepare_svc_down_msg(buffer, server_type, server_inst, pid);
		dtm_internode_snd_msg_to_all_nodes(buffer, (uint16_t)DTM_DOWN_MSG_SIZE_FULL);
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	} else {
		/* dont send to any node */
		TRACE("DTM :No matching entry found in svc dist list for deletion");
		/* assert */
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

}

/**
 * Function to prepare svc up message
 *
 * @param buffer server_type server_inst pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_prepare_svc_up_msg(uint8_t *buffer, uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_UP_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTERNODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTERNODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_UP_MSG_TYPE);
	ncs_encode_16bit(&data, (uint16_t)0x01);
	ncs_encode_32bit(&data, server_type);
	ncs_encode_32bit(&data, server_inst);
	ncs_encode_32bit(&data, pid);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to prepare svc down message
 *
 * @param buffer server_type server_inst pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_prepare_svc_down_msg(uint8_t *buffer, uint32_t server_type, uint32_t server_inst, uint32_t pid)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_DOWN_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTERNODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTERNODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_DOWN_MSG_TYPE);
	ncs_encode_16bit(&data, (uint16_t)0x01);
	ncs_encode_32bit(&data, server_type);
	ncs_encode_32bit(&data, server_inst);
	ncs_encode_32bit(&data, pid);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to prepare svc up msg and send it for node up
 *
 * @param node_id
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_prepare_and_send_svc_up_msg_for_node_up(NODE_ID node_id)
{
	uint16_t num_elem = 0, buff_len = 0;
	uint8_t *data = NULL;
	DTM_SVC_DATA *mov_ptr = NULL;

	TRACE_ENTER();
	if (NULL == dtm_svc_dist_list) {
		TRACE("DTM :No services to be updated to remote node");
		return NCSCC_RC_SUCCESS;
	}
	num_elem = dtm_svc_dist_list->num_elem;
	mov_ptr = dtm_svc_dist_list->data_ptr_hdr;

	if (mov_ptr == NULL || (0 == dtm_svc_dist_list->num_elem)) {
		TRACE("DTM :No services to be updated to remote node");
		return NCSCC_RC_SUCCESS;
	}
	buff_len = (uint16_t)(DTM_UP_MSG_SIZE_FULL + ((num_elem - 1) * 12));

	if (NULL == mov_ptr) {
		TRACE("DTM :No services to be updated to remote node");
		return NCSCC_RC_SUCCESS;
	} else {
		uint8_t *buffer = NULL;
		if (NULL == (buffer = calloc(1, buff_len))) {
			TRACE("DTM :CALLOC FAILED FOR buffer, unable to send down to all nodes");
			return NCSCC_RC_FAILURE;
		}
		data = buffer;
		ncs_encode_16bit(&data, (uint16_t)(buff_len - 2));
		ncs_encode_32bit(&data, (uint32_t)DTM_INTERNODE_SND_MSG_IDENTIFIER);
		ncs_encode_8bit(&data, (uint8_t)DTM_INTERNODE_SND_MSG_VER);
		ncs_encode_8bit(&data, (uint8_t)DTM_UP_MSG_TYPE);
		ncs_encode_16bit(&data, num_elem);
		while (NULL != mov_ptr) {
			ncs_encode_32bit(&data, mov_ptr->type);
			ncs_encode_32bit(&data, mov_ptr->inst);
			ncs_encode_32bit(&data, mov_ptr->pid);
			mov_ptr = mov_ptr->next;
		}
		dtm_internode_snd_msg_to_node(buffer, buff_len, node_id);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
