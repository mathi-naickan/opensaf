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


#ifndef _SA_IMM_OM_A_2_14_H
#define _SA_IMM_OM_A_2_14_H

#ifdef  __cplusplus
extern "C" {
#endif

/* 4.8.x saImmOmCcb */

	extern SaAisErrorT
	 saImmOmCcbValidate(SaImmCcbHandleT ccbHandle);

	extern SaAisErrorT
	 saImmOmCcbAbort(SaImmCcbHandleT ccbHandle);

#ifdef  __cplusplus
}
#endif

#include <saImmOm_A_2_15.h>

#endif   /* _SA_IMM_OM_A_2_14_H */
