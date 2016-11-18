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

  DESCRIPTION: This file inclused following routines:

   asapi_msg_hdlr...............ASAPi Message Handler
   asapi_dest_get...............ASAPi Queue Get routine
   asapi_cache_update...........Updates the local cache
   asapi_msg_send...............Sends ASAPi message to the destination
   asapi_queue_select...........Queue selection algorithm routine
   asapi_object_find............Locates entry from the local cache
   asapi_msg_process............processes ASAPi messages
   asapi_obj_cmp................Compare routine
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "msg/mqsv.h"

ASAPi_CB asapi;			/* Global ASAPi Control Block */

/******************************** LOCAL ROUTINES *****************************/
static uint32_t asapi_msg_hdlr(ASAPi_MSG_ARG *);
static uint32_t asapi_dest_get(ASAPi_DEST_INFO *);
static uint32_t asapi_queue_get(ASAPi_Q_INFO *);
static uint32_t asapi_cache_update(ASAPi_OBJECT_INFO *, ASAPi_OBJECT_OPR, ASAPi_CACHE_INFO **);
static uint32_t asapi_msg_send(ASAPi_MSG_INFO *, MQSV_SEND_INFO *, NCSMDS_INFO *);
static ASAPi_CACHE_INFO *asapi_object_find(SaNameT *);
static void asapi_object_destroy(SaNameT *);
static uint32_t asapi_msg_process(ASAPi_MSG_INFO *, ASAPi_CACHE_INFO **);
static bool asapi_obj_cmp(void *, void *);
static bool asapi_queue_cmp(void *, void *);
static uint32_t asapi_grp_upd(ASAPi_GROUP_INFO *, ASAPi_OBJECT_INFO *, ASAPi_OBJECT_OPR);
static void asapi_usr_unbind(void);
static uint32_t aspai_fetch_info(ASAPi_MSG_INFO *, MQSV_SEND_INFO *, ASAPi_CACHE_INFO **);
static uint32_t asapi_track_process(ASAPi_GRP_TRACK_INFO *);
static uint32_t asapi_cpy_track_info(ASAPi_GROUP_INFO *, ASAPi_GROUP_TRACK_INFO *);
/*****************************************************************************/

