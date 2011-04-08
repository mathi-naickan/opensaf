
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

#include <inttypes.h>

#include "dts.h"
#include "dts_imm.h"
#include "dts_pvt.h"

static SaVersionT imm_version = {
	DTS_IMM_RELEASE_CODE,
	DTS_IMM_MAJOR_VERSION,
	DTS_IMM_MINOR_VERSION
};

static SaAisErrorT dts_saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						      const SaImmClassNameT className, const SaNameT *parentName,
						      const SaImmAttrValuesT_2 **attr);
static SaAisErrorT dts_saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
						      SaImmOiCcbIdT ccbId, const SaNameT *objectName);
static SaAisErrorT dts_saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						      const SaNameT *objectName,
						      const SaImmAttrModificationT_2 **attrMods);
static void dts_saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static SaAisErrorT dts_saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static void dts_saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
static SaAisErrorT dts_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className);

SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = dts_saImmOiCcbAbortCallback,
	.saImmOiCcbApplyCallback = dts_saImmOiCcbApplyCallback,
	.saImmOiCcbCompletedCallback = dts_saImmOiCcbCompletedCallback,
	.saImmOiCcbObjectCreateCallback = dts_saImmOiCcbObjectCreateCallback,
	.saImmOiCcbObjectDeleteCallback = dts_saImmOiCcbObjectDeleteCallback,
	.saImmOiCcbObjectModifyCallback = dts_saImmOiCcbObjectModifyCallback,
	.saImmOiRtAttrUpdateCallback = NULL
};

/**************************************************************************\
 Function: dts_imm_initialize

 Purpose:  Function which registers DTS with IMM.  

 Input:    None 

 Returns:  OK/ERROR

 Notes:    If it fails to initialize it will exit the process
\**************************************************************************/
SaAisErrorT dts_imm_initialize(DTS_CB *cb)
{
	SaAisErrorT rc;
	unsigned int nTries = 1;

	rc = saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		rc = saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
		nTries++;
	}

	if (rc == SA_AIS_OK) {
		nTries = 1;
		rc = saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
		while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
			usleep(400 * 1000);
			rc = saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
			nTries++;
		}
	}

	if (rc == SA_AIS_OK)
		TRACE("imm initialization success\n");

	return rc;
}

/**************************************************************************\
 Function: dts_imm_declare_implementer

 Purpose:  Function which declares this process as OI implementer and owns
		all the Global, Node and Service log policy objects
        
 Returns:  void *

 Notes:    If it fails it will exit the process
\**************************************************************************/
void *_dts_imm_declare_implementer(void *cb)
{
	SaAisErrorT error = SA_AIS_OK;
	DTS_CB *dts_cb = (DTS_CB *)cb;

	error = saImmOiImplementerSet(dts_cb->immOiHandle, IMPLEMENTER_NAME);
	unsigned int nTries = 1;
	while (error == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		error = saImmOiImplementerSet(dts_cb->immOiHandle, IMPLEMENTER_NAME);
		nTries++;
	}
	if (error != SA_AIS_OK) {
		LOG_ER("ImplementerSet Failed: %u exiting", error);
		exit(1);
	}
	TRACE("imm implementer set success\n");

	dts_saImmOiClassImplementerSet(dts_cb->immOiHandle, DTS_GLOBAL_CLASS_NAME);
	dts_saImmOiClassImplementerSet(dts_cb->immOiHandle, DTS_NODE_CLASS_NAME);
	dts_saImmOiClassImplementerSet(dts_cb->immOiHandle, DTS_SERVICE_CLASS_NAME);

	return NULL;
}

/**************************************************************************\
 Function: dts_saImmOiClassImplementerRelease

 Purpose:  creates a new thread for _dts_imm_declare_implementer
        
 Returns:  void

 Notes:    If it fails it will exit the process
\**************************************************************************/
void dts_imm_declare_implementer(DTS_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	TRACE_ENTER();
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, NULL, _dts_imm_declare_implementer, cb) != 0) {
		LOG_ER("pthread create failed :%s, exiting",strerror(errno) );
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
}

