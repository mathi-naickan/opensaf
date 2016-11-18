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

  FILE NAME: mqd_sbevt.c

..............................................................................

  DESCRIPTION:

  MQD  Standby processing functions.

******************************************************************************/

/***********************************************************************************************************
      Fucntions degfined in this file:
     mqd_process_a2s_event...................Process the events and call the respective event hanlder function
     mqd_process_a2s_register_req............Process the register request from active to standby
     mqd_process_a2s_deregister_req..........Process the deregister request from active to standby
     mqd_process_a2s_track_req...............Process the track request from active to standby
     mqd_process_a2s_queueinfo_req...........Process the queueinfo request from active to standby
     mqd_process_a2s_userevent_req...........Process the userevent request from active to standby
     mqd_process_a2s_mqnd_status_req.........Process the mqnd related timer information
     mqd_process_a2s_mqnd_timer_expiry_event.Process the mqnd timer expiry operation.
**************************************************************************************************************/

#include "msg/msgd/mqd.h"

typedef uint32_t (*MQD_PROCESS_A2S_EVENT_FUNC_PTR) (MQD_CB *pMqd, MQD_A2S_MSG msg);

static uint32_t mqd_process_a2s_register_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_deregister_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_track_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_queueinfo_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_userevent_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_mqnd_status_req(MQD_CB *pMqd, MQD_A2S_MSG msg);
static uint32_t mqd_process_a2s_mqnd_timer_expiry_event(MQD_CB *pMqd, MQD_A2S_MSG msg);

static const MQD_PROCESS_A2S_EVENT_FUNC_PTR mqd_process_a2s_event_handler[MQD_A2S_MSG_TYPE_MAX -
									  MQD_A2S_MSG_TYPE_BASE] = {
	mqd_process_a2s_register_req,
	mqd_process_a2s_deregister_req,
	mqd_process_a2s_track_req,
	mqd_process_a2s_queueinfo_req,
	mqd_process_a2s_userevent_req,
	mqd_process_a2s_mqnd_status_req,
	mqd_process_a2s_mqnd_timer_expiry_event
};

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_event
*
*  Description :  This is the function handler for the active to standby
*                 events.It checks the type of the event and invokes
*                 the respective function.
*  Parameters :   pMqd- Control block pointer
*                 msg- Active to Standby Update Message
*  Event Types : 
*                 MQD_A2S_MSG_TYPE_REG............Active to Standby Register request
*                 MQD_A2S_MSG_TYPE_DEREG..........Active to Standby Deregister request
*                 MQD_A2S_MSG_TYPE_TRACK..........Active to Standby Track request
*                 MQD_A2S_MSG_TYPE_QINFO..........Active to Standby ColdSync  request
*                 MQD_A2S_MSG_TYPE_USEREVT........Active to Standby User Event request
*                 MQD_A2S_MSG_TYPE_MQND_STATEVT...Active to Standby MQND Down/ Up event
*                 MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT.. Active to Standby MQND Timer Expiry event 
*            
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/

