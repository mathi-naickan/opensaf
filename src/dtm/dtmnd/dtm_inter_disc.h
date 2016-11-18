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

#ifndef DTM_DTMND_DTM_INTER_DISC_H_
#define DTM_DTMND_DTM_INTER_DISC_H_

/*iden-4 , ver-1, type-1, num_elements-2, type-4, inst-4, pid-4 */

#define DTM_UP_MSG_SIZE         20
#define DTM_DOWN_MSG_SIZE       DTM_UP_MSG_SIZE

#define DTM_UP_MSG_SIZE_FULL (DTM_UP_MSG_SIZE + 2)
#define DTM_DOWN_MSG_SIZE_FULL DTM_UP_MSG_SIZE_FULL

typedef struct dtm_svc_data {
  struct dtm_svc_data *next;
  uint32_t type;
  uint32_t inst;
  uint32_t pid;
} DTM_SVC_DATA;

typedef struct dtm_svc_distribution_list {
  DTM_SVC_DATA *data_ptr_hdr;
  DTM_SVC_DATA *data_ptr_tail;
  uint16_t num_elem;
} DTM_SVC_DISTRIBUTION_LIST;

extern DTM_SVC_DISTRIBUTION_LIST *dtm_svc_dist_list;

typedef struct dtm_up_msg {
  uint32_t type;
  uint32_t inst;
  uint32_t process_id;
} DTM_UP_MSG;

typedef DTM_UP_MSG DTM_DOWN_MSG;

uint32_t dtm_del_from_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid);
uint32_t dtm_add_to_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid);


uint32_t dtm_internode_add_to_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid);
uint32_t dtm_internode_del_from_svc_dist_list(uint32_t server_type, uint32_t server_inst, uint32_t pid);


#endif  // DTM_DTMND_DTM_INTER_DISC_H_