/****************************************************************************\
   PROCEDURE NAME :  asapi_opr_hdlr

   DESCRIPTION    :  This is a operational handler for all ASAPi request issued
                     by the user of this library..
                     it performs following functionality
                     - Bind with ASAPi layer
                     - Unbind from ASAPi layer
                     - Get destination queue parameter.
                     - ASAPi Message routine
   ARGUMENTS      :  msg - Pointer to structure containing info on the request                                          

   RETURNS        :  SA_AIS_OK - All went well
                     SA_AIS_ERROR - internal processing didn't like something.
\****************************************************************************/
uint32_t asapi_opr_hdlr(ASAPi_OPR_INFO *opr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Do the required operation based on the request/operation type */
	if (ASAPi_OPR_BIND == opr->type) {
		/* Store the User specific attributes */
		asapi.usrhdlr = opr->info.bind.i_indhdlr;
		asapi.mds_hdl = opr->info.bind.i_mds_hdl;
		asapi.mds_svc_id = opr->info.bind.i_mds_id;
		asapi.my_svc_id = opr->info.bind.i_my_id;
		asapi.my_dest = opr->info.bind.i_mydest;

		/* Initialize the Cache */
		ncs_create_queue(&asapi.cache);
	} else if (ASAPi_OPR_UNBIND == opr->type) {
		asapi_usr_unbind();
	} else if (ASAPi_OPR_GET_DEST == opr->type) {
		rc = asapi_dest_get(&opr->info.dest);
	} else if (ASAPi_OPR_GET_QUEUE == opr->type) {
		rc = asapi_queue_get(&opr->info.queue);
	} else if (ASAPi_OPR_TRACK == opr->type) {
		rc = asapi_track_process(&opr->info.track);
	} else if (ASAPi_OPR_MSG == opr->type) {
		rc = asapi_msg_hdlr(&opr->info.msg);	/* Handle ASAPi Messages */
	}

	if (NCSCC_RC_SUCCESS == rc)
		rc = SA_AIS_OK;
	return rc;
}	/* End of asapi_opr_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_usr_unbind

   DESCRIPTION    :  This routines unbinds the user from the ASAP library and 
                     free all the resources 

   ARGUMENTS      :  none
   RETURNS        :  none
\****************************************************************************/
static void asapi_usr_unbind(void)
{
	ASAPi_QUEUE_INFO *pQinfo = 0;
	ASAPi_CACHE_INFO *pCache = 0;

	/* Check whetehr Cache exist & if so then we need to clean it up .. */
	while ((pCache = ncs_dequeue(&asapi.cache))) {
		m_NCS_LOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Lock the cache */

		if (ASAPi_OBJ_GROUP == pCache->objtype) {
			while ((pQinfo = ncs_dequeue(&pCache->info.ginfo.qlist))) {
				m_MMGR_FREE_ASAPi_QUEUE_INFO(pQinfo, asapi.my_svc_id);	/* Clean up the queues */
			}

			/* Destroy the queue list */
			ncs_destroy_queue(&pCache->info.ginfo.qlist);
		}
		m_NCS_UNLOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Unlock the cache */

		m_NCS_LOCK_DESTROY(&pCache->clock);
		m_MMGR_FREE_ASAPi_CACHE_INFO(pCache, asapi.my_svc_id);	/* Clean up cache nodes */
	}

	/* Destroy the Cache */
	ncs_destroy_queue(&asapi.cache);

	/* Reset the user information */
	memset(&asapi, 0, sizeof(asapi));
}

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_hdlr

   DESCRIPTION    :  This is a message handler for all ASAPi message.
                     Based on the action type it performs corrresponding 
                     functionality.

   ARGUMENTS      :  msg - ASAPi Message                                          

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     SA_AIS_ERROR
\****************************************************************************/
static uint32_t asapi_msg_hdlr(ASAPi_MSG_ARG *msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_CACHE_INFO *pCache = 0;

	/* Check what action needs to be carried out vis-a-vis message */
	if (ASAPi_MSG_SEND == msg->opr) {	/* Message has to be sent */
		NCSMDS_INFO mds;

		/* Send to the specified destination */
		rc = asapi_msg_send(&msg->req, &msg->sinfo, &mds);
		if (NCSCC_RC_REQ_TIMOUT == rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TIMEOUT);
		}

		if (NCSCC_RC_SUCCESS != rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TRY_AGAIN);
		}

		if (MDS_SENDTYPE_SNDRSP == msg->sinfo.stype) {
			if (mds.info.svc_send.info.sndrsp.o_rsp) {
				msg->resp = ((MQSV_EVT *)(mds.info.svc_send.info.sndrsp.o_rsp))->msg.asapi;
				m_MMGR_FREE_MQSV_EVT(mds.info.svc_send.info.sndrsp.o_rsp, asapi.my_svc_id);
			}
			if (!msg->resp) {
				return m_ASAPi_DBG_SINK(SA_AIS_ERR_MESSAGE_ERROR);
			}
		}
	} else if (ASAPI_MSG_RECEIVE == msg->opr) {	/* Message received */
		asapi_msg_process(msg->resp, &pCache);
		asapi_msg_free(&msg->resp);	/* Free up the ASAPi Message */
	} else {
		return m_ASAPi_DBG_SINK(SA_AIS_ERR_INVALID_PARAM);	/* Wrong Request */
	}

	return rc;
}	/* End of asapi_msg_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_dest_get

   DESCRIPTION    :  This is a syncronous call, it retuns the destination 
                     address of the queue based on the availabilty and 
                     selction policy of the queue.

   ARGUMENTS      :  dinfo - ASAPi Destination Message                                          

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_dest_get(ASAPi_DEST_INFO *dinfo)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_CACHE_INFO *pNode = 0;

	/* Check whether Object with that name exist in Local Cache */
	pNode = asapi_object_find(&dinfo->i_object);
	if (!pNode) {
		/* If the request is for the queue, then check if the queue exist with 
		 * any of the queue group before requesting the information MQD 
		 */

		/* If the information doesn't exist within the Local Cahce, then need
		   to send ASAPi Name Resolution request to get the information form the 
		   server.
		 */
		ASAPi_MSG_INFO msg;

		memset(&msg, 0, sizeof(ASAPi_MSG_INFO));

		/* Polulate the ASAPi Name Resolution message */
		msg.msgtype = ASAPi_MSG_NRESOLVE;

		memcpy(&msg.info.nresolve.object, &dinfo->i_object, sizeof(SaNameT));
		msg.info.nresolve.track = dinfo->i_track;
		dinfo->i_sinfo.stype = MDS_SENDTYPE_SNDRSP;	/* Make syncronous request */

		/* Fetch the information from the server and update the Cache */
		rc = aspai_fetch_info(&msg, &dinfo->i_sinfo, &pNode);
		if (NCSCC_RC_SUCCESS != rc) {
			return m_ASAPi_DBG_SINK(rc);
		}
	}
	/* The following if-else checks wheather the particular MQND is up or pwn
	   If MQND is down then rc is set to TRYAGAIN    */

	if (pNode->objtype == ASAPi_OBJ_GROUP) {
		ASAPi_QUEUE_INFO *pQelmSelected = pNode->info.ginfo.pQueue;
		ASAPi_QUEUE_INFO *pQelmLast = pNode->info.ginfo.plaQueue;

		pNode->info.ginfo.pQueue = 0;
		asapi_queue_select(&(pNode->info.ginfo));
		if (pNode->info.ginfo.pQueue) {
			if (pNode->info.ginfo.pQueue->param.is_mqnd_down == true) {
				rc = SA_AIS_ERR_TRY_AGAIN;
			}
		}
		pNode->info.ginfo.pQueue = pQelmSelected;	/* This is current selected Queue , retaining the old value for the Selected and last selected queues */
		pNode->info.ginfo.plaQueue = pQelmLast;	/* update last selected Queue */
	} else if (pNode->objtype == ASAPi_OBJ_QUEUE) {
		if (pNode->info.qinfo.param.is_mqnd_down == true) {
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}
	dinfo->o_cache = pNode;	/* Return the cache node */
	return rc;
}	/* End of asapi_dest_get() */

