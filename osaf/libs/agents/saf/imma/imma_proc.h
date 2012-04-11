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

#ifndef IMMA_PROC_H
#define IMMA_PROC_H

/* Call back Types */
typedef enum imma_callback_type {
	IMMA_CALLBACK_OM_ADMIN_OP = 1,
	IMMA_CALLBACK_PBE_ADMIN_OP,
	IMMA_CALLBACK_OM_ADMIN_OP_RSP,
	IMMA_CALLBACK_OI_CCB_COMPLETED,
	IMMA_CALLBACK_OI_CCB_CREATE,
	IMMA_CALLBACK_PBE_PRT_OBJ_CREATE,
	IMMA_CALLBACK_PBE_PRT_OBJ_DELETE,
	IMMA_CALLBACK_PBE_PRTO_DELETES_COMPLETED,
	IMMA_CALLBACK_PBE_PRT_ATTR_UPDATE,
	IMMA_CALLBACK_OI_CCB_DELETE,
	IMMA_CALLBACK_OI_CCB_MODIFY,
	IMMA_CALLBACK_OI_CCB_APPLY,
	IMMA_CALLBACK_OI_CCB_ABORT,
	IMMA_CALLBACK_OI_RT_ATTR_UPDATE,
	IMMA_CALLBACK_STALE_HANDLE,
	IMMA_CALLBACK_TYPE_SYNC,	/* NOTE this should be removed */
	IMMA_CALLBACK_TYPE_MAX = IMMA_CALLBACK_TYPE_SYNC
} IMMA_CALLBACK_TYPE;

/* Info required for Call back */
typedef struct imma_callback_info {
	struct imma_callback_info *next;	/* This is required, as this struct 
						   is posted to mailbox */
	IMMA_CALLBACK_TYPE type;
	/*Note: We should perhaps have a union here. But the logic for 
	  de-allocation is much simpler when we dont need a big switch on 
	  callback type.
	 */
	SaImmHandleT lcl_imm_hdl;
	SaInvocationT invocation;	//ABT: Warning, overloaded use
	SaImmClassNameT className;
	SaNameT name;
	SaImmAdminOperationIdT operationId;
	SaImmAdminOperationParamsT_2 **params;
	IMMSV_ATTR_VALUES_LIST *attrValues;
	IMMSV_ATTR_MODS_LIST *attrMods;
	IMMSV_ATTR_NAME_LIST *attrNames;
	SaUint32T ccbID;
	SaUint32T implId;
	SaUint32T inv;
	SaUint32T requestNodeId;

	SaAisErrorT retval;
	SaAisErrorT sa_err;

	/* Extra pointer to create callback param needed in some cases
	   inside getAdmoName() in imma_oi_api.c */
	const SaImmAttrValuesT_2 **attrValsForCreateUc;
} IMMA_CALLBACK_INFO;

void imma_process_evt(IMMA_CB *cb, IMMSV_EVT *evt);
uint32_t imma_version_validate(SaVersionT *version);

uint32_t imma_callback_ipc_init(IMMA_CLIENT_NODE *client_info);
void imma_callback_ipc_destroy(IMMA_CLIENT_NODE *client_info);

uint32_t imma_finalize_client(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node);

void imma_proc_stale_dispatch(IMMA_CB *cb, IMMA_CLIENT_NODE *clnd);

void imma_determine_clients_to_resurrect(IMMA_CB *cb, bool* locked);
uint32_t imma_proc_resurrect_client(IMMA_CB *cb, SaImmHandleT immHandle, bool isOm, SaAisErrorT *err_resurrect);

void imma_proc_increment_pending_reply(IMMA_CLIENT_NODE *clnd);
void imma_proc_decrement_pending_reply(IMMA_CLIENT_NODE *clnd);

SaAisErrorT imma_proc_recover_ccb_result(IMMA_CB *cb, SaUint32T ccbId);

int imma_proc_is_adminop_params_valid(const SaImmAdminOperationParamsT_2 **params);
int imma_proc_is_valid_type(const SaImmValueTypeT theType);

SaImmAdminOperationParamsT_2 **imma_proc_get_params(IMMSV_ADMIN_OPERATION_PARAM *in_params);

void imma_proc_free_pointers(IMMA_CB *cb, IMMA_EVT *evt);

/* callback prototypes */
IMMA_CALLBACK_INFO *imma_callback_ipc_rcv(IMMA_CLIENT_NODE *clnd);
uint32_t imma_hdl_callbk_dispatch_one(IMMA_CB *cb, SaImmHandleT immHandle);
uint32_t imma_hdl_callbk_dispatch_all(IMMA_CB *cb, SaImmHandleT immHandle);
uint32_t imma_hdl_callbk_dispatch_block(IMMA_CB *cb, SaImmHandleT immHandle);

/* Admin operation continuation functions */
int imma_popAsyncAdmOpContinuation(IMMA_CB *cb,
					SaInt32T invocation, SaImmHandleT *immHandle, SaInvocationT *userInvoc);

#endif
