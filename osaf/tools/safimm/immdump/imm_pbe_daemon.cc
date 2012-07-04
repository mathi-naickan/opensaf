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
 * Author(s): Ericsson AB
 *
 */

#include "imm_dumper.hh"
#include <errno.h>
#include <signal.h>
#include <saImmOi.h>
#include "immutil.h"
#include <poll.h>
#include <stdio.h>
#include <cstdlib>
#include <nid_api.h> /* To define NCS_SEL_OBJ */

extern struct ImmutilWrapperProfile immutilWrapperProfile;

#define FD_IMM_PBE_OI 0
#define FD_IMM_PBE_OM 1
#define FD_IMM_PBE_TERM 2

static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_ms = 5 * 1000;	/* 5 secs */
/*static const SaImmClassNameT immServiceClassName = "SaImmMngt";*/

static SaImmHandleT pbeOmHandle;
static SaImmOiHandleT pbeOiHandle;
static SaSelectionObjectT immOiSelectionObject;
static SaSelectionObjectT immOmSelectionObject;
static int category_mask;

static struct pollfd fds[3];
static nfds_t nfds = 3;

static void* sDbHandle=NULL;
static ClassMap* sClassIdMap=NULL;
static unsigned int sObjCount=0;
static unsigned int sClassCount=0;
static unsigned int sEpoch=0;
static unsigned int sNoStdFlags=0x00000000;

