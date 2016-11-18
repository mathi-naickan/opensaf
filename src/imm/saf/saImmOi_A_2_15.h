/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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
 */

/*
 * DESCRIPTION:
 *   This file provides the suggested additions to the C language binding for 
 *   the Service Availability(TM) Forum Information Model Management Service (IMM).
 *   It contains only the prototypes and type definitions that are part of this
 *   proposed addition. 
 *   These additions are currently NON STANDARD. But the intention is to get these
 *   additions approved formally by SAF in the future.
 *
 *   For detailed explanation of the new API, see osaf/services/saf/immsv/README.
 */


#ifndef _SA_IMM_OI_A_2_15_H
#define _SA_IMM_OI_A_2_15_H

#ifdef  __cplusplus
extern "C" {
#endif

	/* From 5.5.4  */
	typedef SaAisErrorT
	 (*SaImmOiRtAttrUpdateCallbackT_o3) (SaImmOiHandleT immOiHandle,
					  SaConstStringT objectName, const SaImmAttrNameT *attributeNames);
	/* From 5.6.1 */
	typedef SaAisErrorT
	 (*SaImmOiCcbObjectCreateCallbackT_o3) (SaImmOiHandleT immOiHandle,
					       SaImmOiCcbIdT ccbId,
					       const SaImmClassNameT className,
					       SaConstStringT objectName, const SaImmAttrValuesT_2 **attr);

	/* From 5.6.2  */
	typedef SaAisErrorT
	 (*SaImmOiCcbObjectDeleteCallbackT_o3) (SaImmOiHandleT immOiHandle,
					     SaImmOiCcbIdT ccbId, SaConstStringT objectName);
	/* From 5.6.3  */
	typedef SaAisErrorT
	 (*SaImmOiCcbObjectModifyCallbackT_o3) (SaImmOiHandleT immOiHandle,
					       SaImmOiCcbIdT ccbId,
					       SaConstStringT objectName, const SaImmAttrModificationT_2 **attrMods);

	/* From 5.7.1  */
	typedef void
	 (*SaImmOiAdminOperationCallbackT_o3) (SaImmOiHandleT immOiHandle,
					      SaInvocationT invocation,
					      SaConstStringT objectName,
					      SaImmAdminOperationIdT operationId,
					      const SaImmAdminOperationParamsT_2 **params);

	/* SaImmOiCallbacksT_2 */
	typedef struct {
		SaImmOiAdminOperationCallbackT_o3 saImmOiAdminOperationCallback;
		SaImmOiCcbAbortCallbackT saImmOiCcbAbortCallback;
		SaImmOiCcbApplyCallbackT saImmOiCcbApplyCallback;
		SaImmOiCcbCompletedCallbackT saImmOiCcbCompletedCallback;
		SaImmOiCcbObjectCreateCallbackT_o3 saImmOiCcbObjectCreateCallback;
		SaImmOiCcbObjectDeleteCallbackT_o3 saImmOiCcbObjectDeleteCallback;
		SaImmOiCcbObjectModifyCallbackT_o3 saImmOiCcbObjectModifyCallback;
		SaImmOiRtAttrUpdateCallbackT_o3 saImmOiRtAttrUpdateCallback;
	} SaImmOiCallbacksT_o3;


	/* 5.3.1 saImmOiInitialize() */
	extern SaAisErrorT
	 saImmOiInitialize_o3(SaImmOiHandleT *immOiHandle,
			     const SaImmOiCallbacksT_o3 *immOiCallbacks, SaVersionT *version);


	extern SaAisErrorT
	 saImmOiObjectImplementerSet_o3(SaImmOiHandleT immOiHandle, SaConstStringT objectName, SaImmScopeT scope);

	/* 5.4.6 saImmOiObjectImplementerRelease() */

	extern SaAisErrorT
	 saImmOiObjectImplementerRelease_o3(SaImmOiHandleT immOiHandle, SaConstStringT objectName, SaImmScopeT scope);


	/*
	 *
	 * Runtime Object Management Routines
	 *
	 */

	/* 5.5.1 saImmOiRtObjectCreate() */

	extern SaAisErrorT
	 saImmOiRtObjectCreate_o3(SaImmOiHandleT immOiHandle,
				 const SaImmClassNameT className,
				 SaConstStringT objectName, const SaImmAttrValuesT_2 **attrValues);

	/* 5.5.2 saImmOiRtObjectDelete() */

	extern SaAisErrorT
	 saImmOiRtObjectDelete_o3(SaImmOiHandleT immOiHandle, SaConstStringT objectName);

	/* 5.5.3 saImmOiRtObjectUpdate() */

	extern SaAisErrorT
	 saImmOiRtObjectUpdate_o3(SaImmOiHandleT immOiHandle,
				 SaConstStringT objectName, const SaImmAttrModificationT_2 **attrMods);

#ifdef  __cplusplus
}
#endif

#endif   /* _SA_IMM_OI_A_2_15_H */
