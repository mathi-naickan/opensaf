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
  FILE NAME: GLD_API.C

  DESCRIPTION: Contain functions to create and destroy the GLD

  FUNCTIONS INCLUDED in this module:
  
******************************************************************************/

#include "gld.h"
#include "gld_imm.h"
#include <poll.h>
uns32 gl_gld_hdl;

static void gld_main_process(SYSF_MBX *mbx);

#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_IMM 3		/* Must be the last in the fds array */

static struct pollfd fds[4];
static nfds_t nfds = 4;

/****************************************************************************
 * Name          : ncs_glsv_gld_lib_req
 *
 * Description   : This is the NCS SE API which is used to init/destroy GLD
 *                 module
 *                 
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uns32 res = NCSCC_RC_FAILURE;

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		/* need to fetch the information from the "NCS_LIB_REQ_INFO" struct - TBD */
		res = gld_se_lib_init(req_info);
		if (res == NCSCC_RC_SUCCESS)
			m_LOG_GLD_API(GLD_SE_API_CREATE_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);
		else
			m_LOG_GLD_API(GLD_SE_API_CREATE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		break;

	case NCS_LIB_REQ_DESTROY:
		/* need to fetch the information from the "NCS_LIB_REQ_INFO" struct - TBD */
		res = gld_se_lib_destroy(req_info);
		if (res == NCSCC_RC_SUCCESS)
			m_LOG_GLD_API(GLD_SE_API_DESTROY_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);
		else
			m_LOG_GLD_API(GLD_SE_API_DESTROY_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
#if (NCS_GLSV_LOG == 1)
		gld_flx_log_dereg();
#endif
		break;

	default:
		m_LOG_GLD_API(GLD_SE_API_UNKNOWN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		break;
	}
	return (res);
}

/****************************************************************************
 * Name          : gld_se_lib_init
 *
 * Description   : Invoked to Initialize the GLD
 *                 
 *
 * Arguments     : 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_se_lib_init(NCS_LIB_REQ_INFO *req_info)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT amf_error;
	uns32 res = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT Healthy;
	int8 *health_key;

	/* Register with Logging subsystem */
	if (NCS_GLSV_LOG == 1)
		gld_flx_log_reg();

	/* Allocate and initialize the control block */
	gld_cb = m_MMGR_ALLOC_GLSV_GLD_CB;

	if (gld_cb == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_CB_ALLOC_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(gld_cb, 0, sizeof(GLSV_GLD_CB));

	/* TBD- Pool id is to be set */
	gl_gld_hdl = gld_cb->my_hdl = ncshm_create_hdl(gld_cb->hm_poolid, NCS_SERVICE_ID_GLD, (NCSCONTEXT)gld_cb);
	if (0 == gld_cb->my_hdl) {
		m_LOG_GLD_HEADLINE(GLD_CREATE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return NCSCC_RC_FAILURE;
	}

	/* Initialize the cb parameters */
	if (gld_cb_init(gld_cb) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return NCSCC_RC_FAILURE;
	}

	/* Initialize amf framework */
	if (gld_amf_init(gld_cb) != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_INIT_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return NCSCC_RC_FAILURE;
	}
	m_LOG_GLD_SVC_PRVDR(GLD_AMF_INIT_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);

	/* Bind to MDS */
	if (gld_mds_init(gld_cb) != NCSCC_RC_SUCCESS) {
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_INSTALL_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	} else
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_INSTALL_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);

	/*   Initialise with the MBCSV service  */
	if (glsv_gld_mbcsv_register(gld_cb) != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return NCSCC_RC_FAILURE;

	} else {
		m_LOG_GLD_MBCSV(GLD_MBCSV_INIT_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);

	}

	/* register glsv with imm */
	amf_error = gld_imm_init(gld_cb);
	if (amf_error != SA_AIS_OK) {
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		gld_log(NCSFL_SEV_ERROR, "Imm Init Failed %u\n", amf_error);
		return NCSCC_RC_FAILURE;
	}

	/* TASK CREATION AND INITIALIZING THE MAILBOX */
	if ((m_NCS_IPC_CREATE(&gld_cb->mbx) != NCSCC_RC_SUCCESS) ||
	    (m_NCS_IPC_ATTACH(&gld_cb->mbx) != NCSCC_RC_SUCCESS) ||
	    (m_NCS_TASK_CREATE((NCS_OS_CB)gld_main_process,
			       &gld_cb->mbx, "GLD", m_GLD_TASK_PRIORITY,
			       m_GLD_STACKSIZE,
			       &gld_cb->task_hdl) != NCSCC_RC_SUCCESS) ||
	    (m_NCS_TASK_START(gld_cb->task_hdl) != NCSCC_RC_SUCCESS)) {
		m_LOG_GLD_HEADLINE(GLD_IPC_TASK_INIT, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_NCS_TASK_RELEASE(gld_cb->task_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return (NCSCC_RC_FAILURE);
	}

	m_NCS_EDU_HDL_INIT(&gld_cb->edu_hdl);

	/* register GLD component with AvSv */
	amf_error = saAmfComponentRegister(gld_cb->amf_hdl, &gld_cb->comp_name, (SaNameT *)NULL);
	if (amf_error != SA_AIS_OK) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_REG_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		m_NCS_EDU_HDL_FLUSH(&gld_cb->edu_hdl);
		m_NCS_TASK_RELEASE(gld_cb->task_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		return NCSCC_RC_FAILURE;
	} else
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_REG_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);

   /** start the AMF health check **/
	memset(&Healthy, 0, sizeof(Healthy));
	health_key = (int8 *)getenv("GLSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		if (strlen("A1B2") < sizeof(Healthy.key))
			strncpy((char *)Healthy.key, "A1B2", sizeof(Healthy.key));
		m_LOG_GLD_HEADLINE(GLD_HEALTH_KEY_DEFAULT_SET, NCSFL_SEV_INFO, __FILE__, __LINE__, 0);
	} else {
		if (strlen((char *)health_key) < sizeof(Healthy.key))
			strncpy((char *)Healthy.key, (char *)health_key, SA_AMF_HEALTHCHECK_KEY_MAX - 1);
	}
	Healthy.keyLen = strlen((char *)Healthy.key);

	amf_error = saAmfHealthcheckStart(gld_cb->amf_hdl, &gld_cb->comp_name, &Healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);
	if (amf_error != SA_AIS_OK) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_HLTH_CHK_START_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		saAmfComponentUnregister(gld_cb->amf_hdl, &gld_cb->comp_name, (SaNameT *)NULL);
		m_NCS_EDU_HDL_FLUSH(&gld_cb->edu_hdl);
		m_NCS_TASK_RELEASE(gld_cb->task_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
	} else
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_HLTH_CHK_START_DONE, NCSFL_SEV_INFO, __FILE__, __LINE__);

	return (res);
}

/****************************************************************************
 * Name          : gld_se_lib_destroy
 *
 * Description   : Invoked to destroy the GLD
 *                 
 *
 * Arguments     : 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_se_lib_destroy(NCS_LIB_REQ_INFO *req_info)
{
	GLSV_GLD_CB *gld_cb;

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return (NCSCC_RC_FAILURE);
	} else {
		/* Disconnect from MDS */
		gld_mds_shut(gld_cb);

		saAmfComponentUnregister(gld_cb->amf_hdl, &gld_cb->comp_name, (SaNameT *)NULL);
		saAmfFinalize(gld_cb->amf_hdl);

		ncshm_give_hdl(gl_gld_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
		m_NCS_IPC_DETACH(&gld_cb->mbx, gld_clear_mbx, gld_cb);

		gld_cb_destroy(gld_cb);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
	}
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : gld_cb_init
 *
 * Description   : This function is invoked at init time. Initiaziles all the
 *                 parameters in the CB
 *
 * Arguments     : gld_cb  - GLD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_cb_init(GLSV_GLD_CB *gld_cb)
{
	NCS_PATRICIA_PARAMS params;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	/* Intialize all the patrica trees */
	params.key_size = sizeof(uns32);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->glnd_details, &params))
	    != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
	gld_cb->glnd_details_tree_up = TRUE;

	params.key_size = sizeof(uns32);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->rsc_info_id, &params))
	    != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
	gld_cb->rsc_info_id_tree_up = TRUE;

	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->rsc_map_info, &params))
	    != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* Initialize the next resource id */
	gld_cb->nxt_rsc_id = 1;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_cb_destroy
 *
 * Description   : This function is invoked at destroy time. This function will
 *                 free all the dynamically allocated memory
 *
 * Arguments     : gld_cb  - GLD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_cb_destroy(GLSV_GLD_CB *gld_cb)
{
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_NODE_LIST *node_list;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;

	/* destroy the patricia trees */
	while ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uns8 *)0))) {
		while ((glnd_rsc =
			(GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uns8 *)0))) {
			if (ncs_patricia_tree_del(&node_details->rsc_info_tree, (NCS_PATRICIA_NODE *)glnd_rsc) !=
			    NCSCC_RC_SUCCESS) {
				m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_DEL_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
						   0);
				return NCSCC_RC_FAILURE;
			}
			m_MMGR_FREE_GLSV_GLD_GLND_RSC_REF(glnd_rsc);
		}
		ncs_patricia_tree_destroy(&node_details->rsc_info_tree);
		if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_DEL_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
					   node_details->node_id);
			return NCSCC_RC_FAILURE;
		}

		m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
	}

	while ((rsc_info = (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id, (uns8 *)0))) {
		/* Free the node list */
		while (rsc_info->node_list != NULL) {
			node_list = rsc_info->node_list;
			rsc_info->node_list = node_list->next;
			m_MMGR_FREE_GLSV_NODE_LIST(node_list);
		}

		gld_free_rsc_info(gld_cb, rsc_info);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_BOOL gld_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	GLSV_GLD_EVT *pEvt = (GLSV_GLD_EVT *)msg;
	GLSV_GLD_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		gld_evt_destroy(pEvt);
		pEvt = pnext;
	}
	return TRUE;
}

