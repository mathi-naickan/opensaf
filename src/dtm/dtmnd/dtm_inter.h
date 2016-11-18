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

#ifndef DTM_DTMND_DTM_INTER_H_
#define DTM_DTMND_DTM_INTER_H_

#define DTM_INTERNODE_RCV_MSG_IDENTIFIER 0x56123456

#define DTM_INTERNODE_RCV_MSG_VER       1

#define DTM_INTERNODE_SND_MSG_IDENTIFIER 0x56123456

#define DTM_INTERNODE_SND_MSG_VER       1

typedef enum dtm_msg_types {
  DTM_CONN_DETAILS_MSG_TYPE = 1,
  DTM_UP_MSG_TYPE = 2,
  DTM_DOWN_MSG_TYPE = 3,
  DTM_MESSAGE_MSG_TYPE = 4,
} DTM_MSG_TYPES;

extern uint32_t dtm_node_up(NODE_ID node_id, char *node_name, char *node_ip, DTM_IP_ADDR_TYPE i_addr_family, SYSF_MBX mbx);
extern uint32_t dtm_internode_process_rcv_up_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id);
extern uint32_t dtm_internode_process_rcv_down_msg(uint8_t *buffer, uint16_t len, NODE_ID node_id);
extern uint32_t dtm_node_down(NODE_ID node_id, char *node_name, SYSF_MBX mbx);
extern uint32_t dtm_internode_process_rcv_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len);

#endif  // DTM_DTMND_DTM_INTER_H_
