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

#ifndef IMMSV_EVT_MODEL_H
#define IMMSV_EVT_MODEL_H
#include "saImm.h"
#include "saImmOm.h"
#include "saImmOi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BUGBUG ABT types converted from ASN1 to OpenSaf (transport over MDS). */

	typedef struct immsv_octet_string {
		SaUint32T size;	/* Size of the buffer */
		char *buf;	/* Buffer with consecutive OCTET_STRING bits */
	} IMMSV_OCTET_STRING;

	/* BUGBUG ABT remove the EDU/edu from the name! */

	typedef struct immsv_edu_attr_val {
		union {
			SaInt32T saint32;
			SaUint32T sauint32;
			SaInt64T saint64;
			SaUint64T sauint64;
			SaTimeT satime;
			SaFloatT safloat;
			SaDoubleT sadouble;
			IMMSV_OCTET_STRING x;	//SaNameT, SaStringT, SaAnyT.
		} val;
	} IMMSV_EDU_ATTR_VAL;

	typedef struct immsv_edu_attr_val_list {
		IMMSV_EDU_ATTR_VAL n;
		struct immsv_edu_attr_val_list *next;
	} IMMSV_EDU_ATTR_VAL_LIST;

	typedef struct ImmsvAttrValues {
		IMMSV_OCTET_STRING attrName;	//Should perhaps be a char* ??
		long attrValuesNumber;	//If zero => NO VALUE !!!
		IMMSV_EDU_ATTR_VAL attrValue;
		long attrValueType;
		IMMSV_EDU_ATTR_VAL_LIST *attrMoreValues;
	} IMMSV_ATTR_VALUES;

	typedef struct immsv_attr_values_list {
		IMMSV_ATTR_VALUES n;
		struct immsv_attr_values_list *next;
	} IMMSV_ATTR_VALUES_LIST;

	typedef struct immsv_attr_mods_list {
		SaUint32T attrModType;
		IMMSV_ATTR_VALUES attrValue;
		struct immsv_attr_mods_list *next;
	} IMMSV_ATTR_MODS_LIST;

	typedef struct ImmsvOmRspSearchNext {
		IMMSV_OCTET_STRING objectName;
		IMMSV_ATTR_VALUES_LIST *attrValuesList;
	} IMMSV_OM_RSP_SEARCH_NEXT;

	typedef struct ImmsvOmRspSearchRemote {
		SaAisErrorT result;
		SaUint32T requestNodeId;
		SaUint32T remoteNodeId;
		SaUint32T searchId;
		IMMSV_OM_RSP_SEARCH_NEXT runtimeAttrs;
	} IMMSV_OM_RSP_SEARCH_REMOTE;

	typedef struct ImmsvAdminOperationParam {
		IMMSV_OCTET_STRING paramName;
		long paramType;
		IMMSV_EDU_ATTR_VAL paramBuffer;
		struct ImmsvAdminOperationParam *next;
	} IMMSV_ADMIN_OPERATION_PARAM;

	typedef struct ImmsvOmAdminOperationInvoke {
		SaUint32T adminOwnerId;
		SaInt32T invocation;	//Negative => asyncronous admop
		SaUint64T operationId;
		SaUint64T continuationId;
		SaTimeT timeout;
		IMMSV_OCTET_STRING objectName;
		IMMSV_ADMIN_OPERATION_PARAM *params;
	} IMMSV_OM_ADMIN_OP_INVOKE;

	typedef struct ImmsvAttrDefinition {
		IMMSV_OCTET_STRING attrName;
		SaUint32T attrValueType;
		SaUint64T attrFlags;
		SaUint32T attrNtfId;
		IMMSV_EDU_ATTR_VAL *attrDefaultValue;	//CHANGED !!!
	} IMMSV_ATTR_DEFINITION;

	typedef struct ImmsvAttrDefList {
		IMMSV_ATTR_DEFINITION d;
		struct ImmsvAttrDefList *next;
	} IMMSV_ATTR_DEF_LIST;

	typedef struct ImmsvOmClassDescr {
		IMMSV_OCTET_STRING className;
		SaUint32T classCategory;
		IMMSV_ATTR_DEF_LIST *attrDefinitions;
	} IMMSV_OM_CLASS_DESCR;

	typedef struct ImmsvOmAdminOwnerInitialize {
		SaNameT adminOwnerName;
		SaBoolT releaseOwnershipOnFinalize;
	} IMMSV_OM_ADMIN_OWNER_INITIALIZE;

	typedef struct ImmsvOmCcbInitialize {
		SaUint32T adminOwnerId;
		SaUint64T ccbFlags;
		SaImmHandleT client_hdl;	//odd to put client_hdl here..
	} IMMSV_OM_CCB_INITIALIZE;

	typedef struct ImmsvOmCcbObjectCreate {
		SaUint32T ccbId;
		SaUint32T adminOwnerId;
		IMMSV_OCTET_STRING className;
		IMMSV_OCTET_STRING parentName;
		IMMSV_ATTR_VALUES_LIST *attrValues;
		SaUint64T immHandle;	//only used for the ND->A up-call (use seprt msg?)
	} IMMSV_OM_CCB_OBJECT_CREATE;

	typedef struct ImmsvOmCcbObjectModify {
		SaUint32T ccbId;
		SaUint32T adminOwnerId;
		IMMSV_OCTET_STRING objectName;
		IMMSV_ATTR_MODS_LIST *attrMods;
		SaUint64T immHandle;	//Used for the ND->A up-call in OM(use seprt msg?)
		/* Used for first hop A->ND in OiRtUpdate. */
	} IMMSV_OM_CCB_OBJECT_MODIFY;

	typedef struct ImmsvOmCcbObjectDelete {
		SaUint32T ccbId;
		SaUint32T adminOwnerId;	//Rename? used for both adminOwnerId & continuationId
		IMMSV_OCTET_STRING objectName;
		SaUint64T immHandle;	//only used for the ND->A up-call
	} IMMSV_OM_CCB_OBJECT_DELETE;

	typedef struct ImmsvOmCcbCompleted {
		SaUint32T ccbId;
		SaUint32T implId;
		SaUint32T invocation;
		SaUint64T immHandle;	//only used for the ND->A up-call
	} IMMSV_OM_CCB_COMPLETED;

	typedef struct ImmsvOmSearchOneAttr {
		IMMSV_OCTET_STRING attrName;
		SaUint32T attrValueType;
		IMMSV_EDU_ATTR_VAL attrValue;
	} IMMSV_OM_SEARCH_ONE_ATTR;

	typedef enum {
		ImmOmSearchParameter_PR_NOTHING = 1,	/* No components present */
		ImmOmSearchParameter_PR_oneAttrParam = 2,
	} ImmsvOmSearchParm_PR;

	typedef struct ImmsvSearchParam {
		ImmsvOmSearchParm_PR present;
		union ImmsvOmSearchParameter_u {
			IMMSV_OM_SEARCH_ONE_ATTR oneAttrParam;
		} choice;
	} IMMSV_SEARCH_PARAM;

	typedef struct ImmsvAttrNameList {
		IMMSV_OCTET_STRING name;
		struct ImmsvAttrNameList *next;
	} IMMSV_ATTR_NAME_LIST;

	typedef struct ImmsvOmSearchInit {
		SaImmHandleT client_hdl;
		IMMSV_OCTET_STRING rootName;
		SaUint32T scope;
		SaUint64T searchOptions;
		IMMSV_SEARCH_PARAM searchParam;
		IMMSV_ATTR_NAME_LIST *attributeNames;
	} IMMSV_OM_SEARCH_INIT;

	typedef struct ImmsvOmSearchRemote {
		SaImmHandleT client_hdl;
		SaUint32T requestNodeId;
		SaUint32T remoteNodeId;
		SaUint32T searchId;
		IMMSV_OCTET_STRING objectName;
		IMMSV_ATTR_NAME_LIST *attributeNames;
	} IMMSV_OM_SEARCH_REMOTE;

	typedef struct ImmsvOmObjectSync {
		IMMSV_OCTET_STRING className;
		IMMSV_OCTET_STRING objectName;
		IMMSV_ATTR_VALUES_LIST *attrValues;
	} IMMSV_OM_OBJECT_SYNC;

	typedef struct ImmsvObjNameList {
		IMMSV_OCTET_STRING name;
		struct ImmsvObjNameList *next;
	} IMMSV_OBJ_NAME_LIST;

	typedef struct ImmsvAdmoList {
		SaUint32T id;
		SaUint32T nodeId;
		IMMSV_OCTET_STRING adminOwnerName;
		SaBoolT releaseOnFinalize;
		SaBoolT isDying;
		IMMSV_OBJ_NAME_LIST *touchedObjects;
		struct ImmsvAdmoList *next;
	} IMMSV_ADMO_LIST;

	typedef struct ImmsvImplList {
		SaUint32T id;
		SaUint32T nodeId;
		IMMSV_OCTET_STRING implementerName;
		SaUint64T mds_dest;
		struct ImmsvImplList *next;
	} IMMSV_IMPL_LIST;

	typedef struct ImmsvClassList {
		IMMSV_OCTET_STRING className;
		IMMSV_OCTET_STRING classImplName;
		SaUint32T nrofInstances;
		struct ImmsvClassList *next;
	} IMMSV_CLASS_LIST;

	typedef struct ImmsvCcbOutcomeList {
		SaUint32T ccbId;
		SaUint32T ccbState;
		struct ImmsvCcbOutcomeList *next;
	} IMMSV_CCB_OUTCOME_LIST;

	typedef struct ImmsvOmFinalizeSync {
		SaUint32T lastContinuationId;
		IMMSV_ADMO_LIST *adminOwners;
		IMMSV_IMPL_LIST *implementers;
		IMMSV_CLASS_LIST *classes;
		IMMSV_CCB_OUTCOME_LIST *ccbResults;
	} IMMSV_OM_FINALIZE_SYNC;

	typedef struct ImmsvOiImplSetReq	//used for both implSet & classImplSet
	{
		SaImmOiHandleT client_hdl;
		IMMSV_OCTET_STRING impl_name;	/*and className and objName */
		SaUint32T impl_id;
		SaUint32T scope;	/*Only for obj impl set/rel */
	} IMMSV_OI_IMPLSET_REQ;

	typedef struct immsv_oi_ccb_upcall_rsp {
		SaImmOiHandleT oi_client_hdl;	//needed ?? does not seem like it
		SaUint32T ccbId;
		SaUint32T implId;
		SaUint32T inv;
		SaAisErrorT result;
		SaNameT name;
	} IMMSV_OI_CCB_UPCALL_RSP;

#ifdef __cplusplus
}
#endif

#endif