static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
					  SaInvocationT invocation,
					  const SaNameT *objectName,
					  SaImmAdminOperationIdT opId, const SaImmAdminOperationParamsT_2 **params)
{
	const SaImmAdminOperationParamsT_2 *param = params[0];
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrValuesT_2 attrValues;
	SaStringT sastringVal;
	SaImmAttrValueT val;
	std::string opensafObj((const char *) objectName->value);

	TRACE_ENTER();

	if(opId == OPENSAF_IMM_PBE_CLASS_CREATE) {
		bool schemaChange = false;
		bool persistentExtent = false; /* ConfigC or PrtoC. Only relevant if schmaChange == true */
		if(!param || (param->paramType != SA_IMM_ATTR_SASTRINGT)) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}
		TRACE("paramName: %s paramType: %u value:%s", param->paramName, param->paramType, *((SaStringT *) param->paramBuffer));
		std::string className(*((SaStringT *) param->paramBuffer));

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start transaction for class create");
			goto done;
		}
		TRACE("Begin PBE transaction for class create OK");

		if(sNoStdFlags & OPENSAF_IMM_FLAG_SCHCH_ALLOW) {
			/* Schema change allowed, check for possible upgrade. */
			ClassInfo* theClass = (*sClassIdMap)[className];
			if(theClass) {
				unsigned int instances=0;
				LOG_IN("PBE detected schema change for class %s", className.c_str());
				schemaChange = true;
				AttrMap::iterator ami = theClass->mAttrMap.begin();
				if(ami == theClass->mAttrMap.end()) {
					LOG_ER("No attributes in class definition");
					rc = SA_AIS_ERR_FAILED_OPERATION;
					goto done;
				}
				while(ami != theClass->mAttrMap.end()) {
					SaImmAttrFlagsT attrFlags = ami->second;
					if(attrFlags & SA_IMM_ATTR_CONFIG) {
						persistentExtent = true;
						break;
					}
					if(attrFlags & SA_IMM_ATTR_PERSISTENT) {
						persistentExtent = true;
						break;
					}
					++ami;
				}

				if(persistentExtent) {
					instances = purgeInstancesOfClassToPBE(pbeOmHandle, className, sDbHandle);
					LOG_IN("PBE removed %u old instances of class %s", instances, className.c_str());
				}
				deleteClassToPBE(className, sDbHandle, theClass);
				if(sClassIdMap->erase(className) != 1) {
					LOG_ER("sClassIdMap->erase(className) != 1");
					rc = SA_AIS_ERR_FAILED_OPERATION;
					goto done;
				}
				delete theClass;
				theClass = NULL;
				LOG_IN("PBE removed old class definition for %s", className.c_str());
				/* Note: We dont remove the classname from the opensaf object,
				   since we will shortly add the new version of the class, with same name. 
				 */
			}
		}

		/* Note that schema upgrade generates a new PBE class_id for the new version of the class */
		(*sClassIdMap)[className] = classToPBE(className, pbeOmHandle, sDbHandle, ++sClassCount);

		if(schemaChange) {
			unsigned int obj_count=0;
			LOG_IN("PBE created new class definition for %s", className.c_str());
			if(persistentExtent) {
				TRACE_5("sObjCount:%u", sObjCount);
				obj_count = dumpInstancesOfClassToPBE(pbeOmHandle, sClassIdMap, className, &sObjCount, sDbHandle);
				LOG_IN("PBE dumped %u objects of new class definition for %s", obj_count, className.c_str());
				TRACE_5("sObjCount:%u", sObjCount);
			}
		} else {
			/* Add classname to opensaf object when this is not an upgrade. */
			attrValues.attrName = (char *) OPENSAF_IMM_ATTR_CLASSES;
			attrValues.attrValueType = SA_IMM_ATTR_SASTRINGT;
			attrValues.attrValuesNumber = 1;
			attrValues.attrValues = &val;
			val = &sastringVal;
			sastringVal = (SaStringT) className.c_str();
			
			objectModifyAddValuesOfAttrToPBE(sDbHandle, opensafObj,	&attrValues, 0);
		}

		rc = pbeCommitTrans(sDbHandle, 0, sEpoch);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction for class create");
			goto done;
		}
		TRACE("Commit PBE transaction for class create");
		/* We only reply with ok result. PBE failed class create handled by timeout, cleanup.
		   We encode OK result for OPENSAF_IMM_PBE_CLASS_CREATE with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_REPAIR_PENDING. This way the immnd can tell that this
		   is supposed to be an ok reply on PBE_CLASS_CREATE. This usage is internal to immsv.
		*/
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_REPAIR_PENDING);

	} else if(opId == OPENSAF_IMM_PBE_CLASS_DELETE) {
		if(!param || (param->paramType != SA_IMM_ATTR_SASTRINGT)) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}
		TRACE("paramName: %s paramType: %u value:%s", param->paramName, param->paramType, *((SaStringT *) param->paramBuffer));
		std::string className(*((SaStringT *) param->paramBuffer));

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start transaction for class delete");
			goto done;
		}
		TRACE("Begin PBE transaction for class delete OK");

		ClassInfo* theClass = (*sClassIdMap)[className];
		if(!theClass) {
			LOG_ER("Class %s missing from classIdMap", className.c_str());
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		deleteClassToPBE(className, sDbHandle, theClass);

		attrValues.attrName = (char *) OPENSAF_IMM_ATTR_CLASSES;
		attrValues.attrValueType = SA_IMM_ATTR_SASTRINGT;
		attrValues.attrValuesNumber = 1;
		attrValues.attrValues = &val;
		val = &sastringVal;
		sastringVal = (SaStringT) className.c_str();

		objectModifyDiscardMatchingValuesOfAttrToPBE(sDbHandle, opensafObj,
			&attrValues, 0);			

		rc = pbeCommitTrans(sDbHandle, 0, sEpoch);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction for class delete");
			goto done;
		}

		if(sClassIdMap->erase(className) != 1) {
			LOG_ER("sClassIdMap->erase(className) != 1");
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
		delete theClass;
		TRACE("Commit PBE transaction for class delete");
		/* We only reply with ok result. PBE failed class delete handled by timeout, cleanup.
		   We encode OK result for OPENSAF_IMM_PBE_CLASS_DELETE with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_NO_SPACE. This way the immnd can tell that this
		   is supposed to be an ok reply on PBE_CLASS_DELETE. This usage is internal to immsv.
		*/
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_NO_SPACE);

	} else if(opId == OPENSAF_IMM_PBE_UPDATE_EPOCH) {
		if(!param || (param->paramType != SA_IMM_ATTR_SAUINT32T)) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}
		TRACE("paramName: %s paramType: %u value:%u", param->paramName, param->paramType, *((SaUint32T *) param->paramBuffer));
		SaUint32T epoch = (*((SaUint32T *) param->paramBuffer));

		if(sEpoch >= epoch) {
			TRACE("Current epoch %u already equal or greater than %u ignoring", sEpoch, epoch);
			goto done;
		}
		sEpoch = epoch;

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start transaction for update epoch");
			goto done;
		}
		TRACE("Begin PBE transaction for update epoch OK");

		attrValues.attrName = (char *) OPENSAF_IMM_ATTR_EPOCH;
		attrValues.attrValueType = SA_IMM_ATTR_SAUINT32T;
		attrValues.attrValuesNumber = 1;
		attrValues.attrValues = &val;
		val = &epoch;

		/*objectModifyDiscardMatchingValuesOfAttrToPBE(sDbHandle, opensafObj,
		  &attrValues, 0);*/

		objectModifyAddValuesOfAttrToPBE(sDbHandle, opensafObj,	&attrValues, 0);

		purgeCcbCommitsFromPbe(sDbHandle, sEpoch);

		rc = pbeCommitTrans(sDbHandle, 0, sEpoch);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction for update epoch");
			goto done;
		}
		TRACE("Commit PBE transaction for update epoch");
		/* We only reply with ok result. PBE failed update epoch handled by timeout, cleanup.
		   We encode OK result for OPENSAF_IMM_PBE_UPDATE_EPOCH with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_QUEUE_NOT_AVAILABLE. This way the immnd can
		   tell that this is supposed to be an ok reply on PBE_UPDATE_EPOCH. This usage is
		   internal to immsv.
		*/
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_QUEUE_NOT_AVAILABLE);
	} else if(opId == OPENSAF_IMM_NOST_FLAG_ON) {
		SaNameT myObj;
		strcpy((char *) myObj.value, OPENSAF_IMM_OBJECT_DN);
		myObj.length = strlen((const char *) myObj.value);
		SaImmAttrValueT val = &sNoStdFlags;
		SaImmAttrModificationT_2 attMod = {SA_IMM_ATTR_VALUES_REPLACE,
						   {(char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS, SA_IMM_ATTR_SAUINT32T, 1, &val}};
		const SaImmAttrModificationT_2* attrMods[] = 
			{&attMod, NULL};
		
		if(!param || (param->paramType != SA_IMM_ATTR_SAUINT32T) || 
			strcmp(param->paramName, (char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS)) {
			LOG_IN("Incorrect parameter to NostdFlags-ON, should be 'flags:SA_UINT32_T:nnnn");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		SaUint32T flagsToSet = (*((SaUint32T *) param->paramBuffer));

		sNoStdFlags |= flagsToSet;


		rc = saImmOiRtObjectUpdate_2(immOiHandle, &myObj, attrMods);
		if(rc != SA_AIS_OK) {
			LOG_WA("Update of attr %s in %s failed, rc=%u",
				attMod.modAttr.attrName, (char *) myObj.value, rc);
		}

		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
	} else if(opId == OPENSAF_IMM_NOST_FLAG_OFF) {
		SaNameT myObj;
		strcpy((char *) myObj.value, OPENSAF_IMM_OBJECT_DN);
		myObj.length = strlen((const char *) myObj.value);
		SaImmAttrValueT val = &sNoStdFlags;
		SaImmAttrModificationT_2 attMod = {SA_IMM_ATTR_VALUES_REPLACE,
						   {(char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS, SA_IMM_ATTR_SAUINT32T, 1, &val}};
		const SaImmAttrModificationT_2* attrMods[] = 
			{&attMod, NULL};
		
		if(!param || (param->paramType != SA_IMM_ATTR_SAUINT32T) ||
			strcmp(param->paramName, (char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS)) {
			LOG_IN("Incorrect parameter to NostdFlags-OFF, should be 'flags:SA_UINT32_T:nnnn");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		SaUint32T flagsToUnSet = (*((SaUint32T *) param->paramBuffer));

		sNoStdFlags &= ~flagsToUnSet;


		rc = saImmOiRtObjectUpdate_2(immOiHandle, &myObj, attrMods);
		if(rc != SA_AIS_OK) {
			LOG_WA("Update of attr %s in %s failed, rc=%u",
				attMod.modAttr.attrName, (char *) myObj.value, rc);
		}

		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
	} else {
		LOG_WA("Invalid operation ID %llu", (SaUint64T) opId);
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
	}
 done:
	TRACE_LEAVE();
	if(rc != SA_AIS_OK) {
		pbeRepositoryClose(sDbHandle);
		LOG_ER("Exiting rc=%u (line:%u)", rc, __LINE__);
		exit(1);
	}
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
						  SaImmOiCcbIdT ccbId,
						  const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData = NULL;
	struct CcbUtilOperationData  *operation = NULL;

	TRACE_ENTER2("Modify callback for CCB:%llu object:%s", ccbId, objectName->value);
	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	if(strncmp((char *) objectName->value, (char *) OPENSAF_IMM_OBJECT_DN, objectName->length) ==0) {
		LOG_WA("Pbe will not allow modifications to object %s", (char *) OPENSAF_IMM_OBJECT_DN);
		rc = SA_AIS_ERR_BAD_OPERATION;
		/* We will actually get invoked twice on this object, both as normal implementer and as PBE
		   the response on the modify upcall from PBE is discarded, but not for the regular implemener.
		 */
	} else {
		/* "memorize the modification request" */
		if(ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods) != 0) {
			LOG_ER("ccbutil_ccbAddModifyOperation did NOT return zero");
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	if(ccbId == 0) {
		const SaImmAttrModificationT_2 *attMod = NULL;
		int ix=0;
		std::string objName;
		operation = ccbutil_getNextCcbOp(0, NULL);
		if(operation == NULL) {
			LOG_ER("ccbutil_getNextCcbOp(0, NULL) returned NULL");
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		
		TRACE("Update of PERSISTENT runtime attributes in object with DN: %s",
			operation->objectName.value);

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start transaction for PRT attr update");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
		TRACE("Begin PBE transaction for rt obj update OK");


		/* Note: it is important that the code in this update case follow
		   the same logic as performed by ImmModel::rtObjectUpdate()
		   We DO NOT want the PBE repository to diverge from the main memory
		   represenation of the immsv data. 
		   This is not the only way to solve this. In fact the current solution is
		   very unoptimal since it generates possibly several sql commands for what
		   could be one. The advantage with the current solution is that it follows
		   the logic of the ImmModel and can do so using the data provided by the
		   unmodified rtObjectUpdate upcall.
		*/
		TRACE("Update of object with DN: %s", operation->param.modify.objectName->value);
		objName.append((const char *) operation->param.modify.objectName->value);		
		attrMods = operation->param.modify.attrMods;
		while((attMod = attrMods[ix++]) != NULL) {
			switch(attMod->modType) {
				case SA_IMM_ATTR_VALUES_REPLACE:
					objectModifyDiscardAllValuesOfAttrToPBE(sDbHandle, objName,
						&(attMod->modAttr), ccbId);

					if(attMod->modAttr.attrValuesNumber == 0) {
						continue; //Ok to replace with nothing
					}
					//else intentional fall through

				case SA_IMM_ATTR_VALUES_ADD:
					if(attMod->modAttr.attrValuesNumber == 0) {
						LOG_ER("Empty value used for adding to attribute %s",
							attMod->modAttr.attrName);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto abort;
					}
					
					objectModifyAddValuesOfAttrToPBE(sDbHandle, objName,
						&(attMod->modAttr), ccbId);
							
					break;

				case SA_IMM_ATTR_VALUES_DELETE:
					if(attMod->modAttr.attrValuesNumber == 0) {
						LOG_ER("Empty value used for deleting from attribute %s",
							attMod->modAttr.attrName);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto abort;
					}

					objectModifyDiscardMatchingValuesOfAttrToPBE(sDbHandle, objName,
						&(attMod->modAttr), ccbId);
					
					break;
				default: abort();
			}
		}


		ccbutil_deleteCcbData(ccbutil_findCcbData(0));

		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction for PRT attr update");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
		TRACE("Commit PBE transaction for rt attr update OK");
	}

	goto done;
 abort:
	pbeAbortTrans(sDbHandle);
 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	/* The completed callback is actually PREPARE AND COMMIT of the transaction.
	   The PBE receives the completed callback only AFTER all other (regular) 
	   implementers have replied on completed AND ONLY IF all of them replied
	   with SA_AIS_OK.

	   In this upcall the PBE has taken over sole responsibility of deciding on
	   commit/abort for this ccb/transaction. If we successfully commit towards
	   disk, that is pbeCommitTrans returns SA_AIS_OK, then this callback returns OK 
	   and the rest of the immsv is obligated to commit. Returning Ok from this
	   upcall is then very special and precious.

	   If we unabiguously fail to commit, that is if pbeCommitTrans returns 
	   not SA_AIS_OK, or we fail before that, then this upcall will return with failure
	   and the rest of the imsmv is obliged NOT to commit. 
	   The failure case is the same as for any implementer, returning not OK means 
	   this implemener vetoes the transaction.

	   There is a third case that is non standard. 
	   If this upcall does not reply (i.e. the PBE crashes or gets stuck in an endless 
	   loop or gets blocked on disk i/o, or...) then the rest of the immsv is obliged
	   to wait until the outcome is resolved. The outcome of a "lost" ccb/transaction
	   can be resolved in two ways. 

	   The first way is for the IMMSv to restart the PBE with argument -restart.
	   The restarted PBE will then try to reconnect to the existing pbe file.
	   If successfull, a recovery protocol can be executed to determine the 
	   outcome of any ccb-id. This is the primary solution and is done by
	   getCcbOutcomeFromPbe below.
	   

	   The second way is for the IMMSv to overrule the PBE by discarding the entire
	   pbe file. This is done by just restarting the pbe in a normal way which
	   will cause it to dump the current contents of the immsv, ignoring any previous
	   disk rep. Discarding ccbs that could have been commited to disk may seem wrong,
	   but as long as no client of the imsmv has seen the result of the ccb as commited,
	   there is no problem. But care should be taken to discard the old disk rep so
	   that it is not re-used later to see an alternative history.
	   If the first solution fails then this solution will be used as fallback.
	   getCcbOutcomeFromPbe will then return BAD_OPERATION i.e. presumed abort.
	 */

	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	TRACE_ENTER2("Completed callback for CCB:%llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_WA("Failed to find CCB object for %llu - checking DB file for outcome", ccbId);
		rc = getCcbOutcomeFromPbe(sDbHandle, ccbId, sEpoch);/* rc=BAD_OPERATION or OK */
		(void) ccbutil_getCcbData(ccbId); /*generate an empty record*/
		goto done;
	}

	if((rc =  pbeBeginTrans(sDbHandle)) != SA_AIS_OK)
	{ goto done;}

	TRACE("Begin PBE transaction for CCB %llu OK", ccbId);

	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL && rc == SA_AIS_OK) {
		const SaImmAttrModificationT_2 **attrMods = NULL;
		const SaImmAttrModificationT_2 *attMod = NULL;
		int ix=0;
		std::string objName;
						
		switch (ccbUtilOperationData->operationType) {
			case CCBUTIL_CREATE:
				do {
					TRACE("Create of object with DN: %s",
						ccbUtilOperationData->objectName.value);
					objectToPBE(std::string((const char *) ccbUtilOperationData->objectName.value), 
						ccbUtilOperationData->param.create.attrValues,
						sClassIdMap, sDbHandle, ++sObjCount,
						ccbUtilOperationData->param.create.className, ccbId);
				} while (0);
				break;

			case CCBUTIL_DELETE:
				TRACE("Delete of object with DN: %s",
					ccbUtilOperationData->param.deleteOp.objectName->value);
				objectDeleteToPBE(std::string((const char *) 
					ccbUtilOperationData->param.deleteOp.objectName->value),
					sDbHandle);
					
				break;

			
			case CCBUTIL_MODIFY:
				/* Note: it is important that the code in this MODIFY case follow
				   the same logic as performed by ImmModel::ccbObjectModify()
				   We DO NOT want the PBE repository to diverge from the main memory
				   represenation of the immsv data. 
				   This is not the only way to solve this. In fact the current solution is
				   very unoptimal since it generates possibly several sql commands for what
				   could be one. The advantage with the current solution is that it follows
				   the logic of the ImmModel and can do so using the data provided by the
				   unmodified ccbObjectModify upcall.
				 */
				TRACE("Modify of object with DN: %s",
					ccbUtilOperationData->param.modify.objectName->value);

				objName.append((const char *) ccbUtilOperationData->
					param.modify.objectName->value);
				attrMods = ccbUtilOperationData->param.modify.attrMods;
				while((attMod = attrMods[ix++]) != NULL) {
					switch(attMod->modType) {
						case SA_IMM_ATTR_VALUES_REPLACE:
							objectModifyDiscardAllValuesOfAttrToPBE(sDbHandle, objName,
								&(attMod->modAttr), ccbId);

							if(attMod->modAttr.attrValuesNumber == 0) {
								continue; //Ok to replace with nothing
							}
							//else intentional fall through

						case SA_IMM_ATTR_VALUES_ADD:
							if(attMod->modAttr.attrValuesNumber == 0) {
								LOG_ER("Empty value used for adding to attribute %s",
									attMod->modAttr.attrName);
								rc = SA_AIS_ERR_BAD_OPERATION;
								goto abort;
							}
							
							objectModifyAddValuesOfAttrToPBE(sDbHandle, objName,
								&(attMod->modAttr), ccbId);
							
							break;

						case SA_IMM_ATTR_VALUES_DELETE:
							if(attMod->modAttr.attrValuesNumber == 0) {
								LOG_ER("Empty value used for deleting from attribute %s",
									attMod->modAttr.attrName);
								rc = SA_AIS_ERR_BAD_OPERATION;
								goto abort;
							}
							objectModifyDiscardMatchingValuesOfAttrToPBE(sDbHandle, objName,
								&(attMod->modAttr), ccbId);

							break;
						default: abort();
					}
				}
				break;

			default: abort();

		}
		ccbUtilOperationData = ccbUtilOperationData->next;
	}

	rc =  pbeCommitTrans(sDbHandle, ccbId, sEpoch);
	if(rc == SA_AIS_OK) {
		TRACE("COMMIT PBE TRANSACTION for ccb %llu epoch:%u OK", ccbId, sEpoch);
		/* Use ccbUtilCcbData->userData to record ccb outcome, verify in ccbApply/ccbAbort */
	} else {
		TRACE("COMMIT TRANSACTION %llu epoch%u FAILED rc:%u", ccbId, sEpoch, rc);
	}

	/* Fault injection.
	if(ccbId == 3) { exit(1);} 
	*/
	goto done;

 abort:	
	pbeAbortTrans(sDbHandle);
 done:
	TRACE_LEAVE();
	return rc;
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	//const SaImmAttrModificationT_2 *attrMod;

	TRACE_ENTER2("PBE APPLY CALLBACK cleanup CCB:%llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_WA("Failed to find CCB object for %llu - PBE restarted recently?", ccbId);
		goto done;
	}

	ccbUtilOperationData = ccbUtilCcbData->operationListHead;

	/* Verify ok outcome with ccbUtilCcbData->userData */

	ccbutil_deleteCcbData(ccbUtilCcbData);
 done:
	TRACE_LEAVE();
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("ABORT callback. Cleanup CCB %llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL) {
		/* Verify nok outcome with ccbUtilCcbData->userData */
		ccbutil_deleteCcbData(ccbUtilCcbData);
	} else {
		/* changed from ER to WA. We can get here when PBE was restarted
		   after PBE crash, in the recovery processing. */
		LOG_WA("Failed to find CCB object for %llu", ccbId);
	}

	TRACE_LEAVE();
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	bool rdnFound=false;
	const SaImmAttrValuesT_2 *attrValue;
	int i = 0;
	std::string classNameString(className);
	struct CcbUtilOperationData  *operation = NULL;
	ClassInfo* classInfo = (*sClassIdMap)[classNameString];
	if(parentName && parentName->length) {
		TRACE_ENTER2("CREATE CALLBACK CCB:%llu class:%s parent:%s", ccbId, className, parentName->value);
	} else {
		TRACE_ENTER2("CREATE CALLBACK CCB:%llu class:%s ROOT OBJECT (no parent)", ccbId, className);
	}
	if(!classInfo) {
		LOG_ER("Class '%s' not found in classIdMap", className);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	if(strncmp((char *) className, (char *) OPENSAF_IMM_CLASS_NAME, strlen(className)) == 0) {
		LOG_WA("Pbe will not allow creates of instances of class %s", (char *) OPENSAF_IMM_CLASS_NAME);
		rc = SA_AIS_ERR_BAD_OPERATION;
		/* We will actually get invoked twice on this create, both as normal implementer and as PBE
		   the response on the create upcall from PBE is discarded, but not for the regular implementer UC.
		 */
		goto done;
	} 

	/* "memorize the creation request" */

	operation = ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attr);
	if(operation == NULL) {
		LOG_ER("ccbutil_ccbAddCreateOperation returned NULL");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/* Find the RDN attribute, construct the object DN and store it with the object. */
	while((attrValue = attr[i++]) != NULL) {
		std::string attName(attrValue->attrName);
		SaImmAttrFlagsT attrFlags = classInfo->mAttrMap[attName];
		if(attrFlags & SA_IMM_ATTR_RDN) {
			if(rdnFound) {
					
				LOG_NO("PBE: More than one RDN attribute found in attribute list ccb:%llu", ccbId);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;				
			}
			rdnFound = true;
			if(attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
				SaStringT rdnVal = *((SaStringT *) attrValue->attrValues[0]);
				if((parentName==NULL) || (parentName->length == 0)) {
					operation->objectName.length =
						sprintf((char *) operation->objectName.value,
							"%s", rdnVal);
				} else {
					operation->objectName.length = 
						sprintf((char *) operation->objectName.value,
							"%s,%s", rdnVal, parentName->value);
				}
			} else if(attrValue->attrValueType == SA_IMM_ATTR_SANAMET) {
				SaNameT *rdnVal = ((SaNameT *) attrValue->attrValues[0]);
				if((parentName==NULL) || (parentName->length == 0)) {
					operation->objectName.length =
						sprintf((char *) operation->objectName.value,
							"%s", rdnVal->value);
				} else {
					operation->objectName.length = 
						sprintf((char *) operation->objectName.value,
							"%s,%s", rdnVal->value, parentName->value);
				}
			} else {
				LOG_ER("Rdn attribute %s for class '%s' is neither SaStringT nor SaNameT!", 
					attrValue->attrName, className);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			TRACE("Extracted DN: %s(%u)", operation->objectName.value,  operation->objectName.length);
		}
	}

	if(!rdnFound) {
		LOG_ER("Could not find Rdn attribute for class '%s'!", 	className);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if(operation->objectName.length <= 0) {
		LOG_ER("operation->objectName.length can not be zero or negative");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;		
	}

	if(ccbId == 0) {
		TRACE("Create of PERSISTENT runtime object with DN: %s",
			operation->objectName.value);
		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start transaction for PRTO create");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		TRACE("Begin PBE transaction for rt obj create OK");

		objectToPBE(std::string((const char *) operation->objectName.value), 
			operation->param.create.attrValues,
			sClassIdMap, sDbHandle, ++sObjCount,
			operation->param.create.className, ccbId);

		ccbutil_deleteCcbData(ccbutil_findCcbData(0));

		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch);

		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction for PRTO create");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
		TRACE("Commit PBE transaction for rt obj create OK");
	}



 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	TRACE_ENTER2("DELETE CALLBACK CCB:%llu object:%s", ccbId, objectName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	if(strncmp((char *) objectName->value, (char *) OPENSAF_IMM_OBJECT_DN, objectName->length) ==0) {
		LOG_WA("Pbe will not allow delete of object %s", (char *) OPENSAF_IMM_OBJECT_DN);
		rc = SA_AIS_ERR_BAD_OPERATION;
		/* We will actually get invoked twice on this object, both as normal implementer and as PBE
		   the response on the delete upcall from PBE is discarded, but not for the regular implemener.
		 */
	} else {
		/* "memorize the deletion request" */
		ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);
	}

 done:
	TRACE_LEAVE();
	return rc;
}
/**
 * IMM requests us to update a non cached runtime attribute.
 * @param immOiHandle
 * @param objectName
 * @param attributeNames
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
					       const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	SaAisErrorT rc = SA_AIS_OK;
	//SaImmAttrNameT attributeName;

	TRACE_ENTER2("RT ATTR UPDATE CALLBACK");

	TRACE_LEAVE();
	return rc;
}

static const SaImmOiCallbacksT_2 callbacks = {
	saImmOiAdminOperationCallback,
	saImmOiCcbAbortCallback,
	saImmOiCcbApplyCallback,
	saImmOiCcbCompletedCallback,
	saImmOiCcbObjectCreateCallback,
	saImmOiCcbObjectDeleteCallback,
	saImmOiCcbObjectModifyCallback,
	saImmOiRtAttrUpdateCallback
};

/**
 * Ssignal handler to dump information from all data structures
 * @param sig
 */
static void dump_sig_handler(int sig)
{
	/*
	int old_category_mask = category_mask;

	if (trace_category_set(CATEGORY_ALL) == -1)
		printf("trace_category_set failed");

	if (trace_category_set(old_category_mask) == -1)
		printf("trace_category_set failed");
	*/
}

/**
 * USR2 signal handler to enable/disable trace (toggle)
 * @param sig
 */
static void sigusr2_handler(int sig)
{
	if (category_mask == 0) {
		category_mask = CATEGORY_ALL;
		printf("Enabling traces");
		dump_sig_handler(sig); /* Do a dump each time we toggle on.*/
	} else {
		category_mask = 0;
		printf("Disabling traces");
	}

	if (trace_category_set(category_mask) == -1)
		printf("trace_category_set failed");
}

/**
 * TERM signal handler to close sqlite handle
 * @param sig
 */
static void sigterm_handler(int sig)
{
	ncs_sel_obj_ind(term_sel_obj);
	signal(SIGTERM, SIG_IGN);
}

SaAisErrorT pbe_daemon_imm_init(SaImmHandleT immHandle)
{
	SaAisErrorT rc;
	unsigned int msecs_waited = 0;
	SaVersionT  immVersion;
        SaImmAccessorHandleT accessorHandle;
	TRACE_ENTER();

	immVersion.releaseCode = RELEASE_CODE;
	immVersion.majorVersion = MAJOR_VERSION;
	immVersion.minorVersion = MINOR_VERSION;

	pbeOmHandle = immHandle;

	rc = saImmOiInitialize_2(&pbeOiHandle, &callbacks, &immVersion);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiInitialize_2(&pbeOiHandle, &callbacks, &immVersion);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize_2 failed %u", rc);
		return rc;
	}

	rc = saImmOiImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_PBE_IMPL_NAME);
	while ((rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_EXIST) && 
		(msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_PBE_IMPL_NAME);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet for %s failed %u", OPENSAF_IMM_PBE_IMPL_NAME, rc);
		return rc;
	}
	/* Second implementer: OPENSAF_IMM_SERVICE_IMPL_NAME */

	rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_CLASS_NAME);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;	
		rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_CLASS_NAME);
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiClassImplementerSet for %s failed %u", OPENSAF_IMM_CLASS_NAME, rc);
		return rc;
	}

	rc = saImmOiSelectionObjectGet(pbeOiHandle, &immOiSelectionObject);
	/* SelectionObjectGet is library local, no need for retry loop */
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiSelectionObjectGet failed %u", rc);
		return rc;
	}

	rc = saImmOmSelectionObjectGet(immHandle, &immOmSelectionObject);
	/* SelectionObjectGet is library local, no need for retry loop */
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmSelectionObjectGet failed %u", rc);
		return rc;
	}

	rc = saImmOmAccessorInitialize(immHandle, &accessorHandle);
	if(rc == SA_AIS_OK) {
		const SaImmAttrNameT attName = (char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS;
		SaNameT myObj;
		strcpy((char *) myObj.value, OPENSAF_IMM_OBJECT_DN);
		myObj.length = strlen((const char *) myObj.value);
		SaImmAttrNameT attNames[] = {attName, NULL};
		SaImmAttrValuesT_2 ** resultAttrs;
		rc = saImmOmAccessorGet_2(accessorHandle, &myObj, attNames, &resultAttrs);
		if(rc == SA_AIS_OK) {
			if(resultAttrs[0]==NULL || 
				(resultAttrs[0]->attrValueType != SA_IMM_ATTR_SAUINT32T) ||
				(resultAttrs[0]->attrValuesNumber != 1)) {
				rc = SA_AIS_ERR_FAILED_OPERATION;
			} else {
				sNoStdFlags = *((SaUint32T *) resultAttrs[0]->attrValues[0]);
				TRACE("NostdFlags initialized to %x", sNoStdFlags);
			}
		}
	}

	if(rc != SA_AIS_OK) {
		LOG_WA("PBE failed to fetch initial value for NoStdFlags, assuming all flags are zero");
	}
	TRACE_LEAVE();

	return rc;
}

void pbeDaemon(SaImmHandleT immHandle, void* dbHandle, ClassMap* classIdMap,
	unsigned int objCount)
{

	SaAisErrorT error = SA_AIS_OK;
	ClassMap::iterator ci;

	/* cbHandle, classIdMap and objCount are needed by the PBE OI upcalls.
	   Make them available by assigning to module local static variables. 
	 */
	sDbHandle = dbHandle;
	sClassIdMap = classIdMap;
	sObjCount = objCount;

	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.retryInterval = 400;
	immutilWrapperProfile.nTries = 5;


	/* Restore also sClassCount. */
	for(ci=sClassIdMap->begin(); ci!=sClassIdMap->end();++ci) {

		if((ci)->second->mClassId > sClassCount) {
			sClassCount = (ci)->second->mClassId;
		}
	}

	if (pbe_daemon_imm_init(immHandle) != SA_AIS_OK) {
		pbeRepositoryClose(sDbHandle);
		exit(1);
	}

	if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
		LOG_ER("signal USR2 failed: %s", strerror(errno));
		pbeRepositoryClose(sDbHandle);
		exit(1);
	}

	if (ncs_sel_obj_create(&term_sel_obj) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		pbeRepositoryClose(sDbHandle);
		exit(1);
	}

	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		LOG_ER("Failed to registe signal handler for TERM: %s", strerror(errno));
		pbeRepositoryClose(sDbHandle);
		exit(1);
	}

	/* Set up all file descriptors to listen to */
	fds[FD_IMM_PBE_OI].fd = immOiSelectionObject;
	fds[FD_IMM_PBE_OI].events = POLLIN;
	fds[FD_IMM_PBE_OM].fd = immOmSelectionObject;
	fds[FD_IMM_PBE_OM].events = POLLIN;
	fds[FD_IMM_PBE_TERM].fd = term_sel_obj.rmv_obj;
	fds[FD_IMM_PBE_TERM].events = POLLIN;

        while(1) {
		TRACE("PBE Daemon entering poll");
		int ret = poll(fds, nfds, -1);
		TRACE("PBE Daemon returned from poll ret: %d", ret);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (pbeOiHandle && fds[FD_IMM_PBE_OI].revents & POLLIN) {
			error = saImmOiDispatch(pbeOiHandle, SA_DISPATCH_ALL);

			if (error == SA_AIS_ERR_BAD_HANDLE) {
				TRACE("saImmOiDispatch returned BAD_HANDLE");
				pbeOiHandle = 0;
				break;
			}
		}
		/* Attch as OI for 
		   SaImmMngt: safRdn=immManagement,safApp=safImmService
		   ?? OpensafImm: opensafImm=opensafImm,safApp=safImmService ??
		*/

		if (immHandle && fds[FD_IMM_PBE_OM].revents & POLLIN) {
			error = saImmOmDispatch(immHandle, SA_DISPATCH_ALL);
			if (error != SA_AIS_OK) {
				LOG_WA("saImmOmDispatch returned %u PBE lost contact with IMMND - exiting", error);
				pbeOiHandle = 0;
				immHandle = 0;
				break;
			}
		}

		if (fds[FD_IMM_PBE_TERM].revents & POLLIN) {
			ncs_sel_obj_rmv_ind(term_sel_obj, true, true);
			if (sDbHandle != NULL) {
				LOG_NO("PBE received SIG_TERM, closing db handle");
				pbeRepositoryClose(sDbHandle);
				sDbHandle = NULL;
			}
			break;
		}
	}

	LOG_IN("IMM PBE process EXITING...");
	TRACE_LEAVE();
	exit(1);
}


