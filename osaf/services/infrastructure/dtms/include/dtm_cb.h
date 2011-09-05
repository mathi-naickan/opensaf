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

#ifndef DTM_CB_H
#define DTM_CB_H

#define MAX_NAME_LENGTH 256
#define MAX_PORT_LENGTH 256

typedef enum dtm_ip_addr_type {
	DTM_IP_ADDR_TYPE_NONE,
	DTM_IP_ADDR_TYPE_IPV4 = AF_INET,
	DTM_IP_ADDR_TYPE_IPV6 = AF_INET6,
	DTM_IP_ADDR_TYPE_MAX	/* Must be last. */
} DTM_IP_ADDR_TYPE;

typedef struct dtm_internode_unsent_msgs {
	struct dtm_internode_unsent_msgs *next;
	uns16 len;
	uns8 *buffer;
} DTM_INTERNODE_UNSENT_MSGS;

/* Node structure */
typedef struct node_list {
	uns16 cluster_id;
	NODE_ID node_id;
	char node_name[MAX_NAME_LENGTH];
	char node_ip[INET6_ADDRSTRLEN];
	int comm_socket;
	NCS_PATRICIA_NODE pat_nodeid;
	NCS_PATRICIA_NODE pat_comm_socket;
	NCS_PATRICIA_NODE pat_ip_address;
	uns8 comm_status;
	NCS_LOCK node_lock;
	SYSF_MBX mbx;
	int mbx_fd;
	/*<SERVICE> */
	DTM_INTERNODE_UNSENT_MSGS *msgs_hdr;
	DTM_INTERNODE_UNSENT_MSGS *msgs_tail;
	/* Message related */
	uns16 bytes_tb_read;
	uns16 buff_total_len;
	uns8 len_buff[2];
	uns8 num_by_read_for_len_buff;
	uns8 *buffer;
} DTM_NODE_DB;

char remoteIP[INET6_ADDRSTRLEN];

/*  control block */
typedef struct dtm_internode_cb {
	uns16 cluster_id;
	NODE_ID node_id;	/* Self  Node Id  */
	char node_name[MAX_NAME_LENGTH];	/* optional */
	char ip_addr[INET6_ADDRSTRLEN];	/* ipv4 ipv6 addrBuffer */
	char mcast_addr[INET6_ADDRSTRLEN];	/* ipv4 ipv6 addrBuffer */
	char bcast_addr[INET6_ADDRSTRLEN];
	char   ifname[IFNAMSIZ]; /* ipv6mr_interface to */
	in_port_t stream_port;
	in_port_t dgram_port_sndr;
	in_port_t dgram_port_rcvr;
	int stream_sock;	/*  */
	int dgram_sock_sndr;	/*  */
	int dgram_sock_rcvr;	/*  */
	DTM_IP_ADDR_TYPE i_addr_family;	/* Indicates V4 or V6 */
	uns8 mcast_flag;	/* Indicates mcast */
	int32 initial_dis_timeout;
	int32 bcast_msg_freq;
	NCS_PATRICIA_TREE nodeid_tree;	/* NODE_DB information of Nodes */
	NCS_PATRICIA_TREE comm_sock_tree;	/* NODE_DB information of Nodes */
	NCS_PATRICIA_TREE ip_addr_tree;	/* NODE_DB information of Nodes */
	int so_keepalive;
	NCS_LOCK cb_lock;
	int comm_keepidle_time;
	int comm_keepalive_intvl;
	int comm_keepalive_probes;
	SYSF_MBX mbx;
	int mbx_fd;
} DTM_INTERNODE_CB;

 /*extern DTM_INTERNODE_CB *dtms_gl_cb; */

typedef struct dtm_intranode_pollfd_list {
	struct dtm_intranode_pollfd_list *next;
	struct pollfd dtm_intranode_fd;
} DTM_INTRANODE_POLLFD_LIST;

typedef struct dtm_intranode_cb {
	int server_sockfd;
	NODE_ID nodeid;
	int task_hdl;
	void *dtm_intranode_hdl_task;
	DTM_INTRANODE_POLLFD_LIST *fd_list_ptr_head;
	DTM_INTRANODE_POLLFD_LIST *fd_list_ptr_tail;
	MDS_DEST adest;
	NCS_PATRICIA_TREE dtm_intranode_pid_list;	/* Tree of pid info */
	NCS_PATRICIA_TREE dtm_intranode_fd_list;	/* Tree of fd info */
	NCS_PATRICIA_TREE dtm_svc_subscr_list;	/* Tree of service subscription info */
	NCS_PATRICIA_TREE dtm_svc_install_list;	/* Tree of service install info */
	SYSF_MBX mbx;
	int mbx_fd;
	int sock_domain;
} DTM_INTRANODE_CB;

extern DTM_INTRANODE_CB *dtm_intranode_cb;

#endif
