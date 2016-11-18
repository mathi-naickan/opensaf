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

  This include file contains the message definitions for AvND and AvND
  communication. This will use most of the data structures of AvA to AvND 
  communication.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMF_ND2NDMSG_H_
#define AMF_AMF_ND2NDMSG_H_

#include "amf/amf_n2avamsg.h"

/* Message format versions */
#define AVSV_AVND_AVND_MSG_FMT_VER_1    1

#ifdef __cplusplus
extern "C" {
#endif
    
typedef AVSV_NDA_AVA_MSG AVSV_ND2ND_AVA_MSG;

/* AvND-AvND event type */
typedef enum avnd_evnt_evt_type {
	AVND_AVND_AVA_MSG = 1,
	AVND_AVND_CBK_DEL,
	AVND_AVND_EVT_MAX
} AVND_AVND_EVT_TYPE;

typedef struct avsv_nd2nd_cbk_del {
	SaNameT comp_name;
	uint32_t opq_hdl;
} AVSV_ND2ND_CBK_DEL;

typedef struct avsv_nd2nd_avnd_msg {
	/* mds_ctxt and component name will be used for sending resp back 
	   to AvA from AvND. */
	AVND_AVND_EVT_TYPE type;
	MDS_SYNC_SND_CTXT mds_ctxt;
	SaNameT comp_name;	/* Used to wait for registration validation
				   resp from AvND of proxied comp. */
	union {
		AVSV_ND2ND_AVA_MSG *msg;
		AVSV_ND2ND_CBK_DEL cbk_del;
	} info;

} AVSV_ND2ND_AVND_MSG;

void avsv_nd2nd_avnd_msg_free(AVSV_ND2ND_AVND_MSG *msg);
uint32_t avsv_ndnd_avnd_msg_copy(AVSV_ND2ND_AVND_MSG *dmsg, AVSV_ND2ND_AVND_MSG *smsg);
uint32_t avsv_edp_ndnd_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#ifdef __cplusplus
}
#endif
#endif  // AMF_AMF_ND2NDMSG_H_
