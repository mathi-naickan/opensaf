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

  DESCRIPTION:

  This file contains all Public APIs for the Flex Log server (DTA) portion
  of the Distributed Tracing Service (DTSv) subsystem.

  ..............................................................................

  FUNCTIONS INCLUDED in this module:

  dta_lib_req                  - SE API to create DTA.
  dta_lib_init                 - Create DTA service.
  dta_lib_destroy              - Distroy DTA service.

  TRACE GUIDE:
  Policy is to not use logging/syslog from library code.
  Only the trace part of logtrace is used from library. 
 
  It is possible to turn on trace for the DTA library used
  by an application process. This is done by the application 
  defining the environment variable: DTA_TRACE_PATHNAME.
  The trace will end up in the file defined by that variable.

  TRACE   debug traces                 - aproximates DEBUG  
  TRACE_1 normal but important events  - aproximates INFO.
  TRACE_2 user errors with return code - aproximates NOTICE.
  TRACE_3 unusual or strange events    - aproximates WARNING
  TRACE_4 library errors ERR_LIBRARY   - aproximates ERROR
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "dta.h"
#include "ncs_tasks.h"
#include "ncssysf_mem.h"
#include "ncssysf_tsk.h"

static NCS_BOOL dta_clear_mbx(NCSCONTEXT arg, NCSCONTEXT mbx_msg);

SYSF_MBX gl_dta_mbx;
DTA_CB dta_cb;

/****************************************************************************
 * Name          : dta_lib_req
 *
 * Description   : This is the NCS SE API which is used to init/destroy or 
 *                 Create/destroy DTA. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uns32 dta_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uns32 res = NCSCC_RC_FAILURE;

	if (req_info == NULL)
		return res;

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		res = dta_lib_init(req_info);
		break;

	case NCS_LIB_REQ_DESTROY:
		res = dta_lib_destroy();
		break;

	default:
		break;
	}
	return (res);
}

/****************************************************************************
 * Name          : dta_lib_init
 *
 * Description   : This is the function which initalize the dta libarary.
 *                 This function creates an IPC mail Box and spawns DTA
 *                 thread.
 *                 This function initializes the CB, handle manager and MDS.
 *
 * Arguments     : req_info - Request info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 dta_lib_init(NCS_LIB_REQ_INFO *req_info)
{
	DTA_LM_ARG arg;
	NCSCONTEXT task_handle;
	DTA_CB *inst = &dta_cb;
	char *value = NULL;	
	
	 /* Initialize trace system first of all so we can see what is going. */
        if ((value = getenv("DTA_TRACE_PATHNAME")) != NULL) {
               logtrace_init("dta", value, CATEGORY_ALL);
        }

	memset(inst, 0, sizeof(DTA_CB));
	/* Smik - Changes to create a task for DTA */
	/* Create DTA mailbox */
	if (m_NCS_IPC_CREATE(&gl_dta_mbx) != NCSCC_RC_SUCCESS) {
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_init:Failed to create DTA mail box");
	}

	/* Create DTA's task */
	if (m_NCS_TASK_CREATE
	    ((NCS_OS_CB)dta_do_evts, &gl_dta_mbx, NCS_DTA_TASKNAME, NCS_DTA_PRIORITY, NCS_DTA_STACKSIZE,
	     &task_handle) != NCSCC_RC_SUCCESS) {
		m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_init:Failed to create DTA thread.");
	}

	if (NCSCC_RC_SUCCESS != m_NCS_IPC_ATTACH(&gl_dta_mbx)) {
		m_NCS_TASK_RELEASE(task_handle);
		m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_init: IPC attach failed.");
	}
#if (DTA_FLOW == 1)
	/* Keeping count of messages on DTA mailbox */
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_CONFIG_USR_COUNTERS(&gl_dta_mbx, NCS_IPC_PRIORITY_LOW, &inst->msg_count)) {
		m_NCS_TASK_RELEASE(task_handle);
		m_NCS_IPC_DETACH(&gl_dta_mbx, dta_clear_mbx, inst);
		m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
				      "dta_lib_init: Failed to initialize DTA msg counters with LEAP");
	}
#endif

	/* Start DTA task */
	if (m_NCS_TASK_START(task_handle) != NCSCC_RC_SUCCESS) {
		m_NCS_TASK_RELEASE(task_handle);
		m_NCS_IPC_DETACH(&gl_dta_mbx, dta_clear_mbx, inst);
		m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_init: Failed to start DTA task");
	}

	inst->task_handle = task_handle;

	memset(&arg, 0, sizeof(DTA_LM_ARG));

	/* Create the DTA CB */
	arg.i_op = DTA_LM_OP_CREATE;

	if (dta_lm(&arg) != NCSCC_RC_SUCCESS) {
		m_NCS_TASK_STOP(task_handle);
		m_NCS_TASK_RELEASE(task_handle);
		m_NCS_IPC_DETACH(&gl_dta_mbx, dta_clear_mbx, inst);
		m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_init: DTA CB initialization failed.");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : dta_lib_destroy
 *
 * Description   : This is the function which destroy's the DTA lib.
 *                 This function releases the Task and the IPX mail Box.
 *
 * Arguments     : None.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 dta_lib_destroy(void)
{

	DTSV_MSG *msg = NULL;

	msg = m_MMGR_ALLOC_DTSV_MSG;
	if (msg == NULL) {
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_destroy: Memory allocation failed.");
	}
	memset(msg, 0, sizeof(DTSV_MSG));
	msg->msg_type = DTA_DESTROY_EVT;

	if (m_DTA_SND_MSG(&gl_dta_mbx, msg, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_DTSV_MSG(msg);

		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lib_destroy: IPC send failed");
	}

	m_NCS_TASK_JOIN(dta_cb.task_handle);
	m_NCS_TASK_RELEASE(dta_cb.task_handle);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : dta_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 mbx_msg - Message start pointer.
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static NCS_BOOL dta_clear_mbx(NCSCONTEXT arg, NCSCONTEXT mbx_msg)
{
	DTSV_MSG *msg = (DTSV_MSG *)mbx_msg;
	DTSV_MSG *mnext;

	mnext = msg;
	while (mnext) {
		mnext = (DTSV_MSG *)msg->next;
		if (msg->msg_type == DTA_LOG_DATA) {
			m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
			if (msg->data.data.msg.log_msg.uba.start != NULL)
				m_MMGR_FREE_BUFR_LIST(msg->data.data.msg.log_msg.uba.start);
		}
		m_MMGR_FREE_DTSV_MSG(msg);

		msg = mnext;
	}
	return TRUE;
}

/****************************************************************************
 * Name          : dta_cleanup_seq
 *
 * Description   : This is the function which cleans up DTA in sequence
 *
 * Arguments     : None.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
 *
 * Notes         : None.
 *****************************************************************************/
uns32 dta_cleanup_seq(void)
{
	DTA_LM_ARG arg;
	int warning_rmval = 0;

	/* Destroy the DTA CB */
	arg.i_op = DTA_LM_OP_DESTROY;
	arg.info.destroy.i_meaningless = (void *)0x1234;

	if (dta_lm(&arg) != NCSCC_RC_SUCCESS) {
		warning_rmval = m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_cleanup_seq: DTA svc destroy failed");
	}

	/* DTA shutdown support re-arrangement */
	m_NCS_IPC_DETACH(&gl_dta_mbx, dta_clear_mbx, &dta_cb);
	m_NCS_IPC_RELEASE(&gl_dta_mbx, NULL);
	gl_dta_mbx = 0;

	return NCSCC_RC_SUCCESS;
}
