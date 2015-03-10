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

#ifndef DTM_H
#define DTM_H

/* From /base/common/inc */

#include <logtrace.h>
#include <poll.h>
#include "usrbuf.h"
#include "ncsencdec_pub.h"
#include "ncs_mda_papi.h"
#include "ncs_edu_pub.h"
#include <net/if.h>
#include "dtm_cb.h"

extern DTM_INTERNODE_CB *dtms_gl_cb;
extern uint8_t initial_discovery_phase;

#define BMCAST_MSG_LEN ( sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t)

#define m_NODE_DISCOVERY_TASKNAME       "NODE_DISCOVERY"
#define m_NODE_DISCOVERY_STACKSIZE       NCS_STACKSIZE_HUGE

/* The default value of SO_RCVBUF & SO_SNDBUF , it is set by the rmem_default/wmem_default  */
#define DTM_MAX_TAG_LEN	256

typedef enum {
	DTM_MBX_UP_TYPE = 1,
	DTM_MBX_DOWN_TYPE = 2,
	DTM_MBX_NODE_UP_TYPE = 3,
	DTM_MBX_NODE_DOWN_TYPE = 4,
	DTM_MBX_MSG_TYPE = 5,
} MBX_POST_TYPES;

typedef struct dtm_rcv_msg_elem {
	void *next;
	MBX_POST_TYPES type;
	uint8_t pri;
	union {
		struct {
			uint16_t len;
			uint8_t *buffer;
			uint32_t dst_pid;
		} data;

		struct {
			uint16_t len;
			uint8_t *buffer;
			NODE_ID node_id;
		} svc_event;

		struct {
			char node_name[255];
			NODE_ID node_id;
			SYSF_MBX mbx;
			DTM_IP_ADDR_TYPE i_addr_family; /* Indicates V4 or V6 */
			char node_ip[INET6_ADDRSTRLEN];

		} node;
	} info;

} DTM_RCV_MSG_ELEM;

typedef enum {
	DTM_MBX_ADD_DISTR_TYPE = 1,
	DTM_MBX_DEL_DISTR_TYPE = 2,
	DTM_MBX_DATA_MSG_TYPE = 3,
} MBX_SND_POST_TYPES;

typedef struct dtm_snd_msg_elem {
	void *next;
	MBX_SND_POST_TYPES type;
	uint8_t pri;
	union {
		struct {
			uint32_t server_type;
			uint32_t server_inst;
			uint32_t pid;
		} svc_event;
		struct {
			NODE_ID dst_nodeid;
			uint16_t buff_len;
			uint8_t *buffer;
		} data;
	} info;
} DTM_SND_MSG_ELEM;

extern void node_discovery_process(void *arg);
extern uint32_t dtm_cb_init(DTM_INTERNODE_CB * dtms_cb);
extern DTM_NODE_DB *dtm_node_get_by_id(uint32_t nodeid);
extern DTM_NODE_DB *dtm_node_getnext_by_id(uint32_t node_id);
extern DTM_NODE_DB *dtm_node_get_by_comm_socket(uint32_t comm_socket);
extern DTM_NODE_DB *dtm_node_getnext_by_comm_socket(uint32_t comm_socket);
extern DTM_NODE_DB *dtm_node_get_by_node_ip(uint8_t *node_ip);
extern DTM_NODE_DB *dtm_node_getnext_by_nodeaddr(uint8_t *node_ip);
extern uint32_t dtm_node_add(DTM_NODE_DB * node, int i);
extern uint32_t dtm_node_delete(DTM_NODE_DB * nnode, int i);
extern DTM_NODE_DB *dtm_node_new(DTM_NODE_DB * new_node);
extern int dtm_read_config(DTM_INTERNODE_CB * config, char *dtm_config_file);
uint32_t dtm_service_discovery_init(DTM_INTERNODE_CB *dtms_cb);

#endif
