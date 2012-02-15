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
  FILE NAME: mqsv_proc.c

  DESCRIPTION: MQND Processing routines.

******************************************************************************/

#include <immutil.h>
#include "mqnd.h"
#include "mqnd_imm.h"

extern NCS_OS_MQ_MSG transfer_mq_msg;	/* used in queue owner ship */

static uint32_t mqnd_existing_queue_open(MQND_CB *cb, MQSV_SEND_INFO *sinfo, MQP_OPEN_REQ *open,
				      ASAPi_NRESOLVE_RESP_INFO *qinfo, SaAisErrorT *err, uint32_t *existing_msg_count);

static uint32_t mqnd_send_transfer_owner_req(MQND_CB *cb, MQP_REQ_MSG *mqp_req,
					  MQSV_SEND_INFO *rcvr_mqa_sinfo, MDS_DEST *old_owner,
					  SaMsgQueueHandleT old_hdl, MQP_REQ_TYPE openType);
void mqnd_proc_ckpt_clm_node_left(MQND_CB *cb);
void mqnd_proc_ckpt_clm_node_joined(MQND_CB *cb);

uint32_t mqnd_evt_proc_mqp_qtransfer_complete(MQND_CB *cb, MQSV_EVT *req)
{
	SaMsgQueueHandleT qhdl;
	NCS_OS_MQ_REQ_INFO info;
	MQND_QUEUE_NODE *qnode = NULL;
	MQND_QNAME_NODE *pnode = NULL;
	ASAPi_OPR_INFO opr;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaNameT qname;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	SaAisErrorT error;
	TRACE_ENTER();

	qhdl = req->msg.mqp_req.info.transferComplete.queueHandle;

	/* Check if queue exists */
	mqnd_queue_node_get(cb, qhdl, &qnode);
	if (!qnode) {
		LOG_ER("Get queue node Failed");
		return NCSCC_RC_FAILURE;
	}

	/*Timer to keep track of Q which is being transfered can be stopped as we got the response */
	if (qnode->qinfo.qtransfer_complete_tmr.is_active)
		mqnd_tmr_stop(&qnode->qinfo.qtransfer_complete_tmr);

	if (req->msg.mqp_req.info.transferComplete.error == NCSCC_RC_SUCCESS) {

		/* Delete the native message queue */
		memset(&info, 0, sizeof(NCS_OS_MQ_REQ_INFO));
		info.req = NCS_OS_MQ_REQ_DESTROY;
		info.info.destroy.i_hdl = qhdl;

		if (m_NCS_OS_MQ(&info) != NCSCC_RC_SUCCESS) {
			LOG_ER("MSGQ Destroy routine Failed");
			return (NCSCC_RC_FAILURE);
		}

		/* Invalidate shm slot used by this queue */
		mqnd_shm_queue_ckpt_section_invalidate(cb, qnode);

		/* Delete the mapping entry from the qname database */
		memset(&qname, 0, sizeof(SaNameT));
		qname = qnode->qinfo.queueName;
		mqnd_qname_node_get(cb, qname, &pnode);
		mqnd_qname_node_del(cb, pnode);
		if (pnode)
			m_MMGR_FREE_MQND_QNAME_NODE(pnode);

		rc = mqnd_queue_node_del(cb, qnode);

		if (rc == NCSCC_RC_FAILURE)
			return rc;

		/* Free the Queue Node */
		m_MMGR_FREE_MQND_QUEUE_NODE(qnode);

		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	} else {
		/*Q Timer expired on N1 so Trans Complete req with error code set was sent either from N1 
		   or Timer Expiry routine running on N2 */
		/* Register with MQD the change in status to MQSV_QUEUE_ORPHAN */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_REG;
		opr.info.msg.req.info.reg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
		opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
		opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
		opr.info.msg.req.info.reg.queue.owner = MQSV_QUEUE_OWN_STATE_ORPHAN;
		opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
		opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
		opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

		memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
		       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

		/* Request the ASAPi */
		rc = asapi_opr_hdlr(&opr);

		if (rc == SA_AIS_ERR_TIMEOUT) {
			/*This is the case when REG request is assumed to be lost or not proceesed at MQD becoz of failover.So
			   to compensate for the lost request we send an async REG request to MQD */
			/*Request the ASAPi (at MQD) for queue REG */
			TRACE_1("UNLINK TIMEOUT:ASYNC REG CHANGE IN STATUS TO ORPHAN QUEUE");
			memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
			opr.type = ASAPi_OPR_MSG;
			opr.info.msg.opr = ASAPi_MSG_SEND;

			/* Fill MDS info */
			opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
			opr.info.msg.sinfo.dest = cb->mqd_dest;
			opr.info.msg.sinfo.stype = MDS_SENDTYPE_SND;

			opr.info.msg.req.msgtype = ASAPi_MSG_REG;
			opr.info.msg.req.info.reg.objtype = ASAPi_OBJ_QUEUE;
			opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
			opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
			opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
			opr.info.msg.req.info.reg.queue.owner = MQSV_QUEUE_OWN_STATE_ORPHAN;
			opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
			opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
			opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

			memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
			       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));
			rc = asapi_opr_hdlr(&opr);	/*May be insert a log */
		} else {
			if (rc != SA_AIS_OK || (!opr.info.msg.resp) || (opr.info.msg.resp->info.rresp.err.flag)) {
				LOG_ER("Asapi Opr handler for Queue Register Failed with return code %u", rc); 
				return rc;
			}
		}

		if (rc == SA_AIS_OK) {
			qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_ORPHAN;
			memset(&queue_ckpt_node, 0, sizeof(MQND_QUEUE_CKPT_INFO));
			mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

			rc = mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
			if (rc != SA_AIS_OK) {
				LOG_ER("Ckpt Overwrite Failed");
				return rc;
			}
			/* Immsv Runtime Object Create  */
			error =
			    mqnd_create_runtime_MsgQobject((char *)qnode->qinfo.queueName.value,
							   qnode->qinfo.creationTime, qnode, cb->immOiHandle);

			if (error != SA_AIS_OK) {
				LOG_ER("Create MsgQobject %s Failed with error: %u", qnode->qinfo.queueName.value, error);
				return NCSCC_RC_FAILURE;
			}

			error = mqnd_create_runtime_MsgQPriorityobject((char *)qnode->qinfo.queueName.value, qnode,
								       cb->immOiHandle);

			if (error != SA_AIS_OK) {
				LOG_ER("Create MsgQPriorityobject %s Failed : %u", qnode->qinfo.queueName.value, error);
				return NCSCC_RC_FAILURE;
			}
		}

		/* Free the ASAPi response Event */
		if (opr.info.msg.resp)
			asapi_msg_free(&opr.info.msg.resp);
		return rc;
	}
}

