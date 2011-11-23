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

/*****************************************************************************
..............................................................................

 FILE NAME: immnd_init.h

..............................................................................

  DESCRIPTION:
  This file consists of constants, enums and data structs used by immnd_xxx.c
******************************************************************************/

#ifndef IMMND_INIT_H
#define IMMND_INIT_H

#include <saClm.h>
#include "immsv_evt_model.h"

EXTERN_C IMMND_CB *immnd_cb;

/* internal helping function declaration */

/* file : -  immnd_proc.c */

void immnd_proc_imma_down(IMMND_CB *cb, MDS_DEST dest, NCSMDS_SVC_ID sv_id);
uns32 immnd_proc_imma_discard_connection(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *cl_node);
void immnd_proc_imma_discard_stales(IMMND_CB *cb);

void immnd_cb_dump(void);
void immnd_proc_immd_down(IMMND_CB *cb);

void immnd_proc_imma_up(IMMND_CB *cb, MDS_DEST dest);
void immnd_proc_app_status(IMMND_CB *cb);
void immnd_adjustEpoch(IMMND_CB *cb, SaBoolT increment);
uns32 immnd_introduceMe(IMMND_CB *cb);
void immnd_announceDump(IMMND_CB *cb);
uns32 immnd_is_immd_up(IMMND_CB *cb);
IMMSV_ADMIN_OPERATION_PARAM * immnd_getOsafImmPbeAdmopParam(
	    SaImmAdminOperationIdT operationId, 
	    void* evt,
	    IMMSV_ADMIN_OPERATION_PARAM * param);
void search_req_continue(IMMND_CB *cb, IMMSV_OM_RSP_SEARCH_REMOTE *reply, SaUint32T reqConn);
void immnd_ackToNid(uns32 rc);
SaBoolT immnd_syncComplete(IMMND_CB *cb, SaBoolT coordinator, SaUint32T step);

void immnd_proc_global_abort_ccb(IMMND_CB *cb, SaUint32T ccbId);

/* End immnd_proc.c */

