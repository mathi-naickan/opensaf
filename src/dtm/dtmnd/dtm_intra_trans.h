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
#ifndef DTM_DTMND_DTM_INTRA_TRANS_H_
#define DTM_DTMND_DTM_INTRA_TRANS_H_

uint32_t dtm_intranode_process_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len);
uint32_t dtm_process_rcv_internode_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len);

uint32_t dtm_intranode_send_msg(uint16_t len, uint8_t *buffer, DTM_INTRANODE_PID_INFO * pid_node);

uint32_t dtm_intranode_process_rcv_data_msg(uint8_t *buffer, uint32_t dst_pid, uint16_t len);

uint32_t dtm_intranode_process_pollout(int fd);

uint32_t dtm_intranode_set_poll_fdlist(int fd, uint16_t events);

#endif  // DTM_DTMND_DTM_INTRA_TRANS_H_
