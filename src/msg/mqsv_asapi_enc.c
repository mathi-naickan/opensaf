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

  DESCRIPTION: This file includes routines for encoding ASAPi messages:

   asapi_msg_enc............ASAPi Message encode routine   
   asapi_reg_enc............ASAPi Registration Message Encode routine
   asapi_reg_resp_enc.......ASAPi Registration Response Message Encode routine
   asapi_dereg_enc..........ASAPi De-registration Message Encode routine
   asapi_dereg_resp_enc.....ASAPi De-registration Response Message Encode routine
   asapi_nreslove_enc.......ASAPi Name Resolution Message Encode routine
   asapi_nreslove_resp_enc..ASAPi Name Resolution Response Message Encode routine
   asapi_getqueue_enc.......ASAPi Getqueue Message Encode routine
   asapi_getqueue_resp_enc..ASAPi Getqueue Response Message Encode routine
   asapi_track_enc..........ASAPi Track Message Encode routine
   asapi_track_resp_enc.....ASAPi Track Response Message Encode routine
   asapi_track_ntfy_enc.....ASAPi Track Notification encode routine
******************************************************************************
*/
#include "msg/mqsv.h"

/******************************** LOCAL ROUTINES *****************************/
static void asapi_reg_enc(ASAPi_REG_INFO *, NCS_UBAID *);
static void asapi_reg_resp_enc(ASAPi_REG_RESP_INFO *, NCS_UBAID *);
static void asapi_dereg_enc(ASAPi_DEREG_INFO *, NCS_UBAID *);
static void asapi_dereg_resp_enc(ASAPi_DEREG_RESP_INFO *, NCS_UBAID *);
static void asapi_nreslove_enc(ASAPi_NRESOLVE_INFO *, NCS_UBAID *);
static void asapi_nreslove_resp_enc(ASAPi_NRESOLVE_RESP_INFO *, NCS_UBAID *);
static void asapi_getqueue_enc(ASAPi_GETQUEUE_INFO *, NCS_UBAID *);
static void asapi_getqueue_resp_enc(ASAPi_GETQUEUE_RESP_INFO *, NCS_UBAID *);
static void asapi_track_enc(ASAPi_TRACK_INFO *, NCS_UBAID *);
static void asapi_track_resp_enc(ASAPi_TRACK_RESP_INFO *, NCS_UBAID *);
static void asapi_track_ntfy_enc(ASAPi_TRACK_NTFY_INFO *, NCS_UBAID *);
static void asapi_ginfo_enc(SaNameT *, SaMsgQueueGroupPolicyT, NCS_UBAID *);
static void asapi_qinfo_enc(ASAPi_QUEUE_PARAM *, NCS_UBAID *);
static void asapi_name_enc(SaNameT *, NCS_UBAID *);
static void asapi_obj_info_enc(ASAPi_OBJECT_INFO *, ASAPi_ERR_INFO *, NCS_UBAID *);
/*****************************************************************************/

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_enc

   DESCRIPTION    :  This is ASAPi message encode routine, Based on the ASAPi
                     message type it encodes the messages. This routine will be 
                     invoked within the flow of MDS when it deceides to encode 
                     the ASAPi messages.
   
   ARGUMENTS      :  msg   - ASAPi Message                     
                     pBuff - buffer

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
   NOTE           :  This encode routine is common for both, inter process 
                     encoding & inter card encoding.