/**************************************************************************\
 Function: dts_saImmOiClassImplementerRelease

 Purpose:  Function which releases ownership of all the objects of Global, 
		Node and Service log policy objects
        
 Returns:  OK/ERROR

 Notes:    If it fails it will exit the process
\**************************************************************************/
SaAisErrorT dts_saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	unsigned int nTries = 1;
	SaAisErrorT rc = saImmOiClassImplementerRelease(immOiHandle, className);
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		rc = saImmOiClassImplementerRelease(immOiHandle, className);
		nTries++;
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiClassImplementerRelease of ClassName = %s is Failed, rc = %u, exiting",
			className, rc);
		exit(EXIT_FAILURE);
	}

	dts_log(NCSFL_SEV_DEBUG, "dts_saImmOiClassImplementerRelease Success\n");
	return rc;
}

/**************************************************************************\
 Function: dts_saImmOiClassImplementerSet

 Purpose:  Function which declares this process as OI implementer
        
 Returns:  OK/ERROR

 Notes:    If it fails it will exit the process
\**************************************************************************/
SaAisErrorT dts_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	unsigned int nTries = 1;
	SaAisErrorT rc = saImmOiClassImplementerSet(immOiHandle, className);
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		rc = saImmOiClassImplementerSet(immOiHandle, className);
		nTries++;
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiClassImplementerSet of ClassName = %s is Failed, rc = %u, exiting", className,
			rc);
		exit(EXIT_FAILURE);
	}

	dts_log(NCSFL_SEV_DEBUG, "dts_dts_saImmOiClassImplementerSet Success\n");
	return rc;
}

/**************************************************************************\
 Function: dts_saImmOiImplementerClear

 Purpose:  Function which giveup this process as OI implementer. and releases
		all Global, Node and Service log policy objects.
        
 Returns:  OK/ERROR 

 Notes:    If it fails it will exit the process
\**************************************************************************/
SaAisErrorT dts_saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	unsigned int nTries = 1;
	SaAisErrorT rc;

	dts_saImmOiClassImplementerRelease(immOiHandle, DTS_GLOBAL_CLASS_NAME);
	dts_saImmOiClassImplementerRelease(immOiHandle, DTS_NODE_CLASS_NAME);
	dts_saImmOiClassImplementerRelease(immOiHandle, DTS_SERVICE_CLASS_NAME);

	rc = saImmOiImplementerClear(immOiHandle);
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		rc = saImmOiImplementerClear(immOiHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerClear FAILED, rc = %u, exiting", rc);
		exit(EXIT_FAILURE);
	}

	dts_log(NCSFL_SEV_DEBUG, "dts_saImmOiImplementerClear Success\n");

	return rc;
}