/****************************************************************************\
  PROCEDURE NAME :  asapi_queue_get

  DESCRIPTION    :  This is a syncronous call, it retuns the queue information
                    either from the local cache or fetches the information
                    from the server

  ARGUMENTS      :  qinfo - ASAPi Queue Message                                          

  RETURNS        :  SUCCESS - All went well
                    FAILURE - internal processing didn't like something.
                    SA_AIS_ERROR
\****************************************************************************/
static uint32_t asapi_queue_get(ASAPi_Q_INFO *qinfo)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_CACHE_INFO *pNode = 0;

	/* Check whether Object with that name exist in Local Cache */
	pNode = asapi_object_find(&qinfo->i_name);
	if (!pNode) {
		/* If the request is for the queue, then check if the queue exist with 
		 * any of the queue group before requesting the information MQD 
		 */

		/* If the information doesn't exist within the Local Cahce, then need
		   to send ASAPi Name Resolution request to get the information form the 
		   server.
		 */
		ASAPi_MSG_INFO msg;
		ASAPi_MSG_INFO *pMsg = 0;
		NCSMDS_INFO mds;

		/* Polulate the ASAPi Name Resolution message */
		msg.msgtype = ASAPi_MSG_GETQUEUE;

		memcpy(&msg.info.getqueue.queue, &qinfo->i_name, sizeof(SaNameT));
		qinfo->i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

		/* Send the message to the specified destination */
		rc = asapi_msg_send(&msg, &qinfo->i_sinfo, &mds);

		if (NCSCC_RC_REQ_TIMOUT == rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TIMEOUT);
		}

		if (NCSCC_RC_SUCCESS != rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TRY_AGAIN);
		}

		if (mds.info.svc_send.info.sndrsp.o_rsp) {
			pMsg = ((MQSV_EVT *)(mds.info.svc_send.info.sndrsp.o_rsp))->msg.asapi;
			m_MMGR_FREE_MQSV_EVT(mds.info.svc_send.info.sndrsp.o_rsp, asapi.my_svc_id);
		}

		if (!pMsg) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_MESSAGE_ERROR);
		}

		if (pMsg->info.vresp.err.flag) {
			rc = pMsg->info.vresp.err.errcode;
		} else {
			/* Populate the information that needed by the user */
			qinfo->o_parm = pMsg->info.vresp.queue;
		}

		/* Free up the ASAPi Message */
		asapi_msg_free(&pMsg);
		return rc;
	}

	if (ASAPi_OBJ_QUEUE == pNode->objtype)
		qinfo->o_parm = pNode->info.qinfo.param;
	else {
		/* Make sure the user has request for Queue & not fro Group */
		return m_ASAPi_DBG_SINK(SA_AIS_ERR_NOT_EXIST);
	}

	return rc;
}	/* End of asapi_queue_get() */

/****************************************************************************\
   PROCEDURE NAME :  aspai_fetch_info

   DESCRIPTION    :  This routine fetches the information from the MQD and 
                     updates the local cache with the received information

   ARGUMENTS      :  msg - ASAPi Message                                          

   RETURNS        :  MDS_DEST - All went well
                     NULL - internal processing didn't like something.
\****************************************************************************/
static uint32_t aspai_fetch_info(ASAPi_MSG_INFO *msg, MQSV_SEND_INFO *sinfo, ASAPi_CACHE_INFO **o_cache)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO *pMsg = 0;
	NCSMDS_INFO mds;

	/* Send the message to the specified destination */
	rc = asapi_msg_send(msg, sinfo, &mds);

	if (NCSCC_RC_REQ_TIMOUT == rc) {
		return m_ASAPi_DBG_SINK(SA_AIS_ERR_TIMEOUT);
	}

	if (NCSCC_RC_SUCCESS != rc) {
		return m_ASAPi_DBG_SINK(SA_AIS_ERR_TRY_AGAIN);
	}

	/* Get hold of the received information */
	if (mds.info.svc_send.info.sndrsp.o_rsp) {
		pMsg = ((MQSV_EVT *)(mds.info.svc_send.info.sndrsp.o_rsp))->msg.asapi;
		m_MMGR_FREE_MQSV_EVT(mds.info.svc_send.info.sndrsp.o_rsp, asapi.my_svc_id);
	}

	if (!pMsg) {
		return m_ASAPi_DBG_SINK(SA_AIS_ERR_MESSAGE_ERROR);
	}

	/* Process the message */
	rc = asapi_msg_process(pMsg, o_cache);
	asapi_msg_free(&pMsg);	/* Free up the ASAPi Message */
	if (NCSCC_RC_SUCCESS != rc) {
		return m_ASAPi_DBG_SINK(rc);
	}
	return rc;
}