/* file : - ImmModel.cc */
#ifdef __cplusplus
extern "C" {
#endif

	/* These are functions that bridge from the C world of the OpenSaf framework
	   to the C++ world of the IMM server model. */

	void immModel_abortSync(IMMND_CB *cb);

	void immModel_pbePrtoPurgeMutations(IMMND_CB *cb, unsigned int nodeId, SaUint32T *reqArrSize,
		SaUint32T **reqConArr);

	SaAisErrorT
	    immModel_adminOwnerCreate(IMMND_CB *cb,
				      const struct ImmsvOmAdminOwnerInitialize *req,
				      SaUint32T ownerId, SaUint32T conn, unsigned int nodeId);
	SaAisErrorT immModel_adminOwnerDelete(IMMND_CB *cb, SaUint32T ownerId, SaUint32T hard);

	SaAisErrorT
	    immModel_ccbCreate(IMMND_CB *cb,
			       SaUint32T adminOwnerId,
			       SaUint32T ccbFlags,
			       SaUint32T ccbId, unsigned int originatingNode, SaUint32T originatingConn);

	SaAisErrorT
	    immModel_adminOperationInvoke(IMMND_CB *cb,
					  const struct ImmsvOmAdminOperationInvoke *req,
					  SaUint32T reqConn,
					  SaUint64T reply_dest,
					  SaInvocationT inv, SaUint32T *implConn, SaClmNodeIdT *implNodeId);

	SaAisErrorT immModel_classCreate(IMMND_CB *cb, 
		const struct ImmsvOmClassDescr *req,
		SaUint32T reqConn,
		SaUint32T nodeId,
		SaUint32T* continuationId,
		SaUint32T* pbeConn,
		unsigned int* pbeNodeId);

	SaAisErrorT immModel_classDelete(IMMND_CB *cb,
		const struct ImmsvOmClassDescr *req,
		SaUint32T reqConn,
		SaUint32T nodeId,
		SaUint32T* continuationId,
		SaUint32T* pbeConn,
		unsigned int* pbeNodeId);

	SaAisErrorT
	    immModel_classDescriptionGet(IMMND_CB *cb, const IMMSV_OCTET_STRING *clName, struct ImmsvOmClassDescr *res);

	SaUint32T immModel_cleanTheBasement(IMMND_CB *cb,
		SaUint32T seconds,
		SaInvocationT **admReqArr,
		SaUint32T *admReqArrSize,
		SaInvocationT **searchReqArr,
		SaUint32T *searchReqArrSize,
		SaUint32T **ccbIdArr,
		SaUint32T *ccbIdArrSize, 
		SaUint32T **pbePrtoReqIdArr,
		SaUint32T *pbePrtoReqArrSize, 
		SaBoolT iAmCoordNow);

	void immModel_getNonCriticalCcbs(IMMND_CB *cb, SaUint32T **ccbIdArr, SaUint32T *ccbIdArrSize);

	void immModel_getOldCriticalCcbs(IMMND_CB *cb, SaUint32T **ccbIdArr, SaUint32T *ccbIdArrSize, 
		SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId, SaUint32T *pbeId);

	SaAisErrorT
	    immModel_ccbApply(IMMND_CB *cb,
			      SaUint32T ccbId,
			      SaUint32T replyConn,
			      SaUint32T *arrSize, SaUint32T **implConnArr, SaUint32T **implIdArr, SaUint32T **ctnArr);

	void immModel_ccbAbort(IMMND_CB *cb,
		SaUint32T ccbId, SaUint32T *arrSize, SaUint32T **implConnArr, SaUint32T *client,
		SaClmNodeIdT *pbeNodeId);

	void immModel_getCcbIdsForOrigCon(IMMND_CB *cb, SaUint32T origConn, SaUint32T *arrSize, SaUint32T **ccbIdArr);

	void immModel_discardNode(IMMND_CB *cb, SaUint32T nodeId, SaUint32T *arrSize, SaUint32T **ccbIdArr);

	SaAisErrorT
	    immModel_ccbObjectDelete(IMMND_CB *cb,
		    const struct ImmsvOmCcbObjectDelete *req,
		    SaUint32T reqConn, SaUint32T *arrSize, SaUint32T **implConnArr, SaUint32T **implIdArr, 
		    SaStringT **objNameArr, SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId);


	SaAisErrorT
	    immModel_ccbObjectCreate(IMMND_CB *cb,
		    const struct ImmsvOmCcbObjectCreate *req, SaUint32T *implConn, SaClmNodeIdT *implNodeId, 
		    SaUint32T *continuationId, SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId);

	SaAisErrorT
	    immModel_ccbObjectModify(IMMND_CB *cb,
		    const struct ImmsvOmCcbObjectModify *req, SaUint32T *implConn, SaClmNodeIdT *implNodeId, 
		    SaUint32T *continuationId, SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId);

	void immModel_ccbCompletedContinuation(IMMND_CB *cb,
					       const struct immsv_oi_ccb_upcall_rsp *rsp, SaUint32T *reqConn);

	void immModel_ccbObjDelContinuation(IMMND_CB *cb,
					    const struct immsv_oi_ccb_upcall_rsp *rsp, SaUint32T *reqConn);

	SaBoolT immModel_ccbWaitForCompletedAck(IMMND_CB *cb, SaUint32T ccbId, SaAisErrorT *err, 
                                            SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId, 
                                            SaUint32T *pbeId, SaUint32T *pbeCtn);

	SaBoolT immModel_ccbWaitForDeleteImplAck(IMMND_CB *cb, SaUint32T ccbId, SaAisErrorT *err);

	SaBoolT immModel_ccbCommit(IMMND_CB *cb, SaUint32T ccbId, SaUint32T *arrSize, SaUint32T **implConnArr);
	SaAisErrorT immModel_ccbFinalize(IMMND_CB *cb, SaUint32T ccbId);

	SaAisErrorT immModel_searchInitialize(IMMND_CB *cb, struct ImmsvOmSearchInit *req, void **searchOp);

	SaAisErrorT
	    immModel_nextResult(IMMND_CB *cb, void *searchOp,
				IMMSV_OM_RSP_SEARCH_NEXT **rsp,
				SaUint32T *implConn, SaUint32T *implNodeId,
				struct ImmsvAttrNameList **rtAttrsToFetch, MDS_DEST *implDest);

	void immModel_deleteSearchOp(void *searchOp);

	void immModel_clearLastResult(void *searchOp);

	void immModel_fetchLastResult(void *searchOp, IMMSV_OM_RSP_SEARCH_NEXT **rsp);

	void immModel_setAdmReqContinuation(IMMND_CB *cb, SaInvocationT invoc, SaUint32T reqCon);

	void immModel_setSearchReqContinuation(IMMND_CB *cb, SaInvocationT invoc, SaUint32T reqCon);

	void immModel_setSearchImplContinuation(IMMND_CB *cb, SaUint32T searchId,
						SaUint32T requestnodeId, MDS_DEST reply_dest);

	SaAisErrorT
	    immModel_implementerSet(IMMND_CB *cb, const IMMSV_OCTET_STRING *implName,
				    SaUint32T implConn, SaUint32T implNodeId, SaUint32T implId, MDS_DEST mds_dest);

	SaAisErrorT
	    immModel_implementerClear(IMMND_CB *cb, const struct ImmsvOiImplSetReq *req,
				      SaUint32T implConn, SaUint32T implNodeId);
	SaAisErrorT
	    immModel_classImplementerSet(IMMND_CB *cb, const struct ImmsvOiImplSetReq *req,
					 SaUint32T implConn, SaUint32T implNodeId);

	SaAisErrorT
	    immModel_classImplementerClear(IMMND_CB *cb,
					   const struct ImmsvOiImplSetReq *req,
					   SaUint32T implConn, SaUint32T implNodeId);

	SaAisErrorT
	    immModel_classImplementerRelease(IMMND_CB *cb,
					     const struct ImmsvOiImplSetReq *req,
					     SaUint32T implConn, SaUint32T implNodeId);

	SaAisErrorT
	    immModel_objectImplementerSet(IMMND_CB *cb,
					  const struct ImmsvOiImplSetReq *req,
					  SaUint32T implConn, SaUint32T implNodeId);

	SaAisErrorT
	    immModel_objectImplementerRelease(IMMND_CB *cb,
					      const struct ImmsvOiImplSetReq *req,
					      SaUint32T implConn, SaUint32T implNodeId);
	SaUint32T immModel_getImplementerId(IMMND_CB *cb, SaUint32T implConn);

	void immModel_discardImplementer(IMMND_CB *cb, SaUint32T implId, SaBoolT reallyDiscard);

	void immModel_fetchAdmOpContinuations(IMMND_CB *cb, SaInvocationT inv,
					      SaBoolT local, SaUint32T *implConn,
					      SaUint32T *reqConn, SaUint64T *reply_dest);

	void immModel_fetchSearchReqContinuation(IMMND_CB *cb, SaInvocationT inv, SaUint32T *reqConn);
	void immModel_fetchSearchImplContinuation(IMMND_CB *cb, SaUint32T searchId,
						  SaUint32T reqNodeId, MDS_DEST *reply_dest);

	SaUint32T immModel_findConnForImplementerOfObject(IMMND_CB *cb, IMMSV_OCTET_STRING *objectName);

	void immModel_prepareForLoading(IMMND_CB *cb);

	SaBoolT immModel_readyForLoading(IMMND_CB *cb);

	SaInt32T immModel_getLoader(IMMND_CB *cb);

	void immModel_setLoader(IMMND_CB *cb, SaInt32T loaderPid);

	SaBoolT immModel_pbeOiExists(IMMND_CB *cb);

	void immModel_recognizedIsolated(IMMND_CB *cb);

	SaBoolT immModel_syncComplete(IMMND_CB *cb);

	SaBoolT immModel_ccbsTerminated(IMMND_CB *cb);

	void immModel_prepareForSync(IMMND_CB *cb, SaBoolT isJoining);

	SaAisErrorT immModel_objectSync(IMMND_CB *cb, const struct ImmsvOmObjectSync *req);

	SaAisErrorT
	    immModel_finalizeSync(IMMND_CB *cb, struct ImmsvOmFinalizeSync *req, SaBoolT isCoord, SaBoolT isSyncClient);

	SaUint32T immModel_adjustEpoch(IMMND_CB *cb, SaUint32T suggestedEpoch,
		SaUint32T *continuationId, SaUint32T *pbeConn,
		SaClmNodeIdT *pbeNodeId, SaBoolT increment);

	SaUint32T immModel_adminOwnerChange(IMMND_CB *cb, const struct immsv_a2nd_admown_set *req, SaBoolT isRelease);
	void immModel_getAdminOwnerIdsForCon(IMMND_CB *cb, SaUint32T conn, SaUint32T *arrSize, SaUint32T **ccbIdArr);

	void immModel_ccbObjCreateContinuation(IMMND_CB *cb,
		SaUint32T ccbId, SaUint32T invocation,
		SaAisErrorT error, SaUint32T *reqConn);

	void immModel_pbePrtObjCreateContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaAisErrorT err,
		SaClmNodeIdT nodeId, SaUint32T *reqConn);

	void immModel_pbePrtObjDeletesContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaAisErrorT err,
		SaClmNodeIdT nodeId, SaUint32T *reqConn);

	void immModel_pbePrtAttrUpdateContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaAisErrorT err,
		SaClmNodeIdT nodeId, SaUint32T *reqConn);

	void immModel_pbeClassCreateContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaClmNodeIdT nodeId, SaUint32T *reqConn);

	void immModel_pbeClassDeleteContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaClmNodeIdT nodeId, SaUint32T *reqConn);

	void immModel_pbeUpdateEpochContinuation(IMMND_CB *cb,
		SaUint32T invocation, SaClmNodeIdT nodeId);


	void immModel_ccbObjModifyContinuation(IMMND_CB *cb,
					       SaUint32T ccbId,
					       SaUint32T invocation, SaAisErrorT error, SaUint32T *reqConn);

	void immModel_discardContinuations(IMMND_CB *cb, SaUint32T deadConn);

	SaBoolT immModel_immNotWritable(IMMND_CB *cb);

	SaBoolT immModel_pbeIsInSync(IMMND_CB *cb);

	SaImmRepositoryInitModeT immModel_getRepositoryInitMode(IMMND_CB *cb);

	SaAisErrorT
	    immModel_rtObjectCreate(IMMND_CB *cb,
		    struct ImmsvOmCcbObjectCreate *req,
		    SaUint32T implConn, SaClmNodeIdT implNodeId,
		    SaUint32T *continuationId, SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId);

	SaAisErrorT
	    immModel_rtObjectDelete(IMMND_CB *cb,
		    const struct ImmsvOmCcbObjectDelete *req,
		    SaUint32T implConn, SaClmNodeIdT implNodeId,
		    SaUint32T *continuationId, SaUint32T *pbeConn,
		    SaClmNodeIdT *pbeNodeId, SaStringT **objNameArr,
		    SaUint32T* arrSize);

	SaAisErrorT
	    immModel_rtObjectUpdate(IMMND_CB *cb,
		    const struct ImmsvOmCcbObjectModify *req,
		    SaUint32T implConn, SaClmNodeIdT implNodeId, 
		    unsigned int *isPureLocal, 
		    SaUint32T *continuationId, SaUint32T *pbeConn, SaClmNodeIdT *pbeNodeId);

	SaAisErrorT immModel_ccbResult(IMMND_CB *cb, SaUint32T ccbId);

