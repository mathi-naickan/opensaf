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
  FILE NAME: cpd_init.c

  DESCRIPTION: APIs used to initialize & Destroy the cpd Service Part.

  FUNCTIONS INCLUDED in this module:
  cpd_lib_req ............ SE API to init and create PWE for CPD
  cpd_lib_init ........... Function used to init CPD.  
  cpd_lib_destroy ........ Function used to destroy CPD.
  cpd_main_process ........Process all the events posted to CPD.
******************************************************************************/

#include <poll.h>
#include <stdlib.h>

#include <daemon.h>
#include "cpd.h"
#include "cpd_imm.h"
#include <rda_papi.h>
#include "osaf_time.h"

enum {
	FD_TERM = 0,
	FD_AMF,
	FD_CLM,
	FD_MBCSV,
	FD_MBX,
	FD_IMM,
	NUM_FD
};

static struct pollfd fds[NUM_FD];
static nfds_t nfds = NUM_FD;
uint32_t gl_cpd_cb_hdl = 0;

/* Static Function Declerations */
static uint32_t cpd_extract_create_info(int argc, char *argv[], CPD_CREATE_INFO *create_info);

static uint32_t cpd_extract_destroy_info(int argc, char *argv[], CPD_DESTROY_INFO *destroy_info);

static uint32_t cpd_lib_init(CPD_CREATE_INFO *info);

static uint32_t cpd_lib_destroy(CPD_DESTROY_INFO *info);

static bool cpd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);

void cpd_main_process(CPD_CB *cb);

/****************************************************************************
 * Name          : cpd_lib_req
 *
 * Description   : This is the SE API which is used to init/destroy or 
 *                 Create/destroy PWE's of CPD. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t cpd_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	CPD_CREATE_INFO create_info;
	CPD_DESTROY_INFO destroy_info;

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		if (cpd_extract_create_info(req_info->info.create.argc,
					    req_info->info.create.argv, &create_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = cpd_lib_init(&create_info);
		break;
	case NCS_LIB_REQ_DESTROY:
		if (cpd_extract_destroy_info(req_info->info.create.argc,
					     req_info->info.create.argv, &destroy_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = cpd_lib_destroy(&destroy_info);
		break;
	default:
		break;
	}
	return (rc);
}

/****************************************************************************
 * Name          : cpd_extract_create_info
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
static uint32_t cpd_extract_create_info(int argc, char *argv[], CPD_CREATE_INFO *create_info)
{

	memset(create_info, 0, sizeof(CPD_CREATE_INFO));

	/* Need to change this once we get these parameters in the argv */
	create_info->pool_id = NCS_HM_POOL_ID_COMMON;

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpd_extract_destroy_info
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
static uint32_t cpd_extract_destroy_info(int argc, char *argv[], CPD_DESTROY_INFO *destroy_info)
{

	memset(destroy_info, 0, sizeof(CPD_DESTROY_INFO));

	/* Need to fill this once we get these parameters in the argv */

	return (NCSCC_RC_SUCCESS);
}

static SaAisErrorT cpd_clm_init(CPD_CB *cb)
{
	SaAisErrorT error;
	SaClmCallbacksT cpd_clm_cbk;

	/* Callbacks to register with CLM */
	cpd_clm_cbk.saClmClusterNodeGetCallback = NULL;
	cpd_clm_cbk.saClmClusterTrackCallback = cpd_clm_cluster_track_cb;

	for (;;) {
		SaVersionT clm_version;
		m_CPSV_GET_AMF_VER(clm_version);
		error = saClmInitialize(&cb->clm_hdl, &cpd_clm_cbk, &clm_version);
		if (error == SA_AIS_ERR_TRY_AGAIN ||
		    error == SA_AIS_ERR_TIMEOUT ||
                    error == SA_AIS_ERR_UNAVAILABLE) {
			if (error != SA_AIS_ERR_TRY_AGAIN) {
				LOG_WA("saClmInitialize returned %u",
				       (unsigned) error);
			}
			osaf_nanosleep(&kHundredMilliseconds);
			continue;
		}
		if (error == SA_AIS_OK) break;
		LOG_ER("Failed to Initialize with CLM: %u", error);
		return error;
	}

	error =	saClmSelectionObjectGet(cb->clm_hdl, &cb->clm_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("cpd clm selectionobjget failed %u",error);
		goto done;
	}

	/* Start Cluster Tracking */
	error = saClmClusterTrack(cb->clm_hdl, SA_TRACK_CHANGES_ONLY, NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("cpd clm cluster track failed %u",error);
		goto done;
        }

	return error;

done:
	saClmFinalize(cb->clm_hdl);
	
	return error;
}