/****************************************************************************\
   PROCEDURE NAME :  asapi_track_process

   DESCRIPTION    :  This is a syncronous call, it retuns the track 
                     inforamation based on the option specified by the user

   ARGUMENTS      :  dinfo - ASAPi Destination Message                                          

   RETURNS        :  NCSCC_RC_SUCCESS - All went well
                     SA_AIS_ERROR - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_track_process(ASAPi_GRP_TRACK_INFO *tinfo)
{
	ASAPi_CACHE_INFO *pNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (m_ASAPi_TRACK_IS_ENABLE(tinfo->i_option)) {
		pNode = asapi_object_find(&tinfo->i_group);
		if (!pNode) {
			/* If the information doesn't exist within the Local Cahce, then need
			 * to send ASAPi Track request to get the information form the server.
			 */
			ASAPi_MSG_INFO msg;

			/* Polulate the ASAPi Name Resolution message */
			msg.msgtype = ASAPi_MSG_TRACK;

			memcpy(&msg.info.track.object, &tinfo->i_group, sizeof(SaNameT));
			msg.info.track.val = tinfo->i_option;
			tinfo->i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

			/* Fetch the information from the server and update the Cache */
			rc = aspai_fetch_info(&msg, &tinfo->i_sinfo, &pNode);
			if (NCSCC_RC_SUCCESS != rc) {
				return m_ASAPi_DBG_SINK(rc);
			}

			if (SA_TRACK_CURRENT == tinfo->i_flags) {
				rc = asapi_cpy_track_info(&pNode->info.ginfo, &tinfo->o_ginfo);
			}
			return rc;
		}

		/* Track current information */
		/* No need to fill notification buffer in case of TrackChanges or TrackChangesOnly */
		if (tinfo->i_flags && SA_TRACK_CURRENT) {
			rc = asapi_cpy_track_info(&pNode->info.ginfo, &tinfo->o_ginfo);
		}
	} else {
		ASAPi_MSG_INFO msg;
		ASAPi_MSG_INFO *pMsg = 0;
		NCSMDS_INFO mds;

		/* Polulate the ASAPi Name Resolution message */
		msg.msgtype = ASAPi_MSG_TRACK;

		memcpy(&msg.info.track.object, &tinfo->i_group, sizeof(SaNameT));
		msg.info.track.val = tinfo->i_option;
		tinfo->i_sinfo.stype = MDS_SENDTYPE_SNDRSP;	/* Make syncronous request */

		rc = asapi_msg_send(&msg, &tinfo->i_sinfo, &mds);

		if (NCSCC_RC_REQ_TIMOUT == rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TIMEOUT);
		}
		if (NCSCC_RC_SUCCESS != rc) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_TRY_AGAIN);
		}

		/* Update the local cache with the received information */
		if (mds.info.svc_send.info.sndrsp.o_rsp) {
			pMsg = ((MQSV_EVT *)(mds.info.svc_send.info.sndrsp.o_rsp))->msg.asapi;
			m_MMGR_FREE_MQSV_EVT(mds.info.svc_send.info.sndrsp.o_rsp, asapi.my_svc_id);
		}

		if (!pMsg) {
			return m_ASAPi_DBG_SINK(SA_AIS_ERR_MESSAGE_ERROR);
		}
		asapi_msg_free(&pMsg);

		/* Remove the Group from the cache */
		asapi_object_destroy(&tinfo->i_group);
	}

	return rc;
}

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_process

   DESCRIPTION    :  This routine process the ASAPi message and invokes the 
                     routine to update the local cache.

   ARGUMENTS      :  msg - ASAPi Message                                          

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     SA_AIS_ERR
\****************************************************************************/
static uint32_t asapi_msg_process(ASAPi_MSG_INFO *msg, ASAPi_CACHE_INFO **o_cache)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Update the local cache with the received information */
	*o_cache = 0;
	if (ASAPi_MSG_NRESOLVE_RESP == msg->msgtype) {
		if (!msg->info.nresp.err.flag) {
			rc = asapi_cache_update(&msg->info.nresp.oinfo, ASAPi_QUEUE_ADD, o_cache);
			if (NCSCC_RC_SUCCESS != rc)
				rc = SA_AIS_ERR_LIBRARY;
		} else
			rc = msg->info.nresp.err.errcode;
		return rc;
	} else if (ASAPi_MSG_TRACK_NTFY == msg->msgtype) {
		rc = asapi_cache_update(&msg->info.tntfy.oinfo, msg->info.tntfy.opr, o_cache);
	} else if (ASAPi_MSG_TRACK_RESP == msg->msgtype) {
		if (!msg->info.tresp.err.flag) {
			rc = asapi_cache_update(&msg->info.tresp.oinfo, ASAPi_QUEUE_ADD, o_cache);
			if (NCSCC_RC_SUCCESS != rc)
				rc = SA_AIS_ERR_LIBRARY;
		} else
			rc = msg->info.tresp.err.errcode;
		return rc;
	}

	/* Indicate the upper layer about the arrival of the message */
	if (asapi.usrhdlr) {
		rc = asapi.usrhdlr(msg);
	}
	if (NCSCC_RC_SUCCESS != rc)
		rc = SA_AIS_ERR_LIBRARY;

	return rc;
}

