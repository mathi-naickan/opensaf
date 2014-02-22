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
  FILE NAME: GLND_API.C

  DESCRIPTION: API's used to init and create PWE of GLND.

  FUNCTIONS INCLUDED in this module:
  glnd_se_lib_create ........ library used to init GLND.
  glnd_amf_init ........... GLND AMF init.
  glnd_se_lib_destroy...... library used to destroy GLND.
  ncsglnd_lib_req ......... SE API to init or create PWE.
  glnd_main_process ....... main process which is given as thread input.
  glnd_process_mbx ........ Process Mail box.  

******************************************************************************/

#include "glnd.h"
#include "osaf_poll.h"

enum {
	FD_TERM = 0,
	FD_AMF,
	FD_MBX,
	NUM_FD
};

void glnd_main_process(SYSF_MBX *mbx);

/****************************************************************************
 * Name          : glnd_se_lib_init
 *
 * Description   : This is the function which initalize the GLND libarary.
 *
 * Arguments     : pool_id - This is the pool ID.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_se_lib_create(uint8_t pool_id)
{

	GLND_CB *glnd_cb;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER2("pool id %u", pool_id);

	/* create the CB */
	glnd_cb = glnd_cb_create(pool_id);

	if (!glnd_cb) {
		TRACE_2("GLND cb creation failed");
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
end:
	TRACE_LEAVE2("%s return value %u", (rc == NCSCC_RC_SUCCESS)?"SUCCESS":"FAILURE", rc);
	return rc;
}

/****************************************************************************
 * Name          : glnd_se_lib_destroy
 *
 * Description   : This is the function which destroy the GLND libarary.
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_se_lib_destroy()
{
	GLND_CB *glnd_cb;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		goto end;
	}

	if ((rc = glnd_cb_destroy(glnd_cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLND cb destroy failed");
		rc= NCSCC_RC_FAILURE;
	}
end:
	TRACE_LEAVE2("%s return value %u", (rc == NCSCC_RC_SUCCESS)?"SUCCESS":"FAILURE", rc);
	return rc;
}

/****************************************************************************
 * Name          : glnd_lib_req
 *
 * Description   : This is the NCS SE API which is used to init/destroy or 
 *                 Create/destroy PWE's. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t res = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	
	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		if( (res = glnd_se_lib_create(NCS_HM_POOL_ID_COMMON))!= NCSCC_RC_SUCCESS )
			TRACE_2("GLND se lib create failed");	
		break;
	case NCS_LIB_REQ_DESTROY:
		if( (res = glnd_se_lib_destroy()) != NCSCC_RC_SUCCESS )
			TRACE_2("GLND se lib destroy failed");
		break;
	default:
		break;
	}
	TRACE_LEAVE2("%s return value %u", (res == NCSCC_RC_SUCCESS)?"SUCCESS":"FAILURE", res);
	return (res);
}

/****************************************************************************
 * Name          : glnd_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 GLND.
 *
 * Arguments     : mbx  - This is the mail box pointer on which IfD/IfND is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void glnd_process_mbx(GLND_CB *cb, SYSF_MBX *mbx)
{
	GLSV_GLND_EVT *evt = NULL;
	TRACE_ENTER();

	while ((evt = (GLSV_GLND_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, evt))) {
		if ((evt->type >= GLSV_GLND_EVT_BASE) && (evt->type < GLSV_GLND_EVT_MAX)) {
			/* process mail box */
			glnd_process_evt((NCSCONTEXT)cb, evt);
		} else {
			TRACE_1("Unknown glnd evt rcvd: event_type %d", evt->type);
			m_MMGR_FREE_GLND_EVT(evt);
		}
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : glnd_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 GLND task.
 *
 * Arguments     : mbx  - This is the mail box pointer on which GLND is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void glnd_main_process(SYSF_MBX *mbx)
{
	NCS_SEL_OBJ mbx_fd = m_NCS_IPC_GET_SEL_OBJ(mbx);
	GLND_CB *glnd_cb = NULL;
	TRACE_ENTER();	
	
	SaAmfHandleT amf_hdl;

	SaSelectionObjectT amf_sel_obj;
	SaAisErrorT amf_error;

	struct pollfd sel[NUM_FD];
	int term_fd;

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		goto end;
	}

	amf_hdl = glnd_cb->amf_hdl;

	/*giveup the handle */
	m_GLND_GIVEUP_GLND_CB;

	amf_error = saAmfSelectionObjectGet(amf_hdl, &amf_sel_obj);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("GLND amf get sel obj error");
		goto end;
	}

	daemon_sigterm_install(&term_fd);

	sel[FD_TERM].fd = term_fd;
	sel[FD_TERM].events = POLLIN;
	sel[FD_AMF].fd = amf_sel_obj;
	sel[FD_AMF].events = POLLIN;
	sel[FD_MBX].fd = m_GET_FD_FROM_SEL_OBJ(mbx_fd);
	sel[FD_MBX].events = POLLIN;

	while (osaf_poll(&sel[0], NUM_FD, -1) > 0) {

		if (sel[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (((sel[FD_AMF].revents | sel[FD_MBX].revents) &
			(POLLERR | POLLHUP | POLLNVAL)) != 0) {
			LOG_ER("GLND poll() failure: %hd %hd",
				sel[FD_AMF].revents, sel[FD_MBX].revents);
			TRACE_LEAVE();
			return;
		}
		/* process all the AMF messages */
		if (sel[FD_AMF].revents & POLLIN) {
			/* dispatch all the AMF pending function */
			amf_error = saAmfDispatch(amf_hdl, SA_DISPATCH_ALL);
			if (amf_error != SA_AIS_OK) {
				TRACE_2("GLND amf dispatch failure");
			}
		}
		/* process the GLND Mail box */
		if (sel[FD_MBX].revents & POLLIN) {
			glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
			if (glnd_cb) {
				/* now got the IPC mail box event */
				glnd_process_mbx(glnd_cb, mbx);
				m_GLND_GIVEUP_GLND_CB;	/* giveup the handle */
			} else
				break;
		}
	}

	TRACE("DANGER: Exiting the Select loop of GLND");
end:
	TRACE_LEAVE();
	return;
}
