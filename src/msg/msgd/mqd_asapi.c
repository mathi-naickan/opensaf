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

  DESCRIPTION: This file includes following routines:
   mqd_asapi_msg_hdlr.............ASAPi message handler
   mqd_asapi_reg_hdlr.............ASAPi registration msg handler
   mqd_asapi_dereg_hdlr...........ASAPi deregistration msg handler
   mqd_asapi_nresolve_hdlr........ASAPi name resolution msg handler
   mqd_asapi_getqueue_hdlr........ASAPi getqueue msg handler
   mqd_asapi_track_hdlr...........ASAPi track msg handler
   mqd_asapi_track_ntfy_send......ASAPi track notification hansler
   mqd_asapi_resp_send............ASAPi response send routine
   mqd_asapi_queue_make...........ASAPi queue make routine
   mqd_asapi_db_upd...............Data base update routine
   mqd_asapi_dereg_db_upd.........Deregister database update routine
   mqd_asapi_track_db_upd.........Track database update routine
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "osaf/immutil/immutil.h"
#include "mqd_imm.h"
#include "msg/msgd/mqd.h"

extern MQDLIB_INFO gl_mqdinfo;

/******************************** LOCAL ROUTINES *****************************/
static uint32_t mqd_asapi_reg_hdlr(MQD_CB *, ASAPi_REG_INFO *, MQSV_SEND_INFO *);
static uint32_t mqd_asapi_nresolve_hdlr(MQD_CB *, ASAPi_NRESOLVE_INFO *, MQSV_SEND_INFO *);
static uint32_t mqd_asapi_getqueue_hdlr(MQD_CB *, ASAPi_GETQUEUE_INFO *, MQSV_SEND_INFO *);
static uint32_t mqd_asapi_track_hdlr(MQD_CB *, ASAPi_TRACK_INFO *, MQSV_SEND_INFO *);
static bool mqd_asapi_obj_validate(MQD_CB *, SaNameT *, MQD_OBJ_NODE **);
static uint32_t mqd_asapi_queue_make(MQD_OBJ_INFO *, ASAPi_QUEUE_PARAM **, uint16_t *, bool);
static uint32_t mqd_asapi_resp_send(ASAPi_MSG_INFO *, MQSV_SEND_INFO *);
static uint32_t mqd_asapi_track_ntfy_send(MQD_OBJ_INFO *, ASAPi_OBJECT_OPR);
static bool mqd_check_for_namespace_collision(MQD_OBJ_NODE *, MQSV_OBJ_TYPE);

/*****************************************************************************/

