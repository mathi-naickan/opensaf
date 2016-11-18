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
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/**************************************************************************

Description: This function contains the PLMS routines for MDS interaction

****************************************************************************/
#include "base/ncsgl_defs.h"
#include "plm/plms.h"
#include "plm/agent/plma.h"
#include "base/ncs_util.h"
#include "plm/plms_evt.h"

MDS_CLIENT_MSG_FORMAT_VER
 PLMS_WRT_PLMA_MSG_FMT_ARRAY[PLMS_WRT_PLMA_SUBPART_VER_RANGE] = {
	                  1 /*msg format version for PLMA subpart version 1 */
};
MDS_CLIENT_MSG_FORMAT_VER
 PLMA_WRT_PLMS_MSG_FMT_ARRAY[PLMA_WRT_PLMS_SUBPART_VER_RANGE] = {
	                  1 /*msg format version for PLMA subpart version 1 */
};

/****************** Function Prototypes ***********************************/
SaUint32T plms_mds_enc(MDS_CALLBACK_ENC_INFO *info, EDU_HDL *edu_hdl);
SaUint32T plms_mds_dec(MDS_CALLBACK_DEC_INFO *info, EDU_HDL *edu_hdl);
SaUint32T plms_mds_enc_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl);
SaUint32T plms_mds_dec_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl);

/****************************************************************************
  Name          : plms_mds_enc

  Description   : This function encodes an events sent from PLMS.

  Arguments     : cb    : PLMS control Block.
                  info  : Info for encoding

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None.
******************************************************************************/
SaUint32T plms_mds_enc(MDS_CALLBACK_ENC_INFO *enc_info, EDU_HDL * edu_hdl)
{
	PLMS_EVT *evt;
	NCS_UBAID *uba;
	EDU_ERR        ederror = 0;
	SaUint32T rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	/* Get the Msg Format version from the SERVICE_ID &
	   RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_PLMA) {
		enc_info->o_msg_fmt_ver =
	            m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
			      PLMS_WRT_PLMA_SUBPART_VER_MIN,
			      PLMS_WRT_PLMA_SUBPART_VER_MAX,
			      PLMS_WRT_PLMA_MSG_FMT_ARRAY);
	}
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_PLMS) {
		enc_info->o_msg_fmt_ver =
	            m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
			      PLMA_WRT_PLMS_SUBPART_VER_MIN,
			      PLMA_WRT_PLMS_SUBPART_VER_MAX,
			      PLMA_WRT_PLMS_MSG_FMT_ARRAY);
	}
	
	if (0 == enc_info->o_msg_fmt_ver  ) {  /*Does not work. */
		LOG_ER("INVALID MSG FORMAT IN ENC");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE; 
	}

	evt = (PLMS_EVT *)enc_info->i_msg;
	uba = enc_info->io_uba;

	if (uba == NULL)
	{
		LOG_ER("MDS Error. uba is NULL ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Call the Wrapper enc function which encods the msg to be sent */
	rc = m_NCS_EDU_EXEC(edu_hdl, plms_edp_plms_evt, enc_info->io_uba,EDP_OP_TYPE_ENC, evt, &ederror);
	
	if(rc != NCSCC_RC_SUCCESS)
	{
		/* Free calls to be added here. */
		m_NCS_EDU_PRINT_ERROR_STRING(ederror);
		LOG_ER("MDS encode failed");
		return rc;
	}
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   plms_mds_dec

  DESCRIPTION   :
  	This function decodes a buffer containing an PLMA events that was recd 

  ARGUMENTS:
	  dec_info  : Info for decoding

  RETURNS       : NCSCC_RC_SUCCESS/Error Code.

  NOTES:

*****************************************************************************/
SaUint32T plms_mds_dec (MDS_CALLBACK_DEC_INFO *dec_info, EDU_HDL *edu_hdl)
{
	EDU_ERR   ederror = 0;
	PLMS_EVT  *evt;
	SaUint32T     rc = NCSCC_RC_SUCCESS;

	if( 0 == m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
				PLMS_WRT_PLMA_SUBPART_VER_MIN,
  				PLMS_WRT_PLMA_SUBPART_VER_MAX,
				PLMS_WRT_PLMA_MSG_FMT_ARRAY))
	{
		LOG_ER("MDS Decode failure. version mismatch ");
		return NCSCC_RC_FAILURE;
	}
	evt= (PLMS_EVT *)malloc(sizeof(PLMS_EVT));
	if (evt == NULL)
	{
		LOG_ER("Mem Alloc failed in MDS dec function");
		return NCSCC_RC_FAILURE;
	}
	
	memset(evt,0,sizeof(PLMS_EVT));
	dec_info->o_msg = (NCSCONTEXT)evt;

	rc = m_NCS_EDU_EXEC(edu_hdl, plms_edp_plms_evt, dec_info->io_uba,
		EDP_OP_TYPE_DEC, (PLMS_EVT**)&dec_info->o_msg, &ederror);

	if(rc != NCSCC_RC_SUCCESS)
	{
		free(evt);
		m_NCS_EDU_PRINT_ERROR_STRING(ederror);
		LOG_ER("MDS decode failed");
		return rc;
	}
	return rc;
}

/****************************************************************************
 * Name          : plms_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

SaUint32T plms_mds_enc_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl)
{
	SaUint32T rc = NCSCC_RC_SUCCESS;

	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO * enc = &info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
/*	info->info.enc = enc; */
	/* Invoke the regular mds_enc routine */
	rc = plms_mds_enc(enc, edu_hdl);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS Flat encode failed");
	return rc;
}

