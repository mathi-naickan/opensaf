/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:
  
  MND Restart Checkpoint Data Structure definition.
    
******************************************************************************
*/

#ifndef MSG_MSGND_MQND_RESTART_H_
#define MSG_MSGND_MQND_RESTART_H_

uint32_t mqnd_restart_init(MQND_CB *cb);
uint32_t mqnd_add_node_to_mqalist(MQND_CB *cb, MDS_DEST dest);
uint32_t mqnd_ckpt_queue_info_write(MQND_CB *cb, MQND_QUEUE_CKPT_INFO *queue_ckpt_node, uint32_t index);

void mqnd_cpy_qnodeinfo_to_ckptinfo(MQND_CB *cb, MQND_QUEUE_NODE *queue_info,
					     MQND_QUEUE_CKPT_INFO *ckpt_queue_info);

#endif  // MSG_MSGND_MQND_RESTART_H_