\****************************************************************************/
void asapi_msg_enc(ASAPi_MSG_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0;

	/* Encode ASAPi message type */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint8_t));
	ncs_encode_8bit(&pStream, msg->msgtype);
	ncs_enc_claim_space(pBuff, sizeof(uint8_t));

	if (ASAPi_MSG_REG == msg->msgtype) {
		/* Encode ASAPi Registration message */
		asapi_reg_enc(&msg->info.reg, pBuff);
	} else if (ASAPi_MSG_REG_RESP == msg->msgtype) {
		/* Encode ASAPi Registration Response message */
		asapi_reg_resp_enc(&msg->info.rresp, pBuff);
	} else if (ASAPi_MSG_DEREG == msg->msgtype) {
		/* Encode ASAPi De-registration message */
		asapi_dereg_enc(&msg->info.dereg, pBuff);
	} else if (ASAPi_MSG_DEREG_RESP == msg->msgtype) {
		/* Encode ASAPi De-registration Response message */
		asapi_dereg_resp_enc(&msg->info.dresp, pBuff);
	} else if (ASAPi_MSG_NRESOLVE == msg->msgtype) {
		/* Encode Name Resolution message */
		asapi_nreslove_enc(&msg->info.nresolve, pBuff);
	} else if (ASAPi_MSG_NRESOLVE_RESP == msg->msgtype) {
		/* Encode Name Resolution Response message */
		asapi_nreslove_resp_enc(&msg->info.nresp, pBuff);
	} else if (ASAPi_MSG_GETQUEUE == msg->msgtype) {
		/* Encode ASAPi Getqueue queue message */
		asapi_getqueue_enc(&msg->info.getqueue, pBuff);
	} else if (ASAPi_MSG_GETQUEUE_RESP == msg->msgtype) {
		/* Encode ASAPi Getqueue Queue Response message */
		asapi_getqueue_resp_enc(&msg->info.vresp, pBuff);
	} else if (ASAPi_MSG_TRACK == msg->msgtype) {
		/* Encode ASAPi Track message */
		asapi_track_enc(&msg->info.track, pBuff);
	} else if (ASAPi_MSG_TRACK_RESP == msg->msgtype) {
		/* Encode ASAPi Track Response message */
		asapi_track_resp_enc(&msg->info.tresp, pBuff);
	} else if (ASAPi_MSG_TRACK_NTFY == msg->msgtype) {
		/* Encode ASAPi Track Notification message */
		asapi_track_ntfy_enc(&msg->info.tntfy, pBuff);
	}
}	/* End of asapi_msg_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_reg_enc

   DESCRIPTION    :  Routine to encode ASAPi Registration message

   ARGUMENTS      :  msg   - Registration Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_reg_enc(ASAPi_REG_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Null value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint8_t));
	ncs_encode_8bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint8_t));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	if (ASAPi_OBJ_GROUP == msg->objtype) {
		/* Encode Group Information */
		asapi_ginfo_enc(&msg->group, msg->policy, pBuff);

		/* Encode Queue Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));
	} else if (ASAPi_OBJ_QUEUE == msg->objtype) {
		/* Encode Group Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));

		/* Encode Queue Information */
		asapi_qinfo_enc(&msg->queue, pBuff);
	} else if (ASAPi_OBJ_BOTH == msg->objtype) {
		/* Encode Group Information */
		asapi_ginfo_enc(&msg->group, msg->policy, pBuff);

		/* Encode Queue Information */
		asapi_qinfo_enc(&msg->queue, pBuff);
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the registration message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_reg_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_reg_resp_enc

   DESCRIPTION    :  Routine to encode ASAPi Registration Response message

   ARGUMENTS      :  msg   - Registration Response Message 
                     pBuff - Data buffer
   
   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_reg_resp_enc(ASAPi_REG_RESP_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode error value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.flag));
	ncs_encode_8bit(&pStream, msg->err.flag);
	ncs_enc_claim_space(pBuff, sizeof(msg->err.flag));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	if (ASAPi_OBJ_GROUP == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);

		/* Encode Queue Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));
	} else if (ASAPi_OBJ_QUEUE == msg->objtype) {
		/* Encode Group Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	} else if (ASAPi_OBJ_BOTH == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	}

	/* Encode error code (optional) */
	if (msg->err.flag) {
		/* Encode Error Code */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.errcode));
		ncs_encode_32bit(&pStream, (msg->err.errcode));
		ncs_enc_claim_space(pBuff, sizeof(msg->err.errcode));
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the registration message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));

}	/* End of asapi_reg_resp_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_dereg_enc

   DESCRIPTION    :  Routine to encode ASAPi DE-registration message

   ARGUMENTS      :  msg   - De-registration Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_dereg_enc(ASAPi_DEREG_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Null value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint8_t));
	ncs_encode_8bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint8_t));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	if (ASAPi_OBJ_GROUP == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);
		asapi_name_enc(&msg->queue, pBuff);
	} else if (ASAPi_OBJ_QUEUE == msg->objtype) {
		/* Encode Group Name & Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	} else if (ASAPi_OBJ_BOTH == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the de-registration message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_dereg_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_dereg_resp_enc

   DESCRIPTION    :  Routine to encode ASAPi De-registration Response message

   ARGUMENTS      :  msg   - De-registration Response Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_dereg_resp_enc(ASAPi_DEREG_RESP_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode error value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.flag));
	ncs_encode_8bit(&pStream, msg->err.flag);
	ncs_enc_claim_space(pBuff, sizeof(msg->err.flag));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	if (ASAPi_OBJ_GROUP == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);

		/* Encode Queue Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));
	} else if (ASAPi_OBJ_QUEUE == msg->objtype) {
		/* Encode Group Name Length */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
		ncs_encode_16bit(&pStream, 0);
		ncs_enc_claim_space(pBuff, sizeof(uint16_t));

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	} else if (ASAPi_OBJ_BOTH == msg->objtype) {
		/* Encode Group Name & Length */
		asapi_name_enc(&msg->group, pBuff);

		/* Encode Queue Name & Length */
		asapi_name_enc(&msg->queue, pBuff);
	}

	/* Encode error description (optional) */
	if (msg->err.flag) {
		/* Encode Error Code */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.errcode));
		ncs_encode_32bit(&pStream, (msg->err.errcode));
		ncs_enc_claim_space(pBuff, sizeof(msg->err.errcode));
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the de-registration message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_dereg_resp_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_nreslove_enc

   DESCRIPTION    :  Routine to encode ASAPi Name Resolution message

   ARGUMENTS      :  msg   - Name Resolution Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_nreslove_enc(ASAPi_NRESOLVE_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Track enable value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->track));
	ncs_encode_8bit(&pStream, msg->track);
	ncs_enc_claim_space(pBuff, sizeof(msg->track));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Name */
	asapi_name_enc(&msg->object, pBuff);

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the Name Resolution message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_nreslove_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_nreslove_resp_enc

   DESCRIPTION    :  Routine to encode ASAPi Name Resolution Response message

   ARGUMENTS      :  msg   - Name Resolution Response Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_nreslove_resp_enc(ASAPi_NRESOLVE_RESP_INFO *msg, NCS_UBAID *pBuff)
{
	asapi_obj_info_enc(&msg->oinfo, &msg->err, pBuff);
}	/* End of asapi_nreslove_resp_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_getqueue_enc

   DESCRIPTION    :  Routine to encode ASAPi Getqueue message

   ARGUMENTS      :  msg   - Getqueue Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_getqueue_enc(ASAPi_GETQUEUE_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Null value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint8_t));
	ncs_encode_8bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint8_t));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Queue name & length */
	asapi_name_enc(&msg->queue, pBuff);

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the Name Resolution message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_getqueue_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_getqueue_resp_enc

   DESCRIPTION    :  Routine to encode ASAPi Getqueue Response message

   ARGUMENTS      :  msg   - Getqueue Response Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_getqueue_resp_enc(ASAPi_GETQUEUE_RESP_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Error value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.flag));
	ncs_encode_8bit(&pStream, msg->err.flag);
	ncs_enc_claim_space(pBuff, sizeof(msg->err.flag));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Queue information */
	asapi_qinfo_enc(&msg->queue, pBuff);

	/* Encode error description (optional) */
	if (msg->err.flag) {
		/* Encode Error Code */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->err.errcode));
		ncs_encode_32bit(&pStream, (msg->err.errcode));
		ncs_enc_claim_space(pBuff, sizeof(msg->err.errcode));
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the Name Resolution message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_getqueue_resp_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_enc

   DESCRIPTION    :  Routine to encode ASAPi Track message

   ARGUMENTS      :  msg   - Track Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_track_enc(ASAPi_TRACK_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Track enable value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->val));
	ncs_encode_8bit(&pStream, msg->val);
	ncs_enc_claim_space(pBuff, sizeof(msg->val));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Object name & length */
	asapi_name_enc(&msg->object, pBuff);

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the registration message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_track_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_resp_enc

   DESCRIPTION    :  Routine to encode ASAPi Track Response message

   ARGUMENTS      :  msg   - Track Response Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_track_resp_enc(ASAPi_TRACK_RESP_INFO *msg, NCS_UBAID *pBuff)
{
	asapi_obj_info_enc(&msg->oinfo, &msg->err, pBuff);
}	/* End of asapi_track_resp_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_obj_info_enc

   DESCRIPTION    :  Routine to encode ASAPi Track Response message

   ARGUMENTS      :  info  - Object Info pointer
                     err   - Error info piinter
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_obj_info_enc(ASAPi_OBJECT_INFO *info, ASAPi_ERR_INFO *err, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	uint16_t idx = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Error value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(err->flag));
	ncs_encode_8bit(&pStream, err->flag);
	ncs_enc_claim_space(pBuff, sizeof(err->flag));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Group Information */
	asapi_ginfo_enc(&info->group, info->policy, pBuff);

	/* Encode number of sources */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(info->qcnt));
	ncs_encode_16bit(&pStream, ((uint16_t)(info->qcnt)));
	ncs_enc_claim_space(pBuff, sizeof(info->qcnt));

	/* Encode Queue information */
	for (idx = 0; idx < info->qcnt; idx++) {
		asapi_qinfo_enc(&info->qparam[idx], pBuff);
	}

	/* Encode error description (optional) */
	if (err->flag) {
		/* Encode Error Code */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(err->errcode));
		ncs_encode_32bit(&pStream, (err->errcode));
		ncs_enc_claim_space(pBuff, sizeof(err->errcode));
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the Name Resolution message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_obj_info_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_ntfy_enc

   DESCRIPTION    :  Routine to encode ASAPi Track Notification message

   ARGUMENTS      :  msg   - Track Notification Message 
                     pBuff - Data buffer

   RETURNS        :  Nothin   