/****************************************************************************
 * Name          : cpd_lib_init
 *
 * Description   : This is the function which initalize the CPD libarary.
 *                 This function creates an IPC mail Box and spawns CPD
 *                 thread.
 *                 This function initializes the CB, handle manager, MDS, CPD 
 *                 and Registers with AMF with respect to the component Type 
 *                 (CPD).
 *
 * Arguments     : create_info: pointer to struct CPD_CREATE_INFO.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t cpd_lib_init(CPD_CREATE_INFO *info)
{
	CPD_CB *cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_error;
	SaAmfHealthcheckKeyT healthy;
	int8_t *health_key;

	/* allocate a CB  */
	cb = m_MMGR_ALLOC_CPD_CB;

	if (cb == NULL) {
		LOG_ER("cpd cb memory allocation failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto cpd_cb_alloc_fail;
	}
	memset(cb, 0, sizeof(CPD_CB));
	cb->hm_poolid = info->pool_id;

	/* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	if ((rc = cpd_cb_db_init(cb)) == NCSCC_RC_FAILURE) {
		LOG_ER("CPD_CB_DB_INIT FAILED");
		goto cpd_cb_init_fail;
	}

	if ((cb->cpd_hdl = ncshm_create_hdl(cb->hm_poolid, NCS_SERVICE_ID_CPD, (NCSCONTEXT)cb)) == 0) {
		LOG_ER("cpd handle creation failed");
		rc = NCSCC_RC_FAILURE;
		goto cpd_hdl_fail;
	}

	/* Store the handle in some global location */
	m_CPD_STORE_CB_HDL(cb->cpd_hdl);

	if (cpd_get_scAbsenceAllowed_attr() != 0 ) {
		cb->scAbsenceAllowed = true;
		TRACE("cpd scAbsenceAllowed = true");
	} else
		cb->scAbsenceAllowed = false;

	/* create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&cb->cpd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mailbox creation failed");
		goto cpd_ipc_create_fail;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&cb->cpd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mailbox attach failed");
		goto cpd_ipc_att_fail;
	}

	/* Initialise with the AMF service */
	if (cpd_amf_init(cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd amf init failed");	
		goto amf_init_err;
	}

	cb->cold_or_warm_sync_on = true;

	/* register with the AMF service */
	if (cpd_amf_register(cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd amf register failed");
		goto amf_reg_err;
	}

	/* Request AMF for Healthcheck */
	memset(&healthy, 0, sizeof(healthy));
	health_key = (int8_t *)getenv("CPSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		strcpy((char *)healthy.key, "A1B2");
		/* TBD Log */
	} else {
		if (strlen((char *)health_key) >= SA_AMF_HEALTHCHECK_KEY_MAX) {
			rc = NCSCC_RC_FAILURE;
			LOG_ER("cpd health check key failed");
			goto cpd_mab_fail;
		} else
			strcpy((char *)healthy.key, (char *)health_key);
	}
	healthy.keyLen = strlen((char *)healthy.key);

	amf_error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (amf_error != SA_AIS_OK) {
		LOG_ER("cpd health check start failed");
	}

	/* Initialize with CLM */
	if (cpd_clm_init(cb) != SA_AIS_OK)
		goto cpd_mab_fail;

	if ((rc = initialize_for_assignment(cb, cb->ha_state)) !=
		NCSCC_RC_SUCCESS) {
		LOG_ER("initialize_for_assignment FAILED %u", (unsigned) rc);
		exit(EXIT_FAILURE);
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 cpd_mab_fail:
	cpd_amf_deregister(cb);

 amf_reg_err:
	cpd_amf_de_init(cb);

 amf_init_err:
	m_NCS_IPC_DETACH(&cb->cpd_mbx, cpd_clear_mbx, cb);

 cpd_ipc_att_fail:
	m_NCS_IPC_RELEASE(&cb->cpd_mbx, NULL);

 cpd_ipc_create_fail:
	ncshm_destroy_hdl(NCS_SERVICE_ID_CPD, cb->cpd_hdl);

 cpd_hdl_fail:
	cpd_cb_db_destroy(cb);

 cpd_cb_init_fail:
	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);
	m_MMGR_FREE_CPD_CB(cb);

 cpd_cb_alloc_fail:

	TRACE_LEAVE();
	return (rc);
}

uint32_t initialize_for_assignment(CPD_CB *cb, SaAmfHAStateT ha_state)
{
	TRACE_ENTER2("ha_state = %d", (int) ha_state);
	uint32_t rc = NCSCC_RC_SUCCESS;
	if (cb->fully_initialized || ha_state == SA_AMF_HA_QUIESCED) {
		goto done;
	}
	cb->ha_state = ha_state;
	if ((rc = cpd_mds_register(cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mds register failed");
		goto cpd_mds_fail;
	}
	/*   Initialise with the MBCSV service  */
	if ((rc = cpd_mbcsv_register(cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mbcsv register failed");
		goto mbcsv_reg_err;
	}

	if (cpd_imm_init(&cb->immOiHandle, &cb->imm_sel_obj) != SA_AIS_OK) {
		LOG_ER("cpd imm initialize failed ");
		rc = NCSCC_RC_FAILURE;
		goto cpd_imm_fail;
	}
	cb->fully_initialized = true;
done:
	TRACE_LEAVE();
	return rc;
cpd_imm_fail:
	saClmFinalize(cb->clm_hdl);
	cpd_mbcsv_finalize(cb);
mbcsv_reg_err:
	cpd_mds_unregister(cb);
cpd_mds_fail:
	TRACE_LEAVE2("rc = %u", rc);
	return rc;
}

/****************************************************************************
 * Name          : cpd_lib_destroy
 *
 * Description   : This is the function which destroy the cpd libarary.
 *                 This function releases the Task and the IPX mail Box.
 *                 This function unregisters with AMF, destroies handle 
 *                 manager, CB and clean up all the component specific 
 *                 databases.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t cpd_lib_destroy(CPD_DESTROY_INFO *info)
{
	CPD_CB *cb;
	uint32_t rc = NCSCC_RC_SUCCESS;

	m_CPD_RETRIEVE_CB(cb);
	if (cb == NULL) {
		LOG_ER("cpd destroy fail");
		return (NCSCC_RC_FAILURE);
	}

	m_NCS_IPC_DETACH(&cb->cpd_mbx, cpd_clear_mbx, cb);

	m_NCS_IPC_RELEASE(&cb->cpd_mbx, NULL);

	ncshm_destroy_hdl(NCS_SERVICE_ID_CPD, cb->cpd_hdl);

	saClmFinalize(cb->clm_hdl);

	cpd_mds_unregister(cb);

	/* deregister with the AMF */
	if (cpd_amf_deregister(cb) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* uninitialise with AMF */
	cpd_amf_de_init(cb);

	/* deregister with MBCSV */
	cpd_mbcsv_finalize(cb);

	ncshm_give_hdl(cb->cpd_hdl);

	cpd_cb_db_destroy(cb);

	m_MMGR_FREE_CPD_CB(cb);

	if (rc == NCSCC_RC_SUCCESS) {
		TRACE_2("cpd destroy success");
	} else {
		TRACE_4("cpd destroy failed");
	}
	return rc;
}

/****************************************************************************
 * Name          : cpd_clear_mbx
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
static bool cpd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	CPSV_EVT *pEvt = (CPSV_EVT *)msg;
	CPSV_EVT *pnext = NULL;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		m_MMGR_FREE_CPSV_EVT(pEvt, NCS_SERVICE_ID_CPD);
		pEvt = pnext;
	}

	return true;
}

/****************************************************************************
 * Name          : cpd_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 CPD task.
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
void cpd_main_process(CPD_CB *cb)
{
	CPSV_EVT *evt = NULL;
	NCS_SEL_OBJ mbx_fd;
	SYSF_MBX mbx = cb->cpd_mbx;
	NCS_MBCSV_ARG mbcsv_arg;
	SaSelectionObjectT amf_sel_obj;
	SaAisErrorT error = SA_AIS_OK;
	int term_fd;
	SaAmfHAStateT rda_role = SA_AMF_HA_STANDBY;

	mbx_fd = ncs_ipc_get_sel_obj(&cb->cpd_mbx);
	error = saAmfSelectionObjectGet(cb->amf_hdl, &amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("cpd amf selectionobjget failed %u",error);
		return;
	}

	daemon_sigterm_install(&term_fd);

	/* Get the role. If role=SA_AMF_HA_ACTIVE, this is the first checkpoint director
	 *    starting after both directors have been down.
	 *    In this case delete old checkpoint runtime objects */
	if (rda_get_role(&rda_role) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd rda_get_role FAILED");
		return;
	}
	if (rda_role == SA_AMF_HA_ACTIVE) {
		LOG_NO("cpd RDA role is active, clean old checkpoint runtime objects");
		if (cpd_clean_checkpoint_objects(cb)!= NCSCC_RC_SUCCESS) {
			LOG_ER("cpd_clean_checkpoint_objects FAILED");
			return;
		}
	}

	/* Set up all file descriptors to listen to */
	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;
	fds[FD_AMF].fd = amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

	while (1) {
		fds[FD_CLM].fd = cb->clm_sel_obj;
		fds[FD_CLM].events = POLLIN;
		fds[FD_MBCSV].fd = cb->mbcsv_sel_obj;
		fds[FD_MBCSV].events = POLLIN;

		if (cb->immOiHandle != 0) {
			fds[FD_IMM].fd = cb->imm_sel_obj;
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

		/* process the sig term */
		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		/* process all the AMF messages */
		if (fds[FD_AMF].revents & POLLIN) {
			/* dispatch all the AMF pending function */
			error = saAmfDispatch(cb->amf_hdl, SA_DISPATCH_ALL);
			if (error != SA_AIS_OK) {
				LOG_ER("saAmfDispatch: %u", error);
			}
		}

		/* Process all Clm Messages */
		if (fds[FD_CLM].revents & POLLIN) {
			/* dispatch all the CLM pending function */
			error = saClmDispatch(cb->clm_hdl, SA_DISPATCH_ALL);
			if (error != SA_AIS_OK) {
				LOG_ER("saClmDispatch failed: %u", error);
			}
		}

		/* Process all the MBCSV messages  */
		if (fds[FD_MBCSV].revents & POLLIN) {
			/* dispatch all the MBCSV pending callbacks */
			mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
			mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_handle;
			mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;
			error = ncs_mbcsv_svc(&mbcsv_arg);
			if (error != SA_AIS_OK) {
				LOG_ER("Mbcsv Dispatch failed: %u", error);
			}
		}

		/* process the CPD Mail box */
		if (fds[FD_MBX].revents & POLLIN) {
			if (NULL != (evt = (CPSV_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&mbx, evt))) {
				/* now got the IPC mail box event */
				cpd_process_evt(evt);
			}
		}
		/* process the IMM messages */
		if (cb->immOiHandle && fds[FD_IMM].revents & POLLIN) {
			/* dispatch all the IMM pending function */
			error = saImmOiDispatch(cb->immOiHandle, SA_DISPATCH_ONE);

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
				TRACE_4("cpd saImmOiDispatch returned Bad_handle %u",error);

				/* 
				 ** Invalidate the IMM OI handle, this info is used in other
				 ** locations. E.g. giving TRY_AGAIN responses to a create and
				 ** close resource requests. That is needed since the IMM OI
				 ** is used in context of these functions.
				 */
				saImmOiFinalize(cb->immOiHandle);
				cb->immOiHandle = 0;
				cb->imm_sel_obj = -1;
				cpd_imm_reinit_bg(cb);

			} else if (error != SA_AIS_OK) {
				LOG_ER("cpd saImmOiDispatch failed %u",error);	
				break;
			}
		}
	}
	return;

}
