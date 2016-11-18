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
/******************************************************************************/
#include "mqnd.h"
#include "mqnd_imm.h"
extern struct ImmutilWrapperProfile immutilWrapperProfile;
static SaUint32T getdata_from_mqd(MQND_CB *, MQND_QUEUE_NODE *);
static SaAisErrorT mqnd_saImmOiRtAttrUpdateCallback(SaImmOiHandleT, const SaNameT *, const SaImmAttrNameT *);
SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = NULL,
	.saImmOiCcbApplyCallback = NULL,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = NULL,
	.saImmOiRtAttrUpdateCallback = mqnd_saImmOiRtAttrUpdateCallback
};

/* IMMSv Defs */
#define MQND_IMM_RELEASE_CODE 'A'
#define MQND_IMM_MAJOR_VERSION 0x02
#define MQND_IMM_MINOR_VERSION 0x01

static SaVersionT imm_version = {
	MQND_IMM_RELEASE_CODE,
	MQND_IMM_MAJOR_VERSION,
	MQND_IMM_MINOR_VERSION
};

/****************************************************************************
 * Name          : mqnd_saImmOiRtAttrUpdateCallback 
 *
 * Description   : This callback function is invoked when a OM requests for object 
 *                 information .
 *
 * Arguments     : immOiHandle      - IMM handle
 *                 objectName       - Object name (DN) 
 *                 attributeNames   - attribute names of the object to be updated
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
static SaAisErrorT mqnd_saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
						    const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	int i = 0, attrCnt = 0;
	SaNameT mQueueName;
	SaUint32T attr2 = 0, attr3 = 0, value = 0;
	SaUint64T attr1 = 0;
	SaImmAttrNameT attributeName;
	SaImmAttrModificationT_2 attr_output[3];
	const SaImmAttrModificationT_2 *attrMods[4];

	SaImmAttrValueT attrUpdateValues1[] = { &attr1 };
	SaImmAttrValueT attrUpdateValues2[] = { &attr2 };
	SaImmAttrValueT attrUpdateValues3[] = { &attr3 };

	MQND_CB *mqnd_cb = NULL;
	MQND_QUEUE_NODE *qNode = NULL;
	MQND_QNAME_NODE *pNode = NULL;
	MQND_QUEUE_CKPT_INFO *shmBaseAddr = NULL;
	uint32_t offset;
	TRACE_ENTER();

	uint32_t cb_hdl = m_MQND_GET_HDL();
	mqnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (!mqnd_cb) {
		LOG_ER("ERR_FAILED_OPERATION: CB Take Failed");
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	memset(&mQueueName, 0, sizeof(SaNameT));

	if (strncmp(((char *)objectName->value), "safMqPrio=", 10) == 0) {
		char *mQPrio = strchr((char *)objectName->value, ',');
		if (mQPrio != NULL) {
			mQPrio++;
			strncpy((char *)mQueueName.value, mQPrio, strlen(mQPrio));
			mQueueName.length = strlen(mQPrio);
		} else
			return SA_AIS_ERR_FAILED_OPERATION;
	} else {
		strncpy((char *)mQueueName.value, (char *)objectName->value, objectName->length);
		mQueueName.length = objectName->length;
	}
	mqnd_qname_node_get(mqnd_cb, mQueueName, &pNode);

	if (pNode == NULL) {
		ncshm_give_hdl(cb_hdl);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	mqnd_queue_node_get(mqnd_cb, pNode->qhdl, &qNode);

	if (qNode == NULL) {
		ncshm_give_hdl(cb_hdl);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	/* Checking for an unlinked queue */
	if (qNode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE) {
		ncshm_give_hdl(cb_hdl);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	shmBaseAddr = mqnd_cb->mqnd_shm.shm_base_addr;
	offset = qNode->qinfo.shm_queue_index;

	if (m_CMP_HORDER_SANAMET(*objectName, qNode->qinfo.queueName) == 0) {
		/* Walk through the MsgQueue Object attribute Name list */
		while ((attributeName = attributeNames[i]) != NULL) {

			if (strcmp(attributeName, "saMsgQueueUsedSize") == 0) {

				attr1 = shmBaseAddr[offset].QueueStatsShm.totalQueueUsed;
				attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
				attr_output[attrCnt].modAttr.attrName = attributeName;
				attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
				attr_output[attrCnt].modAttr.attrValuesNumber = 1;
				attr_output[attrCnt].modAttr.attrValues = attrUpdateValues1;
				attrMods[attrCnt] = &attr_output[attrCnt];
				++attrCnt;
			} else if (strcmp(attributeName, "saMsgQueueNumMsgs") == 0) {

				attr2 = shmBaseAddr[offset].QueueStatsShm.totalNumberOfMessages;
				attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
				attr_output[attrCnt].modAttr.attrName = attributeName;
				attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
				attr_output[attrCnt].modAttr.attrValuesNumber = 1;
				attr_output[attrCnt].modAttr.attrValues = attrUpdateValues2;
				attrMods[attrCnt] = &attr_output[attrCnt];
				++attrCnt;
			} else if (strcmp(attributeName, "saMsgQueueNumMemberQueueGroups") == 0) {

				value = getdata_from_mqd(mqnd_cb, qNode);
				attr3 = value;
				attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
				attr_output[attrCnt].modAttr.attrName = attributeName;
				attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
				attr_output[attrCnt].modAttr.attrValuesNumber = 1;
				attr_output[attrCnt].modAttr.attrValues = attrUpdateValues3;
				attrMods[attrCnt] = &attr_output[attrCnt];
				++attrCnt;
			}
			i++;
		}		/*End while attributesNames() */
		attrMods[attrCnt] = NULL;
		saImmOiRtObjectUpdate_2(mqnd_cb->immOiHandle, objectName, attrMods);
		TRACE_LEAVE();
		return SA_AIS_OK;
	} /* End if  m_CMP_HORDER_SANAMET */
	else {			/* walk through the MqPriotity Object Attributes List */
		char *indx = strchr((char *)objectName->value, '=');
		int prioVal = 0, j = 0;
		if (indx) {
			prioVal = atoi((indx + 1));
			j = 0;
		} else
			return SA_AIS_ERR_FAILED_OPERATION;

		for (j = SA_MSG_MESSAGE_HIGHEST_PRIORITY; j < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; j++) {
			if (j == prioVal) {
				SaMsgQueueUsageT *qUsage = NULL;
				qUsage = &(shmBaseAddr[offset].QueueStatsShm.saMsgQueueUsage[j]);
				while ((attributeName = attributeNames[i]) != NULL) {
					if (strcmp(attributeName, "saMsgQueuePriorityQUsedSize") == 0) {
						attr1 = qUsage->queueUsed;
						attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attrCnt].modAttr.attrName = attributeName;
						attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
						attr_output[attrCnt].modAttr.attrValuesNumber = 1;
						attr_output[attrCnt].modAttr.attrValues = attrUpdateValues1;
						attrMods[attrCnt] = &attr_output[attrCnt];
						++attrCnt;
					} else if (strcmp(attributeName, "saMsgQueuePriorityQNumMessages") == 0) {

						attr2 = qUsage->numberOfMessages;
						attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attrCnt].modAttr.attrName = attributeName;
						attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attrCnt].modAttr.attrValuesNumber = 1;
						attr_output[attrCnt].modAttr.attrValues = attrUpdateValues2;
						attrMods[attrCnt] = &attr_output[attrCnt];
						++attrCnt;
					} else if (strcmp(attributeName, "saMsgQueuePriorityQNumFullErrors") == 0) {

						attr3 = qNode->qinfo.numberOfFullErrors[j];
						attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attrCnt].modAttr.attrName = attributeName;
						attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attrCnt].modAttr.attrValuesNumber = 1;
						attr_output[attrCnt].modAttr.attrValues = attrUpdateValues3;
						attrMods[attrCnt] = &attr_output[attrCnt];
						++attrCnt;
					}
					i++;
				}	/* end of while */
				attrMods[attrCnt] = NULL;
				saImmOiRtObjectUpdate_2(mqnd_cb->immOiHandle, objectName, attrMods);
				return SA_AIS_OK;
			}	/* enf of <if i==j> */
		}		/* end of <for i=> */
	}			/* end of <else > */
	return SA_AIS_ERR_FAILED_OPERATION;
}

