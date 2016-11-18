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

  DESCRIPTION: This file includes routines for decoding ASAPi messages:

   asapi_msg_dec............ASAPi Message decode routine
   asapi_reg_dec............ASAPi Registration Message Decode routine
   asapi_reg_resp_dec.......ASAPi Registration Response Message Decode routine
   asapi_dereg_dec..........ASAPi De-registration Message Decode routine
   asapi_dereg_resp_dec.....ASAPi De-registration Response Message Decode routine
   asapi_nreslove_dec.......ASAPi Name Resolution Message Decode routine
   asapi_nreslove_resp_dec..ASAPi Name Resolution Response Message Decode routine
   asapi_getqueue_dec.......ASAPi Getqueue Message Decode routine
   asapi_getqueue_resp_dec..ASAPi Getqueue Response Message Decode routine
   asapi_track_dec..........ASAPi Track Message Decode routine
   asapi_track_resp_dec.....ASAPi Track Response Message Decode routine
   asapi_track_ntfy_dec.....ASAPi Track Notifiaction Decode routine
******************************************************************************
*/
#include "msg/mqsv.h"

/******************************** LOCAL ROUTINES *****************************/
static uint32_t asapi_reg_dec(NCS_UBAID *, ASAPi_REG_INFO *);
static uint32_t asapi_reg_resp_dec(NCS_UBAID *, ASAPi_REG_RESP_INFO *);
static uint32_t asapi_dereg_dec(NCS_UBAID *, ASAPi_DEREG_INFO *);
static uint32_t asapi_dereg_resp_dec(NCS_UBAID *, ASAPi_DEREG_RESP_INFO *);
static uint32_t asapi_nreslove_dec(NCS_UBAID *, ASAPi_NRESOLVE_INFO *);
static uint32_t asapi_nreslove_resp_dec(NCS_UBAID *, ASAPi_NRESOLVE_RESP_INFO *);
static uint32_t asapi_getqueue_dec(NCS_UBAID *, ASAPi_GETQUEUE_INFO *);
static uint32_t asapi_getqueue_resp_dec(NCS_UBAID *, ASAPi_GETQUEUE_RESP_INFO *);
static uint32_t asapi_track_dec(NCS_UBAID *, ASAPi_TRACK_INFO *);
static uint32_t asapi_track_resp_dec(NCS_UBAID *, ASAPi_TRACK_RESP_INFO *);
static uint32_t asapi_track_ntfy_dec(NCS_UBAID *, ASAPi_TRACK_NTFY_INFO *);
static uint32_t asapi_ginfo_dec(NCS_UBAID *, SaNameT *, SaMsgQueueGroupPolicyT *);
static uint32_t asapi_qinfo_dec(NCS_UBAID *, ASAPi_QUEUE_PARAM *);
static uint32_t asapi_name_dec(NCS_UBAID *, SaNameT *);
static uint32_t asapi_obj_info_dec(NCS_UBAID *, ASAPi_OBJECT_INFO *, ASAPi_ERR_INFO *);
/*****************************************************************************/

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_dec

   DESCRIPTION    :  This is ASAPi decode routine, Based on the ASAPi message 
                     type it decodes request messages. This routine will be 
                     invoked within the flow of MDS when it deceides to decodes
                     the ASAPi messages.
   ARGUMENTS      :  pBuff  - Data buffer
                     o_pMsg - ASAPi Message   

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
   NOTE           :  This decode routine is common for both, inter process 
                     decoding & inter card decoding.
