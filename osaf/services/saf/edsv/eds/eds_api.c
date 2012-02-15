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
..............................................................................
  
    
..............................................................................
      
DESCRIPTION:
        
This include file contains SE api instrumentation for EDS
          
*******************************************************************************/
#include <configmake.h>
#include "eds.h"
#include "logtrace.h"

/* global cb handle */
uint32_t gl_eds_hdl = 0;
/****************************************************************************
 * Name          : eds_se_lib_init
 *
 * Description   : Invoked to Initialize the EDS
 *                 
 *
 * Arguments     : 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_se_lib_init(NCS_LIB_REQ_INFO *req_info)
{
	EDS_CB *eds_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();


	/* Allocate and initialize the control block */
	if (NULL == (eds_cb = m_MMGR_ALLOC_EDS_CB)) {
		LOG_CR("malloc failed for control block");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	memset(eds_cb, '\0', sizeof(EDS_CB));

	/* Obtain the hdl for EDS_CB from hdl-mgr */
	gl_eds_hdl = eds_cb->my_hdl = ncshm_create_hdl(1, NCS_SERVICE_ID_EDS, (NCSCONTEXT)eds_cb);

	if (0 == eds_cb->my_hdl) {
		LOG_ER("Handle create failed for global eds handle");
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl);
		gl_eds_hdl = 0;
		m_MMGR_FREE_EDS_CB(eds_cb);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* initialize the eds cb lock */
	m_NCS_LOCK_INIT(&eds_cb->cb_lock);

	/* Initialize eds control block */
	if (NCSCC_RC_SUCCESS != (rc = eds_cb_init(eds_cb))) {
		/* Destroy the hdl for this CB */
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl);
		gl_eds_hdl = 0;
		/* clean up the CB */
		m_MMGR_FREE_EDS_CB(eds_cb);
		/* log the error */
		TRACE_4("EDS control block init failed");
		TRACE_LEAVE();
		return rc;
	}


	m_NCS_EDU_HDL_INIT(&eds_cb->edu_hdl);

	/* Create the mbx to communicate with the EDS thread */
	if (NCSCC_RC_SUCCESS != (rc = m_NCS_IPC_CREATE(&eds_cb->mbx))) {
		LOG_ER("EDS IPC mailbox create failed");
		/* Release EDU handle */
		m_NCS_EDU_HDL_FLUSH(&eds_cb->edu_hdl);
		/* Destroy the hdl for this CB */
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl);
		gl_eds_hdl = 0;
		/* Free the control block */
		m_MMGR_FREE_EDS_CB(eds_cb);
		TRACE_LEAVE();
		return rc;
	}
	/* Attach the IPC to the created thread */
	m_NCS_IPC_ATTACH(&eds_cb->mbx);

	/* Bind to MDS */
	if (NCSCC_RC_SUCCESS != (rc = eds_mds_init(eds_cb))) {
		TRACE_4("eds mds init failed");
		m_NCS_IPC_RELEASE(&eds_cb->mbx, NULL);
		/* Release EDU handle */
		m_NCS_EDU_HDL_FLUSH(&eds_cb->edu_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl);
		gl_eds_hdl = 0;
		m_MMGR_FREE_EDS_CB(eds_cb);
		TRACE_LEAVE();
		return rc;
	}

	/* Initialize and Register with CLM */
	rc = eds_clm_init(eds_cb);
	if (rc != SA_AIS_OK) {
		TRACE_4("CLM Init failed. Exiting");
		exit(EXIT_FAILURE);
	}

	/* Initialize and Register with AMF */
	rc = eds_amf_register(eds_cb);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("AMF Init failed: Exiting.");
		exit(EXIT_FAILURE);
	}

	/* Initialize mbcsv interface */
	if (NCSCC_RC_SUCCESS != (rc = eds_mbcsv_init(eds_cb))) {
		LOG_ER("eds mbcsv init failed");
		/* Log it */
	}

	TRACE("eds init done.");
	TRACE_LEAVE();
	return (rc);
}

/****************************************************************************
 * Name          : eds_clear_mbx
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
static bool eds_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	EDSV_EDS_EVT *pEvt = (EDSV_EDS_EVT *)msg;
	EDSV_EDS_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = ( /* (EDSV_EDS_EVT *)& */ (pEvt->next));
		eds_evt_destroy(pEvt);
		pEvt = pnext;
	}
	return true;
}

/****************************************************************************
 * Name          : eds_se_lib_destroy
 *
 * Description   : Invoked to destroy the EDS
 *                 
 *
 * Arguments     : 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_se_lib_destroy(NCS_LIB_REQ_INFO *req_info)
{
    /** Code to destroy the EDS **/
	EDS_CB *eds_cb;
	m_INIT_CRITICAL;
	TRACE_ENTER();

	if (NULL == (eds_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Handle take failed for global handle");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	} else {
		m_START_CRITICAL;
      /** Lock EDA_CB
       **/
		m_NCS_LOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);

		/* deregister from AMF */
		saAmfComponentUnregister(eds_cb->amf_hdl, &eds_cb->comp_name, NULL);

		/* End association from the AMF lib */
		saAmfFinalize(eds_cb->amf_hdl);

		/* Finalize with CLM */
		saClmFinalize(eds_cb->clm_hdl);

		/* Clean up all internal structures */
		eds_remove_reglist_entry(eds_cb, 0, true);

		/* Destroy the cb */
		eds_cb_destroy(eds_cb);

		/* Give back the handle */
		ncshm_give_hdl(gl_eds_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_GLD, gl_eds_hdl);

		/* Detach from IPC */
		m_NCS_IPC_DETACH(&eds_cb->mbx, eds_clear_mbx, eds_cb);

		/* Disconnect from MDS */
		eds_mds_finalize(eds_cb);

		/* Release the IPC */
		m_NCS_IPC_RELEASE(&eds_cb->mbx, NULL);

      /** UnLock EDA_CB
       **/
		m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);
		m_NCS_LOCK_DESTROY(&eds_cb->cb_lock);
		m_MMGR_FREE_EDS_CB(eds_cb);

		gl_eds_hdl = 0;
		m_END_CRITICAL;
		TRACE("eds-cb-lib destroy done .");
	}

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : ncs_edsv_eds_lib_req
 *
 * Description   : This is the NCS SE API which is used to init/destroy EDS
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
uint32_t ncs_edsv_eds_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = eds_se_lib_init(req_info);
		break;
	case NCS_LIB_REQ_DESTROY:
		rc = eds_se_lib_destroy(req_info);
		break;
	default:
		break;
	}
	TRACE_LEAVE();
	return (rc);
}