/****************************************************************************\
 PROCEDURE NAME : mqd_asapi_evt_hdlr

 DESCRIPTION    : This is the callback handler for ASAPi events.

 ARGUMENTS      : pEvt - This is the pointer which holds the event structure.
                  pMqd - MQD Controll block pointer

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
\*****************************************************************************/
uint32_t mqd_asapi_evt_hdlr(MQSV_EVT *pEvt, MQD_CB *pMqd)
{
	ASAPi_MSG_INFO *pMsg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (pMqd->active) {
		/* Get hold of the ASAPi Message */
		pMsg = pEvt->msg.asapi;
		if (!pMsg) {
			LOG_ER("MQD_CB is NULL");
			return NCSCC_RC_FAILURE;
		}

		/* Check the message type and handle the message */
		if (ASAPi_MSG_REG == pMsg->msgtype) {
			/* Handle the ASAPi Registration request */
			TRACE_1("ASAPi Registration Message Received");
			rc = mqd_asapi_reg_hdlr(pMqd, &pMsg->info.reg, &pEvt->sinfo);
		} else if (ASAPi_MSG_DEREG == pMsg->msgtype) {
			/* Handle the ASAPi De-registration request */
			TRACE_1("ASAPi Deregistration Message Received");
			rc = mqd_asapi_dereg_hdlr(pMqd, &pMsg->info.dereg, &pEvt->sinfo);
		} else if (ASAPi_MSG_NRESOLVE == pMsg->msgtype) {
			/* Handle the ASAPi Name Resolution request */
			TRACE_1("ASAPi Name Resolution Message Received");
			rc = mqd_asapi_nresolve_hdlr(pMqd, &pMsg->info.nresolve, &pEvt->sinfo);
		} else if (ASAPi_MSG_GETQUEUE == pMsg->msgtype) {
			/* Handle the ASAPi Getqueue request */
			TRACE_1("ASAPi Getqueue Message Received");
			rc = mqd_asapi_getqueue_hdlr(pMqd, &pMsg->info.getqueue, &pEvt->sinfo);
		} else if (ASAPi_MSG_TRACK == pMsg->msgtype) {
			/* Handle the ASAPi Track request */
			TRACE_1("ASAPi Track Message Received");
			rc = mqd_asapi_track_hdlr(pMqd, &pMsg->info.track, &pEvt->sinfo);
		}
	}
	TRACE_1("ASAPi Event processed completely with return value %u", rc);
	/* OK -- We finished processing now free the ASAPi message */
	asapi_msg_free(&pMsg);
	pEvt->msg.asapi = 0;
	return rc;
}

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_reg_hdlr

   DESCRIPTION    :  This is a request handler for the ASAPi registration 
                     request. 
                   
   ARGUMENTS      :  pMqd - MQD Controll block 
                     reg  - ASAPi Registration Message
                     info - Send Info

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t mqd_asapi_reg_hdlr(MQD_CB *pMqd, ASAPi_REG_INFO *reg, MQSV_SEND_INFO *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO msg;
	MQD_OBJ_NODE *pObjNode = 0;
	MQD_OBJECT_ELEM *pOelm = 0;
	ASAPi_OBJECT_OPR opr = 0;
	NCS_Q_ITR itr;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(msg));

	if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
		/* Update the database */
		rc = mqd_asapi_db_upd(pMqd, reg, &pObjNode, &opr);

		if (rc == NCSCC_RC_SUCCESS) {
			/* Send Asynchronous update to the standby MQD */
			mqd_a2s_async_update(pMqd, MQD_A2S_MSG_TYPE_REG, (void *)reg);
			if (reg->objtype == ASAPi_OBJ_QUEUE)
				TRACE_1("Registering QUEUE DB Update for processing Success");
			else
				TRACE_1("Registering QUEUE GROUPS DB Update processing Success");
		} else {
			if (reg->objtype == ASAPi_OBJ_QUEUE)
				LOG_ER("Registering QUEUE DB Update processing failed with error %u", rc);
			else
				LOG_ER("Registering QUEUE GROUPS DB Update processing failed with error %u", rc);
			/* Set the Error flag and error code in response message */
			msg.info.rresp.err.flag = true;
			msg.info.rresp.err.errcode = rc;
			goto send_resp;
		}
	} else {
		msg.info.rresp.err.flag = true;
		msg.info.rresp.err.errcode = SA_AIS_ERR_NO_RESOURCES;
		goto send_resp;
	}

	/* Check if anybody has opted for tracking, if so then we need 
	 * to notify them about the update of the object 
	 */
	if (pObjNode) {
		mqd_asapi_track_ntfy_send(&pObjNode->oinfo, opr);

		/* We need to send additional track notification if the Queue is
		 * part of the group and the attributes of the queue got updated
		 */
		if ((MQSV_OBJ_QUEUE == pObjNode->oinfo.type) && (ASAPi_QUEUE_UPD == opr)) {
			itr.state = 0;
			while ((pOelm = (MQD_OBJECT_ELEM *)ncs_walk_items(&pObjNode->oinfo.ilist, &itr))) {
				mqd_asapi_track_ntfy_send(pOelm->pObject, opr);
			}
		}
	}

 send_resp:
	if (info) {
		if (info->stype == MDS_SENDTYPE_RSP) {
			/* Make Registration response message */
			msg.info.rresp.objtype = reg->objtype;

			/* Fill Group name if specified */
			if (ASAPi_OBJ_GROUP == reg->objtype) {
				memcpy(&msg.info.rresp.group, &reg->group, sizeof(SaNameT));
			} else if (ASAPi_OBJ_QUEUE == reg->objtype) {
				memcpy(&msg.info.rresp.queue, &reg->queue.name, sizeof(SaNameT));
			} else if (ASAPi_OBJ_BOTH == reg->objtype) {
				memcpy(&msg.info.rresp.group, &reg->group, sizeof(SaNameT));
				memcpy(&msg.info.rresp.queue, &reg->queue.name, sizeof(SaNameT));
			}

			/* Send Registration Response message */
			msg.msgtype = ASAPi_MSG_REG_RESP;
			rc = mqd_asapi_resp_send(&msg, info);
			if (NCSCC_RC_SUCCESS != rc) {
				if (reg->objtype == ASAPi_OBJ_QUEUE)
					LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi  QUEUE"
						"Registration Response Message");
				else
					LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi QUEUE GROUPS"
						"Registration Response Message");
				return rc;
			}
			if (reg->objtype == ASAPi_OBJ_QUEUE)
				TRACE_1("ASAPi QUEUE Registration Response Message Sent Success");
			else
				TRACE_1("ASAPi QUEUE GROUPS Registration Response Message Sent Success");
		}
	}
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_reg_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_dereg_hdlr

   DESCRIPTION    :  This is a request handler for the ASAPi De-registration 
                     request. 
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer 
                     dereg - ASAPi Deregistration Message
                     info  - Send Info

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_asapi_dereg_hdlr(MQD_CB *pMqd, ASAPi_DEREG_INFO *dereg, MQSV_SEND_INFO *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO msg;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(msg));

	if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
		rc = mqd_asapi_dereg_db_upd(pMqd, dereg, &msg);

		/* Send Async Update to the standby for MQD redundancy */
		if (rc == NCSCC_RC_SUCCESS) {
			mqd_a2s_async_update(pMqd, MQD_A2S_MSG_TYPE_DEREG, (void *)dereg);
			if (dereg->objtype == ASAPi_OBJ_QUEUE)
				TRACE_1("Deregistering QUEUE DB Update for processing Success");
			else
				TRACE_1("Deregistering QUEUE GROUPS DB Update for processing Success");
		} else {
			if (dereg->objtype == ASAPi_OBJ_QUEUE)
				LOG_ER("Deregistering QUEUE DB Update processing failed with error %u", rc);
			else
				LOG_ER("Deregistering QUEUE GROUPS DB Update processing failed with error %u", rc);
		}
	} else {
		/* You are NOT supposed to be here under any circumstances. 
		 * If here, something is seriously wrong */
		LOG_CR("%s:%u: AMF is Not in Active State", __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		msg.info.dresp.err.flag = true;
		msg.info.dresp.err.errcode = SA_AIS_ERR_NO_RESOURCES;
	}

	if (info) {
		/* Make De-registration response message */
		if (info->stype == MDS_SENDTYPE_RSP) {
			msg.info.dresp.objtype = dereg->objtype;

			/* Fill Group name if specified */
			if (ASAPi_OBJ_GROUP == dereg->objtype) {
				memcpy(&msg.info.dresp.group, &dereg->group, sizeof(SaNameT));
			} else if (ASAPi_OBJ_QUEUE == dereg->objtype) {
				memcpy(&msg.info.dresp.queue, &dereg->queue, sizeof(SaNameT));
			} else if (ASAPi_OBJ_BOTH == dereg->objtype) {
				memcpy(&msg.info.dresp.group, &dereg->group, sizeof(SaNameT));
				memcpy(&msg.info.dresp.queue, &dereg->queue, sizeof(SaNameT));
			}

			/* Send Deregistration Response message */
			msg.msgtype = ASAPi_MSG_DEREG_RESP;

			rc = mqd_asapi_resp_send(&msg, info);

			if (NCSCC_RC_SUCCESS != rc) {
				if (dereg->objtype == ASAPi_OBJ_QUEUE)
					LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi  QUEUE"
						"Deregistration Response Message");
				else
					LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi  QUEUE GROUPS"
						"Deregistration Response Message");
				return rc;
			}
			if (dereg->objtype == ASAPi_OBJ_QUEUE)
				TRACE_1("ASAPi QUEUE Deregistration Response Message Sent Success");
			else
				TRACE_1("ASAPi QUEUE GROUPS Deregistration Response Message Sent Success");
		}
	}
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_dereg_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_dereg_db_upd

   DESCRIPTION    :  This routine updates the database  based on the 
                     pre-existence of the object. If the object is already 
                    present then if just updates the param if the queue is to
                    be removed from the group else deletes the entry in the DB
                    And updates the params.It also notifies the elements in the
                    track list of the update in the case of Active.. 
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer 
                     dereg - ASAPi Deregistration Message
                     onode - Object node

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_asapi_dereg_db_upd(MQD_CB *pMqd, ASAPi_DEREG_INFO *dereg, ASAPi_MSG_INFO *msg)
{
	bool qexist = false, qgrpexist = false;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_OBJ_NODE *pObjNode = NULL, *pObj = NULL;
	MQD_OBJECT_ELEM *pOelm = NULL, *pQGelm = NULL;
	TRACE_ENTER();

	switch (dereg->objtype) {

		/* Remove queue from QueueGroup */
	case ASAPi_OBJ_BOTH:
		{
			/* Check for existence of Queue & QueueGroup */
			if ((!(qgrpexist = mqd_asapi_obj_validate(pMqd, &dereg->group, &pObjNode)))
			    || (!(qexist = mqd_asapi_obj_validate(pMqd, &dereg->queue, &pObj)))) {
				TRACE_2("ERR_NOT_EXIST: Database Operation (DEL) Failed");
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			/* check if QueueGroup passed is actually a Queue */
			if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QUEUE)) {
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			/* check if Queue passed is actually QueueGroup */
			if (mqd_check_for_namespace_collision(pObj, MQSV_OBJ_QGROUP)) {
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			/* Find the Queue with the name from that group if exist */
			pOelm = ncs_find_item(&pObjNode->oinfo.ilist, &dereg->queue, mqd_obj_cmp);
			if (!pOelm) {	/* Object doesn't exist ... */
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
			} else {
				/* Check if anybody has opted for tracking, if so then we need
				 * to notify them about the update of the object */
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					pOelm->pObject->info.q.adv = true;
					mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_DEL);
				}

				/* Remove the Queue from the Group */
				pOelm = ncs_remove_item(&pObjNode->oinfo.ilist, &dereg->queue, mqd_obj_cmp);
				if (pOelm) {
					MQD_OBJ_INFO *pqObj = 0;
					MQD_OBJECT_ELEM *pGelm = 0;

					pqObj = pOelm->pObject;
					pGelm = ncs_remove_item(&pqObj->ilist, &dereg->group, mqd_obj_cmp);
					m_MMGR_FREE_MQD_OBJECT_ELEM(pOelm);

					if (pGelm)
						m_MMGR_FREE_MQD_OBJECT_ELEM(pGelm);
					TRACE_1("Removing a queue from Queue Group Success");
				}
				/* Update Runtime Attribute to IMMSV */
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					immutil_update_one_rattr(pMqd->immOiHandle, (char *)pObjNode->oinfo.name.value,
								 "saMsgQueueGroupNumQueues", SA_IMM_ATTR_SAUINT32T,
								 &pObjNode->oinfo.ilist.count);
					mqd_runtime_update_grpmembers_attr(pMqd, pObjNode);
				}
			}
		}
		break;

		/* QueueGroup delete */
	case ASAPi_OBJ_GROUP:
		{
			/* Check for existence of QueueGroup */
			if (!mqd_asapi_obj_validate(pMqd, &dereg->group, &pObjNode)) {
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				TRACE_2("ERR_NOT_EXIST: Database Operation (DEL) Failed");
				goto resp_send;
			}

			/* check if QueueGroup passed is actually a Queue */
			if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QUEUE)) {
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
				if (immutil_saImmOiRtObjectDelete(pMqd->immOiHandle, &pObjNode->oinfo.name) !=
				    SA_AIS_OK) {
					TRACE_2("Deleting MsgQGrp object %s FAILED", pObjNode->oinfo.name.value);
					return NCSCC_RC_FAILURE;
				}
			}
			/* Clean all the queue associated with this group */
			while ((pOelm = ncs_dequeue(&pObjNode->oinfo.ilist))) {
				if (pOelm) {
					MQD_OBJ_INFO *pqObj = 0;
					MQD_OBJECT_ELEM *pGelm = 0;

					pqObj = pOelm->pObject;
					pGelm = ncs_remove_item(&pqObj->ilist, &dereg->group, mqd_obj_cmp);
					m_MMGR_FREE_MQD_OBJECT_ELEM(pOelm);

					if (pGelm)
						m_MMGR_FREE_MQD_OBJECT_ELEM(pGelm);
				}
			}

			/* Check if anybody has opted for tracking, if so then we need
			 * to notify them about the update of the object
			 */
			if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_GROUP_DEL);
			}

			/* Destroy the Node */
			mqd_db_node_del(pMqd, pObjNode);
			TRACE_1("Deleting the Queue Group Success");
		}
		break;

		/* Queue close/unlink */
	case ASAPi_OBJ_QUEUE:
		{
			/* Check for existence of Queue */
			if (!mqd_asapi_obj_validate(pMqd, &dereg->queue, &pObjNode)) {
				TRACE_2("ERR_NOT_EXIST: Database Operation (DEL) Failed");
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			/* check if Queue passed is actually QueueGroup */
			if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QGROUP)) {
				TRACE_2("ERR_NOT_EXIST: Database Operation (DEL) Failed");
				rc = SA_AIS_ERR_NOT_EXIST;
				msg->info.dresp.err.flag = true;
				msg->info.dresp.err.errcode = rc;
				goto resp_send;
			}

			/* Check if anybody has opted for tracking, if so then we need
			 * to notify them about the update of the object
			 */
			if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
				pObjNode->oinfo.info.q.adv = true;
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_DEL);
			}

			/* We need to send additional track notification if the Queue is
			 * part of the group and the queue got deleted
			 */
			while ((pOelm = ncs_dequeue(&pObjNode->oinfo.ilist))) {
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					pObjNode->oinfo.info.q.adv = true;
					mqd_asapi_track_ntfy_send(pOelm->pObject, ASAPi_QUEUE_DEL);
				}

				/* Remove the Queue from the Group */
				pQGelm = ncs_remove_item(&pOelm->pObject->ilist, &dereg->queue, mqd_obj_cmp);

				/* Update Runtime Attribute to IMMSV */
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					immutil_update_one_rattr(pMqd->immOiHandle, (char *)pOelm->pObject->name.value,
								 "saMsgQueueGroupNumQueues", SA_IMM_ATTR_SAUINT32T,
								 &pOelm->pObject->ilist.count);
				}

				if (pQGelm)
					m_MMGR_FREE_MQD_OBJECT_ELEM(pQGelm);

				if (pOelm)
					m_MMGR_FREE_MQD_OBJECT_ELEM(pOelm);	/* Clean up the queue group */
			}

			/* Destroy the Node */
			mqd_db_node_del(pMqd, pObjNode);
			TRACE_1("Destroying the Queue Success(Unlink or Close case)");
		}
		break;

	default:
		TRACE_2("ERR_INVALID_PARAM: ASAPi_DEREG_INFO type does not match %d", dereg->objtype);
		return SA_AIS_ERR_INVALID_PARAM;
		break;
	}

 resp_send:
	TRACE_LEAVE2(" return code %u", rc);
	return rc;
}	/* End of mqd_asapi_dereg_db_upd() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_nresolve_hdlr

   DESCRIPTION    :  This is a request handler for the ASAPi Name Resolution 
                     request. 
                   
   ARGUMENTS      :  pMqd     - MQD Controll block pointer
                     nresolve - ASAPi Name Resolution Message
                     info     - Send Info

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t mqd_asapi_nresolve_hdlr(MQD_CB *pMqd, ASAPi_NRESOLVE_INFO *nresolve, MQSV_SEND_INFO *info)
{
	bool exist = false;
	MQD_OBJ_NODE *pObjNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_QUEUE_PARAM *pQueue = 0;
	uint16_t qcnt = 0;
	ASAPi_MSG_INFO msg;
	MQD_A2S_TRACK_INFO track;
	TRACE_ENTER();

	memset(&track, 0, sizeof(track));
	memset(&msg, 0, sizeof(msg));

	if (pMqd->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("%s:%u: ERR_RESOURCES: AMF is Not in Active State",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NO_RESOURCES;

		/* Set the error flag and fill the description */
		msg.info.nresp.err.flag = true;
		msg.info.nresp.err.errcode = rc;

		/* Log suitable msg */
		goto send_resp;
	}
	/* Validate the object */
	exist = mqd_asapi_obj_validate(pMqd, &nresolve->object, &pObjNode);
	if (!exist) {		/* Object doesn't exist ... */
		TRACE_1("Nameresolvehandler Database access not found");
		rc = SA_AIS_ERR_NOT_EXIST;

		/* Set the error flag and fill the description */
		msg.info.nresp.err.flag = true;
		msg.info.nresp.err.errcode = rc;

		goto send_resp;
	}

	if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE) {
		if (pObjNode->oinfo.info.q.is_mqnd_down) {
			LOG_ER("%s:%u: ERR_TRY_AGAIN: The corresponding MQND is down", __FILE__, __LINE__);
			rc = SA_AIS_ERR_TRY_AGAIN;

			/* Set the error flag and fill the description */
			msg.info.nresp.err.flag = true;
			msg.info.nresp.err.errcode = rc;
			goto send_resp;
		}
	}

	/* Update the tracking information */
	if (m_ASAPi_TRACK_IS_ENABLE(nresolve->track)) {

		/* Enable tracking for the sender vis-a-vis object */
		rc = mqd_track_add(&pObjNode->oinfo.tlist, &info->dest, info->to_svc);
		if (NCSCC_RC_SUCCESS != rc) {
			msg.info.nresp.err.flag = true;
			msg.info.nresp.err.errcode = rc;
		} else {
			/* Send Async update at active side by filling the track info */
			memcpy(&(track.track.object), &(nresolve->object), sizeof(SaNameT));
			/*  m_NTOH_SANAMET_LEN(track.track.object.length); */
			track.track.val = ASAPi_TRACK_ENABLE;
			track.dest = info->dest;
			track.to_svc = info->to_svc;
			/* Send async update to the stand by for MQD redundancy */
			mqd_a2s_async_update(pMqd, MQD_A2S_MSG_TYPE_TRACK, (void *)&track);
			TRACE_1("Database Operation (TRACK) Added");
		}
	}

	if (MQSV_OBJ_QGROUP == pObjNode->oinfo.type) {
		memcpy(&msg.info.nresp.oinfo.group, &pObjNode->oinfo.name, sizeof(SaNameT));
		/* m_NTOH_SANAMET_LEN(msg.info.nresp.oinfo.group.length); */
		msg.info.nresp.oinfo.policy = pObjNode->oinfo.info.qgrp.policy;
	}

	/* Make Queue List */
	rc = mqd_asapi_queue_make(&pObjNode->oinfo, &pQueue, &qcnt, true);
	if (NCSCC_RC_SUCCESS != rc) {
		msg.info.nresp.err.flag = true;
		msg.info.nresp.err.errcode = rc;
	} else {
		msg.info.nresp.oinfo.qcnt = qcnt;
		msg.info.nresp.oinfo.qparam = pQueue;
	}

 send_resp:
	/* Send the ASAPi Name Resolution Response message */
	msg.msgtype = ASAPi_MSG_NRESOLVE_RESP;
	rc = mqd_asapi_resp_send(&msg, info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi Name Resolution Response Message");
	} else {
		TRACE_1("ASAPi Name Resolution Response Message Sent Success");
	}

	/* Free up the queue information */
	if (pQueue) {
		m_MMGR_FREE_ASAPi_DEFAULT_VAL(pQueue, asapi.my_svc_id);
	}
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_nresolve_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_getqueue_hdlr

   DESCRIPTION    :  This is a request handler for the ASAPi Getqueue 
                     request. 
                   
   ARGUMENTS      :  pMqd     - MQD Controll block pointer
                     getqueue - ASAPi Getqueue Message                                          
                     info     - Send Information

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t mqd_asapi_getqueue_hdlr(MQD_CB *pMqd, ASAPi_GETQUEUE_INFO *getqueue, MQSV_SEND_INFO *info)
{
	bool exist = false;
	MQD_OBJ_NODE *pObjNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO msg;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(msg));

	if (pMqd->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("%s:%u: ERR_RESOURCES: AMF is Not in Active State",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NO_RESOURCES;

		/* Set the error flag and fill the description */
		msg.info.nresp.err.flag = true;
		msg.info.nresp.err.errcode = rc;

		/* Log suitable msg */
		goto send_resp;
	}

	/* Validate the object */
	exist = mqd_asapi_obj_validate(pMqd, &getqueue->queue, &pObjNode);
	if (!exist) {		/* Object doesn't exist ... */
		LOG_ER("%s:%u: ERR_NOT_EXIST: Database Operation (UPD) Failed",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;

		/* Set the error flag and fill the description */
		msg.info.vresp.err.flag = true;
		msg.info.vresp.err.errcode = rc;

		goto send_resp;
	}

	/* Check if queue name passed is actually a Q-Group */
	if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QGROUP)) {
		LOG_ER("%s:%u: ERR_NOT_EXIST: Database Operation (UPD) Failed",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;

		/* Set the error flag and fill the description */
		msg.info.vresp.err.flag = true;
		msg.info.vresp.err.errcode = rc;

		goto send_resp;
	}

	if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE) {
		if (pObjNode->oinfo.info.q.is_mqnd_down) {
			LOG_ER("%s:%u: ERR_TRY_AGAIN: The corresponding MQND is down", __FILE__, __LINE__);
			rc = SA_AIS_ERR_TRY_AGAIN;

			/* Set the error flag and fill the description */
			msg.info.vresp.err.flag = true;
			msg.info.vresp.err.errcode = rc;

			goto send_resp;
		}
	}

	/* Polulate the queue fields */
	memcpy(&msg.info.vresp.queue.name, &pObjNode->oinfo.name, sizeof(SaNameT));
	/*m_NTOH_SANAMET_LEN(msg.info.vresp.queue.name.length); */
	mqd_qparam_fill(&pObjNode->oinfo.info.q, &msg.info.vresp.queue);

 send_resp:
	/* Send the ASAPi Getqueue Response message */
	msg.msgtype = ASAPi_MSG_GETQUEUE_RESP;
	rc = mqd_asapi_resp_send(&msg, info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi Getqueue Response Message");
		return rc;
	}
	TRACE_1("ASAPi Getqueue Response Message Sent");

	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_getqueue_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_track_hdlr

   DESCRIPTION    :  This is a request handler for the ASAPi Track request. 
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     track - ASAPi Track Message                                          
                     info  - Send Information

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t mqd_asapi_track_hdlr(MQD_CB *pMqd, ASAPi_TRACK_INFO *track, MQSV_SEND_INFO *info)
{
	bool exist = false;
	MQD_OBJ_NODE *pObjNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO msg;
	ASAPi_QUEUE_PARAM *pQueue = 0;
	uint16_t qcnt = 0;
	MQD_A2S_TRACK_INFO async_track;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(msg));
	memset(&async_track, 0, sizeof(async_track));

	if (pMqd->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("%s:%u: ERR_RESOURCES: AMF is Not in Active State",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NO_RESOURCES;

		/* Set the error flag and fill the description */
		msg.info.tresp.err.flag = true;
		msg.info.tresp.err.errcode = rc;

		/* Log suitable msg */
		goto send_resp;
	}

	/* Validate the object */
	exist = mqd_asapi_obj_validate(pMqd, &track->object, &pObjNode);
	if (!exist) {		/* Object doesn't exist ... */
		LOG_ER("%s:%u: ERR_NOT_EXIST: Database Operation (UPD) Failed",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;

		/* Set the error flag and fill the description */
		msg.info.tresp.err.flag = true;
		msg.info.tresp.err.errcode = rc;

		goto send_resp;
	}

	/* check if QueueGroup passed is actually a Queue */
	if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QUEUE)) {
		LOG_ER("%s:%u: ERR_NOT_EXIST: Database Operation (UPD) Failed",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;

		/* Set the error flag and fill the description */
		msg.info.tresp.err.flag = true;
		msg.info.tresp.err.errcode = rc;

		goto send_resp;
	}

	rc = mqd_asapi_track_db_upd(pMqd, track, info, &pObjNode);

	if (NCSCC_RC_SUCCESS != rc) {
		msg.info.tresp.err.flag = true;
		msg.info.tresp.err.errcode = rc;
		goto send_resp;
	}

	if (m_ASAPi_TRACK_IS_ENABLE(track->val)) {
		if (rc == NCSCC_RC_SUCCESS) {
			async_track.dest = info->dest;
			async_track.to_svc = info->to_svc;
			async_track.track = *track;

			/* Send async update to the standby for MQD redundancy */
			mqd_a2s_async_update(pMqd, MQD_A2S_MSG_TYPE_TRACK, (void *)&async_track);
		}

		/* Get Queue Information */
		rc = mqd_asapi_queue_make(&pObjNode->oinfo, &pQueue, &qcnt, true);

		if (NCSCC_RC_SUCCESS != rc) {
			msg.info.tresp.err.flag = true;
			msg.info.tresp.err.errcode = rc;
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_2("The function queue_make making of Queue list for update"
					"of QUEUE is failed with error %u",rc);
			else
				TRACE_2("The function queue_make making of Queue list for update"
					"of QUEUE GROUPS is failed with error %u",rc);
		} else {
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_1("The function queue_make making of Queue list for update"
					"of QUEUE Success");
			else
				TRACE_1("The function queue_make making of Queue list for update"
					"of QUEUE GROUPS Success");
		}
	} else if (m_ASAPi_TRACK_IS_DISABLE(track->val)) {
		if (rc == NCSCC_RC_SUCCESS) {
			async_track.dest = info->dest;
			async_track.to_svc = info->to_svc;
			async_track.track = *track;

			/* Send async update to the standby for MQD redundancy */
			mqd_a2s_async_update(pMqd, MQD_A2S_MSG_TYPE_TRACK, (void *)&async_track);
		}
	}

	if (MQSV_OBJ_QGROUP == pObjNode->oinfo.type) {
		memcpy(&msg.info.tresp.oinfo.group, &pObjNode->oinfo.name, sizeof(SaNameT));
		/*m_NTOH_SANAMET_LEN(msg.info.tresp.oinfo.group.length); */
		msg.info.tresp.oinfo.policy = pObjNode->oinfo.info.qgrp.policy;
	}

 send_resp:
	/* Send the ASAPi Track Response message */
	msg.info.tresp.oinfo.qcnt = qcnt;
	msg.info.tresp.oinfo.qparam = pQueue;
	msg.msgtype = ASAPi_MSG_TRACK_RESP;

	rc = mqd_asapi_resp_send(&msg, info);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi Track Response Message");
		return rc;
	}

	TRACE_1("ASAPi Track Response Message Sent Success");
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_track_hdlr() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_track_db_upd

   DESCRIPTION    :  This routine updates the database based on the pre-existenc
                    -of the queue object. If the object is already present then
                     if just updates the param.
                    If the track is enabled then the node is added into the track
                   list.Else the object is deleted from the tracklist. 
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     track - ASAPi Track Message                                                     
                     info  - Send Info
                     onode - Object noden

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_asapi_track_db_upd(MQD_CB *pMqd, ASAPi_TRACK_INFO *track, MQSV_SEND_INFO *info, MQD_OBJ_NODE **onode)
{
	bool exist = false;
	MQD_OBJ_NODE *pObjNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Validate the object */
	exist = mqd_asapi_obj_validate(pMqd, &track->object, &pObjNode);
	if (!exist) {		/* Object doesn't exist ... */
		LOG_ER("%s:%u: ERR_NOT_EXIST: Database Operation (UPD) Failed",__FILE__,__LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;
		return rc;
	}

	/* Update the tracking information */
	if (m_ASAPi_TRACK_IS_ENABLE(track->val)) {
		/* Enable tracking for the sender vis-a-vis object */
		rc = mqd_track_add(&pObjNode->oinfo.tlist, &info->dest, info->to_svc);
		if (rc == NCSCC_RC_SUCCESS) {
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_1("Track QUEUE DB Update is Success");
			else
				TRACE_1("Track QUEUE GROUPS DB Update is Success");
		} else {
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_4("Track QUEUE DB Update is Failed with error %u", rc);
			else
				TRACE_4("Track QUEUE GROUPS DB Update is Failed with error %u", rc);
		}
	} else if (m_ASAPi_TRACK_IS_DISABLE(track->val)) {
		/* Disable tracking for the sender vis-a-vis object */
		rc = mqd_track_del(&pObjNode->oinfo.tlist, &info->dest);
		if (rc == NCSCC_RC_SUCCESS) {
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_1("TrackStop QUEUE DB Update is Success");
			else
				TRACE_1("TrackStop QUEUE GROUPS DB Update is Success");
		} else {
			if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE)
				TRACE_4("TrackStop QUEUE DB Update is Failed with error %u", rc);
			else
				TRACE_4("TrackStop QUEUE GROUPS DB Update is Failed with error %u", rc);
		}
	}
	*onode = pObjNode;
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_track_db_upd() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_track_ntfy_send

   DESCRIPTION    :  This routines sends track notification ASAPi message to
                     all the user who has opted for track corresponding to the
                     object. 
                   
   ARGUMENTS      :  pObjNode - Object node
                     opr      - Track operation
 
   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
static uint32_t mqd_asapi_track_ntfy_send(MQD_OBJ_INFO *pObjInfo, ASAPi_OBJECT_OPR opr)
{
	ASAPi_QUEUE_PARAM *pQueue = 0;
	uint16_t qcnt = 0;
	ASAPi_MSG_INFO msg;
	MQSV_SEND_INFO info = {0};
	NCS_Q_ITR itr;
	MQD_TRACK_OBJ *pTrack = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t cons_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(msg));

	if (MQSV_OBJ_QGROUP == pObjInfo->type) {
		memcpy(&msg.info.tntfy.oinfo.group, &pObjInfo->name, sizeof(SaNameT));
		/*m_NTOH_SANAMET_LEN(msg.info.tntfy.oinfo.group.length);   */
		msg.info.tntfy.oinfo.policy = pObjInfo->info.qgrp.policy;
	}

	/* Make Queue List */

	rc = mqd_asapi_queue_make(pObjInfo, &pQueue, &qcnt, false);
	if (NCSCC_RC_SUCCESS != rc) {
		if (pObjInfo->type == MQSV_OBJ_QUEUE)
				TRACE_2("The function queue_make making of Queue list for update"
					"of QUEUE is failed with error %u",rc);
		else
			TRACE_2("The function queue_make making of Queue list for update"
				"of QUEUE GROUPS is failed with error %u",rc);
		return rc;
	} else {
		if (pObjInfo->type == MQSV_OBJ_QUEUE)
			TRACE_1("The function queue_make making of Queue list for update"
				"of QUEUE Success");
		else
			TRACE_1("The function queue_make making of Queue list for update"
				"of QUEUE GROUPS Success");
	}

	msg.info.tntfy.oinfo.qcnt = qcnt;
	msg.info.tntfy.oinfo.qparam = pQueue;

	/* Send Track Notification message */
	msg.info.tntfy.opr = opr;
	msg.msgtype = ASAPi_MSG_TRACK_NTFY;

	/* Send Track Notification to all the user opted for the notification */
	itr.state = 0;
	while ((pTrack = (MQD_TRACK_OBJ *)ncs_walk_items(&pObjInfo->tlist, &itr))) {
		info.to_svc = pTrack->to_svc;
		info.dest = pTrack->dest;
		info.stype = MDS_SENDTYPE_SND;	/* Send */

		rc = mqd_asapi_resp_send(&msg, &info);
		if (NCSCC_RC_SUCCESS != rc) {
			if (pObjInfo->type == MQSV_OBJ_QUEUE)
				LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi QUEUE Track Notification");
			else
				LOG_ER("ERR_FAILED_OPERATION: Couldn't Send ASAPi QUEUE GROUPS Track Notification");
			if (cons_rc == NCSCC_RC_SUCCESS)
				cons_rc = rc;
			continue;	/* Continue sending to the other dests */
		}
		if (pObjInfo->type == MQSV_OBJ_QUEUE)
			TRACE_1("ASAPi QUEUE Track Notification Sent Success");
		else
			TRACE_1("ASAPi QUEUE GROUPS Track Notification Sent Success");
	}

	/* Free up the Queue information */
	if (pQueue) {
		m_MMGR_FREE_ASAPi_DEFAULT_VAL(pQueue, asapi.my_svc_id);
	}

	TRACE_LEAVE();
	return cons_rc;
}	/* End of mqd_asapi_track_ntfy_send() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_resp_send

   DESCRIPTION    :  This routines fill the necessary params for the ASAPi 
                     response message and invokes the ASAPi handler to send
                     the message to the specified destination.
                   
   ARGUMENTS      :  msg   - ASAPi message
                     indo  - Send information

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.                     
\****************************************************************************/
static uint32_t mqd_asapi_resp_send(ASAPi_MSG_INFO *msg, MQSV_SEND_INFO *info)
{
	ASAPi_OPR_INFO opr;
	uint32_t rc = NCSCC_RC_SUCCESS;

	opr.type = ASAPi_OPR_MSG;
	opr.info.msg.opr = ASAPi_MSG_SEND;
	opr.info.msg.req = *msg;
	opr.info.msg.sinfo = *info;
	rc = asapi_opr_hdlr(&opr);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("Sending the message to the specified destination with error %u", rc);
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	return rc;
}

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_queue_make

   DESCRIPTION    :  This routines makes queue information from the 
                     database. It makes list of queues, depending whether the 
                     queue is stand alone or part of the group
                   
   ARGUMENTS      :  pObjNode - Object node
                     o_queue  - Queue list
                     o_cnt    - Queue count 
                     select   - true means all, FALSe means on the one which 
                                marked

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     <ERR_CODE> Specific errors
\****************************************************************************/
static uint32_t mqd_asapi_queue_make(MQD_OBJ_INFO *pObjInfo, ASAPi_QUEUE_PARAM **o_queue, uint16_t *o_cnt, bool select)
{
	uint16_t qcnt = 0;
	ASAPi_QUEUE_PARAM *pQueue = 0;
	MQD_OBJECT_ELEM *pOelm = 0;
	uint32_t idx = 0;
	NCS_Q_ITR itr;
	TRACE_ENTER();

	*o_cnt = 0;
	*o_queue = 0;

	if (MQSV_OBJ_QUEUE == pObjInfo->type) {
		pQueue = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
		if (!pQueue) {
			if (pObjInfo->type == MQSV_OBJ_QUEUE)
				LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory for Queue", __FILE__, __LINE__);
			else
				LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory for Queue Groups",__FILE__,__LINE__);
			return SA_AIS_ERR_NO_MEMORY;
		}

		/* Polulate the queue fields */
		memcpy(&pQueue->name, &pObjInfo->name, sizeof(SaNameT));
		/*m_NTOH_SANAMET_LEN(pQueue->name.length);        */
		mqd_qparam_fill(&pObjInfo->info.q, pQueue);
		qcnt = 1;
	} else if ((MQSV_OBJ_QGROUP == pObjInfo->type) && (!select)) {
		itr.state = 0;
		while ((pOelm = (MQD_OBJECT_ELEM *)ncs_queue_get_next(&pObjInfo->ilist, &itr))) {
			if (pOelm->pObject->info.q.adv) {	/* Check if we need to advertise the Queue */
				pOelm->pObject->info.q.adv = false;	/* Reset the Advertisement flag */
				pQueue = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
				if (!pQueue) {
					if (pObjInfo->type == MQSV_OBJ_QUEUE)
						LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory",__FILE__,__LINE__);
					else
						LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory",__FILE__,__LINE__);
					return SA_AIS_ERR_NO_MEMORY;
				}

				/* Get the queue params */
				memcpy(&pQueue->name, &pOelm->pObject->name, sizeof(SaNameT));
				mqd_qparam_fill(&pOelm->pObject->info.q, pQueue);
				qcnt = 1;
				break;
			}
		}
	} else if ((MQSV_OBJ_QGROUP == pObjInfo->type) && (select)) {
		qcnt = pObjInfo->ilist.count;
		if (qcnt) {
			pQueue = m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(qcnt * sizeof(ASAPi_QUEUE_PARAM), asapi.my_svc_id);
			if (!pQueue) {
				if (pObjInfo->type == MQSV_OBJ_QUEUE)
					LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory for Queue",__FILE__,__LINE__);
				else
					LOG_CR("%s:%u:ERR_MEMORY:Failed To Allocate Memory for QGroups",__FILE__,__LINE__);
					return SA_AIS_ERR_NO_MEMORY;
				return SA_AIS_ERR_NO_MEMORY;
			}

			itr.state = 0;
			for (idx = 0; idx < qcnt; idx++) {
				pOelm = (MQD_OBJECT_ELEM *)ncs_walk_items(&pObjInfo->ilist, &itr);

				memcpy(&pQueue[idx].name, &pOelm->pObject->name, sizeof(SaNameT));
				mqd_qparam_fill(&pOelm->pObject->info.q, &pQueue[idx]);
			}
		}
	} else {
		LOG_ER("ERR_INVALID_PARAM: MQD_OBJ_INFO type does not match %d", pObjInfo->type);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	*o_cnt = qcnt;
	*o_queue = pQueue;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_db_upd

   DESCRIPTION    :  This routines updates the database based on the 
                     pre-existence of the object. If the object is already 
                     present then it just updates the param otherwise it 
                     addes an new entry in the DB and updates the params. 
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     reg   - ASAPi Registration Message
                     onode - Object node 
                     opr   - Operation type

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     <ERR_CODE> Specific errors
\****************************************************************************/
uint32_t mqd_asapi_db_upd(MQD_CB *pMqd, ASAPi_REG_INFO *reg, MQD_OBJ_NODE **onode, ASAPi_OBJECT_OPR *opr)
{
	MQD_OBJ_NODE *pObjNode = 0, *pQNode = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT error;
	TRACE_ENTER();

	switch (reg->objtype) {
		/* Insert queue into a QueueGroup */
	case ASAPi_OBJ_BOTH:
		{
			MQD_OBJECT_ELEM *pOelm = 0;

			/* Check for existence of Queue & QueueGroup */
			if ((!mqd_asapi_obj_validate(pMqd, &reg->group, &pObjNode))
			    || (!mqd_asapi_obj_validate(pMqd, &reg->queue.name, &pQNode))) {
				TRACE_2("ERR_NOT_EXIST: Existence of Queue & QueueGroup failed");
				rc = SA_AIS_ERR_NOT_EXIST;
				return rc;
			}

			/* check if QueueGroup passed is actually a Queue */
			if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QUEUE))
				return SA_AIS_ERR_NOT_EXIST;

			/* check if Queue passed is actually QueueGroup */
			if (mqd_check_for_namespace_collision(pQNode, MQSV_OBJ_QGROUP))
				return SA_AIS_ERR_NOT_EXIST;

			/* Check if Queue with the name within that group exist */
			pOelm = ncs_find_item(&pObjNode->oinfo.ilist, &reg->queue.name, mqd_obj_cmp);

			if (!pOelm) {
				/* Add the new queue to be the member of the Q-Group */
				pOelm = m_MMGR_ALLOC_MQD_OBJECT_ELEM;
				if (!pOelm) {
					rc = SA_AIS_ERR_NO_MEMORY;
					LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
					return SA_AIS_ERR_NO_MEMORY;
				}

				memset(pOelm, 0, sizeof(MQD_OBJECT_ELEM));

				pOelm->pObject = &pQNode->oinfo;
				ncs_enqueue(&pObjNode->oinfo.ilist, pOelm);
				*opr = ASAPi_QUEUE_ADD;

				/* This queue needs to be advertised on Active side of MQD */
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					pQNode->oinfo.info.q.adv = true;
				}
				/* Add the Q-Group to the queue */
				pOelm = m_MMGR_ALLOC_MQD_OBJECT_ELEM;
				if (!pOelm) {
					rc = SA_AIS_ERR_NO_MEMORY;
					LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
					return SA_AIS_ERR_NO_MEMORY;
				}

				memset(pOelm, 0, sizeof(MQD_OBJECT_ELEM));

				pOelm->pObject = &pObjNode->oinfo;
				ncs_enqueue(&pQNode->oinfo.ilist, pOelm);

				/* Update Runtime Attribute to IMMSV */
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					immutil_update_one_rattr(pMqd->immOiHandle, (char *)pObjNode->oinfo.name.value,
								 "saMsgQueueGroupNumQueues", SA_IMM_ATTR_SAUINT32T,
								 &pObjNode->oinfo.ilist.count);
					mqd_runtime_update_grpmembers_attr(pMqd, pObjNode);
				}
				TRACE_1("Database Operation (ADD) Success");
			} else {
				/* The queue is already the member of the group, identified by the
				 * group name */
				TRACE_2("The queue is already member of the queue group");
				return SA_AIS_ERR_EXIST;
			}
			TRACE_1("Group Insert is Successfull");
		}
		break;

		/* QueueGroup create */
	case ASAPi_OBJ_GROUP:
		{
			/* Check for existence of QueueGroup */
			if (mqd_asapi_obj_validate(pMqd, &reg->group, &pObjNode)) {
				rc = SA_AIS_ERR_EXIST;
				TRACE_2("ERR_EXIST: The queue is already member of the queue group");
				return rc;
			}

			/* Create Object Node */
			rc = mqd_db_node_create(pMqd, &pObjNode);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
				return SA_AIS_ERR_NO_MEMORY;
			}
			memcpy(&pObjNode->oinfo.name, &reg->group, sizeof(SaNameT));
			pObjNode->oinfo.type = MQSV_OBJ_QGROUP;
			pObjNode->oinfo.info.qgrp.policy = reg->policy;

			*opr = ASAPi_GROUP_ADD;	/* This is Group create */
			if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
				error = mqd_create_runtime_MqGrpObj(pObjNode, pMqd->immOiHandle);
				if (error != SA_AIS_OK) {
					TRACE_2("Creation of MqGrpobj FAILED: %u", error);
					return NCSCC_RC_FAILURE;
				}
			}

			/* Add the object node */
			rc = mqd_db_node_add(pMqd, pObjNode);
			if (NCSCC_RC_SUCCESS != rc) {
				TRACE_2("Queue Group Creation at the MQD is Failed");
			} else {
				TRACE_1("Queue Group Creation at the MQD is Successfull");
			}
		}
		break;

		/* Queue create/update */
	case ASAPi_OBJ_QUEUE:
		{
			/* if Queue exists */
			if (mqd_asapi_obj_validate(pMqd, &reg->queue.name, &pObjNode)) {

				/* check if the queue is actually a Q-Group */
				if (mqd_check_for_namespace_collision(pObjNode, MQSV_OBJ_QGROUP)) {
					TRACE_2("ERR_EXIST: The queue is already member of the queue group");
					rc = SA_AIS_ERR_EXIST;
					return rc;
				}

				/* Update the Queue params */
				mqd_qparam_upd(pObjNode, &reg->queue);

				/* This queue needs to be advertised */
				*opr = ASAPi_QUEUE_UPD;
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					pObjNode->oinfo.info.q.adv = true;
				}
				TRACE_1("Queue updation at the MQD is Successfull");
			} else {

				/* Create Object Node */
				rc = mqd_db_node_create(pMqd, &pObjNode);
				if (NCSCC_RC_SUCCESS != rc) {
					TRACE_4("ERR_MEMORY: Memory Allocation failed");
					return SA_AIS_ERR_NO_MEMORY;
				}

				memcpy(&pObjNode->oinfo.name, &reg->queue.name, sizeof(SaNameT));
				pObjNode->oinfo.type = MQSV_OBJ_QUEUE;

				/* Update the Queue params */
				mqd_qparam_upd(pObjNode, &reg->queue);

				/* This queue needs to be advertised */
				*opr = ASAPi_QUEUE_ADD;
				if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
					pObjNode->oinfo.info.q.adv = true;
				}
				/* Add the object node */
				rc = mqd_db_node_add(pMqd, pObjNode);
				if (NCSCC_RC_SUCCESS != rc) {
					TRACE_2("Queue Creation at the MQD is Failed");
				} else
					TRACE_1("Queue Creation at the MQD is Success");
			}
		}
		break;

	default:
		TRACE_2("ERR_INVALID_PARAM: ASAPi_REG_INFO type does not match %d", reg->objtype);
		return SA_AIS_ERR_INVALID_PARAM;
		break;
	}

	*onode = pObjNode;
	TRACE_LEAVE();
	return rc;
}	/* End of mqd_asapi_db_upd() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_asapi_obj_validate

   DESCRIPTION    :  This routines validates the name passed to function by 
                     checking the existance of the object in the cluster.
                     If the object corresponding to the name exist then it 
                     returns the object otherwise NULL.
                   
   ARGUMENTS      :  pMqd   - MQD Controll block pointer
                     name   - object name
                     o_node - object node

   RETURNS        :  true  - Object exist
                     false - Object doesn't exist
\****************************************************************************/
static bool mqd_asapi_obj_validate(MQD_CB *pMqd, SaNameT *name, MQD_OBJ_NODE **o_node)
{
	MQD_OBJ_NODE *pObjNode = 0;

	/* Get hold of the MQD controll block */
	pObjNode = (MQD_OBJ_NODE *)ncs_patricia_tree_get(&pMqd->qdb, (uint8_t *)name);

	*o_node = pObjNode;
	if (pObjNode)
		return true;
	return false;
}	/* End of mqd_asapi_obj_validate() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_obj_cmp

   DESCRIPTION    :  This routines is invoked to compare the object in the list 
   
   ARGUMENTS      :  key   - what to match
                     elem  - with whom to match
   
   RETURNS        :  true(If sucessfully matched)/FALSE(No match)                     
\****************************************************************************/
bool mqd_obj_cmp(void *key, void *elem)
{
	MQD_OBJECT_ELEM *pOelm = (MQD_OBJECT_ELEM *)elem;
	SaNameT *local_key = (SaNameT *)key;

	if (pOelm->pObject->name.length == local_key->length) {
		if (!memcmp(pOelm->pObject->name.value, local_key->value, local_key->length)) {
			return true;
		}
	}
	return false;
}	/* End of mqd_obj_cmp() */

