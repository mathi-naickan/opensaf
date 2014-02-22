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
#include <string.h>
uint32_t gl_gld_hdl;

void gld_main_process(SYSF_MBX *mbx);

enum {
	FD_TERM = 0,
	FD_AMF,
	FD_MBCSV,
	FD_MBX,
	FD_IMM,
	NUM_FD
};

static struct pollfd fds[NUM_FD];
static nfds_t nfds = NUM_FD;

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
uint32_t gld_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t res = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		/* need to fetch the information from the "NCS_LIB_REQ_INFO" struct - TBD */
		res = gld_se_lib_init(req_info);
		if (res == NCSCC_RC_SUCCESS)
			TRACE_1("GLD api create success");
		else
			LOG_ER("GLD api create failed");
		break;

	case NCS_LIB_REQ_DESTROY:
		/* need to fetch the information from the "NCS_LIB_REQ_INFO" struct - TBD */
		res = gld_se_lib_destroy(req_info);
		if (res == NCSCC_RC_SUCCESS)
			TRACE_1("GLD api destroy success");
		else
			LOG_ER("GLD api destroy failed");
		break;

	default:
		LOG_ER("GLD api unknown call");
		break;
	}
	TRACE_LEAVE();
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
uint32_t gld_se_lib_init(NCS_LIB_REQ_INFO *req_info)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT amf_error;
	uint32_t res = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT Healthy;
	int8_t *health_key;
	TRACE_ENTER();

	/* Allocate and initialize the control block */
	gld_cb = m_MMGR_ALLOC_GLSV_GLD_CB;

	if (gld_cb == NULL) {
		LOG_CR("Control block alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(gld_cb, 0, sizeof(GLSV_GLD_CB));

	/* TBD- Pool id is to be set */
	gl_gld_hdl = gld_cb->my_hdl = ncshm_create_hdl(gld_cb->hm_poolid, NCS_SERVICE_ID_GLD, (NCSCONTEXT)gld_cb);
	if (0 == gld_cb->my_hdl) {
		LOG_ER("Handle create failed");
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		goto end;
	}

	/* Initialize the cb parameters */
	if (gld_cb_init(gld_cb) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		TRACE_2("GLD cb init failed");	
		goto end;
	}

	/* Initialize amf framework */
	if (gld_amf_init(gld_cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("AMF Initialize error");
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		goto end;
	}
	TRACE_1("AMF Initialize success");

	/* Bind to MDS */
	if (gld_mds_init(gld_cb) != NCSCC_RC_SUCCESS) {
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		TRACE_2("MDS Install failed");
		res = NCSCC_RC_FAILURE;
		goto end;
	} else
		TRACE_1("MDS Install success");

	/*   Initialise with the MBCSV service  */
	if (glsv_gld_mbcsv_register(gld_cb) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLD mbcsv init failed");
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		goto end;

	} else {
		TRACE_1("GLD mbcsv init success");

	}

	/* register glsv with imm */
	amf_error = gld_imm_init(gld_cb);
	if (amf_error != SA_AIS_OK) {
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		LOG_ER("Imm Init Failed %u\n", amf_error);
		res = NCSCC_RC_FAILURE;
		goto end;
	}

	/* TASK CREATION AND INITIALIZING THE MAILBOX */
	if ((m_NCS_IPC_CREATE(&gld_cb->mbx) != NCSCC_RC_SUCCESS) ||
	    (m_NCS_IPC_ATTACH(&gld_cb->mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("Failure in task initiation");
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		goto end;
	}

	m_NCS_EDU_HDL_INIT(&gld_cb->edu_hdl);

	/* register GLD component with AvSv */
	amf_error = saAmfComponentRegister(gld_cb->amf_hdl, &gld_cb->comp_name, (SaNameT *)NULL);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("AMF Registration Failed");
		m_NCS_EDU_HDL_FLUSH(&gld_cb->edu_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
		res = NCSCC_RC_FAILURE;
		goto end;	
	} else
		TRACE_1("AMF Registration Success");

   /** start the AMF health check **/
	memset(&Healthy, 0, sizeof(Healthy));
	health_key = (int8_t *)getenv("GLSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		if (strlen("A1B2") < sizeof(Healthy.key))
			strncpy((char *)Healthy.key, "A1B2", sizeof(Healthy.key));
		TRACE_1("GLD health key default set");
	} else {
		if (strlen((char *)health_key) < sizeof(Healthy.key))
			strncpy((char *)Healthy.key, (char *)health_key, SA_AMF_HEALTHCHECK_KEY_MAX - 1);
	}
	Healthy.keyLen = strlen((char *)Healthy.key);

	amf_error = saAmfHealthcheckStart(gld_cb->amf_hdl, &gld_cb->comp_name, &Healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("AMF Health Check start failed");
		saAmfComponentUnregister(gld_cb->amf_hdl, &gld_cb->comp_name, (SaNameT *)NULL);
		m_NCS_EDU_HDL_FLUSH(&gld_cb->edu_hdl);
		m_NCS_IPC_RELEASE(&gld_cb->mbx, NULL);
		saImmOiFinalize(gld_cb->immOiHandle);
		glsv_gld_mbcsv_unregister(gld_cb);
		gld_mds_shut(gld_cb);
		saAmfFinalize(gld_cb->amf_hdl);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);
	} else
		TRACE_1("AMF Health Check started");
 end:
	TRACE_LEAVE();
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
uint32_t gld_se_lib_destroy(NCS_LIB_REQ_INFO *req_info)
{
	GLSV_GLD_CB *gld_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		rc = NCSCC_RC_FAILURE;
		goto end;
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
 end:
	TRACE_LEAVE();
	return rc;
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
uint32_t gld_cb_init(GLSV_GLD_CB *gld_cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	/* Intialize all the patrica trees */
	params.key_size = sizeof(uint32_t);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->glnd_details, &params))
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree init failed");
		rc =  NCSCC_RC_FAILURE;
		goto end;
	}
	gld_cb->glnd_details_tree_up = true;

	params.key_size = sizeof(uint32_t);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->rsc_info_id, &params))
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree init failed");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	gld_cb->rsc_info_id_tree_up = true;

	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&gld_cb->rsc_map_info, &params))
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree init failed");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	/* Initialize the next resource id */
	gld_cb->nxt_rsc_id = 1;
 end:
	TRACE_LEAVE();
	return rc;
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
uint32_t gld_cb_destroy(GLSV_GLD_CB *gld_cb)
{
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_NODE_LIST *node_list;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* destroy the patricia trees */
	while ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uint8_t *)0))) {
		while ((glnd_rsc =
			(GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0))) {
			if (ncs_patricia_tree_del(&node_details->rsc_info_tree, (NCS_PATRICIA_NODE *)glnd_rsc) !=
			    NCSCC_RC_SUCCESS) {
				LOG_ER("Patricia tree del failed");
				rc =  NCSCC_RC_FAILURE;
				goto end;
			}
			m_MMGR_FREE_GLSV_GLD_GLND_RSC_REF(glnd_rsc);
		}
		ncs_patricia_tree_destroy(&node_details->rsc_info_tree);
		if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed: node_id %u", node_details->node_id);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}

		m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
	}

	while ((rsc_info = (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id, (uint8_t *)0))) {
		/* Free the node list */
		while (rsc_info->node_list != NULL) {
			node_list = rsc_info->node_list;
			rsc_info->node_list = node_list->next;
			m_MMGR_FREE_GLSV_NODE_LIST(node_list);
		}

		gld_free_rsc_info(gld_cb, rsc_info);
	}
 end:
	TRACE_LEAVE();
	return rc;
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
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
bool gld_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	GLSV_GLD_EVT *pEvt = (GLSV_GLD_EVT *)msg;
	GLSV_GLD_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		gld_evt_destroy(pEvt);
		pEvt = pnext;
	}
	return true;
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
			LOG_ER("Unknown event received");
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
void gld_main_process(SYSF_MBX *mbx)
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;
	GLSV_GLD_CB *gld_cb = NULL;
	NCS_MBCSV_ARG mbcsv_arg;
	SaSelectionObjectT amf_sel_obj;
	int term_fd;

	TRACE_ENTER();

	if ((gld_cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		goto end;
	}
	gld_cb->is_impl_set = false;
	mbx_fd = ncs_ipc_get_sel_obj(&gld_cb->mbx);
	error = saAmfSelectionObjectGet(gld_cb->amf_hdl, &amf_sel_obj);

	if (error != SA_AIS_OK) {
		LOG_ER("AMF Selection object get error");
		goto end;
	}

	daemon_sigterm_install(&term_fd);

	/* Set up all file descriptors to listen to */
	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;
	fds[FD_AMF].fd = amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_MBCSV].fd = gld_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = gld_cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

	while (1) {
		if ((gld_cb->immOiHandle != 0) && (gld_cb->is_impl_set == true)){
			fds[FD_IMM].fd = gld_cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = NUM_FD;
		} else {
			nfds = NUM_FD - 1;
		}

		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (fds[FD_AMF].revents & POLLIN) {
			if (gld_cb->amf_hdl != 0) {
				/* dispatch all the AMF pending function */
				error = saAmfDispatch(gld_cb->amf_hdl, SA_DISPATCH_ALL);
				if (error != SA_AIS_OK) {
					TRACE_2("AMF Selection object get error");
				}
			} else
				TRACE_2("gld_cb->amf_hdl == 0");
		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			/* dispatch all the MBCSV pending callbacks */
			mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
			mbcsv_arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
			mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;
			if (ncs_mbcsv_svc(&mbcsv_arg) != SA_AIS_OK) {
				TRACE_2("GLD mbcsv dispatch failure");
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
				TRACE_2("saImmOiDispatch returned BAD_HANDLE %u", error);

				/* 
				 ** Invalidate the IMM OI handle, this info is used in other
				 ** locations. E.g. giving TRY_AGAIN responses to a create and
				 ** close resource requests. That is needed since the IMM OI
				 ** is used in context of these functions.
				 */
				gld_cb->immOiHandle = 0;
				gld_cb->is_impl_set = false;
				gld_imm_reinit_bg(gld_cb);
			} else if (error != SA_AIS_OK) {
				TRACE_2("saImmOiDispatch FAILED: %u", error);
				break;
			}
		}

	}
 end:
	TRACE_LEAVE();
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
	uint32_t node_id = 0;
	TRACE_ENTER();

	gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
	if (!gld_cb) {
		LOG_ER("Handle take failed");
		goto end;
	}

	memset(&mds_dest_id, 0, sizeof(MDS_DEST));

	TRACE("************ GLD CB info *************** ");
	/* print Amf Info */
	TRACE("AMF HA state : %d ", gld_cb->ha_state);
	/* print the Node details */
	TRACE("GLND info :");
	while ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details,
										  (uint8_t *)&node_id))) {
		node_id = node_details->node_id;
		TRACE("Node Id - :%d ", node_details->node_id);
	}

	/* print the Resource details */
	while ((rsc_info = (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id, (uint8_t *)&rsc_id))) {
		GLSV_NODE_LIST *list;
		rsc_id = rsc_info->rsc_id;
		TRACE("Resource Id - : %d  Resource Name - %.10s ", (uint32_t)rsc_info->rsc_id, rsc_info->lck_name.value);
		TRACE("Can Orphan - %d Mode - %d ", rsc_info->can_orphan, (uint32_t)rsc_info->orphan_lck_mode);
		list = rsc_info->node_list;
		TRACE("List of Nodes :");
		while (list != NULL) {
			TRACE("from mds_dest: %d", m_NCS_NODE_ID_FROM_MDS_DEST(list->dest_id));
			list = list->next;
		}
	}
	ncshm_give_hdl(gl_gld_hdl);
	TRACE("************************************************** ");
 end:	
	TRACE_LEAVE();
}
