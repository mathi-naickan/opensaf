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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>
#include <logtrace.h>
#include <saflog.h>

#include <evt.h>
#include <util.h>
#include <cb.h>
#include <amf_defs.h>
#include <imm.h>
#include <cluster.h>
#include <app.h>
#include <sgtype.h>
#include <sg.h>
#include <su.h>
#include <sutype.h>
#include <comp.h>
#include <si.h>
#include <csi.h>
#include <si_dep.h>
#include "osaf_utility.h"

#include "osaf_time.h"
#include <stdint.h>
#include <unordered_map>
#include <string>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */
/* mutex for synchronising  imm initialization and role change events. */
pthread_mutex_t imm_reinit_mutex = PTHREAD_MUTEX_INITIALIZER;
/* mutex for synchronising  amfd thread and imm initialization thread startup 
   to make sure that imm initialization thread executes first and take
   imm_reinit_mutex and then proceeds for initialization. */
static pthread_mutex_t imm_reinit_thread_startup_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t imm_reinit_thread_startup_cond = PTHREAD_COND_INITIALIZER;

typedef std::unordered_map<std::string, AVSV_AMF_CLASS_ID> type_map;

type_map amf_type_map = {
	/* Cluster and Node Class Related */
	{"safAmfCluster",AVSV_SA_AMF_CLUSTER},
	{"safAmfNode",AVSV_SA_AMF_NODE},
	{"safAmfNodeGroup",AVSV_SA_AMF_NODE_GROUP},
	{"safInstalledSwBundle",AVSV_SA_AMF_NODE_SW_BUNDLE},
	/* Application Class Related */
	{"safApp",AVSV_SA_AMF_APP},
	{"safAppType",AVSV_SA_AMF_APP_BASE_TYPE},
	/* Service Group Class Related */
	{"safSg",AVSV_SA_AMF_SG},
	{"safSgType",AVSV_SA_AMF_SG_BASE_TYPE},
	/* Service Unit Class Related */
	{"safSu",AVSV_SA_AMF_SU},
	{"safSuType",AVSV_SA_AMF_SU_BASE_TYPE},
	{"safMemberCompType",AVSV_SA_AMF_SUT_COMP_TYPE},
	/* Service Instance Class Related */
	{"safSi",AVSV_SA_AMF_SI},
	{"safSvcType",AVSV_SA_AMF_SVC_BASE_TYPE},
	{"safDepend",AVSV_SA_AMF_SI_DEPENDENCY},
	{"safRankedSu",AVSV_SA_AMF_SI_RANKED_SU},
	{"safSISU",AVSV_SA_AMF_SI_ASSIGNMENT},
	{"safMemberCSType",AVSV_SA_AMF_SVC_TYPE_CS_TYPES},
	/* Component Service Instance Class Related */
	{"safCsi",AVSV_SA_AMF_CSI},
	{"safCSType",AVSV_SA_AMF_CS_BASE_TYPE},
	{"safCsiAttr",AVSV_SA_AMF_CSI_ATTRIBUTE},
	{"safCSIComp",AVSV_SA_AMF_CSI_ASSIGNMENT},
	/* Component and component types Related */
	{"safCompType",AVSV_SA_AMF_COMP_BASE_TYPE},
	{"safSupportedCsType",AVSV_SA_AMF_CLASS_INVALID},
	{"safComp",AVSV_SA_AMF_COMP},
	/* Global Component Attributes and Health Check Related */
	{"safRdn",AVSV_SA_AMF_COMP_GLOBAL_ATTR},
	{"safHealthcheckKey",AVSV_SA_AMF_CLASS_INVALID},
	/* Common Version Related */
	{"safVersion",AVSV_SA_AMF_CLASS_INVALID}
};

type_map versioned_types = {
	{"safAppType",AVSV_SA_AMF_APP_TYPE},
	{"safSgType",AVSV_SA_AMF_SG_TYPE},
	{"safSuType",AVSV_SA_AMF_SU_TYPE},
	{"safSvcType",AVSV_SA_AMF_SVC_TYPE},
	{"safCSType",AVSV_SA_AMF_CS_TYPE},
	{"safCompType",AVSV_SA_AMF_COMP_TYPE}
};

//
// TODO(HANO) Temporary use this function instead of strdup which uses malloc.
// Later on remove this function and use std::string instead
#include <cstring>
static char *StrDup(const char *s)
{
	char *c = new char[strlen(s) + 1];
	std::strcpy(c,s);
	return c;
}
uint32_t const MAX_JOB_SIZE_AT_STANDBY = 200;

//
Job::~Job()
{
}

//
AvdJobDequeueResultT ImmObjCreate::exec(const AVD_CL_CB *cb)
{
	SaAisErrorT rc;
	AvdJobDequeueResultT res;
	const SaImmOiHandleT immOiHandle = cb->immOiHandle;
	TRACE_ENTER2("Create %s", parentName_.c_str());

	const SaNameTWrapper parent_name(parentName_);
	//Check in AMF Db before creation in IMM.
	if (immobj_update_required() == false){
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
		goto done;	
	}
	rc = saImmOiRtObjectCreate_2(immOiHandle, className_,
				     parent_name, attrValues_);

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_EXIST)) {
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		res = JOB_ETRYAGAIN;
	} else {
		delete Fifo::dequeue();
		LOG_ER("%s: create FAILED %u", __FUNCTION__, rc);
		res = JOB_ERR;
	}
done:
	TRACE_LEAVE();
	
	return res;
}

//
ImmObjCreate::~ImmObjCreate()
{
	unsigned int i, j;

	for (i = 0; attrValues_[i] != nullptr; i++) {
		SaImmAttrValuesT_2 *attrValue =
			(SaImmAttrValuesT_2 *)attrValues_[i];
		
		if (attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			for (j = 0; j < attrValue->attrValuesNumber; j++) {
				char *p = *((char**) attrValue->attrValues[j]);
				delete [] p;
			}
		} else if (attrValue->attrValueType == SA_IMM_ATTR_SANAMET) {
			for (j = 0; j < attrValue->attrValuesNumber; j++) {
				SaNameT* name = reinterpret_cast<SaNameT*>(attrValue->attrValues[i]);
				osaf_extended_name_free(name);
			}
		}
		delete [] attrValue->attrName;
		delete [] static_cast<char*>(attrValue->attrValues[0]); // free blob shared by all values
		delete [] attrValue->attrValues;
		delete attrValue;
	}

	delete [] className_;
	delete [] attrValues_;
}

//
AvdJobDequeueResultT ImmObjUpdate::exec(const AVD_CL_CB *cb)
{
	SaAisErrorT rc;
	AvdJobDequeueResultT res;
	const SaImmOiHandleT immOiHandle = cb->immOiHandle;

	SaImmAttrModificationT_2 attrMod;
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, nullptr};
	SaImmAttrValueT attrValues[] = {value_};

	TRACE_ENTER2("Update '%s' %s", dn.c_str(), attributeName_);

	//update latest values.
	if (immobj_update_required() == false){
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
		goto done;	
	}
	attrMod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	attrMod.modAttr.attrName = attributeName_;
	attrMod.modAttr.attrValuesNumber = 1;
	attrMod.modAttr.attrValueType = attrValueType_;
	attrMod.modAttr.attrValues = attrValues;

	rc = saImmOiRtObjectUpdate_o3(immOiHandle, dn.c_str(), attrMods);

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_NOT_EXIST)) {
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		res = JOB_ETRYAGAIN;
	} else {
		delete Fifo::dequeue();
		LOG_ER("%s: update FAILED %u", __FUNCTION__, rc);
		res = JOB_ERR;
	}
done:
	TRACE_LEAVE();
	return res;
}
	
//
ImmObjUpdate::~ImmObjUpdate()
{
	if (attrValueType_ == SA_IMM_ATTR_SANAMET) {
		osaf_extended_name_free(static_cast<SaNameT*>(value_));
	}
	delete [] attributeName_;
	delete [] static_cast<char*>(value_);
}

//
AvdJobDequeueResultT ImmObjDelete::exec(const AVD_CL_CB *cb)
{
	SaAisErrorT rc;
	AvdJobDequeueResultT res;
	const SaImmOiHandleT immOiHandle = cb->immOiHandle;

	TRACE_ENTER2("Delete %s", dn.c_str());

	//Check AMF db before deletion.
	if (immobj_update_required() == false){
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
		goto done;	
	}
	rc = saImmOiRtObjectDelete_o3(immOiHandle, dn.c_str());

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_NOT_EXIST)) {
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		res = JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		res = JOB_ETRYAGAIN;
	} else {
		delete Fifo::dequeue();
		LOG_ER("%s: delete FAILED %u", __FUNCTION__, rc);
		res = JOB_ERR;
	}
done:	
	TRACE_LEAVE();
	return res;
}

ImmObjDelete::~ImmObjDelete()
{
}

//
AvdJobDequeueResultT ImmAdminResponse::exec(const AVD_CL_CB *cb) {
	SaAisErrorT rc;
	AvdJobDequeueResultT res;
	const SaImmOiHandleT handle = cb->immOiHandle;

	TRACE_ENTER2("Admin resp inv:%llu res:%u", invocation_, result_);

	rc = saImmOiAdminOperationResult(handle, invocation_, result_);

	switch (rc) {
	case SA_AIS_OK:
		saflog(LOG_NOTICE, amfSvcUsrName,
			"Admin op done for invocation: %llu, result %u",
			invocation_, result_);
		delete Fifo::dequeue();
		res = JOB_EXECUTED;
		break;
	case SA_AIS_ERR_TRY_AGAIN:
		TRACE("TRY-AGAIN");
		res = JOB_ETRYAGAIN;
		break;
	case SA_AIS_ERR_BAD_HANDLE:
		// there is no need to reattempt reply,
		// fall through to default to remove from queue
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
	default:
		LOG_ER("saImmOiAdminOperationResult failed with %u for admin op "
				"invocation: %llu, result %u", rc, invocation_, result_);
		delete Fifo::dequeue();
		res = JOB_ERR;
		break;
	}

	TRACE_LEAVE2("%u", res);
	return res;
}