/*****************************************************************************\
 Function: dts_saImmOiCcbObjectCreateCallback

 Purpose: This function handles object create callback for all config classes.
          Its purpose is to memorize the request until the completed callback.

 Returns: OK/ERROR

 NOTES: None.

\**************************************************************************/
static SaAisErrorT dts_saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						      const SaImmClassNameT className, const SaNameT *parentName,
						      const SaImmAttrValuesT_2 **attr)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	const SaImmAttrValuesT_2 *attrValue;
	CcbUtilOperationData_t *operation;
	SVC_KEY key;
	int i = 0;

	/* There shall be only one instance of the Global Log policy object */
	if (!strcmp(className, DTS_GLOBAL_CLASS_NAME)) {
		dts_log(NCSFL_SEV_ERROR, "Not allowed to create Global Policy config ojbect");
		return SA_AIS_ERR_BAD_OPERATION;

	} else if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
		dts_log(NCSFL_SEV_ERROR, "Failed to get CCB object");
		return SA_AIS_ERR_NO_MEMORY;
	} else {

		operation = ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attr);

		if (operation == NULL) {
			dts_log(NCSFL_SEV_ERROR, "Failed to get CCB operation object for %llu", ccbId);
			return SA_AIS_ERR_NO_MEMORY;
		}

		/* Find the RDN attribute and store the object DN */
		while ((attrValue = attr[i++]) != NULL) {
			if (!strncmp(attrValue->attrName, "opensaf", 7)) {
				if (attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
					SaStringT rdnVal = *((SaStringT *)attrValue->attrValues[0]);
					if ((parentName != NULL) && (parentName->length > 0)) {
						operation->objectName.length =
						    sprintf((char *)operation->objectName.value, "%s,%s\n", rdnVal,
							    parentName->value);
					} else {
						operation->objectName.length =
						    sprintf((char *)operation->objectName.value, "%s\n", rdnVal);
					}
				} else {
					SaNameT *rdnVal = ((SaNameT *)attrValue->attrValues[0]);
					operation->objectName.length = sprintf((char *)operation->objectName.value,
									       "%s,%s\n", rdnVal->value,
									       parentName->value);
				}
			}
		}

		if (operation->objectName.length == 0) {
			dts_log(NCSFL_SEV_ERROR, "Malformed DN %llu\n", ccbId);
			return SA_AIS_ERR_INVALID_PARAM;
		}
		/* memorize the create request */
		if (!strncmp((char *)operation->objectName.value, "opensafNodeLogPolicy=", 21)) {
			if (dts_parse_node_policy_DN((char *)operation->objectName.value, &key) != SA_AIS_OK)
				return SA_AIS_ERR_BAD_OPERATION;
		} else if (!strncmp((char *)operation->objectName.value, "opensafServiceLogPolicy=", 24)) {
			if (dts_parse_service_policy_DN((char *)operation->objectName.value, &key) != SA_AIS_OK)
				return SA_AIS_ERR_BAD_OPERATION;
		} else {
			dts_log(NCSFL_SEV_ERROR, "DTSv: DN is not according to the specifications\n");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attr);
	}
	return SA_AIS_OK;
}

/*****************************************************************************\
 Function: dts_saImmOiCcbObjectDeleteCallback

 Purpose: This function handles object delete callback for all config classes.
          Its purpose is to memorize the request until the completed callback.

 Returns: OK/ERROR

 NOTES: None.

\**************************************************************************/
static SaAisErrorT dts_saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
						      SaImmOiCcbIdT ccbId, const SaNameT *objectName)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	if (!strncmp((char *)objectName->value, "opensafGlobalLogPolicy=", 23)) {
		dts_log(NCSFL_SEV_ERROR, "Not allowed to create Global Policy config ojbect");
		return SA_AIS_ERR_BAD_OPERATION;
	}
	if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
		dts_log(NCSFL_SEV_ERROR, "Failed to find CCB object being delete\n");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	/* memorize the delete request */
	ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);
	return SA_AIS_OK;
}

/*****************************************************************************\
 Function: dts_saImmOiCcbAbortCallback

 Purpose: This function handles abort callback for the corresponding 
          CCB operation.

 Returns: OK/ERROR

 NOTES: None.

\**************************************************************************/
static void dts_saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		dts_log(NCSFL_SEV_ERROR, "Failed to find CCB object");

}

/*****************************************************************************\
 Function: dts_saImmOiCcbAbortCallback

 Purpose: This function handles object modify callback for all config classes.
          Its purpose is to memorize the request until the completed callback.

 Returns: OK/ERROR

 NOTES: None.

\**************************************************************************/
static SaAisErrorT dts_saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						      const SaNameT *objectName,
						      const SaImmAttrModificationT_2 **attrMods)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
		dts_log(NCSFL_SEV_ERROR, "Failed to get CCB object\n");
		return SA_AIS_ERR_NO_MEMORY;
	}

	/* memorize the modification request */
	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);
	return SA_AIS_OK;
}