uint32_t mqd_process_a2s_event(MQD_CB *pMqd, MQD_A2S_MSG *msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (msg->type < MQD_A2S_MSG_TYPE_BASE || msg->type >= MQD_A2S_MSG_TYPE_MAX) {
		LOG_ER("Bad Active to Standby Message Type");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	rc = mqd_process_a2s_event_handler[msg->type - MQD_A2S_MSG_TYPE_BASE - 1] (pMqd, *msg);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Processing at Standby is failed");
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_queueinfo_req
*
*  Description :  This is the function is a dummy function to process the 
*                 Coldsync Datastructure event from 
*                 active to standby.
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_queueinfo_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;

	return rc;

}

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_register_req
*
*  Description :  This is the function to process the register request from the
*                 active to standby.   
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message 
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_register_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_OBJ_NODE *pObjNode = 0;
	ASAPi_OBJECT_OPR opr = 0;
	TRACE_ENTER();

	rc = mqd_asapi_db_upd(pMqd, (ASAPi_REG_INFO *)(&(msg.info.reg)), &pObjNode, &opr);
	if (!pObjNode) {
		LOG_ER("Standby Processing Queue with the given name is not present");
		rc = NCSCC_RC_FAILURE;
	}
	if (rc == NCSCC_RC_SUCCESS) {
		TRACE_1("Standby Processing Queue with the given name is not present");
	}
	TRACE_LEAVE();
	return rc;

}

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_deregister_req
*
*  Description :  This is the function to process the deregister request from the
*                 active to standby.   
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message 
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_deregister_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO mesg;
	TRACE_ENTER();

	memset(&mesg, 0, sizeof(mesg));

	rc = mqd_asapi_dereg_db_upd(pMqd, (ASAPi_DEREG_INFO *)(&(msg.info.dereg)), &mesg);

	if (rc == NCSCC_RC_SUCCESS) {
		LOG_ER("Standby Processing for DEREG message Success");
	} else
		TRACE_1("Standby Processing for DEREG message Failed with error %u", rc);
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_track_req
*
*  Description :  This is the function to process the track request from the
*                 active to standby.   
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message 
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_track_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_OBJ_NODE *pObjNode = 0;
	MQSV_SEND_INFO info;
	TRACE_ENTER();

	memset(&info, 0, sizeof(MQSV_SEND_INFO));

	info.dest = msg.info.track.dest;
	info.to_svc = msg.info.track.to_svc;
	rc = mqd_asapi_track_db_upd(pMqd, (ASAPi_TRACK_INFO *)(&(msg.info.track.track)), &info, &pObjNode);
	if (!pObjNode) {
		rc = NCSCC_RC_FAILURE;
		LOG_ER("Standby Processing Queue with the given name is not present");
	}
	if (rc == NCSCC_RC_SUCCESS)
		TRACE_1("Standby Processing for TRACK message Success");
	else
		TRACE_2("Standby Processing for TRACK message Failed with error %u", rc);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  Name :         mqd_process_a2s_userevent_req
*
*  Description :  This is the function to process the user event request from the
*                 active to standby.It will delete the tracking of the user who
*                 is down.   
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message 
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_userevent_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = mqd_user_evt_track_delete(pMqd, &msg.info.user_evt.dest);

	if (rc == NCSCC_RC_SUCCESS)
		TRACE_1("Standby Processing for USEREVT message success");
	return rc;
}

/*****************************************************************************\
*
*  Name :          mqd_process_a2s_mqnd_status_req 
*
*  Description :  This is the function to process MQND status event.
*                 It will start the Expiry timer for MQND which is down.
*                    
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD Register message 
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/
static uint32_t mqd_process_a2s_mqnd_status_req(MQD_CB *pMqd, MQD_A2S_MSG msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_ND_DB_NODE *pNdNode = 0;
	/*Update the node info structure and start the expiry timer for MQND */
	TRACE("mqd_process_a2s_mqnd_status_req ");
	if (msg.type == MQD_A2S_MSG_TYPE_MQND_STATEVT) {
		if (!msg.info.nd_stat_evt.is_restarting) {
			TRACE("A2S DOWN EVT PROCESSED");
		} else {
			pNdNode =
			    (MQD_ND_DB_NODE *)ncs_patricia_tree_get(&pMqd->node_db,
								    (uint8_t *)&msg.info.nd_stat_evt.nodeid);
			if (pNdNode) {
				mqd_tmr_stop(&pNdNode->info.timer);
				mqd_red_db_node_del(pMqd, pNdNode);

				TRACE("A2S UP EVT PROCESSED");
			}
		}
	}
	return rc;
}

/*****************************************************************************\
*
*  Name :          mqd_process_a2s_mqnd_timer_expiry_event
*
*  Description :  This is the function to process MQND timer expiry event.
*
*
*  Parameters :   pMqd- MQD Control Block Pointer
*                 msg - Active MQD to Standby MQD message
*  RETURNS:       SUCCESS - All went well
*                 FAILURE - Some thing went wrong
*
*****************************************************************************/

static uint32_t mqd_process_a2s_mqnd_timer_expiry_event(MQD_CB *pMqd, MQD_A2S_MSG msg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_ND_DB_NODE *pNdNode = 0;

	/* At standby if the timer expires do nothing */
	pNdNode = (MQD_ND_DB_NODE *)ncs_patricia_tree_get(&pMqd->node_db, (uint8_t *)&msg.info.nd_tmr_exp_evt.nodeid);

	if (pNdNode) {
		TRACE("mqd_process_a2s_mqnd_timer_expiry_event, pNdNode found");
		mqd_tmr_stop(&pNdNode->info.timer);
		mqd_red_db_node_del(pMqd, pNdNode);
	}
	TRACE("A2S TIMER EXPIRY EVT PROCESSED");
	return rc;
}