Job* Fifo::peek()
{
	Job* tmp;
	
	if (job_.empty()) {
		tmp = 0;
	} else {
		tmp = job_.front();
	}
	
	return tmp;
}

//
void Fifo::queue(Job* job)
{
	job_.push(job);
}

//
Job* Fifo::dequeue()
{
	Job* tmp;
	
	if (job_.empty()) {
		tmp = 0;
	} else {
		tmp = job_.front();
		job_.pop();
	}
	
	return tmp;
}

/**
 * @brief   As of now standby AMFD will maintain immjobs for object of few classes.
 *          Flush all the jobs without updating to imm if MAX_JOB_SIZE_AT_STANDBY is 
 *	    reached. 
 *
 */
void check_and_flush_job_queue_standby_amfd(void)
{
        if (Fifo::size() >= MAX_JOB_SIZE_AT_STANDBY) {
		TRACE("Emptying imm job queue of size:%u",Fifo::size());
		Fifo::empty();
        }
}

//
AvdJobDequeueResultT Fifo::execute(const AVD_CL_CB *cb)
{
	Job *ajob;
	AvdJobDequeueResultT ret;

	// Keep in queue during controller switch-over
	if (!avd_cb->active_services_exist)
		return JOB_ETRYAGAIN;

	if ((!avd_cb->is_implementer) && (avd_cb->avail_state_avd == SA_AMF_HA_STANDBY)) {
		check_and_flush_job_queue_standby_amfd();
		return JOB_EINVH;
	}

	if ((ajob = peek()) == nullptr)
		return JOB_ENOTEXIST;

	TRACE_ENTER();

	ret = ajob->exec(cb);
	
	TRACE_LEAVE2("%d", ret);

	return ret;
}

//
void Fifo::empty()
{
	Job *ajob;

	TRACE_ENTER();

	while ((ajob = dequeue()) != nullptr) {
		delete ajob;
	}

	TRACE_LEAVE();
}

uint32_t Fifo::size()
{
       return job_.size();
}

//
std::queue<Job*> Fifo::job_;
//

extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

static const SaImmOiImplementerNameT implementerName =
	(SaImmOiImplementerNameT)"safAmfService";
static const SaImmOiImplementerNameT applierNamePrefix =
	(SaImmOiImplementerNameT)"@safAmfService";
static SaVersionT immVersion = { 'A', 2, 15 };

/* This string array must match the AVSV_AMF_CLASS_ID enum */
static const char *avd_class_names[] = {
	"Invalid",

	"SaAmfCompBaseType",
	"SaAmfSUBaseType",
	"SaAmfSGBaseType",
	"SaAmfAppBaseType",
	"SaAmfSvcBaseType",
	"SaAmfCSBaseType",
	"SaAmfCompGlobalAttributes",

	"SaAmfCompType",
	"SaAmfCSType",
	"SaAmfCtCsType",
	"SaAmfHealthcheckType",
	"SaAmfSvcType",
	"SaAmfSvcTypeCSTypes",
	"SaAmfSUType",
	"SaAmfSutCompType",
	"SaAmfSGType",
	"SaAmfAppType",

	"SaAmfCluster",
	"SaAmfNode",
	"SaAmfNodeGroup",
	"SaAmfNodeSwBundle",

	"SaAmfApplication",
	"SaAmfSG",
	"SaAmfSI",
	"SaAmfCSI",
	"SaAmfCSIAttribute",
	"SaAmfSU",
	"SaAmfComp",
	"SaAmfHealthcheck",
	"SaAmfCompCsType",
	"SaAmfSIDependency",
	"SaAmfSIRankedSU",

	"SaAmfSIAssignment",
	"SaAmfCSIAssignment"
};

static AvdImmOiCcbApplyCallbackT ccb_apply_callback[AVSV_SA_AMF_CLASS_MAX];
static AvdImmOiCcbCompletedCallbackT ccb_completed_callback[AVSV_SA_AMF_CLASS_MAX];
static SaImmOiAdminOperationCallbackT_2 admin_op_callback[AVSV_SA_AMF_CLASS_MAX];
static SaImmOiRtAttrUpdateCallbackT rtattr_update_callback[AVSV_SA_AMF_CLASS_MAX];

typedef struct avd_ccb_apply_ordered_list {
	AvdImmOiCcbApplyCallbackT ccb_apply_cb;
	CcbUtilOperationData_t *opdata;
	AVSV_AMF_CLASS_ID class_type; 
	struct avd_ccb_apply_ordered_list *next_ccb_to_apply;
} AvdCcbApplyOrderedListT;

static AvdCcbApplyOrderedListT *ccb_apply_list;

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

static size_t value_size(SaImmValueTypeT attrValueType)
{
	size_t valueSize = 0;

	switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		valueSize = sizeof(SaInt32T);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		valueSize = sizeof(SaUint32T);
		break;
	case SA_IMM_ATTR_SAINT64T:
		valueSize = sizeof(SaInt64T);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		valueSize = sizeof(SaUint64T);
		break;
	case SA_IMM_ATTR_SATIMET:
		valueSize = sizeof(SaTimeT);
		break;
	case SA_IMM_ATTR_SANAMET:
		valueSize = sizeof(SaNameT);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		valueSize = sizeof(SaFloatT);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		valueSize = sizeof(SaDoubleT);
		break;
	case SA_IMM_ATTR_SASTRINGT:
		valueSize = sizeof(SaStringT);
		break;
	case SA_IMM_ATTR_SAANYT:
		osafassert(0);
		break;
	}

	return valueSize;
}

static void copySaImmAttrValuesT(SaImmAttrValuesT_2 *copy, const SaImmAttrValuesT_2 *original)
{
	size_t valueSize = 0;
	unsigned int i, valueCount = original->attrValuesNumber;
	char *databuffer;

	copy->attrName = StrDup(original->attrName);

	copy->attrValuesNumber = valueCount;
	copy->attrValueType = original->attrValueType;
	if (valueCount == 0)
		return;		/* (just in case...) */

	copy->attrValues = new SaImmAttrValueT[valueCount];

	valueSize = value_size(original->attrValueType);

	// alloc blob shared by all values
	databuffer = new char[valueCount * valueSize];

	for (i = 0; i < valueCount; i++) {
		copy->attrValues[i] = databuffer;
		if (original->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			char *cporig = *((char **)original->attrValues[i]);
			char **cpp = (char **)databuffer;
			*cpp = StrDup(cporig);
		} else if ( original->attrValueType == SA_IMM_ATTR_SANAMET) {
			SaNameT* orig = reinterpret_cast<SaNameT*>(original->attrValues[i]);
			SaNameT* dest = reinterpret_cast<SaNameT*>(databuffer);
			osaf_extended_name_alloc(osaf_extended_name_borrow(orig),
				dest);
		} else {
			memcpy(databuffer, original->attrValues[i], valueSize);
		}
		databuffer += valueSize;
	}
}

static const SaImmAttrValuesT_2 *dupSaImmAttrValuesT(const SaImmAttrValuesT_2 *original)
{
	SaImmAttrValuesT_2 *copy = new SaImmAttrValuesT_2;

	copySaImmAttrValuesT(copy, original);
	return copy;
}

static const SaImmAttrValuesT_2 **dupSaImmAttrValuesT_array(const SaImmAttrValuesT_2 **original)
{
	const SaImmAttrValuesT_2 **copy;
	unsigned int i, alen = 0;

	if (original == nullptr)
		return nullptr;

	while (original[alen] != nullptr)
		alen++;

	copy = new const SaImmAttrValuesT_2*[alen + 1]();

	for (i = 0; i < alen; i++)
		copy[i] = dupSaImmAttrValuesT(original[i]);

	return copy;
}

static AVSV_AMF_CLASS_ID class_name_to_class_type(const std::string& className)
{
	int i;

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if (className.compare(avd_class_names[i]) == 0) {
			return static_cast<AVSV_AMF_CLASS_ID>(i);
		}
	}

	osafassert(0);
	return AVSV_SA_AMF_CLASS_INVALID;
}

