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
#include "dtm_socket.h"
#include "dtm_node.h"
#include "dtm_inter.h"
#include "dtm_inter_disc.h"
#include "dtm_inter_trans.h"

#define MAX_FD 103
#define DTM_TCP_POLL_TIMEOUT 20000
#define DTM_INTERNODE_RECV_BUFFER_SIZE 1024

/* packet_size +  mds_indentifier + mds_version +  msg_type  +node_id + node_name_len + node_name */
#define NODE_INFO_HDR_SIZE 16

#define NODE_INFO_PKT_SIZE (NODE_INFO_HDR_SIZE + MAX_NAME_LENGTH)

static struct pollfd fds[MAX_FD];	/* Poll fds global list */
static int nfds = 0;

/**
 * Function to construct the node info hdr
 *
 * @param dtms_cb buf_ptr pack_size 
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_construct_node_info_hdr(DTM_INTERNODE_CB * dtms_cb, uint8_t *buf_ptr, int *pack_size)
{
	/* Add the FRAG HDR to the Buffer */
	uint8_t *data;
	data = buf_ptr;

	*pack_size = NODE_INFO_HDR_SIZE + strlen(dtms_cb->node_name);

	ncs_encode_16bit(&data, (uint16_t)(*pack_size - 2));	/*pkt_type  */
	ncs_encode_32bit(&data, (uint32_t)(DTM_INTERNODE_SND_MSG_IDENTIFIER));
	ncs_encode_8bit(&data, (uint8_t)DTM_INTERNODE_SND_MSG_VER);
	ncs_encode_8bit(&data, (uint8_t)DTM_CONN_DETAILS_MSG_TYPE);
	ncs_encode_32bit(&data, dtms_cb->node_id);
	ncs_encode_32bit(&data, strlen(dtms_cb->node_name));
	ncs_encode_octets(&data, (uint8_t *)dtms_cb->node_name, (uint8_t)strlen(dtms_cb->node_name));

	return NCSCC_RC_SUCCESS;

}