/****************************************************************************
 * Name          : plms_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

SaUint32T plms_mds_dec_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl)
{
	SaUint32T rc = NCSCC_RC_SUCCESS;

	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO * dec = &info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	/* info->info.dec = dec;*/
	/* Invoke the regular mds_dec routine */
	rc = plms_mds_dec(dec,edu_hdl);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS Flat Decode Failed");
	return rc;
}
	     
/****************************************************************************
 * Name          : plms_mds_normal_send
 *
 * Description   : This is the function which is used to send the message
 *                 using MDS.
 *
 * Arguments     : mds_hdl  - MDS handle
 *                 from_svc - From Serivce ID.
 *                 evt      - Event to be sent.
 *                 dest     - MDS destination to be sent to.
 *                 to_svc   - To Service ID.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
SaUint32T
plms_mds_normal_send (MDS_HDL mds_hdl,
                     NCSMDS_SVC_ID from_svc,
                     NCSCONTEXT evt, MDS_DEST dest,
                     NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO   info;
	SaUint32T         res;

	memset(&info, 0, sizeof(info));

	info.i_mds_hdl  = mds_hdl;
	info.i_op       = MDS_SEND;
	info.i_svc_id   = from_svc;

	info.info.svc_send.i_msg = evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	info.info.svc_send.i_to_svc   = to_svc;

	memset(&info.info.svc_send.info.snd.i_to_dest, 0, sizeof(MDS_DEST));
	info.info.svc_send.info.snd.i_to_dest = dest;

	res = ncsmds_api(&info);
	return(res);
}

/****************************************************************************
 * Name          : plms_mds_scoped_send
 *
 * Description   : This is the function which is used to send the message
 *                 using MDS only with the scope which we want to.
 *
 * Arguments     : mds_hdl  - MDS handle
 *                 scope    - Scope to be sent
 *                            NCSMDS_SCOPE_INTRANODE/NCSMDS_SCOPE_INTRAPCON
 *                 from_svc - From Serivce ID.
 *                 evt      - Event to be sent.
 *                 to_svc   - To Service ID.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
SaUint32T
plms_mds_scoped_send (MDS_HDL mds_hdl,
			NCSMDS_SCOPE_TYPE scope,
			NCSMDS_SVC_ID from_svc,
			NCSCONTEXT evt,
			NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO   info;
	SaUint32T         res;

	memset(&info, 0, sizeof(info));

	info.i_mds_hdl  = mds_hdl;
	info.i_op       = MDS_SEND;
	info.i_svc_id   = from_svc;

	info.info.svc_send.i_msg = evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc   = to_svc;

	/* There is some message context in this broadcast structure - TBD */
	info.info.svc_send.info.bcast.i_bcast_scope = scope;

	res = ncsmds_api(&info);
	return(res);
}
		
