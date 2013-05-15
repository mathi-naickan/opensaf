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

#include "dtm.h"
#include "dtm_cb.h"
#include "dtm_inter.h"
#include "dtm_node.h"

uint32_t dtm_internode_snd_msg_to_all_nodes(uint8_t *buffer, uint16_t len);

uint32_t dtm_internode_snd_msg_to_node(uint8_t *buffer, uint16_t len, NODE_ID node_id);

uint32_t dtm_internode_process_pollout(int fd);
uint32_t dtm_prepare_data_msg(uint8_t *buffer, uint16_t len);
static uint32_t dtm_internode_snd_unsent_msg(DTM_NODE_DB * node);
static uint32_t dtm_internode_snd_msg_common(DTM_NODE_DB * node, uint8_t *buffer, uint16_t len);

/**
 * Function to process rcv data message internode
 *
 * @param buffer dst_pid len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_process_rcv_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len)
{
	/* Post the event to the mailbox of the intra_thread */
	DTM_RCV_MSG_ELEM *dtm_msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (dtm_msg_elem = calloc(1, sizeof(DTM_RCV_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	dtm_msg_elem->type = DTM_MBX_MSG_TYPE;
	dtm_msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	dtm_msg_elem->info.data.len = len;
	dtm_msg_elem->info.data.dst_pid = dst_pid;
	dtm_msg_elem->info.data.buffer = buffer;
	if ((m_NCS_IPC_SEND(&dtm_intranode_cb->mbx, dtm_msg_elem, dtm_msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(dtm_msg_elem);
		TRACE("DTM : Intranode IPC_SEND : DATA MSG: FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Intranode IPC_SEND : DATA MSG: SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to add message to msg dist list
 *
 * @param buffer dst_pid len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_add_to_msg_dist_list(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	/* Post the event to the mailbox of the inter_thread */
	DTM_SND_MSG_ELEM *msg_elem = NULL;
	TRACE_ENTER();
	if (NULL == (msg_elem = calloc(1, sizeof(DTM_SND_MSG_ELEM)))) {
		return NCSCC_RC_FAILURE;
	}
	msg_elem->type = DTM_MBX_DATA_MSG_TYPE;
	msg_elem->pri = NCS_IPC_PRIORITY_HIGH;
	msg_elem->info.data.buffer = buffer;
	msg_elem->info.data.dst_nodeid = node_id;
	msg_elem->info.data.buff_len = len;
	if ((m_NCS_IPC_SEND(&dtms_gl_cb->mbx, msg_elem, msg_elem->pri)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed */
		free(msg_elem);
		TRACE("DTM : Internode IPC_SEND : MSG EVENT : FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	} else {
		TRACE("DTM : Internode IPC_SEND : MSG EVENT : SUCC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to prepare the data message
 *
 * @param buffer len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_prepare_data_msg(uint8_t *buffer, uint16_t len)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (len - 2));
	ncs_encode_32bit(&data, (uint32_t)DTM_INTERNODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTERNODE_SND_MSG_VER);
	ncs_encode_8bit(&data, DTM_MESSAGE_MSG_TYPE);
	/* Remaining data already in buffer */
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to send message to all nodes
 *
 * @param buffer len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_snd_msg_to_all_nodes(uint8_t *buffer, uint16_t len)
{
	/* Get each node and send message */
	NODE_ID node_id = 0;
	DTM_NODE_DB *node = NULL;
	TRACE_ENTER();

	while (NULL != (node = dtm_node_getnext_by_id(node_id))) {
		node_id = node->node_id;
		/* send only to fully connected peers */
		if ((node_id != dtms_gl_cb->node_id) &&	node->comm_status) {
			uint8_t *buf_send = NULL;
			if (NULL == (buf_send = calloc(1, len))) {
				TRACE("DTM :calloc failed for snd_msg_to_all_nodes");
				free(buffer);
				return NCSCC_RC_FAILURE;
			}
			memcpy(buf_send, buffer, len);	
			dtm_internode_snd_msg_common(node, buf_send, len);
		}
	}
	free(buffer);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to send message
 *
 * @param node buffer len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_internode_snd_msg_common(DTM_NODE_DB * node, uint8_t *buffer, uint16_t len)
{
	DTM_INTERNODE_UNSENT_MSGS *add_ptr = NULL, *hdr = node->msgs_hdr, *tail = node->msgs_tail;
	TRACE_ENTER();
	if (NULL == hdr) {
		/* Send the message */
		int send_len = 0;
		send_len = send(node->comm_socket, buffer, len, MSG_NOSIGNAL);

		if (send_len == len) {
			free(buffer);
			return NCSCC_RC_SUCCESS;
		} else {
			TRACE("DTM: nsend failed, total_len : %d, send_len : %d", len, send_len);
			/* Queue the message */
			if (NULL == (add_ptr = calloc(1, sizeof(DTM_INTERNODE_UNSENT_MSGS)))) {
				TRACE("DTM :Calloc failed DTM_INTERNODE_UNSENT_MSGS");
				return NCSCC_RC_FAILURE;
			}
			add_ptr->next = NULL;
			add_ptr->buffer = buffer;
			add_ptr->len = len;
			node->msgs_hdr = add_ptr;
			node->msgs_tail = add_ptr;
			dtm_internode_set_poll_fdlist(node->comm_socket, POLLOUT);
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Queue the message */
		if (NULL == (add_ptr = calloc(1, sizeof(DTM_INTERNODE_UNSENT_MSGS)))) {
			TRACE("DTM :Calloc failed DTM_INTERNODE_UNSENT_MSGS");
			return NCSCC_RC_FAILURE;
		} else {
			add_ptr->next = NULL;
			add_ptr->buffer = buffer;
			add_ptr->len = len;
			tail->next = add_ptr;
			node->msgs_tail = add_ptr;
			dtm_internode_set_poll_fdlist(node->comm_socket, POLLOUT);
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Fucntion to send internode message
 *
 * @param node_id buffer len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_snd_msg_to_node(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	DTM_NODE_DB *node = NULL;

	TRACE_ENTER();
	node = dtm_node_get_by_id(node_id);

	if (NULL != node) {
		if (NCSCC_RC_SUCCESS != dtm_internode_snd_msg_common(node, buffer, len)) {
			free(buffer);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
	} else {
		free(buffer);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/* Get the node entry and send message */
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to process pollout
 *
 * @param fd
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_process_pollout(int fd)
{
	DTM_NODE_DB *node = NULL;

	TRACE_ENTER();
	node = dtm_node_get_by_comm_socket((uint32_t)fd);
	if (NULL == node) {
		TRACE("DTM :No node matching the fd for pollout, delete this fd from fd list ");
		assert(0);
		return NCSCC_RC_FAILURE;
	} else {
		/* Get the unsent messages from the list and send them */
		DTM_INTERNODE_UNSENT_MSGS *hdr = node->msgs_hdr;
		if (NULL == hdr) {
			/* No messages to be sent, reset the POLLOUT event on this fd */
			dtm_internode_reset_poll_fdlist(node->comm_socket);
		} else {
			dtm_internode_snd_unsent_msg(node);
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

#define DTM_INTERNODE_SND_MAX_COUNT 15


/**
 * Function to process unsent message
 *
 * @param node
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_internode_snd_unsent_msg(DTM_NODE_DB * node)
{
	DTM_INTERNODE_UNSENT_MSGS *hdr = node->msgs_hdr, *unsent_msg = node->msgs_hdr;
	int snd_count = 0;
	TRACE_ENTER();
	if (NULL == unsent_msg) {
		dtm_internode_reset_poll_fdlist(node->comm_socket);
		return NCSCC_RC_SUCCESS;
	}
	while (NULL != unsent_msg) {
		/* Send the message */
		int send_len = 0;
		send_len = send(node->comm_socket, unsent_msg->buffer, unsent_msg->len, MSG_NOSIGNAL);

		if (send_len == unsent_msg->len) {
			free(unsent_msg->buffer);
			unsent_msg->buffer = NULL;
			snd_count++;
			TRACE("DTM: send success, total_len : %d, send_len : %d", unsent_msg->len, send_len);
			if (DTM_INTERNODE_SND_MAX_COUNT == snd_count) {
				break;
			}
		} else {
			break;
		}
	}

	if (snd_count > 0) {
		DTM_INTERNODE_UNSENT_MSGS *mov_ptr = node->msgs_hdr, *del_ptr = NULL;
		/* Messages send from unsent messages list, now delete their entries */
		while (NULL != hdr) {
			if (NULL == hdr->buffer) {
				del_ptr = hdr;
				mov_ptr = hdr->next;
			} else {
				break;
			}
			hdr = hdr->next;
			free(del_ptr);
		}
		if (NULL == mov_ptr) {
			node->msgs_hdr = NULL;
			node->msgs_tail = NULL;
		} else if (NULL == mov_ptr->next) {
			node->msgs_hdr = mov_ptr;
			node->msgs_tail = mov_ptr;
		} else if (NULL != mov_ptr->next) {
			node->msgs_hdr = mov_ptr;
		}
	}
	if (NULL == node->msgs_hdr) {
		dtm_internode_reset_poll_fdlist(node->comm_socket);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