/**
 * Function to process the node info hdr
 *
 * @param dtms_cb stream_sock buffer node_info_hrd buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_process_node_info(DTM_INTERNODE_CB * dtms_cb, int stream_sock, uint8_t *buffer, uint8_t *node_info_hrd,
			    int buffer_len)
{
	uint32_t node_id;
	DTM_NODE_DB *node;
	uint32_t nodename_len;
	char nodename[MAX_NAME_LENGTH];
	int rc = 0;
	uint8_t *data = buffer;
	TRACE_ENTER();

	node_id = ncs_decode_32bit(&data);
	nodename_len = ncs_decode_32bit(&data);
	strncpy((char *)nodename, (char *)data, nodename_len);

	node = dtm_node_get_by_comm_socket(stream_sock);

	if (node == NULL) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (!node->comm_status) {

		/*****************************************************/
		/* nodeinfo data back to the client  NODE is still */
		/*****************************************************/

		if (node->node_id == 0) {
			node->node_id = node_id;
			strncpy((char *)&node->node_name, nodename, nodename_len);
			node->comm_status = true;
			if (dtm_node_add(node, 0) != NCSCC_RC_SUCCESS) {
				assert(0);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

		} else if (node->node_id == node_id) {
			strncpy((char *)&node->node_name, nodename, nodename_len);
			rc = dtm_comm_socket_send(stream_sock, node_info_hrd, buffer_len);
			if (rc != NCSCC_RC_SUCCESS) {

				LOG_ER("DTM: dtm_comm_socket_send() failed rc : %d", rc);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
			node->comm_status = true;

		} else
			assert(0);

		rc = dtm_process_node_up_down(node->node_id, node->node_name, node->comm_status);

		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM: dtm_process_node_up_down() failed rc : %d ", rc);
			rc = NCSCC_RC_FAILURE;
		}
	} else {
		LOG_ER(" conn details msg recd when conn_status is true");
		assert(0);
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Function to add self node
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t add_self_node(DTM_INTERNODE_CB * dtms_cb)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	DTM_NODE_DB tmp_node;
	DTM_NODE_DB *node;

	memset(&tmp_node, 0, sizeof(DTM_NODE_DB));

	tmp_node.cluster_id = dtms_cb->cluster_id;
	tmp_node.node_id = dtms_cb->node_id;
	memcpy(tmp_node.node_ip, (uint8_t *)dtms_cb->ip_addr, INET6_ADDRSTRLEN);

	strncpy(tmp_node.node_name, dtms_cb->node_name, strlen(dtms_cb->node_name));
	tmp_node.comm_status = true;
	tmp_node.comm_socket = 0;

	node = dtm_node_new(&tmp_node);

	if (node == NULL) {
		LOG_ER("DTM:  dtm_node_new failed .node_ip : %s ", tmp_node.node_ip);
		goto node_fail;
	}

	rc = dtm_node_add(node, 0);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: dtm_node_add fail  rc : %d node->node_id : %d node->node_ip : %s", rc, node->node_id,
		       node->node_ip);
		rc = NCSCC_RC_FAILURE;
		free(node);
		goto done;
	}

	rc = dtm_node_add(node, 1);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: dtm_node_add fail  rc : %d node->node_id : %d node->node_ip : %s", rc, node->node_id,
		       node->node_ip);
		if (dtm_node_delete(node, 0) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed ");
			rc = NCSCC_RC_FAILURE;
			free(node);
			goto done;
		}

	}

	rc = dtm_node_add(node, 2);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: dtm_node_add fail  rc : %d node->node_id : %d node->node_ip : %s", rc, node->node_id,
		       node->node_ip);
		if (dtm_node_delete(node, 0) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed ");
		}
		if (dtm_node_delete(node, 1) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed");
		}
		free(node);
		rc = NCSCC_RC_FAILURE;
	}

 done:
 node_fail:
	return rc;

}

/**
 * Function to take dump of datagram
 *
 * @param buff len max
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void datagram_buff_dump(uint8_t *buff, uint32_t len, uint32_t max)
{
	/* TBD */
}