/****************************************************************************
 * Name          : getdata_from_mqd 
 *
 * Description   : This function is called to get saMsgQueueNumMemberQueueGroups attribute value  
 *                 attribute value form MQD .
 *
 * Arguments     : MQND_CB *cb               - MQND Control Block pointer 
 *                 MQND_QUEUE_NODE *pNode    - MQND queue node pointer 
 * 
 * Return Values : SaUint32T param  - The param value returned by MQD   
 *
 * Notes         : None.
 *****************************************************************************/
static SaUint32T getdata_from_mqd(MQND_CB *cb, MQND_QUEUE_NODE *pNode)
{
	SaUint32T param = 0;
	MQSV_EVT req, *rsp = NULL;
	memset(&req, 0, sizeof(MQSV_EVT));
	req.type = MQSV_EVT_MQD_CTRL;
	req.msg.mqd_ctrl.type = MQD_QGRP_CNT_GET;
	req.msg.mqd_ctrl.info.qgrp_cnt_info.info.queueName = pNode->qinfo.queueName;
	TRACE_ENTER();

	/* Send the MDS sync request to remote MQND */
	mqnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_MQD, cb->mqd_dest, &req, &rsp, MQSV_WAIT_TIME);

	if ((rsp) && (rsp->type == MQSV_EVT_MQND_CTRL) &&
	    (rsp->msg.mqnd_ctrl.type == MQND_CTRL_EVT_QGRP_CNT_RSP) &&
	    (rsp->msg.mqnd_ctrl.info.qgrp_cnt_info.error == SA_AIS_OK)) {
		param = rsp->msg.mqnd_ctrl.info.qgrp_cnt_info.info.noOfQueueGroupMemOf;
	} else
		LOG_ER("getdata_from_mqd FAILED");

	if (rsp)
		m_MMGR_FREE_MQSV_EVT(rsp, NCS_SERVICE_IiD_MQND);

	TRACE_LEAVE();
	return param;
}