\****************************************************************************/
static void asapi_track_ntfy_enc(ASAPi_TRACK_NTFY_INFO *msg, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0, *pLength = 0;
	uint16_t idx = 0;
	int32_t ival = 0, fval = 0;

	ival = pBuff->ttl - sizeof(uint8_t);	/* Store the current TTL value */

	/* Encode Null value */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint8_t));
	ncs_encode_8bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint8_t));

	/* Encode Total Length of the Message */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(uint16_t));
	pLength = pStream;
	/* As of now encode 0 coz we donno the total length of the msg yet */
	ncs_encode_16bit(&pStream, 0);
	ncs_enc_claim_space(pBuff, sizeof(uint16_t));

	/* Encode Track Operation */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->opr));
	ncs_encode_32bit(&pStream, (msg->opr));
	ncs_enc_claim_space(pBuff, sizeof(msg->opr));

	/* Encode Group Information */
	asapi_ginfo_enc(&msg->oinfo.group, msg->oinfo.policy, pBuff);

	/* Encode number of sources */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(msg->oinfo.qcnt));
	ncs_encode_16bit(&pStream, ((uint16_t)(msg->oinfo.qcnt)));
	ncs_enc_claim_space(pBuff, sizeof(msg->oinfo.qcnt));

	/* Encode Queue information */
	for (idx = 0; idx < msg->oinfo.qcnt; idx++) {
		asapi_qinfo_enc(&msg->oinfo.qparam[idx], pBuff);
	}

	/* OK we are alll set with encoding all the required params, 
	   Now encode the total length of the Name Resolution message 
	 */
	fval += pBuff->ttl;
	ncs_encode_16bit(&pLength, ((uint16_t)(fval - ival)));
}	/* End of asapi_track_ntfy_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_ginfo_enc

   DESCRIPTION    :  Routine to encode Group Information

   ARGUMENTS      :  group  - Group to encode 
                     policy - Member selection policy
                     pBuff  - Data buffer

   RETURNS        :  none
\****************************************************************************/
static void asapi_ginfo_enc(SaNameT *group, SaMsgQueueGroupPolicyT policy, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0;

	/* Encode Group Name Length */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(group->length));
	ncs_encode_16bit(&pStream, ((uint16_t)(group->length)));
	ncs_enc_claim_space(pBuff, sizeof(group->length));

	if (group->length) {
		/* Encode Group Selection Policy */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(policy));
		ncs_encode_32bit(&pStream, (policy));
		ncs_enc_claim_space(pBuff, sizeof(policy));

		/* Encode Group Name */
		pStream = ncs_enc_reserve_space(pBuff, group->length);
		ncs_encode_octets(&pStream, group->value, (uint8_t)group->length);
		ncs_enc_claim_space(pBuff, group->length);
	}
}	/* End of asapi_ginfo_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_qinfo_enc

   DESCRIPTION    :  Routine to encode Queue Information

   ARGUMENTS      :  queue - Queue to encode 
                     pBuff - Data buffer

   RETURNS        :  none
\****************************************************************************/
static void asapi_qinfo_enc(ASAPi_QUEUE_PARAM *queue, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0;
	uint32_t i;

	/* Encode Queue name & length */
	asapi_name_enc(&queue->name, pBuff);
	if (queue->name.length) {
		/* Encode Queue handle */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->hdl));
		ncs_encode_32bit(&pStream, (queue->hdl));
		ncs_enc_claim_space(pBuff, sizeof(queue->hdl));

		/* Encode Registration Life */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->retentionTime));
		ncs_encode_64bit(&pStream, queue->retentionTime);
		ncs_enc_claim_space(pBuff, sizeof(queue->retentionTime));

		/* Encode MDS Address */
		mds_uba_encode_mds_dest(pBuff, &queue->addr);

		/* Encode Selection Policy A. Ownership */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->owner));
		ncs_encode_32bit(&pStream, (queue->owner));
		ncs_enc_claim_space(pBuff, sizeof(queue->owner));

		/* Encode Selection Policy A. Sending State */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->status));
		ncs_encode_32bit(&pStream, (queue->status));
		ncs_enc_claim_space(pBuff, sizeof(queue->status));

		/* Encode is_mqnd_down  */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->is_mqnd_down));
		ncs_encode_8bit(&pStream, (queue->is_mqnd_down));
		ncs_enc_claim_space(pBuff, sizeof(queue->is_mqnd_down));

		/* Encode creationFlags */
		pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->creationFlags));
		ncs_encode_32bit(&pStream, (queue->creationFlags));
		ncs_enc_claim_space(pBuff, sizeof(queue->creationFlags));

		/* Encode size[] array */
		for (i = 0; i < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; i++) {
			pStream = ncs_enc_reserve_space(pBuff, sizeof(queue->size[i]));
			ncs_encode_64bit(&pStream, queue->size[i]);
			ncs_enc_claim_space(pBuff, sizeof(queue->size[i]));
		}

	}
}	/* End of asapi_qinfo_enc() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_name_enc

   DESCRIPTION    :  Routine to encode Name & length

   ARGUMENTS      :  name  - Name to encode 
                     pBuff - Data buffer

   RETURNS        :  none
\****************************************************************************/
static void asapi_name_enc(SaNameT *name, NCS_UBAID *pBuff)
{
	uint8_t *pStream = 0;

	/* Encode Name Length */
	pStream = ncs_enc_reserve_space(pBuff, sizeof(name->length));
	ncs_encode_16bit(&pStream, ((uint16_t)(name->length)));
	ncs_enc_claim_space(pBuff, sizeof(name->length));

	/* Encode Name */
	if (name->length) {
		pStream = ncs_enc_reserve_space(pBuff, name->length);
		ncs_encode_octets(&pStream, name->value, (uint8_t)name->length);
		ncs_enc_claim_space(pBuff, name->length);
	}
}	/* End of asapi_name_enc() */