/****************************************************************************\
   PROCEDURE NAME :  asapi_cache_update

   DESCRIPTION    :  This routines updates the local cache. The update of the
                     Local cache is required only when we get Name Resolution
                     request or Track Notification.
   
   ARGUMENTS      :  pInfo   - Object Information
                     opr     - Operation to be performed

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_cache_update(ASAPi_OBJECT_INFO *pInfo, ASAPi_OBJECT_OPR opr, ASAPi_CACHE_INFO **o_cache)
{
	ASAPi_CACHE_INFO *pCache = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaNameT *pObject = 0;

	/* Check if cache entry exist with us or not */
	*o_cache = 0;
	if (pInfo->group.length)
		pObject = &pInfo->group;	/* Group request */
	else
		pObject = &pInfo->qparam->name;	/* Queue request */

	pCache = asapi_object_find(pObject);
	if (pCache) {		/* Node exist with us, we olny need to update the information */
		m_NCS_LOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Lock the cache */

		if (ASAPi_OBJ_QUEUE == pCache->objtype) {	/* Object is a queue */
			if (ASAPi_QUEUE_DEL == opr) {
				m_NCS_UNLOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Unlock the cache */
				m_NCS_LOCK_DESTROY(&pCache->clock);

				asapi_object_destroy(pObject);
				return rc;
			} else if (ASAPi_QUEUE_UPD == opr) {
				pCache->info.qinfo.param = *(pInfo->qparam);
			} else if (ASAPi_QUEUE_MQND_UP == opr) {
				pCache->info.qinfo.param = *(pInfo->qparam);
			} else if (ASAPi_QUEUE_MQND_DOWN == opr) {
				pCache->info.qinfo.param = *(pInfo->qparam);
			}
		} else {	/* Object is a group */
			if (ASAPi_GROUP_DEL == opr) {
				m_NCS_UNLOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Unlock the cache */
				m_NCS_LOCK_DESTROY(&pCache->clock);

				/* Remove the Cache node */
				asapi_object_destroy(pObject);
				return rc;
			} else {
				/* Update the queue group */
				asapi_grp_upd(&pCache->info.ginfo, pInfo, opr);
			}
		}

		/* Unlock Cache */
		m_NCS_UNLOCK(&pCache->clock, NCS_LOCK_WRITE);	/* Unlock the cache */
	} else {		/* Node deosn't exist */
		/* Allocate the Cache Informaton node */
		pCache = m_MMGR_ALLOC_ASAPi_CACHE_INFO(asapi.my_svc_id);
		if (!pCache) {
			return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
		}
		memset(pCache, 0, sizeof(ASAPi_CACHE_INFO));
		m_NCS_LOCK_INIT(&pCache->clock);

		if (!pInfo->group.length) {
			pCache->objtype = ASAPi_OBJ_QUEUE;
			pCache->info.qinfo.param = *(pInfo->qparam);
		} else {
			pCache->objtype = ASAPi_OBJ_GROUP;
			memcpy(&pCache->info.ginfo.group, &pInfo->group, sizeof(SaNameT));

			/* Initilize the group list for queues */
			ncs_create_queue(&pCache->info.ginfo.qlist);

			/* Update the queue group */
			rc = asapi_grp_upd(&pCache->info.ginfo, pInfo, opr);
			if (NCSCC_RC_SUCCESS != rc) {

				/* Remove the Cache node */
				m_NCS_LOCK_DESTROY(&pCache->clock);
				asapi_object_destroy(pObject);
			}
		}

		/* Add the cache node */
		ncs_enqueue(&asapi.cache, pCache);
	}

	*o_cache = pCache;
	return rc;
}	/* End of asapi_cache_update() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_grp_upd

   DESCRIPTION    :  This routines supdates the queue group by updating the 
                     group param and adding queues to the list if it doesn't 
                     exist otherwise updates the queue param if already exist..
   
   ARGUMENTS      :  pGrp  - Group Info                    
                     pInfo - Object Info

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_grp_upd(ASAPi_GROUP_INFO *pGrp, ASAPi_OBJECT_INFO *pInfo, ASAPi_OBJECT_OPR opr)
{
	ASAPi_QUEUE_INFO *pQinfo = 0;
	uint32_t idx = 0;

	/* Update the group policy ... */
	pGrp->policy = pInfo->policy;

	for (idx = 0; idx < pInfo->qcnt; idx++) {
		if (ASAPi_QUEUE_ADD == opr) {
			pQinfo = ncs_find_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);
			if (pQinfo)
				continue;

			pQinfo = m_MMGR_ALLOC_ASAPi_QUEUE_INFO(asapi.my_svc_id);
			if (!pQinfo) {
				return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
			}

			/* Update the parameters */
			pQinfo->param = pInfo->qparam[idx];

			if (m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
			    m_NCS_NODE_ID_FROM_MDS_DEST(pQinfo->param.addr)) {
				pGrp->lcl_qcnt++;
			}

			/* Add the node to the Group */
			ncs_enqueue(&pGrp->qlist, pQinfo);
		} else if (ASAPi_QUEUE_UPD == opr) {
			pQinfo = ncs_find_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);
			if (pQinfo) {
				/* Udjust the the local reference count before update */
				if (m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(pQinfo->param.addr)) {
					pGrp->lcl_qcnt--;
				}

				pQinfo->param = pInfo->qparam[idx];

				/* Udjust the the local reference count after update */
				if (m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(pQinfo->param.addr)) {
					pGrp->lcl_qcnt++;
				}

			} else {
				return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
			}
		} else if (ASAPi_QUEUE_DEL == opr) {

			/* Find, whether queue that is going to delete is in the last send queue (plaQueue) 
			   in Cache, if it is the case it will update the plaQueue */
			pQinfo = ncs_find_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);

			if (pQinfo) {
				if (pQinfo == pGrp->plaQueue) {
					ASAPi_QUEUE_INFO *pQprev = 0, *pQnext = 0;
					NCS_Q_ITR itr;
					itr.state = 0;
					pQnext = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGrp->qlist, &itr);
					while (pQnext) {
						if (pQnext == pGrp->plaQueue) {
							pGrp->plaQueue = pQprev;
							break;
						}
						itr.state = pQnext;
						pQprev = pQnext;
						pQnext = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGrp->qlist, &itr);
					}
				}
			}

			/* Updated the plaQueue info, now delete the queue from the list */

			pQinfo = ncs_remove_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);
			if (pQinfo) {
				/* Reduce the local reference count before delete */
				if (m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(pQinfo->param.addr)) {
					pGrp->lcl_qcnt--;
				}

				m_MMGR_FREE_ASAPi_QUEUE_INFO(pQinfo, asapi.my_svc_id);	/* Clean up the queue */
			} else {
				return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
			}
		} else if (ASAPi_QUEUE_MQND_DOWN == opr) {
			if (pInfo->qparam[idx].is_mqnd_down == true) {
				pQinfo = ncs_find_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);
				if (pQinfo) {
					pQinfo->param = pInfo->qparam[idx];
					continue;
				}
			}
		} else if (ASAPi_QUEUE_MQND_UP == opr) {
			if (pInfo->qparam[idx].is_mqnd_down == false) {
				pQinfo = ncs_find_item(&pGrp->qlist, &pInfo->qparam[idx].name, asapi_queue_cmp);
				if (pQinfo) {
					pQinfo->param = pInfo->qparam[idx];
					continue;
				}

				pQinfo = m_MMGR_ALLOC_ASAPi_QUEUE_INFO(asapi.my_svc_id);
				if (!pQinfo) {
					return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
				}

				/* Update the parameters */
				pQinfo->param = pInfo->qparam[idx];

				if (m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(pQinfo->param.addr)) {
					pGrp->lcl_qcnt++;
				}

				/* Add the node to the Group */
				ncs_enqueue(&pGrp->qlist, pQinfo);
			}
		}
	}
	pInfo->qcnt = pGrp->qlist.count;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_send

   DESCRIPTION    :  This routines sends ASAPi messages to the specified 
                     destination using MDS. It uses MDS synchrous mechanisim. 
                     This routines expects the MDS envelop to be passed. In 
                     case of MDS sync call the same envlop is used to return 
                     the response message back.
   
   ARGUMENTS      :  msg   - ASAPi Message                     
                     mds   - MDS message

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t asapi_msg_send(ASAPi_MSG_INFO *msg, MQSV_SEND_INFO *sinfo, NCSMDS_INFO *mds)
{
	MQSV_EVT evt;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Populate the MDS envolpe & Send the message to destination */
	evt.type = MQSV_EVT_ASAPI;
	evt.msg.asapi = msg;

	/* Send to the specified destination */
	memset(mds, 0, sizeof(NCSMDS_INFO));
	mds->i_mds_hdl = asapi.mds_hdl;	/* MDS handle */
	mds->i_svc_id = asapi.mds_svc_id;	/* User's service id */
	mds->i_op = MDS_SEND;

	mds->info.svc_send.i_msg = &evt;
	mds->info.svc_send.i_priority = MDS_SEND_PRIORITY_LOW;
	mds->info.svc_send.i_sendtype = sinfo->stype;
	mds->info.svc_send.i_to_svc = sinfo->to_svc;

	if (MDS_SENDTYPE_SNDRSP == sinfo->stype) {
		if (mds->i_svc_id == NCSMDS_SVC_ID_MQND)
			mds->info.svc_send.info.sndrsp.i_time_to_wait = MQSV_WAIT_TIME_MQND;
		else
			mds->info.svc_send.info.sndrsp.i_time_to_wait = MQSV_WAIT_TIME;
		mds->info.svc_send.info.sndrsp.i_to_dest = sinfo->dest;
	} else if (MDS_SENDTYPE_RSP == sinfo->stype) {
		mds->info.svc_send.info.rsp.i_msg_ctxt = sinfo->ctxt;
		mds->info.svc_send.info.rsp.i_sender_dest = sinfo->dest;
	} else if (MDS_SENDTYPE_SND == sinfo->stype) {
		mds->info.svc_send.info.snd.i_to_dest = sinfo->dest;
	} else {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* Invalid send type */
	}

	rc = ncsmds_api(mds);
	return rc;
}	/* End of asapi_msg_send() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_fill_track_info

   DESCRIPTION    :  This routines walks the list of queues and populates
                     the passed notificationBuffer.

   ARGUMENTS      :  info - Group information
                     dest - destination information

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something
\****************************************************************************/
static uint32_t asapi_cpy_track_info(ASAPi_GROUP_INFO *pGinfo, ASAPi_GROUP_TRACK_INFO *pGTrackinfo)
{
	ASAPi_QUEUE_INFO *pQelm = 0;
	NCS_Q_ITR itr;
	uint32_t q_cnt = 0;
	uint32_t i = 0;

	itr.state = 0;
	while ((pQelm = (ASAPi_QUEUE_INFO *)ncs_walk_items(&pGinfo->qlist, &itr))) {
		q_cnt++;
		itr.state = pQelm;

	}

	if (q_cnt > pGTrackinfo->notification_buffer.numberOfItems)
		return NCSCC_RC_FAILURE;

	itr.state = 0;
	while ((pQelm = (ASAPi_QUEUE_INFO *)ncs_walk_items(&pGinfo->qlist, &itr))) {

		pGTrackinfo->notification_buffer.notification[i].change = SA_MSG_QUEUE_GROUP_NO_CHANGE;
		pGTrackinfo->notification_buffer.notification[i].member.queueName = pQelm->param.name;

		/* Sending state is removed from B.1.1 
		   pGTrackinfo->notification_buffer.notification[i].member.sendingState =
		   pQelm->param.status; */
		i++;
		itr.state = pQelm;

	}

	pGTrackinfo->notification_buffer.numberOfItems = i;
	pGTrackinfo->policy = pGinfo->policy;
	pGTrackinfo->qcnt = i;

	return NCSCC_RC_SUCCESS;
}	/* End of asapi_cpy_track_info() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_queue_select

   DESCRIPTION    :  This routines selects the queue from the list of queues 
                     based on the selection policy for the group. If the 
                     selection policy is multicast then it selects all the 
                     queues in the list otherwise if the selection policy is
                     unicast then it selects the queue in round robin manner.

   ARGUMENTS      :  info - Group information 
                     dest - destination information

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something                     
\****************************************************************************/
uint32_t asapi_queue_select(ASAPi_GROUP_INFO *pGinfo)
{
	ASAPi_QUEUE_INFO *pQelm = 0;
	NCS_Q_ITR itr;
	uint32_t q_cnt = pGinfo->qlist.count;
	uint32_t queue_open_flag = false;

	/* Ignore the request if there is no queue under the group */
	if (!pGinfo->qlist.count)
		return NCSCC_RC_SUCCESS;

	itr.state = pGinfo->plaQueue;
	q_cnt = pGinfo->qlist.count;

	/* changes for local round robin and round robin takeover */
	switch (pGinfo->policy) {
	case SA_MSG_QUEUE_GROUP_ROUND_ROBIN:
		do {
			pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

			if (!pQelm) {
				/* It has reach end of the list, need to get the first Queue */
				itr.state = 0;
				pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
			}
			if (pQelm->param.owner != MQSV_QUEUE_OWN_STATE_ORPHAN) {
				queue_open_flag = true;
				break;
			}
			q_cnt--;
		} while (q_cnt > 0);

		if (!queue_open_flag) {
			pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

			if (!pQelm) {
				/* It has reach end of the list, need to get the first Queue */
				itr.state = 0;
				pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
			}
		}
		break;

	case SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN:
		do {
			pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

			if (!pQelm) {
				/* It has reach end of the list, need to get the first Queue */
				itr.state = 0;
				pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
			}
			if ((m_NCS_NODE_ID_FROM_MDS_DEST(asapi.my_dest) ==
			     m_NCS_NODE_ID_FROM_MDS_DEST(pQelm->param.addr)) &&
			    (pQelm->param.owner != MQSV_QUEUE_OWN_STATE_ORPHAN)) {
				queue_open_flag = true;
				break;
			}
			q_cnt--;
		} while (q_cnt > 0);

		q_cnt = pGinfo->qlist.count;

		if (!queue_open_flag) {
			do {
				pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

				if (!pQelm) {
					/* It has reach end of the list, need to get the first Queue */
					itr.state = 0;
					pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
				}
				if (pQelm->param.owner != MQSV_QUEUE_OWN_STATE_ORPHAN) {
					queue_open_flag = true;
					break;
				}
				q_cnt--;
			} while (q_cnt > 0);
		}

		if (!queue_open_flag) {
			pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

			if (!pQelm) {
				/* It has reach end of the list, need to get the first Queue */
				itr.state = 0;
				pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
			}
		}

		break;

	case SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE:
		pGinfo->pQueue = 0;
		break;

	case SA_MSG_QUEUE_GROUP_BROADCAST:
		pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);

		if (!pQelm) {
			/* It has reach end of the list, need to get the first Queue */
			itr.state = 0;
			pQelm = (ASAPi_QUEUE_INFO *)ncs_queue_get_next(&pGinfo->qlist, &itr);
		}
		break;

	default:
		pGinfo->pQueue = 0;
	}
	pGinfo->pQueue = pQelm;	/* This is current selected Queue */
	pGinfo->plaQueue = pQelm;	/* update last selected Queue */

	return NCSCC_RC_SUCCESS;
}	/* End of asapi_queue_select() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_object_find

   DESCRIPTION    :  This routines finds either the queue or the queue group 
                     from the local cahce. It return the cache node in case of 
                     successful match otherwise retuns NULL.

   ARGUMENTS      :  pInfo - Object information to be located   

   RETURNS        :  ASAPi_CACHE_INFO * - Cache node                     
\****************************************************************************/
static ASAPi_CACHE_INFO *asapi_object_find(SaNameT *pObj)
{
	ASAPi_CACHE_INFO *pNode = 0;

	/* Find the cache node : If exist */
	pNode = ncs_find_item(&asapi.cache, (NCSCONTEXT)pObj, asapi_obj_cmp);
	return pNode;

}	/* End of asapi_object_find() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_object_destroy

   DESCRIPTION    :  This routines finds either the queue or the queue group 
                     from the local cache and destroyes the object and free's
                     all the resources of that object

   ARGUMENTS      :  pInfo - Object information to be destroyed

   RETURNS        :  none                     
\****************************************************************************/
static void asapi_object_destroy(SaNameT *pObj)
{
	ASAPi_CACHE_INFO *pCache = 0;
	ASAPi_QUEUE_INFO *pQinfo = 0;

	/* Find the cache node : If exist */
	pCache = ncs_remove_item(&asapi.cache, (NCSCONTEXT)pObj, asapi_obj_cmp);
	if (pCache) {
		if (ASAPi_OBJ_GROUP == pCache->objtype) {
			/* Clean up all the Queues if any in the group */
			while ((pQinfo = ncs_dequeue(&pCache->info.ginfo.qlist))) {
				m_MMGR_FREE_ASAPi_QUEUE_INFO(pQinfo, asapi.my_svc_id);
			}

			/* Destroy the queue list */
			ncs_destroy_queue(&pCache->info.ginfo.qlist);
		}
		m_MMGR_FREE_ASAPi_CACHE_INFO(pCache, asapi.my_svc_id);
	}
}	/* End of asapi_object_destroy() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_obj_cmp

   DESCRIPTION    :  This routines invoked to compare the nodes in the list 
   
   ARGUMENTS      :  key   - what to match
                     qelem - with whom to match
   
   RETURNS        :  true(If sucessfully matched)/FALSE(No match)                     
\****************************************************************************/
static bool asapi_obj_cmp(void *key, void *qelem)
{
	bool match = false;
	ASAPi_CACHE_INFO *pNode = (ASAPi_CACHE_INFO *)qelem;
	uint32_t cmp = 0;

	/* Match the object content */
	if (ASAPi_OBJ_QUEUE == pNode->objtype) {
		if (((SaNameT *)key)->length != pNode->info.qinfo.param.name.length)
			return false;
		cmp = memcmp(((SaNameT *)key)->value, pNode->info.qinfo.param.name.value, ((SaNameT *)key)->length);
	} else if (ASAPi_OBJ_GROUP == pNode->objtype) {
		if (((SaNameT *)key)->length != pNode->info.ginfo.group.length)
			return false;
		cmp = memcmp(((SaNameT *)key)->value, pNode->info.ginfo.group.value, ((SaNameT *)key)->length);
	}

	if (!cmp)
		match = true;
	return match;
}	/* End of asapi_obj_cmp() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_queue_cmp

   DESCRIPTION    :  This routines is invoked to compare the queue name from 
                     the list of the queue 
   
   ARGUMENTS      :  key   - what to match
                     elem  - with whom to match
   
   RETURNS        :  true(If sucessfully matched)/FALSE(No match)                     
\****************************************************************************/
static bool asapi_queue_cmp(void *key, void *elem)
{
	ASAPi_QUEUE_INFO *pQelm = (ASAPi_QUEUE_INFO *)elem;

	if (!memcmp(&pQelm->param.name, (SaNameT *)key, sizeof(SaNameT))) {
		return true;
	}
	return false;
}	/* End of asapi_queue_cmp() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_free

   DESCRIPTION    :  This routines frees the ASAPi message and internal message
                     if any allocated.
                     
   ARGUMENTS      :  msg   - ASAPi message                        

   RETURNS        :  nothin  
\****************************************************************************/
void asapi_msg_free(ASAPi_MSG_INFO **msg)
{
	ASAPi_OBJECT_INFO *pInfo = 0;

	if (!(*msg))
		return;

	/* Ceck the usage count before freeing the message */
	if ((*msg)->usg_cnt) {
		(*msg)->usg_cnt--;
		TRACE_1("Can't free the message check the count value");
		return;		/* We can't free ... */
	}

	if (ASAPi_MSG_NRESOLVE_RESP == (*msg)->msgtype)
		pInfo = &((*msg)->info.nresp.oinfo);
	else if (ASAPi_MSG_TRACK_NTFY == (*msg)->msgtype)
		pInfo = &((*msg)->info.tntfy.oinfo);

	if ((pInfo) && (pInfo->qparam)) {
		m_MMGR_FREE_ASAPi_DEFAULT_VAL(pInfo->qparam, asapi.my_svc_id);	/* Free the queue */
		pInfo->qparam = 0;
	}

	m_MMGR_FREE_ASAPi_MSG_INFO((*msg), asapi.my_svc_id);	/* Free the ASAPi Message */
	*msg = 0;
}	/* End of asapi_msg_free() */

