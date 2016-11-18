/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s): GoAhead Software
 *
 */

#ifndef SMF_AGENT_SMFA_H_
#define SMF_AGENT_SMFA_H_

#include "osaf/saf/saAis.h"
#include "smf/saf/saSmf.h"

#include "base/ncsgl_defs.h"
#include "base/ncssysf_ipc.h"
#include "base/ncssysf_lck.h"
#include "base/ncs_main_papi.h"
#include "mds/mds_papi.h"
#include "base/logtrace.h"
#include "smf/smfsv_evt.h"

typedef struct smfa_scope_info{
	SaSmfCallbackScopeIdT 	scope_id;
	SaSmfLabelFilterArrayT	scope_of_interest; 
	struct smfa_scope_info 	*next_scope;
}SMFA_SCOPE_INFO;

typedef struct smfa_client_info{
	SaSmfHandleT		client_hdl;
	SaSmfCallbacksT		reg_cbk;
	SYSF_MBX		cbk_mbx;
	SMFA_SCOPE_INFO		*scope_info_list;	
	struct smfa_client_info	*next_client;
}SMFA_CLIENT_INFO;

typedef struct smfa_cbk_hdl_list{
	SaSmfHandleT			hdl;
	uint32_t				cnt;
	struct smfa_cbk_hdl_list 	*next_hdl;
}SMFA_CBK_HDL_LIST;

typedef struct smfa_cbk_list{
	SaInvocationT		inv_id;
	SMFA_CBK_HDL_LIST	*hdl_list;
	struct smfa_cbk_list 	*next_cbk;
}SMFA_CBK_LIST;

typedef struct smfa_cb{
	MDS_HDL			smfa_mds_hdl;
	MDS_DEST		smfnd_adest;
	bool		is_smfnd_up;
	bool		is_finalized;
	NCS_LOCK		cb_lock;
	SMFA_CLIENT_INFO	*smfa_client_info_list;
	SMFA_CBK_LIST		*cbk_list;
}SMFA_CB;

extern SMFA_CB _smfa_cb;

uint32_t smfa_init();
uint32_t smfa_finalize();
void smfa_client_info_add(SMFA_CLIENT_INFO *);
bool smfa_client_mbx_clnup(NCSCONTEXT , NCSCONTEXT );
SMFA_CLIENT_INFO * smfa_client_info_get(SaSmfHandleT );
SMFA_SCOPE_INFO* smfa_scope_info_get(SMFA_CLIENT_INFO *, SaSmfCallbackScopeIdT );
void smfa_scope_info_free(SMFA_SCOPE_INFO *);
void smfa_scope_info_add(SMFA_CLIENT_INFO *, SMFA_SCOPE_INFO *);
SaAisErrorT smfa_dispatch_cbk_one(SMFA_CLIENT_INFO *);
SaAisErrorT smfa_dispatch_cbk_all(SMFA_CLIENT_INFO *);
SaAisErrorT smfa_dispatch_cbk_block(SMFA_CLIENT_INFO *);
uint32_t smfa_cbk_err_resp_process(SaInvocationT,SaSmfHandleT);
uint32_t smfa_cbk_ok_resp_process(SaSmfHandleT smfHandle,SaInvocationT );
uint32_t smfa_to_smfnd_mds_async_send(NCSCONTEXT );
uint32_t smfa_scope_info_rmv(SMFA_CLIENT_INFO *, SaSmfCallbackScopeIdT );
void smfa_client_info_clean(SMFA_CLIENT_INFO *);
uint32_t smfa_cbk_list_cleanup(SaSmfHandleT );
uint32_t smfa_client_info_rmv(SaSmfHandleT );
uint32_t smfa_mds_register();
uint32_t smfa_mds_unregister();
void smfa_evt_free(SMF_EVT *);
uint32_t smfa_cbk_filter_type_match(SaSmfLabelFilterT *,SaSmfCallbackLabelT *);
uint32_t smfa_cbk_filter_match(SMFA_CLIENT_INFO *,SMF_CBK_EVT *);
#endif  // SMF_AGENT_SMFA_H_
