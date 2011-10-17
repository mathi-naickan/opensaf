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

EXTERN_C uns32 immd_amf_init(IMMD_CB *immd_cb);
EXTERN_C uns32 immd_evt_proc_fevs_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo, NCS_BOOL dealocate);
EXTERN_C int immd_proc_elect_coord(IMMD_CB *immd_cb, NCS_BOOL new_active);
void immd_proc_rebroadcast_fevs(IMMD_CB *immd_cb, uns16 back_count);

/* AMF Function Declerations */

EXTERN_C uns32 immd_process_immnd_down(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node, NCS_BOOL active);

EXTERN_C void immd_cb_dump(void);

EXTERN_C uns32 immd_mbcsv_chgrole(IMMD_CB *cb);

EXTERN_C uns32 immd_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg);
EXTERN_C uns32 immd_get_slot_and_subslot_id_from_mds_dest(MDS_DEST dest);
EXTERN_C uns32 immd_get_slot_and_subslot_id_from_node_id(NCS_NODE_ID node_id);
EXTERN_C uns32 immd_mbcsv_register(IMMD_CB *cb);
EXTERN_C uns32 immd_mbcsv_finalize(IMMD_CB *cb);
EXTERN_C uns32 immd_mbcsv_close(IMMD_CB *cb);
EXTERN_C uns32 immd_mbcsv_dispatch(IMMD_CB *cb);

EXTERN_C void immd_db_save_fevs(IMMD_CB *cb, IMMSV_FEVS *fevs_msg);
EXTERN_C IMMSV_FEVS *immd_db_get_fevs(IMMD_CB *cb, const uns16 back_count);
EXTERN_C void immd_db_purge_fevs(IMMD_CB *cb);
EXTERN_C void immd_db_purg_fevs(IMMD_CB *cb);

#endif