uint32_t mqnd_evt_proc_mqp_qtransfer(MQND_CB *cb, MQSV_EVT *req)
{
	MQSV_EVT transfer_rsp;
	SaMsgQueueHandleT qhdl;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT err = SA_AIS_OK;
	uint32_t num_messages;
	MQND_QUEUE_NODE *qnode = NULL;
	char *mqsv_message = NULL;
	char *mqsv_message_cpy = NULL;
	MQSV_MESSAGE *tmp_msg;
	ASAPi_OPR_INFO opr;
	uint32_t msg_count = 0;
	uint32_t offset = 0;
	uint32_t size = 0;
	uint32_t i;
	NCS_OS_POSIX_MQ_REQ_INFO qreq;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	TRACE_ENTER();

	memset(&transfer_rsp, 0, sizeof(MQSV_EVT));

	qhdl = req->msg.mqp_req.info.transferReq.old_queueHandle;

	/* Check if queue exists */
	mqnd_queue_node_get(cb, qhdl, &qnode);
	if (!qnode) {
		LOG_ER("ERR_NOT_EXIST: Get queue node Failed");
		err = SA_AIS_ERR_NOT_EXIST;
		goto send_rsp;
	}

	/* If queue is opened by some body, return SA_AIS_ERR_BUSY to MQA */
	if (qnode->qinfo.owner_flag != MQSV_QUEUE_OWN_STATE_ORPHAN) {
		LOG_ER("%s:%u: ERR_BUSY: MSGQ is Busy (Opened by someone)", __FILE__, __LINE__);
		err = SA_AIS_ERR_BUSY;
		goto send_rsp;
	}

	/* Read all the messages from the queue and pack it into buffer */
	qreq.req = NCS_OS_POSIX_MQ_REQ_GET_ATTR;
	qreq.info.attr.i_mqd = qhdl;
	if (m_NCS_OS_POSIX_MQ(&qreq) != NCSCC_RC_SUCCESS) {
		LOG_ER("ERR_RESOURCES: Unable to get the queue attributes from the queue");
		err = SA_AIS_ERR_NO_RESOURCES;
		goto send_rsp;
	}

	num_messages = qreq.info.attr.o_attr.mq_curmsgs;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.creationTime = qnode->qinfo.creationTime;
	if ((qreq.info.attr.o_attr.mq_curmsgs == 0) || (req->msg.mqp_req.info.transferReq.empty_queue)) {
		transfer_rsp.msg.mqp_rsp.info.transferRsp.mqsv_message = NULL;
		transfer_rsp.msg.mqp_rsp.info.transferRsp.msg_count = 0;
		transfer_rsp.msg.mqp_rsp.info.transferRsp.msg_bytes = 0;
		goto reg_req;
	} else {

		mqsv_message = m_MMGR_ALLOC_MQND_DEFAULT(qreq.info.attr.o_attr.mq_msgsize);

		if (!mqsv_message) {
			LOG_ER("ERR_MEMORY: Memory Allocation Failed");
			err = SA_AIS_ERR_NO_MEMORY;
			goto send_rsp;
		}

		memset(mqsv_message, 0, qreq.info.attr.o_attr.mq_msgsize);
		transfer_rsp.msg.mqp_rsp.info.transferRsp.mqsv_message = mqsv_message;
	}

	mqsv_message_cpy = mqsv_message;
	msg_count = num_messages;

	while (num_messages) {

		if (mqnd_mq_rcv(qhdl) != NCSCC_RC_SUCCESS) {
			TRACE_2("ERR_RESOURCES: Unable to receive the message from the Queue");
			err = SA_AIS_ERR_NO_RESOURCES;
			goto send_rsp;
		}

		tmp_msg = (MQSV_MESSAGE *)transfer_mq_msg.data;

		if (tmp_msg->type == MQP_EVT_CANCEL_REQ) {
			num_messages--;
			msg_count--;
			continue;
		}

		size = (uint32_t)(sizeof(MQSV_MESSAGE) + tmp_msg->info.msg.message.size);
		memcpy(mqsv_message, tmp_msg, size);

		offset += size;
		mqsv_message += size;
		num_messages--;

	}

	transfer_rsp.msg.mqp_rsp.info.transferRsp.msg_count = msg_count;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.msg_bytes = offset;

	/* Put the messages back in queue to take care of queue transfer failure scenario */
	/* Also, convert the messages in buffer to Network order before sending the transfer response */
	offset = 0;
	for (i = 0; i < transfer_rsp.msg.mqp_rsp.info.transferRsp.msg_count; i++) {

		tmp_msg = (MQSV_MESSAGE *)&transfer_rsp.msg.mqp_rsp.info.transferRsp.mqsv_message[offset];

		size = (uint32_t)(sizeof(MQSV_MESSAGE) + tmp_msg->info.msg.message.size);
		offset += size;

		rc = mqnd_mq_msg_send(qhdl, tmp_msg, (uint32_t)size);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_2("ERR_RESOURCES: Unable to send the message to the Queue");
			err = SA_AIS_ERR_NO_RESOURCES;
			goto send_rsp;
		}

		m_MQND_HTON_MQSV_MESSAGE(tmp_msg);
	}
 reg_req:
	/* Change owner flag to TransferInProgress */
	qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_PROGRESS;
	/*Before sending the transfer response start the timer to check the Queue Transfer progress */
	qnode->qinfo.qtransfer_complete_tmr.type = MQND_TMR_TYPE_NODE2_QTRANSFER;
	qnode->qinfo.qtransfer_complete_tmr.qhdl = qnode->qinfo.queueHandle;
	qnode->qinfo.qtransfer_complete_tmr.uarg = cb->cb_hdl;
	qnode->qinfo.qtransfer_complete_tmr.tmr_id = 0;
	qnode->qinfo.qtransfer_complete_tmr.is_active = false;

	rc = mqnd_tmr_start(&qnode->qinfo.qtransfer_complete_tmr, MQND_QTRANSFER_REQ_TIMER);

	if (rc == NCSCC_RC_SUCCESS) {
		TRACE_1("QTransfer Tmr on Node 2 started %llu", qhdl);
		mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

		rc = mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
		if (rc != SA_AIS_OK) {
			TRACE_2("Ckpt Overwrite Failed with return code %u", rc);
			return rc;
		}

	} else {
		LOG_ER("QTransfer Tmr on Node 2 Failed %llu", qhdl);
		return rc;
	}

	/* Register with MQD the change in status to MQSV_QUEUE_OWN_STATE_PROGRESS */
	memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
	opr.type = ASAPi_OPR_MSG;
	opr.info.msg.opr = ASAPi_MSG_SEND;

	opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	opr.info.msg.sinfo.dest = cb->mqd_dest;
	opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	opr.info.msg.req.msgtype = ASAPi_MSG_REG;
	opr.info.msg.req.info.reg.objtype = ASAPi_OBJ_QUEUE;
	opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
	opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
	opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
	opr.info.msg.req.info.reg.queue.owner = MQSV_QUEUE_OWN_STATE_PROGRESS;
	opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
	opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
	opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

	memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
	       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

	/* Request the ASAPi */
	rc = asapi_opr_hdlr(&opr);
	if (rc == SA_AIS_ERR_TIMEOUT) {
		/*This is the case when REG request is assumed to be lost or not proceesed at MQD becoz of failover.So
		   to compensate for the lost request we send an async REG request to MQD */
		/*Request the ASAPi (at MQD) for queue REG */
		TRACE_1("UNLINK TIMEOUT:Async Reg of Queue");
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SND;

		opr.info.msg.req.msgtype = ASAPi_MSG_REG;
		opr.info.msg.req.info.reg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
		opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
		opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
		opr.info.msg.req.info.reg.queue.owner = MQSV_QUEUE_OWN_STATE_PROGRESS;
		opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
		opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
		opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

		memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
		       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));
		rc = asapi_opr_hdlr(&opr);	/*May be insert a log */
		err = SA_AIS_OK;
	} else {
		if (rc != SA_AIS_OK || (!opr.info.msg.resp) || (opr.info.msg.resp->info.rresp.err.flag)) {
			LOG_ER("ERR_LIBRARY: Asapi Opr handler for Queue Register Failed");
			qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_ORPHAN;
			err = SA_AIS_ERR_LIBRARY;
		} else
			err = SA_AIS_OK;
	}

	/* Free the ASAPi response Event */
	if (opr.info.msg.resp)
		asapi_msg_free(&opr.info.msg.resp);

 send_rsp:
	/* Send the response */
	transfer_rsp.type = MQSV_EVT_MQP_RSP;
	transfer_rsp.msg.mqp_rsp.type = MQP_EVT_TRANSFER_QUEUE_RSP;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.old_queueHandle = req->msg.mqp_req.info.transferReq.old_queueHandle;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.openReq = req->msg.mqp_req.info.transferReq.openReq;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.invocation = req->msg.mqp_req.info.transferReq.invocation;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.rcvr_mqa_sinfo = req->msg.mqp_req.info.transferReq.rcvr_mqa_sinfo;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.openType = req->msg.mqp_req.info.transferReq.openType;
	transfer_rsp.msg.mqp_rsp.info.transferRsp.old_owner = cb->my_dest;
	transfer_rsp.msg.mqp_rsp.error = err;

	rc = mqnd_mds_send_rsp(cb, &req->sinfo, &transfer_rsp);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_2("Queue Attribute get :Mds Send Response Failed %" PRIx64, cb->my_dest);
	else
		/* delete Message Queue Objetc at IMMSV */
	if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &qnode->qinfo.queueName) != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiRtObjectDelete: Deletion of MsgQueue object %s", qnode->qinfo.queueName.value);
		return NCSCC_RC_FAILURE;
	}

	if (mqsv_message_cpy)
		m_MMGR_FREE_MQND_DEFAULT(mqsv_message_cpy);

	TRACE_LEAVE();
	return rc;
}

