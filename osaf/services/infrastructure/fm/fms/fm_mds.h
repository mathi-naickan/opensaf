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

#ifndef FM_MDS_H
#define FM_MDS_H

#define FM_MDS_SUB_PART_VERSION 1

#define FM_SUBPART_VER_MIN      1
#define FM_SUBPART_VER_MAX      1

#define FM_FM_MSG_FMT_VER_1     1

#define m_MSG_FMT_VER_GET(rem_ver,my_ver_min,my_ver_max,msg_fmt_array) \
(((rem_ver) < (my_ver_min))?0:\
(((rem_ver) >= (my_ver_max))?((msg_fmt_array)[(my_ver_max) - (my_ver_min)]):\
((msg_fmt_array)[(rem_ver) - (my_ver_min)])))

#define m_MSG_FORMAT_IS_VALID(                                    \
msg_fmt_version,rem_ver_min,rem_ver_max,msg_fmt_array) \
(((msg_fmt_version) > (msg_fmt_array)[(rem_ver_max) - (rem_ver_min)])?\
0:(((msg_fmt_version) < (msg_fmt_array)[0])?0:1))

uint32_t fm_mds_init(FM_CB *cb);

uint32_t fm_mds_finalize(FM_CB *cb);

uint32_t fm_mds_sync_send(FM_CB *fm_cb, NCSCONTEXT msg,
				NCSMDS_SVC_ID svc_id,
				MDS_SEND_PRIORITY_TYPE priority,
				MDS_SENDTYPES send_type, MDS_DEST *i_to_dest, MDS_SYNC_SND_CTXT *mds_ctxt);

uint32_t fm_mds_async_send(FM_CB *fm_cb, NCSCONTEXT msg,
				 NCSMDS_SVC_ID svc_id,
				 MDS_SEND_PRIORITY_TYPE priority,
				 MDS_SENDTYPES send_type, MDS_DEST i_to_dest, NCSMDS_SCOPE_TYPE bcast_scope);

extern uint32_t fm_send_node_down_to_mbx(FM_CB *cb, uint32_t node_id);
#endif