/****************************************************************************\
   PROCEDURE NAME :  asapi_msg_cpy

   DESCRIPTION    :  This routines copies the ASAPi message and internal data
                     if any allocated. It allocates the memory for ASAPi 
                     destination message and then copies the content 
                     
   ARGUMENTS      :  from   - ASAPi source message
                     to     - ASAPi destination message

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something  
\****************************************************************************/
uint32_t asapi_msg_cpy(ASAPi_MSG_INFO *from, ASAPi_MSG_INFO **to)
{
	ASAPi_MSG_INFO *pMsg = 0;
	ASAPi_OBJECT_INFO *info = 0;

	if (!from) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
	}

	*to = 0;
	pMsg = m_MMGR_ALLOC_ASAPi_MSG_INFO(asapi.my_svc_id);	/* Allocate new MSG envelop */
	if (!pMsg) {
		return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);	/* Memmory failure ... */
	}
	memcpy(pMsg, from, sizeof(ASAPi_MSG_INFO));

	/* Need to futher re-alloc internal messages if any offor certian type of 
	 * messages
	 */
	if (ASAPi_MSG_NRESOLVE_RESP == pMsg->msgtype)
		info = &pMsg->info.nresp.oinfo;
	else if (ASAPi_MSG_TRACK_NTFY == pMsg->msgtype)
		info = &pMsg->info.tntfy.oinfo;

	if ((info) && (info->qcnt)) {
		info->qparam = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(info->qcnt * sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
		if (!info->qparam) {
			return m_ASAPi_DBG_SINK(NCSCC_RC_FAILURE);
		}
		memcpy(info->qparam, (ASAPi_MSG_NRESOLVE_RESP == pMsg->msgtype) ?
		       &pMsg->info.nresp.oinfo.qparam : &pMsg->info.tntfy.oinfo.qparam, sizeof(ASAPi_QUEUE_PARAM));
	}

	*to = pMsg;
	return NCSCC_RC_SUCCESS;
}	/* End of asapi_msg_cpy() */

/*****************************************************************************

  PROCEDURE NAME:    asapi_dbg_sink

  DESCRIPTION:

   ASAPi is entirely instrumented to fall into this debug sink function
   if it hits any if-conditional that fails, but should not fail. This
   is the default implementation of that SINK macro.
    
  ARGUMENTS:

  uint32_t   l             line # in file
  char*   f             file name where macro invoked
  code    code          Error code value.. Usually FAILURE

  RETURNS:

  code    - just echo'ed back 

*****************************************************************************/

#if (ASAPi_DEBUG == 1)

uint32_t asapi_dbg_sink(uint32_t l, char *f, uint32_t code)
{
	TRACE("IN ASAPi_DBG_SINK: line %d, file %s", l, f);
	return code;
}

#endif
