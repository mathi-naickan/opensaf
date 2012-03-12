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

  This include file contains the message definitions for AvA and AvND
  communication.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVSV_N2AVAMSG_H
#define AVSV_N2AVAMSG_H

/* Message format versions */
#define AVSV_AVND_AVA_MSG_FMT_VER_1    1

/* AMF API enums */
typedef enum avsv_nda_ava_msg_type {
	AVSV_AVA_API_MSG = 1,
	AVSV_AVND_AMF_CBK_MSG,
	AVSV_AVND_AMF_API_RESP_MSG,
	AVSV_NDA_AVA_MSG_MAX
} AVSV_NDA_AVA_MSG_TYPE;

/* API param definition */
typedef struct avsv_amf_api_info {
	MDS_DEST dest;		/* mds dest */
	AVSV_AMF_API_TYPE type;	/* api type */
	union {
		AVSV_AMF_FINALIZE_PARAM finalize;
		AVSV_AMF_COMP_REG_PARAM reg;
		AVSV_AMF_COMP_UNREG_PARAM unreg;
		AVSV_AMF_PM_START_PARAM pm_start;
		AVSV_AMF_PM_STOP_PARAM pm_stop;
		AVSV_AMF_HC_START_PARAM hc_start;
		AVSV_AMF_HC_STOP_PARAM hc_stop;
		AVSV_AMF_HC_CONFIRM_PARAM hc_confirm;
		AVSV_AMF_CSI_QUIESCING_COMPL_PARAM csiq_compl;
		AVSV_AMF_HA_GET_PARAM ha_get;
		AVSV_AMF_PG_START_PARAM pg_start;
		AVSV_AMF_PG_STOP_PARAM pg_stop;
		AVSV_AMF_ERR_REP_PARAM err_rep;
		AVSV_AMF_ERR_CLEAR_PARAM err_clear;
		AVSV_AMF_RESP_PARAM resp;
	} param;
} AVSV_AMF_API_INFO;

/* API response definition */
typedef struct avsv_amf_api_resp_info {
	AVSV_AMF_API_TYPE type;	/* api type */
	SaAisErrorT rc;		/* return code */
	union {
		AVSV_AMF_HA_GET_PARAM ha_get;
	} param;
} AVSV_AMF_API_RESP_INFO;

/* message used for AvND-AvA interaction */
typedef struct avsv_nda_ava_msg {
	AVSV_NDA_AVA_MSG_TYPE type;	/* message type */

	union {
		/* elements encoded by AvA (& decoded by AvND) */
		AVSV_AMF_API_INFO api_info;	/* api info */

		/* elements encoded by AvND (& decoded by AvA) */
		AVSV_AMF_CBK_INFO *cbk_info;	/* callbk info */
		AVSV_AMF_API_RESP_INFO api_resp_info;	/* api response info */
	} info;
} AVSV_NDA_AVA_MSG;

/* Extern Function Prototypes */

void avsv_nda_ava_msg_free(AVSV_NDA_AVA_MSG *);
void avsv_nda_ava_msg_content_free(AVSV_NDA_AVA_MSG *);

uint32_t avsv_nda_ava_msg_copy(AVSV_NDA_AVA_MSG *, AVSV_NDA_AVA_MSG *);

uint32_t avsv_amf_cbk_copy(AVSV_AMF_CBK_INFO **, AVSV_AMF_CBK_INFO *);
uint32_t avsv_amf_csi_attr_list_copy(SaAmfCSIAttributeListT *, const SaAmfCSIAttributeListT *);
void avsv_amf_cbk_free(AVSV_AMF_CBK_INFO *);
void avsv_amf_csi_attr_list_free(SaAmfCSIAttributeListT *);

uint32_t avsv_amf_csi_attr_convert(AVSV_AMF_CBK_INFO *);

#endif   /* !AVSV_N2AVAMSG_H */