static uint32_t mqnd_send_transfer_owner_req(MQND_CB *cb, MQP_REQ_MSG *mqp_req,
					  MQSV_SEND_INFO *rcvr_mqa_sinfo, MDS_DEST *old_owner,
					  SaMsgQueueHandleT old_hdl, MQP_REQ_TYPE openType)
{
	MQSV_EVT transfer_req;
	MQND_QTRANSFER_EVT_NODE *qevt_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQP_OPEN_REQ *open = NULL;
	SaTimeT timeout = 0;
	TRACE_ENTER();

	/* Build and send transfer ownership request to remote MQND */
	memset(&transfer_req, 0, sizeof(MQSV_EVT));

	transfer_req.type = MQSV_EVT_MQP_REQ;
	transfer_req.msg.mqp_req.type = MQP_EVT_TRANSFER_QUEUE_REQ;

	if (mqp_req->type == MQP_EVT_OPEN_REQ) {
		open = &mqp_req->info.openReq;
		timeout = open->timeout - 100;	/*Start a timer with reduced magnitude to compensate for the req to reach mqnd */
		timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);
	} else if (mqp_req->type == MQP_EVT_OPEN_ASYNC_REQ) {
		open = &mqp_req->info.openAsyncReq.mqpOpenReq;
		transfer_req.msg.mqp_req.info.transferReq.invocation = mqp_req->info.openAsyncReq.invocation;
		timeout = MQND_QTRANSFER_REQ_TIMER;	/*Async req the timer at mqa is 10 secs so start one here of 9.99 secs */
	}

	transfer_req.msg.mqp_req.info.transferReq.openReq = *open;
	transfer_req.msg.mqp_req.info.transferReq.old_queueHandle = old_hdl;
	transfer_req.msg.mqp_req.info.transferReq.new_owner = cb->my_dest;
	transfer_req.msg.mqp_req.info.transferReq.rcvr_mqa_sinfo = *rcvr_mqa_sinfo;
	transfer_req.msg.mqp_req.info.transferReq.openType = openType;

	if (open->openFlags & SA_MSG_QUEUE_EMPTY)
		transfer_req.msg.mqp_req.info.transferReq.empty_queue = true;
	else
		transfer_req.msg.mqp_req.info.transferReq.empty_queue = false;

	transfer_req.sinfo.to_svc = NCSMDS_SVC_ID_MQND;
	transfer_req.sinfo.dest = cb->my_dest;
	transfer_req.sinfo.stype = MDS_SENDTYPE_RSP;

	/* Send transfer request to remote MQND */
	rc = mqnd_mds_send(cb, NCSMDS_SVC_ID_MQND, *old_owner, &transfer_req);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("Unable to send the respons in async case");
		return rc;
	}

	qevt_node = m_MMGR_ALLOC_MQND_QTRANSFER_EVT_NODE;
	if (!qevt_node) {
		LOG_CR("Queue Transfer Event node Alloc Failed for handle %llu", old_hdl);
		goto free_mem;
	}

	memset(qevt_node, 0, sizeof(MQND_QTRANSFER_EVT_NODE));

	qevt_node->addr = *old_owner;
	qevt_node->tmr.type = MQND_TMR_TYPE_NODE1_QTRANSFER;
	qevt_node->tmr.qhdl = old_hdl;
	qevt_node->tmr.uarg = cb->cb_hdl;
	qevt_node->tmr.tmr_id = 0;
	qevt_node->tmr.is_active = false;

	rc = mqnd_tmr_start(&qevt_node->tmr, timeout);
	if (rc == NCSCC_RC_SUCCESS)
		TRACE_1("QTransfer Tmr on Node 1 started for handle %llu", old_hdl);
	else {
		LOG_ER("QTransfer Tmr on Node 1 failed for handle %llu", old_hdl);
		goto free_mem;
	}

	rc = mqnd_qevt_node_add(cb, qevt_node);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("Adding the Q Trans Evt node to the DB failed for handle %llu", old_hdl);
		goto qevt_fail;
	}
	return rc;

 qevt_fail:
	if (qevt_node)
		mqnd_tmr_stop(&qevt_node->tmr);
 free_mem:
	if (qevt_node)
		m_MMGR_FREE_MQND_QTRANSFER_EVT_NODE(qevt_node);
	TRACE_LEAVE();
	return rc;
}

