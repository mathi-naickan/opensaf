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


#ifndef _SA_IMM_OM_A_2_15_H
#define _SA_IMM_OM_A_2_15_H

#ifdef  __cplusplus
extern "C" {
#endif

	/* 4.5.1 saImmOmSearchInitialize() */
	extern SaAisErrorT
	 saImmOmSearchInitialize_o3(SaImmHandleT immHandle,
				    SaConstStringT rootName,
				    SaImmScopeT scope,
				    SaImmSearchOptionsT searchOptions,
				    const SaImmSearchParametersT_2 *searchParam, 
				    const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle);

	/* 4.5.2 saImmOmSearchNext() */
	extern SaAisErrorT
	 saImmOmSearchNext_o3(SaImmSearchHandleT searchHandle, SaStringT *objectName, SaImmAttrValuesT_2 ***attributes);

	/* 4.6.2 saImmOmAccessorGet() */
	extern SaAisErrorT
	 saImmOmAccessorGet_o3(SaImmAccessorHandleT accessorHandle,
			       SaConstStringT objectName,
			       const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes);



	/* 4.7.2 saImmOmAdminOwnerSet() */

	extern SaAisErrorT
	 saImmOmAdminOwnerSet_o3(SaImmAdminOwnerHandleT ownerHandle, SaConstStringT *objectNames, SaImmScopeT scope);

	/* 4.7.3 saImmOmAdminOwnerRelease() */

	extern SaAisErrorT
	 saImmOmAdminOwnerRelease_o3(SaImmAdminOwnerHandleT ownerHandle, SaConstStringT *objectNames, SaImmScopeT scope);

	/* 4.7.5 saImmOmAdminOwnerClear() */

	extern SaAisErrorT
	 saImmOmAdminOwnerClear_o3(SaImmHandleT immHandle, SaConstStringT *objectNames, SaImmScopeT scope);



	/* 4.8.2 saImmOmCcbObjectCreate() */
	extern SaAisErrorT
	 saImmOmCcbObjectCreate_o3(SaImmCcbHandleT ccbHandle,
				   const SaImmClassNameT className,
				   SaConstStringT objectName, const SaImmAttrValuesT_2 **attrValues);

	/* 4.8.3 saImmOmCcbObjectDelete() */

	extern SaAisErrorT
	 saImmOmCcbObjectDelete_o3(SaImmCcbHandleT ccbHandle, SaConstStringT objectName);

	/* 4.8.4 saImmOmCcbObjectModify() */
	extern SaAisErrorT
	 saImmOmCcbObjectModify_o3(SaImmCcbHandleT ccbHandle,
				   SaConstStringT objectName, const SaImmAttrModificationT_2 **attrMods);


	/* 4.9.1 saImmOmAdminOperationInvoke(),
	   saImmOmAdminOperationInvokeAsync() 
	   See http://devel.opensaf.org/ticket/1764 */
	extern SaAisErrorT
	 saImmOmAdminOperationInvoke_o3(
					 SaImmAdminOwnerHandleT ownerHandle,
					 SaConstStringT objectName,
					 SaImmContinuationIdT continuationId,
					 SaImmAdminOperationIdT operationId,
					 const SaImmAdminOperationParamsT_2 **params,
					 SaAisErrorT *operationReturnValue,
					 SaTimeT timeout,
					 SaImmAdminOperationParamsT_2 ***returnParams);

	extern SaAisErrorT
	 saImmOmAdminOperationInvokeAsync_o3(SaImmAdminOwnerHandleT ownerHandle,
					     SaInvocationT invocation,
					     SaConstStringT objectName,
					     SaImmContinuationIdT continuationId,
					     SaImmAdminOperationIdT operationId,
					     const SaImmAdminOperationParamsT_2 **params);

	/* 4.9.3 saImmOmAdminOperationContinue(),
	   saImmOmAdminOperationContinueAsync() */

	/* This is not implemented in OpenSAF, but define the API anyway for completenes. 
           See http://devel.opensaf.org/ticket/1764 */

	extern SaAisErrorT
	 saImmOmAdminOperationContinue_o3(
					    SaImmAdminOwnerHandleT ownerHandle,
					    SaConstStringT objectName,
					    SaImmContinuationIdT continuationId,
					    SaAisErrorT *operationReturnValue,
					    SaImmAdminOperationParamsT_2 ***returnParams);

	extern SaAisErrorT
	 saImmOmAdminOperationContinueAsync_o3(SaImmAdminOwnerHandleT ownerHandle,
					       SaInvocationT invocation,
					       SaConstStringT objectName, SaImmContinuationIdT continuationId);

	/* 4.9.4 saImmOmAdminOperationContinueClear() */

	extern SaAisErrorT
	 saImmOmAdminOperationContinuationClear_o3(SaImmAdminOwnerHandleT ownerHandle,
						   SaConstStringT objectName, SaImmContinuationIdT continuationId);


	/* 4.2.5 SaImmAttrFlagsT */
	/*
#define SA_IMM_ATTR_MULTI_VALUE   0x00000001
#define SA_IMM_ATTR_RDN           0x00000002
#define SA_IMM_ATTR_CONFIG        0x00000100
#define SA_IMM_ATTR_WRITABLE      0x00000200
#define SA_IMM_ATTR_INITIALIZED   0x00000400
#define SA_IMM_ATTR_RUNTIME       0x00010000
#define SA_IMM_ATTR_PERSISTENT    0x00020000
#define SA_IMM_ATTR_CACHED        0x00040000
#define SA_IMM_ATTR_NO_DUPLICATES 0x0000000001000000 / * See: http://devel.opensaf.org/ticket/1545
							 Supported in OpenSaf 4.3 * /
#define SA_IMM_ATTR_NOTIFY        0x0000000002000000 / * See: http://devel.opensaf.org/ticket/2883
							 Supported in OpenSaf 4.3 * /
#define SA_IMM_ATTR_NO_DANGLING   0x0000000004000000    / * See: https://sourceforge.net/p/opensaf/tickets/49/
                                                         Supported in OpenSaf 4.4 * /
	*/

#define SA_IMM_ATTR_DN            0x0000000008000000    /* See: https://sourceforge.net/p/opensaf/tickets/643
                                                         Supported in OpenSaf 4.6 */




#ifdef  __cplusplus
}
#endif

#include <saImmOm_A_2_16.h>

#endif   /* _SA_IMM_OM_A_2_15_H */