/*****************************************************************************
 * Function: avd_class_type_find
 *
 * Purpose: This function returns class enum corresponding to Object name.
 *
 * Input: cb  - Object Name, Class type.
 *
 * Returns: OK/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static AVSV_AMF_CLASS_ID object_name_to_class_type(const std::string& obj_name)
{
	TRACE_ENTER2("%s", obj_name.c_str());
	AVSV_AMF_CLASS_ID class_type = AVSV_SA_AMF_CLASS_INVALID;

	const std::string::size_type first_equal = obj_name.find('=');
	const std::string prefix(obj_name.substr(0, first_equal));

	if (first_equal != std::string::npos) {
		// prefix contains all the chars before the first comma
		class_type = amf_type_map[prefix];
	} else {
		//For immadm -a safAmfService -o 99 safAmfService.
		if (obj_name.compare(implementerName) == 0 ||
		    obj_name.compare(0, strlen(applierNamePrefix), applierNamePrefix) == 0) {
			//return invalid class id for operation 99 to dump amf state.
			return class_type;
		}
		LOG_NO("unknown type: %s", obj_name.c_str());
		osafassert(false);
	}

	if (class_type == AVSV_SA_AMF_CLASS_INVALID) {
		// we weren't able to do a simple lookup
		if (prefix.compare("safSupportedCsType") == 0) {
			if (obj_name.find("safCompType=") != std::string::npos) {
				class_type = AVSV_SA_AMF_CT_CS_TYPE;
			} else if (obj_name.find("safComp=") != std::string::npos) {
				 class_type = AVSV_SA_AMF_COMP_CS_TYPE;
			}
		} else if (prefix.compare("safHealthcheckKey") == 0) {
			if (obj_name.find("safVersion=") != std::string::npos) {
				class_type = AVSV_SA_AMF_HEALTH_CHECK_TYPE;
			} else if (obj_name.find("safComp=") != std::string::npos) {
				class_type = AVSV_SA_AMF_HEALTH_CHECK;
			} 
		} else if (prefix.compare("safVersion") == 0) {
			// eg safVersion=4.0.0,safAppType=OpenSafApplicationType
			const std::string::size_type first_comma = obj_name.find(',');
			const std::string::size_type second_equal = obj_name.find('=', first_comma + 1);
			const std::string type(obj_name.substr(first_comma + 1,
				second_equal - first_comma - 1));
			
			TRACE("versioned type %s", type.c_str());
			
			class_type = versioned_types[type];
		}
	}
	TRACE_LEAVE2("%u", class_type);
	return class_type;
}

/*****************************************************************************
 * Function: avd_oi_admin_operation_cb
 *
 * Purpose: This function handles all admin operations.
 *
 * Input: cb  - Oi Handle, Invocation, Object Name, Operation Id, Parameters 
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void admin_operation_cb(SaImmOiHandleT immoi_handle,
	SaInvocationT invocation, const SaNameT *obj_name,
	SaImmAdminOperationIdT op_id, const SaImmAdminOperationParamsT_2 **params)
{
	const std::string object_name(Amf::to_string(obj_name));
	AVSV_AMF_CLASS_ID type = object_name_to_class_type(object_name);

 	TRACE_ENTER2("'%s', invocation: %llu, op: %llu", object_name.c_str(), invocation, op_id);

	if (object_name.compare(implementerName) == 0 ||
		object_name.compare(0, strlen(applierNamePrefix), applierNamePrefix) == 0) {
		// admin op targeted at the AMF implementer itself
 		if (op_id == 99) {
 	 		char *filename = nullptr;
 	 		if (params[0] != nullptr) {
 	 			const SaImmAdminOperationParamsT_2 *param = params[0];
 	 			if (param->paramType == SA_IMM_ATTR_SASTRINGT &&
 	 					strcmp(param->paramName, "filename") == 0)
 	 				filename = *((SaStringT *)param->paramBuffer);
 	 		}
 			int rc = amfd_file_dump(filename);
 			if (rc == 0)
 				(void) saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
 			else
 	 			report_admin_op_error(immoi_handle, invocation,
 	 				SA_AIS_ERR_INVALID_PARAM, nullptr,	"%s", strerror(rc));
 		} else
 			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_INVALID_PARAM, nullptr,
 				"Admin operation not supported for %s (%u)", object_name.c_str(), type);
 		goto done;
	}

	/* ignore admin ops if we are in the middle of a role switch */
	if (avd_cb->swap_switch == SA_TRUE) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
			"Admin op received during a role switch");
		goto done;
	}

	saflog(LOG_NOTICE, amfSvcUsrName, "Admin op \"%s\" initiated for '%s', invocation: %llu",
	       admin_op_name(static_cast<SaAmfAdminOperationIdT>(op_id)), object_name.c_str(), invocation);

	if (admin_op_callback[type] != nullptr) {
		if (admin_op_is_valid(op_id, type) == false) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"AMF (state %u) is not available for admin op'%llu' on '%s'",
					avd_cb->init_state, op_id, object_name.c_str());
			goto done;
		} else {
			admin_op_callback[type](immoi_handle, invocation, obj_name, op_id, params);
		}
	} else {
		LOG_ER("Admin operation not supported for %s (%u)", object_name.c_str(), type);
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_INVALID_PARAM, nullptr,
			"Admin operation not supported for %s (%u)", object_name.c_str(), type);
	}

done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_oi_rt_attr_update_cb
 *
 * Purpose: This function handles all Runtime attr update callbacks.
 *
 * Input: cb  - Oi Handle, Invocation, Object Name, Operation Id, Parameters 
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT rt_attr_update_cb(SaImmOiHandleT immoi_handle,
	const SaNameT *object_name, const SaImmAttrNameT *attribute_names)
{
	SaAisErrorT error;
	AVSV_AMF_CLASS_ID type = object_name_to_class_type(Amf::to_string(object_name));

	TRACE_ENTER2("%s", osaf_extended_name_borrow(object_name));
	osafassert(rtattr_update_callback[type] != nullptr);
	error = rtattr_update_callback[type](immoi_handle, object_name, attribute_names);
	TRACE_LEAVE2("%u", error);
	return error;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_create_cb
 *
 * Purpose: This function handles object create callback for all config classes.
 *          Its purpose is to memorize the request until the completed callback.
 *
 * Input: cb  - Oi Handle, Ccb Id, Class Name, Parent Name, Attributes. 
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_create_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaImmClassNameT class_name,
	const SaNameT *parent_name, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	CcbUtilCcbData_t *ccb_util_ccb_data;
	CcbUtilOperationData_t *operation;
	int i = 0;
	const SaImmAttrValuesT_2 *attrValue;
	AVSV_AMF_CLASS_ID id_from_class_name, id_from_dn;
	char err_str[] = "Ccb create failed, m/w role switch going on";

	TRACE_ENTER2("CCB ID %llu, class %s, parent '%s'", ccb_id, class_name, osaf_extended_name_borrow(parent_name));

	/* Reject adm ops if we are in the middle of a role switch. */
	if (avd_cb->swap_switch == SA_TRUE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_ER("CCB %llu validation error: %s", ccb_id, err_str);
		saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu validation error: %s",
				ccb_id, err_str);
		(void) saImmOiCcbSetErrorString(avd_cb->immOiHandle, ccb_id, err_str);
		goto done;
	}

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) == nullptr) {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	operation = ccbutil_ccbAddCreateOperation(ccb_util_ccb_data, class_name, parent_name, attr);

	if (operation == nullptr) {
		LOG_ER("Failed to get CCB operation object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Find the RDN attribute and store the object DN */
	while ((attrValue = attr[i++]) != nullptr) {
		if (!strncmp(attrValue->attrName, "saf", 3)) {
			std::string object_name;
			if (attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
				SaStringT rdnVal = *((SaStringT *)attrValue->attrValues[0]);
				if (parent_name != nullptr && osaf_extended_name_length(parent_name) > 0) {
					object_name = rdnVal;
					object_name += ",";
					object_name += Amf::to_string(parent_name);
				} else {
					object_name = rdnVal;
				}
			} else {
				SaNameT *rdnVal = ((SaNameT *)attrValue->attrValues[0]);
				object_name = Amf::to_string(rdnVal);
				object_name += ",";
				object_name += Amf::to_string(parent_name);
			}

			osaf_extended_name_alloc(object_name.c_str(), &operation->objectName);
			TRACE("%s(%zu)", object_name.c_str(), object_name.length());
			break;
		}
	}

	if (osaf_extended_name_length(&operation->objectName) == 0) {
		LOG_ER("Malformed DN %llu", ccb_id);
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verify that DN is valid for class */
	id_from_class_name = class_name_to_class_type(class_name);
	id_from_dn = object_name_to_class_type(Amf::to_string(&operation->objectName));
	if (id_from_class_name != id_from_dn) {
		LOG_ER("Illegal DN '%s' for class '%s'", osaf_extended_name_borrow(&operation->objectName), class_name);
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_delete_cb
 *
 * Purpose: This function handles object delete callback for all config classes
 *
 * Input: cb  - Oi Handle, Ccb Id, object name.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_delete_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaNameT *object_name)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccb_util_ccb_data;
	char err_str[] = "Ccb delete failed, m/w role switch going on";

	TRACE_ENTER2("CCB ID %llu, %s", ccb_id, osaf_extended_name_borrow(object_name));

	/* Reject adm ops if we are in the middle of a role switch. */
	if (avd_cb->swap_switch == SA_TRUE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu validation error: %s",
				ccb_id, err_str);
		(void) saImmOiCcbSetErrorString(avd_cb->immOiHandle, ccb_id,
				err_str);
		goto done;
	}

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != nullptr) {
		/* "memorize the request" */
		ccbutil_ccbAddDeleteOperation(ccb_util_ccb_data, object_name);
	} else {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
	}
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_modify_cb
 *
 * Purpose: This function handles object modify callback for config classes.
 *
 * Input: cb  - Oi Handle, Ccb Id, Class Name, Parent Name, Attributes.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_modify_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaNameT *object_name,
	const SaImmAttrModificationT_2 **attr_mods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccb_util_ccb_data;
	char err_str[] = "Ccb modify failed, m/w role switch going on";

	TRACE_ENTER2("CCB ID %llu, %s", ccb_id, osaf_extended_name_borrow(object_name));

	/* Reject adm ops if we are in the middle of a role switch. */
	if (avd_cb->swap_switch == SA_TRUE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu validation error: %s",
				ccb_id, err_str);
		(void) saImmOiCcbSetErrorString(avd_cb->immOiHandle, ccb_id,
				err_str);
		goto done;
        }

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != nullptr) {
		/* "memorize the request" */
		if (ccbutil_ccbAddModifyOperation(ccb_util_ccb_data, object_name, attr_mods) != 0) {
			LOG_ER("Failed '%s'", osaf_extended_name_borrow(object_name));
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
	} else {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
	}
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_completed_cb
 *
 * Purpose: This function handles completed callback for the corresponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_completed_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id)
{
	SaAisErrorT rc = SA_AIS_OK;
	CcbUtilOperationData_t *opdata = nullptr;
	AVSV_AMF_CLASS_ID type;

	TRACE_ENTER2("CCB ID %llu", ccb_id);



	/* "check that the sequence of change requests contained in the CCB is
	   valid and that no errors will be generated when these changes
	   are applied." */

	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != nullptr) {
		type = object_name_to_class_type(Amf::to_string(&opdata->objectName));

		/* Standby AMFD should process CCB completed callback only for CCB_DELETE. */
		if ((avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) &&
				(opdata->operationType != CCBUTIL_DELETE))
			continue;
		if (ccb_completed_callback[type] == nullptr) {
			/* this can happen for malformed DNs */
			LOG_ER("Class implementer for '%s' not found", osaf_extended_name_borrow(&opdata->objectName));
			goto done;
		}
		rc = ccb_completed_callback[type](opdata);

		/* Get out at first error */
		if (rc != SA_AIS_OK)
			break;
	}
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: ccb_abort_cb
 *
 * Purpose: This function handles abort callback for the corresponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_abort_cb(SaImmOiHandleT immoi_handle, SaImmOiCcbIdT ccb_id)
{
	CcbUtilCcbData_t *ccb_util_ccb_data;

	TRACE_ENTER2("CCB ID %llu", ccb_id);

	/* Return CCB container memory */
	ccb_util_ccb_data = ccbutil_findCcbData(ccb_id);
	/* ccb_util_ccb_data may be nullptr when first create/modify/delete cbk
	   was rejected before adding the ccb information in the data base.*/
	if (ccb_util_ccb_data) {
		ccbutil_deleteCcbData(ccb_util_ccb_data);
	}

	TRACE_LEAVE();
}
/*****************************************************************************
 * Function: ccb_insert_ordered_list
 *
 * Purpose: This function sorts the Ccb apply callbacks for the correponding 
 *          CCB operation in order of dependency/heirarchy
 *
 * Input: ccb_apply_cb - apply callback for specific object
 * 	  opdata - Ccb Operation data
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_insert_ordered_list(AvdImmOiCcbApplyCallbackT ccb_apply_cb,
		CcbUtilOperationData_t *opdata, AVSV_AMF_CLASS_ID type)
{
	AvdCcbApplyOrderedListT *temp = nullptr;
	AvdCcbApplyOrderedListT *prev = nullptr;
	AvdCcbApplyOrderedListT *next = nullptr;

	/* allocate memory */

	temp = new AvdCcbApplyOrderedListT;

	temp->ccb_apply_cb = ccb_apply_cb;
	temp->opdata = opdata;
	temp->class_type = type;
	temp->next_ccb_to_apply = nullptr;

	/* ccbs are sorted in top-down order in create/modify operations and
	 * sorted in bottom-up order for delete operation. All the ccbs are 
	 * appended to a single list in the order of create operations first
	 * then modify operations and lastly delete operations
	 */

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		next = ccb_apply_list;
		while (next != nullptr) {
			if((next->opdata->operationType != CCBUTIL_CREATE) ||
					(next->class_type > temp->class_type)) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != nullptr) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	case CCBUTIL_MODIFY:
		next = ccb_apply_list;

		/* traverse to the end of all the create CCBs */
		while (next && next->opdata->operationType == CCBUTIL_CREATE) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		while (next != nullptr) {
			if((next->opdata->operationType != CCBUTIL_MODIFY) ||
					(next->class_type > temp->class_type)) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != nullptr) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	case CCBUTIL_DELETE:
		next = ccb_apply_list;

		/* traverse to the end of all the create CCBs */
		while (next && next->opdata->operationType == CCBUTIL_CREATE) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		/* traverse to the end of all the modify CCBs */
		while (next && next->opdata->operationType == CCBUTIL_MODIFY) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		while (next != nullptr) {
			if(next->class_type < temp->class_type) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != nullptr) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	default:
		osafassert(0);
			break;
	}
}