/****************************************************************************\
 PROCEDURE NAME : mqd_nd_restart_update_dest_info

 DESCRIPTION    : This routine updates the queue information with the latest mqnd destination information.

 ARGUMENTS      : pMqd - MQD Control block pointer
                  dest - New MDS Destination of the MQND

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
\*****************************************************************************/
void mqd_nd_restart_update_dest_info(MQD_CB *pMqd, MDS_DEST dest)
{
	MQD_OBJ_NODE *pObjNode = 0;
	SaNameT name;
	MQD_OBJECT_ELEM *pOelm = 0;
	NCS_Q_ITR itr;
	uint32_t count = 0;

	pObjNode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)NULL);
	while (pObjNode) {
		if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE) {
			if (m_NCS_NODE_ID_FROM_MDS_DEST(pObjNode->oinfo.info.q.dest) ==
			    m_NCS_NODE_ID_FROM_MDS_DEST(dest)) {
				pObjNode->oinfo.info.q.dest = dest;
				pObjNode->oinfo.info.q.is_mqnd_down = false;
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_MQND_UP);
			}
		} else {
			itr.state = 0;
			while (count < pObjNode->oinfo.ilist.count) {
				count++;
				while ((pOelm = (MQD_OBJECT_ELEM *)ncs_queue_get_next(&pObjNode->oinfo.ilist, &itr))) {
					if (pOelm) {
						if (pOelm->pObject->type == MQSV_OBJ_QUEUE) {
							if (m_NCS_NODE_ID_FROM_MDS_DEST(pOelm->pObject->info.q.dest)
							    == m_NCS_NODE_ID_FROM_MDS_DEST(dest)) {
								pOelm->pObject->info.q.adv = true;
								pOelm->pObject->info.q.is_mqnd_down = false;
								pOelm->pObject->info.q.dest = dest;
								break;
							}
						}
					}
				}	/* end of checking the ilist of the queue group */
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_MQND_UP);
			}
		}
		name = pObjNode->oinfo.name;
		pObjNode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&name);
	}
}