/******************************************************************************\
 Function: dts_saImmOiCcbCompletedCallback
 
 Purpose: This function handles completed callback for the corresponding 
          CCB operation.
 
 Returns: OK/ERROR
 
 NOTES: None.
 
\********************************************************************************/
static SaAisErrorT dts_saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	const SaImmAttrModificationT_2 *attrMod;
	const SaImmAttrValuesT_2 *attribute = NULL;

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		dts_log(NCSFL_SEV_ERROR, "Failed to find CCB object");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	/*
	 ** "check that the sequence of change requests contained in the CCB is 
	 ** valid and that no errors will be generated when these changes
	 ** are applied."
	 */
	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData) {
		int i = 0;
		if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafServiceLogPolicy=", 24)) {
			if (ccbUtilOperationData->operationType == CCBUTIL_DELETE)
				return SA_AIS_OK;
			if (ccbUtilOperationData->operationType == CCBUTIL_CREATE
			    || ccbUtilOperationData->operationType == CCBUTIL_MODIFY) {
				if (ccbUtilOperationData->operationType == CCBUTIL_MODIFY) {
					attrMod = ccbUtilOperationData->param.modify.attrMods[i];
					if (attrMod)
						attribute = &attrMod->modAttr;
					else
						attribute = NULL;
				} else
					attribute = ccbUtilOperationData->param.create.attrValues[i];

				while (attribute) {
					uintptr_t value;

					if (attribute->attrValuesNumber == 0) {
						return SA_AIS_ERR_BAD_OPERATION;
					}

					value = *(uintptr_t *)attribute->attrValues[0];
					if (!strcmp(attribute->attrName, "osafDtsvServiceLogDevice")) {
						uns8 LogDevice = *((uns8 *)&value);
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLogDevice  %u\n", LogDevice);
						if (LogDevice > DTS_LOG_DEV_VAL_MAX)
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceLogFileSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLogFileSize %u\n", value);
						if ((value < 100) || (value > 10000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceFileLogCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceFileLogCompFormat %u\n",
							value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceCircularBuffSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceCircularBuffSize %u\n", value);
						if ((value < 10) || (value > 1000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceCirBuffCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceCirBuffCompFormat %u\n",
							value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceLoggingState")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLoggingState %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceCategoryBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceCategoryBitMap %u\n", value);
					} else if (!strcmp(attribute->attrName, "osafDtsvServiceSeverityBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceSeverityBitMap %u\n",
							*(uns8 *)&value);
					} else if (!strcmp(attribute->attrName, "opensafServiceLogPolicy")) {
						dts_log(NCSFL_SEV_DEBUG, "RDN = %s", (char *)value);
					} else {
						dts_log(NCSFL_SEV_ERROR, "invalid attribute = %s\n",
							attribute->attrName);
						return SA_AIS_ERR_BAD_OPERATION;
					}

					if (ccbUtilOperationData->operationType == CCBUTIL_MODIFY) {
						attrMod = ccbUtilOperationData->param.modify.attrMods[++i];
						if (attrMod)
							attribute = &attrMod->modAttr;
						else
							attribute = NULL;
					} else
						attribute = ccbUtilOperationData->param.create.attrValues[++i];

				}	/* end of while(attribute) */
			}	/* end of if */
		}
		/* end of if opensafServicePolily */
		if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafGlobalLogPolicy=", 23)) {
			switch (ccbUtilOperationData->operationType) {
			case CCBUTIL_CREATE:
				return SA_AIS_ERR_BAD_OPERATION;
			case CCBUTIL_DELETE:
				return SA_AIS_ERR_BAD_OPERATION;
			case CCBUTIL_MODIFY:
				attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
				while (attrMod) {
					SaUint32T value;
					const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

					if (attribute->attrValuesNumber == 0) {
						return SA_AIS_ERR_BAD_OPERATION;
					}

					value = *(SaUint32T *)attribute->attrValues[0];
					if (!strcmp(attribute->attrName, "osafDtsvGlobalMessageLogging")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalMessageLogging = %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogDevice")) {
						SaUint32T LogDevice = *((uns8 *)&value);
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogDevice %u\n", LogDevice);
						if (LogDevice > DTS_LOG_DEV_VAL_MAX)
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogFileSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogFileSize %u\n", value);
						if ((value < 100) || (value > 1000000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalFileLogCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalFileLogCompFormat %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCircularBuffSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCircularBuffSize %u\n", value);
						if ((value < 10) || (value > 100000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCirBuffCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCirBuffCompFormat %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLoggingState")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLoggingState %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCategoryBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCategoryBitMap %u\n", value);
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalSeverityBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalSeverityBitMap %u\n",
							*(uns8 *)&value);
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalNumOfLogFiles")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalNumOfLogFiles %u\n", value);
						if ((value < 1) || (value > 255))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogMsgSequencing")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogMsgSequencing %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCloseOpenFiles")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCloseOpenFiles %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else {
						dts_log(NCSFL_SEV_ERROR, "invalid attribute = %s\n",
							attribute->attrName);
						return SA_AIS_ERR_BAD_OPERATION;
					}

					attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
				}
			}
		}

		if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafNodeLogPolicy=", 21)) {
			if (ccbUtilOperationData->operationType == CCBUTIL_DELETE)
				return SA_AIS_OK;
			if ((ccbUtilOperationData->operationType == CCBUTIL_CREATE)
			    || (ccbUtilOperationData->operationType == CCBUTIL_MODIFY)) {
				if (ccbUtilOperationData->operationType == CCBUTIL_MODIFY) {
					attrMod = ccbUtilOperationData->param.modify.attrMods[i];
					if (attrMod)
						attribute = &attrMod->modAttr;
					else
						attribute = NULL;
				} else
					attribute = ccbUtilOperationData->param.create.attrValues[i];

				while (attribute) {
					uintptr_t value;

					if (attribute->attrValuesNumber == 0) {
						return SA_AIS_ERR_BAD_OPERATION;
					}

					value = *(uintptr_t *)attribute->attrValues[0];

					if (!strcmp(attribute->attrName, "osafDtsvNodeMessageLogging")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeMessageLogging = %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeLogDevice")) {
						SaUint32T LogDevice = *((uns8 *)&value);
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLogDevice %u\n", LogDevice);
						if (LogDevice > DTS_LOG_DEV_VAL_MAX)
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeLogFileSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLogFileSize %u\n", value);
						if ((value < 100) || (value > 100000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeFileLogCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeFileLogCompFormat %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeCircularBuffSize")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeCircularBuffSize %u\n", value);
						if ((value < 10) || (value > 10000))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeCirBuffCompFormat")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeCirBuffCompFormat %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeLoggingState")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLoggingState %u\n", value);
						if ((value < 0) || (value > 1))
							return SA_AIS_ERR_BAD_OPERATION;
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeCategoryBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeCategoryBitMap %u\n", value);
					} else if (!strcmp(attribute->attrName, "osafDtsvNodeSeverityBitMap")) {
						dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeSeverityBitMap %u\n",
							*(uns8 *)&value);
					} else if (!strcmp(attribute->attrName, "opensafNodeLogPolicy")) {
						dts_log(NCSFL_SEV_DEBUG, "RDN = %s", (char *)value);
					} else {
						dts_log(NCSFL_SEV_ERROR, "invalid attribute = %s\n",
							attribute->attrName);
						return SA_AIS_ERR_BAD_OPERATION;
					}

					if (ccbUtilOperationData->operationType == CCBUTIL_MODIFY) {
						attrMod = ccbUtilOperationData->param.modify.attrMods[++i];
						if (attrMod)
							attribute = &attrMod->modAttr;
						else
							attribute = NULL;
					} else
						attribute = ccbUtilOperationData->param.create.attrValues[++i];
				}	/* end of while (attribute) */
			}
		}		/* end of strcmp(objName, Nodepolicy) */
		ccbUtilOperationData = ccbUtilOperationData->next;
	}			/* end of while (ccbUtilOperationData) */
	return SA_AIS_OK;
}

