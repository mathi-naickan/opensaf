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

#include <sys/socket.h>
#include <sys/un.h>
#include "ncs_main_papi.h"
#include"dtm.h"
#include"dtm_cb.h"
#include"dtm_intra.h"
#include"dtm_intra_disc.h"
#include"dtm_intra_trans.h"
#include"dtm_inter_disc.h"

DTM_NODE_SUBSCR_INFO *dtm_node_subscr_list = NULL;
DTM_INTRANODE_NODE_DB *dtm_intranode_node_list_db = NULL;


static uint32_t dtm_lib_msg_snd_common(uint8_t *buffer, uint32_t pid, uint16_t msg_size);
static uint32_t dtm_intranode_del_svclist_from_pid_tree(DTM_INTRANODE_PID_INFO * pid_node,
						     DTM_PID_SVC_INSTALLED_INFO * del_info);

static uint32_t dtm_intranode_add_svclist_to_svc_tree(DTM_SVC_INSTALL_INFO * svc_node, DTM_SVC_LIST * add_list);
static uint32_t dtm_intranode_add_svclist_to_pid_tree(DTM_INTRANODE_PID_INFO * pid_node,
						   DTM_PID_SVC_INSTALLED_INFO * add_info);

static uint32_t dtm_lib_prepare_svc_up_msg(DTM_LIB_UP_MSG * up_msg, uint8_t *buffer);
static uint32_t dtm_lib_prepare_svc_down_msg(DTM_LIB_UP_MSG * up_msg, uint8_t *buffer);

static uint32_t dtm_intranode_del_svclist_from_svc_tree(DTM_SVC_INSTALL_INFO * svc_node, DTM_SVC_LIST * del_list);
static uint32_t dtm_internode_del_svclist_from_svc_tree(DTM_INTRANODE_NODE_DB * node_info, DTM_SVC_INSTALL_INFO * svc_node,
						     DTM_SVC_LIST * del_list);
static uint32_t dtm_intranode_del_subscrlist_from_subscr_tree(DTM_SVC_SUBSCR_INFO * subscr_node, uint64_t ref_val, uint32_t pid);
static uint32_t dtm_intranode_add_subscrlist_to_subscr_tree(DTM_SVC_SUBSCR_INFO * subscr_node,
							 DTM_SUBSCRIBER_LIST * add_info);
static uint32_t dtm_add_to_node_subscr_list(DTM_NODE_SUBSCR_INFO * node_subscr_info);
static DTM_NODE_SUBSCR_INFO *dtm_get_from_node_subscr_list(uint32_t pid);
static uint32_t dtm_lib_prepare_node_up_msg(DTM_LIB_NODE_UP_MSG * up_msg, uint8_t *buffer);
static uint32_t dtm_del_from_node_subscr_list(uint32_t pid, uint64_t ref_val);

static uint32_t dtm_deliver_svc_down(NODE_ID node_id);
static uint32_t dtm_internode_delete_svc_installed_list_from_svc_tree(DTM_INTRANODE_NODE_DB * node);

static uint32_t dtm_intranode_process_svc_down_common(DTM_INTRANODE_NODE_DB * node_info);
static DTM_SVC_INSTALL_INFO *dtm_intranode_getnext_remote_install_svc(DTM_INTRANODE_NODE_DB * node_info, uint32_t server);
static uint32_t dtm_intranode_del_subscr_from_pid_info(DTM_INTRANODE_PID_INFO * pid_node, DTM_PID_SVC_SUSBCR_INFO * data);
static uint32_t dtm_intranode_add_subscr_to_pid_info(DTM_INTRANODE_PID_INFO * pid_node, DTM_PID_SVC_SUSBCR_INFO * data);

static DTM_PID_SVC_SUSBCR_INFO *dtm_intranode_get_subscr_from_pid_info(DTM_INTRANODE_PID_INFO * pid_node,
								       uint64_t ref_val);

static DTM_SVC_INSTALL_INFO *dtm_intranode_get_svc_node(uint32_t server_type);
static DTM_SVC_SUBSCR_INFO *dtm_intranode_get_subscr_node(uint32_t server_type);

