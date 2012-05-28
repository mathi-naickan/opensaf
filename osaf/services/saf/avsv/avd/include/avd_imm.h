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
/****************************************************************************

  DESCRIPTION:

  This module is the include file for IMM integration.
  
****************************************************************************/
#ifndef AVD_IMM_H
#define AVD_IMM_H

#include <immutil.h>

typedef void (*AvdImmOiCcbApplyCallbackT) (CcbUtilOperationData_t *opdata);
typedef SaAisErrorT (*AvdImmOiCcbCompletedCallbackT) (CcbUtilOperationData_t *opdata);

extern SaAisErrorT avd_imm_init(void *avd_cb);
extern void avd_imm_impl_set_task_create(void);
extern SaAisErrorT avd_imm_impl_set(void);
extern void avd_imm_applier_set_task_create(void);
extern SaAisErrorT avd_imm_applier_set(void);
extern void avd_imm_update_runtime_attrs(void);

/**
 * Possible return values from avd_job_fifo_dequeue
 */
typedef enum {
	JOB_EXECUTED = 1,  /* A job was successfully executed */
	JOB_ETRYAGAIN = 2, /* A job exist but execution was rejected with TRY_AGAIN */
	JOB_ENOTEXIST = 3, /* No job exist */
	JOB_EINVH = 4,     /* Invalid handle */ 
	JOB_ERR = 5        /* Other error than TRY_AGAIN, job was removed/deleted */

} AvdJobDequeueResultT;

extern AvdJobDequeueResultT avd_job_fifo_execute(SaImmOiHandleT immOiHandle);
extern void avd_job_fifo_empty(void);

/**
 * Install callbacks associated with classNames
 * @param className
 * @param rtattr_cb
 * @param adminop_cb
 * @param ccb_compl_cb
 * @param ccb_apply_cb
 * @param ccb_abort_cb
 */
void avd_class_impl_set(const SaImmClassNameT className,
	SaImmOiRtAttrUpdateCallbackT rtattr_cb, SaImmOiAdminOperationCallbackT_2 adminop_cb,
	AvdImmOiCcbCompletedCallbackT ccb_compl_cb, AvdImmOiCcbApplyCallbackT ccb_apply_cb);
/**
 * Default callback that can be uses for e.g. base types, will return OK for create & delete ops.
 * Modify returns error.
 * 
 * @param opdata
 * 
 * @return SaAisErrorT
 */
SaAisErrorT avd_imm_default_OK_completed_cb(CcbUtilOperationData_t *opdata);

extern unsigned int avd_imm_config_get(void);
extern void avd_saImmOiRtObjectUpdate_sync(const SaNameT *dn,
	SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType, void *value);
extern void avd_saImmOiRtObjectUpdate(const SaNameT* dn, SaImmAttrNameT attributeName,
     SaImmValueTypeT attrValueType, void* value);
extern void avd_saImmOiRtObjectCreate(const SaImmClassNameT className,
	const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues);
extern void avd_saImmOiRtObjectDelete(const SaNameT* objectName);

extern void avd_imm_reinit_bg(void);
extern void avd_saImmOiAdminOperationResult(SaImmOiHandleT immOiHandle,
									 SaInvocationT invocation,
									 SaAisErrorT result);

#endif