/*****************************************************************************
 * Function: ccb_apply_cb
 *
 * Purpose: This function handles apply callback for the correponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_apply_cb(SaImmOiHandleT immoi_handle, SaImmOiCcbIdT ccb_id)
{
	CcbUtilCcbData_t *ccb_util_ccb_data;
	CcbUtilOperationData_t *opdata = nullptr;
	AVSV_AMF_CLASS_ID type;
	AvdCcbApplyOrderedListT *next = nullptr;
	AvdCcbApplyOrderedListT *temp = nullptr;

	TRACE_ENTER2("CCB ID %llu", ccb_id);

	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != nullptr) {
		const std::string object_name(Amf::to_string(&opdata->objectName));
		type = object_name_to_class_type(object_name);
		/* Base types will not have an apply callback, skip empty ones */
		if (ccb_apply_callback[type] != nullptr) {
			/* insert the apply callback into the sorted list
			 * to be applied later, after all the ccb apply 
			 * callback are sorted as required by the internal
			 * AMFD's information model
			 */
			ccb_insert_ordered_list(ccb_apply_callback[type], opdata, type);
		}

		// SAF LOG all changes on active
		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			switch (opdata->operationType) {
			case CCBUTIL_CREATE:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Created %s",
						ccb_id, object_name.c_str());
				break;
			case CCBUTIL_MODIFY:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Modified %s",
						ccb_id, object_name.c_str());
				break;
			case CCBUTIL_DELETE:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Deleted %s",
						ccb_id, object_name.c_str());
				break;
			default:
				osafassert(0);
			}
		}
	}

	/* First pass: apply all the CCBs in the sorted order */
	next = ccb_apply_list;
	while (next != nullptr) {
		next->ccb_apply_cb(next->opdata);
		temp = next;
		next = next->next_ccb_to_apply;
	}

	/* Second pass: adjust configuration after all model changes has been applied */
	next = ccb_apply_list;
	while (next != nullptr) {
		// TODO: would be more elegant with yet another function pointer
		type = object_name_to_class_type(Amf::to_string(&next->opdata->objectName));
		if ((type == AVSV_SA_AMF_SG) && (next->opdata->operationType == CCBUTIL_CREATE)) {
			AVD_SG *sg = sg_db->find(Amf::to_string(&next->opdata->objectName));
			avd_sg_adjust_config(sg);
		}
		next = next->next_ccb_to_apply;
	}

	/* Third pass: free allocated memory */
	next = ccb_apply_list;
	while (next != nullptr) {
		temp = next;
		next = next->next_ccb_to_apply;
		delete temp;
	}

	ccb_apply_list = nullptr;

	/* Return CCB container memory */
	ccb_util_ccb_data = ccbutil_findCcbData(ccb_id);
	osafassert(ccb_util_ccb_data);
	ccbutil_deleteCcbData(ccb_util_ccb_data);
	TRACE_LEAVE();
}

static const SaImmOiCallbacksT_2 avd_callbacks = {
	admin_operation_cb,
	ccb_abort_cb,
	ccb_apply_cb,
	ccb_completed_cb,
	ccb_object_create_cb,
	ccb_object_delete_cb,
	ccb_object_modify_cb,
	rt_attr_update_cb
};

/*****************************************************************************
 * Function: hydra_config_get
 *
 * Purpose: This function checks if Hydra configuration is enabled in IMM
 *          then set the corresponding value to scs_absence_max_duration variable in
 *          avd_cb.
 *
 * Input: None.
 *
 * Returns: SaAisErrorT
 *
 * NOTES: If IMM attribute fetching fails that means Hydra
 *        configuration is disabled.
 *
 **************************************************************************/
static SaAisErrorT hydra_config_get(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	const std::string dn = "opensafImm=opensafImm,safApp=safImmService";
	SaImmAttrNameT attrName = const_cast<SaImmAttrNameT>("scAbsenceAllowed");
	SaImmAttrNameT attributeNames[] = {attrName, nullptr};
	const SaUint32T *value = nullptr;

	TRACE_ENTER();

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);
	rc = immutil_saImmOmAccessorGet_o2(accessorHandle, dn.c_str(), attributeNames,
				(SaImmAttrValuesT_2 ***)&attributes);

	if (rc != SA_AIS_OK) {
		LOG_WA("saImmOmAccessorGet_2 FAILED %u for %s", rc, dn.c_str());
		goto done;
	}

	value = immutil_getUint32Attr(attributes, attrName, 0);
	if (value == nullptr) {
		LOG_WA("immutil_getUint32Attr FAILED for %s", dn.c_str());
		goto done;
	}

	avd_cb->scs_absence_max_duration = *value;

done:
	immutil_saImmOmAccessorFinalize(accessorHandle);
	LOG_IN("scs_absence_max_duration: %d", avd_cb->scs_absence_max_duration);

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*****************************************************************************
 * Function: avd_imm_init
 *
 * Purpose: This function Initialize the OI interface and get a selection 
 *          object.
 *
 * Input: cb  - AVD control block
 * 
 * Returns: Void pointer.
 * 
 * NOTES: None.
 * 
 **************************************************************************/

