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

#include "immpbe.hh"
#include <errno.h>
#include <signal.h>
#include <saImmOi.h>
#include "immutil.h"
#include <poll.h>
#include <stdio.h>
#include <cstdlib>
#include <nid_api.h> /* To define NCS_SEL_OBJ */
#include <immsv_evt_model.h>


#define FD_IMM_PBE_OI 0
#define FD_IMM_PBE_OM 1
#define FD_IMM_PBE_TERM 2

#define FD_IMM_PBE_RT_OI 0

static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_ms = 5 * 1000;	/* 5 secs */
/*static const SaImmClassNameT immServiceClassName = "SaImmMngt";*/

static SaImmHandleT pbeOmHandle;
static SaImmOiHandleT pbeOiHandle;
static SaImmOiHandleT pbeOiRtHandle;
static SaImmAdminOwnerHandleT sOwnerHandle;
static SaSelectionObjectT immOiSelectionObject;
static SaSelectionObjectT immOiRtSelectionObject;
static SaSelectionObjectT immOmSelectionObject;
static int category_mask;

static struct pollfd fds[3];
static nfds_t nfds = 3;

static struct pollfd rtfds[1];
static nfds_t nrtfds = 1;

static void* sDbHandle=NULL;
static ClassMap* sClassIdMap=NULL;
static unsigned int sObjCount=0;
static unsigned int sClassCount=0;
static unsigned int sEpoch=0;
static unsigned int sNoStdFlags=0x00000000;
static unsigned int sBufsize=256;
static bool sPbe2=false;
static bool sPbe2B=false;
static SaUint64T sLastCcbCommit=0LL;
static SaTimeT sLastCcbCommitTime=0LL;
static const SaStringT ccb_id_string = (SaStringT) "ccbId";
static const SaStringT num_ops_string = (SaStringT) "numOps";

/*The following are only used by 2PBE logic. */
static volatile SaUint64T s2PbeBCcbToCompleteAtB=0LL;
static volatile SaUint32T s2PbeBCcbOpCountToExpectAtB=0;
static volatile SaUint32T s2PbeBCcbOpCountNowAtB=0;
static volatile struct CcbUtilCcbData *s2PbeBCcbUtilCcbData = NULL;

extern struct ImmutilWrapperProfile immutilWrapperProfile;

static SaAisErrorT sqlite_prepare_ccb(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	struct CcbUtilOperationData *ccbUtilOperationData)
{
	/* This function contains the "body" of the buildup of the sqlite transaction
	   content corresponding to a ccb.*/

	SaAisErrorT rc = SA_AIS_OK;
	char buf[sBufsize];

	if(!pbeTransStarted()) {
		LOG_ER("sqlite_prepare_ccb invoked when no sqlite transaction has been started.");
		abort();
	}

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
				   representation of the immsv data. 
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
								snprintf(buf, sBufsize,
									"PBE: Empty value used for adding to attribute %s",
									attMod->modAttr.attrName);
								LOG_NO("%s", buf);
								saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
								rc = SA_AIS_ERR_BAD_OPERATION;
								goto ccb_abort;
							}
							
							objectModifyAddValuesOfAttrToPBE(sDbHandle, objName,
								&(attMod->modAttr), ccbId);
							
							break;

						case SA_IMM_ATTR_VALUES_DELETE:
							if(attMod->modAttr.attrValuesNumber == 0) {
								snprintf(buf, sBufsize,
									"PBE: Empty value used for deleting from attribute %s",
									attMod->modAttr.attrName);
								LOG_NO("%s", buf);
								saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
								rc = SA_AIS_ERR_BAD_OPERATION;
								goto ccb_abort;
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
	pbeClosePrepareTrans();
 ccb_abort:
	return rc;
}

/* 2PBE: Note potential
   concurrency problem towards sqlite here, between the main thread and the runtime-object thread. Sqlite is
   supposed to be threadsafe, but we dont want to rely on that. The actual risk for interference should be small
   and the consequences of a conflict must be non catastrophic. First, the risk is zero at the primary PBE
   because it (should!) only be executing sqlite from the main thread. The runtime object thread is hardly used
   at the primary. At the slave, the opposite is (almost) true. The RTO thread will execute all sqlite access,
   except for one important case, explained below. Second, pbeBeginTrans() and pbeCommitTrans() implement a
   spinlock, see osaf/libs/common/immsv/immpbe_dump.cc. Thus interference is detected before any transactional
   operations are allowed to begin. Third, at the slave, the only sqlite operation performed by the main thread
   is the commit of an sqlite transaction for a regular CCB. The start of the sqlite transaction for a regular CCB
   and all the buildup of the transsaction is handled by the RTO thread at the slave. Note the main thread, as an
   applier will collect all the operations into the immutils collection. But actual sqlite processing for a CCB
   is triggered at the slave only when all operations for the CCB have arrived. 

   The entire prepare processing (start transaction and buildup) is done by the RTO thread at the slave.
   CCB is committed at primary, then in imm-ram. Finally the sqlite commit of a ccb at the slave is done
   by the slave only when it receives the completed & apply callbacks in the applier thread.
*/

static bool pbe2_start_prepare_ccb_A_to_B(SaImmOiCcbIdT ccbId, SaUint32T numOps)
{
	TRACE_ENTER();
	bool retval=false;
	
	/* PBE primary (A) sends the start-prepare order to slave(B). */
	/* Check if slave PBE is down, if so return true if slave PBE is inactive or disabled. */

	unsigned int msecs_waited = 0;
	SaAisErrorT rc2B = SA_AIS_OK;
	SaAisErrorT slavePbeRtReply = SA_AIS_OK;
	SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
	
	const SaImmAdminOperationParamsT_2 param0 = {
		ccb_id_string,
		SA_IMM_ATTR_SAUINT64T,
		&ccbId
	};

	const SaImmAdminOperationParamsT_2 param1 = {
		num_ops_string,
		SA_IMM_ATTR_SAUINT32T,
		&numOps
	};

	const SaImmAdminOperationParamsT_2 *params[] = {&param0, &param1, NULL};

	osafassert(sPbe2 && !sPbe2B); /* Must be 2PBE and NOT at slave. */

	do{
		rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_PBE_CCB_PREPARE,
			params, &slavePbeRtReply, SA_TIME_ONE_SECOND * 10);

		if(rc2B == SA_AIS_ERR_TRY_AGAIN || (rc2B==SA_AIS_OK && slavePbeRtReply==SA_AIS_ERR_TRY_AGAIN)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;	
			LOG_NO("Slave PBE %u or Immsv (%u) replied with TRY_AGAIN on prepare for ccb:%llx/%llu", 
				rc2B, slavePbeRtReply, ccbId, ccbId);
		}
		/* Adjust the waiting time,a bove & below  to be more appropriate .... */
	} while (((rc2B == SA_AIS_ERR_TRY_AGAIN) || (slavePbeRtReply == SA_AIS_ERR_TRY_AGAIN)) && (msecs_waited < 3000));

	if(rc2B != SA_AIS_OK) {
		if((rc2B == SA_AIS_ERR_NOT_EXIST) && (sNoStdFlags & OPENSAF_IMM_FLAG_2PBE1_ALLOW)) {
			LOG_NO("2PBE slave NOT available but NoStFlags 0x8 set");
			retval=true;
		} else {
			LOG_WA("Start prepare for ccb: %llx/%llu towards slave PBE returned: '%u' from Immsv", ccbId, ccbId, rc2B);
		}
	} else if(slavePbeRtReply != SA_AIS_OK) {
		LOG_WA("Start prepare for ccb: %llx/%llu towards slave PBE returned: '%u' from sttandby PBE", ccbId, ccbId, slavePbeRtReply);
	} else {
		LOG_IN("Slave PBE replied with OK on attempt to start prepare of ccb:%llx/%llu", ccbId, ccbId);
		retval=true;
	}
	//if(retval && ccbId == 4) exit(1); /* Fault inject. */
	TRACE_LEAVE();
	return retval;
}