uint32_t mqnd_evt_proc_mqp_qtransfer_response(MQND_CB *cb, MQSV_EVT *evt)
{
	MQSV_EVT transfer_complete;
	MQP_TRANSFERQ_RSP *transfer_rsp = NULL;
	MQND_QTRANSFER_EVT_NODE *qevt_node = NULL;
	SaMsgQueueHandleT qhdl = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT err = SA_AIS_OK;
	MQP_REQ_MSG mqp_req;
	TRACE_ENTER();

	transfer_rsp = &evt->msg.mqp_rsp.info.transferRsp;
	if (transfer_rsp == NULL) {
		LOG_ER("ERR_BAD_HANDLE: Reponse to be transferred is NULL");
		err = SA_AIS_ERR_BAD_HANDLE;
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	/*Received Transfer Req Response so in case of Async open req remove the event node,stop the timer and free the memory */
	mqnd_qevt_node_get(cb, transfer_rsp->old_queueHandle, &qevt_node);
	if (!qevt_node) {
		LOG_ER("ERR_RESOURCES: Get queue node Failed");
		err = SA_AIS_ERR_NO_RESOURCES;
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	mqnd_tmr_stop(&qevt_node->tmr);
	mqnd_qevt_node_del(cb, qevt_node);

	if (evt->msg.mqp_rsp.error != SA_AIS_OK) {
		err = evt->msg.mqp_rsp.error;
		LOG_ER("Transfer Request to the remote MQND is failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Create the queue locally and set the ownership flag from PROGRESS to OWNED */
	rc = mqnd_queue_create(cb, &transfer_rsp->openReq, &transfer_rsp->rcvr_mqa_sinfo.dest, &qhdl, transfer_rsp,
			       &err);
	if (rc != NCSCC_RC_SUCCESS) {
		if (err != SA_AIS_ERR_TRY_AGAIN)
			err = SA_AIS_ERR_NO_RESOURCES;
		LOG_ER("ERR_RESOURCES: Queue Creation Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Send the Queue remove message to old MQND */
	memset(&transfer_complete, 0, sizeof(MQSV_EVT));
	transfer_complete.type = MQSV_EVT_MQP_REQ;
	transfer_complete.msg.mqp_req.type = MQP_EVT_TRANSFER_QUEUE_COMPLETE;
	transfer_complete.msg.mqp_req.info.transferComplete.queueHandle = transfer_rsp->old_queueHandle;
	transfer_complete.msg.mqp_req.info.transferComplete.error = NCSCC_RC_SUCCESS;
	rc = mqnd_mds_send(cb, NCSMDS_SVC_ID_MQND, transfer_rsp->old_owner, &transfer_complete);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("ERR_TIMEOUT: Unable to send the respons in async case");
		err = SA_AIS_ERR_TIMEOUT;
	}

 done:
	/* Send response back to MQA */
	mqp_req.type = transfer_rsp->openType;

	if (transfer_rsp->openType == MQP_EVT_OPEN_REQ)
		mqp_req.info.openReq = transfer_rsp->openReq;
	else if (transfer_rsp->openType == MQP_EVT_OPEN_ASYNC_REQ) {
		mqp_req.info.openAsyncReq.mqpOpenReq = transfer_rsp->openReq;
		mqp_req.info.openAsyncReq.invocation = transfer_rsp->invocation;
	}

	mqnd_send_mqp_open_rsp(cb, &transfer_rsp->rcvr_mqa_sinfo, &mqp_req, err, qhdl, transfer_rsp->msg_count);

	if (transfer_rsp && transfer_rsp->mqsv_message) {
		m_MMGR_FREE_MQSV_OS_MEMORY(transfer_rsp->mqsv_message);
		transfer_rsp->mqsv_message = NULL;
	}

	TRACE_LEAVE();
	return rc;
}

uint32_t mqnd_fill_queue_from_transfered_buffer(MQND_CB *cb, MQND_QUEUE_NODE *qnode, MQP_TRANSFERQ_RSP *transfer_rsp)
{
	uint32_t rc = NCSCC_RC_SUCCESS, i = 0;
	MQSV_MESSAGE *mqsv_message;
	uint32_t offset = 0;
	uint32_t size = 0;
	NCS_OS_POSIX_MQ_REQ_INFO info;
	if ((transfer_rsp->openReq.openFlags & SA_MSG_QUEUE_EMPTY) || (transfer_rsp->msg_count == 0)) {
		mqnd_reset_queue_stats(cb, qnode->qinfo.shm_queue_index);
		return rc;
	}

	/* Resizze the new queue so that its able to hold all the incoming messages */
	memset(&info, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));

	info.req = NCS_OS_POSIX_MQ_REQ_RESIZE;
	info.info.resize.mqd = qnode->qinfo.queueHandle;

	/* Increase queue size so that its able to hold all messages + their MQSV + LEAP headers */
	info.info.resize.i_newqsize = transfer_rsp->msg_bytes +
	    (transfer_rsp->msg_count * (sizeof(MQSV_MESSAGE) + sizeof(NCS_OS_MQ_MSG_LL_HDR)));

	if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS) {
		LOG_ER("Unable to resize the queue to the given size");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	/* Receive all the messages and send it to the queue. */
	for (i = 0; i < transfer_rsp->msg_count; i++) {

		mqsv_message = (MQSV_MESSAGE *)&transfer_rsp->mqsv_message[offset];

		m_MQND_NTOH_MQSV_MESSAGE(mqsv_message);

		size = (uint32_t)(sizeof(MQSV_MESSAGE) + mqsv_message->info.msg.message.size);
		offset += size;

		/* Update the stats */
		mqnd_send_msg_update_stats_shm(cb, qnode, mqsv_message->info.msg.message.size,
					       mqsv_message->info.msg.message.priority);

		rc = mqnd_mq_msg_send(qnode->qinfo.queueHandle, mqsv_message, (uint32_t)size);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_2("Unable to send the message to the Queue");
			return rc;
		}
	}

	return NCSCC_RC_SUCCESS;
}

uint32_t mqnd_existing_queue_open(MQND_CB *cb, MQSV_SEND_INFO *sinfo, MQP_OPEN_REQ *open,
			       ASAPi_NRESOLVE_RESP_INFO *qinfo, SaAisErrorT *err, uint32_t *existing_msg_count)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	MQND_QUEUE_NODE tmpnode, *qnode = NULL;
	uint32_t priority = 0, offset;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	bool is_q_reopen = true;
	TRACE_ENTER();

	mqnd_queue_node_get(cb, qinfo->oinfo.qparam->hdl, &qnode);

	if (!qnode) {
		LOG_ER("ERR_LIBRARY: Get queue node Failed");
		*err = SA_AIS_ERR_LIBRARY;
		return NCSCC_RC_FAILURE;
	}

	/* make a copy, this can be used to restore settings in case of error */
	tmpnode = *qnode;

	*existing_msg_count = 0;
	qnode->qinfo.queueStatus.closeTime = 0;

	if (open->openFlags & SA_MSG_QUEUE_EMPTY) {
		/* Update the Stats */
		mqnd_reset_queue_stats(cb, qnode->qinfo.shm_queue_index);
	} else {
		shm_base_addr = cb->mqnd_shm.shm_base_addr;
		offset = qnode->qinfo.shm_queue_index;
		for (priority = SA_MSG_MESSAGE_HIGHEST_PRIORITY; priority <= SA_MSG_MESSAGE_LOWEST_PRIORITY; priority++) {
			*existing_msg_count +=
			    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[priority].numberOfMessages;
		}
	}

	/* Create a listener queue and empty it if the RECEIVED_CALLBACK is enabled for the queue */
	if (open->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK) {
		if (mqnd_listenerq_create(&qnode->qinfo) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_RESOURCES: Listener Queue creation Failed");
			*err = SA_AIS_ERR_NO_RESOURCES;
			goto error1;
		}
	}

	/* Change the queue owner from orphan to owned */
	if (qnode->qinfo.owner_flag == MQSV_QUEUE_OWN_STATE_ORPHAN) {
		SaUint32T isQueueOpen = true;
		qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_OWNED;
		/*update the Message Queue Owner update to IMMSV */
		immutil_update_one_rattr(cb->immOiHandle, (char *)qnode->qinfo.queueName.value,
					 "saMsgQueueIsOpened", SA_IMM_ATTR_SAUINT32T, &isQueueOpen);
		qnode->qinfo.rcvr_mqa = sinfo->dest;
		qnode->qinfo.msgHandle = open->msgHandle;
	}

	/* Emptying the queue is to be done as last step coz if done earlier and if so other error occurrs, we will need
	   to put in an elaborate logic in place to refill the queue with the messages that were deleted */
	if (open->openFlags & SA_MSG_QUEUE_EMPTY) {
		if (mqnd_mq_empty(qinfo->oinfo.qparam->hdl) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_RESOURCES: MSGQ Empty is not Successfull");
			*err = SA_AIS_ERR_NO_RESOURCES;
			goto error2;
		}
	}

	/*MQND Restart. Here the owner of the queue is updated. Update the checkpoint for this record. */
	memset(&queue_ckpt_node, 0, sizeof(MQND_QUEUE_CKPT_INFO));
	mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

	rc = mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);
	if (rc != SA_AIS_OK) {
		LOG_ER("ERR_RESOURCES: Ckpt Overwrite Failed");
		rc = NCSCC_RC_FAILURE;
		*err = SA_AIS_ERR_NO_RESOURCES;
		goto error2;
	}

	/* Update MQD database */
	rc = mqnd_queue_reg_with_mqd(cb, qnode, err, is_q_reopen);
	if (rc != NCSCC_RC_SUCCESS) {
		if (*err != SA_AIS_ERR_TRY_AGAIN)
			*err = SA_AIS_ERR_NO_RESOURCES;
		LOG_ER("Queue Reg with MQD failed with return code %u", rc);
		goto error2;
	}

	/* Stop any retention timer that has been started */
	mqnd_tmr_stop(&qnode->qinfo.tmr);

	*err = SA_AIS_OK;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 error2:
	mqnd_listenerq_destroy(&qnode->qinfo);
 error1:
	*qnode = tmpnode;
	qnode->qinfo.listenerHandle = 0;
	mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);
	mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);

	return rc;
}