SaAisErrorT avd_imm_init(void *avd_cb)
{
	SaAisErrorT error = SA_AIS_OK;
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;

	TRACE_ENTER();

	/* Setup immutils profile once and for all */
	immutilWrapperProfile.errorsAreFatal = false;
	immutilWrapperProfile.retryInterval = 1000;
	immutilWrapperProfile.nTries = 180;


	if ((error = immutil_saImmOiInitialize_2(&cb->immOiHandle, &avd_callbacks, &immVersion)) != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize failed %u", error);
		goto done;
	}

	if ((error = immutil_saImmOmInitialize(&cb->immOmHandle, nullptr, &immVersion)) != SA_AIS_OK) {
		LOG_ER("saImmOmInitialize failed %u", error);
		goto done;
	}

	if ((error = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj)) != SA_AIS_OK) {
		LOG_ER("saImmOiSelectionObjectGet failed %u", error);
		goto done;
	}

	TRACE("Successfully initialized IMM");

done:
	TRACE_LEAVE();
	return error;
}

/*****************************************************************************
 * Function: avd_imm_impl_set
 *
 * Purpose: This task makes class implementer.
 *
 * Input: -
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
SaAisErrorT avd_imm_impl_set(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t i;

	TRACE_ENTER();

	if ((rc = immutil_saImmOiImplementerSet(avd_cb->immOiHandle, implementerName)) != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		return rc;
	}

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if ((nullptr != ccb_completed_callback[i]) &&
		    (rc = immutil_saImmOiClassImplementerSet(avd_cb->immOiHandle, avd_class_names[i])) != SA_AIS_OK) {

			LOG_ER("Impl Set Failed for %s, returned %d",	avd_class_names[i], rc);
			break;
		}
	}

	avd_cb->is_implementer = true;

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * @brief		This function becomes applier and sets the applier name
 *			for all AMF objects
 *
 * @param[in] 		Nothing
 *
 * @return 		SA_AIS_OK or error
 *
 **************************************************************************/
SaAisErrorT avd_imm_applier_set(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t i;
	char applier_name[SA_MAX_UNEXTENDED_NAME_LENGTH] = {0};

	TRACE_ENTER();
	snprintf(applier_name, SA_MAX_UNEXTENDED_NAME_LENGTH, "%s%x", applierNamePrefix, avd_cb->node_id_avd);

	if ((rc = immutil_saImmOiImplementerSet(avd_cb->immOiHandle, applier_name)) != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		return rc;
	}

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if ((nullptr != ccb_completed_callback[i]) &&
		    (rc = immutil_saImmOiClassImplementerSet(avd_cb->immOiHandle, avd_class_names[i])) != SA_AIS_OK) {

			LOG_ER("Impl Set Failed for %s, returned %d",	avd_class_names[i], rc);
			break;
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_imm_impl_set_task
 *
 * Purpose: This task makes object and class implementer.
 *
 * Input: cb  - AVD control block
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void *avd_imm_impl_set_task(void *_cb)
{
	if (avd_imm_impl_set() != SA_AIS_OK) {
		LOG_ER("exiting since avd_imm_impl_set failed");
		exit(EXIT_FAILURE);	// TODO ncs_reboot?
	}

	return nullptr;
}

/*****************************************************************************
 * Function: avd_imm_impl_set_task_create
 *
 * Purpose: This function spawns thread for setting object and class 
 *          implementer, non-blocking.
 *
 * Input: -
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
void avd_imm_impl_set_task_create(void)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avd_imm_impl_set_task, avd_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void avd_class_impl_set(const std::string& className,
	SaImmOiRtAttrUpdateCallbackT rtattr_cb, SaImmOiAdminOperationCallbackT_2 adminop_cb,
	AvdImmOiCcbCompletedCallbackT ccb_compl_cb, AvdImmOiCcbApplyCallbackT ccb_apply_cb)
{
	AVSV_AMF_CLASS_ID type = class_name_to_class_type(className);

	rtattr_update_callback[type] = rtattr_cb;
	admin_op_callback[type] = adminop_cb;
	osafassert(ccb_completed_callback[type] == nullptr);
	ccb_completed_callback[type] = ccb_compl_cb;
	ccb_apply_callback[type] = ccb_apply_cb;
}

SaAisErrorT avd_imm_default_OK_completed_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	/* Only create and delete operations are OK */
	if (opdata->operationType != CCBUTIL_MODIFY) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_BAD_OPERATION;
	}
}

unsigned int avd_imm_config_get(void)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	/*
	** Get types first since instances are dependent of them.
	**
	** Important to get the types in the right order since
	** certain validation will be done dependent on the
	** existence of another type.
	**
	** Name space validation is done per type, for instances it is done
	** by parsing the model top down. Objects "outside" the name space
	** will not be found and neglected... Downside is they will not be 
	** reported either. Off line tools can do this better.
	*/

	/* SaAmfCSType needed by validation of SaAmfCtCsType */
	if (avd_cstype_config_get() != SA_AIS_OK)
		goto done;

	/* SaAmfCompType indirectly needed by SaAmfSUType */
	if (avd_comptype_config_get() != SA_AIS_OK)
		goto done;

	/* SaAmfSUType needed by SaAmfSGType */
	if (avd_sutype_config_get() != SA_AIS_OK)
		goto done;

	/* SaAmfSGType needed by SaAmfAppType */
	if (avd_sgtype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_apptype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_svctype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_compglobalattrs_config_get() != SA_AIS_OK)
		goto done;

	if (avd_cluster_config_get() != SA_AIS_OK)
		goto done;

	if (avd_node_config_get() != SA_AIS_OK)
		goto done;

	if (avd_ng_config_get() != SA_AIS_OK)
		goto done;

	if (avd_app_config_get() != SA_AIS_OK)
		goto done;

	if (avd_sidep_config_get() != SA_AIS_OK)
		goto done;

	/* retrieve hydra configuration from IMM */
	if (hydra_config_get() != SA_AIS_OK)
		goto done;

	// SGs needs to adjust configuration once all instances have been added
	{
		for (std::map<std::string, AVD_SG*>::const_iterator it = sg_db->begin();
				it != sg_db->end(); it++) {
			AVD_SG *sg = it->second;
			avd_sg_adjust_config(sg);
		}
	}

	rc = NCSCC_RC_SUCCESS;

done:
	if (rc == NCSCC_RC_SUCCESS)
		TRACE("AMF Configuration successfully read from IMM");
	else
		LOG_ER("Failed to read configuration, AMF will not start");

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * IM object update, BLOCKING
 * @param dn
 * @param attributeName
 * @param attrValueType
 * @param value
 */
SaAisErrorT avd_saImmOiRtObjectUpdate_sync(const std::string& dn, SaImmAttrNameT attributeName,
		SaImmValueTypeT attrValueType, void *value,
		SaImmAttrModificationTypeT modifyType)
{
	SaAisErrorT rc;
	SaImmAttrModificationT_2 attrMod;
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, nullptr};
	SaImmAttrValueT attrValues[] = {value};

	TRACE_ENTER2("'%s' %s", dn.c_str(), attributeName);

	attrMod.modType = modifyType;
	attrMod.modAttr.attrName = attributeName;
	attrMod.modAttr.attrValuesNumber = 1;
	attrMod.modAttr.attrValueType = attrValueType;
	attrMod.modAttr.attrValues = attrValues;

	rc = saImmOiRtObjectUpdate_o3(avd_cb->immOiHandle, dn.c_str(), attrMods);
	if (rc != SA_AIS_OK) {
		LOG_WA("saImmOiRtObjectUpdate of '%s' %s failed with %u", 
			dn.c_str(), attributeName, rc);
	}
	return rc;
}

/**
 * @brief   As of now standby AMFD will maintain immjobs for object of few classes.
 *          This function checks if immjobs for this object can be maintained at standby. 
 *
 * @param  dn (ptr to SaNameT)
 * @return true/false
 */
bool check_to_create_immjob_at_standby_amfd(const std::string& dn)
{

	AVSV_AMF_CLASS_ID class_type = AVSV_SA_AMF_CLASS_INVALID;
	class_type = object_name_to_class_type(dn);
	/*
	 SI and CSI are config classes, so AMFD will not create any object for them
         in IMM. But for creation of runtime objects of classes SUSI and CSICOMP, parent
	 name of SI and CSI, respecitvely, will be given. However, for deletion of SUSI or
	 CSICOMP objects their own dn will be given. Because of this reason below check 
	 includes classType of CSI and SI also.
	*/
	if ((class_type == AVSV_SA_AMF_SU) || (class_type == AVSV_SA_AMF_COMP) ||
			(class_type == AVSV_SA_AMF_SI_ASSIGNMENT) ||
			(class_type == AVSV_SA_AMF_CSI_ASSIGNMENT) ||
			(class_type == AVSV_SA_AMF_SI) || 
			(class_type == AVSV_SA_AMF_CSI)) {
		TRACE("Class Type:%s",avd_class_names[class_type]);
		return true;
	}
	return false;
}

/**
 * Queue an IM object update to be executed later, NON BLOCKING
 * @param dn
 * @param attributeName
 * @param attrValueType
 * @param value
 */