static SaAisErrorT pbe2_ok_to_prepare_ccb_at_B(SaImmOiCcbIdT ccbId, SaUint32T expectedNumOps,
	struct CcbUtilOperationData **ccbUtilOperationDataP)
{
	/* PBE slave (B) receives the start-prepare order from primary PBE(A). */
	SaAisErrorT rc = SA_AIS_OK;
	SaUint64T numReceivedOps = 0LL;

	osafassert(sPbe2 && sPbe2B); /* Must be at slave PBE. */
	if(s2PbeBCcbToCompleteAtB == 0) { 
		TRACE("First try at prepare for ccb: %llu at slave PBE", ccbId);
		s2PbeBCcbUtilCcbData = ccbutil_findCcbData(ccbId);
		if(s2PbeBCcbUtilCcbData == NULL) {
			TRACE("First ccb-op for ccb:%llu not yet received at slave PBE", ccbId);
		} else if(s2PbeBCcbUtilCcbData->ccbId != ccbId) {
			/* This should never happen, but since we dont use any locking and since the applier
			   thread may mutate ccb-utils concurrently with this lookup for a specific ccb-record,
			   there is a non-zero risk that the lookup goes wrong. If this happens then it would be
			   incredible if it still found the ccb-record with the matching ccb-id. Per definition
			   then the lookup did not go wrong. When the pointer to the ccb-record has been found *once*
			   here by the runtime thread, the pointer value is assigned to a static variable only
			   used by the runtime thread. This ccb-strucure will only be removed from immutils by
			   the (the thread that created it) in the apply or abort callback. 
			   The runtime thread thus reads from the immutils structure only from a stable pointer
			   verified to point at a ccb record with corrrect ccb-id. 
			*/
			LOG_WA("Missmatch on record for ccbId:%llx/%llu - thread interference problems ?", ccbId, ccbId);
			s2PbeBCcbUtilCcbData = NULL; 
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		} else {
			/* Normal case. Found the ccb-record. */
			numReceivedOps = (long long unsigned int) s2PbeBCcbUtilCcbData->userData;
			osafassert(numReceivedOps < 0xffffffff);
			TRACE("Slave PBE found record for ccb:%llu received ops:%llu", 
				ccbId, numReceivedOps);
		}

		osafassert(s2PbeBCcbOpCountToExpectAtB == 0);
		osafassert(s2PbeBCcbOpCountNowAtB == 0);

		s2PbeBCcbOpCountNowAtB = numReceivedOps;
		s2PbeBCcbOpCountToExpectAtB = expectedNumOps;
		s2PbeBCcbToCompleteAtB = ccbId; /* opens up for applier thread to add. */
	}

	osafassert(s2PbeBCcbToCompleteAtB == ccbId); /* Checked already by admop-code. */

	if(s2PbeBCcbOpCountToExpectAtB > s2PbeBCcbOpCountNowAtB) {
		if(s2PbeBCcbUtilCcbData == NULL) {
			s2PbeBCcbUtilCcbData = ccbutil_findCcbData(ccbId);
		}

		if(s2PbeBCcbUtilCcbData) {
			numReceivedOps = (long long unsigned int) s2PbeBCcbUtilCcbData->userData;
			osafassert(numReceivedOps < 0xffffffff);
			s2PbeBCcbOpCountNowAtB = numReceivedOps;
		}
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if(s2PbeBCcbOpCountToExpectAtB < s2PbeBCcbOpCountNowAtB) {
		LOG_ER("Current op-count %u for ccb:%llx/%llu is greater than expected: %u",
			s2PbeBCcbOpCountNowAtB,
			s2PbeBCcbToCompleteAtB,
			s2PbeBCcbToCompleteAtB,
			s2PbeBCcbOpCountToExpectAtB);
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	osafassert(s2PbeBCcbOpCountToExpectAtB == s2PbeBCcbOpCountNowAtB);
	/* Assert is redundant but keep it here just in case code is changed.
	   We never want to accidentally reply ok on prepare if we are not
	   really ready at slave.
	 */

	if(s2PbeBCcbUtilCcbData && ccbUtilOperationDataP) {
		(*ccbUtilOperationDataP) = s2PbeBCcbUtilCcbData->operationListHead;
	}
 done:
	return rc;
}

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
	std::string opensafObj;
	SaUint32T invoc=m_IMMSV_UNPACK_HANDLE_LOW(invocation);
	SaImmOiCcbIdT ccbId=0LL;	
	const SaImmAdminOperationParamsT_2 ccbIdParam = {
		ccb_id_string,
		SA_IMM_ATTR_SAUINT64T,
		&ccbId
	};

	TRACE_ENTER();

	if(sPbe2B) {
		opensafObj.append(OPENSAF_IMM_OBJECT_DN);
	} else {
		opensafObj.append((const char *) objectName->value);
		/* Weak ccb-id assigned at primary PBE (1PBE or 2PBE) 
		   Slave PBE will get same ccbId as a parameter appended to the admin-op
		   forwarded and "replicated" from primary. 
		*/
		ccbId =  invoc + 0x100000000LL;
	}

	if(opId == OPENSAF_IMM_PBE_CLASS_CREATE) {
		/* If this case ever replies with error (not OK and not TRY_AGAIN), i.e. does not persistify the class,
		   then PBE must also terminate. Class-create can not be reverted by the IMMNDs (imm-ram). 
		 */
		bool schemaChange = false;
		bool persistentExtent = false; /* ConfigC or PrtoC. Only relevant if schmaChange == true */
		bool knownConfigClass=false;
		int ix=1;
		std::string className;

		if(!param) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto fail;
		}

		do {
			TRACE("paramName: %s paramType: %u", param->paramName,	param->paramType);
			if(strcmp(param->paramName, (char *) ccb_id_string)==0) {
				/* Pick up ccbId from primary. */
				osafassert(param->paramType == SA_IMM_ATTR_SAUINT64T);
				osafassert(sPbe2 && sPbe2B);
				ccbId = *((SaUint64T *) param->paramBuffer);
			} else if(param->paramType == SA_IMM_ATTR_SASTRINGT) {
				className.append(*((SaStringT *) param->paramBuffer));
			} else {
				LOG_WA("Unhandled parameter in PBE class-create:%s - ignoring", param->paramName);
			}
			param = params[ix];
			++ix;
		} while (param);

		osafassert(className.size());

		if(sPbe2 && !sPbe2B) {
			/* Primary PBE forwards class create to slave PBE. */
			SaAisErrorT rc2B = SA_AIS_OK;
			SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
			SaAisErrorT slavePbeRtReply = SA_AIS_OK;
			const SaImmAdminOperationParamsT_2 *paramsToSlave[] = 
				{params[0], &ccbIdParam, NULL};

			rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_PBE_CLASS_CREATE,
				paramsToSlave, &slavePbeRtReply, SA_TIME_ONE_SECOND * 15);
			if(rc2B != SA_AIS_OK) {
				if(rc2B == SA_AIS_ERR_NOT_EXIST) {
					LOG_IN("Primary PBE got ERR_NOT_EXIST on atempt to create class towards slave PBE - ignoring");
				} else {
					LOG_WA("Primary PBE failed to create class towards slave PBE. Library or immsv replied Rc:%u - ignoring", rc2B);
				}
				rc2B = SA_AIS_OK;
			} else if(slavePbeRtReply == SA_AIS_ERR_REPAIR_PENDING) {
				LOG_IN("Slave PBE replied with OK to primary PBE on attempt to create class %s", className.c_str());
			} else {
				LOG_WA("PBE got error '%u' from PBE slave on create class. Should not happen.", slavePbeRtReply);
				/* The slave behaves just like the primary in this case, if anything goes wrong in create class
				   it logs the error and then exits. See the actual PBE create class transaction code below.
				   So we should never end up here! The primary PBE here ignores the error reported by the slave. 
				   Either the problem was "local" to the slave or the primary will get the same problem below
				   and also exit. But we dont immediately exit the primary PBE just because the slave had a
				   problem and reported an error. The slave could be broken or inconsistent without the primary
				   being so. */ 
			}
		} else if(sPbe2B && s2PbeBCcbToCompleteAtB) {
			/* Slave PBE may need to wait for previous transaction to complete. */
			if(pbeTransStarted()) {
				/* Extra wait for max 3 seconds */
				unsigned int try_count=0;
				unsigned int max_tries=6;
				do {
					LOG_IN("Delaying class create at slave PBE due to ongoing commit of ccb:%llx/%llu",
						s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
					usleep(500000);
					++try_count;
				} while(pbeTransStarted() && (try_count < max_tries));
			} else {
				LOG_IN("s2PbeBCcbToCompleteAtB==(%llx/%llu) => Class-create "
					"ignoring wait for applyCallback at slave", s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
			}			
		}

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start sqlite transaction for class create");
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}
		TRACE("Begin PBE transaction for class create OK");

		if(sNoStdFlags & OPENSAF_IMM_FLAG_SCHCH_ALLOW) {
			/* Schema change allowed, check for possible upgrade. */
			ClassInfo* theClass = (*sClassIdMap)[className];
			if(theClass) {
				unsigned int instances=0;
				LOG_NO("PBE detected schema change for class %s", className.c_str());
				schemaChange = true;
				AttrMap::iterator ami = theClass->mAttrMap.begin();
				if(ami == theClass->mAttrMap.end()) {
					LOG_ER("No attributes in class definition");
					rc = SA_AIS_ERR_FAILED_OPERATION;
					goto fail;
				}
				while(ami != theClass->mAttrMap.end()) {
					SaImmAttrFlagsT attrFlags = ami->second;
					if(attrFlags & SA_IMM_ATTR_CONFIG) {
						persistentExtent = true;
						knownConfigClass = true;
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
					goto fail;
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
				LOG_NO("PBE dumped %u objects of new class definition for %s", obj_count, className.c_str());
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

		LOG_IN("Create of class %s committing with ccbId:%llx", className.c_str(), ccbId);
		pbeClosePrepareTrans();
		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction %llx for class create", ccbId);
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}
		TRACE("Commited PBE transaction for class create");

		if(sPbe2B) {
			/* 2PBE slave must register as class applier for new config class. */
			if(!knownConfigClass) {
				ClassInfo* theClass = (*sClassIdMap)[className];
				osafassert(theClass);
				AttrMap::iterator ami = theClass->mAttrMap.begin();
				osafassert(ami != theClass->mAttrMap.end());

				while(ami != theClass->mAttrMap.end()) {
					SaImmAttrFlagsT attrFlags = ami->second;
					if(attrFlags & SA_IMM_ATTR_CONFIG) {
						knownConfigClass = true;
						TRACE("SLAVE PBE detected config class");
						break;
					}
					if(attrFlags & SA_IMM_ATTR_RUNTIME) {
						TRACE("SLAVE PBE detected runtime class");
						break;
					}
					++ami;
				}
			}

			if(knownConfigClass) {
				rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) className.c_str());
				if(rc != SA_AIS_OK) {
					LOG_WA("SLAVE PBE failed to become applier for config class %s",
						className.c_str());
					goto fail;
				}
				LOG_NO("SLAVE PBE is applier for config class %s", className.c_str());
			}
		}

		/* We only reply with ok result. PBE failed class create handled by timeout, no revert in imm-ram.
		   We encode OK result for OPENSAF_IMM_PBE_CLASS_CREATE with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_REPAIR_PENDING. This way the immnd can tell that this
		   is supposed to be an ok reply on PBE_CLASS_CREATE. This usage is internal to immsv.
		*/
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_REPAIR_PENDING);


	} else if(opId == OPENSAF_IMM_PBE_CLASS_DELETE) {
		/* If this case ever replies with error (not OK and not TRY_AGAIN), i.e. does not persistify the class delete,
		   then PBE must also terminate. Class-delete can not be reverted by the IMMNDs (imm-ram). 
		 */
		int ix=1;
		std::string className;

		if(!param) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto fail;
		}

