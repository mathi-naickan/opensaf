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

#include"dtm.h"
#include"dtm_cb.h"
#include"dtm_intra.h"
#include"dtm_intra_disc.h"
#include"dtm_intra_trans.h"


static uint32_t dtm_lib_prepare_data_msg(uint8_t *buffer, uint16_t len);
static uint32_t dtm_intranode_snd_unsent_msg(DTM_INTRANODE_PID_INFO * pid_node, int fd);


/**
 * Function to scv intranode data messages
 *
 * @param buffer dst_pid len
 * 
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_process_rcv_internode_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len)
{
	return dtm_intranode_process_rcv_data_msg(buffer, dst_pid, len);
}

/**
 * Function to process the svc data messages
 *
 * @param buffer dst_pid len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_intranode_process_rcv_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	pid_node = dtm_intranode_get_pid_info_using_pid(dst_pid);
	if (NULL == pid_node) {
		TRACE("DTM: Destination PID not found : %d", dst_pid);
		free(buffer);
		return NCSCC_RC_FAILURE;
	} else {
		/* Prepare the message */
		dtm_lib_prepare_data_msg(buffer, (len - 2));
		dtm_intranode_send_msg(len, buffer, pid_node);
	}
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to prepare the data messages
 *
 * @param buffer len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_lib_prepare_data_msg(uint8_t *buffer, uint16_t len)
{
	uint8_t *data = buffer;
	ncs_encode_16bit(&data, (uint16_t)(len));
	ncs_encode_32bit(&data, (uint32_t)DTM_INTRANODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTRANODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_LIB_MESSAGE_TYPE);
	/* Remaining data already in buffer */
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to send the messages
 *
 * @param buffer pid_node
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_intranode_send_msg(uint16_t len, uint8_t *buffer, DTM_INTRANODE_PID_INFO * pid_node)
{
	DTM_INTRANODE_UNSENT_MSGS *add_ptr = NULL, *hdr = pid_node->msgs_hdr, *tail = pid_node->msgs_tail;

	if (NULL == hdr) {
		/* Send the message */
		int send_len = 0;
		send_len = send(pid_node->accepted_fd, buffer, len, MSG_NOSIGNAL);

		if (send_len == len) {
			free(buffer);
			return NCSCC_RC_SUCCESS;
		} else {
			TRACE("DTM: send failed, total_len : %d, send_len :%d", len, send_len);
			/* Queue the message */
			if (NULL == (add_ptr = calloc(1, sizeof(DTM_INTRANODE_UNSENT_MSGS)))) {
				TRACE("DTM : Calloc failed DTM_INTRANODE_UNSENT_MSGS");
				return NCSCC_RC_FAILURE;
			}
			add_ptr->next = NULL;
			add_ptr->buffer = buffer;
			add_ptr->len = len;
			pid_node->msgs_hdr = add_ptr;
			pid_node->msgs_tail = add_ptr;
			dtm_intranode_set_poll_fdlist(pid_node->accepted_fd, POLLOUT);
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Queue the message */
		if (NULL == (add_ptr = calloc(1, sizeof(DTM_INTRANODE_UNSENT_MSGS)))) {
			TRACE("DTM : Calloc failed DTM_INTRANODE_UNSENT_MSGS");
			return NCSCC_RC_FAILURE;
		}
		add_ptr->next = NULL;
		add_ptr->buffer = buffer;
		add_ptr->len = len;
		tail->next = add_ptr;
		pid_node->msgs_tail = add_ptr;
		dtm_intranode_set_poll_fdlist(pid_node->accepted_fd, POLLOUT);
		return NCSCC_RC_SUCCESS;
	}
}

/**
 * Function to process the pollout
 *
 * @param fd
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_intranode_process_pollout(int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		TRACE("DTM:Destination PID not found : %d", fd);
		assert(0);
		return NCSCC_RC_FAILURE;
	} else {
		/* Get the unsent messages from the list and send them */
		DTM_INTRANODE_UNSENT_MSGS *hdr = pid_node->msgs_hdr;
		if (NULL == hdr) {
			/* No messages to be sent, reset the POLLOUT event on this fd */
			dtm_intranode_reset_poll_fdlist(fd);
		} else {
			dtm_intranode_snd_unsent_msg(pid_node, fd);
		}
	}
	return NCSCC_RC_SUCCESS;
}

#define DTM_INTRANODE_SND_MAX_COUNT 50

/**
 * Function to process the send the unsend messages
 *
 * @param pid_node fd
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_intranode_snd_unsent_msg(DTM_INTRANODE_PID_INFO * pid_node, int fd)
{
	DTM_INTRANODE_UNSENT_MSGS *hdr = pid_node->msgs_hdr, *unsent_msg = pid_node->msgs_hdr;
	int snd_count = 0;
	if (NULL == unsent_msg) {
		dtm_intranode_reset_poll_fdlist(fd);
		return NCSCC_RC_SUCCESS;
	}
	while (NULL != unsent_msg) {
		/* Send the message */
		int send_len = 0;
		send_len = send(fd, unsent_msg->buffer, unsent_msg->len, MSG_NOSIGNAL);

		if (send_len == unsent_msg->len) {
			free(unsent_msg->buffer);
			unsent_msg->buffer = NULL;
			snd_count++;
			TRACE("DTM:send success, total_len : %d, send_len : %d", unsent_msg->len, send_len);
			if (DTM_INTRANODE_SND_MAX_COUNT == snd_count) {
				break;
			}
			unsent_msg = unsent_msg->next;
		} else {
			break;
		}
	}

	if (snd_count > 0) {
		DTM_INTRANODE_UNSENT_MSGS *mov_ptr = pid_node->msgs_hdr, *del_ptr = NULL;
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
			pid_node->msgs_hdr = NULL;
			pid_node->msgs_tail = NULL;
		} else if (NULL == mov_ptr->next) {
			pid_node->msgs_hdr = mov_ptr;
			pid_node->msgs_tail = mov_ptr;
		} else if (NULL != mov_ptr->next) {
			pid_node->msgs_hdr = mov_ptr;
		}
	}
	if (NULL == pid_node->msgs_hdr) {
		dtm_intranode_reset_poll_fdlist(fd);
	}
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to get the pid info using pid
 *
 * @param pid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_INTRANODE_PID_INFO *dtm_intranode_get_pid_info_using_pid(uint32_t pid)
{
	DTM_INTRANODE_PID_INFO *node = NULL;
	node =
	    (DTM_INTRANODE_PID_INFO *) ncs_patricia_tree_get(&dtm_intranode_cb->dtm_intranode_pid_list, (uint8_t *)&pid);
	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_INTRANODE_PID_INFO *) (((char *)node)
						   - (((char *)&(((DTM_INTRANODE_PID_INFO *) 0)->pid_node))
						      - ((char *)((DTM_INTRANODE_PID_INFO *) 0))));
	}
	return node;
}