#ifdef __cplusplus
}
#endif
/* End ImmModel.cc *//* File : ---  immnd_amf.c */ uns32 immnd_amf_init(IMMND_CB *immnd_cb);

/* End immnd_amf.c */

/* File immnd_db.c */

void immnd_client_node_get(IMMND_CB *cb, SaImmHandleT imm_client_hdl, IMMND_IMM_CLIENT_NODE **imm_client_node);
void immnd_client_node_getnext(IMMND_CB *cb, SaImmHandleT imm_client_hdl, IMMND_IMM_CLIENT_NODE **imm_client_node);
uns32 immnd_client_node_add(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *imm_node);
uns32 immnd_client_node_del(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *imm_client_node);

IMMSV_OCTET_STRING *immnd_dequeue_incoming_fevs_msg(IMMSV_OCTET_STRING *msg,
	IMMND_CB *cb, SaUint64T msgNo, SaImmHandleT *clnt_hdl,
	MDS_DEST *reply_dest);

void immnd_enqueue_incoming_fevs_msg(IMMND_CB *cb, SaUint64T msgNo,
	SaImmHandleT clnt_hdl, MDS_DEST reply_dest, IMMSV_OCTET_STRING *msg,
	SaUint64T *next_expected,	//out
	SaUint32T *andHowManyMore);	//out

