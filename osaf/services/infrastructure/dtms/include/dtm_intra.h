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
#ifndef DTM_INTRA_H
#define DTM_INTRA_H

#include <netinet/tcp.h>

#define DTM_INTRANODE_RCV_MSG_IDENTIFIER 0x56123456

#define DTM_INTRANODE_RCV_MSG_VER	1

#define DTM_INTRANODE_SND_MSG_IDENTIFIER 0x56123456

#define DTM_INTRANODE_SND_MSG_VER	1

typedef enum dtm_intranode_rcv_msg_types {
	DTM_INTRANODE_RCV_PID_TYPE = 1,
	DTM_INTRANODE_RCV_BIND_TYPE = 2,
	DTM_INTRANODE_RCV_UNBIND_TYPE = 3,
	DTM_INTRANODE_RCV_SUBSCRIBE_TYPE = 4,
	DTM_INTRANODE_RCV_UNSUBSCRIBE_TYPE = 5,
	DTM_INTRANODE_RCV_NODE_SUBSCRIBE_TYPE = 6,
	DTM_INTRANODE_RCV_NODE_UNSUBSCRIBE_TYPE = 7,
	DTM_INTRANODE_RCV_MESSAGE_TYPE = 8,
} DTM_INTRANODE_RCV_MSG_TYPES;

typedef enum dtm_lib_types {
	DTM_LIB_UP_TYPE = 1,
	DTM_LIB_DOWN_TYPE = 2,
	DTM_LIB_NODE_UP_TYPE = 3,
	DTM_LIB_NODE_DOWN_TYPE = 4,
	DTM_LIB_MESSAGE_TYPE = 5,
} DTM_LIB_TYPES;

extern uint32_t dtm_intranode_add_self_node_to_node_db(NODE_ID node_id);
uint32_t dtm_intranode_reset_poll_fdlist(int fd);

#endif
