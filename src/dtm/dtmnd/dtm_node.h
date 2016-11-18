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
#ifndef DTM_DTMND_DTM_NODE_H_
#define DTM_DTMND_DTM_NODE_H_

typedef void raw_type;

extern char *dtm_validate_listening_ip_addr(DTM_INTERNODE_CB * config);
extern uint32_t dtm_stream_nonblocking_listener(DTM_INTERNODE_CB * dtms_cb);
extern uint32_t dtm_dgram_mcast_listener(DTM_INTERNODE_CB * dtms_cb);
extern uint32_t dtm_dgram_mcast_sender(DTM_INTERNODE_CB * dtms_cb, int mcast_ttl);
extern uint32_t dtm_dgram_bcast_listener(DTM_INTERNODE_CB * dtms_cb);
extern uint32_t dtm_dgram_bcast_sender(DTM_INTERNODE_CB * dtms_cb);
extern uint32_t dtm_dgram_sendto_bcast(DTM_INTERNODE_CB * dtms_cb, const void *buffer, int buffer_len);
extern uint32_t dtm_dgram_sendto_mcast(DTM_INTERNODE_CB * dtms_cb, const void *buffer, int buffer_len);
extern uint32_t dtm_sockdesc_close(int sock_desc);
extern int dtm_process_connect(DTM_INTERNODE_CB * dtms_cb, char *node_ip, uint8_t *buffer, uint16_t len);
extern int dtm_process_accept(DTM_INTERNODE_CB * dtms_cb, int stream_sock);
extern int dtm_dgram_recvfrom_bmcast(DTM_INTERNODE_CB * dtms_cb, char *node_ip, void *buffer, int buffer_len);
extern uint32_t dtm_comm_socket_send(int sock_desc, const void *buffer, int buffer_len);
extern uint32_t dtm_comm_socket_close(int *comm_socket);
extern uint32_t dtm_process_node_up_down(NODE_ID node_id, char *node_name, char *node_ip, DTM_IP_ADDR_TYPE i_addr_family, uint8_t comm_status);
uint32_t dtm_internode_set_poll_fdlist(int fd, uint16_t event);
uint32_t dtm_internode_reset_poll_fdlist(int fd);

#endif  // DTM_DTMND_DTM_NODE_H_