/**
 * Function to process node up and down
 *
 * @param node_id node_name comm_status
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_process_node_up_down(NODE_ID node_id, char *node_name, uint8_t comm_status)
{
	if (true == comm_status) {
		dtm_node_up(node_id, node_name, 0);
	} else {
		dtm_node_down(node_id, node_name, 0);
	}
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to process internode poll and rcv message
 *
 * @param node local_len_buf node_info_hrd node_info_buffer_len close_conn fd
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void dtm_internode_process_poll_rcv_msg_common(DTM_NODE_DB * node, uint16_t local_len_buf, uint8_t *node_info_hrd,
					       uint16_t node_info_buffer_len, int fd, int *close_conn)
{
	DTM_MSG_TYPES pkt_type = 0;
	uint32_t identifier = 0;
	uint8_t version = 0;
	uint8_t *data = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;

	node->buffer[local_len_buf + 2] = '\0';
	data = &node->buffer[2];
	/* Decode the message */
	identifier = ncs_decode_32bit(&data);
	version = ncs_decode_8bit(&data);

	if ((DTM_INTERNODE_RCV_MSG_IDENTIFIER != identifier) || (DTM_INTERNODE_RCV_MSG_VER != version)) {
		LOG_ER("DTM: Malformed packet recd, Ident : %d, ver : %d", identifier, version);
		goto done;
	}

	pkt_type = ncs_decode_8bit(&data);

	if (pkt_type == DTM_UP_MSG_TYPE) {
		uint8_t *alloc_buffer = NULL;

		if (NULL == (alloc_buffer = calloc(1, (local_len_buf - 6)))) {
			LOG_ER("\nMemory allocation failed in dtm_internode_processing");
			goto done;
		}
		memcpy(alloc_buffer, &node->buffer[8], (local_len_buf - 6));

		dtm_internode_process_rcv_up_msg(alloc_buffer, (local_len_buf - 6), node->node_id);
	} else if (pkt_type == DTM_CONN_DETAILS_MSG_TYPE) {
		if (dtm_process_node_info(dtms_cb, fd, &node->buffer[8], node_info_hrd, node_info_buffer_len) !=
		    NCSCC_RC_SUCCESS) {
			LOG_ER(" DTM : communication socket Connection closed\n");
			*close_conn = true;
		}
	} else if (pkt_type == DTM_DOWN_MSG_TYPE) {
		uint8_t *alloc_buffer = NULL;

		if (NULL == (alloc_buffer = calloc(1, (local_len_buf - 6)))) {
			LOG_ER("\nMemory allocation failed in dtm_internode_processing");
			goto done;
		}
		memcpy(alloc_buffer, &node->buffer[8], (local_len_buf - 6));
		dtm_internode_process_rcv_down_msg(alloc_buffer, (local_len_buf - 6), node->node_id);
	} else if (pkt_type == DTM_MESSAGE_MSG_TYPE) {
		NODE_ID dst_nodeid = 0;
		uint32_t dst_processid = 0;
		dst_nodeid = ncs_decode_32bit(&data);
		if (dtms_cb->node_id != dst_nodeid)
			LOG_ER("Invalid dest_nodeid: %u received in dtm_internode_processing", dst_nodeid);
		dst_processid = ncs_decode_32bit(&data);
		dtm_internode_process_rcv_data_msg(node->buffer, dst_processid, (local_len_buf + 2));
		node->bytes_tb_read = 0;
		node->buff_total_len = 0;
		node->num_by_read_for_len_buff = 0;
		return;
	}
 done:
	node->bytes_tb_read = 0;
	node->buff_total_len = 0;
	node->num_by_read_for_len_buff = 0;
	free(node->buffer);
	node->buffer = NULL;
	return;
}