		do {
			TRACE("paramName: %s paramType: %u", param->paramName,	param->paramType);
			if(strcmp(param->paramName, (char *) ccb_id_string)==0) {
				/* Pick up ccbId from primary. */
				osafassert(param->paramType == SA_IMM_ATTR_SAUINT64T);
				osafassert(sPbe2 && sPbe2B);
				ccbId = *((SaUint64T *) param->paramBuffer);
			} else if(param->paramType == SA_IMM_ATTR_SASTRINGT) {
				className.append(*((SaStringT *) param->paramBuffer));
			} else {
				LOG_WA("Unhandled parameter in PBE class-create:%s - ignoring", param->paramName);
			}
			param = params[ix];
			++ix;
		} while (param);

		osafassert(className.size());

		if(sPbe2 && !sPbe2B) {
			/* Primary PBE forwards class delete to slave PBE. */
			SaAisErrorT rc2B = SA_AIS_OK;
			SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
			SaAisErrorT slavePbeRtReply = SA_AIS_OK;
			const SaImmAdminOperationParamsT_2 *paramsToSlave[] = 
				{params[0], &ccbIdParam, NULL};

			rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_PBE_CLASS_DELETE,
				paramsToSlave, &slavePbeRtReply, SA_TIME_ONE_SECOND * 15);
			if(rc2B != SA_AIS_OK) {
				if(rc2B == SA_AIS_ERR_NOT_EXIST) {
					LOG_NO("Primary PBE Got ERR_NOT_EXIST on atempt to delete class towards slave PBE - ignoring");
				} else {
					LOG_WA("Failed to delete class towards slave PBE. Library or immsv replied Rc:%u - ignoring", rc2B);
				}
				rc2B = SA_AIS_OK;
			} else if(slavePbeRtReply == SA_AIS_ERR_NO_SPACE) {
				LOG_NO("Slave PBE replied with OK on attempt to delete class %s", className.c_str());
			} else {
				LOG_WA("PBE got error '%u' from PBE slave on attempt to delete class. Should not happen.", slavePbeRtReply);
				/* The slave behaves just like the primary in this case, if anything goes wrong in delete class
				   it logs the error and then exits. See the actual PBE delete class transaction code below.
				   So we should never end up here! The primary PBE here ignores the error reported by the slave. 
				   Either the problem was "local" to the slave or the primary will get the same problem below
				   and also exit. But we dont immediately exit the primary PBE just because the slave had a
				   problem and reported an error. The slave could be broken or inconsistent without the primary
				   being so. */ 
			}
		} else if(sPbe2B && s2PbeBCcbToCompleteAtB) {
			/* Slave PBE may need to wait for previous ccb transaction to complete. */
			if(pbeTransStarted()) {
				/* Extra wait for max 3 seconds */
				unsigned int try_count=0;
				unsigned int max_tries=6;
				do {
					LOG_IN("Delaying class delete at slave PBE due to ongoing commit of ccb:%llx/%llu",
						s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
					usleep(500000);
					++try_count;
				} while(pbeTransStarted() && (try_count < max_tries));
			} else {
				LOG_IN("s2PbeBCcbToCompleteAtB==(%llx/%llu) => Class-delete "
					"ignoring wait for applyCallback at slave", s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
			}
		}


		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start sqlite transaction for class delete");
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}
		TRACE("Begin PBE transaction for class delete OK");

		ClassInfo* theClass = (*sClassIdMap)[className];
		if(!theClass) {
			LOG_ER("Class %s missing from classIdMap", className.c_str());
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto fail;
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

		LOG_IN("Delete of class %s committing with ccbId:%llx", className.c_str(), ccbId);

		pbeClosePrepareTrans();
		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit transaction (ccb:%llx) for class delete", ccbId);
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}

		if(sClassIdMap->erase(className) != 1) {
			LOG_ER("sClassIdMap->erase(className) != 1");
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto fail;
		}
		delete theClass;
		TRACE("Commit PBE transaction for class delete");
		/* We only reply with ok result. PBE failed class delete handled by timeout, no revert in imm-ram.
		   We encode OK result for OPENSAF_IMM_PBE_CLASS_DELETE with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_NO_SPACE. This way the immnd can tell that this
		   is supposed to be an ok reply on PBE_CLASS_DELETE. This usage is internal to immsv.
		*/
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_NO_SPACE);

	} else if(opId == OPENSAF_IMM_PBE_UPDATE_EPOCH) {
		SaUint32T epoch = 0;
		int ix=1;
		if(!param) {
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto fail;
		}

		do {
			TRACE("paramName: %s paramType: %u", param->paramName,	param->paramType);
			if(strcmp(param->paramName, (char *) ccb_id_string)==0) {
				/* Pick up ccbId from primary. */
				osafassert(param->paramType == SA_IMM_ATTR_SAUINT64T);
				osafassert(sPbe2 && sPbe2B);
				ccbId = *((SaUint64T *) param->paramBuffer);
			} else if(param->paramType == SA_IMM_ATTR_SAUINT32T) {
				epoch = (*((SaUint32T *) param->paramBuffer));
			} else {
				LOG_WA("Unhandled parameter in PBE class-create:%s - ignoring", param->paramName);
			}
			param = params[ix];
			++ix;
		} while (param);


		if(sPbe2 && !sPbe2B) {
			/* Primary PBE forward update epoch to slave PBE. */
			SaAisErrorT rc2B = SA_AIS_OK;
			SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
			SaAisErrorT slavePbeRtReply = SA_AIS_OK;
			const SaImmAdminOperationParamsT_2 *paramsToSlave[] = 
				{params[0], &ccbIdParam, NULL};
			rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_PBE_UPDATE_EPOCH,
				paramsToSlave, &slavePbeRtReply, SA_TIME_ONE_SECOND * 10);
			if(rc2B != SA_AIS_OK) {
				if(rc2B == SA_AIS_ERR_NOT_EXIST) {
					LOG_IN("Primary PBE got ERR_NOT_EXIST on atempt to update epoch towards slave PBE - ignoring");
				} else {
					LOG_WA("Primary PBE failed to update epoch towards slave PBE. Rc:%u - ignoring", rc2B);
				}
			} else if(slavePbeRtReply == SA_AIS_ERR_QUEUE_NOT_AVAILABLE) {
				LOG_IN("Slave PBE replied with OK on attempt to update epoch");
			} else {
				LOG_WA("Slave PBE replied with error '%u' on attempt to update epoch", slavePbeRtReply);
				/* This should never happen.*/
			}
		}

		if(sEpoch >= epoch) {
			TRACE("Current epoch %u already equal or greater than %u ignoring", sEpoch, epoch);
			if(sEpoch == epoch) {
				goto reply_ok;
			}
			goto fail; /* dont reply on atempt to set old epoch. */
		}
		sEpoch = epoch;

		if(sPbe2B && s2PbeBCcbToCompleteAtB) {
			/* Slave PBE may need to wait for previous transaction to complete. */
			if(pbeTransStarted()) {
				/* Extra wait for max 3 seconds */
				unsigned int try_count=0;
				unsigned int max_tries=6;
				do {
					LOG_IN("Delaying update epoch at slave PBE due to ongoing commit of ccb:%llx/%llu",
						s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
					usleep(500000);
					++try_count;
				} while(pbeTransStarted() && (try_count < max_tries));
			} else {
				LOG_IN("s2PbeBCcbToCompleteAtB==(%llx/%llu) => Update-epoch "
					"ignoring wait for applyCallback at slave", s2PbeBCcbToCompleteAtB, s2PbeBCcbToCompleteAtB);
			}
		}

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start sqlite transaction for update epoch");
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}
		TRACE("Begin PBE transaction for update epoch OK");

		attrValues.attrName = (char *) OPENSAF_IMM_ATTR_EPOCH;
		attrValues.attrValueType = SA_IMM_ATTR_SAUINT32T;
		attrValues.attrValuesNumber = 1;
		attrValues.attrValues = &val;
		val = &epoch;

		objectModifyAddValuesOfAttrToPBE(sDbHandle, opensafObj,	&attrValues, 0);

		purgeCcbCommitsFromPbe(sDbHandle, sEpoch);
		
		LOG_NO("Update epoch %u committing with ccbId:%llx/%llu", sEpoch, ccbId, ccbId);
		pbeClosePrepareTrans();
		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit sqlite transaction for update epoch");
			osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
			goto fail;
		}
		TRACE("Commit PBE transaction for update epoch");
		/* We only reply with ok result. PBE failed update epoch handled by timeout, cleanup.
		   We encode OK result for OPENSAF_IMM_PBE_UPDATE_EPOCH with the arbitrary and otherwise
		   unused (in immsv) error code SA_AIS_ERR_QUEUE_NOT_AVAILABLE. This way the immnd can
		   tell that this is supposed to be an ok reply on PBE_UPDATE_EPOCH. This usage is
		   internal to immsv.
		*/
	reply_ok:
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
			LOG_IN("Incorrect parameter to NostdFlags-ON, should be 'opensafImmNostdFlags:SA_UINT32_T:nnnn'");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto fail;
		}

		SaUint32T flagsToSet = (*((SaUint32T *) param->paramBuffer));

		if(sPbe2 && !sPbe2B) {
			/* Forward nost flag ON to slave PBE. */
			SaAisErrorT rc2B = SA_AIS_OK;
			SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
			SaAisErrorT slavePbeRtReply = SA_AIS_OK;
			rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_NOST_FLAG_ON,
				params, &slavePbeRtReply, SA_TIME_ONE_SECOND * 10);
			if(rc2B != SA_AIS_OK) {
				if(rc2B == SA_AIS_ERR_NOT_EXIST) {
					LOG_IN("Got ERR_NOT_EXIST on atempt to set nostFlags towards slave PBE - ignoring");
					rc = rc2B = SA_AIS_OK;
				} else {
					LOG_NO("Failed to update nostFlags towards slave PBE. Rc:%u", rc2B);
					rc = rc2B;
				}
			} else if(slavePbeRtReply == SA_AIS_OK) {
				LOG_IN("Slave PBE replied with OK on attempt to set nostFlags");
			} else {
				LOG_WA("Slave PBE replied with error %u on attempt to set nostFlags", slavePbeRtReply);
				rc = slavePbeRtReply;
			}
		}

		if(sPbe2B && (flagsToSet == OPENSAF_IMM_FLAG_2PBE1_ALLOW)) {
			LOG_WA("Attempt to togle *ON* value x%x in nostdFlags not allowed when PBE slave is running.",
				OPENSAF_IMM_FLAG_2PBE1_ALLOW);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}


		if(rc == SA_AIS_OK) {
			sNoStdFlags |= flagsToSet;
			LOG_NO("NOSTD FLAGS value 0x%x switched ON result:0x%x", flagsToSet, sNoStdFlags);
			if(!sPbe2B) { /* Only primary PBE updates the cached RTA. */
				rc = saImmOiRtObjectUpdate_2(immOiHandle, &myObj, attrMods);
				if(rc != SA_AIS_OK) { 
					/* Should never get TRY_AGAIN here. RTA update is always accepted,
					   Except if local IMMND goes down, but then this PBE will go down also.
					 */
					sNoStdFlags &= ~flagsToSet; /* restore the flag value */
					LOG_WA("Update of cached attr %s in %s failed, rc=%u",
						attMod.modAttr.attrName, (char *) myObj.value, rc);
					/* ABT shoot down or undo for slave ?
					   The flags value in the slave is not really used.
					   Slave only "becomes" primary by process restart,
					   which will get noStdFlags from imm cached RTA.
					 */
				}
			}

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
			LOG_IN("Incorrect parameter to NostdFlags-OFF, should be 'opensafImmNostdFlags:SA_UINT32_T:nnnn'");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto fail;
		}

		SaUint32T flagsToUnSet = (*((SaUint32T *) param->paramBuffer));

		if(sPbe2 && !sPbe2B) {
			/* Forward nost flag OFF to slave PBE. */
			SaAisErrorT rc2B = SA_AIS_OK;
			SaNameT slavePbeRtObjName = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B), OPENSAF_IMM_PBE_RT_OBJECT_DN_B};
			SaAisErrorT slavePbeRtReply = SA_AIS_OK;
			rc2B = saImmOmAdminOperationInvoke_2(sOwnerHandle, &slavePbeRtObjName, 0, OPENSAF_IMM_NOST_FLAG_OFF,
				params, &slavePbeRtReply, SA_TIME_ONE_SECOND * 10);
			if(rc2B != SA_AIS_OK) {
				if(rc2B == SA_AIS_ERR_NOT_EXIST) {
					LOG_IN("Got ERR_NOT_EXIST on atempt to un-set nostFlags towards slave PBE - ignoring");
					rc = rc2B = SA_AIS_OK;
				} else {
					if(flagsToUnSet == OPENSAF_IMM_FLAG_2PBE1_ALLOW) {
						LOG_NO("Failed to un-set flag 2PBE1_ALLOW towards slave PBE. Rc:%u - overriding",
							rc2B);
						rc = rc2B = SA_AIS_OK;
					} else {
						LOG_WA("Failed to update nostFlags towards slave PBE. Rc:%u", rc2B);
						rc = rc2B;
					}
				}
				rc2B = SA_AIS_OK;
			} else if(slavePbeRtReply == SA_AIS_OK) {
				LOG_IN("Slave PBE replied with OK on attempt to un-set nostFlags");
			} else {
				LOG_WA("Slave PBE replied with error '%u' on attempt to un-set nostFlags", slavePbeRtReply);
				rc = slavePbeRtReply;
			}
		}

		if(rc == SA_AIS_OK) {
			sNoStdFlags &= ~flagsToUnSet;
			LOG_NO("NOSTD FLAGS value 0x%x switched OFF result:0x%x", flagsToUnSet, sNoStdFlags);
			if(!sPbe2B) { /* Only primary PBE updates the cached RTA. */
				rc = saImmOiRtObjectUpdate_2(immOiHandle, &myObj, attrMods);
				if(rc != SA_AIS_OK) {
					sNoStdFlags |= flagsToUnSet; /* restore the flag value */
					LOG_WA("Update of cached attribute attr %s in %s failed, rc=%u",
						attMod.modAttr.attrName, (char *) myObj.value, rc);
					/* ABT shoot down or undo for slave ?
					   The flags value in the slave is not really used.
					   Slave only "becomes" primary by process restart,
					   which will get noStdFlags from imm cached RTA.
					 */
				}
			}
		}

		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
	} else if(opId == OPENSAF_IMM_PBE_CCB_PREPARE) {
		SaUint32T numOps=0;
		int ix=1;
		struct CcbUtilOperationData *ccbUtilOperationData = NULL;
		if(!param) {
			LOG_ER("No parameters received in PBE slave ccb prepare");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			return;
		}
		/* First pick up ccb-id and num-ops. */
		do {
			if(strcmp(param->paramName, (char *) ccb_id_string)==0) {
				osafassert(param->paramType == SA_IMM_ATTR_SAUINT64T);
				ccbId = *((SaUint64T *) param->paramBuffer);
			} else if(strcmp(param->paramName, (char *) num_ops_string)==0) {
				osafassert(param->paramType == SA_IMM_ATTR_SAUINT32T);
				numOps = *((SaUint32T *) param->paramBuffer);
			} else {
				LOG_WA("Unhandled parameter in slave ccb prepare:%s - ignoring", param->paramName);
			}
			param = params[ix];
			++ix;
		} while (param);
		if(!ccbId) {
			LOG_ER("Received zero ccbId in PBE slave ccb prepare");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			return;
		}
		if(!numOps) {
			LOG_ER("Received zero numOps in PBE slave ccb prepare");
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			return;
		}
		LOG_IN("ccb-prepare received at PBE slave ccbId:%llx/%llu numOps:%u", ccbId, ccbId, numOps);

		if((s2PbeBCcbToCompleteAtB!=0) && (s2PbeBCcbToCompleteAtB!=ccbId)) {
			LOG_NO("Prepare ccb:%llx/%llu received at Pbe slave when Prior Ccb %llu still processing",
				ccbId, ccbId, s2PbeBCcbToCompleteAtB);
			rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN);
			if(rc != SA_AIS_OK) {
				LOG_WA("2PBE slave failed in reply TRY_AGAIN on ccb prepare rc:%u", rc);
			}
			return;
		}
		
		/* Then short wait locally for numOps to be received at slave. */
		rc = pbe2_ok_to_prepare_ccb_at_B(ccbId, numOps, &ccbUtilOperationData);
		if(rc == SA_AIS_OK) {
			rc =  pbeBeginTrans(sDbHandle);
			if(rc != SA_AIS_OK) {
				LOG_WA("PBE failed to start sqlite transaction for CCB %llx/%llu", ccbId, ccbId);
				osafassert(rc != SA_AIS_ERR_TRY_AGAIN);
				goto fail;
			}
		} else if(rc != SA_AIS_ERR_TRY_AGAIN) {
			LOG_WA("Prepare for ccb %llx/%llu failed at slave PBE rc:%u", ccbId, ccbId, rc);
		}

		if(immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc) != SA_AIS_OK) {
			LOG_WA("Slave PBE failed in replying to primary on prepare for ccb %llx/%llu.", ccbId, ccbId);
			rc = SA_AIS_ERR_NO_RESOURCES;
		}

		if(rc == SA_AIS_OK) {
			/*Now actually do the sqlite slave prepare here instead of in the completed
			  callback. This is (A) faster i.e. more tight and in parallel with primary.
			  The actuall commit of the ccb sqlite transaction is still done by the 
			  applier thread. This is to avoid the slave ever committing a CCB *before*
			  the primary. 

			  There is the issue of cleanup and mutating the immutils structure.
			  Mutating the immutils structure is only done by the applier thread.
			  It builds up the ccb ummutils and tears it down in apply/abort callback.
			  The RTO thread does read/navigate immutills though. 
			*/

			rc = sqlite_prepare_ccb(immOiHandle, ccbId, ccbUtilOperationData);
		}
		//if(rc == SA_AIS_OK && ccbId == 4) exit(1); /* Fault inject. */
	} else 	if(opId == OPENSAF_IMM_BAD_OP_BOUNCE) {
		LOG_WA("ERR_BAD_OPERATION: bounced in PBE");
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION);
	} else {
		LOG_WA("Invalid operation ID %llu", (SaUint64T) opId);
		rc = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
	}
 fail:
	TRACE_LEAVE();
	if(rc != SA_AIS_OK && rc != SA_AIS_ERR_TRY_AGAIN) {
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
        long long unsigned int opCount=0;

	TRACE_ENTER2("Modify callback for CCB:%llu object:%s", ccbId, objectName->value);
	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llx/%llu", ccbId, ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	opCount = (long long unsigned int) ccbUtilCcbData->userData;

	if(strncmp((char *) objectName->value, (char *) OPENSAF_IMM_OBJECT_DN, objectName->length) ==0) {
		char buf[sBufsize];
		snprintf(buf, sBufsize,
			"PBE will not allow modifications to object %s", (char *) OPENSAF_IMM_OBJECT_DN);
		LOG_NO("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
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

	ccbUtilCcbData->userData = (void *) (opCount +1);

	if(ccbId > 0x100000000LL) { /* PRTA update operation. */
		const SaImmAttrModificationT_2 *attMod = NULL;
		int ix=0;
		std::string objName;
		operation = ccbutil_getNextCcbOp(ccbId, NULL);
		if(operation == NULL) {
			LOG_ER("ccbutil_getNextCcbOp(%llx, NULL) returned NULL", ccbId);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		
		TRACE("Update of PERSISTENT runtime attributes in object with DN: %s",
			operation->objectName.value);

		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start sqlite transaction for PRT attr update");
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
						goto abort_trans;
					}
					
					objectModifyAddValuesOfAttrToPBE(sDbHandle, objName,
						&(attMod->modAttr), ccbId);
							
					break;

				case SA_IMM_ATTR_VALUES_DELETE:
					if(attMod->modAttr.attrValuesNumber == 0) {
						LOG_ER("Empty value used for deleting from attribute %s",
							attMod->modAttr.attrName);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto abort_trans;
					}

					objectModifyDiscardMatchingValuesOfAttrToPBE(sDbHandle, objName,
						&(attMod->modAttr), ccbId);
					
					break;
				default: abort();
			}
		}


		ccbutil_deleteCcbData(ccbutil_findCcbData(ccbId));
		pbeClosePrepareTrans();
		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit sqlite transaction (ccb:%llx) for PRT attr update", ccbId);
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
		TRACE("Commit PBE transaction %llx for rt attr update OK", ccbId);
	}


	goto done;
 abort_trans:
	pbeAbortTrans(sDbHandle);
 done:
	if((rc != SA_AIS_OK) && sPbe2 && (ccbId > 0x100000000LL)) {
		/* For 2PBE, PRTA update is done in parallell at primary and slave. 
		   This means four possible outcommes:
		   i) Ok at primary & Ok at slave => normal case.
		   ii) Ok at primary _& failure at slave.
		   iii) Failure at primary & Ok at slave.
		   iv) Failure at primary and failure at slave.
		   Current PBE implementation can not distinguish cases (11) - (iv).
		 */
		LOG_ER("2PBE can not tolerate error (%u) in PRTA update (ccbId:%llx) - exiting", rc, ccbId);
		exit(1);
	}
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

	   If we unambiguously fail to commit, that is if pbeCommitTrans returns 
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
	struct CcbUtilCcbData *ccbUtilCcbData = NULL;
	char buf[sBufsize];
	SaUint64T numOps=0;


	TRACE_ENTER2("Completed callback for CCB:%llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_WA("Failed to find CCB object for %llx/%llu - checking DB file for outcome", ccbId, ccbId);
		rc = getCcbOutcomeFromPbe(sDbHandle, ccbId, sEpoch);/* rc=BAD_OPERATION or OK */
		(void) ccbutil_getCcbData(ccbId); /*generate an empty record*/
		goto done;
	}

	if(sPbe2) {
		/* Primary PBE has requested slave PBE to start preparing. If slave replied Ok
		   then slave was ready to start prepare and has started its prepare.
		   Reply sent from slave before slave prepare was completed, so both 
		   primary and slave will do the prepare in parallell, more or less.
		   Primary will commit as soon as it has completed its prepare.
		   Then ack to imm-ram (immnd). The Immnd at the slave will send
		   completed and apply to the slave PBE (as an applier). 
		 */
		numOps = (long long unsigned int) ccbUtilCcbData->userData;
		osafassert(numOps < 0xffffffff);
		if(sPbe2B) { /* PbeB == slave */
			if(s2PbeBCcbToCompleteAtB != ccbId) {
				LOG_ER("PBE-B got completed callback for Ccb:%llx/%llu before prepare from PBE-A", ccbId, ccbId);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto abort_trans;
			}

			if(s2PbeBCcbOpCountToExpectAtB != numOps) {
				LOG_ER("PBE-B got completed callback for Ccb:%llx/%llu but numOps:%llu should be: %u",
					ccbId, ccbId, numOps, s2PbeBCcbOpCountToExpectAtB);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto abort_trans;
			}
			goto commit_trans; /* Jump over begin-trans & prepare. Done already at PBESlave/B */

		} else { /* PbeA == primary */
			LOG_IN("Starting distributed PBE commit for Ccb:%llx/%llu", ccbId, ccbId);
			if(!pbe2_start_prepare_ccb_A_to_B(ccbId, (SaUint32T) numOps)) { /* Order slave to start preparing. */
				LOG_WA("PBE-A failed to prepare Ccb:%llx/%llu towards PBE-B", ccbId, ccbId);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto abort_trans;
			}
		}
	}

	if((rc =  pbeBeginTrans(sDbHandle)) != SA_AIS_OK) { goto done;}

	TRACE("Begin PBE transaction for CCB %llu OK", ccbId);
	rc = sqlite_prepare_ccb(immOiHandle, ccbId, ccbUtilCcbData->operationListHead);
	if(rc != SA_AIS_OK) {
		goto abort_trans;
	}

	/* Query slave on prepare success, if success continue with commit
	   If not success on prepare at slave then abort.
	   If timeout ..... a timeout that should be long in the first place,
	   then possible retry the query.
	   IF NOT EXIST then...
	   The slave will comit when/if it receives the apply callback. 
	 */
 commit_trans:
	rc =  pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
	if(rc == SA_AIS_OK) {
		TRACE("COMMIT PBE TRANSACTION for ccb %llu epoch:%u OK", ccbId, sEpoch);
		sLastCcbCommit = ccbId;
	} else {
		snprintf(buf, sBufsize,
			"PBE COMMIT of sqlite transaction %llu epoch%u FAILED rc:%u", ccbId, sEpoch, rc);
		TRACE("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
	}

	/* Fault injection.*/
	/*if(ccbId == 4) { exit(1);} 
	*/
	goto done;

 abort_trans:
	if(pbeTransStarted()) {
		pbeAbortTrans(sDbHandle);
	}
	if(sPbe2B) { /* PBE-B slave can not abort unilateraly. */
		LOG_WA("PBE slave exiting in prepare for ccb %llx/%llu, file should be regenerated.", ccbId, ccbId);
		exit(0);
	}
 done:
	TRACE_LEAVE();
	return rc;
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	//const SaImmAttrModificationT_2 *attrMod;

	TRACE_ENTER2("PBE APPLY CALLBACK cleanup CCB:%llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_WA("Failed to find CCB object for %llx/%llu - PBE restarted recently?", ccbId, ccbId);
		goto done;
	}

	if(sPbe2) {
		/* Reset 2pbe-ccb-syncronization variables. */
		if(s2PbeBCcbToCompleteAtB == ccbId) {
			s2PbeBCcbToCompleteAtB=0; 
			s2PbeBCcbOpCountToExpectAtB=0;
			s2PbeBCcbOpCountNowAtB=0;
			s2PbeBCcbUtilCcbData = NULL;
		}
	}

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
		LOG_WA("Failed to find CCB object for %llx/%llu", ccbId, ccbId);
	}

	/* Reset 2pbe-ccb-syncronization variables. */
	if(s2PbeBCcbToCompleteAtB == ccbId) {
		s2PbeBCcbToCompleteAtB=0; 
		s2PbeBCcbOpCountToExpectAtB=0;
		s2PbeBCcbOpCountNowAtB=0;
		s2PbeBCcbUtilCcbData = NULL;
	}

	TRACE_LEAVE();
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	char buf[sBufsize];
	struct CcbUtilCcbData *ccbUtilCcbData;
	bool rdnFound=false;
	const SaImmAttrValuesT_2 *attrValue;
	int i = 0;
	std::string classNameString(className);
	struct CcbUtilOperationData  *operation = NULL;
	ClassInfo* classInfo = (*sClassIdMap)[classNameString];
	long long unsigned int opCount=0;

	if(parentName && parentName->length) {
		TRACE_ENTER2("CREATE CALLBACK CCB:%llu class:%s parent:%s", ccbId, className, parentName->value);
	} else {
		TRACE_ENTER2("CREATE CALLBACK CCB:%llu class:%s ROOT OBJECT (no parent)", ccbId, className);
	}
	if(!classInfo) {
		snprintf(buf, sBufsize, "PBE Internal error: class '%s' not found in classIdMap", className);
		LOG_ER("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			snprintf(buf, sBufsize, "PBE Internal error: Failed to get CCB objectfor %llu", ccbId);
			LOG_ER("%s", buf);
			saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	opCount = (long long unsigned int) ccbUtilCcbData->userData;

	if(strncmp((char *) className, (char *) OPENSAF_IMM_CLASS_NAME, strlen(className)) == 0) {
		snprintf(buf, sBufsize,
			"PBE: will not allow creates of instances of class %s", (char *) OPENSAF_IMM_CLASS_NAME);
		LOG_NO("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
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
				snprintf(buf, sBufsize,
					"PBE: More than one RDN attribute found in attribute list");
				LOG_NO("%s", buf);
				saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
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
				snprintf(buf, sBufsize,
					"PBE: Rdn attribute %s for class '%s' is neither SaStringT nor SaNameT!", 
					attrValue->attrName, className);
				LOG_NO("%s", buf);
				saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			TRACE("Extracted DN: %s(%u)", operation->objectName.value,  operation->objectName.length);
		}
	}

	if(!rdnFound) {
		snprintf(buf, sBufsize, "PBE: Could not find Rdn attribute for class '%s'!", className);
		LOG_ER("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if(operation->objectName.length <= 0) {
		LOG_ER("operation->objectName.length can not be zero or negative");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;		
	}

	ccbUtilCcbData->userData = (void *) (opCount +1);

	if(ccbId > 0x100000000LL) {
		TRACE("Create of PERSISTENT runtime object with DN: %s",
			operation->objectName.value);
		rc = pbeBeginTrans(sDbHandle);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to start sqlite transaction (ccbId:%llx)for PRTO create", ccbId);
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		TRACE("Begin PBE transaction for rt obj create OK");

		objectToPBE(std::string((const char *) operation->objectName.value), 
			operation->param.create.attrValues,
			sClassIdMap, sDbHandle, ++sObjCount,
			operation->param.create.className, ccbId);

		ccbutil_deleteCcbData(ccbutil_findCcbData(0));
		pbeClosePrepareTrans();
		rc = pbeCommitTrans(sDbHandle, ccbId, sEpoch, &sLastCcbCommitTime);
		if(rc != SA_AIS_OK) {
			LOG_WA("PBE failed to commit sqlite transaction (ccbId:%llx) for PRTO create", ccbId);
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
		TRACE("Commit PBE transaction for rt obj create OK");
	}

 done:
	if((rc != SA_AIS_OK) && sPbe2 && (ccbId > 0x100000000LL)) {
		/* For 2PBE, PRTA update is done in parallell at primary and slave. 
		   This means four possible outcommes:
		   i) Ok at primary & Ok at slave => normal case.
		   ii) Ok at primary _& failure at slave.
		   iii) Failure at primary & Ok at slave.
		   iv) Failure at primary and failure at slave.
		   Current PBE implementation can not distinguish cases (11) - (iv).
		 */
		LOG_ER("2PBE can not tolerate error (%u) in PRTA update - exiting", rc);
		exit(1);
	}
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	char buf[sBufsize];
	long long unsigned int opCount=0;
	TRACE_ENTER2("DELETE CALLBACK CCB:%llu object:%s", ccbId, objectName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			snprintf(buf, sBufsize, "PBE: Failed to get CCB objectfor %llx/%llu", ccbId, ccbId);
			LOG_WA("%s", buf);
			saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	opCount = (long long unsigned int) ccbUtilCcbData->userData;

	if(strncmp((char *) objectName->value, (char *) OPENSAF_IMM_OBJECT_DN, objectName->length) ==0) {
		snprintf(buf, sBufsize, "PBE: will not allow delete of object %s", (char *) OPENSAF_IMM_OBJECT_DN);
		LOG_NO("%s", buf);
		saImmOiCcbSetErrorString(immOiHandle, ccbId, buf);
		rc = SA_AIS_ERR_BAD_OPERATION;
		/* We will actually get invoked twice on this object, both as normal implementer and as PBE
		   the response on the delete upcall from PBE is discarded, but not for the regular implemener.
		 */
	} else {
		/* "memorize the deletion request" */
		ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);
	}

	ccbUtilCcbData->userData = (void *) (opCount +1);

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
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;
	TRACE_ENTER2("RT ATTR UPDATE CALLBACK PBE2B:%u DN:%s A:%s", sPbe2B, 
		(const char *) objectName->value, OPENSAF_IMM_PBE_RT_OBJECT_DN_A);

	if((sPbe2B && strcmp((const char *) objectName->value, OPENSAF_IMM_PBE_RT_OBJECT_DN_B)==0) ||
	   (!sPbe2B && strcmp((const char *) objectName->value, OPENSAF_IMM_PBE_RT_OBJECT_DN_A)==0))
	{
		SaImmAttrValueT attrUpdateValues1[] = {&sEpoch};
		SaImmAttrValueT attrUpdateValues2[] = {&sLastCcbCommit};
		SaImmAttrValueT attrUpdateValues3[] = {&sLastCcbCommitTime};

		SaImmAttrValuesT_2 attr1 = {
			(char *) OPENSAF_IMM_ATTR_PBE_RT_EPOCH,
			SA_IMM_ATTR_SAUINT32T,
			1,
			attrUpdateValues1
		};

		SaImmAttrValuesT_2 attr2 = {
			(char *) OPENSAF_IMM_ATTR_PBE_RT_CCB,
			SA_IMM_ATTR_SAUINT64T,
			1,
			attrUpdateValues2
		};

		SaImmAttrValuesT_2 attr3 = {
			(char *) OPENSAF_IMM_ATTR_PBE_RT_TIME,
			SA_IMM_ATTR_SATIMET,
			1,
			attrUpdateValues3
		};

		SaImmAttrModificationT_2 attrMod1 = {SA_IMM_ATTR_VALUES_REPLACE, attr1};
		SaImmAttrModificationT_2 attrMod2 = {SA_IMM_ATTR_VALUES_REPLACE, attr2};
		SaImmAttrModificationT_2 attrMod3 = {SA_IMM_ATTR_VALUES_REPLACE, attr3};

		const SaImmAttrModificationT_2* attrMods[] = {&attrMod1, &attrMod2, &attrMod3, NULL};
 
		rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
		TRACE("saImmOiRtObjectUpdate_2 returned:%u", rc);
	}

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

static const SaImmOiCallbacksT_2 rtCallbacks = {
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
	std::string pbeImplName; /* Used for PBE-OI and PBE-applier */
	std::string pbeRtImplName; /* Used for handling PBE runtime data */
	const SaNameT myParent = {sizeof(OPENSAF_IMM_OBJECT_DN), OPENSAF_IMM_OBJECT_DN};
	const SaStringT rdnStr=(SaStringT) ((sPbe2B)?"osafImmPbeRt=B":"osafImmPbeRt=A");
	const SaImmAttrValueT nameValues[] = {(SaImmAttrValueT) &rdnStr};
	const SaImmAttrValuesT_2 v1 = {(SaImmAttrNameT) OPENSAF_IMM_ATTR_PBE_RT_RDN, SA_IMM_ATTR_SASTRINGT, 1, 
				       (void**)nameValues };
	const SaImmAttrValuesT_2 *attrValues[] = {&v1, NULL};

	ClassMap::iterator ci;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;

	SaNameT pbeRtObjNameA = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_A),OPENSAF_IMM_PBE_RT_OBJECT_DN_A};
	SaNameT pbeRtObjNameB = {sizeof(OPENSAF_IMM_PBE_RT_OBJECT_DN_B),OPENSAF_IMM_PBE_RT_OBJECT_DN_B};

	const SaNameT* admOwnNames[] = {(sPbe2B)?(&pbeRtObjNameB):(&pbeRtObjNameA), &myParent, NULL};

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

	if(sPbe2B) {
		/* Second PBE (applier) */
		pbeImplName.append((char *) OPENSAF_IMM_2PBE_APPL_NAME);
		pbeRtImplName.append((char *) OPENSAF_IMM_PBE_RT_IMPL_NAME_B);
	} else {
		/* Primary PBE OI */
		pbeImplName.append((char *) OPENSAF_IMM_PBE_IMPL_NAME);
		pbeRtImplName.append((char *) OPENSAF_IMM_PBE_RT_IMPL_NAME_A);
	}

	rc = saImmOiImplementerSet(pbeOiHandle, (char *) pbeImplName.c_str());
	msecs_waited = 0;
	while ((rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_EXIST) && 
		(msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiImplementerSet(pbeOiHandle,  (char *) pbeImplName.c_str());
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet for %s failed %u", pbeImplName.c_str(), rc);
		return rc;
	}

	if(sPbe2) {
		rc = saImmOiInitialize_2(&pbeOiRtHandle, &rtCallbacks, &immVersion);
		while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			rc = saImmOiInitialize_2(&pbeOiRtHandle, &rtCallbacks, &immVersion);
		}
		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiInitialize_2 failed for RtHandle: %u", rc);
			return rc;
		}

		rc = saImmOiImplementerSet(pbeOiRtHandle, (char *) pbeRtImplName.c_str());
		msecs_waited = 0;
		while ((rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_EXIST) && 
			(msecs_waited < max_waiting_time_ms)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			rc = saImmOiImplementerSet(pbeOiRtHandle,  (char *) pbeRtImplName.c_str());
		}
		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiImplementerSet for %s failed %u", pbeRtImplName.c_str(), rc);
			return rc;
		}

		rc = saImmOiSelectionObjectGet(pbeOiRtHandle, &immOiRtSelectionObject);
		/* SelectionObjectGet is library local, no need for retry loop */
		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiSelectionObjectGet failed for RT %u", rc);
			return rc;
		}

		msecs_waited = 0;
		do {
			rc = saImmOiRtObjectCreate_2(pbeOiRtHandle,
				(SaImmClassNameT) OPENSAF_IMM_PBE_RT_CLASS_NAME, &myParent, attrValues);
			if(rc == SA_AIS_ERR_EXIST) {rc = SA_AIS_OK;}

			/* Set admin owner on the RTO just created. */
			if(rc == SA_AIS_OK) {
				rc = saImmOmAdminOwnerSet(sOwnerHandle, admOwnNames, SA_IMM_ONE);
			}
			if(rc == SA_AIS_ERR_TRY_AGAIN) {
				usleep(sleep_delay_ms * 1000);
				msecs_waited += sleep_delay_ms;
			}
		} while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms*12)); /* 60 secs */

		if(sPbe2B) {
			/* Set slave PBE as class applier for ALL config classes. */
			for(ci=sClassIdMap->begin(); ci!=sClassIdMap->end();++ci) {
				if((ci)->second->mClassId > sClassCount) {
					sClassCount = (ci)->second->mClassId;
				}

				/* classCategory should really also be in sClassIdMap */
				rc = saImmOmClassDescriptionGet_2(immHandle,
					(char*)(ci)->first.c_str(),
					&classCategory,
					&attrDefinitions);
				/* PBE should never get TRY_AGAIN from saImmOmClassDescriptionGet. */
				if (rc != SA_AIS_OK) 	{
					LOG_ER("Failed to get the description for the %s class error:%u",
						(char*)(ci)->first.c_str(), rc);
					return rc;
				}

				rc = saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
				osafassert(rc == SA_AIS_OK);

				if(classCategory == SA_IMM_CLASS_RUNTIME) {continue;} /* with next class */

				osafassert(classCategory == SA_IMM_CLASS_CONFIG);

				rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) (ci)->first.c_str());
				msecs_waited = 0;
				while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
					usleep(sleep_delay_ms * 1000);
					msecs_waited += sleep_delay_ms;	
					rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) (ci)->first.c_str());
				}

				if (rc != SA_AIS_OK) {
					LOG_ER("saImmOiClassImplementerSet for %s failed %u", 
						(char *) (ci)->first.c_str(), rc);
					return rc;
				}
			}
		}
	}

	if(!sPbe2B) {	/* Not pbe2B i.e. pbe2A or not 2PBE at all. */
		rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_CLASS_NAME);
		msecs_waited = 0;
		while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;	
			rc = saImmOiClassImplementerSet(pbeOiHandle, (char *) OPENSAF_IMM_CLASS_NAME);
		}

		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiClassImplementerSet for %s failed %u", OPENSAF_IMM_CLASS_NAME, rc);
			return rc;
		}
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