void avd_saImmOiRtObjectUpdate(const std::string& dn, const std::string& attributeName,
	SaImmValueTypeT attrValueType, void *value)
{
	TRACE_ENTER2("'%s' %s", dn.c_str(), attributeName.c_str());
	
	size_t sz;

	if ((avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) &&
			(check_to_create_immjob_at_standby_amfd(dn) == false))
		return;

	ImmObjUpdate *ajob = new ImmObjUpdate;
	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		ajob->implementer = false;

	sz = value_size(attrValueType);

	ajob->dn = dn;
	ajob->attributeName_= StrDup(attributeName.c_str());
	ajob->attrValueType_ = attrValueType;
	ajob->value_ = new char[sz];

	if (attrValueType == SA_IMM_ATTR_SANAMET) {
		osaf_extended_name_alloc(osaf_extended_name_borrow(static_cast<SaNameT *>(value)),
			static_cast<SaNameT *>(ajob->value_));
	} else if (attrValueType == SA_IMM_ATTR_SASTRINGT) {
			delete [] static_cast<char*>(ajob->value_);
			ajob->value_ = StrDup(static_cast<SaStringT>(value));
	} else {
		memcpy(ajob->value_, value, sz);
	}
	Fifo::queue(ajob);
	
	TRACE_LEAVE();
}

/**
 * Queue an IM object create to be executed later, non blocking
 * @param className
 * @param parentName
 * @param attrValues
 */
void avd_saImmOiRtObjectCreate(const std::string& className,
	const std::string& parentName, const SaImmAttrValuesT_2 **attrValues)
{
	TRACE_ENTER2("%s %s", className.c_str(), parentName.c_str());

	if ((avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) &&
			(check_to_create_immjob_at_standby_amfd(parentName) == false))
		return;

	ImmObjCreate* ajob = new ImmObjCreate;
	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		ajob->implementer = false;

	ajob->className_ = StrDup(className.c_str());
	osafassert(ajob->className_ != nullptr);
	ajob->parentName_ = parentName;
	ajob->attrValues_ = dupSaImmAttrValuesT_array(attrValues);
	Fifo::queue(ajob);
	
	TRACE_LEAVE();
}

/**
 * Queue an IM object delete to be executed later, non blocking
 * @param dn
 */
void avd_saImmOiRtObjectDelete(const std::string& dn)
{
	TRACE_ENTER2("%s", dn.c_str());
	
	if ((avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) &&
			(check_to_create_immjob_at_standby_amfd(dn) == false))
		return;

	ImmObjDelete *ajob = new ImmObjDelete;
	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		ajob->implementer = false;

	ajob->dn = dn;
	Fifo::queue(ajob);
	
	TRACE_LEAVE();
}

/**
 * Update cached runtime attributes in IMM
 */
void avd_imm_update_runtime_attrs(void)
{
	/* Update SU Class runtime cached attributes. */
	for (std::map<std::string, AVD_SU*>::const_iterator it = su_db->begin();
			it != su_db->end(); it++) {
		AVD_SU *su = it->second;
		avd_saImmOiRtObjectUpdate(su->name, "saAmfSUPreInstantiable",
			SA_IMM_ATTR_SAUINT32T,  &su->saAmfSUPreInstantiable);

		SaNameT hosted_by_node;
		// we could use borrow, but seems safer this way in case the SU is moved?
		// hosted_by_node needs to be freed after the job is completed!
		osaf_extended_name_alloc(su->saAmfSUHostedByNode.c_str(), &hosted_by_node);
		avd_saImmOiRtObjectUpdate(su->name, "saAmfSUHostedByNode",
			SA_IMM_ATTR_SANAMET, (void*)&hosted_by_node);

		avd_saImmOiRtObjectUpdate(su->name, "saAmfSUPresenceState",
			SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPresenceState);

		avd_saImmOiRtObjectUpdate(su->name, "saAmfSUOperState",
			SA_IMM_ATTR_SAUINT32T, &su->saAmfSUOperState);

		avd_saImmOiRtObjectUpdate(su->name, "saAmfSUReadinessState",
			SA_IMM_ATTR_SAUINT32T, &su->saAmfSuReadinessState);

	}

	/* Update Component Class runtime cached attributes. */
	for (std::map<std::string, AVD_COMP*>::const_iterator it = comp_db->begin();
			it != comp_db->end(); it++) {
		AVD_COMP *comp  = it->second;
		avd_saImmOiRtObjectUpdate(Amf::to_string(&comp->comp_info.name),
			"saAmfCompReadinessState", SA_IMM_ATTR_SAUINT32T,
			&comp->saAmfCompReadinessState);

		avd_saImmOiRtObjectUpdate(Amf::to_string(&comp->comp_info.name),
			"saAmfCompOperState", SA_IMM_ATTR_SAUINT32T,
			&comp->saAmfCompOperState);

		avd_saImmOiRtObjectUpdate(Amf::to_string(&comp->comp_info.name),
			"saAmfCompPresenceState", SA_IMM_ATTR_SAUINT32T,
			&comp->saAmfCompPresenceState);

	}

	/* Update Node Class runtime cached attributes. */
	for (std::map<std::string, AVD_AVND *>::const_iterator it = node_name_db->begin();
			it != node_name_db->end(); it++) {
		AVD_AVND *node = it->second;
		avd_saImmOiRtObjectUpdate(node->name, "saAmfNodeOperState",
				SA_IMM_ATTR_SAUINT32T, &node->saAmfNodeOperState);
	}

	/* Update Node Class runtime cached attributes. */
	for (std::map<std::string, AVD_SI*>::const_iterator it = si_db->begin();
			it != si_db->end(); it++) {
		AVD_SI *si = it->second;
		avd_saImmOiRtObjectUpdate(si->name, "saAmfSIAssignmentState",
			SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAssignmentState);
	}
}

/**
 * Thread main to re-initialize as IMM OI.
 * @param _cb
 * 
 * @return void*
 */
static void *avd_imm_reinit_bg_thread(void *_cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_CL_CB *cb = (AVD_CL_CB *)_cb;
	AVD_EVT *evt;
	uint32_t status;

	struct timespec time = {0, 0 };
	uint32_t no_of_retries = 0;
	const uint32_t MAX_NO_RETRIES = immutilWrapperProfile.nTries;
	osaf_millis_to_timespec(immutilWrapperProfile.retryInterval, &time);

	TRACE_ENTER();
	osaf_mutex_lock_ordie(&imm_reinit_mutex);
	/* Send signal that imm_reinit_mutex has been taken. */
	osaf_mutex_lock_ordie(&imm_reinit_thread_startup_mutex);
	pthread_cond_signal(&imm_reinit_thread_startup_cond);
	osaf_mutex_unlock_ordie(&imm_reinit_thread_startup_mutex);

	immutilWrapperProfile.errorsAreFatal = 0;

	while (++no_of_retries < MAX_NO_RETRIES) {
		(void) saImmOiFinalize(avd_cb->immOiHandle);

		avd_cb->immOiHandle = 0;
		avd_cb->is_implementer = false;

		if ((rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &avd_callbacks, &immVersion)) != SA_AIS_OK) {
			LOG_ER("saImmOiInitialize failed %u", rc);
			osaf_mutex_unlock_ordie(&imm_reinit_mutex);
			exit(EXIT_FAILURE);
		}

		rc = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
		if (rc == SA_AIS_ERR_BAD_HANDLE) {
			osaf_nanosleep(&time);
			continue;
		} else if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiSelectionObjectGet failed %u", rc);
			osaf_mutex_unlock_ordie(&imm_reinit_mutex);
			exit(EXIT_FAILURE);
		}

		/* If this is the active server, become implementer again. */
		if (cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			rc = avd_imm_impl_set();
			if (rc == SA_AIS_ERR_BAD_HANDLE) {
				osaf_nanosleep(&time);
				continue;
			} else if (rc != SA_AIS_OK) {
				LOG_ER("exiting since avd_imm_impl_set failed");
				osaf_mutex_unlock_ordie(&imm_reinit_mutex);
				exit(EXIT_FAILURE);
			}
		} else {
			/* become applier and re-read the config */
			rc = avd_imm_applier_set();
			if (rc == SA_AIS_ERR_BAD_HANDLE) {
				osaf_nanosleep(&time);
				continue;
			} else if (rc != SA_AIS_OK) {
				LOG_ER("exiting since avd_imm_applier_set failed");
				osaf_mutex_unlock_ordie(&imm_reinit_mutex);
				exit(EXIT_FAILURE);
			}

			if (avd_imm_config_get() != NCSCC_RC_SUCCESS) {
				LOG_ER("avd_imm_config_get FAILED");
				osaf_mutex_unlock_ordie(&imm_reinit_mutex);
				exit(EXIT_FAILURE);
			}
		}
		break;
	}

	if (no_of_retries >= MAX_NO_RETRIES) {
		LOG_ER("Re-init with IMM FAILED");
		osaf_mutex_unlock_ordie(&imm_reinit_mutex);
		exit(EXIT_FAILURE);
	}

	/* Wake up the main thread so it discovers the new IMM handle. */
	evt = new AVD_EVT();

	evt->rcv_evt = AVD_IMM_REINITIALIZED;
	status = m_NCS_IPC_SEND(&avd_cb->avd_mbx, evt, NCS_IPC_PRIORITY_VERY_HIGH);
	osafassert(status == NCSCC_RC_SUCCESS);

	LOG_NO("Finished re-initializing with IMM");
	/* Release mutex taken.*/
	osaf_mutex_unlock_ordie(&imm_reinit_mutex);
	TRACE_LEAVE();
	return nullptr;
}

/**
 * Finalize the current IMM OI handle and start a thread to do re-initialization.
 */