/****************************************************************************
 * Name          : plms_mds_bcast_send
 *
 * Description   : This is the function which is used to send the message
 *                 using MDS broadcast.
 *
 * Arguments     : mds_hdl  - MDS handle
 *                 from_svc - From Serivce ID.
 *                 evt      - Event to be sent.
 *                 to_svc   - To Service ID.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
SaUint32T
plms_mds_bcast_send (MDS_HDL mds_hdl,
			NCSMDS_SVC_ID from_svc,
			NCSCONTEXT evt,
			NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO   info;
	SaUint32T         res;

	memset(&info, 0, sizeof(info));

	info.i_mds_hdl  = mds_hdl;
	info.i_op       = MDS_SEND;
	info.i_svc_id   = from_svc;

	info.info.svc_send.i_msg = evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc   = to_svc;

	/* There is some message context in this broadcast structure - TBD */
	info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_NONE;

	res = ncsmds_api(&info);
	return(res);
}


/****************************************************************************
  Name          : plm_mds_msg_sync_send

  Description   : This routine sends the mds sync msgs from/to PLM/PLMS

  Arguments     : i_evt - ptr to the message to be sent
                  o_evt - ptr to the response msg
                  timeout - timeout value in 10 ms

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/

SaUint32T plm_mds_msg_sync_send (MDS_HDL mds_hdl,
                              uint32_t       from_svc,
                              uint32_t       to_svc,
                              MDS_DEST    to_dest,
                              PLMS_EVT     *i_evt,
                              PLMS_EVT     **o_evt,
                              SaTimeT       timeout)
{


   NCSMDS_INFO                mds_info;
   SaUint32T                      rc;

   if(!i_evt)
      return NCSCC_RC_FAILURE;

   memset(&mds_info, 0, sizeof(NCSMDS_INFO));
   mds_info.i_mds_hdl = mds_hdl;
   mds_info.i_svc_id = from_svc;
   mds_info.i_op = MDS_SEND;

   /* fill the send structure */
   mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
   mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
   mds_info.info.svc_send.i_to_svc = to_svc;
   mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

   /* fill the send rsp strcuture */
   mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout; /* timeto wait in 10ms */
   mds_info.info.svc_send.info.sndrsp.i_to_dest = to_dest;

   /* send the message */
   rc = ncsmds_api(&mds_info);
   if ( rc == NCSCC_RC_SUCCESS)
      *o_evt = mds_info.info.svc_send.info.sndrsp.o_rsp;

   return rc;
}


/****************************************************************************
 * Name          : plm_send_mds_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     :
 *
 * Return Values :
 *
 * Notes         :
 *****************************************************************************/

SaUint32T plm_send_mds_rsp(MDS_HDL mds_hdl,
                        NCSMDS_SVC_ID  from_svc,
                        PLMS_SEND_INFO *s_info,
                        PLMS_EVT       *evt)
{
   NCSMDS_INFO                mds_info;
   SaUint32T                      rc = NCSCC_RC_SUCCESS;

   if(s_info->stype != MDS_SENDTYPE_RSP)
      return rc;

   memset(&mds_info, 0, sizeof(NCSMDS_INFO));
   mds_info.i_mds_hdl = mds_hdl;
   mds_info.i_svc_id = from_svc;
   mds_info.i_op = MDS_SEND;

   /* fill the send structure */
   mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
   mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;

   mds_info.info.svc_send.i_to_svc = s_info->to_svc;
   mds_info.info.svc_send.i_sendtype = s_info->stype;
   mds_info.info.svc_send.info.rsp.i_msg_ctxt = s_info->ctxt;
   mds_info.info.svc_send.info.rsp.i_sender_dest = s_info->dest;

   /* send the message */
   rc = ncsmds_api(&mds_info);

   return rc;
}