/****************************************************************************
 * Name          : gld_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 GLD 
 *
 * Arguments     : mbx  - This is the mail box pointer on which IfD/IfND is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void gld_process_mbx(SYSF_MBX *mbx)
{
	GLSV_GLD_EVT *evt = GLSV_GLD_EVT_NULL;

	while (GLSV_GLD_EVT_NULL != (evt = (GLSV_GLD_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, evt))) {
		if ((evt->evt_type >= GLSV_GLD_EVT_BASE) && (evt->evt_type < GLSV_GLD_EVT_MAX)) {
			/* This event belongs to GLD */
			gld_process_evt(evt);
		} else {
			m_LOG_GLD_HEADLINE(GLD_UNKNOWN_EVT_RCVD, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
			m_MMGR_FREE_GLSV_GLD_EVT(evt);
		}
	}
	return;
}

/****************************************************************************
 * Name          : gld_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 GLD task.
 *                 This function will be select of both the FD's (AMF FD and
 *                 Mail Box FD), depending on which FD has been selected, it
 *                 will call the corresponding routines.
 *
 * Arguments     : mbx  - This is the mail box pointer on which GLD is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
static void gld_main_process(SYSF_MBX *mbx)
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;
	GLSV_GLD_CB *gld_cb = NULL;
	NCS_MBCSV_ARG mbcsv_arg;
	SaSelectionObjectT amf_sel_obj;

	if ((gld_cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return;
	}
	mbx_fd = ncs_ipc_get_sel_obj(&gld_cb->mbx);
	error = saAmfSelectionObjectGet(gld_cb->amf_hdl, &amf_sel_obj);

	if (error != SA_AIS_OK) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_SEL_OBJ_GET_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}

	/* Set up all file descriptors to listen to */
	fds[FD_AMF].fd = amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_MBCSV].fd = gld_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = gld_cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

	while (1) {
		if (gld_cb->immOiHandle != 0) {
			fds[FD_IMM].fd = gld_cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = FD_IMM + 1;
		} else {
			nfds = FD_IMM;
		}

		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			gld_log(NCSFL_SEV_ERROR, "poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_AMF].revents & POLLIN) {
			if (gld_cb->amf_hdl != 0) {
				/* dispatch all the AMF pending function */
				error = saAmfDispatch(gld_cb->amf_hdl, SA_DISPATCH_ALL);
				if (error != SA_AIS_OK) {
					m_LOG_GLD_SVC_PRVDR(GLD_AMF_DISPATCH_ERROR, NCSFL_SEV_ERROR, __FILE__,
							    __LINE__);
				}
			} else
				gld_log(NCSFL_SEV_ERROR, "gld_cb->amf_hdl == 0");
		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			/* dispatch all the MBCSV pending callbacks */
			mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
			mbcsv_arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
			mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;
			if (ncs_mbcsv_svc(&mbcsv_arg) != SA_AIS_OK) {
				m_LOG_GLD_HEADLINE(GLD_MBCSV_DISPATCH_FAILURE, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
			}
		}

		if (fds[FD_MBX].revents & POLLIN)
			gld_process_mbx(mbx);

		/* process the IMM messages */
		if (gld_cb->immOiHandle && fds[FD_IMM].revents & POLLIN) {
			/* dispatch all the IMM pending function */
			error = saImmOiDispatch(gld_cb->immOiHandle, SA_DISPATCH_ONE);

			/*
			 ** BAD_HANDLE is interpreted as an IMM service restart. Try 
			 ** reinitialize the IMM OI API in a background thread and let 
			 ** this thread do business as usual especially handling write 
			 ** requests.
			 **
			 ** All other errors are treated as non-recoverable (fatal) and will
			 ** cause an exit of the process.
			 */
			if (error == SA_AIS_ERR_BAD_HANDLE) {
				gld_log(NCSFL_SEV_ERROR, "saImmOiDispatch returned BAD_HANDLE %u", error);

				/* 
				 ** Invalidate the IMM OI handle, this info is used in other
				 ** locations. E.g. giving TRY_AGAIN responses to a create and
				 ** close resource requests. That is needed since the IMM OI
				 ** is used in context of these functions.
				 */
				gld_cb->immOiHandle = 0;
				gld_imm_reinit_bg(gld_cb);
			} else if (error != SA_AIS_OK) {
				gld_log(NCSFL_SEV_ERROR, "saImmOiDispatch FAILED: %u", error);
				break;
			}
		}

	}
	return;
}