/******************************************************************************\
 Function: dts_saImmOiCcbApplyCallback
 
 Purpose: This function handles apply callback for the corresponding 
          CCB operation.
 
 Returns: void
 
 NOTES: None.
 
\********************************************************************************/
static void dts_saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	DTS_CB *inst = &dts_cb;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		dts_log(NCSFL_SEV_ERROR, "Failed to find CCB object for %llu", ccbId);
	}
	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData) {
		if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafGlobalLogPolicy=", 23)) {
			switch (ccbUtilOperationData->operationType) {
			case CCBUTIL_CREATE:
				/* Global object creation is not alloowed */
				break;
			case CCBUTIL_DELETE:
				/* Global object deletion is not alloowed */
				break;
			case CCBUTIL_MODIFY:
				dts_global_log_policy_set(inst, ccbUtilOperationData);
			}	/* end of switch (ccbUtilOperationData->operationType) */
		} else if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafNodeLogPolicy=", 21)) {
			switch (ccbUtilOperationData->operationType) {
			case CCBUTIL_CREATE:
				dts_node_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value,
							ccbUtilOperationData->param.create.attrValues, CCBUTIL_CREATE);
				break;
			case CCBUTIL_DELETE:
				dts_node_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value, NULL,
							CCBUTIL_DELETE);
				break;
			case CCBUTIL_MODIFY:
				dts_node_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value,
							ccbUtilOperationData->param.modify.attrMods, CCBUTIL_MODIFY);
			}	/* end of switch */
		} else if (!strncmp((char *)ccbUtilOperationData->objectName.value, "opensafServiceLogPolicy=", 24)) {
			switch (ccbUtilOperationData->operationType) {
			case CCBUTIL_CREATE:
				dts_service_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value,
							   ccbUtilOperationData->param.create.attrValues,
							   CCBUTIL_CREATE);
				break;
			case CCBUTIL_DELETE:
				dts_service_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value, NULL,
							   CCBUTIL_DELETE);
				break;
			case CCBUTIL_MODIFY:
				dts_service_log_policy_set(inst, (char *)ccbUtilOperationData->objectName.value,
							   ccbUtilOperationData->param.modify.attrMods, CCBUTIL_MODIFY);
			}	/* end of switch (ccbUtilOperationData->operationType) */
		}
		ccbUtilOperationData = ccbUtilOperationData->next;
	}			/* end of while (ccbUtilOperationData) */
}