/*********************************************************

  Function NAME: dtm_intranode_process_pid_msg

  DESCRIPTION: function to process the pid message

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_pid_msg(uint8_t *buffer, int fd)
{
	uint8_t *data;		/* Used for decoding */
	NODE_ID node_id = 0;
	uint32_t process_id = 0;
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();

	pid_node = dtm_intranode_get_pid_info_using_fd(fd);

	if (NULL == pid_node) {
		LOG_ER("DTM INTRA : PID info coressponding to fd doesnt exist, database mismatch. fd :%d",fd);
		osafassert(0);
		return NCSCC_RC_FAILURE;
	}

	data = buffer;

	node_id = ncs_decode_32bit(&data);
	process_id = ncs_decode_32bit(&data);

	pid_node->accepted_fd = fd;
	if (0 == pid_node->pid) {
	pid_node->pid = process_id;
	} else {
		osafassert(0);
	}
	pid_node->node_id = m_NCS_GET_NODE_ID;
	pid_node->pid_node.key_info = (uint8_t *)&pid_node->pid;

	TRACE_1("DTM: INTRA: Processid message rcvd: pid=%d, node_id=%u", process_id, node_id);

	ncs_patricia_tree_add(&dtm_intranode_cb->dtm_intranode_pid_list, (NCS_PATRICIA_NODE *)&pid_node->pid_node);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_pid_down

  DESCRIPTION: function to process the pid down message

  ARGUMENTS: fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_pid_down(int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	TRACE_ENTER();
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come,database mismatch */
	} else {
		DTM_PID_SVC_INSTALLED_INFO *svc_list = pid_node->svc_installed_list, *del_ptr = NULL;
		DTM_INTRANODE_PID_INFO *pid_node1 = NULL;
		DTM_PID_SVC_SUSBCR_INFO *subscr_data = pid_node->subscr_list;
		DTM_NODE_SUBSCR_INFO *node_subscr = NULL;

		while (NULL != subscr_data) {
			DTM_SVC_SUBSCR_INFO *subscr_tmp = NULL;
			DTM_PID_SVC_SUSBCR_INFO *tmp_del_ptr = subscr_data->next;
			subscr_tmp = dtm_intranode_get_subscr_node(subscr_data->server_type);

			if (NULL == subscr_tmp) {
				/* Data base mismatch, unsubscribe without subscribe , is this possible */
				osafassert(0);
			} else {
				/* Now Delete the entry */
				dtm_intranode_del_subscrlist_from_subscr_tree(subscr_tmp, subscr_data->ref_hdl,
									      pid_node->pid);
			}
			free(subscr_data);
			subscr_data = tmp_del_ptr;
		}
		pid_node->subscr_list = NULL;

		/* Delete if any node subscriptions are present */
		node_subscr = dtm_get_from_node_subscr_list(pid_node->pid);

		if (NULL != node_subscr) {
			dtm_del_from_node_subscr_list(node_subscr->process_id, node_subscr->subtn_ref_val);
		}

		pid_node1 = dtm_intranode_get_pid_info_using_pid(pid_node->pid);

		TRACE_1("DTM: INTRA: Process Down process-id=%d", pid_node->pid);
		while (NULL != svc_list) {
			DTM_SVC_SUBSCR_INFO *subscr_node = NULL;
			DTM_SUBSCRIBER_LIST *subscr_list = NULL;
			DTM_SVC_INSTALL_INFO *svc_node = NULL;
			svc_node = dtm_intranode_get_svc_node(svc_list->server_type);
			if (NULL != svc_node) {
				/* delete the entries */
				DTM_SVC_LIST list = { 0 };
				list.server_inst_lower = svc_list->server_instance_lower;
				list.server_inst_higher = svc_list->server_instance_upper;
				list.node_id = pid_node->node_id;
				list.process_id = pid_node->pid;
				list.install_scope = svc_list->install_scope;
				dtm_intranode_del_svclist_from_svc_tree(svc_node, &list);
			}

			if (svc_list->install_scope > DTM_SVC_INSTALL_SCOPE_NODE)
				dtm_del_from_svc_dist_list(svc_list->server_type, svc_list->server_instance_lower,
							   pid_node->pid);

			subscr_node = dtm_intranode_get_subscr_node(svc_list->server_type);

			if (NULL != subscr_node) {

				/* Subscriptions present send to all the processes in the local node */
				uint8_t buffer[DTM_LIB_DOWN_MSG_SIZE_FULL];
				DTM_LIB_DOWN_MSG down_msg = { 0 };

				subscr_list = subscr_node->subscriber_list;
				down_msg.server_type = svc_list->server_type;
				down_msg.server_instance_lower = svc_list->server_instance_lower;
				down_msg.server_instance_upper = svc_list->server_instance_upper;

				down_msg.node_id = pid_node->node_id;
				down_msg.process_id = pid_node->pid;
				dtm_lib_prepare_svc_down_msg(&down_msg, buffer);

				while (NULL != subscr_list) {
					if (subscr_list->pid != pid_node->pid) {
						uint8_t *snd_buf = NULL;
						if (NULL == (snd_buf = calloc(1, DTM_LIB_DOWN_MSG_SIZE_FULL))) {
							TRACE("DTM :calloc failed for svc_down msg");
						} else {
							/* Send the message of SVC UP */
							uint8_t *ptr = &buffer[20];
							ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
							memcpy(snd_buf, buffer, DTM_LIB_DOWN_MSG_SIZE_FULL);
							dtm_lib_msg_snd_common(snd_buf,
									       subscr_list->pid,
									       DTM_LIB_DOWN_MSG_SIZE_FULL);
						}
					}
					subscr_list = subscr_list->next;
				}
			}
			del_ptr = svc_list;

			svc_list = svc_list->next;
			dtm_intranode_del_svclist_from_pid_tree(pid_node, del_ptr);
		}
		if (pid_node1) {
			ncs_patricia_tree_del(&dtm_intranode_cb->dtm_intranode_pid_list,
					      (NCS_PATRICIA_NODE *)&pid_node1->pid_node);
		}
		ncs_patricia_tree_del(&dtm_intranode_cb->dtm_intranode_fd_list,
				      (NCS_PATRICIA_NODE *)&pid_node->fd_node);
		close(fd);
		m_NCS_IPC_DETACH(&pid_node->mbx, NULL, NULL);
		m_NCS_IPC_RELEASE(&pid_node->mbx, NULL);

		close(pid_node->mbx_fd);
		DTM_INTRANODE_UNSENT_MSGS  *tmp = NULL;
		while(pid_node->msgs_hdr != NULL) {
			tmp = pid_node->msgs_hdr;
			if (tmp->buffer != NULL)
				free(tmp->buffer);
			pid_node->msgs_hdr = pid_node->msgs_hdr->next;
			free(tmp);
		}
		free(pid_node);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_bind_msg

  DESCRIPTION: function to process the bind message

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_bind_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		/* Decode the message  */
		uint8_t *data = buff;	/* Used for decoding */
		DTM_PID_SVC_INSTALLED_INFO *svc_install_info = NULL;
		DTM_SVC_INSTALL_INFO *svc_node = NULL;
		DTM_SVC_LIST *svc_list = NULL;
		DTM_SVC_SUBSCR_INFO *subscr_node = NULL;

		if (NULL == (svc_install_info = calloc(1, sizeof(DTM_PID_SVC_INSTALLED_INFO)))) {
			TRACE("DTM :Memory allocation failed for DTM_PID_SVC_INSTALLED_INFO");
			return NCSCC_RC_FAILURE;
		}
		svc_install_info->install_scope = (uint32_t)ncs_decode_8bit(&data);
		svc_install_info->install_scope = svc_install_info->install_scope + 1;
		svc_install_info->server_type = ncs_decode_32bit(&data);
		svc_install_info->server_instance_lower = ncs_decode_32bit(&data);
		svc_install_info->server_instance_upper = ncs_decode_32bit(&data);

		TRACE_1("DTM: INTRA: bind type=%d, inst=%d, scope=%d, pid=%d", svc_install_info->server_type,
		       svc_install_info->server_instance_lower, svc_install_info->install_scope, pid_node->pid);
		dtm_intranode_add_svclist_to_pid_tree(pid_node, svc_install_info);
		if (svc_install_info->install_scope == DTM_SVC_INSTALL_SCOPE_NONE) {
			/* Message should be sent to other nodes as well */
			dtm_add_to_svc_dist_list(svc_install_info->server_type,
						 svc_install_info->server_instance_lower, pid_node->pid);
		}
		/* local node  */

		svc_node = dtm_intranode_get_svc_node(svc_install_info->server_type);
		if (NULL == svc_node) {
			if (NULL == (svc_node = calloc(1, sizeof(DTM_SVC_INSTALL_INFO)))) {
				TRACE("DTM :calloc failed for DTM_SVC_INSTALL_INFO");
				dtm_intranode_del_svclist_from_pid_tree(pid_node, svc_install_info);
				return NCSCC_RC_FAILURE;
			}
			svc_node->server_type = svc_install_info->server_type;
			svc_node->svc_install_node.key_info = (uint8_t *)&svc_node->server_type;
			ncs_patricia_tree_add(&dtm_intranode_cb->dtm_svc_install_list,
					      (NCS_PATRICIA_NODE *)&svc_node->svc_install_node);
		}
		if (NULL == (svc_list = calloc(1, sizeof(DTM_SVC_LIST)))) {
			TRACE("DTM :calloc failed for DTM_SVC_LIST");
			dtm_intranode_del_svclist_from_pid_tree(pid_node, svc_install_info);
			return NCSCC_RC_FAILURE;
		}
		svc_list->server_inst_lower = svc_install_info->server_instance_lower;
		svc_list->server_inst_higher = svc_install_info->server_instance_upper;
		svc_list->node_id = pid_node->node_id;
		svc_list->process_id = pid_node->pid;
		svc_list->install_scope = svc_install_info->install_scope;

		dtm_intranode_add_svclist_to_svc_tree(svc_node, svc_list);
		subscr_node = dtm_intranode_get_subscr_node(svc_install_info->server_type);
		if (NULL == subscr_node) {
			/* No subscriber , just return */
			return NCSCC_RC_SUCCESS;
		} else {
			/* Subscriptions present send to all the processes in the local node */
			DTM_SUBSCRIBER_LIST *subscr_list = subscr_node->subscriber_list;
			uint8_t buffer[DTM_LIB_UP_MSG_SIZE_FULL];
			DTM_LIB_UP_MSG up_msg = { 0 };
			up_msg.server_type = svc_install_info->server_type;
			up_msg.server_instance_lower = svc_install_info->server_instance_lower;
			up_msg.server_instance_upper = svc_install_info->server_instance_upper;
			up_msg.node_id = pid_node->node_id;
			up_msg.process_id = pid_node->pid;
			dtm_lib_prepare_svc_up_msg(&up_msg, buffer);

			while (NULL != subscr_list) {
				uint8_t *snd_buf = NULL;

				if (NULL == (snd_buf = calloc(1, DTM_LIB_UP_MSG_SIZE_FULL))) {
					TRACE("DTM :calloc failed for svc_up msg");
				} else {
					/* Send the message of SVC UP */
					uint8_t *ptr = &buffer[20];
					ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
					memcpy(snd_buf, buffer, DTM_LIB_UP_MSG_SIZE_FULL);
					dtm_lib_msg_snd_common(snd_buf, subscr_list->pid, DTM_LIB_UP_MSG_SIZE_FULL);
				}
				subscr_list = subscr_list->next;
			}
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_unbind_msg

  DESCRIPTION: fucntion to process the unbind messages

  ARGUMENTS: buff, fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_unbind_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	DTM_PID_SVC_INSTALLED_INFO pid_svc_info = { 0 };
	DTM_SVC_LIST svc_list = { 0 };
	uint8_t *data = buff;	/* Used for decoding */
	uint8_t install_scope;
	uint32_t server_type;

	TRACE_ENTER();
	install_scope = ncs_decode_8bit(&data);
	server_type = ncs_decode_32bit(&data);
	svc_list.server_inst_lower = ncs_decode_32bit(&data);
	svc_list.server_inst_higher = ncs_decode_32bit(&data);
	svc_list.node_id = ncs_decode_32bit(&data);
	svc_list.process_id = ncs_decode_32bit(&data);

	pid_svc_info.server_type = server_type;
	pid_svc_info.server_instance_lower = svc_list.server_inst_lower;
	pid_svc_info.server_instance_upper = svc_list.server_inst_higher;

	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	install_scope = install_scope + 1;

	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		DTM_SVC_INSTALL_INFO *svc_node = NULL;
		DTM_SVC_SUBSCR_INFO *subscr_node = NULL;

		TRACE_1("DTM: INTRA: unbind type=%d, inst=%d, scope=%d, pid=%d", server_type,
		       svc_list.server_inst_lower, install_scope, pid_node->pid);
		if (install_scope == DTM_SVC_INSTALL_SCOPE_NONE) {
			/* Message should be sent to other nodes as well */
			dtm_del_from_svc_dist_list(server_type, svc_list.server_inst_lower, pid_node->pid);
		}
		/* Only local node  */

		svc_node = dtm_intranode_get_svc_node(server_type);
		if (NULL == svc_node) {
			/* Data base mismatch */
			osafassert(0);
		}

		dtm_intranode_del_svclist_from_pid_tree(pid_node, &pid_svc_info);
		dtm_intranode_del_svclist_from_svc_tree(svc_node, &svc_list);

		subscr_node = dtm_intranode_get_subscr_node(server_type);

		if (NULL == subscr_node) {
			return NCSCC_RC_SUCCESS;
		} else {
			/* Subscriptions present send to all the processes in the local node */
			DTM_SUBSCRIBER_LIST *subscr_list = subscr_node->subscriber_list;
			uint8_t buffer[DTM_LIB_DOWN_MSG_SIZE_FULL];
			DTM_LIB_DOWN_MSG down_msg = { 0 };
			down_msg.server_type = server_type;
			down_msg.server_instance_lower = svc_list.server_inst_lower;
			down_msg.server_instance_upper = svc_list.server_inst_lower;
			down_msg.node_id = pid_node->node_id;
			down_msg.process_id = pid_node->pid;
			dtm_lib_prepare_svc_down_msg(&down_msg, buffer);

			while (NULL != subscr_list) {
				uint8_t *snd_buf = NULL;

				if (NULL == (snd_buf = calloc(1, DTM_LIB_DOWN_MSG_SIZE_FULL))) {
					TRACE("DTM :calloc failed for svc_down msg");
				} else {
					/* Send the message of SVC down */
					uint8_t *ptr = &buffer[20];
					ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
					memcpy(snd_buf, buffer, DTM_LIB_DOWN_MSG_SIZE_FULL);
					dtm_lib_msg_snd_common(snd_buf, subscr_list->pid, DTM_LIB_DOWN_MSG_SIZE_FULL);
				}
				subscr_list = subscr_list->next;
			}
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_subscribe_msg

  DESCRIPTION: function to process the subscribe messages

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_subscribe_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		/* Decode the message  */
		uint8_t *data = buff;	/* Used for decoding */
		DTM_SVC_SUBSCR_INFO *subscr_node = NULL;
		DTM_SUBSCRIBER_LIST *subscr_info = NULL;
		uint32_t server_type = 0, server_inst_lower = 0, server_inst_higher = 0;
		DTM_SVC_SUBSCR_SCOPE subscr_scope = 0;
		DTM_SVC_INSTALL_INFO *svc_node = NULL;
		DTM_SVC_LIST *svc_list = NULL;
		DTM_PID_SVC_SUSBCR_INFO *subscr_data = NULL;

		subscr_scope = ncs_decode_8bit(&data);
		subscr_scope = subscr_scope + 1;
		server_type = ncs_decode_32bit(&data);

		subscr_node = dtm_intranode_get_subscr_node(server_type);

		if (NULL == subscr_node) {
			if (NULL == (subscr_node = calloc(1, sizeof(DTM_SVC_SUBSCR_INFO)))) {
				TRACE("DTM :calloc failed for DTM_SVC_SUBSCR_INFO");
				return NCSCC_RC_FAILURE;
			}
			subscr_node->server_type = server_type;
			subscr_node->svc_subscr_node.key_info = (uint8_t *)&subscr_node->server_type;
			ncs_patricia_tree_add(&dtm_intranode_cb->dtm_svc_subscr_list,
					      (NCS_PATRICIA_NODE *)&subscr_node->svc_subscr_node);
		}

		if (NULL == (subscr_info = calloc(1, sizeof(DTM_SUBSCRIBER_LIST)))) {
			TRACE("DTM :calloc failed for DTM_SUBSCRIBER_LIST");
			return NCSCC_RC_FAILURE;
		}
		if (NULL == (subscr_data = calloc(1, sizeof(DTM_PID_SVC_SUSBCR_INFO)))) {
			TRACE("DTM :calloc failed for DTM_PID_SVC_SUSBCR_INFO");
			return NCSCC_RC_FAILURE;
		}

		/* Fill the parameters */
		server_inst_lower = ncs_decode_32bit(&data);
		server_inst_higher = ncs_decode_32bit(&data);
		subscr_info->pid = pid_node->pid;
		subscr_info->connected_fd = fd;
		subscr_info->subscr_ref_hdl = ncs_decode_64bit(&data);
		subscr_info->mbx = pid_node->mbx;
		subscr_data->server_type = server_type;
		subscr_data->ref_hdl = subscr_info->subscr_ref_hdl;

		dtm_intranode_add_subscr_to_pid_info(pid_node, subscr_data);
		dtm_intranode_add_subscrlist_to_subscr_tree(subscr_node, subscr_info);

		TRACE_1("DTM: INTRA: svc subscribe type=%d, inst_low=%u, inst_high=%u, scope=%d, pid=%d", server_type,
		       server_inst_lower, server_inst_higher, subscr_scope, pid_node->pid);
		/* Now check the service installed list and send up message */

		svc_node = dtm_intranode_get_svc_node(server_type);
		if (NULL == svc_node) {
			/* No service installed */
			return NCSCC_RC_SUCCESS;
		} else {
			svc_list = svc_node->svc_list;

			while (NULL != svc_list) {
				if ((subscr_scope == DTM_SVC_INSTALL_SCOPE_NODE)
				    && (svc_list->node_id != pid_node->node_id)) {
					/* Dont send any message, match hasnt found */

				} else {
					uint8_t *buffer = NULL;
					DTM_LIB_UP_MSG up_msg = { 0 };

					if (NULL == (buffer = calloc(1, DTM_LIB_UP_MSG_SIZE_FULL))) {
						TRACE("DTM :calloc failed for svc_up msg");
					} else {
						up_msg.server_type = server_type;
						up_msg.server_instance_lower = svc_list->server_inst_lower;
						up_msg.server_instance_upper = svc_list->server_inst_higher;
						up_msg.node_id = svc_list->node_id;
						up_msg.process_id = svc_list->process_id;
						up_msg.ref_val = subscr_info->subscr_ref_hdl;
						dtm_lib_prepare_svc_up_msg(&up_msg, buffer);
						dtm_lib_msg_snd_common(buffer, subscr_info->pid,
								       DTM_LIB_UP_MSG_SIZE_FULL);
					}
				}
				svc_list = svc_list->next;
			}
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_unsubscribe_msg

  DESCRIPTION: functions to process the unsubscribe messages

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_unsubscribe_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		/* Decode the message  */
		uint8_t *data = buff;
		DTM_SVC_SUBSCR_INFO *subscr_node = NULL;
		uint64_t ref_val = 0;
		uint32_t server_type = 0;
		DTM_PID_SVC_SUSBCR_INFO *data_subscr = NULL;
		ref_val = ncs_decode_64bit(&data);

		data_subscr = dtm_intranode_get_subscr_from_pid_info(pid_node, ref_val);
		if (NULL == data_subscr) {
			osafassert(0);
		} else {
			server_type = data_subscr->server_type;
			dtm_intranode_del_subscr_from_pid_info(pid_node, data_subscr);
			TRACE_1("DTM: INTRA: svc unsubscribe type=%d, ref_hdl =%" PRIu64 " pid=%d", server_type, ref_val, pid_node->pid);
		}

		subscr_node = dtm_intranode_get_subscr_node(server_type);

		if (NULL == subscr_node) {
			/* Data base mismatch, unsubscribe without subscribe , is this possible */
			osafassert(0);
		} else {
			/* Now Delete the entry */
			dtm_intranode_del_subscrlist_from_subscr_tree(subscr_node, ref_val, pid_node->pid);
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_node_subscribe_msg

  DESCRIPTION: function to process the node subscribe messages

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_node_subscribe_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		uint8_t *data = buff;
		DTM_NODE_SUBSCR_INFO *node_subscr_info = NULL;
		DTM_INTRANODE_NODE_DB *node_db = dtm_intranode_node_list_db;
		if (NULL == (node_subscr_info = calloc(1, sizeof(DTM_NODE_SUBSCR_INFO)))) {
			TRACE("DTM :Memory allocation failed in dtm_intranode_process_node_subscribe_msg");
			return NCSCC_RC_FAILURE;
		}
		node_subscr_info->node_id = ncs_decode_32bit(&data);
		node_subscr_info->process_id = ncs_decode_32bit(&data);
		node_subscr_info->subtn_ref_val = ncs_decode_64bit(&data);
		TRACE_1("DTM: INTRA: node subscribe pid=%d", pid_node->pid);

		dtm_add_to_node_subscr_list(node_subscr_info);
		while (NULL != node_db) {
			DTM_LIB_NODE_UP_MSG node_up_msg = { 0 };
			uint8_t *buffer = NULL;
			if (NULL == (buffer = calloc(1, DTM_LIB_NODE_UP_MSG_SIZE_FULL))) {
				TRACE("DTM :calloc failed for node_up, dtm_intranode_process_node_subscribe_msg");
			} else {
				node_up_msg.node_id = node_db->node_id;
				node_up_msg.ref_val = node_subscr_info->subtn_ref_val;
				dtm_lib_prepare_node_up_msg(&node_up_msg, buffer);
				dtm_lib_msg_snd_common(buffer,
						       node_subscr_info->process_id, DTM_LIB_NODE_UP_MSG_SIZE_FULL);
			}
			node_db = node_db->next;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_node_unsubscribe_msg

  DESCRIPTION: function to process the node unsubscribe messages

  ARGUMENTS: buff fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_node_unsubscribe_msg(uint8_t *buff, int fd)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	TRACE_ENTER();
	pid_node = dtm_intranode_get_pid_info_using_fd(fd);
	if (NULL == pid_node) {
		osafassert(0);	/* This condition should never come */
	} else {
		uint8_t *data = buff;
		NODE_ID node_id = ncs_decode_32bit(&data);
		uint32_t process_id = ncs_decode_32bit(&data);
		uint64_t ref_val = ncs_decode_64bit(&data);
		node_id = node_id;	/* Just to avoid compilation error */
		TRACE_1("DTM: INTRA: node unsubscribe pid=%d", pid_node->pid);
		dtm_del_from_node_subscr_list(process_id, ref_val);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_add_svclist_to_pid_tree

  DESCRIPTION: function to add the pid to svclist tree

  ARGUMENTS: pid_node add_info

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_add_svclist_to_pid_tree(DTM_INTRANODE_PID_INFO * pid_node, DTM_PID_SVC_INSTALLED_INFO * add_info)
{
	DTM_PID_SVC_INSTALLED_INFO *mov_ptr = pid_node->svc_installed_list;

	TRACE_ENTER();
	if (NULL == mov_ptr) {
		add_info->next = NULL;
	} else {
		add_info->next = mov_ptr;
	}
	pid_node->svc_installed_list = add_info;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_del_svclist_from_pid_tree

  DESCRIPTION: function to delete the pid from svclist

  ARGUMENTS: pid_node del_info

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_del_svclist_from_pid_tree(DTM_INTRANODE_PID_INFO * pid_node,
						     DTM_PID_SVC_INSTALLED_INFO * del_info)
{
	DTM_PID_SVC_INSTALLED_INFO *back, *mov_ptr;

	TRACE_ENTER();
	for (back = NULL, mov_ptr = pid_node->svc_installed_list; mov_ptr != NULL;
	     back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((del_info->server_type == mov_ptr->server_type)
		    && (del_info->server_instance_lower == mov_ptr->server_instance_lower)
		    && (del_info->server_instance_upper == mov_ptr->server_instance_upper)) {
			if (back == NULL) {
				pid_node->svc_installed_list = mov_ptr->next;
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM :Successfully deleted the node from svc list ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM :No matching entry found in svc list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: dtm_intranode_add_svclist_to_svc_tree

  DESCRIPTION: function to ass the svclist to svc tree

  ARGUMENTS: svc_node add_list

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_add_svclist_to_svc_tree(DTM_SVC_INSTALL_INFO * svc_node, DTM_SVC_LIST * add_list)
{
	DTM_SVC_LIST *mov_ptr = svc_node->svc_list;
	TRACE_ENTER();
	if (NULL == mov_ptr) {
		add_list->next = NULL;
	} else {
		add_list->next = mov_ptr;
	}
	svc_node->svc_list = add_list;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_internode_del_svclist_from_svc_tree

  DESCRIPTION: function to delete the svclist from svc_tree

  ARGUMENTS: node_info svc_node del_list

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_internode_del_svclist_from_svc_tree(DTM_INTRANODE_NODE_DB * node_info, DTM_SVC_INSTALL_INFO * svc_node,
						     DTM_SVC_LIST * del_list)
{
	DTM_SVC_LIST *back, *mov_ptr;

	TRACE_ENTER();
	for (back = NULL, mov_ptr = svc_node->svc_list; mov_ptr != NULL; back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((del_list->server_inst_lower == mov_ptr->server_inst_lower) &&
		    (del_list->server_inst_higher == mov_ptr->server_inst_higher) &&
		    (del_list->process_id == mov_ptr->process_id) && (del_list->node_id == mov_ptr->node_id)) {
			if (back == NULL) {
				svc_node->svc_list = mov_ptr->next;
				if (NULL == svc_node->svc_list) {
					/*delete the entire tree */
					ncs_patricia_tree_del(&node_info->dtm_rem_node_svc_tree,
							      (NCS_PATRICIA_NODE *)&svc_node->svc_install_node);
					free(svc_node);
				}
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM :Successfully deleted the node from svc list ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in svc list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: dtm_intranode_del_svclist_from_svc_tree

  DESCRIPTION: function to delete the svclist from svc_tree for intranode

  ARGUMENTS: svc_node del_list

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_del_svclist_from_svc_tree(DTM_SVC_INSTALL_INFO * svc_node, DTM_SVC_LIST * del_list)
{
	DTM_SVC_LIST *back, *mov_ptr;

	TRACE_ENTER();
	for (back = NULL, mov_ptr = svc_node->svc_list; mov_ptr != NULL; back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((del_list->server_inst_lower == mov_ptr->server_inst_lower) &&
		    (del_list->server_inst_higher == mov_ptr->server_inst_higher) &&
		    (del_list->process_id == mov_ptr->process_id) && (del_list->node_id == mov_ptr->node_id)) {
			if (back == NULL) {
				svc_node->svc_list = mov_ptr->next;
				if (NULL == svc_node->svc_list) {
					/*delete the entire tree */
					ncs_patricia_tree_del(&dtm_intranode_cb->dtm_svc_install_list,
							      (NCS_PATRICIA_NODE *)&svc_node->svc_install_node);
					free(svc_node);
				}
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM : Successfully deleted the node from svc list ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in svc list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: dtm_intranode_add_subscr_to_pid_info

  DESCRIPTION: function to add the subscribe info to pid 

  ARGUMENTS: pid_node data

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_add_subscr_to_pid_info(DTM_INTRANODE_PID_INFO * pid_node, DTM_PID_SVC_SUSBCR_INFO * data)
{
	DTM_PID_SVC_SUSBCR_INFO *hdr = pid_node->subscr_list;
	TRACE_ENTER();
	if (NULL == hdr) {
		data->next = NULL;
	} else {
		data->next = hdr;
	}
	pid_node->subscr_list = data;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_get_subscr_from_pid_info

  DESCRIPTION: functio to get the subscribe info from pid

  ARGUMENTS: pid_node ref_val

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_PID_SVC_SUSBCR_INFO *dtm_intranode_get_subscr_from_pid_info(DTM_INTRANODE_PID_INFO * pid_node, uint64_t ref_val)
{
	DTM_PID_SVC_SUSBCR_INFO *hdr = pid_node->subscr_list;
	TRACE_ENTER();
	while (NULL != hdr) {
		if (ref_val == hdr->ref_hdl) {
			return hdr;
		}
		hdr = hdr->next;
	}
	TRACE_LEAVE();
	return hdr;
}

/*********************************************************

  Function NAME: dtm_intranode_del_subscr_from_pid_info

  DESCRIPTION: function to del the subscribe info from pid

  ARGUMENTS: pid_node data

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_del_subscr_from_pid_info(DTM_INTRANODE_PID_INFO * pid_node, DTM_PID_SVC_SUSBCR_INFO * data)
{
	DTM_PID_SVC_SUSBCR_INFO *back, *mov_ptr;
	for (back = NULL, mov_ptr = pid_node->subscr_list; mov_ptr != NULL; back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((data->ref_hdl == mov_ptr->ref_hdl) && (data->server_type == mov_ptr->server_type)) {
			if (back == NULL) {
				pid_node->subscr_list = mov_ptr->next;
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM : Successfully deleted the subscr node from pid info ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in subscr  data for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: dtm_intranode_add_subscrlist_to_subscr_tree

  DESCRIPTION: function to add subscribe list to subscr tree

  ARGUMENTS: subscr_node add_info

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_add_subscrlist_to_subscr_tree(DTM_SVC_SUBSCR_INFO * subscr_node,
							 DTM_SUBSCRIBER_LIST * add_info)
{
	DTM_SUBSCRIBER_LIST *mov_ptr = subscr_node->subscriber_list;

	TRACE_ENTER();
	if (NULL == mov_ptr) {
		add_info->next = NULL;
	} else {
		add_info->next = mov_ptr;
	}
	subscr_node->subscriber_list = add_info;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_del_subscrlist_from_subscr_tree

  DESCRIPTION: function to del subscribe list to subscr tree

  ARGUMENTS: subscr_node ref_val pid

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_del_subscrlist_from_subscr_tree(DTM_SVC_SUBSCR_INFO * subscr_node, uint64_t ref_val, uint32_t pid)
{
	DTM_SUBSCRIBER_LIST *back, *mov_ptr;
	TRACE_ENTER();
	for (back = NULL, mov_ptr = subscr_node->subscriber_list; mov_ptr != NULL;
	     back = mov_ptr, mov_ptr = mov_ptr->next) {
		if ((pid == mov_ptr->pid) && (ref_val == mov_ptr->subscr_ref_hdl)) {
			if (back == NULL) {
				if (NULL == mov_ptr->next) {
					subscr_node->subscriber_list = NULL;
					ncs_patricia_tree_del(&dtm_intranode_cb->dtm_svc_subscr_list,
							      (NCS_PATRICIA_NODE *)&subscr_node->svc_subscr_node);
					free(subscr_node);
				} else {
					subscr_node->subscriber_list = mov_ptr->next;
				}
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM : Successfully deleted the subscr node from subscr list ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in subscr list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: dtm_lib_prepare_svc_up_msg

  DESCRIPTION: function to prepare the svc up messages

  ARGUMENTS: up_msg buffer 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_lib_prepare_svc_up_msg(DTM_LIB_UP_MSG * up_msg, uint8_t *buffer)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_LIB_UP_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTRANODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTRANODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_LIB_UP_TYPE);
	ncs_encode_32bit(&data, up_msg->server_type);
	ncs_encode_32bit(&data, up_msg->server_instance_lower);
	ncs_encode_32bit(&data, up_msg->server_instance_upper);
	ncs_encode_64bit(&data, up_msg->ref_val);
	ncs_encode_32bit(&data, up_msg->node_id);
	ncs_encode_32bit(&data, up_msg->process_id);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_lib_prepare_svc_down_msg 

  DESCRIPTION: function to prepare the svc down messages

  ARGUMENTS: down_msg buffer

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_lib_prepare_svc_down_msg(DTM_LIB_DOWN_MSG * down_msg, uint8_t *buffer)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_LIB_DOWN_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTRANODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTRANODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_LIB_DOWN_TYPE);
	ncs_encode_32bit(&data, down_msg->server_type);
	ncs_encode_32bit(&data, down_msg->server_instance_lower);
	ncs_encode_32bit(&data, down_msg->server_instance_upper);
	ncs_encode_64bit(&data, down_msg->ref_val);
	ncs_encode_32bit(&data, down_msg->node_id);
	ncs_encode_32bit(&data, down_msg->process_id);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_add_to_node_subscr_list

  DESCRIPTION: function to add node to subscr list

  ARGUMENTS: node_subscr_info

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_add_to_node_subscr_list(DTM_NODE_SUBSCR_INFO * node_subscr_info)
{
	DTM_NODE_SUBSCR_INFO *node_subscr_ptr = dtm_node_subscr_list;
	TRACE_ENTER();
	if (NULL == node_subscr_ptr) {
		node_subscr_info->next = NULL;
	} else {
		node_subscr_info->next = node_subscr_ptr;
	}
	dtm_node_subscr_list = node_subscr_info;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_get_from_node_subscr_list

  DESCRIPTION: function to get node to subscr list

  ARGUMENTS: pid

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_NODE_SUBSCR_INFO *dtm_get_from_node_subscr_list(uint32_t pid)
{
	DTM_NODE_SUBSCR_INFO *node_subscr_ptr = dtm_node_subscr_list;
	TRACE_ENTER();
	while (NULL != node_subscr_ptr) {
		if (pid == node_subscr_ptr->process_id) {
			return node_subscr_ptr;
		}
		node_subscr_ptr = node_subscr_ptr->next;
	}
	TRACE_LEAVE();
	return node_subscr_ptr;
}
/*********************************************************

  Function NAME: dtm_del_from_node_subscr_list

  DESCRIPTION: function to del node to subscr list

  ARGUMENTS: pid ref_val

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_del_from_node_subscr_list(uint32_t pid, uint64_t ref_val)
{
	DTM_NODE_SUBSCR_INFO *back, *mov_ptr;

	TRACE_ENTER();
	for (back = NULL, mov_ptr = dtm_node_subscr_list; mov_ptr != NULL; back = mov_ptr, mov_ptr = mov_ptr->next) {
		if (pid == mov_ptr->process_id && ref_val == mov_ptr->subtn_ref_val) {
			if (back == NULL) {
				dtm_node_subscr_list = mov_ptr->next;
			} else {
				back->next = mov_ptr->next;
			}
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("DTM : Successfully deleted the subscr node from node subscr list ");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in Node subscr list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}
/*********************************************************

  Function NAME: dtm_add_to_node_db_list

  DESCRIPTION: function to add the node to db list

  ARGUMENTS: add_node

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_add_to_node_db_list(DTM_INTRANODE_NODE_DB * add_node)
{
	DTM_INTRANODE_NODE_DB *node_db = dtm_intranode_node_list_db;
	TRACE_ENTER();
	if (NULL == node_db) {
		add_node->next = NULL;
		dtm_intranode_node_list_db = add_node;
	} else {
		add_node->next = node_db;
		dtm_intranode_node_list_db = add_node;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_get_from_node_db_list

  DESCRIPTION: function to get the node to db list

  ARGUMENTS: get_node node_id

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_get_from_node_db_list(DTM_INTRANODE_NODE_DB ** get_node, NODE_ID node_id)
{
	DTM_INTRANODE_NODE_DB *node_db = dtm_intranode_node_list_db;
	TRACE_ENTER();
	if (NULL == node_db) {
		TRACE("DTM : Unable to find the node in the node_db_list");
		return NCSCC_RC_FAILURE;
	} else {
		while (NULL != node_db) {
			if (node_id == node_db->node_id) {
				*get_node = node_db;
				return NCSCC_RC_SUCCESS;
			}
			node_db = node_db->next;
		}
	}
	*get_node = NULL;
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}
/*********************************************************

  Function NAME: dtm_del_from_node_db_list

  DESCRIPTION: function to del the node to db list

  ARGUMENTS: node_id

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_del_from_node_db_list(NODE_ID node_id)
{
	DTM_INTRANODE_NODE_DB *back, *mov_ptr;

	TRACE_ENTER();
	for (back = NULL, mov_ptr = dtm_intranode_node_list_db; mov_ptr != NULL;
	     back = mov_ptr, mov_ptr = mov_ptr->next) {
		if (node_id == mov_ptr->node_id) {
			if (back == NULL) {
				dtm_intranode_node_list_db = mov_ptr->next;
			} else {
				back->next = mov_ptr->next;
			}

			/* Need to clean the data base as well */
			dtm_internode_delete_svc_installed_list_from_svc_tree(mov_ptr);
			ncs_patricia_tree_destroy(&mov_ptr->dtm_rem_node_svc_tree);
			free(mov_ptr);
			mov_ptr = NULL;
			TRACE("Successfully deleted the node from node db list");
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE("DTM : No matching entry found in Node db list for deletion");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}
/*********************************************************

  Function NAME: dtm_internode_delete_svc_installed_list_from_svc_tree

  DESCRIPTION: function to del the installed svc from svc tree

  ARGUMENTS: node

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_internode_delete_svc_installed_list_from_svc_tree(DTM_INTRANODE_NODE_DB * node)
{
	DTM_SVC_INSTALL_INFO *svc_info = NULL;
	DTM_SVC_LIST *svc_list = NULL, *del_ptr = NULL, local_svc_list = { 0 };
	uint32_t server_type = 0;

	TRACE_ENTER();
	while (NULL != (svc_info = dtm_intranode_getnext_remote_install_svc(node, server_type))) {

		DTM_SVC_INSTALL_INFO *local_svc_info = NULL;	/* For removal from local node */

		server_type = svc_info->server_type;
		svc_list = svc_info->svc_list;

		local_svc_info = dtm_intranode_get_svc_node(server_type);;

		while (NULL != svc_list) {
			del_ptr = svc_list;
			svc_list = svc_list->next;
			if (NULL != local_svc_info) {
				local_svc_list.server_inst_lower = del_ptr->server_inst_lower;
				local_svc_list.node_id = node->node_id;
				local_svc_list.process_id = del_ptr->process_id;
				local_svc_list.server_inst_higher = del_ptr->server_inst_higher;
				dtm_intranode_del_svclist_from_svc_tree(local_svc_info, &local_svc_list);
			}
			dtm_internode_del_svclist_from_svc_tree(node, svc_info, del_ptr);
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_lib_msg_snd_common 

  DESCRIPTION: function to send message 

  ARGUMENTS: buffer pid msg_size

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_lib_msg_snd_common(uint8_t *buffer, uint32_t pid, uint16_t msg_size)
{
	DTM_INTRANODE_PID_INFO *pid_node = NULL;
	pid_node = dtm_intranode_get_pid_info_using_pid(pid);
	if (NULL == pid_node) {
		TRACE_ENTER();
		TRACE("DTM :pid node not found, dtm_lib_msg_snd_common");
		TRACE_LEAVE();
		free(buffer);
		return NCSCC_RC_FAILURE;
	} else {
		return dtm_intranode_send_msg(msg_size, buffer, pid_node);
	}
}
/*********************************************************

  Function NAME: dtm_lib_prepare_node_up_msg

  DESCRIPTION: function to prepare the node up messages

  ARGUMENTS: up_msg buffer

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_lib_prepare_node_up_msg(DTM_LIB_NODE_UP_MSG * up_msg, uint8_t *buffer)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_LIB_NODE_UP_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTRANODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTRANODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_LIB_NODE_UP_TYPE);
	ncs_encode_32bit(&data, up_msg->node_id);
	ncs_encode_64bit(&data, up_msg->ref_val);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_lib_prepare_node_down_msg

  DESCRIPTION: function to prepare the node down messages

  ARGUMENTS:up_msg buffer

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_lib_prepare_node_down_msg(DTM_LIB_NODE_DOWN_MSG * up_msg, uint8_t *buffer)
{
	uint8_t *data = buffer;
	TRACE_ENTER();
	ncs_encode_16bit(&data, (uint16_t)DTM_LIB_NODE_DOWN_MSG_SIZE);
	ncs_encode_32bit(&data, (uint32_t)DTM_INTRANODE_SND_MSG_IDENTIFIER);
	ncs_encode_8bit(&data, (uint8_t)DTM_INTRANODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_LIB_NODE_DOWN_TYPE);
	ncs_encode_32bit(&data, up_msg->node_id);
	ncs_encode_64bit(&data, up_msg->ref_val);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_intranode_process_node_up 

  DESCRIPTION: function to process the node up

  ARGUMENTS: node_id node_name mbx

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_node_up(NODE_ID node_id, char *node_name, SYSF_MBX mbx)
{
	/* Add to the node db list */
	DTM_INTRANODE_NODE_DB *node_db_info = NULL;
	NCS_PATRICIA_PARAMS pat_tree_params = { 0 };
	TRACE_ENTER();
	if (NULL == (node_db_info = calloc(1, sizeof(DTM_INTRANODE_NODE_DB)))) {
		return NCSCC_RC_FAILURE;
	}
	node_db_info->node_id = node_id;
	strcpy(node_db_info->node_name, node_name);
	node_db_info->mbx = mbx;
	/* Initialize the pat tree */
	pat_tree_params.key_size = sizeof(uint32_t);
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&node_db_info->dtm_rem_node_svc_tree, &pat_tree_params)) {
		LOG_ER(" ncs_patricia_tree_init failed for remote node service list");
		free(node_db_info);
		return NCSCC_RC_FAILURE;
	}

	if (NCSCC_RC_SUCCESS != (dtm_add_to_node_db_list(node_db_info))) {
		/* This is critical */
		TRACE("DTM : Unable to add the node to node_db_list");
		osafassert(0);
	} else {
		/* Deliver node UP */
		DTM_NODE_SUBSCR_INFO *node_subscr_info = dtm_node_subscr_list;
		DTM_LIB_NODE_UP_MSG node_up_msg = { 0 };
		uint8_t buffer[DTM_LIB_NODE_UP_MSG_SIZE_FULL];
		node_up_msg.node_id = node_id;
		dtm_lib_prepare_node_up_msg(&node_up_msg, buffer);
		while (NULL != node_subscr_info) {
			uint8_t *snd_buf = NULL;

			if (NULL == (snd_buf = calloc(1, DTM_LIB_NODE_UP_MSG_SIZE_FULL))) {
				TRACE("DTM : calloc failed for node_up msg, not sending the message");
			} else {
				uint8_t *ptr = &buffer[12];
				ncs_encode_64bit(&ptr, node_subscr_info->subtn_ref_val);
				memcpy(snd_buf, buffer, DTM_LIB_NODE_UP_MSG_SIZE_FULL);
				dtm_lib_msg_snd_common(snd_buf, node_subscr_info->process_id,
						       DTM_LIB_NODE_UP_MSG_SIZE_FULL);
			}
			node_subscr_info = node_subscr_info->next;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_process_node_down

  DESCRIPTION: function to process the node down

  ARGUMENTS:node_id

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_process_node_down(NODE_ID node_id)
{
	DTM_NODE_SUBSCR_INFO *node_subscr_info = dtm_node_subscr_list;
	DTM_LIB_NODE_DOWN_MSG node_down_msg = { 0 };
	uint8_t buffer[DTM_LIB_NODE_DOWN_MSG_SIZE_FULL];

	/* Deliver svc down to all the subscribers of service present on this node */
	TRACE_ENTER();
	dtm_deliver_svc_down(node_id);

	/* Delete from the node db list */
	dtm_del_from_node_db_list(node_id);

	node_down_msg.node_id = node_id;
	dtm_lib_prepare_node_down_msg(&node_down_msg, buffer);

	/* Deliver node down */
	while (NULL != node_subscr_info) {
		uint8_t *snd_buf = NULL;
		if (NULL == (snd_buf = calloc(1, DTM_LIB_NODE_DOWN_MSG_SIZE_FULL))) {
			TRACE("DTM :calloc failed for node_down msg, not sending the message");
		} else {
			uint8_t *ptr = &buffer[12];
			ncs_encode_64bit(&ptr, node_subscr_info->subtn_ref_val);
			memcpy(snd_buf, buffer, DTM_LIB_NODE_UP_MSG_SIZE_FULL);
			dtm_lib_msg_snd_common(snd_buf, node_subscr_info->process_id, DTM_LIB_NODE_DOWN_MSG_SIZE_FULL);
		}
		node_subscr_info = node_subscr_info->next;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_deliver_svc_down 

  DESCRIPTION: function to delive the svc down

  ARGUMENTS: node_id

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

static uint32_t dtm_deliver_svc_down(NODE_ID node_id)
{
	/* Result of node down */
	DTM_INTRANODE_NODE_DB *node_info = NULL;
	TRACE_ENTER();
	dtm_get_from_node_db_list(&node_info, node_id);

	if (NULL == node_info) {
		/* This should not occur, database mismatch */
		osafassert(0);
	} else {
		dtm_intranode_process_svc_down_common(node_info);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/*********************************************************

  Function NAME: dtm_intranode_getnext_remote_install_svc

  DESCRIPTION: function to get the nxt remote install svc

  ARGUMENTS: node_info server

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_SVC_INSTALL_INFO *dtm_intranode_getnext_remote_install_svc(DTM_INTRANODE_NODE_DB * node_info, uint32_t server)
{
	DTM_SVC_INSTALL_INFO *node = NULL;
	node = (DTM_SVC_INSTALL_INFO *) ncs_patricia_tree_getnext(&node_info->dtm_rem_node_svc_tree, (uint8_t *)&server);

	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_SVC_INSTALL_INFO *) (((char *)node)
						 - (((char *)&(((DTM_SVC_INSTALL_INFO *) 0)->svc_install_node))
						    - ((char *)((DTM_SVC_INSTALL_INFO *) 0))));
	}
	return node;
}
/*********************************************************

  Function NAME: dtm_intranode_process_svc_down_common

  DESCRIPTION: common function to process the svc down

  ARGUMENTS: node_info

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uint32_t dtm_intranode_process_svc_down_common(DTM_INTRANODE_NODE_DB * node_info)
{
	DTM_SVC_INSTALL_INFO *svc_info = NULL;
	DTM_SVC_LIST *svc_list = NULL;
	uint32_t server_type = 0;
	DTM_SVC_SUBSCR_INFO *subscr_node = NULL;
	DTM_SUBSCRIBER_LIST *subscr_list = NULL;

	TRACE_ENTER();
	while (NULL != (svc_info = dtm_intranode_getnext_remote_install_svc(node_info, server_type))) {
		server_type = svc_info->server_type;
		svc_list = svc_info->svc_list;
		subscr_node = dtm_intranode_get_subscr_node(server_type);

		while (NULL != svc_list) {
			if (NULL != subscr_node) {
				subscr_list = subscr_node->subscriber_list;
				/* Subscriptions present send to all the processes in the local node */
				uint8_t buffer[DTM_LIB_DOWN_MSG_SIZE_FULL];
				DTM_LIB_DOWN_MSG down_msg = { 0 };
				down_msg.server_type = server_type;
				down_msg.server_instance_lower = svc_list->server_inst_lower;
				down_msg.server_instance_upper = svc_list->server_inst_higher;

				down_msg.node_id = svc_list->node_id;
				down_msg.process_id = svc_list->process_id;
				dtm_lib_prepare_svc_down_msg(&down_msg, buffer);

				while (NULL != subscr_list) {
					/* Send the message of SVC UP */
					if (subscr_list->subscr_scope == DTM_SVC_INSTALL_SCOPE_NODE) {
						/* Dont send up message as this is not the match */
					} else {
						uint8_t *snd_buf = NULL;

						if (NULL == (snd_buf = calloc(1, DTM_LIB_DOWN_MSG_SIZE_FULL))) {
							TRACE("DTM :calloc failed for svc_down msg");
						} else {
							uint8_t *ptr = &buffer[20];
							ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
							memcpy(snd_buf, buffer, DTM_LIB_DOWN_MSG_SIZE_FULL);
							dtm_lib_msg_snd_common(snd_buf, subscr_list->pid,
									       DTM_LIB_DOWN_MSG_SIZE_FULL);
						}
					}
					subscr_list = subscr_list->next;
				}
			}
			svc_list = svc_list->next;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_get_remote_install_svc

  DESCRIPTION: function to get the remote install svc

  ARGUMENTS: node_info server

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_SVC_INSTALL_INFO *dtm_intranode_get_remote_install_svc(DTM_INTRANODE_NODE_DB * node_info, uint32_t server)
{
	DTM_SVC_INSTALL_INFO *node = NULL;
	node = (DTM_SVC_INSTALL_INFO *) ncs_patricia_tree_get(&node_info->dtm_rem_node_svc_tree, (uint8_t *)&server);

	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_SVC_INSTALL_INFO *) (((char *)node)
						 - (((char *)&(((DTM_SVC_INSTALL_INFO *) 0)->svc_install_node))
						    - ((char *)((DTM_SVC_INSTALL_INFO *) 0))));
	}
	return node;
}
/*********************************************************

  Function NAME: dtm_process_internode_service_up_msg

  DESCRIPTION: function to process internode scv up message

  ARGUMENTS: buffer len node_id 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_process_internode_service_up_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	uint16_t num_of_elements = 0;
	uint8_t *data = buffer;
	DTM_INTRANODE_NODE_DB *node_info = NULL;
	DTM_SVC_SUBSCR_INFO *subscr_node = NULL;

	TRACE_ENTER();
	num_of_elements = ncs_decode_16bit(&data);

	dtm_get_from_node_db_list(&node_info, node_id);

	if (NULL == node_info) {
		/* This should not occur, database mismatch */
		osafassert(0);
	} else {

		while (0 != num_of_elements) {

			DTM_SVC_INSTALL_INFO *svc_info = NULL, *local_svc_info = NULL;
			DTM_SVC_LIST *svc_list = NULL, *svc_list_local = NULL;

			uint32_t server_type = ncs_decode_32bit(&data);

			svc_info = dtm_intranode_get_remote_install_svc(node_info, server_type);

			if (NULL == svc_info) {
				if (NULL == (svc_info = calloc(1, sizeof(DTM_SVC_INSTALL_INFO)))) {
					TRACE("DTM :Memory allocation failed in dtm_process_internode_service_up_msg");
					return NCSCC_RC_FAILURE;
				}
				svc_info->server_type = server_type;
				svc_info->svc_install_node.key_info = (uint8_t *)&svc_info->server_type;
				ncs_patricia_tree_add(&node_info->dtm_rem_node_svc_tree,
						      (NCS_PATRICIA_NODE *)&svc_info->svc_install_node);
			}

			if (NULL == (svc_list = calloc(1, sizeof(DTM_SVC_LIST)))) {
				return NCSCC_RC_FAILURE;
			}
			svc_list->server_inst_lower = ncs_decode_32bit(&data);
			svc_list->server_inst_higher = svc_list->server_inst_lower;
			svc_list->node_id = node_id;
			svc_list->process_id = ncs_decode_32bit(&data);

			TRACE("DTM :rcvd internode UP Msg type :  %d, inst : %d, node : %d , pid :%d", server_type,
			      svc_list->server_inst_lower, node_id, svc_list->process_id);
			dtm_intranode_add_svclist_to_svc_tree(svc_info, svc_list);

			/* adding to local node */

			local_svc_info = dtm_intranode_get_svc_node(server_type);
			if (NULL == local_svc_info) {
				if (NULL == (local_svc_info = calloc(1, sizeof(DTM_SVC_INSTALL_INFO)))) {
					TRACE("DTM :calloc failed for DTM_SVC_INSTALL_INFO");
					return NCSCC_RC_FAILURE;
				}
				local_svc_info->server_type = server_type;
				local_svc_info->svc_install_node.key_info = (uint8_t *)&local_svc_info->server_type;
				ncs_patricia_tree_add(&dtm_intranode_cb->dtm_svc_install_list,
						      (NCS_PATRICIA_NODE *)&local_svc_info->svc_install_node);
			}

			if (NULL == (svc_list_local = calloc(1, sizeof(DTM_SVC_LIST)))) {
				return NCSCC_RC_FAILURE;
			}
			svc_list_local->server_inst_lower = svc_list->server_inst_lower;
			svc_list_local->server_inst_higher = svc_list->server_inst_higher;
			svc_list_local->node_id = node_id;
			svc_list_local->process_id = svc_list->process_id;
			dtm_intranode_add_svclist_to_svc_tree(local_svc_info, svc_list_local);
			/* End */

			/* Now deliver up to the respective user */
			subscr_node = dtm_intranode_get_subscr_node(server_type);

			if (NULL != subscr_node) {
				/* Subscriptions present send to all the processes in the local node */
				DTM_SUBSCRIBER_LIST *subscr_list = subscr_node->subscriber_list;
				uint8_t local_buffer[DTM_LIB_UP_MSG_SIZE_FULL];
				DTM_LIB_UP_MSG up_msg = { 0 };
				up_msg.server_type = svc_info->server_type;
				up_msg.server_instance_lower = svc_list->server_inst_lower;
				up_msg.server_instance_upper = svc_list->server_inst_higher;

				up_msg.node_id = svc_list->node_id;
				up_msg.process_id = svc_list->process_id;
				dtm_lib_prepare_svc_up_msg(&up_msg, local_buffer);

				while (NULL != subscr_list) {
					/* Send the message of SVC UP */
					if (subscr_list->subscr_scope == DTM_SVC_INSTALL_SCOPE_NODE) {
						/* Dont send up message as this is not the match */
					} else {
						uint8_t *snd_buf = NULL;

						if (NULL == (snd_buf = calloc(1, DTM_LIB_UP_MSG_SIZE_FULL))) {
							TRACE("DTM :calloc failed for svc_up msg");
						} else {
							uint8_t *ptr = &local_buffer[20];
							ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
							memcpy(snd_buf, local_buffer, DTM_LIB_UP_MSG_SIZE_FULL);
							dtm_lib_msg_snd_common(snd_buf, subscr_list->pid,
									       DTM_LIB_UP_MSG_SIZE_FULL);
						}
					}
					subscr_list = subscr_list->next;
				}
			}
			num_of_elements--;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_process_internode_service_down_msg

  DESCRIPTION: function to process internode scv down message

  ARGUMENTS: buffer len node_id 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_process_internode_service_down_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id)
{
	uint16_t num_of_elements = 0;
	uint8_t *data = buffer;
	DTM_INTRANODE_NODE_DB *node_info = NULL;

	TRACE_ENTER();
	num_of_elements = ncs_decode_16bit(&data);

	dtm_get_from_node_db_list(&node_info, node_id);

	if (NULL == node_info) {
		/* This should not occur, database mismatch */
		osafassert(0);
	} else {

		while (0 != num_of_elements) {

			DTM_SVC_INSTALL_INFO *svc_info = NULL, *local_svc_node = NULL;
			DTM_SVC_LIST svc_list = { 0 };
			DTM_SVC_SUBSCR_INFO *subscr_node = NULL;

			uint32_t server_type = ncs_decode_32bit(&data);

			svc_info = dtm_intranode_get_remote_install_svc(node_info, server_type);

			if (NULL != svc_info) {

				svc_list.server_inst_lower = ncs_decode_32bit(&data);
				svc_list.server_inst_higher = svc_list.server_inst_lower;
				svc_list.node_id = node_id;
				svc_list.process_id = ncs_decode_32bit(&data);

				TRACE("DTM :rcvd internode down Msg type:  %d, inst : %d, node: %d , pid :%d", server_type,
				      svc_list.server_inst_lower, node_id, svc_list.process_id);
				dtm_internode_del_svclist_from_svc_tree(node_info, svc_info, &svc_list);

				/* local node delete */
				local_svc_node = dtm_intranode_get_svc_node(server_type);
				if (NULL == local_svc_node) {
					/* Data base mismatch */
					osafassert(0);
				}

				dtm_intranode_del_svclist_from_svc_tree(local_svc_node, &svc_list);
				/* End */

				/* Now deliver down to the respective user */
				subscr_node = dtm_intranode_get_subscr_node(server_type);

				if (NULL != subscr_node) {
					/* Subscriptions present send to all the processes in the local node */
					DTM_SUBSCRIBER_LIST *subscr_list = subscr_node->subscriber_list;
					uint8_t local_buffer[DTM_LIB_DOWN_MSG_SIZE_FULL];
					DTM_LIB_DOWN_MSG down_msg = { 0 };
					down_msg.server_type = server_type;
					down_msg.server_instance_lower = svc_list.server_inst_lower;
					down_msg.server_instance_upper = svc_list.server_inst_higher;

					down_msg.node_id = svc_list.node_id;
					down_msg.process_id = svc_list.process_id;
					dtm_lib_prepare_svc_down_msg(&down_msg, local_buffer);

					while (NULL != subscr_list) {
						/* Send the message of SVC UP */
						if (subscr_list->subscr_scope == DTM_SVC_INSTALL_SCOPE_NODE) {
							/* Dont send up message as this is not the match */
						} else {
							uint8_t *snd_buf = NULL;

							if (NULL == (snd_buf = calloc(1, DTM_LIB_DOWN_MSG_SIZE_FULL))) {
								TRACE("DTM :calloc failed for svc_down msg");
							} else {
								uint8_t *ptr = &local_buffer[20];
								ncs_encode_64bit(&ptr, subscr_list->subscr_ref_hdl);
								memcpy(snd_buf, local_buffer, DTM_LIB_DOWN_MSG_SIZE_FULL);
								dtm_lib_msg_snd_common(snd_buf,
										       subscr_list->pid,
										       DTM_LIB_DOWN_MSG_SIZE_FULL);
							}
						}
						subscr_list = subscr_list->next;
					}
				}
			}

			num_of_elements--;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: dtm_intranode_add_self_node_to_node_db

  DESCRIPTION: function to add self node to node db

  ARGUMENTS: node_id

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uint32_t dtm_intranode_add_self_node_to_node_db(NODE_ID node_id)
{
	/* Add to the node db list */
	DTM_INTRANODE_NODE_DB *node_db_info = NULL;
	TRACE_ENTER();
	if (NULL == (node_db_info = calloc(1, sizeof(DTM_INTRANODE_NODE_DB)))) {
		return NCSCC_RC_FAILURE;
	}
	node_db_info->node_id = node_id;
	if (NCSCC_RC_SUCCESS != (dtm_add_to_node_db_list(node_db_info))) {
		/* This is critical */
		TRACE("DTM : Unable to add the node to node_db_list");
		osafassert(0);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}
/*********************************************************

  Function NAME: dtm_intranode_get_pid_info_using_fd

  DESCRIPTION: funtion to get the pid info using fd

  ARGUMENTS: fd

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
DTM_INTRANODE_PID_INFO *dtm_intranode_get_pid_info_using_fd(int fd)
{
	DTM_INTRANODE_PID_INFO *node = NULL;
	node = (DTM_INTRANODE_PID_INFO *) ncs_patricia_tree_get(&dtm_intranode_cb->dtm_intranode_fd_list, (uint8_t *)&fd);
	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_INTRANODE_PID_INFO *) (((char *)node)
						   - (((char *)&(((DTM_INTRANODE_PID_INFO *) 0)->fd_node))
						      - ((char *)((DTM_INTRANODE_PID_INFO *) 0))));
	}
	return node;
}
/*********************************************************

  Function NAME: dtm_intranode_get_subscr_node

  DESCRIPTION: function to get the subscr node

  ARGUMENTS: server_type

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_SVC_SUBSCR_INFO *dtm_intranode_get_subscr_node(uint32_t server_type)
{
	DTM_SVC_SUBSCR_INFO *node = NULL;
	node =
	    (DTM_SVC_SUBSCR_INFO *) ncs_patricia_tree_get(&dtm_intranode_cb->dtm_svc_subscr_list, (uint8_t *)&server_type);
	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_SVC_SUBSCR_INFO *) (((char *)node)
						- (((char *)&(((DTM_SVC_SUBSCR_INFO *) 0)->svc_subscr_node))
						   - ((char *)((DTM_SVC_SUBSCR_INFO *) 0))));
	}
	return node;
}

/*********************************************************

  Function NAME: dtm_intranode_get_svc_node

  DESCRIPTION: function to get the svc node

  ARGUMENTS: server_type

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static DTM_SVC_INSTALL_INFO *dtm_intranode_get_svc_node(uint32_t server_type)
{
	DTM_SVC_INSTALL_INFO *node = NULL;
	node =
	    (DTM_SVC_INSTALL_INFO *) ncs_patricia_tree_get(&dtm_intranode_cb->dtm_svc_install_list,
							   (uint8_t *)&server_type);
	/* Adjust the pointer */
	if (NULL != node) {
		node = (DTM_SVC_INSTALL_INFO *) (((char *)node)
						 - (((char *)&(((DTM_SVC_INSTALL_INFO *) 0)->svc_install_node))
						    - ((char *)((DTM_SVC_INSTALL_INFO *) 0))));
	}
	return node;
}