/****************************************************************************
 * Name          : mqnd_create_runtime_MsgQobject 
 *
 * Description   : This function is invoked to create a runtime MsgQobject object  
 *
 * Arguments     : rname                      - DN of resource
 *                 create_time                - Creation time of the object 
 *		   MQND_QUEUE_NODE *qnode     - Mqnd Queue Node pointer   
 *		   immOiHandle                - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT mqnd_create_runtime_MsgQobject(SaStringT rname, SaTimeT create_time, MQND_QUEUE_NODE *qnode,
					   SaImmOiHandleT immOiHandle)
{
	SaNameT parent, *parentName = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	char *dndup = strdup(rname);
	char *parent_name = strchr(rname, ',');
	char *rdnstr;
	uint32_t open = 0;
	SaImmAttrValueT arr1[1], arr2[1], arr3[1], arr4[1], arr5[1], arr6[1];
	SaImmAttrValuesT_2 attr_mqrsc, attr_mqIspersistent, attr_mqRetTime, attr_mqSize;
	SaImmAttrValuesT_2 attr_mqCreationTimeStamp, attr_mqIsOpen;
	const SaImmAttrValuesT_2 *attrValues[7];
	TRACE_ENTER();

	if (parent_name != NULL && dndup != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;
		parentName = &parent;
		strncpy((char *)parent.value, parent_name, SA_MAX_NAME_LENGTH-1);
		parent.length = strlen((char *)parent.value);
	} else
		rdnstr = rname;

	if (rdnstr)
		arr1[0] = &rdnstr;
	else
		return SA_AIS_ERR_FAILED_OPERATION;

	arr2[0] = &(qnode->qinfo.queueStatus.creationFlags);
	arr3[0] = &(qnode->qinfo.queueStatus.retentionTime);
	arr4[0] = &(qnode->qinfo.totalQueueSize);
	arr5[0] = &create_time;

	if (qnode->qinfo.owner_flag == MQSV_QUEUE_OWN_STATE_ORPHAN)
		open = 0;
	else
		open = 1;
	arr6[0] = &open;

	attr_mqrsc.attrName = "safMq";
	attr_mqrsc.attrValueType = SA_IMM_ATTR_SASTRINGT;
	attr_mqrsc.attrValuesNumber = 1;
	attr_mqrsc.attrValues = arr1;

	attr_mqIspersistent.attrName = "saMsgQueueIsPersistent";
	attr_mqIspersistent.attrValueType = SA_IMM_ATTR_SAUINT32T;
	attr_mqIspersistent.attrValuesNumber = 1;
	attr_mqIspersistent.attrValues = arr2;

	attr_mqRetTime.attrName = "saMsgQueueRetentionTime";
	attr_mqRetTime.attrValueType = SA_IMM_ATTR_SATIMET;
	attr_mqRetTime.attrValuesNumber = 1;
	attr_mqRetTime.attrValues = arr3;

	attr_mqSize.attrName = "saMsgQueueSize";
	attr_mqSize.attrValueType = SA_IMM_ATTR_SAUINT64T;
	attr_mqSize.attrValuesNumber = 1;
	attr_mqSize.attrValues = arr4;

	attr_mqCreationTimeStamp.attrName = "saMsgQueueCreationTimestamp";
	attr_mqCreationTimeStamp.attrValueType = SA_IMM_ATTR_SATIMET;
	attr_mqCreationTimeStamp.attrValuesNumber = 1;
	attr_mqCreationTimeStamp.attrValues = arr5;

	attr_mqIsOpen.attrName = "saMsgQueueIsOpened";
	attr_mqIsOpen.attrValueType = SA_IMM_ATTR_SAUINT32T;
	attr_mqIsOpen.attrValuesNumber = 1;
	attr_mqIsOpen.attrValues = arr6;

	attrValues[0] = &attr_mqrsc;
	attrValues[1] = &attr_mqIspersistent;
	attrValues[2] = &attr_mqRetTime;
	attrValues[3] = &attr_mqSize;
	attrValues[4] = &attr_mqCreationTimeStamp;
	attrValues[5] = &attr_mqIsOpen;
	attrValues[6] = NULL;

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaMsgQueue", parentName, attrValues);

	if (dndup)
		free(dndup);

	TRACE_LEAVE2("Returned with return code %d", rc);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_create_runtime_MsgQPriorityobject
 *      
 * Description   : This function is invoked to create a runtime MsgQPriorityobject  object  
 *              
 * Arguments     : rname                      - DN of resource
 *                 MQND_QUEUE_NODE *qnode     - Mqnd Queue Node pointer   
 * 		   immOiHandle                - IMM handle
 *              
 * Return Values : SaAisErrorT 
 *      
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT mqnd_create_runtime_MsgQPriorityobject(SaStringT rname, MQND_QUEUE_NODE *qnode, SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint64T def_val = 0;
	int i = 0;
	SaImmAttrValueT arr1[1], arr2[1], arr3[1], arr4[1];
	SaImmAttrValuesT_2 attr_mqprio, attr_mqprioSize, attr_capavail, attr_capreached;
	const SaImmAttrValuesT_2 *attrValues[5];
	SaNameT mqp_parent, *mQPrioDn = NULL;
	TRACE_ENTER();

	memset(&mqp_parent, 0, sizeof(SaNameT));
	char *mqprdn = (char *)malloc(sizeof(char) * SA_MAX_NAME_LENGTH);

	strncpy((char *)mqp_parent.value, (char *)rname, strlen((char *)rname));
	mqp_parent.length = strlen((char *)rname);

	mQPrioDn = &mqp_parent;

	for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i < SA_MSG_MESSAGE_LOWEST_PRIORITY + 1; i++) {
		memset(mqprdn, 0, SA_MAX_NAME_LENGTH);
		sprintf(mqprdn, "%s%d", "safMqPrio=", i);

		arr1[0] = &mqprdn;
		arr2[0] = &(qnode->qinfo.size[i]);
		arr3[0] = &def_val;	/* not implemented */
		arr4[0] = &def_val;	/* not implemented */

		attr_mqprio.attrName = "safMqPrio";
		attr_mqprio.attrValueType = SA_IMM_ATTR_SASTRINGT;
		attr_mqprio.attrValuesNumber = 1;
		attr_mqprio.attrValues = arr1;

		attr_mqprioSize.attrName = "saMsgQueuePriorityQSize";
		attr_mqprioSize.attrValueType = SA_IMM_ATTR_SAUINT64T;
		attr_mqprioSize.attrValuesNumber = 1;
		attr_mqprioSize.attrValues = arr2;

		attr_capavail.attrName = "saMsgQueuePriorityCapacityAvailable";
		attr_capavail.attrValueType = SA_IMM_ATTR_SAUINT64T;
		attr_capavail.attrValuesNumber = 1;
		attr_capavail.attrValues = arr3;

		attr_capreached.attrName = "saMsgQueuePriorityCapacityReached";
		attr_capreached.attrValueType = SA_IMM_ATTR_SAUINT64T;
		attr_capreached.attrValuesNumber = 1;
		attr_capreached.attrValues = arr4;

		attrValues[0] = &attr_mqprio;
		attrValues[1] = &attr_mqprioSize;
		attrValues[2] = &attr_capavail;
		attrValues[3] = &attr_capreached;
		attrValues[4] = NULL;

		rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaMsgQueuePriority", mQPrioDn, attrValues);
	}

	if (mqprdn)
		free(mqprdn);

	TRACE_LEAVE2("Returned with return code %d", rc);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_imm_initialize 
 *
 * Description   : Initialize the OI and get selection object  
 *
 * Arguments     : MQND_CB *cb - MQND Control block pointer           
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT mqnd_imm_initialize(MQND_CB *cb)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;
	TRACE_ENTER();

	rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK) {
		immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
	}
	TRACE_LEAVE2("Returned with return code %d", rc);
	return rc;
}