uint32_t mqnd_proc_queue_open(MQND_CB *cb, MQP_REQ_MSG *mqp_req, MQSV_SEND_INFO *sinfo, ASAPi_NRESOLVE_RESP_INFO *qinfo)
{
	uint32_t rc;
	SaMsgQueueCreationAttributesT cre_attr;
	SaMsgQueueHandleT qhdl = 0;
	bool is_qexists = true;
	MQP_OPEN_REQ *open = NULL;
	MQND_QUEUE_NODE *qnode = NULL;
	uint32_t existing_msg_count = 0;
	SaAisErrorT err = SA_AIS_OK;
	TRACE_ENTER();

	if (mqp_req->type == MQP_EVT_OPEN_REQ)
		open = &mqp_req->info.openReq;
	else if (mqp_req->type == MQP_EVT_OPEN_ASYNC_REQ)
		open = &mqp_req->info.openAsyncReq.mqpOpenReq;

	/* Check if Q exists */
	if (m_MQND_IS_Q_NOT_EXIST(qinfo->err.errcode))
		is_qexists = false;

	/* Queue exists */
	if (is_qexists) {

		/* Get the attributes of existing queue  */
		memset(&cre_attr, 0, sizeof(SaMsgQueueCreationAttributesT));

		cre_attr.creationFlags = qinfo->oinfo.qparam->creationFlags;
		cre_attr.retentionTime = qinfo->oinfo.qparam->retentionTime;
		memcpy(cre_attr.size, qinfo->oinfo.qparam->size,
		       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

		/* SA_MSG_QUEUE_CREATE is set */
		if (m_MQND_IS_SA_MSG_QUEUE_CREATE_SET(open->openFlags)) {
			/*Match the received creation attributes with the attributes of the existing message queue
			   If not matching, a queue of the same name but with different attributes already exists
			   Therefore, send SA_AIS_ERR_EXIST message back to MQA */
			if ((mqnd_compare_create_attr(&open->creationAttributes, &cre_attr)) == false) {
				LOG_ER("ERR_EXIST: Comparison between the Receive attributes and"
					"existing queue attributes Failed");
				err = SA_AIS_ERR_EXIST;
				goto error;
			}
		} else
			/* If create flag is not set, update the creation attributes from the creation attributes in the database */
			open->creationAttributes = cre_attr;

		/* If queue is opened by some body, return SA_AIS_ERR_BUSY to MQA */
		if (qinfo->oinfo.qparam->owner == MQSV_QUEUE_OWN_STATE_PROGRESS) {
			LOG_ER("ERR_TRY_AGAIN: MSGQ Transfer in progress");
			err = SA_AIS_ERR_TRY_AGAIN;
			goto error;
		}

		/* If queue is opened by some body, return SA_AIS_ERR_BUSY to MQA */
		if (qinfo->oinfo.qparam->owner != MQSV_QUEUE_OWN_STATE_ORPHAN) {
			LOG_ER("%s:%u: ERR_BUSY: MSGQ is Busy (Opened by someone)", __FILE__, __LINE__);
			err = SA_AIS_ERR_BUSY;
			goto error;
		}

		/* If NON-LOCAL queue, initiate transfer of queue from current owner MQND */
		if (m_NCS_MDS_DEST_EQUAL(&qinfo->oinfo.qparam->addr, &cb->my_dest) == 0) {
			qhdl = qinfo->oinfo.qparam->hdl;
			/* Send the transfer request to remote MQND */
			rc = mqnd_send_transfer_owner_req(cb, mqp_req, sinfo,
							  &qinfo->oinfo.qparam->addr, qinfo->oinfo.qparam->hdl,
							  mqp_req->type);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE_2("The Queue Transfer owner request Failed");
			return rc;
		} else {	/* LOCAL queue */
			rc = mqnd_existing_queue_open(cb, sinfo, open, qinfo, &err, &existing_msg_count);

			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_2("Attempt to open the existing local message queue failed with %u", rc); 
				goto error;
			}

			qhdl = qinfo->oinfo.qparam->hdl;
		}
	} else {		/* Queue does NOT exist */

		/* check if SA_MSG_QUEUE_CREATE flag is set, if not send SA_AIS_ERR_NOT_EXIST to MQA */
		if (!m_MQND_IS_SA_MSG_QUEUE_CREATE_SET(open->openFlags)) {
			LOG_ER("ERR_NOT_EXIST: Queue Creation is not set for a non existing queue");
			err = SA_AIS_ERR_NOT_EXIST;
			goto error;
		}

		/* Create the new Queue and return the handle */
		rc = mqnd_queue_create(cb, open, &sinfo->dest, &qhdl, NULL, &err);
		if (rc != NCSCC_RC_SUCCESS) {
			if (err != SA_AIS_ERR_TRY_AGAIN)
				TRACE_4("ERR_RESOURCES: Queue Creation Failed");
				err = SA_AIS_ERR_NO_RESOURCES;
			goto error;
		}

		mqnd_queue_node_get(cb, qhdl, &qnode);

	}

 error:
	/* Send response back to MQA */
	mqnd_send_mqp_open_rsp(cb, sinfo, mqp_req, err, qhdl, existing_msg_count);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t mqnd_send_mqp_open_rsp(MQND_CB *cb, MQSV_SEND_INFO *sinfo,
			     MQP_REQ_MSG *mqp_req, SaAisErrorT err, uint32_t qhdl, uint32_t existing_msg_count)
{
	MQSV_EVT rsp_evt;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQP_OPEN_REQ *open_req;
	MQND_QUEUE_NODE *qnode = 0;
	TRACE_ENTER();

	mqnd_queue_node_get(cb, qhdl, &qnode);

	if (mqp_req->type == MQP_EVT_OPEN_REQ) {

		open_req = &mqp_req->info.openReq;
		/*Send the resp to MQA */
		memset(&rsp_evt, 0, sizeof(MQSV_EVT));
		rsp_evt.type = MQSV_EVT_MQP_RSP;
		rsp_evt.msg.mqp_rsp.error = err;
		rsp_evt.msg.mqp_rsp.type = MQP_EVT_OPEN_RSP;
		rsp_evt.msg.mqp_rsp.info.openRsp.msgHandle = open_req->msgHandle;
		rsp_evt.msg.mqp_rsp.info.openRsp.queueName = open_req->queueName;

		if (err == SA_AIS_OK) {
			rsp_evt.msg.mqp_rsp.info.openRsp.queueHandle = qhdl;
			rsp_evt.msg.mqp_rsp.info.openRsp.listenerHandle = qnode->qinfo.listenerHandle;
			rsp_evt.msg.mqp_rsp.info.openRsp.existing_msg_count = existing_msg_count;
		}

		rc = mqnd_mds_send_rsp(cb, sinfo, &rsp_evt);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Queue Attribute get :Mds Send Response Failed %" PRIx64, sinfo->dest);
	} else if (mqp_req->type == MQP_EVT_OPEN_ASYNC_REQ) {

		open_req = &mqp_req->info.openAsyncReq.mqpOpenReq;
		rsp_evt.type = MQSV_EVT_MQA_CALLBACK;
		rsp_evt.msg.mqp_async_rsp.callbackType = MQP_ASYNC_RSP_OPEN;
		rsp_evt.msg.mqp_async_rsp.messageHandle = open_req->msgHandle;
		rsp_evt.msg.mqp_async_rsp.params.qOpen.error = err;
		rsp_evt.msg.mqp_async_rsp.params.qOpen.invocation = mqp_req->info.openAsyncReq.invocation;
		rsp_evt.msg.mqp_async_rsp.params.qOpen.openFlags = open_req->openFlags;

		if (err == SA_AIS_OK) {
			rsp_evt.msg.mqp_async_rsp.params.qOpen.queueHandle = qhdl;
			rsp_evt.msg.mqp_async_rsp.params.qOpen.listenerHandle = qnode->qinfo.listenerHandle;
			rsp_evt.msg.mqp_async_rsp.params.qOpen.existing_msg_count = existing_msg_count;
		}

		rc = mqnd_mds_send(cb, sinfo->to_svc, sinfo->dest, &rsp_evt);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Unable to send the respons in async case");

	}

	TRACE_LEAVE();
	return rc;
}

uint32_t mqnd_proc_queue_close(MQND_CB *cb, MQND_QUEUE_NODE *qnode, SaAisErrorT *err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_OPR_INFO opr;
	SaTimeT timeout;
	MQND_QNAME_NODE *pnode = 0;
	SaNameT qname;
	*err = SA_AIS_OK;
	MQND_QUEUE_CKPT_INFO queue_ckpt_node;
	SaUint32T isQueueOpen = false;
	TRACE_ENTER();

	if (qnode == NULL) {
		LOG_ER("ERR_RESOURCES: MQND_QUEUE_NODE is NULL");
		*err = SA_AIS_ERR_NO_RESOURCES;
		return NCSCC_RC_FAILURE;
	}
	timeout = m_NCS_CONVERT_SATIME_TO_TEN_MILLI_SEC(qnode->qinfo.queueStatus.retentionTime);

	if ((qnode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE) ||
	    (!(qnode->qinfo.queueStatus.creationFlags & SA_MSG_QUEUE_PERSISTENT) && (timeout == 0))) {

		if (qnode->qinfo.sendingState == MSG_QUEUE_AVAILABLE) {

			/* Deregister the Queue with ASAPi */
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
				/* Deregister the Queue with ASAPi */
				TRACE_1("CLOSE TIMEOUT:ASYNC DEREG TO DELETE QUEUE");
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
				if ((rc != SA_AIS_OK) || (!opr.info.msg.resp)
				    || (opr.info.msg.resp->info.dresp.err.flag)) {
					LOG_ER("Asapi Opr Handler for Queue Dereg Failed");
					return NCSCC_RC_FAILURE;
				}
			}

			/* Free the ASAPi response Event */
			if (opr.info.msg.resp)
				asapi_msg_free(&opr.info.msg.resp);
		}

		/* Delete the mapping entry from the qname database */
		memset(&qname, 0, sizeof(SaNameT));
		qname = qnode->qinfo.queueName;

		mqnd_qname_node_get(cb, qname, &pnode);
		if (pnode) {
			mqnd_qname_node_del(cb, pnode);
			m_MMGR_FREE_MQND_QNAME_NODE(pnode);
		}

		/* Delete the queue */
		mqnd_mq_destroy(&qnode->qinfo);

		/* Delete the listener queue if it exists */
		mqnd_listenerq_destroy(&qnode->qinfo);

		/*Invalidate the shm area of the queue */
		mqnd_shm_queue_ckpt_section_invalidate(cb, qnode);

		/* Delete the Queue Node */
		mqnd_queue_node_del(cb, qnode);

		if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &qname) != SA_AIS_OK) {
			LOG_ER("Deletion of MsgQueue object %s FAILED",qnode->qinfo.queueName.value);
			return NCSCC_RC_FAILURE;
		}

		/* Free the Queue Node */
		m_MMGR_FREE_MQND_QUEUE_NODE(qnode);

	} else {
		if (qnode->qinfo.owner_flag == MQSV_QUEUE_OWN_STATE_ORPHAN) {
			*err = SA_AIS_OK;
			return NCSCC_RC_SUCCESS;
		}

		/* Queue is not unlinked */
		qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_ORPHAN;

		/* Request the ASAPi */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_REG;
		opr.info.msg.req.info.reg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
		opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
		opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
		opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
		opr.info.msg.req.info.reg.queue.owner = qnode->qinfo.owner_flag;
		opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
		opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
		opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

		memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
		       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

		/* Request the ASAPi */
		rc = asapi_opr_hdlr(&opr);
		if (rc == SA_AIS_ERR_TIMEOUT) {
			/*This is the case when REG request is assumed to be lost or not proceesed at MQD becoz of failover.So
			   to compensate for the lost request we send an async request to MQD to process it once again to be
			   in sync with database at MQND */
			/* Deregister the Queue with ASAPi */
			TRACE_1("CLOSE TIMEOUT:ASYNC REG:OWNED TO ORPHAN ");
			memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
			opr.type = ASAPi_OPR_MSG;
			opr.info.msg.opr = ASAPi_MSG_SEND;

			/* Fill MDS info */
			opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
			opr.info.msg.sinfo.dest = cb->mqd_dest;
			opr.info.msg.sinfo.stype = MDS_SENDTYPE_SND;

			opr.info.msg.req.msgtype = ASAPi_MSG_REG;
			opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
			opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;
			opr.info.msg.req.info.reg.queue.addr = cb->my_dest;
			opr.info.msg.req.info.reg.queue.hdl = qnode->qinfo.queueHandle;
			opr.info.msg.req.info.reg.queue.name = qnode->qinfo.queueName;
			opr.info.msg.req.info.reg.queue.owner = qnode->qinfo.owner_flag;
			opr.info.msg.req.info.reg.queue.retentionTime = qnode->qinfo.queueStatus.retentionTime;
			opr.info.msg.req.info.reg.queue.status = qnode->qinfo.sendingState;
			opr.info.msg.req.info.reg.queue.creationFlags = qnode->qinfo.queueStatus.creationFlags;

			memcpy(opr.info.msg.req.info.reg.queue.size, qnode->qinfo.size,
			       sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

			rc = asapi_opr_hdlr(&opr);	/*May be insert a log */
		} else {
			if ((rc != SA_AIS_OK) || (!opr.info.msg.resp) || (opr.info.msg.resp->info.rresp.err.flag)) {
				qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_OWNED;
				LOG_ER("ERR_RESOURCES: Asapi Opr handler for Queue Register Failed");
				*err = SA_AIS_ERR_NO_RESOURCES;
				return NCSCC_RC_FAILURE;
			}
		}

		/* Free the ASAPi response Event */
		if (opr.info.msg.resp)
			asapi_msg_free(&opr.info.msg.resp);

		/* Delete the listener queue if it exists */
		mqnd_listenerq_destroy(&qnode->qinfo);
		qnode->qinfo.listenerHandle = 0;

		m_GET_TIME_STAMP(qnode->qinfo.queueStatus.closeTime);

		/* Update checkpoint section. Queue closeTime needs to be checkpointed with new value */
		memset(&queue_ckpt_node, 0, sizeof(MQND_QUEUE_CKPT_INFO));
		mqnd_cpy_qnodeinfo_to_ckptinfo(cb, qnode, &queue_ckpt_node);

		rc = mqnd_ckpt_queue_info_write(cb, &queue_ckpt_node, qnode->qinfo.shm_queue_index);

		if (rc != SA_AIS_OK) {
			/* Log that CKPT Delete is not successfull */
			qnode->qinfo.owner_flag = MQSV_QUEUE_OWN_STATE_OWNED;
			LOG_ER("ERR_RESOURCES: Checkpointing queue info to local shared memory of MQND");
			*err = SA_AIS_ERR_NO_RESOURCES;
			return NCSCC_RC_FAILURE;
		}

		/*update the Message Queue Owner update to IMMSV */
		immutil_update_one_rattr(cb->immOiHandle, (char *)qnode->qinfo.queueName.value,
					 "saMsgQueueIsOpened", SA_IMM_ATTR_SAUINT32T, &isQueueOpen);

		/* start retention timer */
		if (!(qnode->qinfo.queueStatus.creationFlags & SA_MSG_QUEUE_PERSISTENT)) {
			/* Start the retention Timer */
			qnode->qinfo.tmr.type = MQND_TMR_TYPE_RETENTION;
			qnode->qinfo.tmr.qhdl = qnode->qinfo.queueHandle;
			qnode->qinfo.tmr.uarg = cb->cb_hdl;
			qnode->qinfo.tmr.tmr_id = 0;
			qnode->qinfo.tmr.is_active = false;
			rc = mqnd_tmr_start(&qnode->qinfo.tmr, timeout);
			TRACE_1("Retention timer has been strted for the queue");
		}
	}

	TRACE_LEAVE();
	return rc;
}