/******************************************************************************\
 Function: dts_configure_global_policy
 
 Purpose: This function reads global policy scalar object fromo IMMSv database 
          and updates the logging policy at global level.
 
 Returns: OK/ERROR
 
 NOTES: None.
 
\********************************************************************************/
unsigned int dts_configure_global_policy()
{
	SaAisErrorT rc = NCSCC_RC_SUCCESS;
	SaImmHandleT omHandle;
	SaNameT objectName;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	int i = 0;
	DTS_CB *inst = &dts_cb;
	strncpy((char *)objectName.value, DTS_GLOBAL_LOG_POLICY, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *)objectName.value);

	(void)immutil_saImmOmInitialize(&omHandle, NULL, &imm_version);
	(void)immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Get all attributes of the object */
	if (immutil_saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes) != SA_AIS_OK) {
		dts_log(NCSFL_SEV_ERROR, "Configuration object %s not found\n", objectName.value);
		return NCSCC_RC_FAILURE;
	}

	while ((attribute = attributes[i++]) != NULL) {
		SaUint32T value;

		if (attribute->attrValuesNumber == 0)
			continue;

		value = *(SaUint32T *)attribute->attrValues[0];
		if (!strcmp(attribute->attrName, "osafDtsvGlobalMessageLogging")) {
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			if (GLOBAL_LOGGING != value) {
				inst->g_policy.global_logging = value;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalMessageLogging_ID)) {
					dts_log(NCSFL_SEV_ERROR, "Failed to change global logging\n");
					return NCSCC_RC_FAILURE;
				}
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogDevice")) {
			SaUint32T LogDevice = *((uns8 *)&value);
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogDevice %u\n", LogDevice);
			if (LogDevice <= DTS_LOG_DEV_VAL_MAX)
				inst->g_policy.g_policy.log_dev = LogDevice;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogFileSize")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogFileSize %u\n", value);
			if ((value < 100) || (value > 1000000)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_policy.log_file_size = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalFileLogCompFormat")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalFileLogCompFormat %u\n", value);
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_policy.file_log_fmt = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCircularBuffSize")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCircularBuffSize %u\n", value);
			if ((value < 10) || (value > 100000)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_policy.cir_buff_size = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCirBuffCompFormat")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCirBuffCompFormat %u\n", value);
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_policy.buff_log_fmt = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLoggingState")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLoggingState %u\n", value);
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			if (GLOBAL_ENABLE != value) {
				inst->g_policy.g_policy.enable = value;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalLoggingState_ID)) {
					dts_log(NCSFL_SEV_ERROR, "Failed to change global logging state\n");
					return NCSCC_RC_FAILURE;
				}
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCategoryBitMap")) {
			if (GLOBAL_CATEGORY_FILTER != value) {
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCategoryBitMap %d\n", value);
				inst->g_policy.g_policy.category_bit_map = value;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalCategoryBitMap_ID)) {
					dts_log(NCSFL_SEV_ERROR, "Failed to change global category level\n");
					return NCSCC_RC_FAILURE;
				}
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalSeverityBitMap")) {
			if (GLOBAL_SEVERITY_FILTER_DEFAULT != *(uns8 *)&value) {
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalSeverityBitMap %d\n", *(uns8 *)&value);
				inst->g_policy.g_policy.severity_bit_map = *(uns8 *)&value;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalSeverityBitMap_ID)) {
					dts_log(NCSFL_SEV_ERROR, "Failed to change global severity level\n");
					return NCSCC_RC_FAILURE;
				}
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalNumOfLogFiles ")) {
			if ((value < 1) || (value > 255)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalNumOfLogFiles %u\n", value);
			inst->g_policy.g_num_log_files = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogMsgSequencing")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogMsgSequencing %d\n", value);
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_enable_seq = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCloseOpenFiles")) {
			if ((value < 0) || (value > 1)) {
				rc = NCSCC_RC_FAILURE;
				break;
			}
			inst->g_policy.g_close_files = value;
		}

	}
	if (inst->g_policy.g_policy.log_dev == CIRCULAR_BUFFER) {
		if (dts_circular_buffer_alloc(&inst->g_policy.device.cir_buffer, inst->g_policy.g_policy.cir_buff_size)
		    == NCSCC_RC_SUCCESS)
			m_LOG_DTS_CBOP(DTS_CBOP_ALLOCATED, 0, 0);
	} else
		dts_cir_buff_set_default(&inst->g_policy.device.cir_buffer);

	(void)immutil_saImmOmAccessorFinalize(accessorHandle);
	(void)immutil_saImmOmFinalize(omHandle);

	return rc;
}

