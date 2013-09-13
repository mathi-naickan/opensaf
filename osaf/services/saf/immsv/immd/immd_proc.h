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
 * Author(s): Ericsson AB
 *
 */

#ifndef IMMD_PROC_H
#define IMMD_PROC_H

/* IMMD definitions */
#define IMMD_MBCSV_MAX_MSG_CNT  10

/* Event Handler */
void immd_process_evt(void);

uint32_t immd_amf_init(IMMD_CB *immd_cb);
uint32_t immd_evt_proc_fevs_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo, bool dealocate);
int immd_proc_elect_coord(IMMD_CB *immd_cb, bool new_active);
void immd_proc_rebroadcast_fevs(IMMD_CB *immd_cb, uint16_t back_count);

/* AMF Function Declerations */

uint32_t immd_process_immnd_down(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node, bool active);

void immd_pending_payload_discards(IMMD_CB *cb);

void immd_cb_dump(void);

uint32_t immd_mbcsv_chgrole(IMMD_CB *cb);

uint32_t immd_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg);
uint32_t immd_get_slot_and_subslot_id_from_mds_dest(MDS_DEST dest);
uint32_t immd_get_slot_and_subslot_id_from_node_id(NCS_NODE_ID node_id);
uint32_t immd_mbcsv_register(IMMD_CB *cb);
uint32_t immd_mbcsv_finalize(IMMD_CB *cb);
uint32_t immd_mbcsv_close(IMMD_CB *cb);
uint32_t immd_mbcsv_dispatch(IMMD_CB *cb);

void immd_db_save_fevs(IMMD_CB *cb, IMMSV_FEVS *fevs_msg);
IMMSV_FEVS *immd_db_get_fevs(IMMD_CB *cb, const uint16_t back_count);
void immd_db_purge_fevs(IMMD_CB *cb);
void immd_db_purg_fevs(IMMD_CB *cb);

#endif
