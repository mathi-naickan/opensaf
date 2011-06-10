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
  FILE NAME: cpnd_init.c

  DESCRIPTION: APIs used to initialize & Destroy the CPND Service Part.

  FUNCTIONS INCLUDED in this module:
  cpnd_lib_req ............ SE API to init and create PWE for CPND
  cpnd_lib_init ........... Function used to init CPND.  
  cpnd_lib_destroy ........ Function used to destroy CPND.
  cpnd_main_process ...........Process all the events posted to CPND.
******************************************************************************/

#include "cpnd.h"

#define CPND_CLM_API_TIMEOUT 10000000000LL
uns32 gl_cpnd_cb_hdl = 0;

/* Static Function Declerations */
static uns32 cpnd_extract_create_info(int argc, char *argv[], CPND_CREATE_INFO *create_info);

static uns32 cpnd_extract_destroy_info(int argc, char *argv[], CPND_DESTROY_INFO *destroy_info);

static uns32 cpnd_lib_init(CPND_CREATE_INFO *info);

static uns32 cpnd_lib_destroy(CPND_DESTROY_INFO *info);

static uns32 cpnd_cb_db_init(CPND_CB *cb);

static uns32 cpnd_cb_db_destroy(CPND_CB *cb);

static NCS_BOOL cpnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);

static void cpnd_main_process(NCSCONTEXT info);

