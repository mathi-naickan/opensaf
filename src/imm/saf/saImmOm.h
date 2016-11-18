/******************************************************************************
**
** FILE:
**   saImmOm.h
**
** DESCRIPTION:
**   This file provides the C language binding for the Service
**   Availability(TM) Forum Information Model Management Service (IMM).
**   It contains all the prototypes and type definitions required
**   by the IMM Object Management APIs.
**
** SPECIFICATION VERSION:
**   SAI-AIS-IMM-A.02.01
**
** DATE:
**   Thurs  June   28  2007
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS.
**   The Specification and all worldwide copyrights therein are
**   the exclusive property of Licensor.  You may not remove, obscure, or
**   alter any copyright or other proprietary rights notices that are in or
**   on the copy of the Specification you download.  You must reproduce all
**   such notices on all copies of the Specification you make.  Licensor
**   may make changes to the Specification, or to items referenced therein,
**   at any time without notice.  Licensor is not obligated to support or
**   update the Specification.
**
**   Copyright(c) 2007, Service Availability(TM) Forum. All rights
**   reserved.
**
******************************************************************************/

#ifndef _SA_IMM_OM_H
#define _SA_IMM_OM_H

#include <saImm.h>

#ifdef  __cplusplus
extern "C" {
#endif

	/* 4.2.1 Handles Used by the IMM Service */
	typedef SaUint64T SaImmHandleT;
	typedef SaUint64T SaImmAdminOwnerHandleT;
	typedef SaUint64T SaImmCcbHandleT;
	typedef SaUint64T SaImmSearchHandleT;
	typedef SaUint64T SaImmAccessorHandleT;

	/* 4.2.18 SaImmCallbacksT */

	/* From 4.9.2 */
	typedef void
	 (*SaImmOmAdminOperationInvokeCallbackT) (SaInvocationT invocation,
						  SaAisErrorT operationReturnValue, SaAisErrorT error);

	typedef struct {
		SaImmOmAdminOperationInvokeCallbackT
		 saImmOmAdminOperationInvokeCallback;
	} SaImmCallbacksT;

	/* 4.2.19 IMM Service Object Attributes */

#define SA_IMM_ATTR_CLASS_NAME       "SaImmAttrClassName"
#define SA_IMM_ATTR_ADMIN_OWNER_NAME "SaImmAttrAdminOwnerName"
#define SA_IMM_ATTR_IMPLEMENTER_NAME "SaImmAttrImplementerName"

	/* 4.2.20 SaImmRepositoryInitModeT */
	typedef enum {
		SA_IMM_KEEP_REPOSITORY = 1,
		SA_IMM_INIT_FROM_FILE = 2
	} SaImmRepositoryInitModeT;

	/*
	 *************************
	 *                       *
	 *   Om Function Calls   *
	 *                       *
	 *************************
	 */

	/* 4.3.1 saImmOmInitialize() */

	extern SaAisErrorT
	 saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, SaVersionT *version);

	/* 4.3.2 saImmOmSelectionObjectGet() */

	extern SaAisErrorT
	 saImmOmSelectionObjectGet(SaImmHandleT immHandle, SaSelectionObjectT *selectionObject);

	/* 4.3.3 saImmOmDispatch() */

	extern SaAisErrorT
	 saImmOmDispatch(SaImmHandleT immHandle, SaDispatchFlagsT dispatchFlags);

	/* 4.3.4 saImmOmFinalize() */

	extern SaAisErrorT
	 saImmOmFinalize(SaImmHandleT immHandle);

	/* 4.4.1 saImmOmClassCreate() */
	extern SaAisErrorT
	 saImmOmClassCreate_2(SaImmHandleT immHandle,
			      const SaImmClassNameT className,
			      SaImmClassCategoryT classCategory, const SaImmAttrDefinitionT_2 **attrDefinitions);

	/* 4.4.2 saImmOmClassDescriptionGet() */
	extern SaAisErrorT
	 saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
				      const SaImmClassNameT className,
				      SaImmClassCategoryT *classCategory, SaImmAttrDefinitionT_2 ***attrDefinitions);

	/* 4.4.3 saImmOmClassDescriptionMemoryFree() */
	extern SaAisErrorT
	 saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinitions);

	/* 4.4.4 saImmOmClassDelete() */

	extern SaAisErrorT
	 saImmOmClassDelete(SaImmHandleT immHandle, const SaImmClassNameT className);

	/* 4.5.1 saImmOmSearchInitialize() */
	extern SaAisErrorT
	 saImmOmSearchInitialize_2(SaImmHandleT immHandle,
				   const SaNameT *rootName,
				   SaImmScopeT scope,
				   SaImmSearchOptionsT searchOptions,
				   const SaImmSearchParametersT_2 *searchParam,
				   const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle);

	/* 4.5.2 saImmOmSearchNext() */
	extern SaAisErrorT
	 saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes);

	/* 4.5.3 saImmOmSearchFinalize() */

	extern SaAisErrorT
	 saImmOmSearchFinalize(SaImmSearchHandleT searchHandle);

	/* 4.6.1 saImmOmAccessorInitialize() */

	extern SaAisErrorT
	 saImmOmAccessorInitialize(SaImmHandleT immHandle, SaImmAccessorHandleT *accessorHandle);

	/* 4.6.2 saImmOmAccessorGet() */
	extern SaAisErrorT
	 saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
			      const SaNameT *objectName,
			      const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes);

	/* 4.6.3 saImmOmAccessorFinalize() */

	extern SaAisErrorT
	 saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle);

	/* 4.7.1 saImmOmAdminOwnerInitialize() */

	extern SaAisErrorT
	 saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
				     const SaImmAdminOwnerNameT adminOwnerName,
				     SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *ownerHandle);

	/* 4.7.2 saImmOmAdminOwnerSet() */

	extern SaAisErrorT
	 saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope);

	/* 4.7.3 saImmOmAdminOwnerRelease() */

	extern SaAisErrorT
	 saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope);

	/* 4.7.4 saImmOmAdminOwnerFinalize() */

	extern SaAisErrorT
	 saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle);

	/* 4.7.5 saImmOmAdminOwnerClear() */

	extern SaAisErrorT
	 saImmOmAdminOwnerClear(SaImmHandleT immHandle, const SaNameT **objectNames, SaImmScopeT scope);

	/* 4.8.1 saImmOmCcbInitialize() */

	extern SaAisErrorT
	 saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle, SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle);

	/* 4.8.2 saImmOmCcbObjectCreate() */
	extern SaAisErrorT
	 saImmOmCcbObjectCreate_2(SaImmCcbHandleT ccbHandle,
				  const SaImmClassNameT className,
				  const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues);

	/* 4.8.3 saImmOmCcbObjectDelete() */

	extern SaAisErrorT
	 saImmOmCcbObjectDelete(SaImmCcbHandleT ccbHandle, const SaNameT *objectName);

	/* 4.8.4 saImmOmCcbObjectModify() */
	extern SaAisErrorT
	 saImmOmCcbObjectModify_2(SaImmCcbHandleT ccbHandle,
				  const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods);

	/* 4.8.5 saImmOmCcbApply() */

	extern SaAisErrorT
	 saImmOmCcbApply(SaImmCcbHandleT ccbHandle);

	/* 4.8.6 saImmOmCcbFinalize() */

	extern SaAisErrorT
	 saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle);

	/* 4.9.1 saImmOmAdminOperationInvoke[_2](),
	   saImmOmAdminOperationInvokeAsync() */
	extern SaAisErrorT
	 saImmOmAdminOperationInvoke_2(SaImmAdminOwnerHandleT ownerHandle,
				       const SaNameT *objectName,
				       SaImmContinuationIdT continuationId,
				       SaImmAdminOperationIdT operationId,
				       const SaImmAdminOperationParamsT_2 **params,
				       SaAisErrorT *operationReturnValue, SaTimeT timeout);

	extern SaAisErrorT
	 saImmOmAdminOperationInvokeAsync_2(SaImmAdminOwnerHandleT ownerHandle,
					    SaInvocationT invocation,
					    const SaNameT *objectName,
					    SaImmContinuationIdT continuationId,
					    SaImmAdminOperationIdT operationId,
					    const SaImmAdminOperationParamsT_2 **params);

	/* 4.9.2 SaImmOmAdminOperationInvokeCallbackT see 4.2.18 above */

	/* 4.9.3 saImmOmAdminOperationContinue(),
	   saImmOmAdminOperationContinueAsync() */

	extern SaAisErrorT
	 saImmOmAdminOperationContinue(SaImmAdminOwnerHandleT ownerHandle,
				       const SaNameT *objectName,
				       SaImmContinuationIdT continuationId, SaAisErrorT *operationReturnValue);

	extern SaAisErrorT
	 saImmOmAdminOperationContinueAsync(SaImmAdminOwnerHandleT ownerHandle,
					    SaInvocationT invocation,
					    const SaNameT *objectName, SaImmContinuationIdT continuationId);

	/* 4.9.4 saImmOmAdminOperationContinueClear() */

	extern SaAisErrorT
	 saImmOmAdminOperationContinuationClear(SaImmAdminOwnerHandleT ownerHandle,
						const SaNameT *objectName, SaImmContinuationIdT continuationId);

#ifdef  __cplusplus
}
#endif

#include <saImmOm_A_2_11.h>

#endif   /* _SA_IMM_OM_H */
