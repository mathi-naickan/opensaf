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
        
This contains the EDS_CB functions.
          
******************************************************************************
*/
#include "eds.h"
#include "poll.h"
#include "signal.h"
#include "logtrace.h"

enum {
	FD_TERM = 0,
	FD_AMF,
	FD_MBCSV,
	FD_MBX,
	FD_CLM,
	FD_IMM,
	NUM_FD
};

static struct pollfd fds[6];
static nfds_t nfds = 6;


/****************************************************************************
 * Name          : eds_cb_init
 *
 * Description   : This function initializes the EDS_CB including the 
 *                 Patricia trees.
 *                 
 *
 * Arguments     : eds_cb * - Pointer to the EDS_CB.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_cb_init(EDS_CB *eds_cb)
{
	NCS_PATRICIA_PARAMS reg_param, cname_param, nodelist_param;

	memset(&reg_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&cname_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&nodelist_param, 0, sizeof(NCS_PATRICIA_PARAMS));

	reg_param.key_size = sizeof(uint32_t);
	cname_param.key_size = sizeof(SaNameT);
	nodelist_param.key_size = sizeof(uint32_t);
	TRACE_ENTER();

	/* Assign Initial HA state */
	eds_cb->ha_state = EDS_HA_INIT_STATE;
	eds_cb->csi_assigned = false;
	eds_cb->is_impl_set = false;

	/* Assign Version. Currently, hardcoded, This will change later */
	m_GET_MY_VERSION(eds_cb->eds_version);

	/* Initialize patricia tree for reg list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&eds_cb->eda_reg_list, &reg_param)) {
		LOG_ER("Patricia Init for Reg List failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Initialize patricia tree for channel name list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&eds_cb->eds_cname_list, &cname_param)) {
		LOG_ER("Patricia Init for Channel Name List failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Initialize patricia tree for cluster nodes list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&eds_cb->eds_cluster_nodes_list, &nodelist_param)) {
		LOG_ER("Patricia Init for Cluster Nodes List failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_cb_destroy
 *
 * Description   : This function destroys the EDS_CB including the 
 *                 Patricia trees.
 *                 
 *
 * Arguments     : eds_cb * - Pointer to the EDS_CB.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
void eds_cb_destroy(EDS_CB *eds_cb)
{
	ncs_patricia_tree_destroy(&eds_cb->eda_reg_list);
	/* Check if other lists are deleted as well */
	ncs_patricia_tree_destroy(&eds_cb->eds_cname_list);
	ncs_patricia_tree_destroy(&eds_cb->eds_cluster_nodes_list);

	return;
}

/****************************************************************************
 * Name          : eds_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 EDS 
 *
 * Arguments     : mbx  - This is the mail box pointer on which EDS is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
static void eds_process_mbx(SYSF_MBX *mbx)
{
	EDSV_EDS_EVT *evt = NULL;

	evt = (EDSV_EDS_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, evt);
	if (evt != NULL) {
		if ((evt->evt_type >= EDSV_EDS_EVT_BASE) && (evt->evt_type < EDSV_EDS_EVT_MAX)) {
			/* This event belongs to EDS main event dispatcher */
			eds_process_evt(evt);
		} else {
			/* Free the event */
			LOG_IN("Unsupported event type in mailbox");
			eds_evt_destroy(evt);
		}
	}
	return;
}

/****************************************************************************
 * Name          : eds_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 EDS task.
 *                 This function will be select of both the FD's (AMF FD and
 *                 Mail Box FD), depending on which FD has been selected, it
 *                 will call the corresponding routines.
 *
 * Arguments     : mbx  - This is the mail box pointer on which EDS is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void eds_main_process(SYSF_MBX *mbx)
{

	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;
	EDS_CB *eds_cb = NULL;
	int term_fd;
	TRACE_ENTER();

	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Global take handle failed");
		return;
	}

	mbx_fd = m_NCS_IPC_GET_SEL_OBJ(&eds_cb->mbx);

	/* Give back the handle */
	ncshm_give_hdl(gl_eds_hdl);

	/* Initialize with IMM */
	if (eds_imm_init(eds_cb) != SA_AIS_OK) {
		LOG_ER("Imm Init Failed. Exiting");
		exit(EXIT_FAILURE);
	}

	daemon_sigterm_install(&term_fd);

	/* Set up all file descriptors to listen to */
	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;
	fds[FD_AMF].fd = eds_cb->amfSelectionObject;
	fds[FD_AMF].events = POLLIN;
	fds[FD_CLM].fd = eds_cb->clm_sel_obj;
	fds[FD_CLM].events = POLLIN;
	fds[FD_MBCSV].fd = eds_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = eds_cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

	TRACE("Entering the forever loop");

	while (1) {

		if ((eds_cb->immOiHandle != 0) && (eds_cb->is_impl_set == true)){
			fds[FD_IMM].fd = eds_cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = NUM_FD;
		} else {
			nfds = NUM_FD - 1;
		}
		
		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			TRACE("poll failed - %s", strerror(errno));
			break;
		}

		/* process the sigterm */
		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		/* process all the AMF messages */
		if (fds[FD_AMF].revents & POLLIN) {
			/* dispatch all the AMF pending callbacks */
			error = saAmfDispatch(eds_cb->amf_hdl, SA_DISPATCH_ALL);
			if (error != SA_AIS_OK)
				LOG_ER("AMF Dispatch failed with rc = %d",error);
		}

		/* process all mbcsv messages */
		if (fds[FD_MBCSV].revents & POLLIN) {
			error = eds_mbcsv_dispatch(eds_cb->mbcsv_hdl);
			if (NCSCC_RC_SUCCESS != error)
				LOG_ER("MBCSv Dispatch failed with rc = %d",error);
		}

		/* Process the EDS Mail box, if eds is ACTIVE. */
		if (fds[FD_MBX].revents & POLLIN) {
			/* now got the IPC mail box event */
			eds_process_mbx(mbx);
		}

		/* process the CLM messages */
		if (fds[FD_CLM].revents & POLLIN) {
			/* dispatch all the AMF pending callbacks */
			error = saClmDispatch(eds_cb->clm_hdl, SA_DISPATCH_ALL);
			if (error != SA_AIS_OK)
				LOG_ER("CLM Dispatch failed with rc = %d",error);
		}

		/* process the IMM messages */
		if (eds_cb->immOiHandle && fds[FD_IMM].revents & POLLIN) {

			/* dispatch the IMM event */
			error = saImmOiDispatch(eds_cb->immOiHandle, SA_DISPATCH_ONE);

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
				TRACE("saImmOiDispatch returned BAD_HANDLE");

				/* Invalidate the IMM OI handle. */
				saImmOiFinalize(eds_cb->immOiHandle);
				eds_cb->immOiHandle = 0;
				eds_cb->is_impl_set = false;
				eds_imm_reinit_bg(eds_cb);
				
			} else if (error != SA_AIS_OK) {
				LOG_ER("saImmOiDispatch FAILED with rc = %d", error);
				break;
			}

		}
	}

	TRACE_LEAVE();
	return;
}	/* End eds_main_process() */
