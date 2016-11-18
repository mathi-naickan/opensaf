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



#ifndef _SA_IMM_OI_A_2_11_H
#define _SA_IMM_OI_A_2_11_H

#ifdef  __cplusplus
extern "C" {
#endif

	/* 5.7.2 saImmOiAdminOperationResult() See http://devel.opensaf.org/ticket/1764 */

	extern SaAisErrorT
	saImmOiAdminOperationResult_o2(
				      SaImmOiHandleT immOiHandle, 
				      SaInvocationT invocation, 
				      SaAisErrorT result,
                                      const SaImmAdminOperationParamsT_2 **returnParams);


	/* OI sets error string in CCB related upcall. See: http://devel.opensaf.org/ticket/1904 */
	extern SaAisErrorT
	saImmOiCcbSetErrorString(
				 SaImmOiHandleT immOiHandle,
				 SaImmOiCcbIdT ccbId,
				 const SaStringT errorString);



#ifdef _SA_IMM_OM_H
	extern SaAisErrorT
	saImmOiAugmentCcbInitialize(
			     SaImmOiHandleT immOiHandle,
			     SaImmOiCcbIdT ccbId,
			     SaImmCcbHandleT *ccbHandle,
			     SaImmAdminOwnerHandleT *ownerHandle);
#endif


#ifdef  __cplusplus
}
#endif

#include <saImmOi_A_2_15.h>

#endif   /* _SA_IMM_OI_A_2_11_H */