/**
 * Function to process internode poll and rcv message
 *
 * @param fd close_conn node_info_hrd node_info_buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void dtm_internode_process_poll_rcv_msg(int fd, int *close_conn, uint8_t *node_info_hrd, uint16_t node_info_buffer_len)
{
	DTM_NODE_DB *node = NULL;
	TRACE_ENTER();

	node = dtm_node_get_by_comm_socket(fd);

	if (NULL == node) {
		LOG_ER("DTM: database mismatch");
		assert(0);
	}
	if (0 == node->bytes_tb_read) {
		if (0 == node->num_by_read_for_len_buff) {
			uint8_t *data;
			int recd_bytes = 0;

			/*******************************************************/
			/* Receive all incoming data on this socket */
			/*******************************************************/

			recd_bytes = recv(fd, node->len_buff, 2, 0);
			if (0 == recd_bytes) {
				*close_conn = true;
				return;
			} else if (2 == recd_bytes) {
				uint16_t local_len_buf = 0;

				data = node->len_buff;
				local_len_buf = ncs_decode_16bit(&data);
				node->buff_total_len = local_len_buf;
				node->num_by_read_for_len_buff = 2;

				if (NULL == (node->buffer = calloc(1, (local_len_buf + 3)))) {
					/* Length + 2 is done to reuse the same buffer 
					   while sending to other nodes */
					LOG_ER("\nMemory allocation failed in dtm_internode_processing");
					return;
				}
				recd_bytes = recv(fd, &node->buffer[2], local_len_buf, 0);

				if (recd_bytes < 0) {
					return;
				} else if (0 == recd_bytes) {
					*close_conn = true;
					return;
				} else if (local_len_buf > recd_bytes) {
					/* can happen only in two cases, system call interrupt or half data, */
					TRACE("DTM: less data recd, recd bytes : %d, actual len : %d", recd_bytes,
					       local_len_buf);
					node->bytes_tb_read = node->buff_total_len - recd_bytes;
					return;
				} else if (local_len_buf == recd_bytes) {
					/* Call the common rcv function */
					dtm_internode_process_poll_rcv_msg_common(node, local_len_buf, node_info_hrd,
										  node_info_buffer_len, fd, close_conn);
				} else {
					assert(0);
				}
			} else {
				/* we had recd some bytes */
				if (recd_bytes < 0) {
					/* This can happen due to system call interrupt */
					return;
				} else if (1 == recd_bytes) {
					/* We recd one byte of the length part */
					node->num_by_read_for_len_buff = recd_bytes;
				} else {
					assert(0);
				}
			}
		} else if (1 == node->num_by_read_for_len_buff) {
			int recd_bytes = 0;

			recd_bytes = recv(fd, &node->len_buff[1], 1, 0);
			if (recd_bytes < 0) {
				/* This can happen due to system call interrupt */
				return;
			} else if (1 == recd_bytes) {
				/* We recd one byte(remaining) of the length part */
				uint8_t *data = node->len_buff;
				node->num_by_read_for_len_buff = 2;
				node->buff_total_len = ncs_decode_16bit(&data);
				return;
			} else if (0 == recd_bytes) {
				*close_conn = true;
				return;
			} else {
				assert(0);	/* This should never occur */
			}
		} else if (2 == node->num_by_read_for_len_buff) {
			int recd_bytes = 0;

			if (NULL == (node->buffer = calloc(1, (node->buff_total_len + 3)))) {
				/* Length + 2 is done to reuse the same buffer 
				   while sending to other nodes */
				LOG_ER("\nMemory allocation failed in dtm_internode_processing");
				return;
			}
			recd_bytes = recv(fd, &node->buffer[2], node->buff_total_len, 0);

			if (recd_bytes < 0) {
				return;
			} else if (0 == recd_bytes) {
				*close_conn = true;
				return;
			} else if (node->buff_total_len > recd_bytes) {
				/* can happen only in two cases, system call interrupt or half data, */
				TRACE("DTM: less data recd, recd bytes : %d, actual len : %d", recd_bytes,
				       node->buff_total_len);
				node->bytes_tb_read = node->buff_total_len - recd_bytes;
				return;
			} else if (node->buff_total_len == recd_bytes) {
				/* Call the common rcv function */
				dtm_internode_process_poll_rcv_msg_common(node, node->buff_total_len, node_info_hrd,
									  node_info_buffer_len, fd, close_conn);
			} else {
				assert(0);
			}
		} else {
			assert(0);
		}

	} else {
		/* Partial data already read */
		int recd_bytes = 0;

		recd_bytes =
		    recv(fd, &node->buffer[2 + (node->buff_total_len - node->bytes_tb_read)], node->bytes_tb_read, 0);

		if (recd_bytes < 0) {
			return;
		} else if (0 == recd_bytes) {
			*close_conn = true;
			return;
		} else if (node->bytes_tb_read > recd_bytes) {
			/* can happen only in two cases, system call interrupt or half data, */
			TRACE("DTM: less data recd, recd bytes : %d, actual len : %d", recd_bytes, node->bytes_tb_read);
			node->bytes_tb_read = node->bytes_tb_read - recd_bytes;
			return;
		} else if (node->bytes_tb_read == recd_bytes) {
			/* Call the common rcv function */
			dtm_internode_process_poll_rcv_msg_common(node, node->buff_total_len, node_info_hrd,
								  node_info_buffer_len, fd, close_conn);
		} else {
			assert(0);
		}
	}
	TRACE_LEAVE();
	return;
}

