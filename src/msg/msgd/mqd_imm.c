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
#include "mqd_imm.h"

extern struct ImmutilWrapperProfile immutilWrapperProfile;
#define QUEUE_MEMS 100

SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = NULL,
	.saImmOiCcbApplyCallback = NULL,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = NULL,
	.saImmOiRtAttrUpdateCallback = NULL
};

/* IMMSv Defs */
#define MQD_IMM_RELEASE_CODE 'A'
#define MQD_IMM_MAJOR_VERSION 0x02
#define MQD_IMM_MINOR_VERSION 0x01

static SaVersionT imm_version = {
	MQD_IMM_RELEASE_CODE,
	MQD_IMM_MAJOR_VERSION,
	MQD_IMM_MINOR_VERSION
};

/****************************************************************************
 * Name          : mqd_create_runtime_MqGrpObj 
 *
 * Description   : This function is invoked to create a runtime MqGrpObj object  
 *
 * Arguments     : MQD_OBJ_NODE *pNode        - MQD Object Node  
 *                 immOiHandle                - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT mqd_create_runtime_MqGrpObj(MQD_OBJ_NODE *pNode, SaImmOiHandleT immOiHandle)
{
	SaNameT names, parent, *parentName = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	char *dndup = strdup((char *)pNode->oinfo.name.value);
	char *parent_name = strchr((char *)pNode->oinfo.name.value, ',');
	char *rdnstr;
	SaImmAttrValueT arr1[1], arr2[1], arr3[1], arr4[1];
	SaImmAttrValuesT_2 attr_mqGrp, attr_mqGrpPol, attr_mqGrpNumQs, attr_mqGrpMemName;
	const SaImmAttrValuesT_2 *attrValues[5];
	SaUint32T numMem = pNode->oinfo.ilist.count;
	TRACE_ENTER2("Queue name %p", pNode->oinfo.name.value);

	if (parent_name != NULL && dndup != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;
		parentName = &parent;
		strncpy((char *)parent.value, parent_name, SA_MAX_NAME_LENGTH-1);
		parent.length = strlen((char *)parent.value);
	} else {
		rdnstr = (char *)pNode->oinfo.name.value;
	}

	if (rdnstr)
		arr1[0] = &rdnstr;
	else {
		LOG_ER("ERR_FAILED_OPERATION: Queue name is NULL"); 
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	arr2[0] = &(pNode->oinfo.info.qgrp.policy);
	arr3[0] = &numMem;

	memset(&names, 0, sizeof(SaNameT));
	arr4[0] = &names;

	attr_mqGrp.attrName = "safMqg";
	attr_mqGrp.attrValueType = SA_IMM_ATTR_SASTRINGT;
	attr_mqGrp.attrValuesNumber = 1;
	attr_mqGrp.attrValues = arr1;

	attr_mqGrpPol.attrName = "saMsgQueueGroupPolicy";
	attr_mqGrpPol.attrValueType = SA_IMM_ATTR_SAUINT32T;
	attr_mqGrpPol.attrValuesNumber = 1;
	attr_mqGrpPol.attrValues = arr2;

	attr_mqGrpNumQs.attrName = "saMsgQueueGroupNumQueues";
	attr_mqGrpNumQs.attrValueType = SA_IMM_ATTR_SAUINT32T;
	attr_mqGrpNumQs.attrValuesNumber = 1;
	attr_mqGrpNumQs.attrValues = arr3;

	attr_mqGrpMemName.attrName = "saMsgQueueGroupMemberName";
	attr_mqGrpMemName.attrValueType = SA_IMM_ATTR_SANAMET;
	attr_mqGrpMemName.attrValuesNumber = 1;
	attr_mqGrpMemName.attrValues = arr4;

	attrValues[0] = &attr_mqGrp;
	attrValues[1] = &attr_mqGrpPol;
	attrValues[2] = &attr_mqGrpNumQs;
	attrValues[3] = &attr_mqGrpMemName;
	attrValues[4] = NULL;

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaMsgQueueGroup", parentName, attrValues);
	if (rc != SA_AIS_OK)
		LOG_ER("immutil_saImmOiRtObjectCreate_2 Failed: %u", rc);

	if (dndup)
		free(dndup);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : mqd_runtime_update_grpmembers_attr 
 *              
 * Description   : This function is called to Update the runtime object   
 *              
 * Arguments     : MQD_CB *pMqd               - MQD Control Block pointer 
 *		   MQD_OBJ_NODE *pObjNode     - MQD Object Node  
 *
 * Return Values : SaAisErrorT 
 *      
 * Notes         : None.
 *****************************************************************************/
void mqd_runtime_update_grpmembers_attr(MQD_CB *pMqd, MQD_OBJ_NODE *pObjNode)
{
	SaAisErrorT error = SA_AIS_OK;
	SaNameT name[QUEUE_MEMS];
	SaImmAttrValueT attr1[QUEUE_MEMS];
	SaImmAttrModificationT_2 attr_output[1];
	const SaImmAttrModificationT_2 *attrMods[2];
	uint32_t count = pObjNode->oinfo.ilist.count;
	int i = 0, attrCnt = 0;
	NCS_QELEM *Queue = pObjNode->oinfo.ilist.head;

	memset(&name, 0, sizeof(name));

	while (Queue != NCS_QELEM_NULL) {
		MQD_OBJECT_ELEM *ptr = (MQD_OBJECT_ELEM *)Queue;
		memcpy(&name[i].value, ptr->pObject->name.value, ptr->pObject->name.length);
		name[i].length = ptr->pObject->name.length;
		attr1[i] = &name[i];
		Queue = Queue->next;
		i++;
	}

	attr_output[attrCnt].modType = SA_IMM_ATTR_VALUES_REPLACE;
	attr_output[attrCnt].modAttr.attrName = "saMsgQueueGroupMemberName";
	attr_output[attrCnt].modAttr.attrValueType = SA_IMM_ATTR_SANAMET;
	attr_output[attrCnt].modAttr.attrValuesNumber = count;
	attr_output[attrCnt].modAttr.attrValues = attr1;
	attrMods[attrCnt] = &attr_output[attrCnt];
	++attrCnt;
	attrMods[attrCnt] = NULL;
	error = saImmOiRtObjectUpdate_2(pMqd->immOiHandle, &pObjNode->oinfo.name, attrMods);

	if (error != SA_AIS_OK)
		LOG_ER("saImmOiRtObjectUpdate_2 Failed: %u", error);
}

/****************************************************************************
 * Name          : mqd_imm_initialize 
 *
 * Description   : Initialize the OI and get selection object  
 *
 * Arguments     : MQD_CB *cb - MQD Control block pointer           
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/

SaAisErrorT mqd_imm_initialize(MQD_CB *cb)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;
	rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK) {
		immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
	}
	if(rc != SA_AIS_OK)
		LOG_ER("saImmOiInitialize failed with error %d", rc);
	return rc;
}