void immnd_dequeue_outgoing_fevs_msg(IMMND_CB *cb, 
	IMMSV_OCTET_STRING *msg, SaImmHandleT *clnt_hdl);

void immnd_enqueue_outgoing_fevs_msg(IMMND_CB *cb, SaImmHandleT clnt_hdl,
	IMMSV_OCTET_STRING *msg);

void dequeue_outgoing(IMMND_CB *cb);

/* End  File : immnd_db.c */

/* File : --- immnd_mds.c */
SaAisErrorT immnd_mds_client_not_busy(IMMSV_SEND_INFO *s_info);
uns32 immnd_mds_send_rsp(IMMND_CB *cb, IMMSV_SEND_INFO *s_info, IMMSV_EVT *evt);
uns32 immnd_mds_msg_sync_send(IMMND_CB *cb, uns32 to_svc, MDS_DEST to_dest,
			      IMMSV_EVT *i_evt, IMMSV_EVT **o_evt, uns32 timeout);
uns32 immnd_mds_msg_send(IMMND_CB *cb, uns32 to_svc, MDS_DEST to_dest, IMMSV_EVT *evt);
uns32 immnd_mds_register(IMMND_CB *cb);
void immnd_mds_unregister(IMMND_CB *cb);
uns32 immnd_mds_get_handle(IMMND_CB *cb);
/* End : --- immnd_mds.c */

/* File : ----  immnd_evt.c */
void immnd_process_evt(void);
uns32 immnd_evt_destroy(IMMSV_EVT *evt, SaBoolT onheap, uns32 line);
/* End : ----  immnd_evt.c  */

/* File : ----  immnd_proc.c */
uns32 immnd_proc_server(uns32 *timeout);
/* End : ----  immnd_proc.c  */

#endif
