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

/*****************************************************************************
..............................................................................

  DESCRIPTION:  MDS DT headers

******************************************************************************
*/
#ifndef _MDS_DT_TCP_H
#define _MDS_DT_TCP_H

#include "mds_dt_tcp_disc.h"


#define MDTM_TCP_POLL_TIMEOUT 20000
#define MDS_TCP_PREFIX 0x56000000

typedef struct mdtm_tcp_cb {
	int DBSRsock;

	void *mdtm_hdle_task;
	uint64_t adest;
	char adest_details[MDS_MAX_PROCESS_NAME_LEN]; /* <node[slotno]:processname[pid]> */

	SYSF_MBX tmr_mbx;
	int tmr_fd;
	uint32_t node_id;
	/* Added for message reception */
	uint16_t bytes_tb_read;
	uint16_t buff_total_len;
	uint8_t len_buff[2];
	uint8_t num_by_read_for_len_buff;
	uint8_t *buffer;

} MDTM_TCP_CB;

MDTM_TCP_CB *tcp_cb;
NCS_PATRICIA_TREE mdtm_reassembly_list;

typedef enum mds_mdtm_dtm_msg_types {
	MDS_MDTM_DTM_PID_TYPE = 1,
	MDS_MDTM_DTM_BIND_TYPE,
	MDS_MDTM_DTM_UNBIND_TYPE,
	MDS_MDTM_DTM_SUBSCRIBE_TYPE,
	MDS_MDTM_DTM_UNSUBSCRIBE_TYPE,
	MDS_MDTM_DTM_NODE_SUBSCRIBE_TYPE,
	MDS_MDTM_DTM_NODE_UNSUBSCRIBE_TYPE,
	MDS_MDTM_DTM_MESSAGE_TYPE
} MDS_MDTM_DTM_MSG_TYPE;


typedef struct mds_mdtm_processid_msg {
	NODE_ID node_id;
	uint32_t process_id;
} MDS_MDTM_PROCESSID_MSG;



typedef struct mds_mdtm_dtm_msg {

	uint16_t size;
	uint32_t mds_indentifire;
	uint8_t mds_version;
	MDS_MDTM_DTM_MSG_TYPE type;
	
	union {
		MDS_MDTM_PROCESSID_MSG pid;
		MDS_MDTM_BIND_MSG bind;
		MDS_MDTM_UNBIND_MSG unbind;
		MDS_MDTM_SUBSCRIBE_MSG subscribe;
		MDS_MDTM_UNSUBSCRIBE_MSG unsubscribe;
		MDS_MDTM_NODE_SUBSCRIBE_MSG node_subscribe;	
		MDS_MDTM_NODE_UNSUBSCRIBE_MSG node_unsubscribe;	
	} info;
} MDS_MDTM_DTM_MSG;




uint32_t mds_mdtm_init_tcp(NODE_ID nodeid, uint32_t *mds_tipc_ref);
uint32_t mds_mdtm_destroy_tcp(void);
uint32_t mds_sock_send(uint8_t *tcp_buffer, uint32_t bufflen); 

#endif