/****************************************************************************
 * Name          : cpnd_lib_req
 *
 * Description   : This is the SE API which is used to init/destroy or 
 *                 Create/destroy PWE's of CPND. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uns32 rc = NCSCC_RC_FAILURE;
	CPND_CREATE_INFO create_info;
	CPND_DESTROY_INFO destroy_info;

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		if (cpnd_extract_create_info(req_info->info.create.argc,
					     req_info->info.create.argv, &create_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = cpnd_lib_init(&create_info);
		break;
	case NCS_LIB_REQ_DESTROY:
		if (cpnd_extract_destroy_info(req_info->info.create.argc,
					      req_info->info.create.argv, &destroy_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = cpnd_lib_destroy(&destroy_info);
		break;
	default:
		break;
	}
	return (rc);
}

/****************************************************************************
 * Name          : cpnd_extract_create_info
 *
 * Description   : Used to extract the create information from argc & agrv
 *
 * Arguments     : argc  - This is the Number of arguments received.
 *                 argv  - string received.
 *                 create_info - Structure to be filled for initing the service.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_extract_create_info(int argc, char *argv[], CPND_CREATE_INFO *create_info)
{

	memset(create_info, 0, sizeof(CPND_CREATE_INFO));

	/* Need to change this once we get these parameters in the argv */
	create_info->pool_id = NCS_HM_POOL_ID_COMMON;

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpnd_extract_destroy_info
 *
 * Description   : Used to extract the destroy information from argc & agrv
 *
 * Arguments     : argc  - This is the Number of arguments received.
 *                 argv  - string received.
 *                 destroy_info - Structure to be filled for initing the service.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_extract_destroy_info(int argc, char *argv[], CPND_DESTROY_INFO *destroy_info)
{

	memset(destroy_info, 0, sizeof(CPND_DESTROY_INFO));

	/* Need to fill this once we get these parameters in the argv */

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpnd_lib_init
 *
 * Description   : This is the function which initalize the CPND libarary.
 *                 This function creates an IPC mail Box and spawns CPND
 *                 thread.
 *                 This function initializes the CB, handle manager, MDS, CPD 
 *                 and Registers with AMF with respect to the component Type 
 *                 (CPND).
 *
 * Arguments     : create_info: pointer to struct CPND_CREATE_INFO.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_lib_init(CPND_CREATE_INFO *info)
{
	CPND_CB *cb = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT healthy;
	uns8 *health_key;
	SaAisErrorT amf_error;
	NCS_OS_POSIX_SHM_REQ_INFO cpnd_open_req;
	void *shm_ptr;
	GBL_SHM_PTR gbl_shm_addr;
	memset(&cpnd_open_req, '\0', sizeof(cpnd_open_req));
	SaVersionT clm_version;
	SaClmClusterNodeT cluster_node;
	SaClmHandleT clmHandle;

	m_CPSV_GET_AMF_VER(clm_version);
	SaClmCallbacksT gen_cbk;

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));

	/* register with the Flex log service */
	cpnd_flx_log_reg();

	/* allocate a CB  */
	cb = m_MMGR_ALLOC_CPND_CB;

	if (cb == NULL) {
		m_LOG_CPND_CL(CPND_CB_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_OUT_OF_MEM;
		goto cpnd_cb_alloc_fail;
	}
	memset(cb, 0, sizeof(CPND_CB));
	cb->pool_id = info->pool_id;
	cb->cli_id_gen = 1;
	/* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&cb->cpnd_edu_hdl);

	if ((rc = cpnd_cb_db_init(cb)) == NCSCC_RC_FAILURE) {
		m_LOG_CPND_CL(CPND_CB_DB_INIT_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_cb_init_fail;
	}

	if ((cb->cpnd_cb_hdl_id = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_CPND, (NCSCONTEXT)cb)) == 0) {
		m_LOG_CPND_CL(CPND_CB_HDL_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		goto cpnd_hdl_fail;
	}

	/* Store the handle in some global location */
	m_CPND_STORE_CB_HDL(cb->cpnd_cb_hdl_id);

	/* create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&cb->cpnd_mbx)) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_IPC_CREATE_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_ipc_create_fail;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&cb->cpnd_mbx)) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_IPC_ATTACH_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_ipc_att_fail;
	}
	gen_cbk.saClmClusterNodeGetCallback = NULL;
	gen_cbk.saClmClusterTrackCallback = cpnd_clm_cluster_track_cb;
	rc = saClmInitialize(&clmHandle, &gen_cbk, &clm_version);
	if (rc != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_CLM_INIT_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_clm_init_fail;
	}
	cb->clm_hdl = clmHandle;

        if (SA_AIS_OK != (rc = saClmSelectionObjectGet(cb->clm_hdl, &cb->clm_sel_obj))) {
                m_LOG_CPND_CL(CPND_CLM_INIT_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
                return rc;
        }

	rc = saClmClusterNodeGet(cb->clm_hdl, SA_CLM_LOCAL_NODE_ID, CPND_CLM_API_TIMEOUT, &cluster_node);
	if (rc != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_CLM_NODE_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_clm_fail;
	}
	cb->nodeid = cluster_node.nodeId;

	rc = saClmClusterTrack(cb->clm_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL);
	if (rc != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_CLM_INIT_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_clm_fail;
	}
	/* Initialise with the AMF service */
	if (cpnd_amf_init(cb) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_AMF_INIT_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto amf_init_err;
	}

	/* register with the AMF service */
	if (cpnd_amf_register(cb) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_AMF_REGISTER_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto amf_reg_err;
	}

	/* Create the task for CPND */
	if ((rc = m_NCS_TASK_CREATE((NCS_OS_CB)cpnd_main_process,
				    (NCSCONTEXT)cb, m_CPND_TASKNAME, m_CPND_TASK_PRI,
				    m_CPND_STACKSIZE, &cb->task_hdl)) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_TASK_CREATE_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_task_create_fail;
	}

	if ((rc = m_NCS_TASK_START(cb->task_hdl))
	    != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_TASK_START_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_task_start_fail;
	}

	/* CODE  FOR THE NO REDUNDANCY */
	memset(&gbl_shm_addr, 0, sizeof(GBL_SHM_PTR));
	shm_ptr = cpnd_restart_shm_create(&cpnd_open_req, cb, cluster_node.nodeId);
	if (shm_ptr) {
		gbl_shm_addr.base_addr = shm_ptr;	/* Store base address of shared memory, but not used for any operations as of now */
		gbl_shm_addr.cli_addr = shm_ptr + sizeof(CPND_SHM_VERSION);
		gbl_shm_addr.ckpt_addr = (void *)((char *)gbl_shm_addr.cli_addr + sizeof(CLIENT_HDR) +
						  (MAX_CLIENTS * sizeof(CLIENT_INFO)));
		cb->shm_addr = gbl_shm_addr;
	}

	if ((rc = cpnd_mds_register(cb)) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CL(CPND_MDS_REGISTER_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto cpnd_mds_fail;
	}

	if (!cb->cpnd_first_time) {
		cpnd_proc_app_status(cb);
	}

	/*   start the AMF Health Check  */
	memset(&healthy, 0, sizeof(healthy));

	health_key = (uns8 *)getenv("CPSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		strcpy((char *)healthy.key, "A1B2");
	} else {
		if (strlen((char *)health_key) >= SA_AMF_HEALTHCHECK_KEY_MAX) {
			rc = NCSCC_RC_FAILURE;
			m_LOG_CPND_CL(CPND_INIT_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			return rc;
		} else
			strcpy((char *)healthy.key, (char *)health_key);
	}
	healthy.keyLen = strlen((char *)healthy.key);

	amf_error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_HLTH_CHK_START_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}

	m_LOG_CPND_CL(CPND_INIT_SUCCESS, CPND_FC_HDLN, NCSFL_SEV_NOTICE, __FILE__, __LINE__);
	return NCSCC_RC_SUCCESS;

 amf_reg_err:
	cpnd_amf_de_init(cb);
 amf_init_err:
	cpnd_mds_unregister(cb);

 cpnd_mds_fail:
	m_NCS_TASK_STOP(cb->task_hdl);

 cpnd_clm_fail:
	saClmFinalize(cb->clm_hdl);

 cpnd_clm_init_fail:
 cpnd_task_start_fail:
	m_NCS_TASK_RELEASE(cb->task_hdl);

 cpnd_task_create_fail:
	m_NCS_IPC_DETACH(&cb->cpnd_mbx, cpnd_clear_mbx, cb);

 cpnd_ipc_att_fail:
	m_NCS_IPC_RELEASE(&cb->cpnd_mbx, NULL);

 cpnd_ipc_create_fail:
	ncshm_destroy_hdl(NCS_SERVICE_ID_CPND, cb->cpnd_cb_hdl_id);

 cpnd_hdl_fail:
	cpnd_cb_db_destroy(cb);

 cpnd_cb_init_fail:
	m_MMGR_FREE_CPND_CB(cb);

	m_LOG_CPND_CL(CPND_INIT_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
 cpnd_cb_alloc_fail:
	cpnd_flx_log_dereg();
	return (rc);
}

/****************************************************************************
 * Name          : cpnd_lib_destroy
 *
 * Description   : This is the function which destroy the cpnd libarary.
 *                 This function releases the Task and the IPX mail Box.
 *                 This function unregisters with AMF, destroies handle 
 *                 manager, CB and clean up all the component specific 
 *                 databases.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_lib_destroy(CPND_DESTROY_INFO *info)
{
	CPND_CB *cb = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	m_CPND_RETRIEVE_CB(cb);
	if (cb == NULL) {
		m_LOG_CPND_CL(CPND_DESTROY_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return (NCSCC_RC_FAILURE);
	}

	m_NCS_TASK_STOP(cb->task_hdl);

	m_NCS_TASK_RELEASE(cb->task_hdl);

	m_NCS_IPC_DETACH(&cb->cpnd_mbx, cpnd_clear_mbx, cb);

	m_NCS_IPC_RELEASE(&cb->cpnd_mbx, NULL);

	ncshm_destroy_hdl(NCS_SERVICE_ID_CPND, cb->cpnd_cb_hdl_id);

	cpnd_flx_log_dereg();

	cpnd_mds_unregister(cb);

	/* deregister with the AMF */
	if (cpnd_amf_deregister(cb) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* uninitialise with AMF */
	cpnd_amf_de_init(cb);

	ncshm_give_hdl(cb->cpnd_cb_hdl_id);

	cpnd_cb_db_destroy(cb);

	m_MMGR_FREE_CPND_CB(cb);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_cb_db_init
 *
 * Description   : This is the function which initializes all the data 
 *                 structures and locks used belongs to CPND.
 *
 * Arguments     : cb  - CPND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_cb_db_init(CPND_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	rc = cpnd_ckpt_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE) {
		m_LOG_CPND_CL(CPND_CKPT_NODE_TREE_INIT_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	rc = cpnd_client_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE)
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_CL(CPND_CLIENT_NODE_TREE_INIT_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, __FILE__,
				      __LINE__);
			return rc;
		}

	/* cb->evt_bckup_q=NULL; */
	rc = cpnd_allrepl_write_evt_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE) {
		/* LOG */
		return rc;
	}

	return (rc);
}

/****************************************************************************
 * Name          : cpnd_cb_db_destroy
 *
 * Description   : Destoroy the databases in CB
 *
 * Arguments     : cb  - cpsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_cb_db_destroy(CPND_CB *cb)
{

	cpnd_ckpt_node_tree_destroy(cb);
	cpnd_client_node_tree_destroy(cb);
	cpnd_allrepl_write_evt_node_tree_destroy(cb);
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpnd_clear_mbx
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
static NCS_BOOL cpnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	CPSV_EVT *pEvt = (CPSV_EVT *)msg;
	CPSV_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		cpnd_evt_destroy(pEvt);
		pEvt = pnext;
	}

	return TRUE;
}

/****************************************************************************
 * Name          : cpnd_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 CPND task.
 *                 This function will be select of both the FD's (AMF FD and
 *                 Mail Box FD), depending on which FD has been selected, it
 *                 will call the corresponding routines.
 *
 * Arguments     : mbx  - This is the mail box pointer on which IfD/IfND is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
static void cpnd_main_process(NCSCONTEXT info)
{
	CPND_CB *cb = (CPND_CB *)info;
	NCS_SEL_OBJ_SET all_sel_obj;
	NCS_SEL_OBJ mbx_fd;
	SYSF_MBX mbx = cb->cpnd_mbx;
	CPSV_EVT *evt = NULL;
	SaSelectionObjectT amf_sel_obj;
	SaAmfHandleT amf_hdl;
	SaAisErrorT amf_error;
	SaAisErrorT clm_error;
	NCS_SEL_OBJ amf_ncs_sel_obj, clm_ncs_sel_obj, highest_sel_obj;

	mbx_fd = m_NCS_IPC_GET_SEL_OBJ(&cb->cpnd_mbx);
	m_NCS_SEL_OBJ_ZERO(&all_sel_obj);
	m_NCS_SEL_OBJ_SET(mbx_fd, &all_sel_obj);

	amf_hdl = cb->amf_hdl;
	amf_error = saAmfSelectionObjectGet(amf_hdl, &amf_sel_obj);

	if (amf_error != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_GET_SEL_OBJ_FAILURE, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}
	m_SET_FD_IN_SEL_OBJ((uns32)amf_sel_obj, amf_ncs_sel_obj);
	m_NCS_SEL_OBJ_SET(amf_ncs_sel_obj, &all_sel_obj);

	highest_sel_obj = m_GET_HIGHER_SEL_OBJ(amf_ncs_sel_obj, mbx_fd);

	m_SET_FD_IN_SEL_OBJ((uns32)cb->clm_sel_obj, clm_ncs_sel_obj);
	m_NCS_SEL_OBJ_SET(clm_ncs_sel_obj, &all_sel_obj);
	highest_sel_obj = m_GET_HIGHER_SEL_OBJ(clm_ncs_sel_obj, highest_sel_obj);
	while (m_NCS_SEL_OBJ_SELECT(highest_sel_obj, &all_sel_obj, 0, 0, 0) != -1) {

		/* process all the AMF messages */
		if (m_NCS_SEL_OBJ_ISSET(amf_ncs_sel_obj, &all_sel_obj)) {
			/* dispatch all the AMF pending function */
			amf_error = saAmfDispatch(amf_hdl, SA_DISPATCH_ALL);
			if (amf_error != SA_AIS_OK) {
				m_LOG_CPND_CL(CPND_AMF_DISPATCH_FAILURE, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__,
					      __LINE__);
			}
		}

		if (m_NCS_SEL_OBJ_ISSET(clm_ncs_sel_obj, &all_sel_obj)) {
			clm_error = saClmDispatch(cb->clm_hdl, SA_DISPATCH_ALL);
			if (clm_error != SA_AIS_OK) {
				m_LOG_CPND_CL(CPND_AMF_DISPATCH_FAILURE, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__,
					      __LINE__);
			}
		}
		/* process the CPND Mail box */
		if (m_NCS_SEL_OBJ_ISSET(mbx_fd, &all_sel_obj)) {

			if (NULL != (evt = (CPSV_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&mbx, evt))) {
				/* now got the IPC mail box event */
				cpnd_process_evt(evt);
			}
		}
		m_NCS_SEL_OBJ_SET(clm_ncs_sel_obj, &all_sel_obj);
		m_NCS_SEL_OBJ_SET(amf_ncs_sel_obj, &all_sel_obj);
		m_NCS_SEL_OBJ_SET(mbx_fd, &all_sel_obj);

	}
	return;
}
