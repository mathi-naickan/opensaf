/*      -*- OpenSAF -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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
 * Author(s): Oracle 
 *
 */

/*
 * DESCRIPTION:
 *   This file provides the suggested additions to the C language binding for 
 *   the Service Availability(TM) Forum Availability Management Framework (AMF).
 *   This file contains only the prototypes and type definitions that are part of this
 *   proposed addition. 
 *   These additions are currently NON STANDARD. Intention is to get these
 *   additions approved formally by SAF in the future.
 *
 *  Note: For detailed explanation of these new resources, see osaf/services/saf/amf/README.
 */

#ifndef _SA_AMF_B_04_02_H
#define _SA_AMF_B_04_02_H

#ifdef  __cplusplus
extern "C" {
#endif
    
typedef void
(*OsafCsiAttributeChangeCallbackT)(
    SaInvocationT invocation,
    const SaNameT *csiName,
    SaAmfCSIAttributeListT   csiAttr);

typedef struct {
    SaAmfHealthcheckCallbackT                   saAmfHealthcheckCallback;
    SaAmfComponentTerminateCallbackT            saAmfComponentTerminateCallback;
    SaAmfCSISetCallbackT                        saAmfCSISetCallback;
    SaAmfCSIRemoveCallbackT                     saAmfCSIRemoveCallback;
    SaAmfProtectionGroupTrackCallbackT_4        saAmfProtectionGroupTrackCallback;
    SaAmfProxiedComponentInstantiateCallbackT   saAmfProxiedComponentInstantiateCallback;
    SaAmfProxiedComponentCleanupCallbackT       saAmfProxiedComponentCleanupCallback;
    SaAmfContainedComponentInstantiateCallbackT saAmfContainedComponentInstantiateCallback;
    SaAmfContainedComponentCleanupCallbackT     saAmfContainedComponentCleanupCallback;
    OsafCsiAttributeChangeCallbackT             osafCsiAttributeChangeCallback;
} SaAmfCallbacksT_o4;

extern SaAisErrorT 
saAmfInitialize_o4(
    SaAmfHandleT *amfHandle, 
    const SaAmfCallbacksT_o4 *amfCallbacks,
    SaVersionT *version);

#ifdef  __cplusplus
}
#endif
#endif  /* _SA_AMF_B_04_02_H */