uint32_t mqnd_send_mqp_close_rsp(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err, uint32_t qhdl)
{
	MQSV_EVT rsp_evt;
	uint32_t rc;

	/*Send the resp to MQA */
	memset(&rsp_evt, 0, sizeof(MQSV_EVT));
	rsp_evt.type = MQSV_EVT_MQP_RSP;
	rsp_evt.msg.mqp_rsp.error = err;

	rsp_evt.msg.mqp_rsp.type = MQP_EVT_CLOSE_RSP;
	rsp_evt.msg.mqp_rsp.info.closeRsp.queueHandle = qhdl;

	rc = mqnd_mds_send_rsp(cb, sinfo, &rsp_evt);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_2("Queue Attribute get :Mds Send Response Failed %" PRIx64, sinfo->dest);

	return rc;
}

uint32_t mqnd_send_mqp_ulink_rsp(MQND_CB *cb, MQSV_SEND_INFO *sinfo, SaAisErrorT err, MQP_UNLINK_REQ *ulink_req)
{
	MQSV_EVT rsp_evt;
	uint32_t rc;

	/*Send the resp to MQA */
	memset(&rsp_evt, 0, sizeof(MQSV_EVT));
	rsp_evt.type = MQSV_EVT_MQP_RSP;
	rsp_evt.msg.mqp_rsp.error = err;

	rsp_evt.msg.mqp_rsp.type = MQP_EVT_UNLINK_RSP;
	rsp_evt.msg.mqp_rsp.info.unlinkRsp.queueHandle = ulink_req->queueHandle;
	rsp_evt.msg.mqp_rsp.info.unlinkRsp.msgHandle = ulink_req->msgHandle;
	rsp_evt.msg.mqp_rsp.info.unlinkRsp.queueName = ulink_req->queueName;
	rc = mqnd_mds_send_rsp(cb, sinfo, &rsp_evt);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_2("Queue Attribute get :Mds Send Response Failed %" PRIx64, sinfo->dest);
	return rc;
}

 /*****************************************************************************************
 *
 *  Name  : mqnd_clm_cluster_track_cbk 
 *
 *  Description : We will get the callback from CLM for the cluster track
 *
 ******************************************************************************************/