/****************************************************************************
 * Name          : _mqnd_imm_declare_implementer
 *
 * Description   : Become a OI implementer  
 *
 * Arguments     : cb - MQND Control Block pointer            
 *
 * Return Values : None 
 *
 * Notes         : None.
 *****************************************************************************/
void *_mqnd_imm_declare_implementer(void *cb)
{
	SaAisErrorT error = SA_AIS_OK;
	MQND_CB *mqnd_cb = (MQND_CB *)cb;
	TRACE_ENTER();

	SaImmOiImplementerNameT implementer_name;
	char *i_name;
	i_name = (char *)malloc(sizeof(char) * SA_MAX_NAME_LENGTH);
	memset(i_name, 0, SA_MAX_NAME_LENGTH);
	snprintf(i_name, SA_MAX_NAME_LENGTH, "%s%u", "MsgQueueService", mqnd_cb->nodeid);
	implementer_name = i_name;
	error = saImmOiImplementerSet(mqnd_cb->immOiHandle, implementer_name);
	unsigned int nTries = 1;
	while (error == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		error = saImmOiImplementerSet(mqnd_cb->immOiHandle, i_name);
		nTries++;
	}
	if (error != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet FAILED:%u", error);
		exit(EXIT_FAILURE);
	}

	TRACE_LEAVE();
	return NULL;
}

