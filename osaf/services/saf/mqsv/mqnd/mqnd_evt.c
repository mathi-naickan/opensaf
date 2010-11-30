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
  FILE NAME: mqsv_evt.c

  DESCRIPTION: MQND Event handling routines

  FUNCTIONS INCLUDED in this module:
  mqnd_process_evt .........MQND Event processing routine.
******************************************************************************/

#include <immutil.h>
#include "mqnd.h"

static uns32 mqnd_proc_mqp_req_msg(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_proc_mqp_rsp_msg(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_proc_mqnd_ctrl_msg(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_mqp_init(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_mqp_finalize(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_mqp_qopen(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_mqp_qclose(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_status_req(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_unlink(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_send_msg(MQND_CB *cb, MQSV_DSEND_EVT *evt);
static uns32 mqnd_evt_proc_qattr_get(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_evt_proc_update_stats_shm(MQND_CB *cb, MQSV_DSEND_EVT *evt);
static uns32 mqnd_evt_proc_cb_dump(void);
static uns32 mqnd_evt_proc_ret_time_set(MQND_CB *cb, MQSV_EVT *evt);
static void mqnd_dump_queue_status(MQND_CB *cb, SaMsgQueueStatusT *queueStatus, uns32 offset);
static void mqnd_dump_timer_info(MQND_TMR tmr);
void mqnd_process_dsend_evt(MQSV_DSEND_EVT *evt);
void mqnd_process_evt(MQSV_EVT *evt);
static uns32 mqnd_proc_mds_mqa_up(MQND_CB *cb, MQSV_EVT *evt);
static uns32 mqnd_proc_deferred_mqa_rsp(MQND_CB *cb);
static uns32 mqnd_enqueue_evt_cntxt_close(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err, uns32 qhdl);
static uns32 mqnd_enqueue_evt_cntxt_unlink(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err,
					   MQP_UNLINK_REQ *ulink_req);
extern MSG_FRMT_VER mqnd_mqa_msg_fmt_table[];
/*******************************************************************************/

void mqnd_process_evt(MQSV_EVT *evt)
{
	MQND_CB *cb;
	uns32 cb_hdl = m_MQND_GET_HDL();

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (cb == NULL) {
		m_MMGR_FREE_MQSV_EVT(evt, NCS_SERVICE_ID_MQND);
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE, __FILE__,
			      __LINE__);
		return;
	}

	switch (evt->type) {
	case MQSV_EVT_MQP_REQ:
		mqnd_proc_mqp_req_msg(cb, evt);
		break;

	case MQSV_EVT_MQP_RSP:
		mqnd_proc_mqp_rsp_msg(cb, evt);
		break;

	case MQSV_EVT_MQND_CTRL:
		mqnd_proc_mqnd_ctrl_msg(cb, evt);
		break;

	default:
		/* Log the error */
		/* m_LOG_MQND_EVT(evt->type, NCSFL_SEV_ERROR); */
		break;
	}

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	m_MMGR_FREE_MQSV_EVT(evt, NCS_SERVICE_ID_MQND);

	return;
}

void mqnd_process_dsend_evt(MQSV_DSEND_EVT *evt)
{
	MQND_CB *cb;
	uns32 cb_hdl = m_MQND_GET_HDL(), rc = NCSCC_RC_SUCCESS;

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (cb == NULL) {
		mds_free_direct_buff((MDS_DIRECT_BUFF)evt);
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE, __FILE__,
			      __LINE__);
		return;
	}

	switch (evt->type.req_type) {
	case MQP_EVT_SEND_MSG:
	case MQP_EVT_SEND_MSG_ASYNC:
		rc = mqnd_evt_proc_send_msg(cb, evt);
		break;

	case MQP_EVT_STAT_UPD_REQ:
		rc = mqnd_evt_proc_update_stats_shm(cb, evt);
		break;

	default:
		/* Log the error */
		/* m_LOG_MQND_EVT(evt->type, NCSFL_SEV_ERROR); */
		break;
	}

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	/* Free the Event */
	if ((evt->type.req_type == MQP_EVT_SEND_MSG_ASYNC) ||
	    (evt->type.req_type == MQP_EVT_SEND_MSG) || (evt->type.req_type == MQP_EVT_STAT_UPD_REQ))
		mds_free_direct_buff((MDS_DIRECT_BUFF)evt);

	return;
}

static uns32 mqnd_proc_mqp_req_msg(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	switch (evt->msg.mqp_req.type) {
	case MQP_EVT_INIT_REQ:
		rc = mqnd_evt_proc_mqp_init(cb, evt);
		break;
	case MQP_EVT_FINALIZE_REQ:
		rc = mqnd_evt_proc_mqp_finalize(cb, evt);
		break;
	case MQP_EVT_OPEN_REQ:
	case MQP_EVT_OPEN_ASYNC_REQ:
		rc = mqnd_evt_proc_mqp_qopen(cb, evt);
		break;
	case MQP_EVT_TRANSFER_QUEUE_REQ:
		rc = mqnd_evt_proc_mqp_qtransfer(cb, evt);
		break;
	case MQP_EVT_TRANSFER_QUEUE_COMPLETE:
		rc = mqnd_evt_proc_mqp_qtransfer_complete(cb, evt);
		break;
	case MQP_EVT_CLOSE_REQ:
		rc = mqnd_evt_proc_mqp_qclose(cb, evt);
		break;
	case MQP_EVT_STATUS_REQ:
		rc = mqnd_evt_proc_status_req(cb, evt);
		break;
	case MQP_EVT_UNLINK_REQ:
		rc = mqnd_evt_proc_unlink(cb, evt);
		break;
	case MQP_EVT_CB_DUMP:
		rc = mqnd_evt_proc_cb_dump();
		break;
	case MQP_EVT_Q_RET_TIME_SET_REQ:
		rc = mqnd_evt_proc_ret_time_set(cb, evt);
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}
	if (rc == NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_EVT_RECEIVED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_INFO, evt->msg.mqp_req.type, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_EVT_RECEIVED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, evt->msg.mqp_req.type, __FILE__,
			      __LINE__);
	return rc;
}

static uns32 mqnd_proc_mqp_rsp_msg(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc;

	switch (evt->msg.mqp_rsp.type) {
	case MQP_EVT_TRANSFER_QUEUE_RSP:
		rc = mqnd_evt_proc_mqp_qtransfer_response(cb, evt);
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	return rc;
}

static uns32 mqnd_proc_mqnd_ctrl_msg(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc;
	m_LOG_MQSV_ND(MQND_CTRL_EVT_RECEIVED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_INFO, evt->msg.mqnd_ctrl.type, __FILE__,
		      __LINE__);
	switch (evt->msg.mqnd_ctrl.type) {
	case MQND_CTRL_EVT_TMR_EXPIRY:
		rc = mqnd_evt_proc_tmr_expiry(cb, evt);
		break;

	case MQND_CTRL_EVT_QATTR_GET:
		rc = mqnd_evt_proc_qattr_get(cb, evt);
		break;

	case MQND_CTRL_EVT_MDS_INFO:
		rc = mqnd_proc_mqa_down(cb, &evt->msg.mqnd_ctrl.info.mds_info.dest);
		break;

	case MQND_CTRL_EVT_MDS_MQA_UP_INFO:
		rc = mqnd_proc_mds_mqa_up(cb, evt);
		break;

	case MQND_CTRL_EVT_DEFERRED_MQA_RSP:
		rc = mqnd_proc_deferred_mqa_rsp(cb);
		break;

	case MQND_CTRL_EVT_QATTR_INFO:
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}
	return rc;
}

/****************************************************************************
 * Name          : mqnd_proc_mqa_down
 *
 * Description   : Function to close all queues when MQA goes down.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MDS_DEST *mqa - MQA that went down.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_proc_mqa_down(MQND_CB *cb, MDS_DEST *mqa)
{
	MQND_QUEUE_NODE *qnode;
	uns32 qhdl;
	uns32 err = SA_AIS_OK;

	m_LOG_MQSV_ND(MQND_MQA_DOWN, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, m_NCS_NODE_ID_FROM_MDS_DEST(*mqa), __FILE__,
		      __LINE__);
	/* Traverse the Message Queue Database and close all the queues opened by
	   this application */
	mqnd_queue_node_getnext(cb, 0, &qnode);

	while (qnode) {
		qhdl = qnode->qinfo.queueHandle;

		if (m_NCS_MDS_DEST_EQUAL(&qnode->qinfo.rcvr_mqa, mqa) == 1) {
			/* Close this Queue */
			mqnd_proc_queue_close(cb, qnode, &err);
		}

		mqnd_queue_node_getnext(cb, qhdl, &qnode);
	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : mqnd_evt_proc_mqp_finalize
 *
 * Description   : Function to process the Finalize req from Applications
 *                 This will validate the Version & TBD.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_mqp_finalize(MQND_CB *cb, MQSV_EVT *evt)
{
	MQP_FINALIZE_REQ *final;
	uns32 rc = NCSCC_RC_SUCCESS;
	MQND_QUEUE_NODE *qnode;
	uns32 qhdl;
	uns32 err = SA_AIS_OK;
	MQSV_EVT send_evt;

	final = &evt->msg.mqp_req.info.finalReq;

	if (!cb->is_restart_done) {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}
	/* Traverse the Message Queue Database and close all the queues opened by
	   this application */
	mqnd_queue_node_getnext(cb, 0, &qnode);

	while (qnode) {
		qhdl = qnode->qinfo.queueHandle;

		if ((qnode->qinfo.msgHandle == final->msgHandle)
		    && (m_NCS_MDS_DEST_EQUAL(&qnode->qinfo.rcvr_mqa, &evt->msg.mqp_req.agent_mds_dest))) {
			/* Close this Queue */
			mqnd_proc_queue_close(cb, qnode, &err);	/* err returned by this function
								   is not handled in this routine */
			if (err != SA_AIS_OK)
				m_LOG_MQSV_ND(MQND_FINALIZE_CLOSE_STATUS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err,
					      __FILE__, __LINE__);
		}

		mqnd_queue_node_getnext(cb, qhdl, &qnode);
	}
 send_rsp:

	memset(&send_evt, 0, sizeof(MQSV_EVT));

	send_evt.type = MQSV_EVT_MQP_RSP;
	send_evt.msg.mqp_rsp.type = MQP_EVT_FINALIZE_RSP;
	send_evt.msg.mqp_rsp.error = err;
	rc = mqnd_mds_send_rsp(cb, &evt->sinfo, &send_evt);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_FINALIZE_RESP_SENT_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_FINALIZE_RESP_SENT_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
			      __LINE__);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_mqp_init
 *
 * Description   : Function to process the SaMsgInitialize from Applications
 *                 This will validate the Version & TBD.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_mqp_init(MQND_CB *cb, MQSV_EVT *evt)
{
	MQSV_EVT send_evt;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&send_evt, 0, sizeof(MQSV_EVT));

	/* Do the Version Check here MQSV:TBD */

	send_evt.type = MQSV_EVT_MQP_RSP;
	send_evt.msg.mqp_rsp.type = MQP_EVT_INIT_RSP;

	if (!cb->clm_node_joined) {
		send_evt.msg.mqp_rsp.error = SA_AIS_ERR_UNAVAILABLE;
	} else if (cb->is_restart_done) {
		send_evt.msg.mqp_rsp.error = SA_AIS_OK;
	} else {
		send_evt.msg.mqp_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO,
			      SA_AIS_ERR_TRY_AGAIN, __FILE__, __LINE__);
	}
	rc = mqnd_mds_send_rsp(cb, &evt->sinfo, &send_evt);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_INIT_RESP_SENT_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_INIT_RESP_SENT_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
			      __LINE__);

	return rc;

}

/****************************************************************************
 * Name          : mqnd_evt_proc_mqp_qopen
 *
 * Description   : Function to process the Queue open request from application.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_mqp_qopen(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	ASAPi_MSG_INFO *asapi_rsp = NULL;
	MQP_REQ_MSG *mqp_req = NULL;
	MQP_OPEN_REQ *open = NULL;
	SaAisErrorT err = SA_AIS_OK;
	ASAPi_OPR_INFO opr;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		mqp_req = &evt->msg.mqp_req;
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto error1;
	}
	/* Request the ASAPi (at MQD) for queue information */
	memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
	opr.type = ASAPi_OPR_MSG;
	opr.info.msg.opr = ASAPi_MSG_SEND;
	/* Fill MDS info */
	opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	opr.info.msg.sinfo.dest = cb->mqd_dest;
	opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	opr.info.msg.req.msgtype = ASAPi_MSG_NRESOLVE;

	mqp_req = &evt->msg.mqp_req;

	if (mqp_req->type == MQP_EVT_OPEN_REQ)
		open = &evt->msg.mqp_req.info.openReq;
	else if (mqp_req->type == MQP_EVT_OPEN_ASYNC_REQ)
		open = &evt->msg.mqp_req.info.openAsyncReq.mqpOpenReq;

	opr.info.msg.req.info.nresolve.object = open->queueName;
	opr.info.msg.req.info.nresolve.track = FALSE;

	/* Request the ASAPi */
	rc = asapi_opr_hdlr(&opr);

	if (rc == SA_AIS_ERR_TIMEOUT) {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_MDS_SND_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__, __LINE__);
		goto error2;
	}

	if ((rc != SA_AIS_OK) || (!opr.info.msg.resp) || ((opr.info.msg.resp->info.nresp.err.flag)
							  && (opr.info.msg.resp->info.nresp.err.errcode !=
							      SA_AIS_ERR_NOT_EXIST))) {
		/* log this error */
		if ((opr.info.msg.resp) && (opr.info.msg.resp->info.nresp.err.errcode == SA_AIS_ERR_TRY_AGAIN)) {
			err = SA_AIS_ERR_TRY_AGAIN;
			m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err,
				      __FILE__, __LINE__);
		} else {
			err = SA_AIS_ERR_NO_RESOURCES;
			m_LOG_MQSV_ND(MQND_ASAPI_NRESOLVE_HDLR_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err,
				      __FILE__, __LINE__);
		}
		goto error2;
	}

	asapi_rsp = opr.info.msg.resp;

	/* Check if the Queue name is actually a QueueGroup */
	if (asapi_rsp->info.nresp.oinfo.group.length != 0) {
		err = SA_AIS_ERR_EXIST;
		goto error2;
	}

	rc = mqnd_proc_queue_open(cb, mqp_req, &evt->sinfo, &asapi_rsp->info.nresp);

	/* Free the response Event */
	asapi_msg_free(&asapi_rsp);

	return rc;

 error2:
	if (opr.info.msg.resp)
		asapi_msg_free(&opr.info.msg.resp);

 error1:
	/* In case of Error, send the response to MQA */
	rc = mqnd_send_mqp_open_rsp(cb, &evt->sinfo, mqp_req, err, 0, 0);
	m_LOG_MQSV_ND(MQND_PROC_QUEUE_OPEN_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_mqp_qclose
 *
 * Description   : Function to process the queue close request.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_mqp_qclose(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_CLOSE_REQ *close = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;

	close = &evt->msg.mqp_req.info.closeReq;

	if (close == NULL) {
		err = SA_AIS_ERR_BAD_HANDLE;
		goto send_rsp;
	}

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	mqnd_queue_node_get(cb, close->queueHandle, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_BAD_HANDLE;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		goto send_rsp;
	}

	mqnd_proc_queue_close(cb, qnode, &err);

 send_rsp:
	if (err != SA_AIS_OK)
		m_LOG_MQSV_ND(MQND_PROC_QUEUE_CLOSE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_PROC_QUEUE_CLOSE_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
	if (!cb->is_mqd_up) {
		rc = mqnd_enqueue_evt_cntxt_close(cb, &evt->sinfo, err, close->queueHandle);
	} else
		rc = mqnd_send_mqp_close_rsp(cb, &evt->sinfo, err, close->queueHandle);

	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_QUEUE_CLOSE_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_QUEUE_CLOSE_RESP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
			      __LINE__);

	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_unlink
 *
 * Description   : Function to Unlink the 
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_unlink(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_UNLINK_REQ *ulink_req = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	ASAPi_OPR_INFO opr;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	MQND_QNAME_NODE *pnode = NULL;
	SaNameT qname;

	ulink_req = &evt->msg.mqp_req.info.unlinkReq;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	mqnd_queue_node_get(cb, ulink_req->queueHandle, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_NOT_EXIST;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		goto send_rsp;
	}

	if (qnode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE) {
		err = SA_AIS_ERR_NOT_EXIST;
		m_LOG_MQSV_ND(MQND_QUEUE_UNLINK_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	if (qnode->qinfo.owner_flag == MQSV_QUEUE_OWN_STATE_ORPHAN) {

		/* Request the ASAPi (at MQD) for queue DEREG */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
		opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;

		/* Request the ASAPi */
		rc = asapi_opr_hdlr(&opr);
		if (rc == SA_AIS_ERR_TIMEOUT) {
			/*This is the case when DEREG request is assumed to be lost or not proceesed at MQD becoz of failover.So
			   to compensate for the lost request we send an async request to MQD to process it once again to be
			   in sync with database at MQND */
			TRACE("UNLINK TIMEOUT:ASYNC DEREG TO DELETE ORPHAN QUEUE");
			memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
			opr.type = ASAPi_OPR_MSG;
			opr.info.msg.opr = ASAPi_MSG_SEND;

			/* Fill MDS info */
			opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
			opr.info.msg.sinfo.dest = cb->mqd_dest;
			opr.info.msg.sinfo.stype = MDS_SENDTYPE_SND;

			opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
			opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
			opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;
			rc = asapi_opr_hdlr(&opr);	/*May be insert a log */
		} else {
			if ((rc != SA_AIS_OK) || (!opr.info.msg.resp) || (opr.info.msg.resp->info.dresp.err.flag)) {
				m_LOG_MQSV_ND(MQND_ASAPI_DEREG_HDLR_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc,
					      __FILE__, __LINE__);
				err = SA_AIS_ERR_NO_RESOURCES;
				goto send_rsp;
			}
		}

		/* Free the response Event */
		if (opr.info.msg.resp)
			asapi_msg_free(&opr.info.msg.resp);

		if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &qnode->qinfo.queueName) != SA_AIS_OK) {
			mqnd_genlog(NCSFL_SEV_ERROR, "Deleting MessageQueue Object %s FAILED \n",
				    qnode->qinfo.queueName.value);
			return NCSCC_RC_FAILURE;
		}
		/* Delete the queue immediately */
		mqnd_mq_destroy(&qnode->qinfo);

		/* Delete the listener queue if it exists */
		mqnd_listenerq_destroy(&qnode->qinfo);

		/*Invalidate the shm area of the queue */
		mqnd_shm_queue_ckpt_section_invalidate(cb, qnode);

		/* Delete the mapping entry from the qname database */
		memset(&qname, 0, sizeof(SaNameT));
		qname = qnode->qinfo.queueName;
		mqnd_qname_node_get(cb, qname, &pnode);
		mqnd_qname_node_del(cb, pnode);
		if (pnode)
			m_MMGR_FREE_MQND_QNAME_NODE(pnode);

		/* Delete the Queue Node */
		mqnd_queue_node_del(cb, qnode);

		/* Free the Queue Node */
		m_MMGR_FREE_MQND_QUEUE_NODE(qnode);

	} else {
		/* Set the Sending state to unavilable */
		qnode->qinfo.sendingState = MSG_QUEUE_UNAVAILABLE;

		/* MQND Restart. In the else case, the sending state is updated.
		   Update the checkpointing service of this queue node updation */
		memset(&queue_ckpt_node, 0, sizeof(MQND_QUEUE_CKPT_INFO));
		mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

		mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
		if (rc != SA_AIS_OK) {
			m_LOG_MQSV_ND(MQND_CKPT_SECTION_OVERWRITE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc,
				      __FILE__, __LINE__);
			qnode->qinfo.sendingState = MSG_QUEUE_AVAILABLE;
			err = SA_AIS_ERR_NO_RESOURCES;
			goto send_rsp;
		}

		/* Request the ASAPi (at MQD) for queue DEREG */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
		opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;

		/* Request the ASAPi */
		rc = asapi_opr_hdlr(&opr);
		if (rc == SA_AIS_ERR_TIMEOUT) {
			/*This is the case when DEREG request is assumed to be lost or not proceesed at MQD becoz of failover.So
			   to compensate for the lost request we send an async request to MQD to process it once again to be
			   in sync with database at MQND */
			TRACE("UNLINK TIMEOUT:ASYNC DEREG AVAILABLE QUEUE TO UNAVAIL");
			memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
			opr.type = ASAPi_OPR_MSG;
			opr.info.msg.opr = ASAPi_MSG_SEND;

			/* Fill MDS info */
			opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
			opr.info.msg.sinfo.dest = cb->mqd_dest;
			opr.info.msg.sinfo.stype = MDS_SENDTYPE_SND;

			opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
			opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
			opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;

			rc = asapi_opr_hdlr(&opr);	/*May be insert a log */
		} else {
			if ((rc != SA_AIS_OK) || (!opr.info.msg.resp) || (opr.info.msg.resp->info.dresp.err.flag)) {
				m_LOG_MQSV_ND(MQND_ASAPI_DEREG_HDLR_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc,
					      __FILE__, __LINE__);
				qnode->qinfo.sendingState = MSG_QUEUE_AVAILABLE;
				mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

				mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
				if (rc != SA_AIS_OK) {
					m_LOG_MQSV_ND(MQND_CKPT_SECTION_OVERWRITE_FAILED, NCSFL_LC_MQSV_Q_MGMT,
						      NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
				}

				err = SA_AIS_ERR_NO_RESOURCES;
				goto send_rsp;
			}
		}

		/* Delete the mapping entry from the qname database */
		memset(&qname, 0, sizeof(SaNameT));
		qname = qnode->qinfo.queueName;
		mqnd_qname_node_get(cb, qname, &pnode);
		mqnd_qname_node_del(cb, pnode);
		if (pnode)
			m_MMGR_FREE_MQND_QNAME_NODE(pnode);

		/* Free the response Event */
		if (opr.info.msg.resp)
			asapi_msg_free(&opr.info.msg.resp);
	}

 send_rsp:
	/* Send the Response */
	if (!cb->is_mqd_up) {
		rc = mqnd_enqueue_evt_cntxt_unlink(cb, &evt->sinfo, err, ulink_req);
	} else
		rc = mqnd_send_mqp_ulink_rsp(cb, &evt->sinfo, err, ulink_req);

	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_QUEUE_UNLINK_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_QUEUE_UNLINK_RESP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
			      __LINE__);

	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_status_req
 *
 * Description   : Function to process the Status Request from MQA.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_status_req(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_QUEUE_STATUS_REQ *sts_req = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	MQSV_EVT rsp_evt;
	uns32 offset, i;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	sts_req = &evt->msg.mqp_req.info.statusReq;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	mqnd_queue_node_get(cb, sts_req->queueHandle, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_NOT_EXIST;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
	}

 send_rsp:
	/*Send the resp to MQA */
	memset(&rsp_evt, 0, sizeof(MQSV_EVT));
	rsp_evt.type = MQSV_EVT_MQP_RSP;
	rsp_evt.msg.mqp_rsp.error = err;

	rsp_evt.msg.mqp_rsp.type = MQP_EVT_STATUS_RSP;
	rsp_evt.msg.mqp_rsp.info.statusRsp.msgHandle = sts_req->msgHandle;
	rsp_evt.msg.mqp_rsp.info.statusRsp.queueHandle = sts_req->queueHandle;

	if (err == SA_AIS_OK) {
		shm_base_addr = cb->mqnd_shm.shm_base_addr;
		offset = qnode->qinfo.shm_queue_index;
		for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i <= SA_MSG_MESSAGE_LOWEST_PRIORITY; i++) {
			qnode->qinfo.queueStatus.saMsgQueueUsage[i].queueSize =
			    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].queueSize;
			qnode->qinfo.queueStatus.saMsgQueueUsage[i].queueUsed =
			    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].queueUsed;
			qnode->qinfo.queueStatus.saMsgQueueUsage[i].numberOfMessages =
			    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].numberOfMessages;
		}
		rsp_evt.msg.mqp_rsp.info.statusRsp.queueStatus = qnode->qinfo.queueStatus;
	}

	rc = mqnd_mds_send_rsp(cb, &evt->sinfo, &rsp_evt);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_QUEUE_STAT_REQ_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	else
		m_LOG_MQSV_ND(MQND_QUEUE_STAT_REQ_RESP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
			      __LINE__);

	return rc;

}

/****************************************************************************
 * Name          : mqnd_evt_proc_update_stats_shm
 *
 * Description   : Function to update stats of queue in shm when message is received.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_DSEND_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_update_stats_shm(MQND_CB *cb, MQSV_DSEND_EVT *evt)
{
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	MQP_UPDATE_STATS *statsReq;
	uns32 offset, rc = NCSCC_RC_SUCCESS, msg_fmt_ver;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	MQSV_DSEND_EVT *direct_rsp_evt = NULL;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(evt->msg_fmt_version,
						     MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
						     MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqnd_mqa_msg_fmt_table);

	msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(evt->src_dest_version,
					    MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
					    MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqnd_mqa_msg_fmt_table);

	/*Drop the messages with msg_fmt_version=1 as earlier versions are non backward compatible */
	if (!is_valid_msg_fmt || !msg_fmt_ver || (evt->msg_fmt_version == 1)) {
		/* Drop The Message */
		if (!is_valid_msg_fmt) {
			m_LOG_MQSV_ND(MQND_MSG_FRMT_VER_INVALID, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR,
				      is_valid_msg_fmt, __FILE__, __LINE__);
			TRACE("mqnd_evt_proc_update_stats_shm:INVALID MSG FORMAT:1 %d", is_valid_msg_fmt);
		}
		if (!msg_fmt_ver) {
			m_LOG_MQSV_ND(MQND_MSG_FRMT_VER_INVALID, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR,
				      msg_fmt_ver, __FILE__, __LINE__);
			TRACE("mqnd_evt_proc_update_stats_shm:INVALID MSG FORMAT:2 %d", msg_fmt_ver);
		}

		err = SA_AIS_ERR_VERSION;
		goto done;
	}

	statsReq = &evt->info.statsReq;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		rc = NCSCC_RC_FAILURE;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto done;
	}

	mqnd_queue_node_get(cb, statsReq->qhdl, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_BAD_HANDLE;
		rc = NCSCC_RC_FAILURE;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		goto done;
	}

	shm_base_addr = cb->mqnd_shm.shm_base_addr;
	offset = qnode->qinfo.shm_queue_index;

	if (shm_base_addr[offset].valid == SHM_QUEUE_INFO_VALID) {
		shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[statsReq->priority].queueUsed -= statsReq->size;
		shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[statsReq->priority].numberOfMessages--;
		shm_base_addr[offset].QueueStatsShm.totalQueueUsed -= statsReq->size;
		shm_base_addr[offset].QueueStatsShm.totalNumberOfMessages--;
	} else {
		err = SA_AIS_ERR_LIBRARY;
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:

	direct_rsp_evt = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(sizeof(MQSV_DSEND_EVT));
	if (!direct_rsp_evt) {
		m_LOG_MQSV_ND(MQND_MEM_ALLOC_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	memset(direct_rsp_evt, 0, sizeof(MQSV_DSEND_EVT));

	direct_rsp_evt->evt_type = MQSV_DSEND_EVENT;
	direct_rsp_evt->type.rsp_type = MQP_EVT_STAT_UPD_RSP;
	direct_rsp_evt->endianness = machineEndianness();
	direct_rsp_evt->msg_fmt_version = msg_fmt_ver;
	direct_rsp_evt->src_dest_version = MQND_PVT_SUBPART_VERSION;
	direct_rsp_evt->info.sendMsgRsp.error = err;

	rc = mqnd_mds_send_rsp_direct(cb, &evt->sinfo, direct_rsp_evt);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_MQSV_ND(MQND_MDS_SND_RSP_DIRECT_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, rc, __FILE__,
			      __LINE__);
	}

	return rc;

}

/****************************************************************************
 * Name          : mqnd_evt_proc_send_msg
 *
 * Description   : Function to process the Sent Message.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_send_msg(MQND_CB *cb, MQSV_DSEND_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS, msg_fmt_ver;
	MQP_SEND_MSG *snd_msg = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	MQSV_EVT rsp_evt;
	MQSV_DSEND_EVT *direct_rsp_evt = NULL;
	MQSV_MESSAGE *mqsv_msg = NULL;
	uns32 size, qsize = 0, qused = 0, actual_qsize = 0, actual_qused = 0, offset;
	NCS_OS_POSIX_MQ_REQ_INFO info;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(evt->msg_fmt_version,
						     MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
						     MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqnd_mqa_msg_fmt_table);

	msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(evt->src_dest_version,
					    MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
					    MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqnd_mqa_msg_fmt_table);

	/*Drop the messages with msg_fmt_version=1 as earlier versions are non backward compatible */
	if (!is_valid_msg_fmt || !msg_fmt_ver || (evt->msg_fmt_version == 1)) {
		/* Drop The Message */
		if (!is_valid_msg_fmt) {
			m_LOG_MQSV_ND(MQND_MSG_FRMT_VER_INVALID, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR,
				      is_valid_msg_fmt, __FILE__, __LINE__);
			TRACE("mqnd_evt_proc_send_msg:INVALID MSG FORMAT:1 %d", is_valid_msg_fmt);
		}
		if (!msg_fmt_ver) {
			m_LOG_MQSV_ND(MQND_MSG_FRMT_VER_INVALID, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR,
				      msg_fmt_ver, __FILE__, __LINE__);
			TRACE("mqnd_evt_proc_send_msg:INVALID MSG FORMAT:2 %d", msg_fmt_ver);
		}
		err = SA_AIS_ERR_VERSION;
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	if (evt->type.req_type == MQP_EVT_SEND_MSG)
		snd_msg = &evt->info.snd_msg;
	else
		snd_msg = &evt->info.sndMsgAsync.SendMsg;

	if (!cb->is_restart_done) {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	mqnd_queue_node_get(cb, snd_msg->queueHandle, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_BAD_HANDLE;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	if (qnode->qinfo.owner_flag == MQSV_QUEUE_OWN_STATE_PROGRESS) {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_UNDER_TRANSFER_STATE, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	offset = qnode->qinfo.shm_queue_index;
	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	/* Get user defined queue size and usage stats */
	qsize = qnode->qinfo.size[snd_msg->message.priority];
	qused = shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[snd_msg->message.priority].queueUsed;

	/* Check if message size is less than system defined msg size */
	if (snd_msg->message.size > cb->gl_msg_max_msg_size) {
		err = SA_AIS_ERR_TOO_BIG;
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}
	/* Check to see if the message fits into the queue as per user defined statistics */
	if (snd_msg->message.size > (qsize - qused)) {
		qnode->qinfo.numberOfFullErrors[snd_msg->message.priority]++;
		err = SA_AIS_ERR_QUEUE_FULL;
		memset(&queue_ckpt_node, 0, sizeof(MQND_QUEUE_CKPT_INFO));
		mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);
		mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
		m_LOG_MQSV_ND(MQND_QUEUE_FULL, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	/* Get actual queue size and usage stats */
	info.req = NCS_OS_POSIX_MQ_REQ_GET_ATTR;
	info.info.attr.i_mqd = qnode->qinfo.queueHandle;

	if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS) {
		err = SA_AIS_ERR_BAD_HANDLE;
		m_LOG_MQSV_ND(MQND_QUEUE_GET_ATTR_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__,
			      __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	actual_qsize = info.info.attr.o_attr.mq_maxmsg;
	actual_qused = info.info.attr.o_attr.mq_msgsize;

	/* Check to see if the (message + message header + message type) fits into the actual queue as per 
	   its actual stats, if NOT, increase the actual queue size so as to hold the message 
	   The "message type" is part of the NCS_OS_MQ_MSG structure which maps to the "mtype" field of the
	   "struct msqid_ds" posix structure */
	if ((snd_msg->message.size + sizeof(MQSV_MESSAGE) + sizeof(NCS_OS_MQ_MSG_LL_HDR)) >
	    (actual_qsize - actual_qused)) {
		info.req = NCS_OS_POSIX_MQ_REQ_RESIZE;
		info.info.resize.mqd = qnode->qinfo.queueHandle;

		/* Increase queue size so that its able to hold 5 such messages */
		info.info.resize.i_newqsize =
		    actual_qsize + (5 * (snd_msg->message.size + sizeof(MQSV_MESSAGE) + sizeof(NCS_OS_MQ_MSG_LL_HDR)));

		if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS) {
			err = SA_AIS_ERR_NO_RESOURCES;
			m_LOG_MQSV_ND(MQND_QUEUE_RESIZE_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__,
				      __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto send_resp;
		}
	}

	/* Allocate the memory (size of MQSV_MESSAGE + size of received data) */
	size = (uns32)(sizeof(MQSV_MESSAGE) + snd_msg->message.size);
	mqsv_msg = (MQSV_MESSAGE *)m_MMGR_ALLOC_MQND_DEFAULT(size);

	if (!mqsv_msg) {
		err = SA_AIS_ERR_NO_MEMORY;
		m_LOG_MQSV_ND(MQND_MEM_ALLOC_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

	/* Write into Queue */
	memset(mqsv_msg, 0, sizeof(size));

	mqsv_msg->type = MQP_EVT_GET_REQ;
	mqsv_msg->mqsv_version = MQSV_MSG_VERSION;

	/* Fill the Sender info */
	mqsv_msg->info.msg.message_info = snd_msg->messageInfo;
	m_GET_TIME_STAMP(mqsv_msg->info.msg.message_info.sendTime);
	if (snd_msg->messageInfo.sendReceive) {
		mqsv_msg->info.msg.message_info.sender.sender_context.sender_dest = evt->sinfo.dest;
		mqsv_msg->info.msg.message_info.sender.sender_context.context = evt->sinfo.ctxt;
		mqsv_msg->info.msg.message_info.sender.sender_context.reply_buffer_size =
		    snd_msg->messageInfo.sender.sender_context.reply_buffer_size;
	}

	memcpy(mqsv_msg->info.msg.message.data, snd_msg->message.data, (uns32)snd_msg->message.size);
	mqsv_msg->info.msg.message.priority = snd_msg->message.priority;
	mqsv_msg->info.msg.message.size = snd_msg->message.size;
	mqsv_msg->info.msg.message.type = snd_msg->message.type;
	mqsv_msg->info.msg.message.version = snd_msg->message.version;
	mqsv_msg->info.msg.message.senderName = snd_msg->message.senderName;

	rc = mqnd_mq_msg_send(qnode->qinfo.queueHandle, mqsv_msg, (uns32)size);

	/* Free the message */
	if (mqsv_msg)
		m_MMGR_FREE_MQND_DEFAULT(mqsv_msg);

	if (rc != NCSCC_RC_SUCCESS) {
		err = SA_AIS_ERR_NO_RESOURCES;
		m_LOG_MQSV_ND(MQND_SEND_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		goto send_resp;
	}

	/* Send a 1-byte message to the listener queue. The message is sent only 
	   if their exists a listener queue i.e. when listenerHandle <> 0 */
	rc = mqsv_listenerq_msg_send(qnode->qinfo.listenerHandle);

	if (rc != NCSCC_RC_SUCCESS) {
		err = SA_AIS_ERR_NO_RESOURCES;
		m_LOG_MQSV_ND(MQND_LISTENER_SEND_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, err, __FILE__,
			      __LINE__);
		goto send_resp;
	}

	mqnd_send_msg_update_stats_shm(cb, qnode, snd_msg->message.size, snd_msg->message.priority);
 send_resp:
	/* If the error happens in SendReceive case while sending the message then return the 
	   response from here and don't wait for the reply message to come */
	if (snd_msg && ((!snd_msg->messageInfo.sendReceive) ||
			((snd_msg->messageInfo.sendReceive) && (rc != NCSCC_RC_SUCCESS)))) {

		if (((snd_msg->ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK) == SA_MSG_MESSAGE_DELIVERED_ACK) ||
		    ((snd_msg->messageInfo.sendReceive) && (rc != NCSCC_RC_SUCCESS))) {

			if (evt->type.req_type == MQP_EVT_SEND_MSG) {

				direct_rsp_evt = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(sizeof(MQSV_DSEND_EVT));
				if (!direct_rsp_evt) {
					rc = NCSCC_RC_FAILURE;
					m_LOG_MQSV_ND(MQND_MEM_ALLOC_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR,
						      rc, __FILE__, __LINE__);
					return rc;
				}

				memset(direct_rsp_evt, 0, sizeof(MQSV_DSEND_EVT));
				direct_rsp_evt->evt_type = MQSV_DSEND_EVENT;
				direct_rsp_evt->endianness = machineEndianness();
				direct_rsp_evt->msg_fmt_version = msg_fmt_ver;
				direct_rsp_evt->src_dest_version = MQND_PVT_SUBPART_VERSION;
				direct_rsp_evt->type.rsp_type = MQP_EVT_SEND_MSG_RSP;
				direct_rsp_evt->info.sendMsgRsp.error = err;
				direct_rsp_evt->info.sendMsgRsp.msgHandle = snd_msg->msgHandle;

				rc = mqnd_mds_send_rsp_direct(cb, &evt->sinfo, direct_rsp_evt);
				if (rc != NCSCC_RC_SUCCESS) {
					m_LOG_MQSV_ND(MQND_MDS_SND_RSP_DIRECT_FAILED, NCSFL_LC_MQSV_SEND_RCV,
						      NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
				}
			} else {

				memset(&rsp_evt, 0, sizeof(MQSV_EVT));

				rsp_evt.type = MQSV_EVT_MQA_CALLBACK;
				rsp_evt.msg.mqp_async_rsp.callbackType = MQP_ASYNC_RSP_MSGDELIVERED;
				rsp_evt.msg.mqp_async_rsp.messageHandle = snd_msg->msgHandle;
				rsp_evt.msg.mqp_async_rsp.params.msgDelivered.invocation =
				    evt->info.sndMsgAsync.invocation;
				rsp_evt.msg.mqp_async_rsp.params.msgDelivered.error = err;

				/* Async Response */
				rc = mqnd_mds_send(cb, evt->sinfo.to_svc, evt->sinfo.dest, &rsp_evt);
				if (rc != NCSCC_RC_SUCCESS)
					m_LOG_MQSV_ND(MQND_MDS_SND_FAILED, NCSFL_LC_MQSV_SEND_RCV, NCSFL_SEV_ERROR, rc,
						      __FILE__, __LINE__);
			}
		}
	}

	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_tmr_expiry
 *
 * Description   : Function to Process the timer expiry events
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_evt_proc_tmr_expiry(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaMsgQueueHandleT qhdl;
	MQND_QUEUE_NODE *qnode = NULL;
	MQND_QNAME_NODE *pnode = NULL;
	SaNameT qname;
	ASAPi_OPR_INFO opr;
	SaAisErrorT err = SA_AIS_OK;
	MQSV_EVT transfer_complete;
	MQSV_EVT *qtrans_evt = NULL;
	MQND_QTRANSFER_EVT_NODE *qevt_node = NULL;
	ASAPi_NRESOLVE_RESP_INFO *qinfo = NULL;

	qhdl = evt->msg.mqnd_ctrl.info.tmr_info.qhdl;

	switch (evt->msg.mqnd_ctrl.info.tmr_info.type) {
	case MQND_TMR_TYPE_RETENTION:
		m_LOG_MQSV_ND(MQND_RETENTION_TMR_EXPIRED, NCSFL_LC_TIMER, NCSFL_SEV_NOTICE, rc, __FILE__, __LINE__);
		mqnd_queue_node_get(cb, qhdl, &qnode);
		if (!qnode) {
			m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE,
				      __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		/* Request the ASAPi (at MQD) for queue DEREG */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
		opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;

		/* Request the ASAPi */
		if (((rc = asapi_opr_hdlr(&opr)) != SA_AIS_OK) || (!opr.info.msg.resp)
		    || (opr.info.msg.resp->info.dresp.err.flag))
			m_LOG_MQSV_ND(MQND_ASAPI_DEREG_HDLR_FAILED, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, rc, __FILE__,
				      __LINE__);

		/* Free the response Event */
		asapi_msg_free(&opr.info.msg.resp);

		if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &qnode->qinfo.queueName) != SA_AIS_OK) {
			mqnd_genlog(NCSFL_SEV_ERROR, "Deleting MsgQueue Object %s FAILED \n",
				    qnode->qinfo.queueName.value);
			return NCSCC_RC_FAILURE;
		}
		/* Delete the Queue */
		mqnd_mq_destroy(&qnode->qinfo);

		/* Delete the listener queue if it exists */
		mqnd_listenerq_destroy(&qnode->qinfo);

		/*Invalidate the shm area of the queue */
		mqnd_shm_queue_ckpt_section_invalidate(cb, qnode);

		/* Delete the mapping entry from the qname database */
		qname = qnode->qinfo.queueName;
		mqnd_qname_node_get(cb, qname, &pnode);
		if (pnode) {
			mqnd_qname_node_del(cb, pnode);
			m_MMGR_FREE_MQND_QNAME_NODE(pnode);
		}

		/* Delete the Queue Node from the tree */
		mqnd_queue_node_del(cb, qnode);

		/* Free the Queue Node */
		m_MMGR_FREE_MQND_QUEUE_NODE(qnode);
		break;

	case MQND_TMR_TYPE_MQA_EXPIRY:
		if ((rc = mqnd_restart_init(cb)) != SA_AIS_OK)
			m_LOG_MQSV_ND(MQND_RESTART_CPSV_INIT_FAILED, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, rc, __FILE__,
				      __LINE__);
		break;

	case MQND_TMR_TYPE_NODE1_QTRANSFER:
		m_LOG_MQSV_ND(MQND_QTRANSFER_NODE1_TMR_EXPIRED, NCSFL_LC_TIMER, NCSFL_SEV_NOTICE, rc, __FILE__,
			      __LINE__);
		/*Case:  Request response not recieved in Time ,so send a Transfer Complete Response with Error
		   code to change the state of queue from PROGRESS to ORPHAN */
		mqnd_qevt_node_get(cb, qhdl, &qevt_node);
		if (!qevt_node) {
			m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE,
				      __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(&transfer_complete, 0, sizeof(MQSV_EVT));
		transfer_complete.type = MQSV_EVT_MQP_REQ;
		transfer_complete.msg.mqp_req.type = MQP_EVT_TRANSFER_QUEUE_COMPLETE;
		transfer_complete.msg.mqp_req.info.transferComplete.queueHandle = qhdl;
		transfer_complete.msg.mqp_req.info.transferComplete.error = NCSCC_RC_FAILURE;

		rc = mqnd_mds_send(cb, NCSMDS_SVC_ID_MQND, qevt_node->addr, &transfer_complete);
		if (rc != NCSCC_RC_SUCCESS)
			m_LOG_MQSV_ND(MQND_MDS_SND_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__,
				      __LINE__);

		mqnd_qevt_node_del(cb, qevt_node);

		break;

	case MQND_TMR_TYPE_NODE2_QTRANSFER:
		m_LOG_MQSV_ND(MQND_QTRANSFER_NODE2_TMR_EXPIRED, NCSFL_LC_TIMER, NCSFL_SEV_NOTICE, rc, __FILE__,
			      __LINE__);

		mqnd_queue_node_get(cb, qhdl, &qnode);
		if (!qnode) {
			m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE,
				      __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		/*Send req to mqd to find the state and owner 
		   Request the ASAPi (at MQD) for queue information */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;
		opr.info.msg.req.msgtype = ASAPi_MSG_NRESOLVE;
		opr.info.msg.req.info.nresolve.object = qnode->qinfo.queueName;
		opr.info.msg.req.info.nresolve.track = FALSE;

		/* Request the ASAPi */
		rc = asapi_opr_hdlr(&opr);

		if (rc == SA_AIS_ERR_TIMEOUT) {
			m_LOG_MQSV_ND(MQND_MDS_SND_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
				      __LINE__);
			rc = asapi_opr_hdlr(&opr);
		}

		if ((rc != SA_AIS_OK) || (!opr.info.msg.resp) || ((opr.info.msg.resp->info.nresp.err.flag)
								  && (opr.info.msg.resp->info.nresp.err.errcode !=
								      SA_AIS_ERR_NOT_EXIST))) {
			err = SA_AIS_ERR_TRY_AGAIN;
			m_LOG_MQSV_ND(MQND_ASAPI_NRESOLVE_HDLR_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err,
				      __FILE__, __LINE__);
			goto error;
		}

		qinfo = &opr.info.msg.resp->info.nresp;

		/*  case 1 : new owner, delete the old queue(post transfer complete response)
		   case 2 : old owner, change the status from progress to orphan */
		qtrans_evt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);
		if (qtrans_evt == NULL) {
			m_LOG_MQSV_ND(MQND_EVT_ALLOC_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE,
				      __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}
		qtrans_evt->evt_type = MQSV_NOT_DSEND_EVENT;
		qtrans_evt->type = MQSV_EVT_MQP_REQ;
		qtrans_evt->msg.mqp_req.type = MQP_EVT_TRANSFER_QUEUE_COMPLETE;
		qtrans_evt->msg.mqp_req.info.transferComplete.queueHandle = qhdl;
		if (qinfo->oinfo.qparam) {

			if (m_NCS_MDS_DEST_EQUAL(&qinfo->oinfo.qparam->addr, &cb->my_dest) == 0)
				qtrans_evt->msg.mqp_req.info.transferComplete.error = NCSCC_RC_SUCCESS;
			else {
				if (qinfo->oinfo.qparam->owner == MQSV_QUEUE_OWN_STATE_PROGRESS)
					qtrans_evt->msg.mqp_req.info.transferComplete.error = NCSCC_RC_FAILURE;
			}
		} else
			qtrans_evt->msg.mqp_req.info.transferComplete.error = NCSCC_RC_FAILURE;

		rc = m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)qtrans_evt, NCS_IPC_PRIORITY_NORMAL);

 error:
		if (opr.info.msg.resp)
			asapi_msg_free(&opr.info.msg.resp);

		break;
	}
	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_qattr_get
 *
 * Description   : Function to process Queue Attributes from the other MQND.
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MQSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_qattr_get(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQND_QATTR_REQ *qattr_req = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	MQSV_EVT rsp_evt;
	uns32 i;

	qattr_req = &evt->msg.mqnd_ctrl.info.qattr_get;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	mqnd_queue_node_get(cb, qattr_req->qhdl, &qnode);
 send_rsp:
	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_BAD_HANDLE;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
	}

	/*Send the resp to MQA */
	memset(&rsp_evt, 0, sizeof(MQSV_EVT));
	rsp_evt.type = MQSV_EVT_MQND_CTRL;

	rsp_evt.msg.mqnd_ctrl.type = MQND_CTRL_EVT_QATTR_INFO;
	rsp_evt.msg.mqnd_ctrl.info.qattr_info.error = err;
	if (qnode) {
		rsp_evt.msg.mqnd_ctrl.info.qattr_info.qattr.creationFlags = qnode->qinfo.queueStatus.creationFlags;
		rsp_evt.msg.mqnd_ctrl.info.qattr_info.qattr.retentionTime = qnode->qinfo.queueStatus.retentionTime;
		for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i <= SA_MSG_MESSAGE_LOWEST_PRIORITY; i++) {
			rsp_evt.msg.mqnd_ctrl.info.qattr_info.qattr.size[i] = qnode->qinfo.size[i];
		}
	}
	rc = mqnd_mds_send_rsp(cb, &evt->sinfo, &rsp_evt);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_MDS_SND_RSP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	else
		m_LOG_MQSV_ND(MQND_MDS_SND_RSP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__);

	return rc;
}

/****************************************************************************
 * Name          : mqnd_evt_proc_ret_time_set 
 *
 * Description   : Function to set retention time of queue 
 *
 * Arguments     :
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_ret_time_set(MQND_CB *cb, MQSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQND_QUEUE_NODE *qnode = NULL;
	SaAisErrorT err = SA_AIS_OK;
	MQSV_EVT rsp_evt;

	if (cb->is_restart_done)
		err = SA_AIS_OK;
	else {
		err = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_MQSV_ND(MQND_INITIALIZATION_INCOMPLETE, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, err, __FILE__,
			      __LINE__);
		goto send_rsp;
	}

	mqnd_queue_node_get(cb, evt->msg.mqp_req.info.retTimeSetReq.queueHandle, &qnode);

	/* If queue not found */
	if (!qnode) {
		err = SA_AIS_ERR_BAD_HANDLE;
		m_LOG_MQSV_ND(MQND_GET_QNODE_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, err, __FILE__, __LINE__);
		goto send_rsp;
	}

	if (((qnode->qinfo.queueStatus.creationFlags & SA_MSG_QUEUE_PERSISTENT) == SA_MSG_QUEUE_PERSISTENT) ||
	    (qnode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE)) {
		err = SA_AIS_ERR_BAD_OPERATION;
		goto send_rsp;
	}
	/* Set retention time only if timer is not started */
	if (!qnode->qinfo.tmr.is_active) {
		qnode->qinfo.queueStatus.retentionTime = evt->msg.mqp_req.info.retTimeSetReq.retentionTime;
		immutil_update_one_rattr(cb->immOiHandle, (char *)qnode->qinfo.queueName.value,
					 "saMsgQueueRetentionTime", SA_IMM_ATTR_SATIMET,
					 &qnode->qinfo.queueStatus.retentionTime);
	} else {
		err = SA_AIS_ERR_BAD_HANDLE;
		goto send_rsp;
	}

 send_rsp:
	/*Send the resp to MQA */
	memset(&rsp_evt, 0, sizeof(MQSV_EVT));

	rsp_evt.type = MQSV_EVT_MQP_RSP;
	rsp_evt.msg.mqp_rsp.type = MQP_EVT_Q_RET_TIME_SET_RSP;
	rsp_evt.msg.mqp_rsp.error = err;
	rsp_evt.msg.mqp_rsp.info.retTimeSetRsp.queueHandle = evt->msg.mqp_req.info.retTimeSetReq.queueHandle;

	rc = mqnd_mds_send_rsp(cb, &evt->sinfo, &rsp_evt);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_MDS_SND_RSP_FAILED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	else
		m_LOG_MQSV_ND(MQND_MDS_SND_RSP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__);

	return rc;

}

/****************************************************************************
 * Name          : mqnd_evt_proc_cb_dump
 *
 * Description   : Function to print the control block infomration
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 mqnd_evt_proc_cb_dump(void)
{
	MQND_QUEUE_NODE *qnode = 0;
	MQND_QNAME_NODE *pnode = 0;
	MQND_QTRANSFER_EVT_NODE *qevt_node = 0;
	uns32 i = 0, offset;
	SaNameT qname;
	MQND_CB *cb = NULL;
	uns32 cb_hdl = m_MQND_GET_HDL();
	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (cb == NULL) {
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE,
			      __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	memset(&qname, '\0', sizeof(SaNameT));
	TRACE("<<<<<<<<<<<<<<< MQND CB Details >>>>>>>>>>>>>>>");

	TRACE("MQND MDS DEST: %u", m_NCS_NODE_ID_FROM_MDS_DEST(cb->my_dest));
	switch (cb->ha_state) {
	case SA_AMF_HA_ACTIVE:
		TRACE("HA STATE     : SA_AMF_HA_ACTIVE");
		break;
	case SA_AMF_HA_STANDBY:
		TRACE("HA STATE     : SA_AMF_HA_STANDBY");
		break;
	case SA_AMF_HA_QUIESCED:
		TRACE("HA STATE     : SA_AMF_HA_QUIESCED");
		break;
	case SA_AMF_HA_QUIESCING:
		TRACE("HA STATE     : SA_AMF_HA_QUIESCING");
		break;
	default:
		TRACE("HA STATE     : UNDEFINED");
	}
	if (cb->is_restart_done)
		TRACE("MQND RESTART : COMPLETE");
	else
		TRACE("MQND RESTART : INCOMPLETE");
	if (cb->is_mqd_up)
		TRACE("MQD          : UP  ");
	else
		TRACE("MQD          : DOWN ");
	TRACE("------------- QUEUE NODE Details -------------");
	mqnd_queue_node_getnext(cb, 0, &qnode);
	while (qnode) {
		TRACE("~~~~~~~~~~ START %s's INFO ~~~~~~~~~~", qnode->qinfo.queueName.value);
		TRACE(" Message Handle : %llu", qnode->qinfo.msgHandle);
		TRACE(" Queue Handle   : %llu", qnode->qinfo.queueHandle);

		if (qnode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE)
			TRACE(" Sending State  : MSG_QUEUE_UNAVAILABLE");
		else if (qnode->qinfo.sendingState == MSG_QUEUE_AVAILABLE)
			TRACE(" Sending State  : MSG_QUEUE_AVAILABLE");
		switch (qnode->qinfo.owner_flag) {
		case MQSV_QUEUE_OWN_STATE_ORPHAN:
			TRACE(" Owner Flag     : MQSV_QUEUE_OWN_STATE_ORPHAN");
			break;
		case MQSV_QUEUE_OWN_STATE_OWNED:
			TRACE(" Owner Flag     : MQSV_QUEUE_OWN_STATE_OWNED");
			break;
		case MQSV_QUEUE_OWN_STATE_PROGRESS:
			TRACE(" Owner Flag     : MQSV_QUEUE_OWN_STATE_PROGRESS");
			break;
		default:
			TRACE(" Owner Flag     : UNDEFINED");
		}
		TRACE("  Queue Key      : %d", qnode->qinfo.q_key);
		TRACE(" Agent info of the receiver: %u", m_NCS_NODE_ID_FROM_MDS_DEST(qnode->qinfo.rcvr_mqa));

		if (qnode->qinfo.tmr.is_active) {
			TRACE("\n~~~~~~~~~~RETENTION TIMER DETAILS~~~~~~~~~~");
			mqnd_dump_timer_info(qnode->qinfo.tmr);
		} else
			TRACE("\n RETENTION TIMER INACTIVE");
		if (qnode->qinfo.qtransfer_complete_tmr.is_active) {
			TRACE("~~~~~~~~~~QTRANSFER TIMER DETAILS~~~~~~~~~~");
			mqnd_dump_timer_info(qnode->qinfo.qtransfer_complete_tmr);
		} else
			TRACE(" QTRANSFER TIMER INACTIVE");

		offset = qnode->qinfo.shm_queue_index;
		mqnd_dump_queue_status(cb, &qnode->qinfo.queueStatus, offset);
		TRACE("\n Queue Total Size          : %llu", qnode->qinfo.totalQueueSize);
		TRACE(" Queue Total Used Size     : %llu", shm_base_addr[offset].QueueStatsShm.totalQueueUsed);
		TRACE(" Queue Total No of Messages: %u", shm_base_addr[offset].QueueStatsShm.totalNumberOfMessages);
		TRACE("~~~~~~~~~~ Queue Info ~~~~~~~~~~");
		TRACE(" Queue Creation Time:  %llu", qnode->qinfo.creationTime);
		for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; i++)
			TRACE(" Number of Full Errors of %u priority: %u", i, qnode->qinfo.numberOfFullErrors[i]);

		TRACE("~~~~~~~~~~ END %s's INFO ~~~~~~~~~~", qnode->qinfo.queueName.value);
		mqnd_queue_node_getnext(cb, qnode->qinfo.queueHandle, &qnode);
	}
	if (cb->is_qname_db_up) {
		TRACE(" QNAME Database : UP");

		mqnd_qname_node_getnext(cb, qname, &pnode);
		while (pnode) {
			TRACE(" Queue Name     : %s", pnode->qname.value);
			TRACE(" Queue Handle   : %llu", pnode->qhdl);
			qname = pnode->qname;
			qname.length = m_NTOH_SANAMET_LEN(qname.length);
			mqnd_qname_node_getnext(cb, qname, &pnode);
		}
	}
	if (cb->is_qevt_hdl_db_up) {
		TRACE(" QEVT NODE Database  :UP");
		mqnd_qevt_node_getnext(cb, 0, &qevt_node);
		while (qevt_node) {
			TRACE(" Queue Handle : %llu", qevt_node->tmr.qhdl);
			mqnd_qevt_node_getnext(cb, qevt_node->tmr.qhdl, &qevt_node);
		}
	}

	TRACE("<<<<<<<<<<<<<<< End of the CB >>>>>>>>>>>>>>>");

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	return NCSCC_RC_SUCCESS;
}

static void mqnd_dump_queue_status(MQND_CB *cb, SaMsgQueueStatusT *queueStatus, uns32 offset)
{
	uns32 i;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	TRACE("~~~~~~~~~~ Queue Status Parameters ~~~~~~~~~~");
	TRACE(" Creation Flags: %u", queueStatus->creationFlags);
	TRACE(" Retention time: %llu", queueStatus->retentionTime);
	TRACE(" Close Time    : %llu", queueStatus->closeTime);

	shm_base_addr = cb->mqnd_shm.shm_base_addr;
	for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; i++) {
		TRACE(" Priority: %d", i);
		TRACE("     Queue Size: %llu", shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].queueSize);
		TRACE("     Queue Used: %llu", shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].queueUsed);
		TRACE("     No of messages: %u",
		       shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].numberOfMessages);
	}
}

static void mqnd_dump_timer_info(MQND_TMR tmr)
{
	TRACE(" Timer Type: %d", tmr.type);
	TRACE(" Timer ID: %p", tmr.tmr_id);
	TRACE(" Queue Handle: %llu", tmr.qhdl);
	TRACE(" uarg: %ui", tmr.uarg);
}

/****************************************************************************
 * Name          :  mqnd_proc_mds_mqa_up
 *
 * Description   : Function to porcess the MQA up event
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mqnd_proc_mds_mqa_up(MQND_CB *cb, MQSV_EVT *evt)
{
	MDS_DEST i_dest = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	i_dest = evt->msg.mqnd_ctrl.info.mqa_up_info.mqa_up_dest;

	if (i_dest) {
		if (!cb->is_restart_done) {
			cb->mqa_counter++;	/* Do nothing */
			rc = mqnd_add_node_to_mqalist(cb, i_dest);
			if (rc != NCSCC_RC_SUCCESS)
				m_LOG_MQSV_ND(MQND_MQA_ADD_NODE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc,
					      __FILE__, __LINE__);

		}

		if (cb->is_restart_done) {
			m_LOG_MQSV_ND(MQND_RESTART_INIT_FIRST_TIME, NCSFL_LC_MQSV_INIT, NCSFL_SEV_INFO, rc,
				      __FILE__, __LINE__);

		}
	} else
		return NCSCC_RC_FAILURE;
	return rc;
}

/****************************************************************************
 * Name          : mqnd_proc_deferred_mqa_rsp 
 *
 * Description   : Function to porcess the deferred MQA responses
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mqnd_proc_deferred_mqa_rsp(MQND_CB *cb)
{
	MQA_RSP_CNTXT *mqa_rsp_cntx = cb->mqa_dfrd_evt_rsp_list_head;
	MQA_RSP_CNTXT *temp = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (mqa_rsp_cntx == NULL) {
		TRACE(" Deferred mqa event list head NULL");
		return NCSCC_RC_FAILURE;
	}

	while (mqa_rsp_cntx != NULL) {
		if (mqa_rsp_cntx->evt.msg.mqp_rsp.type == MQP_EVT_CLOSE_RSP) {
			rc = mqnd_send_mqp_close_rsp(cb, &mqa_rsp_cntx->sinfo,
						     mqa_rsp_cntx->evt.msg.mqp_rsp.error,
						     mqa_rsp_cntx->evt.msg.mqp_rsp.info.closeRsp.queueHandle);

			if (rc != NCSCC_RC_SUCCESS)
				m_LOG_MQSV_ND(MQND_QUEUE_CLOSE_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT,
					      NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
			else
				m_LOG_MQSV_ND(MQND_QUEUE_CLOSE_RESP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT,
					      NCSFL_SEV_INFO, rc, __FILE__, __LINE__);

			TRACE(" mqnd_proc_deferred_mqa_rsp: CLOSE: Queue Handle - %llu err - %d %s %d",
			       mqa_rsp_cntx->evt.msg.mqp_rsp.info.closeRsp.queueHandle,
			       mqa_rsp_cntx->evt.msg.mqp_rsp.error, __FILE__, __LINE__);
		} else if (mqa_rsp_cntx->evt.msg.mqp_rsp.type == MQP_EVT_UNLINK_RSP) {
			rc = mqnd_send_mqp_ulink_rsp(cb, &mqa_rsp_cntx->evt.sinfo,
						     mqa_rsp_cntx->evt.msg.mqp_rsp.error,
						     &mqa_rsp_cntx->evt.msg.mqp_req.info.unlinkReq);

			if (rc != NCSCC_RC_SUCCESS)
				m_LOG_MQSV_ND(MQND_QUEUE_UNLINK_RESP_FAILED, NCSFL_LC_MQSV_Q_MGMT,
					      NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
			else
				m_LOG_MQSV_ND(MQND_QUEUE_UNLINK_RESP_SUCCESS, NCSFL_LC_MQSV_Q_MGMT,
					      NCSFL_SEV_INFO, rc, __FILE__, __LINE__);
			TRACE("mqnd_proc_deferred_mqa_rsp: UNLINK: err - %d %s %d",
			       mqa_rsp_cntx->evt.msg.mqp_rsp.error, __FILE__, __LINE__);
		} else {
			TRACE("mqnd_proc_deferred_mqa_rsp: Default case");
		}
		temp = mqa_rsp_cntx;
		mqa_rsp_cntx = mqa_rsp_cntx->next;
		free(temp);
		temp = NULL;
	}

	cb->mqa_dfrd_evt_rsp_list_head = NULL;
	return rc;
}

static uns32 mqnd_enqueue_evt_cntxt_close(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err, uns32 qhdl)
{
	MQA_RSP_CNTXT *mqa_rsp = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	mqa_rsp = (MQA_RSP_CNTXT *) malloc(sizeof(MQA_RSP_CNTXT));
	if (mqa_rsp == NULL)
		return SA_AIS_ERR_NO_RESOURCES;

	memset(mqa_rsp, 0, sizeof(MQA_RSP_CNTXT));
	mqa_rsp->evt.type = MQSV_EVT_MQP_RSP;
	mqa_rsp->evt.msg.mqp_rsp.error = err;
	mqa_rsp->evt.msg.mqp_rsp.type = MQP_EVT_CLOSE_RSP;
	mqa_rsp->evt.msg.mqp_rsp.info.closeRsp.queueHandle = qhdl;

	memcpy(&mqa_rsp->sinfo, sinfo, sizeof(MQSV_SEND_INFO));

	if (cb->mqa_dfrd_evt_rsp_list_head != NULL) {
		mqa_rsp->next = cb->mqa_dfrd_evt_rsp_list_head;
		cb->mqa_dfrd_evt_rsp_list_head = mqa_rsp;
	} else {
		mqa_rsp->next = NULL;
		cb->mqa_dfrd_evt_rsp_list_head = mqa_rsp;
	}

	TRACE("mqnd_enqueue_evt_cntxt_close: Queue Handle - %llu Err - %d %s %d",
	       mqa_rsp->evt.msg.mqp_rsp.info.closeRsp.queueHandle, mqa_rsp->evt.msg.mqp_rsp.error, __FILE__, __LINE__);

	return rc;
}

static uns32 mqnd_enqueue_evt_cntxt_unlink(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err,
					   MQP_UNLINK_REQ *ulink_req)
{
	MQA_RSP_CNTXT *mqa_rsp = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	mqa_rsp = (MQA_RSP_CNTXT *) malloc(sizeof(MQA_RSP_CNTXT));
	if (mqa_rsp == NULL)
		return SA_AIS_ERR_NO_RESOURCES;

	memset(mqa_rsp, 0, sizeof(MQA_RSP_CNTXT));
	mqa_rsp->evt.type = MQSV_EVT_MQP_RSP;
	mqa_rsp->evt.msg.mqp_rsp.error = err;
	mqa_rsp->evt.msg.mqp_rsp.type = MQP_EVT_UNLINK_RSP;
	mqa_rsp->evt.msg.mqp_rsp.info.unlinkRsp.queueHandle = ulink_req->queueHandle;
	mqa_rsp->evt.msg.mqp_rsp.info.unlinkRsp.msgHandle = ulink_req->msgHandle;
	mqa_rsp->evt.msg.mqp_rsp.info.unlinkRsp.queueName = ulink_req->queueName;

	memcpy(&mqa_rsp->sinfo, sinfo, sizeof(MQSV_SEND_INFO));

	if (cb->mqa_dfrd_evt_rsp_list_head != NULL) {
		mqa_rsp->next = cb->mqa_dfrd_evt_rsp_list_head;
		cb->mqa_dfrd_evt_rsp_list_head = mqa_rsp;
	} else {
		mqa_rsp->next = NULL;
		cb->mqa_dfrd_evt_rsp_list_head = mqa_rsp;
	}

	TRACE("mqnd_enqueue_evt_cntxt_unlink: Queue Handle - %llu Err - %d %s %d",
	       mqa_rsp->evt.msg.mqp_rsp.info.unlinkRsp.queueHandle, mqa_rsp->evt.msg.mqp_rsp.error, __FILE__, __LINE__);

	return rc;
}