/**
 * Function to handle node discovery process
 *
 * @param args
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void node_discovery_process(void *arg)
{
	TRACE_ENTER();

	int poll_ret = 0;
	int end_server = false, compress_array = false;
	int close_conn = false;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;

	int current_size = 0, i, j;

	/* Data Received */
	uint8_t inbuf[DTM_INTERNODE_RECV_BUFFER_SIZE];
	uint8_t *data1;		/* Used for DATAGRAM decoding */
	uint16_t recd_bytes = 0;
	uint16_t recd_buf_len = 0;
	int node_info_buffer_len = 0;
	uint8_t node_info_hrd[NODE_INFO_PKT_SIZE];
	char node_ip[INET6_ADDRSTRLEN];

	memset(&node_ip, 0, INET6_ADDRSTRLEN);
	/*************************************************************/
	/* Set up the initial bcast or mcast receiver socket */
	/*************************************************************/

	if (dtms_cb->mcast_flag != true) {

		if (NCSCC_RC_SUCCESS != dtm_dgram_bcast_listener(dtms_cb)) {
			LOG_ER("DTM:Set up the initial bcast  receiver socket   failed");
			exit(1);
		}

	} else {

		if (NCSCC_RC_SUCCESS != dtm_dgram_mcast_listener(dtms_cb)) {
			LOG_ER("DTM:Set up the initial mcast  receiver socket   failed");
			exit(1);
		}
	}

	/*************************************************************/
	/* Set up the initial listening socket */
	/*************************************************************/
	if (NCSCC_RC_SUCCESS != dtm_stream_nonblocking_listener(dtms_cb)) {
		LOG_ER("DTM: Set up the initial stream nonblocking serv  failed");
		exit(1);
	}
