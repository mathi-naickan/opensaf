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

  This module is the include file for Availability Director for message 
  processing module.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFD_MSG_H_
#define AMF_AMFD_MSG_H_

#include "amf/amf_d2nmsg.h"
#include "role.h"

typedef enum {
	AVD_D2D_CHANGE_ROLE_REQ = AVSV_D2D_CHANGE_ROLE_REQ,
	AVD_D2D_CHANGE_ROLE_RSP,
	AVD_D2D_MSG_MAX,
} AVD_D2D_MSG_TYPE;

typedef AVSV_DND_MSG AVD_DND_MSG;
#define AVD_DND_MSG_NULL ((AVD_DND_MSG *)0)
#define AVD_D2D_MSG_NULL ((AVD_D2D_MSG *)0)

/* Message structure used by AVD for communication between
 * the active and standby AVD.
 */
typedef struct avd_d2d_msg {
	AVD_D2D_MSG_TYPE msg_type;
	union {
		struct {
			AVD_ROLE_CHG_CAUSE_T cause;
			SaAmfHAStateT role;
		} d2d_chg_role_req;
		struct {
			SaAmfHAStateT role;
			uint32_t status ; 
		} d2d_chg_role_rsp;
	} msg_info;
} AVD_D2D_MSG;

struct cl_cb_tag;
class AVD_AVND;
class AVD_SU;
struct avd_su_si_rel_tag;
class AVD_COMP;
struct avd_comp_csi_rel_tag;
class AVD_CSI;

uint32_t avd_d2n_msg_dequeue(struct cl_cb_tag *cb);
uint32_t avd_d2n_msg_snd(struct cl_cb_tag *cb, AVD_AVND *nd_node, AVD_DND_MSG *snd_msg);
uint32_t avd_n2d_msg_rcv(AVD_DND_MSG *rcv_msg, NODE_ID node_id, uint16_t msg_fmt_ver);
uint32_t avd_mds_cpy(MDS_CALLBACK_COPY_INFO *cpy_info);
uint32_t avd_mds_enc(MDS_CALLBACK_ENC_INFO *enc_info);
uint32_t avd_mds_enc_flat(MDS_CALLBACK_ENC_FLAT_INFO *enc_info);
uint32_t avd_mds_dec(MDS_CALLBACK_DEC_INFO *dec_info);
uint32_t avd_mds_dec_flat(MDS_CALLBACK_DEC_FLAT_INFO *dec_info);
uint32_t avd_d2n_msg_bcast(struct cl_cb_tag *cb, AVD_DND_MSG *bcast_msg);

void avsv_d2d_msg_free(AVD_D2D_MSG *);
void avd_mds_d_enc(MDS_CALLBACK_ENC_INFO *);
void avd_mds_d_dec(MDS_CALLBACK_DEC_INFO *);
uint32_t avd_d2d_msg_snd(struct cl_cb_tag *, AVD_D2D_MSG *);
extern uint32_t avd_d2d_msg_rcv(AVD_D2D_MSG *rcv_msg);

#endif  // AMF_AMFD_MSG_H_
