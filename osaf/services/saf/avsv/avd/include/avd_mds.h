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
  AvD-MDS interaction related definitions.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_MDS_H
#define AVD_MDS_H

/* In Service upgrade support */
#define AVD_MDS_SUB_PART_VERSION_4 4
#define AVD_MDS_SUB_PART_VERSION   5

#define AVD_AVND_SUBPART_VER_MIN   1
#define AVD_AVND_SUBPART_VER_MAX   5

#define AVD_AVD_SUBPART_VER_MIN    1
#define AVD_AVD_SUBPART_VER_MAX    5

/* Message format versions */
#define AVD_AVD_MSG_FMT_VER_1    1
#define AVD_AVD_MSG_FMT_VER_2    2
#define AVD_AVD_MSG_FMT_VER_3    3
#define AVD_AVD_MSG_FMT_VER_4    4
#define AVD_AVD_MSG_FMT_VER_5    5

uint32_t avd_mds_set_vdest_role(struct cl_cb_tag *cb, SaAmfHAStateT role);
uint32_t avd_mds_init(struct cl_cb_tag *cb);
void avd_mds_unreg(struct cl_cb_tag *cb);

uint32_t avd_mds_cbk(struct ncsmds_callback_info *info);
uint32_t avd_avnd_mds_send(struct cl_cb_tag *cb, AVD_AVND *nd_node, AVD_DND_MSG *snd_msg);
extern void avd_mds_avd_up_evh(AVD_CL_CB *cb, AVD_EVT *evt);
extern void avd_mds_avd_down_evh(AVD_CL_CB *cb, AVD_EVT *evt);
extern void avd_standby_avd_down_evh(AVD_CL_CB *cb, AVD_EVT *evt);
extern uint32_t avd_mds_send(MDS_SVC_ID i_to_svc, MDS_DEST i_to_dest, NCSCONTEXT i_msg);

#endif