/****************************************************************************
 * Name          : gld_dump_cb
 *
 * Description   : This is the function dumps the contents of the control block.
 *
 * Arguments     : gld_cb  -  Pointer to the control block
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void gld_dump_cb()
{
	GLSV_GLD_CB *gld_cb = NULL;
	GLSV_GLD_GLND_DETAILS *node_details;
	MDS_DEST mds_dest_id;
	GLSV_GLD_RSC_INFO *rsc_info;
	SaLckResourceIdT rsc_id = 0;
	uns32 node_id = 0;

	gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
	if (!gld_cb) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return;
	}

	memset(&mds_dest_id, 0, sizeof(MDS_DEST));

	TRACE("************ GLD CB info *************** ");
	/* print Amf Info */
	TRACE("AMF HA state : %d ", gld_cb->ha_state);
	/* print the Node details */
	TRACE("GLND info :");
	while ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details,
										  (uns8 *)&node_id))) {
		node_id = node_details->node_id;
		TRACE("Node Id - :%d ", node_details->node_id);
	}

	/* print the Resource details */
	while ((rsc_info = (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id, (uns8 *)&rsc_id))) {
		GLSV_NODE_LIST *list;
		rsc_id = rsc_info->rsc_id;
		TRACE("Resource Id - : %d  Resource Name - %.10s ", (uns32)rsc_info->rsc_id, rsc_info->lck_name.value);
		TRACE("Can Orphan - %d Mode - %d ", rsc_info->can_orphan, (uns32)rsc_info->orphan_lck_mode);
		list = rsc_info->node_list;
		TRACE("List of Nodes :");
		while (list != NULL) {
			TRACE("%d    ", m_NCS_NODE_ID_FROM_MDS_DEST(list->dest_id));
			list = list->next;
		}
	}
	ncshm_give_hdl(gl_gld_hdl);
	TRACE("************************************************** ");

}