/****************************************************************************
 * Name          : mqnd_imm_declare_implementer
 *
 * Description   : Become a OI implementer  
 *
 * Arguments     : cb - MQND Control Block pointer            
 *
 * Return Values : None 
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_imm_declare_implementer(MQND_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	TRACE_ENTER();

	if (pthread_create(&thread, &attr, _mqnd_imm_declare_implementer, cb) != 0) {
		LOG_CR("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

/**
 * Initialize the OI interface and get a selection object. 
 * @param cb
 * 
 * @return SaAisErrorT
 */
static void  *mqnd_imm_reinit_thread(void * _cb)
{
	SaAisErrorT error = SA_AIS_OK;
	MQND_CB *cb = (MQND_CB *)_cb;
	TRACE_ENTER();
	/* Reinitiate IMM */
	error = mqnd_imm_initialize(cb);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE)
			_mqnd_imm_declare_implementer(cb);
	}
	else
	{
		LOG_ER("mqnd_imm_initialize FAILED: %s", strerror(error));
		exit(EXIT_FAILURE);
	}
	TRACE_LEAVE();
	return NULL;
}


/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void mqnd_imm_reinit_bg(MQND_CB * cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();

	if (pthread_create(&thread, &attr, mqnd_imm_reinit_thread, cb) != 0) {
		LOG_CR("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

