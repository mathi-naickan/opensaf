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

#include "base/daemon.h"
#include "ckpt/ckptnd/cpnd.h"
#include "base/osaf_poll.h"
#include "base/osaf_time.h"

enum {
	FD_TERM,
	FD_MBX,
	FD_AMF,
	FD_CLM,
	FD_CLM_UPDATED,
	NUMBER_OF_FDS
};

#define CPND_CLM_API_TIMEOUT 10000000000LL
uint32_t gl_cpnd_cb_hdl = 0;

/* Static Function Declerations */
static uint32_t cpnd_extract_create_info(int argc, char *argv[], CPND_CREATE_INFO *create_info);

static uint32_t cpnd_extract_destroy_info(int argc, char *argv[], CPND_DESTROY_INFO *destroy_info);

static uint32_t cpnd_lib_init(CPND_CREATE_INFO *info);

static uint32_t cpnd_lib_destroy(CPND_DESTROY_INFO *info);

static uint32_t cpnd_cb_db_init(CPND_CB *cb);

static uint32_t cpnd_cb_db_destroy(CPND_CB *cb);

static bool cpnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);

static SaAisErrorT cpnd_clm_init(void);

static SaAisErrorT cpnd_start_clm_init_bg();

void cpnd_main_process(CPND_CB *cb);

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
uint32_t cpnd_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_FAILURE;
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
static uint32_t cpnd_extract_create_info(int argc, char *argv[], CPND_CREATE_INFO *create_info)
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
static uint32_t cpnd_extract_destroy_info(int argc, char *argv[], CPND_DESTROY_INFO *destroy_info)
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
static uint32_t cpnd_lib_init(CPND_CREATE_INFO *info)
{
	CPND_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT healthy;
	uint8_t *health_key;
	SaAisErrorT amf_error;
	NCS_OS_POSIX_SHM_REQ_INFO cpnd_open_req;
	void *shm_ptr;
	GBL_SHM_PTR gbl_shm_addr;
	memset(&cpnd_open_req, '\0', sizeof(cpnd_open_req));

	char *ptr;

	TRACE_ENTER();

	/* allocate a CB  */
	cb = m_MMGR_ALLOC_CPND_CB;

	if (cb == NULL) {
		LOG_ER("cpnd cb memory allocation failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto cpnd_cb_alloc_fail;
	}
	memset(cb, 0, sizeof(CPND_CB));
	cb->pool_id = info->pool_id;
	cb->cli_id_gen = 1;
	/* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&cb->cpnd_edu_hdl);

	if ((rc = cpnd_cb_db_init(cb)) == NCSCC_RC_FAILURE) {
		LOG_ER("cpnd cb db init failed");
		goto cpnd_cb_init_fail;
	}

	if ((cb->cpnd_cb_hdl_id = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_CPND, (NCSCONTEXT)cb)) == 0) {
		LOG_ER("cpnd cb hdl create failed");
		rc = NCSCC_RC_FAILURE;
		goto cpnd_hdl_fail;
	}

	/* Store the handle in some global location */
	m_CPND_STORE_CB_HDL(cb->cpnd_cb_hdl_id);

	if (cpnd_get_scAbsenceAllowed_attr() != 0 ) {
		cb->scAbsenceAllowed = true;
		TRACE("cpnd scAbsenceAllowed = true");
	} else
		cb->scAbsenceAllowed = false;


	/* Get shm_alloc_guaranteed */
	if ((ptr = getenv("OSAF_CKPT_SHM_ALLOC_GUARANTEE")) != NULL) {
		cb->shm_alloc_guaranteed = atoi(ptr);
	} else {
		cb->shm_alloc_guaranteed = false;
	}

	/* create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&cb->cpnd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd ipc create fail");
		goto cpnd_ipc_create_fail;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&cb->cpnd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd ipc attach failed");
		goto cpnd_ipc_att_fail;
	}

	/* Initalize the CLM service */
	rc = cpnd_clm_init();
	if (rc != SA_AIS_OK) {
		LOG_ER("cpnd clm init failed with return value:%d",rc);
		goto cpnd_clm_init_fail;
	}

	/* Initialise with the AMF service */
	if (cpnd_amf_init(cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd amf init failed");
		goto amf_init_err;
	}

	/* register with the AMF service */
	if (cpnd_amf_register(cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd amf register failed");
		goto amf_reg_err;
	}

	/* CODE  FOR THE NO REDUNDANCY */
	memset(&gbl_shm_addr, 0, sizeof(GBL_SHM_PTR));
	shm_ptr = cpnd_restart_shm_create(&cpnd_open_req, cb, cb->nodeid);
	if (shm_ptr) {
		gbl_shm_addr.base_addr = shm_ptr;	/* Store base address of shared memory, but not used for any operations as of now */
		gbl_shm_addr.cli_addr = shm_ptr + sizeof(CPND_SHM_VERSION);
		gbl_shm_addr.ckpt_addr = (void *)((char *)gbl_shm_addr.cli_addr + sizeof(CLIENT_HDR) +
						  (MAX_CLIENTS * sizeof(CLIENT_INFO)));
		cb->shm_addr = gbl_shm_addr;
	} else {
		LOG_ER("cpnd restart_shm create failed");
		rc = NCSCC_RC_FAILURE;
		goto amf_reg_err;
	}

	if ((rc = cpnd_mds_register(cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd mds register failed");
		goto cpnd_mds_fail;
	}

	if (!cb->cpnd_first_time) {
		cpnd_proc_app_status(cb);
	}

	/*   start the AMF Health Check  */
	memset(&healthy, 0, sizeof(healthy));

	health_key = (uint8_t *)getenv("CPSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		strcpy((char *)healthy.key, "A1B2");
	} else {
		if (strlen((char *)health_key) >= SA_AMF_HEALTHCHECK_KEY_MAX) {
			rc = NCSCC_RC_FAILURE;
			LOG_ER("cpnd get health check key failed");
			TRACE_LEAVE();
			return rc;
		} else
			strcpy((char *)healthy.key, (char *)health_key);
	}
	healthy.keyLen = strlen((char *)healthy.key);

	amf_error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("cpnd amf hlth chk start failed ");
	}

	TRACE_1("cpnd init success ");
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 amf_reg_err:
	cpnd_amf_de_init(cb);
 amf_init_err:
	cpnd_mds_unregister(cb);

 cpnd_mds_fail:
	m_NCS_TASK_STOP(cb->task_hdl);

 cpnd_clm_init_fail:
	m_NCS_IPC_DETACH(&cb->cpnd_mbx, cpnd_clear_mbx, cb);

 cpnd_ipc_att_fail:
	m_NCS_IPC_RELEASE(&cb->cpnd_mbx, NULL);

 cpnd_ipc_create_fail:
	ncshm_destroy_hdl(NCS_SERVICE_ID_CPND, cb->cpnd_cb_hdl_id);

 cpnd_hdl_fail:
	cpnd_cb_db_destroy(cb);

 cpnd_cb_init_fail:
	m_MMGR_FREE_CPND_CB(cb);

	LOG_ER("cpnd init failed ");
 cpnd_cb_alloc_fail:
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
static uint32_t cpnd_lib_destroy(CPND_DESTROY_INFO *info)
{
	CPND_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	m_CPND_RETRIEVE_CB(cb);
	if (cb == NULL) {
		TRACE_4("cpnd retrieve cb failed");
		return (NCSCC_RC_FAILURE);
	}

	m_NCS_IPC_DETACH(&cb->cpnd_mbx, cpnd_clear_mbx, cb);

	m_NCS_IPC_RELEASE(&cb->cpnd_mbx, NULL);

	ncshm_destroy_hdl(NCS_SERVICE_ID_CPND, cb->cpnd_cb_hdl_id);


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
static uint32_t cpnd_cb_db_init(CPND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	rc = cpnd_ckpt_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE) {
		LOG_ER("cpnd ckpt node tree init failed ");
		TRACE_LEAVE();
		return rc;
	}

	rc = cpnd_client_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE)
		if (rc == NCSCC_RC_FAILURE) {
			LOG_ER("cpnd client node tree init failed ");
			TRACE_LEAVE();
			return rc;
		}

	/* cb->evt_bckup_q=NULL; */
	rc = cpnd_allrepl_write_evt_node_tree_init(cb);
	if (rc == NCSCC_RC_FAILURE) {
		/* LOG */
		LOG_ER("cpnd allrepl write evt node tree init failed");
		TRACE_LEAVE();
		return rc;
	}

	TRACE_LEAVE();
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
static uint32_t cpnd_cb_db_destroy(CPND_CB *cb)
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
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
static bool cpnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	CPSV_EVT *pEvt = (CPSV_EVT *)msg;
	CPSV_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		cpnd_evt_destroy(pEvt);
		pEvt = pnext;
	}

	return true;
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
void cpnd_main_process(CPND_CB *cb)
{
	NCS_SEL_OBJ mbx_fd;
	SYSF_MBX mbx = cb->cpnd_mbx;
	CPSV_EVT *evt = NULL;
	SaSelectionObjectT amf_sel_obj;
	SaAmfHandleT amf_hdl;
	SaAisErrorT amf_error;
	SaAisErrorT clm_error;
	struct pollfd fds[NUMBER_OF_FDS];
	int term_fd;

	ncs_sel_obj_create(&cb->clm_updated_sel_obj);
	fds[FD_CLM_UPDATED].fd = m_GET_FD_FROM_SEL_OBJ(cb->clm_updated_sel_obj);
	fds[FD_CLM_UPDATED].events = POLLIN;

	mbx_fd = ncs_ipc_get_sel_obj(&cb->cpnd_mbx);
	fds[FD_MBX].fd = m_GET_FD_FROM_SEL_OBJ(mbx_fd);
	fds[FD_MBX].events = POLLIN;

	amf_hdl = cb->amf_hdl;
	amf_error = saAmfSelectionObjectGet(amf_hdl, &amf_sel_obj);

	if (amf_error != SA_AIS_OK) {
		LOG_ER("cpnd amf get sel obj failure %u",amf_error);
		TRACE_LEAVE();
		return;
	}

	daemon_sigterm_install(&term_fd);

	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;
	fds[FD_AMF].fd = amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_CLM].fd = cb->clm_sel_obj;
	fds[FD_CLM].events = POLLIN;
	for (;;) {
		osaf_poll(fds, NUMBER_OF_FDS, -1);

		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (((fds[FD_AMF].revents | fds[FD_CLM].revents | fds[FD_MBX].revents) &
			(POLLERR | POLLHUP | POLLNVAL)) != 0) {
			LOG_ER("cpnd poll() failure: %hd %hd %hd",
					fds[FD_AMF].revents, fds[FD_CLM].revents, fds[FD_MBX].revents);
			TRACE_LEAVE();
			return;
		}

		/* process all the AMF messages */
		if (fds[FD_AMF].revents & POLLIN) {
			/* dispatch all the AMF pending function */
			amf_error = saAmfDispatch(amf_hdl, SA_DISPATCH_ALL);
			if (amf_error != SA_AIS_OK) {
				LOG_ER("cpnd amf dispatch failure %u",amf_error);
			}
		}

		/* process all the CLM messages */
		if (fds[FD_CLM].revents & POLLIN) {
			clm_error = saClmDispatch(cb->clm_hdl, SA_DISPATCH_ALL);
			if (clm_error == SA_AIS_ERR_BAD_HANDLE) {
				LOG_NO("Bad CLM handle. Reinitializing.");
				osaf_nanosleep(&kHundredMilliseconds);

				cpnd_start_clm_init_bg();

				/* Ignore the FD_CLM */
				fds[FD_CLM].fd = -1;
			} else if (clm_error != SA_AIS_OK) {
				LOG_ER("cpnd clm dispatch failure %u", clm_error);
			}
		}

		/* process the CPND Mail box */
		if (fds[FD_MBX].revents & POLLIN) {

			if (NULL != (evt = (CPSV_EVT *)ncs_ipc_non_blk_recv(&mbx))) {
				/* now got the IPC mail box event */
				cpnd_process_evt(evt);
			}
		}

		/* process the CLM object updated event */
		if (fds[FD_CLM_UPDATED].revents & POLLIN) {
			fds[FD_CLM].fd = cb->clm_sel_obj;
			LOG_NO("CLM selection object was updated. (%lld)", cb->clm_sel_obj);

			ncs_sel_obj_rmv_ind(&cb->clm_updated_sel_obj, true, true);
		}
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : cpnd_clm_init
 *
 * Description   : This function initialize CLM, get Node Id, and enalbe 
 *                 tracking callback.
 *
 * Arguments     : -
 *
 * Return Values : -
 *
 * Notes         : None.
 *****************************************************************************/
static SaAisErrorT cpnd_clm_init(void)
{	
	CPND_CB *cb = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	SaVersionT clm_version;
	SaClmClusterNodeT cluster_node;
	SaClmHandleT clmHandle;
	m_CPSV_GET_AMF_VER(clm_version);
	SaClmCallbacksT gen_cbk;

	cb = m_CPND_TAKE_CPND_CB;

	TRACE_ENTER();

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));

	gen_cbk.saClmClusterNodeGetCallback = NULL;
	gen_cbk.saClmClusterTrackCallback = cpnd_clm_cluster_track_cb;
	rc = saClmInitialize(&clmHandle, &gen_cbk, &clm_version);
	while (rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_TIMEOUT || 
		rc == SA_AIS_ERR_NO_RESOURCES || rc == SA_AIS_ERR_UNAVAILABLE) {
		if (rc != SA_AIS_ERR_TRY_AGAIN) {
			LOG_WA("cpnd_lib_init: saClmInitialize returned %u", rc);
		}
		osaf_nanosleep(&kHundredMilliseconds);
		rc = saClmInitialize(&clmHandle, &gen_cbk, &clm_version);
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("cpnd clm init failed with return value:%d",rc);
		goto cpnd_clm_initialize_fail;
	}

	cb->clm_hdl = clmHandle;

	if (SA_AIS_OK != (rc = saClmSelectionObjectGet(cb->clm_hdl, &cb->clm_sel_obj))) {
		LOG_ER("cpnd clm selection object Get failed with return value:%d",rc);
		goto cpnd_clm_fail;
	}

	TRACE("cpnd clm selection object = %lld", cb->clm_sel_obj);

	rc = saClmClusterNodeGet(cb->clm_hdl, SA_CLM_LOCAL_NODE_ID, CPND_CLM_API_TIMEOUT, &cluster_node);
	if (rc != SA_AIS_OK) {
		LOG_ER("cpnd clm node get failed with return value:%d",rc);
		goto cpnd_clm_fail;
	}
	cb->nodeid = cluster_node.nodeId;

	rc = saClmClusterTrack(cb->clm_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL);
	if (rc != SA_AIS_OK) {
		LOG_ER("cpnd clm clusterTrack failed with return value:%d",rc);
		goto cpnd_clm_fail;
	}

	TRACE_LEAVE();
	return rc;

 cpnd_clm_fail:
	saClmFinalize(cb->clm_hdl);

 cpnd_clm_initialize_fail:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : cpnd_clm_init_thread
 *
 * Description   : This function is thread function to initialize clm
 *
 * Arguments     : -
 *
 * Return Values : -
 *
 * Notes         : None.
 *****************************************************************************/
static void* cpnd_clm_init_thread(void* arg)
{
	CPND_CB *cb = m_CPND_TAKE_CPND_CB;

	TRACE_ENTER();

	SaAisErrorT rc = cpnd_clm_init();
	if (rc != SA_AIS_OK) {
		exit(EXIT_FAILURE);
	}

	/* Notify main process to update clm select object */
	ncs_sel_obj_ind(&cb->clm_updated_sel_obj);

	TRACE_LEAVE();
	return NULL;
}

/****************************************************************************
 * Name          : cpnd_start_clm_init_bg
 *
 * Description   : This function is to start initialize clm thread
 *
 * Arguments     : -
 *
 * Return Values : -
 *
 * Notes         : None.
 *****************************************************************************/
static SaAisErrorT cpnd_start_clm_init_bg()
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, cpnd_clm_init_thread, NULL) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	pthread_attr_destroy(&attr);

	return SA_AIS_OK;
}