static void *pbeRtObjThread(void*)
{
	SaAisErrorT rc;

	while(immOiRtSelectionObject) {
		TRACE("##@-PBE RUNTIME thread entering poll");
		int ret = poll(rtfds, nrtfds, -1);
		TRACE("##@-PBE RUNTIME thread continues after poll ret: %d", ret);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if(pbeOiRtHandle && rtfds[FD_IMM_PBE_RT_OI].revents & POLLIN) {
			rc = saImmOiDispatch(pbeOiRtHandle, SA_DISPATCH_ALL);
			if(rc != SA_AIS_OK) {
				TRACE("saImmOiDispatch returned %u", rc);
				pbeOiRtHandle = 0;
				break;		
			}
		}
	}
	
	immOiSelectionObject = 0; /* Stop main thread. */
	return NULL;
}


/**
 * Set up a separate thread for handling the OsafImmPbeRt runtime object.
 */
void pbeRtObjThreadStart()
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();
	if (pthread_create(&thread, &attr, pbeRtObjThread, NULL) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(1);
	}
	
	pthread_attr_destroy(&attr);
	
	TRACE_LEAVE();
}

void pbeDaemon(SaImmHandleT immHandle, void* dbHandle, SaImmAdminOwnerHandleT ownerHandle,
	ClassMap* classIdMap, unsigned int objCount, bool pbe2, bool pbe2B)
{

	SaAisErrorT error = SA_AIS_OK;
	ClassMap::iterator ci;

	/* cbHandle, classIdMap and objCount are needed by the PBE OI upcalls.
	   Make them available by assigning to module local static variables. 
	 */
	sDbHandle = dbHandle;
	sClassIdMap = classIdMap;
	sObjCount = objCount;
	sPbe2B = pbe2B;
	sPbe2 = pbe2;
	sOwnerHandle = ownerHandle;
	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.retryInterval = 400;
	immutilWrapperProfile.nTries = 5;


	LOG_NO("pbeDaemon starting with obj-count:%u", sObjCount);

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

	if(sPbe2) {
		/* Set up file descriptors for pbeRtObjThread */
		rtfds[FD_IMM_PBE_RT_OI].fd = immOiRtSelectionObject;
		rtfds[FD_IMM_PBE_RT_OI].events = POLLIN;	

		pbeRtObjThreadStart();
	}

	/* This main thread Can be stopped by pbeRtObjThread by it zeroing 
	   immOiSelectionObject. That does not affect the fds array since it 
	   contains a copy of the descriptor. 
	 */
	while(immOiSelectionObject) { 
		TRACE("##@-PBE MAIN thread entering poll");
		int ret = poll(fds, nfds, -1);
		TRACE("##@-PBE MAIN thead continues after poll ret: %d", ret);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_IMM_PBE_TERM].revents & POLLIN) {
			ncs_sel_obj_rmv_ind(term_sel_obj, true, true);
			if (sDbHandle != NULL) {
				LOG_NO("IMM %s received SIG_TERM, closing db handle", 
					sPbe2 ? (sPbe2B?"PBE SLAVE":"PBE PRIMARY"):"PBE");
				pbeRepositoryClose(sDbHandle);
				sDbHandle = NULL;
			}
			break;
		}

		if (immHandle && (fds[FD_IMM_PBE_OM].revents & POLLIN)) {
			error = saImmOmDispatch(immHandle, SA_DISPATCH_ONE);
			if (error != SA_AIS_OK) {
				LOG_WA("saImmOmDispatch returned %u PBE lost contact with IMMND - exiting", error);
				if(sPbe2) {
					/* With 2PBE it is safe and prudent to close DB before exit.
					   Because all access to the imm.db is *local*,
					   posix advisory locks MUST work.
					   With regular PBE we should terminate as quickly as possible
					   to minimize risk of file system interference between old
					   and new PBE. 
					 */
					pbeRepositoryClose(sDbHandle);
					sDbHandle = NULL;
				}
				pbeOiHandle = 0;
				immHandle = 0;
				break;
			}
		}

		if (pbeOiHandle && (fds[FD_IMM_PBE_OI].revents & POLLIN)) {
			error = saImmOiDispatch(pbeOiHandle, SA_DISPATCH_ONE);

			if (error != SA_AIS_OK) {
				TRACE("saImmOiDispatch returned %u", error);
				if(sPbe2) {
					pbeRepositoryClose(sDbHandle);
					sDbHandle = NULL;
				}
				pbeOiHandle = 0;
				break;
			}
		}
		/* Attch as OI for 
		   SaImmMngt: safRdn=immManagement,safApp=safImmService
		   ?? OpensafImm: opensafImm=opensafImm,safApp=safImmService ??
		*/


	}

		LOG_IN("IMM %s process EXITING...", sPbe2 ? (sPbe2B?"PBE SLAVE":"PBE PRIMARY"):"PBE");
	TRACE_LEAVE();
	exit(1);
}