/****************************************************************************\
 PROCEDURE NAME : mqd_nd_down_update_info

 DESCRIPTION    : This routine updates the queue information with the latest mqnd destination information.

 ARGUMENTS      : pMqd - MQD Control block pointer
                  dest - New MDS Destination of the MQND

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
\*****************************************************************************/
void mqd_nd_down_update_info(MQD_CB *pMqd, MDS_DEST dest)
{
	MQD_OBJ_NODE *pObjNode = 0;
	SaNameT name;
	MQD_OBJECT_ELEM *pOelm = 0;
	NCS_Q_ITR itr;
	uint32_t count = 0;

	pObjNode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)NULL);
	while (pObjNode) {
		if (pObjNode->oinfo.type == MQSV_OBJ_QUEUE) {
			if (m_NCS_NODE_ID_FROM_MDS_DEST(pObjNode->oinfo.info.q.dest)
			    == m_NCS_NODE_ID_FROM_MDS_DEST(dest)) {
				pObjNode->oinfo.info.q.dest = dest;
				pObjNode->oinfo.info.q.is_mqnd_down = true;
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_MQND_DOWN);
			}
		} else {
			itr.state = 0;
			while (count < pObjNode->oinfo.ilist.count) {
				count++;
				while ((pOelm = (MQD_OBJECT_ELEM *)ncs_queue_get_next(&pObjNode->oinfo.ilist, &itr))) {
					if (pOelm) {
						if (pOelm->pObject->type == MQSV_OBJ_QUEUE) {
							if (m_NCS_NODE_ID_FROM_MDS_DEST(pOelm->pObject->info.q.dest)
							    == m_NCS_NODE_ID_FROM_MDS_DEST(dest)) {
								pOelm->pObject->info.q.is_mqnd_down = true;
								pOelm->pObject->info.q.adv = true;
								break;
							}
						}
					}
				}	/* end of checking the ilist of the queue group */
				mqd_asapi_track_ntfy_send(&pObjNode->oinfo, ASAPi_QUEUE_MQND_DOWN);
			}
		}
		name = pObjNode->oinfo.name;
		pObjNode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&name);
	}
	return;
}

/****************************************************************************\
   PROCEDURE NAME :  mqd_check_for_namespace_collision

   DESCRIPTION    :  This routines checks for namespace collition between Queues  
                     and QueueGroups.
                   
   ARGUMENTS      :  name   - object name
                     collide_type   - collision type (not the expected object type) 

   RETURNS        :  true  - namespace collision
                     false - no collision
\****************************************************************************/
static bool mqd_check_for_namespace_collision(MQD_OBJ_NODE *pObjNode, MQSV_OBJ_TYPE collide_type)
{
	if (collide_type & pObjNode->oinfo.type)
		return true;

	return false;
}