void avd_imm_reinit_bg(void)
{
	pthread_t thread;
	pthread_attr_t attr;
	int rc = 0;

	TRACE_ENTER();

	LOG_NO("Re-initializing with IMM");

	osaf_mutex_lock_ordie(&imm_reinit_thread_startup_mutex);

	(void) saImmOiFinalize(avd_cb->immOiHandle);

	avd_cb->immOiHandle = 0;
	avd_cb->is_implementer = false;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avd_imm_reinit_bg_thread, avd_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		osaf_mutex_unlock_ordie(&imm_reinit_thread_startup_mutex);
		exit(EXIT_FAILURE);
	}

	rc = pthread_cond_wait(&imm_reinit_thread_startup_cond, &imm_reinit_thread_startup_mutex);
	if (rc != 0) osaf_abort(rc);
	osaf_mutex_unlock_ordie(&imm_reinit_thread_startup_mutex);

	pthread_attr_destroy(&attr);

	TRACE_LEAVE();
}

/**
 * Log to SAF LOG and respond admin op to IMM
 * @param immOiHandle
 * @param invocation
 * @param result
 */
void avd_saImmOiAdminOperationResult(SaImmOiHandleT immOiHandle,
									 SaInvocationT invocation,
									 SaAisErrorT result)
{
	TRACE_ENTER2("inv:%llu, res:%u", invocation, result);

	ImmAdminResponse *job = new ImmAdminResponse(invocation, result);
	Fifo::queue(job);

	TRACE_LEAVE();
}

/**
 * To be used in OI callbacks to report errors by setting an error string
 * Also writes the same error string using TRACE
 *
 * @param ccbId
 * @param format
 * @param ...
 */
void report_ccb_validation_error(const CcbUtilOperationData_t *opdata, const char *format, ...)
{
	char err_str[256];
	va_list ap;

	err_str[sizeof(err_str) - 1] = 0;
	va_start(ap, format);
	(void) vsnprintf(err_str, sizeof(err_str), format, ap);
	va_end(ap);

	if (opdata != nullptr) {
		TRACE("%s", err_str);
		saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu validation error: %s",
			   opdata->ccbId, err_str);
		(void) saImmOiCcbSetErrorString(avd_cb->immOiHandle, opdata->ccbId, err_str);
	} else {
		// errors found during initial configuration read
		LOG_WA("configuration validation error: %s", err_str);
	}
}

/**
 * Respond admin op to IMM
 * @param immOiHandle
 * @param invocation
 * @param result
 * @param pend_cbk
 * @param  format 
 * @param  ... 
 */
void report_admin_op_error(SaImmOiHandleT immOiHandle, SaInvocationT invocation, SaAisErrorT result,
		AVD_ADMIN_OPER_CBK *pend_cbk, const char *format, ...)
{
	char ao_err_string[256];
	SaAisErrorT error;
	SaStringT p_ao_err_string = ao_err_string;
	SaImmAdminOperationParamsT_2 ao_err_param = {
		const_cast<SaStringT>(SA_IMM_PARAM_ADMOP_ERROR),
		SA_IMM_ATTR_SASTRINGT,
		const_cast<SaStringT *>	(&p_ao_err_string)};
	const SaImmAdminOperationParamsT_2 *ao_err_params[2] = {
		&ao_err_param,
		nullptr };
	ao_err_string[sizeof(ao_err_string) - 1] = 0;

	va_list ap;
	va_start(ap, format);
	(void) vsnprintf(ao_err_string, sizeof(ao_err_string), format, ap);
	va_end(ap);
	TRACE_ENTER2("inv:%llu, res:%u, Error String: '%s'", invocation, result, ao_err_string);

	// log error string separately to maintain some backwards compatibility
	saflog(LOG_NOTICE, amfSvcUsrName,
			"Admin op invocation: %llu, err: '%s'", invocation, ao_err_string);

	saflog(LOG_NOTICE, amfSvcUsrName, "Admin op done for invocation: %llu, result %u",
		   invocation, result);

	error= saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, ao_err_params);
	if (error != SA_AIS_OK)
		LOG_ER("saImmOiAdminOperationResult_o2 for %llu failed %u", invocation, error);

	if (pend_cbk) {
		pend_cbk->admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
		pend_cbk->invocation = 0;
	}

}


/**
 * @brief  Fetches latest attribute values from AMF db before creating any runtime object in IMM. 
 *         Currently, standby AMFD creates runtime objects of only
 *         AVSV_SA_AMF_SI_ASSIGNMENT and AVSV_SA_AMF_CSI_ASSIGNMENT.
 * @Return true/false.
 */
bool ImmObjCreate::immobj_update_required() {
  
  bool ret = true;
  int i = 0;
  AVD_SU_SI_REL *susi = nullptr; 
  AVSV_AMF_CLASS_ID class_type = AVSV_SA_AMF_CLASS_INVALID;
  const SaImmAttrValuesT_2 *attribute = nullptr; 
  if (implementer == true)
    return true;
  TRACE_ENTER2("class type:'%s'", className_);

  class_type = class_name_to_class_type(className_);
  if ((class_type != AVSV_SA_AMF_SI_ASSIGNMENT) && 
    (class_type != AVSV_SA_AMF_CSI_ASSIGNMENT)) {
    TRACE_LEAVE2("false");
    return false;
  }
   
  if (class_type == AVSV_SA_AMF_SI_ASSIGNMENT) {
    std::string su_name;
    std::string sisu_name;
    std::string::size_type pos;
    while ((attribute = attrValues_[i++]) != nullptr) {
      if (!strcmp(attribute->attrName, "safSISU")) {
	sisu_name = Amf::to_string(reinterpret_cast<SaNameT*>(attribute->attrValues[0]));
	//at present sisu_name : safSISU=safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1.
	avsv_sanamet_init(sisu_name, su_name, "safSu=");
	su_name.erase(std::remove(su_name.begin(), su_name.end(), '\\'), su_name.end());
	TRACE_1("su_name:'%s'",su_name.c_str());
	break;
      }	
    }
    susi = avd_susi_find(avd_cb, su_name, parentName_);
    if (susi == nullptr) {
      TRACE_1("susi not found");
      ret = false;
      goto done;
    }
    i=0;
    while ((attribute = attrValues_[i++]) != nullptr) {
      if (!strcmp(attribute->attrName, "safSISU")) {
        continue;
      } else if (!strcmp(attribute->attrName, "saAmfSISUHAState")) {
	TRACE_1("saAmfSISUHAState old value:%u, new value:%u",
	  *static_cast<SaAmfHAStateT*>(attribute->attrValues[0]), susi->state);
	memcpy(attribute->attrValues[0], &susi->state, value_size(attribute->attrValueType));
      } else if (!strcmp(attribute->attrName, "osafAmfSISUFsmState")) {
	TRACE_1("osafAmfSISUFsmState old value:%u, new value:%u",
	  *static_cast<AVD_SU_SI_STATE*>(attribute->attrValues[0]), susi->fsm);
	memcpy(attribute->attrValues[0], &susi->fsm, value_size(attribute->attrValueType));
      }
    }
  } else if (class_type == AVSV_SA_AMF_CSI_ASSIGNMENT) {
    std::string comp_name;
    std::string csicomp_name;
    std::string::size_type pos;
    AVD_CSI *csi = nullptr;
    AVD_COMP *comp = nullptr;
    AVD_COMP_CSI_REL *compcsi = nullptr; 
    while ((attribute = attrValues_[i++]) != nullptr) {
      if (!strcmp(attribute->attrName, "safCSIComp")) {
	csicomp_name = Amf::to_string(reinterpret_cast<SaNameT*>(attribute->attrValues[0]));
        //at present csicomp_name:safCSIComp=safComp=AmfDemo\,safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1.
	avsv_sanamet_init(csicomp_name, comp_name, "safComp=");
	comp_name.erase(std::remove(comp_name.begin(), comp_name.end(), '\\'), comp_name.end());
	TRACE_1("comp_name:'%s'", comp_name.c_str());
	break;
      }	
    }
    csi =  csi_db->find(parentName_);
    comp = comp_db->find(comp_name);	
    if ((csi == nullptr) || (comp == nullptr)) {
      TRACE_1("comp or csi not found");
      ret = false;
      goto done;
    }
    susi = avd_susi_find(avd_cb, comp->su->name, csi->si->name);
    if (susi == nullptr) {
      TRACE_1("susi not found");
      ret = false;
      goto done;
    }
    for (compcsi = susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
      if ((compcsi->comp == comp) && (compcsi->csi == csi)) 
	break;
    }
    if (compcsi == nullptr) {
      ret = false;
      goto done;
    }
    i = 0;
    while ((attribute = attrValues_[i++]) != nullptr) {
      if (!strcmp(attribute->attrName, "safCSIComp")) {
        continue;
      } else if (!strcmp(attribute->attrName, "saAmfCSICompHAState")) {
	TRACE_1("saAmfCSICompHAState old value:%u, new value:%u",
	  *static_cast<SaAmfHAStateT*>(attribute->attrValues[0]), susi->state);
	memcpy(attribute->attrValues[0], &susi->state, value_size(attribute->attrValueType));
      }
    }
  }
done:
  TRACE_LEAVE2("ret:%u",ret);
  return ret;
}

