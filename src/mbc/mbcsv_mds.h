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




        REVISION HISTORY:


            DESCRIPTION:

              This module contains declarations and structures related to MBCSv.

*******************************************************************************/

/*
 * Module Inclusion Control...
 */
#ifndef MBC_MBCSV_MDS_H_
#define MBC_MBCSV_MDS_H_

#include "mbcsv.h"
#include "base/ncs_util.h"

/*
 * Sync send timeout
 */
#define MBCSV_SYNC_TIMEOUT                 100
/*
 * Message sizes to be encoded and decoded.
 **/
#define MBCSV_MAX_SIZE_DATA                 512
#define MBCSV_MSG_TYPE_SIZE                 (sizeof(uint8_t) + sizeof(uint32_t))
#define MBCSV_MSG_SUB_TYPE                  (sizeof(uint8_t))
#define MBCSV_PEER_UP_MSG_SIZE              sizeof(uint32_t)
#define MBCSV_PEER_DOWN_MSG_SIZE            (2 * sizeof(uint32_t))
#define MBCSV_PEER_INFO_MSG_SIZE            (sizeof(uint8_t) + (2 * sizeof(uint32_t)))
#define MBCSV_PEER_INFO_RSP_MSG_SIZE        (sizeof(uint8_t) + (3 * sizeof(uint32_t)))
#define MBCSV_PEER_CHG_ROLE_MSG_SIZE        (2 * sizeof(uint32_t))
#define MBCSV_INT_CLIENT_MSG_SIZE           ((3 * sizeof(uint8_t)) + (3 * sizeof(uint32_t)))
#define MBCSV_MSG_VER_SIZE                  sizeof(uint16_t)

/* Versioning changes */
#define MBCSV_MDS_SUB_PART_VERSION 1
#define MBCSV_WRT_PEER_SUBPART_VER_MIN 1
#define MBCSV_WRT_PEER_SUBPART_VER_MAX 1
#define MBCSV_WRT_PEER_SUBPART_VER_RANGE        \
  (MBCSV_WRT_PEER_SUBPART_VER_MAX  -            \
   MBCSV_WRT_PEER_SUBPART_VER_MIN  +1)

/*
 * MBCSv MDS function prototypes.
 */
uint32_t mbcsv_mds_reg(uint32_t pwe_hdl, uint32_t svc_hdl, MBCSV_ANCHOR *anchor, MDS_DEST *vdest);
void mbcsv_mds_unreg(uint32_t pwe_hdl);
uint32_t mbcsv_query_mds(uint32_t pwe_hdl, MBCSV_ANCHOR *anchor, MDS_DEST *vdest);
uint32_t mbcsv_mds_send_msg(uint32_t send_type, MBCSV_EVT *msg, CKPT_INST *ckpt, MBCSV_ANCHOR anchor);

uint32_t mbcsv_mds_callback(NCSMDS_CALLBACK_INFO *cbinfo);
uint32_t mbcsv_mds_rcv(NCSMDS_CALLBACK_INFO *cbinfo);
uint32_t mbcsv_mds_evt(MDS_CALLBACK_SVC_EVENT_INFO svc_info, MDS_CLIENT_HDL yr_svc_hdl);
uint32_t mbcsv_mds_enc(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT msg,
                       SS_SVC_ID to_svc, NCS_UBAID *uba,
                       MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver, MDS_CLIENT_MSG_FORMAT_VER *msg_fmat_ver);
uint32_t mbcsv_mds_dec(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT *msg,
                       SS_SVC_ID to_svc, NCS_UBAID *uba, MDS_CLIENT_MSG_FORMAT_VER msg_fmat_ver);
uint32_t mbcsv_mds_cpy(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT msg,
                       SS_SVC_ID to_svc, NCSCONTEXT *cpy,
                       bool last, MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver,
                       MDS_CLIENT_MSG_FORMAT_VER *msg_fmt_ver);
uint32_t mbcsv_encode_version(NCS_UBAID *uba, uint16_t version);
uint32_t mbcsv_decode_version(NCS_UBAID *uba, uint16_t *version);

/*
 * Internally used macros for sending message with different send types.
 */
#define m_NCS_MBCSV_MDS_SYNC_SEND(m, c, a)              \
  mbcsv_mds_send_msg(MDS_SENDTYPE_REDRSP, m, c, a)

#define m_NCS_MBCSV_MDS_ASYNC_SEND(m, c, a)     \
  mbcsv_mds_send_msg(MDS_SENDTYPE_RED, m, c, a)

#define m_NCS_MBCSV_SYNC_MDS_RSP_SEND(m, c, a)          \
  mbcsv_mds_send_msg(MDS_SENDTYPE_RRSP, m, c, a)

#define m_NCS_MBCSV_MDS_BCAST_SEND(p, m, c)     \
  mbcsv_send_brodcast_msg(p, m, c)

#endif  // MBC_MBCSV_MDS_H_
