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
#ifndef DTM_DTMND_DTM_INTER_TRANS_H_
#define DTM_DTMND_DTM_INTER_TRANS_H_

extern uint32_t dtm_add_to_msg_dist_list(uint8_t *buffer, uint16_t len, NODE_ID node_id);

extern uint32_t dtm_internode_snd_msg_to_all_nodes(uint8_t *buffer, uint16_t len);

extern uint32_t dtm_internode_snd_msg_to_node(uint8_t *buffer, uint16_t len, NODE_ID node_id);
extern uint32_t dtm_internode_process_pollout(int fd);
extern uint32_t dtm_prepare_data_msg(uint8_t *buffer, uint16_t len);

#endif  // DTM_DTMND_DTM_INTER_TRANS_H_