//Imm Object atrribute update related member functions.
bool ImmObjUpdate::su_get_attr_value() {	
  bool ret = true;
  AVD_SU *su = nullptr;
  su = su_db->find(dn);
  if (su == nullptr) {
    TRACE_1("su not found");
    ret = false;
    goto done;
  }
  if (!strcmp(attributeName_, "saAmfSUOperState")) {
    TRACE_1("saAmfSUOperState old value:%u, new value:%u",
      *static_cast<SaAmfOperationalStateT*>(value_), su->saAmfSUOperState);
    memcpy(value_, &su->saAmfSUOperState, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "saAmfSUReadinessState")) {
    TRACE_1("saAmfSuReadinessState old value:%u, new value:%u",
      *static_cast<SaAmfReadinessStateT*>(value_), su->saAmfSuReadinessState);
    memcpy(value_, &su->saAmfSuReadinessState, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "saAmfSUPresenceState")) {
    TRACE_1("saAmfSUPresenceState old value:%u, new value:%u",
      *static_cast<SaAmfPresenceStateT*>(value_), su->saAmfSUPresenceState);
    memcpy(value_, &su->saAmfSUPresenceState, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "saAmfSUAdminState")) {
    TRACE_1("saAmfSUAdminState old value:%u, new value:%u",
      *static_cast<SaAmfAdminStateT*>(value_), su->saAmfSUAdminState);
    memcpy(value_, &su->saAmfSUAdminState, value_size(attrValueType_));
  } else {
    //Other attributes not considered.
    ret = false;
  }
done:
  return ret;
}
bool ImmObjUpdate::siass_get_attr_value() {	
  std::string su_name;
  std::string si_name;
  //dn : safSISU=safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safSi=AmfDemo,safApp=AmfDemo1
  avd_association_namet_init(dn, su_name, si_name, AVSV_SA_AMF_SI);
  AVD_SU_SI_REL *susi = avd_susi_find(avd_cb, su_name, si_name);
  if (susi == nullptr) {
    TRACE_1("SUSI not found");
    return false;
  } 
  if (!strcmp(attributeName_, "saAmfSISUHAState")) {
    TRACE_1("saAmfSISUHAState old value:%u, new value:%u",
      *static_cast<SaAmfHAStateT*>(value_), susi->state);
    memcpy(value_, &susi->state, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "osafAmfSISUFsmState")) {
    TRACE_1("osafAmfSISUFsmState old value:%u, new value:%u",
      *static_cast<AVD_SU_SI_STATE*>(value_), susi->fsm);
    memcpy(value_, &susi->fsm, value_size(attrValueType_));
  } else
	return false;
  return true;
}
bool ImmObjUpdate::csiass_get_attr_value() {
  std::string comp_name;
  std::string csi_name;
  AVD_CSI *csi = nullptr;
  AVD_COMP *comp = nullptr;
  AVD_COMP_CSI_REL *compcsi = nullptr;
  //dn:safCSIComp=safComp=AmfDemo\,safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safCsi=AmfDemo,safSi=AmfDemo,safApp=AmfDemo1
  avd_association_namet_init(dn, comp_name, csi_name, AVSV_SA_AMF_CSI);
  csi =  csi_db->find(csi_name);
  comp = comp_db->find(comp_name);
  if ((csi == nullptr) || (comp == nullptr)) {
    TRACE_1("comp or csi not found");
    return false;
  }
  AVD_SU_SI_REL *susi = avd_susi_find(avd_cb, comp->su->name, csi->si->name);
  if (susi == nullptr) {
    TRACE_1("susi not found");
    return false;
  }
  for (compcsi = susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
    if ((compcsi->comp == comp) && (compcsi->csi == csi))
      break;
  }
  if (compcsi == nullptr) {
    return false;
  }
  if (!strcmp(attributeName_, "saAmfCSICompHAState")) {
    TRACE_1("saAmfCSICompHAState old value:%u, new value:%u",
      *static_cast<SaAmfHAStateT*>(value_), susi->state);
    memcpy(value_, &susi->state, value_size(attrValueType_));
  } else
	return false;

  return true;
}
bool ImmObjUpdate::comp_get_attr_value() {
  AVD_COMP *comp = nullptr;
  comp = comp_db->find(dn);	
  if (comp == nullptr) {
    TRACE_1("comp not found");
    return false;
  }
  if (!strcmp(attributeName_, "saAmfCompOperState")) {
    TRACE_1("saAmfCompOperState old value:%u, new value:%u",
      *static_cast<SaAmfOperationalStateT*>(value_), comp->saAmfCompOperState);
    memcpy(value_, &comp->saAmfCompOperState, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "saAmfCompReadinessState")) {
    TRACE_1("saAmfCompReadinessState old value:%u, new value:%u",
      *static_cast<SaAmfReadinessStateT*>(value_), comp->saAmfCompReadinessState);
    memcpy(value_, &comp->saAmfCompReadinessState, value_size(attrValueType_));
  } else if (!strcmp(attributeName_, "saAmfCompPresenceState")) {
    TRACE_1(" saAmfCompPresenceState old value:%u, new value:%u",
      *static_cast<SaAmfPresenceStateT*>(value_), comp->saAmfCompPresenceState);
    memcpy(value_, &comp->saAmfCompPresenceState, value_size(attrValueType_));
  } else {
    //Other attributes not considered.
    return false;
  }
  return true;
}
bool ImmObjUpdate::si_get_attr_value() {
  TRACE("si:'%s'",dn.c_str());
  AVD_SI *si = si_db->find(dn);
  if (si == nullptr) {
    TRACE_1("si not found");
    return false;
  }
  if (!strcmp(attributeName_, "saAmfSIAssignmentState")) {
    TRACE_1("saAmfSIAssignmentState old value:%u, new value:%u",
      *static_cast<SaAmfAssignmentStateT*>(value_), si->saAmfSIAssignmentState);
    memcpy(value_, &si->saAmfSIAssignmentState, value_size(attrValueType_));
  }
  return true;
}
/**
 * @brief  Fetches latest value of attribute from AMF db before updating it in IMM.
 *         Currently, only for SU, Comp, SI, SISU and CSICOMP.
 * @Return true/false.
 */
bool ImmObjUpdate::immobj_update_required() {
  bool ret = true;
  AVSV_AMF_CLASS_ID class_type = object_name_to_class_type(dn);
  if (implementer == true)
    return true;
  TRACE_ENTER2("class type:'%s'", avd_class_names[class_type] );
   
  if (class_type == AVSV_SA_AMF_SI) {
    ret = si_get_attr_value();
  } else if (class_type == AVSV_SA_AMF_COMP) {
    ret = comp_get_attr_value();
  } else if (class_type == AVSV_SA_AMF_SU) {
    ret = su_get_attr_value();
  } else if (class_type == AVSV_SA_AMF_SI_ASSIGNMENT) {
    ret = siass_get_attr_value();	
  } else if (class_type == AVSV_SA_AMF_CSI_ASSIGNMENT){
    ret = csiass_get_attr_value();	
  } else {
    TRACE_1("Class not considered");
    ret = false;
  }
  TRACE_LEAVE2("ret:%u",ret);
  return ret;
}


//Imm Object delete related member functions.
bool ImmObjDelete::is_siass_exist() { 
  std::string su_name;
  std::string si_name;
  //dn : safSISU=safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safSi=AmfDemo,safApp=AmfDemo1
  avd_association_namet_init(dn, su_name, si_name, AVSV_SA_AMF_SI);
  TRACE("si:%s",si_name.c_str());
  TRACE("su:%s",su_name.c_str());
  AVD_SU_SI_REL *susi = avd_susi_find(avd_cb, su_name, si_name);
  //SUSI should not be present in AMF DB.
  if (susi != nullptr) {
    TRACE_1("SUSI found");
    return false;
  } else {
    TRACE_1("SUSI not found");
  }
  return true;
}
bool ImmObjDelete::is_csiass_exist() { 
  bool ret = true;
  std::string comp_name;
  std::string csi_name;
  AVD_CSI *csi = nullptr;
  AVD_COMP *comp = nullptr;
  AVD_COMP_CSI_REL *compcsi = nullptr; 
  AVD_SU_SI_REL *susi = nullptr;
  //dn:safCSIComp=safComp=AmfDemo\,safSu=SU1\,safSg=AmfDemo\,safApp=AmfDemo1,safCsi=AmfDemo,safSi=AmfDemo,safApp=AmfDemo1
  TRACE("csiass:%s",dn.c_str());
  avd_association_namet_init(dn, comp_name, csi_name, AVSV_SA_AMF_CSI);
  TRACE("csi:%s",csi_name.c_str());
  TRACE("comp:%s",comp_name.c_str());
  csi =  csi_db->find(csi_name);
  comp = comp_db->find(comp_name);	
  if ((csi == nullptr) || (comp == nullptr)) {
    TRACE_1("comp or csi not found");
    goto done;
  }
  susi = avd_susi_find(avd_cb, comp->su->name, csi->si->name);
  if (susi != nullptr) {
    TRACE_1("susi found");
    for (compcsi = susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
      if ((compcsi->comp == comp) && (compcsi->csi == csi)) 
	break;
    }
    if (compcsi != nullptr) {
      ret = false;
      goto done;	
    }
  } else {
    TRACE_1("SUSI not found");
  }
  return true;
done:
  return ret;
}

/*
 * @brief  Checks existence of object in AMF db before deleting it from IMM.
 *         Currently checks only for SISU and CSICOMP.
 * @Return true/false.
 */
bool ImmObjDelete::immobj_update_required() { 
  bool ret = true;
  AVSV_AMF_CLASS_ID class_type = object_name_to_class_type(dn);
  if (implementer == true)
    return true;
  TRACE_ENTER2("class type:'%s'", avd_class_names[class_type] );
  if (class_type == AVSV_SA_AMF_SI_ASSIGNMENT) {
    ret = is_siass_exist();
  } else if (class_type == AVSV_SA_AMF_CSI_ASSIGNMENT) {
    ret = is_csiass_exist();
  } else {
    TRACE_1("Class not considered");
    ret = false;
  }
  TRACE_LEAVE2("ret:%u",ret);
  return ret;
}

