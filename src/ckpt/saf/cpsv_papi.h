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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

 _Public_ CPA abstractions and function prototypes

*******************************************************************************/

/*
 * Module Inclusion Control...
 */

#ifndef CPSV_PAPI_H
#define CPSV_PAPI_H

#include "saCkpt.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* Temp Place */

typedef void
(*ncsCkptCkptArrivalCallbackT) (const SaCkptCheckpointHandleT checkpointHandle,
                                SaCkptIOVectorElementT *ioVector, SaUint32T numberOfElements);
SaAisErrorT
ncsCkptRegisterCkptArrivalCallback(SaCkptHandleT ckptHandle, ncsCkptCkptArrivalCallbackT ckptArrivalCallback);
#ifdef  __cplusplus
}
#endif

#endif   /* CPSV_PAPI_H */