\****************************************************************************/
uint32_t asapi_msg_dec(NCS_UBAID *pBuff, ASAPi_MSG_INFO **o_pMsg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO *pMsg = 0;

	*o_pMsg = 0;
	pMsg = m_MMGR_ALLOC_ASAPi_MSG_INFO(asapi.my_svc_id);	/* Allocate new MSG envelop */
	if (!pMsg) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* Memmory failure ... */
	}
	memset(pMsg, 0, sizeof(ASAPi_MSG_INFO));

	/* Decode the ASAPi message type */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(uint8_t));
	pMsg->msgtype = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(uint8_t));

	if (ASAPi_MSG_REG == pMsg->msgtype) {
		/* Decode ASAPi Registration message */
		rc = asapi_reg_dec(pBuff, &pMsg->info.reg);
	} else if (ASAPi_MSG_REG_RESP == pMsg->msgtype) {
		/* Decode ASAPi Registration Response message */
		rc = asapi_reg_resp_dec(pBuff, &pMsg->info.rresp);
	} else if (ASAPi_MSG_DEREG == pMsg->msgtype) {
		/* Decode ASAPi De-registration message */
		rc = asapi_dereg_dec(pBuff, &pMsg->info.dereg);
	} else if (ASAPi_MSG_DEREG_RESP == pMsg->msgtype) {
		/* Decode ASAPi De-registration Response message */
		rc = asapi_dereg_resp_dec(pBuff, &pMsg->info.dresp);
	} else if (ASAPi_MSG_NRESOLVE == pMsg->msgtype) {
		/* Decode Name Resolution message */
		rc = asapi_nreslove_dec(pBuff, &pMsg->info.nresolve);
	} else if (ASAPi_MSG_NRESOLVE_RESP == pMsg->msgtype) {
		/* Decode Name Resolution Response message */
		rc = asapi_nreslove_resp_dec(pBuff, &pMsg->info.nresp);
	} else if (ASAPi_MSG_GETQUEUE == pMsg->msgtype) {
		/* Decode ASAPi Getqueue queue message */
		rc = asapi_getqueue_dec(pBuff, &pMsg->info.getqueue);
	} else if (ASAPi_MSG_GETQUEUE_RESP == pMsg->msgtype) {
		/* Decode ASAPi Getqueue Queue Response message */
		rc = asapi_getqueue_resp_dec(pBuff, &pMsg->info.vresp);
	} else if (ASAPi_MSG_TRACK == pMsg->msgtype) {
		/* Decode ASAPi Track message */
		rc = asapi_track_dec(pBuff, &pMsg->info.track);
	} else if (ASAPi_MSG_TRACK_RESP == pMsg->msgtype) {
		/* Decode ASAPi Track Response message */
		rc = asapi_track_resp_dec(pBuff, &pMsg->info.tresp);
	} else if (ASAPi_MSG_TRACK_NTFY == pMsg->msgtype) {
		/* Decode ASAPi Track Notification message */
		rc = asapi_track_ntfy_dec(pBuff, &pMsg->info.tntfy);
	}

	if (NCSCC_RC_SUCCESS != rc) {
		asapi_msg_free(&pMsg);	/* Free the message ... */
		pMsg = 0;
		return m_ASAPi_DBG_SINK(rc);
	}

	*o_pMsg = pMsg;		/* OK--Done */
	return rc;
}	/* End of asapi_msg_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_reg_dec

   DESCRIPTION    :  Routine to decode ASAPi Registration message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Registration Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_reg_dec(NCS_UBAID *pBuff, ASAPi_REG_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64], flag = 0;	/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;
	bool gflag = false;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(flag));
	flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Group Information */
	rc = asapi_ginfo_dec(pBuff, &msg->group, &msg->policy);
	if (NCSCC_RC_SUCCESS == rc) {
		gflag = true;	/* Group present */
	}

	/* Decode Queue Information */
	rc = asapi_qinfo_dec(pBuff, &msg->queue);
	if (NCSCC_RC_SUCCESS != rc) {	/* Queue Information doesn't exist */
		if (gflag) {
			msg->objtype = ASAPi_OBJ_GROUP;
			rc = NCSCC_RC_SUCCESS;
		} else
			return rc;
	} else {
		if (gflag)
			msg->objtype = ASAPi_OBJ_BOTH;
		else
			msg->objtype = ASAPi_OBJ_QUEUE;
	}
	return rc;
}	/* End of asapi_reg_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_reg_resp_dec

   DESCRIPTION    :  Routine to decode ASAPi Registration Response message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Registration Response Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_reg_resp_dec(NCS_UBAID *pBuff, ASAPi_REG_RESP_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;
	bool gflag = false;

	/* Decode the Error flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.flag));
	msg->err.flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->err.flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Group Name & Length */
	rc = asapi_name_dec(pBuff, &msg->group);
	if (NCSCC_RC_SUCCESS == rc) {
		gflag = true;	/* Group present */
	}

	/* Decode Queue Name & Length */
	rc = asapi_name_dec(pBuff, &msg->queue);
	if (NCSCC_RC_SUCCESS != rc) {	/* Queue Information doesn't exist */
		if (gflag) {
			msg->objtype = ASAPi_OBJ_GROUP;
			rc = NCSCC_RC_SUCCESS;
		} else
			return rc;
	} else {
		if (gflag)
			msg->objtype = ASAPi_OBJ_BOTH;
		else
			msg->objtype = ASAPi_OBJ_QUEUE;
	}

	/* Decode Error Information */
	if (msg->err.flag) {
		stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.errcode));
		msg->err.errcode = ncs_decode_32bit(&stream);
		ncs_dec_skip_space(pBuff, sizeof(msg->err.errcode));
	}

	return rc;
}	/* End of asapi_reg_resp_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_dereg_dec

   DESCRIPTION    :  Routine to decode ASAPi De-registration message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - De-registration Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_dereg_dec(NCS_UBAID *pBuff, ASAPi_DEREG_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64], flag = 0;	/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;
	bool gflag = false;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(flag));
	flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Group Name & Length */
	rc = asapi_name_dec(pBuff, &msg->group);
	if (NCSCC_RC_SUCCESS == rc) {
		gflag = true;	/* Group present */
	}

	/* Decode Queue Name & Length */
	rc = asapi_name_dec(pBuff, &msg->queue);
	if (NCSCC_RC_SUCCESS != rc) {
		if (gflag) {
			msg->objtype = ASAPi_OBJ_GROUP;
			rc = NCSCC_RC_SUCCESS;
		} else
			return rc;
	} else {
		if (gflag)
			msg->objtype = ASAPi_OBJ_BOTH;
		else
			msg->objtype = ASAPi_OBJ_QUEUE;
	}
	return rc;
}	/* End of asapi_dereg_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_dereg_resp_dec

   DESCRIPTION    :  Routine to decode ASAPi De-registration Response message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - De-registration Response Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_dereg_resp_dec(NCS_UBAID *pBuff, ASAPi_DEREG_RESP_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;
	bool gflag = false;

	/* Decode the Error flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.flag));
	msg->err.flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->err.flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Group Name & Length */
	rc = asapi_name_dec(pBuff, &msg->group);
	if (NCSCC_RC_SUCCESS == rc) {
		gflag = true;	/* Group present */
	}

	/* Decode Queue Name & Length */
	rc = asapi_name_dec(pBuff, &msg->queue);
	if (NCSCC_RC_SUCCESS != rc) {	/* Queue Information doesn't exist */
		if (gflag) {
			msg->objtype = ASAPi_OBJ_GROUP;
			rc = NCSCC_RC_SUCCESS;
		} else
			return rc;
	} else {
		if (gflag)
			msg->objtype = ASAPi_OBJ_BOTH;
		else
			msg->objtype = ASAPi_OBJ_QUEUE;
	}

	/* Decode Error Information */
	if (msg->err.flag) {
		stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.errcode));
		msg->err.errcode = ncs_decode_32bit(&stream);
		ncs_dec_skip_space(pBuff, sizeof(msg->err.errcode));
	}

	return rc;
}	/* End of asapi_dereg_resp_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_nreslove_dec

   DESCRIPTION    :  Routine to decode ASAPi Name Resolution message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Name Resolution Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_nreslove_dec(NCS_UBAID *pBuff, ASAPi_NRESOLVE_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->track));
	msg->track = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->track));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Object Name & Length */
	rc = asapi_name_dec(pBuff, &msg->object);
	return rc;
}	/* End of asapi_nreslove_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_nreslove_resp_dec

   DESCRIPTION    :  Routine to decode ASAPi Name Resolution Response message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Nmae Resolution Response Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_nreslove_resp_dec(NCS_UBAID *pBuff, ASAPi_NRESOLVE_RESP_INFO *msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = asapi_obj_info_dec(pBuff, &msg->oinfo, &msg->err);
	return rc;
}	/* End of asapi_nreslove_resp_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_getqueue_dec

   DESCRIPTION    :  Routine to decode ASAPi Getqueue message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Getqueue Message                      

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_getqueue_dec(NCS_UBAID *pBuff, ASAPi_GETQUEUE_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64], flag = 0;	/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(flag));
	flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Queue Name & Length */
	rc = asapi_name_dec(pBuff, &msg->queue);
	return rc;
}	/* End of asapi_getqueue_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_getqueue_resp_dec

   DESCRIPTION    :  Routine to decode ASAPi Getqueue Response message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Getqueue Response Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_getqueue_resp_dec(NCS_UBAID *pBuff, ASAPi_GETQUEUE_RESP_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;

	/* Decode the Error flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.flag));
	msg->err.flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->err.flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Queue Information */
	asapi_qinfo_dec(pBuff, &msg->queue);

	/* Decode Error Information */
	if (msg->err.flag) {
		stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->err.errcode));
		msg->err.errcode = ncs_decode_32bit(&stream);
		ncs_dec_skip_space(pBuff, sizeof(msg->err.errcode));
	}

	return rc;
}	/* End of asapi_getqueue_resp_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_dec

   DESCRIPTION    :  Routine to decode ASAPi Track message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Track Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_track_dec(NCS_UBAID *pBuff, ASAPi_TRACK_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->val));
	msg->val = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->val));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Object Name & Length */
	rc = asapi_name_dec(pBuff, &msg->object);

	return rc;
}	/* End of asapi_track_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_resp_dec

   DESCRIPTION    :  Routine to decode ASAPi Track Response message
   ARGUMENTS      :  pBuff - Data buffer
                     msg   - Track Response Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_track_resp_dec(NCS_UBAID *pBuff, ASAPi_TRACK_RESP_INFO *tresp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = asapi_obj_info_dec(pBuff, &tresp->oinfo, &tresp->err);
	return rc;
}	/* End of asapi_track_resp_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_obj_info_dec

   DESCRIPTION    :  Routine to decode ASAPi Object Information & error info
   ARGUMENTS      :  pBuff - Data buffer
                     info  - Object info pointer
                     err   - Error info pointer
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_obj_info_dec(NCS_UBAID *pBuff, ASAPi_OBJECT_INFO *info, ASAPi_ERR_INFO *err)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0, idx = 0;

	/* Decode the Error flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(err->flag));
	err->flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(err->flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode Group Info */
	asapi_ginfo_dec(pBuff, &info->group, &info->policy);

	/* Decode number of Queues */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(info->qcnt));
	info->qcnt = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(info->qcnt));

	/* Allocate Queue Information */
	if (info->qcnt) {
		info->qparam = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(info->qcnt * sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
		if (!info->qparam) {
			return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* OOOppps Memory Failure */
		}
		memset(info->qparam, 0, sizeof(ASAPi_QUEUE_PARAM));

		for (idx = 0; idx < info->qcnt; idx++) {
			/* Decode Queue Information */
			asapi_qinfo_dec(pBuff, &info->qparam[idx]);
		}
	}

	/* Decode Error Information */
	if (err->flag) {
		stream = ncs_dec_flatten_space(pBuff, space, sizeof(err->errcode));
		err->errcode = ncs_decode_32bit(&stream);
		ncs_dec_skip_space(pBuff, sizeof(err->errcode));
	}

	return rc;
}	/* End of asapi_obj_info_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_ntfy_dec

   DESCRIPTION    :  Routine to decode ASAPi Track Notification message

   ARGUMENTS      :  pBuff - Data buffer                     
                     msg   - Track Notification Message 
                     
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something. 
\****************************************************************************/
static uint32_t asapi_track_ntfy_dec(NCS_UBAID *pBuff, ASAPi_TRACK_NTFY_INFO *msg)
{
	uint8_t *stream = 0;
	uint8_t space[64], flag = 0;	/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t len = 0, idx = 0;

	/* Decode the flag */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(flag));
	flag = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(flag));

	/* Decode message length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(len));
	len = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(len));
	if (!len) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* wrong message length */
	}

	/* Decode track operation */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->opr));
	msg->opr = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->opr));

	/* Decode Group Info */
	asapi_ginfo_dec(pBuff, &msg->oinfo.group, &msg->oinfo.policy);

	/* Decode number of Queues */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(msg->oinfo.qcnt));
	msg->oinfo.qcnt = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(msg->oinfo.qcnt));

	/* Allocate Queue Information */
	if (msg->oinfo.qcnt) {
		msg->oinfo.qparam = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(msg->oinfo.qcnt *
								   sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
		if (!msg->oinfo.qparam) {
			return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
		}
		memset(msg->oinfo.qparam, 0, sizeof(ASAPi_QUEUE_PARAM));

		for (idx = 0; idx < msg->oinfo.qcnt; idx++) {
			/* Decode Queue Information */
			asapi_qinfo_dec(pBuff, &msg->oinfo.qparam[idx]);
		}
	}

	return rc;
}	/* End of asapi_track_ntfy_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_ginfo_dec

   DESCRIPTION    :  Routine to decode Group Information

   ARGUMENTS      :  pBuff  - Data buffer
                     group  - Group to decode 
                     policy - selection policy for the group                     

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_ginfo_dec(NCS_UBAID *pBuff, SaNameT *group, SaMsgQueueGroupPolicyT *policy)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(group, 0, sizeof(SaNameT));

	/* Decode Group Name Length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(group->length));
	group->length = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(group->length));
	if (!group->length)
		return NCSCC_RC_FAILURE;	/* Nothin to decode */

	/* Decode Group Selection Policy */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(SaMsgQueueGroupPolicyT));
	*policy = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(SaMsgQueueGroupPolicyT));

	/* Decode Group Name */
	stream = ncs_dec_flatten_space(pBuff, space, group->length);
	memcpy(group->value, stream, group->length);
	ncs_dec_skip_space(pBuff, group->length);

	return rc;
}	/* End of asapi_ginfo_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_qinfo_dec

   DESCRIPTION    :  Routine to decode Queue Information

   ARGUMENTS      :  pBuff - Data buffer
                     queue - Queue to decode                      

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_qinfo_dec(NCS_UBAID *pBuff, ASAPi_QUEUE_PARAM *queue)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS, i;

	/* Decode Queue Name & Length */
	rc = asapi_name_dec(pBuff, &queue->name);
	if (NCSCC_RC_SUCCESS != rc) {
		return rc;	/* Queue Info doesn't exist */
	}

	/* Decode Queue Handle */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->hdl));
	queue->hdl = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->hdl));

	/* Decode Registration Life */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->retentionTime));
	queue->retentionTime = ncs_decode_64bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->retentionTime));

	/* Decode MDS param */
	mds_uba_decode_mds_dest(pBuff, &queue->addr);

	/* Decode Selection Policy A. Ownership */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->owner));
	queue->owner = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->owner));

	/* Decode Selection Policy A. Sending State */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->status));
	queue->status = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->status));

	/* Decode the flag which says is_mqnd_down or not */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->is_mqnd_down));
	queue->is_mqnd_down = ncs_decode_8bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->is_mqnd_down));

	/* Decode creationFlags */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->creationFlags));
	queue->creationFlags = ncs_decode_32bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(queue->creationFlags));

	/* Decode size[] array */
	for (i = 0; i < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; i++) {
		stream = ncs_dec_flatten_space(pBuff, space, sizeof(queue->size[i]));
		queue->size[i] = ncs_decode_64bit(&stream);
		ncs_dec_skip_space(pBuff, sizeof(queue->size[i]));
	}

	return rc;
}	/* End of asapi_qinfo_dec() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_name_dec

   DESCRIPTION    :  Routine to decode name & length

   ARGUMENTS      :  pBuff - Data buffer
                     name - name to decode                      

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_name_dec(NCS_UBAID *pBuff, SaNameT *name)
{
	uint8_t *stream = 0;
	uint8_t space[64];		/* sufficient space to decode data */
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(name, 0, sizeof(SaNameT));

	/* Decode Name Length */
	stream = ncs_dec_flatten_space(pBuff, space, sizeof(name->length));
	name->length = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(pBuff, sizeof(name->length));
	if (!name->length)
		return NCSCC_RC_FAILURE;	/* Nothin to decode */

	/* Decode Name */
	stream = ncs_dec_flatten_space(pBuff, space, name->length);
	memcpy(name->value, stream, name->length);
	ncs_dec_skip_space(pBuff, name->length);

	return rc;
}	/* End of asapi_name_dec() */