#if 0

	if (add_self_node(dtms_cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: add_self_node  failed");
		exit(1);
	}
#endif

	/*************************************************************/
	/* Initialize the pollfd structure */
	/*************************************************************/
	memset(fds, 0, sizeof(fds));

	/*************************************************************/
	/* Set up the initial listening socket */
	/*************************************************************/

	fds[0].fd = dtms_cb->dgram_sock_rcvr;
	fds[0].events = POLLIN;

	/*************************************************************/
	/* Set up the initial listening socket */
	/*************************************************************/

	fds[1].fd = dtms_cb->stream_sock;
	fds[1].events = POLLIN;

	fds[2].fd = dtms_cb->mbx_fd;
	fds[2].events = POLLIN;
	nfds = 3;

	/*************************************************************/
	/* Set up the initial listening socket */
	/*************************************************************/

	if (dtm_construct_node_info_hdr(dtms_cb, node_info_hrd, &node_info_buffer_len) != NCSCC_RC_SUCCESS) {

		LOG_ER("DTM: dtm_construct_node_info_hdr failed"); 
		goto done;

	}

	/*************************************************************/
	/* Loop waiting for incoming connects or for incoming data */
	/* on any of the connected sockets. */
	/*************************************************************/

	do {
		/***********************************************************/
		/* Call poll() and wait . */
		/***********************************************************/
		int fd_check = 0;
		poll_ret = poll(fds, nfds, DTM_TCP_POLL_TIMEOUT);
		/***********************************************************/

		/* Check to see if the poll call failed. */
		/***********************************************************/
		if (poll_ret < 0) {
			LOG_ER(" poll() failed");
			continue;
		}
		/***********************************************************/
		/* Check to see if the 3 minute time out expired. */
		/***********************************************************/
		if (poll_ret == 0) {
			continue;
		}

		/***********************************************************/
		/* One or more descriptors are readable. Need to */
		/* determine which ones they are. */
		/***********************************************************/
		current_size = nfds;
		for (i = 0; i < current_size; i++) {

			/*********************************************************/
			/* Loop through to find the descriptors that returned */
			/* POLLIN and determine whether it’s the listening */
			/* or the active connection. */
			/*********************************************************/
			if (POLLIN & fds[i].revents) {

				if (fds[i].fd == dtms_cb->dgram_sock_rcvr) {

					fd_check++;
					/* Data Received */
					memset(inbuf, 0, DTM_INTERNODE_RECV_BUFFER_SIZE);
					recd_bytes = 0;
					recd_buf_len = 0;

					recd_bytes = dtm_dgram_recvfrom_bmcast(dtms_cb, node_ip, inbuf, sizeof(inbuf));

					if (recd_bytes == 0) {
						LOG_ER("DTM: recd bytes=0 on DGRAM sock");
						continue;
					}

					data1 = inbuf;	/* take care of previous address */

					recd_buf_len = ncs_decode_16bit(&data1);

					if (recd_buf_len == recd_bytes) {

						int new_sd = -1;

						new_sd = dtm_process_connect(dtms_cb, node_ip, inbuf, (recd_bytes - 2));

						if (new_sd == -1)
							continue;

					/*****************************************************/
						/* Add the new incoming connection to the */
						/* pollfd structure */
					/*****************************************************/
						LOG_IN("DTM: add New incoming connection to fd : %d\n", new_sd);
						fds[nfds].fd = new_sd;
						fds[nfds].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
						nfds++;

					} else {
						/* Log message that we are dropping the data */
						LOG_ER("DTM: BRoadcastLEN-MISMATCH: dropping the data");
					}

				} else if (fds[i].fd == dtms_cb->stream_sock) {

					int new_sd = -1;
					uint32_t local_rc = NCSCC_RC_SUCCESS;
					fd_check++;
				/*******************************************************/
					/* Listening descriptor is readable. */
				/*******************************************************/
					TRACE(" DTM :Listening socket is readable");
				/*******************************************************/
					/* Accept all incoming connections that are */
					/* queued up on the listening socket before we */
					/* loop back and call poll again. */
				/*******************************************************/
					/* do { */
				/*****************************************************/
					/* Accept each incoming connection. If */
					/* accept fails with EWOULDBLOCK, then we */
					/* have accepted all of them. Any other */
					/* failure on accept will cause us to end the */
					/* serv. */
				/*****************************************************/
					new_sd = dtm_process_accept(dtms_cb, dtms_cb->stream_sock);
					if (new_sd < 0) {
						LOG_ER("DTM: accept() failed");
						end_server = true;
						break;
					}

		

				/*****************************************************/
					/* Node info data back to the accept with node info  */
				/*****************************************************/

					local_rc = dtm_comm_socket_send(new_sd, node_info_hrd, node_info_buffer_len);
					if (local_rc != NCSCC_RC_SUCCESS) {
						dtm_comm_socket_close(&new_sd);
						LOG_ER("DTM: send() failed ");
						break;
					}
					
				/*****************************************************/
					/* Add the new incoming connection to the */
					/* pollfd structure */
				/*****************************************************/
					TRACE("DTM :add New incoming connection to fd : %d\n", new_sd);
					fds[nfds].fd = new_sd;
					fds[nfds].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
					nfds++;

				/*****************************************************/
					/* Loop back up and accept another incoming */
					/* connection */
				/*****************************************************/
					/* } while (new_sd != -1); */	/* accept one at a time */

				} else if (fds[i].fd == dtms_cb->mbx_fd) {
					/* MBX fd messages that need to be sent out from this node */
					/* Process the mailbox events */
					DTM_SND_MSG_ELEM *msg_elem = NULL;

					fd_check++;
					msg_elem =
					    (DTM_SND_MSG_ELEM *) (m_NCS_IPC_NON_BLK_RECEIVE(&dtms_cb->mbx, NULL));

					if (NULL == msg_elem) {
						LOG_ER("DTM: Inter Node Mailbox IPC_NON_BLK_RECEIVE Failed");
						continue;
					} else if (DTM_MBX_ADD_DISTR_TYPE == msg_elem->type) {
						dtm_internode_add_to_svc_dist_list(msg_elem->info.svc_event.server_type,
										   msg_elem->info.svc_event.server_inst,
										   msg_elem->info.svc_event.pid);
						free(msg_elem);
						msg_elem = NULL;
					} else if (DTM_MBX_DEL_DISTR_TYPE == msg_elem->type) {
						dtm_internode_del_from_svc_dist_list(msg_elem->info.
										     svc_event.server_type,
										     msg_elem->info.
										     svc_event.server_inst,
										     msg_elem->info.svc_event.pid);
						free(msg_elem);
						msg_elem = NULL;
					} else if (DTM_MBX_DATA_MSG_TYPE == msg_elem->type) {
						dtm_prepare_data_msg(msg_elem->info.data.buffer,
								     msg_elem->info.data.buff_len);
						dtm_internode_snd_msg_to_node(msg_elem->info.data.buffer,
									      msg_elem->info.data.buff_len,
									      msg_elem->info.data.dst_nodeid);
						free(msg_elem);
						msg_elem = NULL;
					} else {
						LOG_ER("DTM Intranode :Invalid evt type from mbx");
						free(msg_elem);
						msg_elem = NULL;
					}
				} else {

			/*********************************************************/
					/* This is not the listening socket, therefore an */
					/* existing connection must be readable */
			/*********************************************************/
					fd_check++;
					dtm_internode_process_poll_rcv_msg(fds[i].fd, &close_conn, node_info_hrd,
									   node_info_buffer_len);

				}
			} else if (fds[i].revents & POLLOUT) {
				fd_check++;
				dtm_internode_process_pollout(fds[i].fd);
			} else if (fds[i].revents & POLLHUP) {
				fd_check++;
				close_conn = true;
			}

		/*******************************************************/
			/* If the close_conn flag was turned on, we need */
			/* to clean up this active connection. This */
			/* clean up process includes removing the */
			/* descriptor. */
		/*******************************************************/
			if (close_conn) {
				dtm_comm_socket_close(&fds[i].fd);
				close_conn = false;
				compress_array = true;
			}
			/* End of existing connection is readable */
			if (poll_ret == fd_check) {
				break;
			}
		}

		/***********************************************************/
		/* If the compress_array flag was turned on, we need */
		/* to squeeze together the array and decrement the number */
		/* of file descriptors. We do not need to move back the */
		/* events and revents fields because the events will always */
		/* be POLLIN in this case, and revents is output. */
		/***********************************************************/

		if (compress_array) {
			compress_array = false;
			for (i = 0; i < nfds; i++) {
				if (fds[i].fd == -1) {
					for (j = i; j < nfds; j++) {
						fds[j].fd = fds[j + 1].fd;
					}
					nfds--;
				}
			}
		}

	} while (end_server == false);

	/* End of serving running. */
	/*************************************************************/
	/* Clean up all of the sockets that are open */
	/*************************************************************/
 done:
	for (i = 0; i < nfds; i++) {
		if (fds[i].fd >= 0)
			dtm_comm_socket_close(&fds[i].fd);
	}
	TRACE_LEAVE();
	return;
}

/**
 * Function to set poll fdlist
 *
 * @param fd events
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_set_poll_fdlist(int fd, uint16_t events)
{
	int i = 0;

	for (i = 0; i < nfds; i++) {
		if (fd == fds[i].fd) {
			fds[i].events = fds[i].events | events;
			LOG_IN("event set success, in the poll fd list");
			return NCSCC_RC_SUCCESS;
		}
	}
	LOG_ER("Unable to set the event in the poll list");
	return NCSCC_RC_FAILURE;
}

/**
 * Function to reset poll fdlist
 *
 * @param fd
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_internode_reset_poll_fdlist(int fd)
{
	int i = 0;
	for (i = 0; i < nfds; i++) {
		if (fd == fds[i].fd) {
			fds[i].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
			LOG_IN("event set success, in the poll fd list");
			return NCSCC_RC_SUCCESS;
		}
	}
	LOG_ER("\nUnable to set the event in the poll list");
	return NCSCC_RC_FAILURE;
}