/******************************************************************************\
 Function: dts_parse_node_policy_DN
 
 Purpose: This function parses object name(complete DN form) and gets key info 
 
 Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
 NOTES: None.
 
\********************************************************************************/
unsigned int dts_parse_node_policy_DN(char *objName, SVC_KEY *key)
{
	char *tmp, *num, node[10] = { 0 };
	int i = 0;
	tmp = strstr(objName, "osafNodeID=");

	if (tmp)
		tmp = strchr(tmp, '=');
	else
		return NCSCC_RC_FAILURE;
	if (tmp)
		num = strchr(tmp, ',');
	else
		return NCSCC_RC_FAILURE;

	/* extracting the node id */
	if (num) {
		while (++tmp != num) {
			if (isdigit(*tmp)) {
				node[i++] = *tmp;
			} else
				return NCSCC_RC_FAILURE;
		}
	} else
		return NCSCC_RC_FAILURE;

	key->node = atoi(node);
	if (!(key->node))
		return NCSCC_RC_FAILURE;
	key->ss_svc_id = 0;

	dts_log(NCSFL_SEV_DEBUG, "key->node = %d\n", key->node);
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************\
 Function: dts_parse_service_policy_DN
 
 Purpose: This function parses object name(complete DN form) and gets key info 
 
 Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
 NOTES: None.
 
\********************************************************************************/
unsigned int dts_parse_service_policy_DN(char *objName, SVC_KEY *key)
{
	int i = 0;

	char *tmp, *num, node[10] = { 0 };
	tmp = strstr(objName, "osafNodeID=");
	if (tmp)
		tmp = strchr(tmp, '=');
	else
		return NCSCC_RC_FAILURE;

	if (tmp)
		num = strchr(tmp, '\\');
	else
		return NCSCC_RC_FAILURE;

	/* extracting the node id */
	if (num) {
		while (++tmp != num) {
			if (isdigit(*tmp)) {
				node[i++] = *tmp;
			} else
				return NCSCC_RC_FAILURE;
		}
	} else
		return NCSCC_RC_FAILURE;

	key->node = atoi(node);

	tmp = strstr(num, "osafSvcID=");
	if (tmp)
		tmp = strchr(tmp, '=');
	else
		return NCSCC_RC_FAILURE;
	if (tmp)
		num = strchr(tmp, ',');
	else
		return NCSCC_RC_FAILURE;
	i = 0;			/* resetting iterator */
	memset(node, 0, sizeof(node));

	/* extracting the service id */
	if (num) {
		while (++tmp != num) {
			if (isdigit(*tmp)) {
				node[i++] = *tmp;
			} else
				return NCSCC_RC_FAILURE;
		}
	} else
		return NCSCC_RC_FAILURE;

	key->ss_svc_id = atoi(node);

	if (!(key->node) || !(key->ss_svc_id))
		return NCSCC_RC_FAILURE;

	dts_log(NCSFL_SEV_DEBUG, "key->node = %d   key->ss_svc_id =  %d\n", key->node, key->ss_svc_id);
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************\
 Function: dts_read_log_policies
 
 Purpose: This function reads all the objects of particular log policy class and 
		updates the patricia tree.
 
 Returns: OK/ERROR
 
 NOTES: None.
 
\********************************************************************************/
SaAisErrorT dts_read_log_policies(char *className)
{
	DTS_CB *inst = &dts_cb;
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmHandleT omHandle;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT objName, root_obj;
	SaImmAttrValuesT_2 **attributes;

	memset(&objName, 0, sizeof(SaNameT));
	memset(&root_obj, 0, sizeof(SaNameT));
	root_obj.length = strlen(DTS_APP_OBJ);
	strncpy((char *)root_obj.value, DTS_APP_OBJ, root_obj.length);

	immutil_saImmOmInitialize(&omHandle, NULL, &imm_version);

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(omHandle, &root_obj,
						    SA_IMM_SUBLEVEL,
						    SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
						    NULL, &searchHandle)) != SA_AIS_OK) {

		dts_log(NCSFL_SEV_ERROR, "saImmOmSearchInitialize_2 failed: %u", rc);
		return rc;
	}

	/* searches for the object with provided info in serach criteria */
	while (immutil_saImmOmSearchNext_2(searchHandle, &objName, &attributes) == SA_AIS_OK) {
		int i = 0;
		const SaImmAttrValuesT_2 *attribute = attributes[0];

		while (attribute) {
			/* checks whether the object found is service log policy object */
			if (!strcmp(attribute->attrName, "opensafServiceLogPolicy")) {
				dts_service_log_policy_set(inst, (char *)objName.value, attributes, CCBUTIL_CREATE);
				/* checks whether the object found is node log policy object */
			} else if (!strcmp(attribute->attrName, "opensafNodeLogPolicy")) {
				dts_node_log_policy_set(inst, (char *)objName.value, attributes, CCBUTIL_CREATE);
			}
			attribute = attributes[++i];
		}
	}

	immutil_saImmOmSearchFinalize(searchHandle);
	return rc;
}

/**
 * Initialize the OI interface and get a selection object. 
 * @param cb
 * 
 * @return SaAisErrorT
 */
static void  *dts_imm_reinit_thread(void * _cb)
{
	SaAisErrorT error = SA_AIS_OK;
	DTS_CB *cb = (DTS_CB *)_cb;
	TRACE_ENTER();

	/* Reinitiate IMM */
	error = dts_imm_initialize(cb);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE)
			_dts_imm_declare_implementer(cb);
	}
	else
	{
		LOG_ER("dts_imm_initialize FAILED: %s", strerror(error));
		exit(EXIT_FAILURE);
	}
	TRACE_LEAVE();
	return NULL;
}


/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void dts_imm_reinit_bg(DTS_CB * cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();

	if (pthread_create(&thread, &attr, dts_imm_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
       pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}