void mqnd_clm_cluster_track_cbk(const SaClmClusterNotificationBufferT *notificationBuffer,
				SaUint32T numberOfMembers, SaAisErrorT error)
{
	MQND_CB *cb;
	uint32_t counter = 0;
	uint32_t cb_hdl = m_MQND_GET_HDL();

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (cb == NULL) {
		LOG_ER("Getting the callback from CLM for the cluster track failed");
		return;
	} else {
		for (counter = 0; counter < notificationBuffer->numberOfItems; counter++) {
			/* Proceed further only if event is for my node */
			if (cb->nodeid == notificationBuffer->notification[counter].clusterNode.nodeId) {
				if (notificationBuffer->notification[counter].clusterChange == SA_CLM_NODE_LEFT) {
					mqnd_proc_ckpt_clm_node_left(cb);
					cb->clm_node_joined = 0;
				} else
				    if ((notificationBuffer->notification[counter].clusterChange ==
					 SA_CLM_NODE_NO_CHANGE)
					|| (notificationBuffer->notification[counter].clusterChange ==
					    SA_CLM_NODE_JOINED)
					|| (notificationBuffer->notification[counter].clusterChange ==
					    SA_CLM_NODE_RECONFIGURED)) {
					mqnd_proc_ckpt_clm_node_joined(cb);
					cb->clm_node_joined = 1;
				}
			}
		}
	}

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	return;
}

/**************************************************************************
 * Name         : mqnd_proc_ckpt_clm_node_left
 *
 * Description  :   
 *
 *************************************************************************/
void mqnd_proc_ckpt_clm_node_left(MQND_CB *cb)
{

	MQSV_EVT send_evt;

	memset(&send_evt, '\0', sizeof(MQSV_EVT));
	send_evt.type = MQSV_EVT_MQP_REQ;
	send_evt.msg.mqp_req.type = MQP_EVT_CLM_NOTIFY;
	send_evt.msg.mqp_req.info.clmNotify.node_joined = 0;

	mqnd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_MQA);

	return;
}

/**************************************************************************
 * Name         : mqnd_proc_ckpt_clm_node_joined
 *
 * Description  :    
 *
 *************************************************************************/
void mqnd_proc_ckpt_clm_node_joined(MQND_CB *cb)
{

	MQSV_EVT send_evt;

	memset(&send_evt, '\0', sizeof(MQSV_EVT));
	send_evt.type = MQSV_EVT_MQP_REQ;
	send_evt.msg.mqp_req.type = MQP_EVT_CLM_NOTIFY;
	send_evt.msg.mqp_req.info.clmNotify.node_joined = 1;

	mqnd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_MQA);

	return;

}
