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
  of the Flex Lof Service (DTSv) subsystem.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  dta_lm..............DTA layer management
  dta_svc_create......DTA service create
  dta_svc_destroy.....DTA service destroy
  ncs_dtsv_su_req.....API for binding and unbinding service
  dta_log_msg.........Function used for logging message
  dta_reg_svc.........Function used for registering service.
  dta_dereg_svc.......Function used for de-registering service.
  dta_svc_reg_log_en..Function checks for logging enable.
  dta_match_service...Finds service match.
  ncs_logmsg..........API used for message logging.
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "dta.h"
#include "os_defs.h"
#include "ncssysf_mem.h"

DTA_CB dta_cb;
uns32 dta_hdl;

/*****************************************************************************

                     DTA LAYER MANAGEMENT IMPLEMENTAION

  PROCEDURE NAME:    dta_lm

  DESCRIPTION:       Core API for all Flex Log Server layer management 
                     operations used by a target system to instantiate and
                     control a DTA. Its operations include:

                     CREATE  a DTA instance
                     DESTROY a DTA instance

  RETURNS:           SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                               enable m_DTA_DBG_SINK() for details.

*****************************************************************************/

uns32 dta_lm(DTA_LM_ARG *arg)
{
	if (arg == NULL)
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lm: NULL pointer passed");

	switch (arg->i_op) {
	case DTA_LM_OP_CREATE:
		return dta_svc_create(&arg->info.create);

	case DTA_LM_OP_DESTROY:
		return dta_svc_destroy(&arg->info.destroy);

	default:
		break;
	}
	return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_lm: Operation type received is wrong");
}

/*#############################################################################
 *
 *                   PRIVATE DTA LAYER MANAGEMENT IMPLEMENTAION
 *
 *############################################################################*/

/*****************************************************************************\
*
*  PROCEDURE NAME:    dta_svc_create
*
*  DESCRIPTION:       Create an instance of DTA, set configuration profile to
*                     default, install this DTA with MDS and subscribe to DTS
*                     events.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - something went wrong. Turn on m_DTA_DBG_SINK()
*                               for details.
*
\*****************************************************************************/
uns32 dta_svc_create(NCSDTA_CREATE *create)
{
	/* Create a new structure and initialize all its fields */
	DTA_CB *inst = &dta_cb;
	NCS_PATRICIA_PARAMS pt_params;

	m_DTA_LK_INIT;

	m_DTA_LK_CREATE(&inst->lock);

	m_DTA_LK(&inst->lock);

	inst->created = TRUE;
	inst->dts_exist = FALSE;
	/* Versioning changes */
	inst->act_dts_ver = DTA_MIN_ACT_DTS_MDS_SUB_PART_VER;

	pt_params.key_size = sizeof(SS_SVC_ID);

	/*Create a Patricia tree for the DTA registration table instead of queue */
	if (ncs_patricia_tree_init(&inst->reg_tbl, &pt_params) != NCSCC_RC_SUCCESS) {
		inst->created = FALSE;
		m_DTA_UNLK(&inst->lock);
		m_DTA_LK_DLT(&inst->lock);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_svc_create: Patricia tree init failed");
	}

	/* 
	 * Get ADEST handle and then register with MDS.
	 */
	if (dta_get_ada_hdl() != NCSCC_RC_SUCCESS) {
		inst->created = FALSE;
		ncs_patricia_tree_destroy(&inst->reg_tbl);
		m_DTA_UNLK(&inst->lock);
		m_DTA_LK_DLT(&inst->lock);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_svc_create: Get ADEST handle failed");
	}

	/* 3_0_b versioning changes - Subscribe to MDS with dta_mds_version */
	inst->dta_mds_version = DTA_MDS_SUB_PART_VERSION;

	if (dta_mds_install_and_subscribe() != NCSCC_RC_SUCCESS) {
		inst->created = FALSE;
		ncs_patricia_tree_destroy(&inst->reg_tbl);
		m_DTA_UNLK(&inst->lock);
		m_DTA_LK_DLT(&inst->lock);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_svc_create: MDS install and subscribe failed");
	}

	m_DTA_UNLK(&inst->lock);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
*
*  PROCEDURE NAME:    dta_svc_destroy
*
*  DESCRIPTION:       Destroy an instance of DTA. Withdraw from MDS and free
*                     this DTA_CB and tend to other resource recovery issues.
*
*  Arguments     :    destroy : Information require for destroy.
*
*  RETURNS:           SUCCESS - all went well.
*                     FAILURE - something went wrong. Turn on m_DTA_DBG_SINK()
*                               for details.
*
*****************************************************************************/
uns32 dta_svc_destroy(NCSDTA_DESTROY *destroy)
{
	DTA_CB *inst = &dta_cb;
	uns32 retval = NCSCC_RC_SUCCESS;
	REG_TBL_ENTRY *svc;

	if (inst->created == FALSE)
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_svc_destroy: DTA does not exist. First create DTA.");

	m_DTA_LK(&inst->lock);

	/* Clear the local datastructures */
	while ((svc = (REG_TBL_ENTRY *)ncs_patricia_tree_getnext(&inst->reg_tbl, NULL)) != NULL) {
		ncs_patricia_tree_del(&inst->reg_tbl, (NCS_PATRICIA_NODE *)svc);
		m_MMGR_FREE_DTA_REG_TBL(svc);
	}

	/*Destroy the patricia tree created */
	ncs_patricia_tree_destroy(&inst->reg_tbl);

	/* Clear the logs buffered in DTA */
	{
		DTA_LOG_BUFFER *list = &dta_cb.log_buffer;
		DTA_BUFFERED_LOG *buf;
		uns32 i, count;
		DTSV_MSG *msg;

		if ((list == NULL) || (list->num_of_logs == 0)) {
			/* Don't print anythig, service users don't like DBG SINKs */
		} else {
			count = list->num_of_logs;
			for (i = 0; i < count; i++) {
				if (!list->head) {
					list->tail = NULL;
					break;
				}
				buf = list->head;
				list->head = list->head->next;

				msg = buf->buf_msg;
				if (msg->data.data.msg.log_msg.uba.start != NULL)
					m_MMGR_FREE_BUFR_LIST(msg->data.data.msg.log_msg.uba.start);
				m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
				if (0 != msg)
					m_MMGR_FREE_DTSV_MSG(msg);

				m_MMGR_FREE_DTA_BUFFERED_LOG(buf);
				list->num_of_logs--;
			}	/*end of for */
		}		/*end of else */
	}

	m_DTA_UNLK(&inst->lock);

	/* Uninstall DTA from MDS */
	if (dta_mds_uninstall() != NCSCC_RC_SUCCESS) {
		retval = m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_svc_destroy: MDS uninstall failed.");
	}

	inst->created = FALSE;

	m_DTA_LK_DLT(&inst->lock);

	return retval;
}

/*****************************************************************************\
*
*  PROCEDURE NAME:    dtsv_log_msg
*
*  DESCRIPTION:       SE_API for registration, de-registration.
*
*  RETURNS:           NCSCC_RC_SUCCESS
*                     NCSCC_RC_FAILURE
*
\*****************************************************************************/
uns32 ncs_dtsv_su_req(NCS_DTSV_RQ *arg)
{
	if (arg == NULL)
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_dtsv_su_req: NULL pointer passed");

	switch (arg->i_op) {
	case NCS_DTSV_OP_BIND:
		return dta_reg_svc(&arg->info.bind_svc);

	case NCS_DTSV_OP_UNBIND:
		return dta_dereg_svc(arg->info.unbind_svc.svc_id);

	default:
		break;
	}
	return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_dtsv_su_req: Operation type passed to ncs_dtsv_su_req is wrong");
}

/*****************************************************************************

  PROCEDURE NAME:    dta_log_msg

  DESCRIPTION:       This function sends the log message to the DTS

  RETURNS:           NCSCC_RC_SUCCESS
                     NCSCC_RC_FAILURE

*****************************************************************************/

uns32 dta_log_msg(NCSFL_NORMAL *lmsg)
{

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    dta_reg_svc

  DESCRIPTION:       This function used for registering the service with DTA/DTS

  RETURNS:           NCSCC_RC_SUCCESS
                     NCSCC_RC_FAILURE

*****************************************************************************/

uns32 dta_reg_svc(NCS_BIND_SVC *bind_svc)
{
	DTA_CB *inst = &dta_cb;
	DTSV_MSG msg;
	REG_TBL_ENTRY *svc;
	/*uns32          send_pri; */
	SS_SVC_ID svc_id = bind_svc->svc_id;

	if (inst->created == FALSE) {
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_reg_svc: DTA does not exist. First create DTA before registering your service.",
					  svc_id);
	}

	if ((strlen(bind_svc->svc_name) + 1) > DTSV_SVC_NAME_MAX)
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_reg_svc: Service name supplied in registration is either too long or not properly initialized",
					  svc_id);

	m_DTA_LK(&inst->lock);

	/* Search the patricia tree for exisiting entries */
	/*if ((svc = (REG_TBL_ENTRY *) ncs_find_item(&inst->reg_tbl, 
	   (NCSCONTEXT)&svc_id, dta_match_service)) != NULL) */
	if ((svc = (REG_TBL_ENTRY *)ncs_patricia_tree_get(&inst->reg_tbl, (const uns8 *)&svc_id)) != NULL) {
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_reg_svc: Service is already registered with DTSv", svc_id);
	}

	svc = m_MMGR_ALLOC_DTA_REG_TBL;
	if (svc == NULL) {
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_reg_svc: Memory allocation failed", svc_id);
	}

	memset(svc, 0, sizeof(REG_TBL_ENTRY));
	svc->svc_id = svc_id;
	svc->log_msg = FALSE;
	svc->enable_log = TRUE;
	/* Restricting bufferring of logs till NOTICE level severity only */
	svc->severity_bit_map = 0xFC;
	svc->category_bit_map = 0xFFFFFFFF;
	svc->version = bind_svc->version;
	strcpy(svc->svc_name, bind_svc->svc_name);

	svc->node.key_info = (uns8 *)&svc->svc_id;

	/* Add to the patricia tree */
	if (ncs_patricia_tree_add(&inst->reg_tbl, (NCS_PATRICIA_NODE *)svc) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_DTA_REG_TBL(svc);
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_reg_svc: Failed to add service registration entry in patricia tree",
					  svc_id);
	}

	/* 
	 * Check whether DTS exist. If DTS does not exist then we will send 
	 * registration information to DTS at later time when DTS comes up. 
	 * For buffering of log messages apply defaults and return success. 
	 */
	if (inst->dts_exist == FALSE) {
		m_DTA_UNLK(&inst->lock);
		return NCSCC_RC_SUCCESS;
	}

	memset(&msg, '\0', sizeof(DTSV_MSG));
	dta_fill_reg_msg(&msg, svc_id, svc->version, svc->svc_name, DTA_REGISTER_SVC);

	m_DTA_UNLK(&inst->lock);

	/* Always register async */
	if (dta_mds_async_send(&msg, inst) != NCSCC_RC_SUCCESS)
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_reg_svc: MDS async send failed", svc_id);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    dta_dereg_svc

  DESCRIPTION:       This function used for de-registering the service with DTA/DTS

  RETURNS:           NCSCC_RC_SUCCESS
                     NCSCC_RC_FAILURE

*****************************************************************************/

uns32 dta_dereg_svc(SS_SVC_ID svc_id)
{
	DTA_CB *inst = &dta_cb;
	REG_TBL_ENTRY *rmv_svc;
	DTSV_MSG *msg;
	uns32 send_pri;

	if (inst->created == FALSE) {
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_dereg_svc: DTA does not exist. First create DTA before de-registering your service.",
					  svc_id);
	}

	m_DTA_LK(&inst->lock);

	/* If DTS is deregistering from DTS means that DTS is going down,
	 * So don't send any registration message to DTS */
	if (NCS_SERVICE_ID_DTSV == svc_id) {
		if ((rmv_svc = (REG_TBL_ENTRY *)ncs_patricia_tree_get(&inst->reg_tbl, (const uns8 *)&svc_id)) == NULL) {
			m_DTA_UNLK(&inst->lock);
			return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
						  "dta_dereg_svc: Specified service registration entry doesn't exist in patricia tree",
						  svc_id);
		}

		/* Remove entry from the Patricia tree */
		ncs_patricia_tree_del(&inst->reg_tbl, (NCS_PATRICIA_NODE *)rmv_svc);

		m_MMGR_FREE_DTA_REG_TBL(rmv_svc);

		m_DTA_UNLK(&inst->lock);
		return NCSCC_RC_SUCCESS;
	}

	if ((rmv_svc = (REG_TBL_ENTRY *)ncs_patricia_tree_get(&inst->reg_tbl, (const uns8 *)&svc_id)) == NULL) {
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_dereg_svc: Specified service registration entry doesn't exist in patricia tree",
					  svc_id);
	}

	/* Remove entry from the Patricia tree */
	ncs_patricia_tree_del(&inst->reg_tbl, (NCS_PATRICIA_NODE *)rmv_svc);

	if (inst->dts_exist == FALSE) {
		if (0 != rmv_svc)
			m_MMGR_FREE_DTA_REG_TBL(rmv_svc);
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_dereg_svc: DTS does not exist, de-registration request is not sent to DTS.",
					  svc_id);
	}

	msg = m_MMGR_ALLOC_DTSV_MSG;
	if (msg == NULL) {
		if (0 != rmv_svc)
			m_MMGR_FREE_DTA_REG_TBL(rmv_svc);
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "dta_dereg_svc: Mem allocation failed, de-registration request is not sent to DTS.",
					  svc_id);
	}
	memset(msg, '\0', sizeof(DTSV_MSG));

	dta_fill_reg_msg(msg, svc_id, rmv_svc->version, rmv_svc->svc_name, DTA_UNREGISTER_SVC);

	if (0 != rmv_svc)
		m_MMGR_FREE_DTA_REG_TBL(rmv_svc);

	m_DTA_UNLK(&inst->lock);

	/* Smik - Don't send msg to MDS but to DTA msgbox */
	send_pri = NCS_IPC_PRIORITY_LOW;

	if (m_DTA_SND_MSG(&gl_dta_mbx, msg, send_pri) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_DTSV_MSG(msg);
		/*if (dta_mds_async_send(&msg, inst) != NCSCC_RC_SUCCESS) */
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_dereg_svc: send to DTA msgbox failed", svc_id);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: dta_svc_reg_log_en
 *
 * Purpose: Function checks whether the service which is trying to log the 
 *          the message has registered with the DTA and also whether logging
 *          is enabled. If both these conditions satisfies then return success.
 *
 ****************************************************************************/
uns32 dta_svc_reg_log_en(REG_TBL_ENTRY *svc, NCSFL_NORMAL *lmsg)
{
	if ((svc->enable_log == FALSE) ||
	    ((svc->category_bit_map & lmsg->hdr.category) != lmsg->hdr.category) ||
	    ((svc->severity_bit_map & lmsg->hdr.severity) != lmsg->hdr.severity)) {
		return NCSCC_RC_FAILURE;
	} else
		return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: dta_match_service
 *
 * Purpose: criteria for finding an service match
 *
 ****************************************************************************/

NCS_BOOL dta_match_service(void *key, void *qelem)
{
	SS_SVC_ID *svc_id = (SS_SVC_ID *)key;
	REG_TBL_ENTRY *svc = (REG_TBL_ENTRY *)qelem;

	if (svc == NULL)
		return FALSE;

	if (*svc_id == svc->svc_id)
		return TRUE;

	return FALSE;
}

/*****************************************************************************

  PROCEDURE NAME:    dta_fill_reg_msg

  DESCRIPTION:       This function used for filling the registration message 
                     to be sent to the DTS.

  RETURNS:           NCSCC_RC_SUCCESS
                     NCSCC_RC_FAILURE

*****************************************************************************/
uns32 dta_fill_reg_msg(DTSV_MSG *msg, SS_SVC_ID svc_id, const uns16 version, const char *svc_name, uns8 operation)
{
	DTA_CB *inst = &dta_cb;

	msg->vrid = inst->vrid;
	msg->msg_type = operation;

	switch (operation) {
	case DTA_REGISTER_SVC:
		msg->data.data.reg.svc_id = svc_id;
		msg->data.data.reg.version = version;
		if (svc_name != NULL)
			strcpy(msg->data.data.reg.svc_name, svc_name);

		break;

	case DTA_UNREGISTER_SVC:
		msg->data.data.unreg.svc_id = svc_id;
		msg->data.data.unreg.version = version;

		if (svc_name != NULL)
			strcpy(msg->data.data.unreg.svc_name, svc_name);

		break;

	default:
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_fill_reg_msg: Wrong operation type.", svc_id);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Name          : ncs_logmsg_v2
 *
 * Description   : This API is use for Message Logging with instance ID.
 *
 * Arguments     : svc_id       : Service ID.
 *                 inst_id      : Instance ID to be logged with message.
 *                 fmat_id      : Format ID.
 *                 str_table_id : String tabel ID.
 *                 category     : Message category.
 *                 severity     : Message Severity.
 *                 fmat_type    : Format type to be used for logging.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
\*****************************************************************************/
uns32 ncs_logmsg_v2(SS_SVC_ID svc_id,
		    uns32 inst_id, uns8 fmat_id, uns8 str_table_id, uns32 category, uns8 severity, char *fmat_type, ...)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	va_list argp;
	va_start(argp, fmat_type);

	rc = ncs_logmsg_int(svc_id, inst_id, fmat_id, str_table_id, category, severity, fmat_type, argp);

	va_end(argp);
	return rc;
}

/****************************************************************************\
 * Name          : ncs_logmsg
 *
 * Description   : This API is use for Message Logging.
 *
 * Arguments     : svc_id       : Service ID.
 *                 fmat_id      : Format ID.
 *                 str_table_id : String tabel ID.
 *                 category     : Message category.
 *                 severity     : Message Severity.
 *                 fmat_type    : Format type to be used for logging.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
\*****************************************************************************/
uns32 ncs_logmsg(SS_SVC_ID svc_id, uns8 fmat_id, uns8 str_table_id, uns32 category, uns8 severity, char *fmat_type, ...)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	va_list argp;
	va_start(argp, fmat_type);

	rc = ncs_logmsg_int(svc_id, DEFAULT_INST_ID, fmat_id, str_table_id, category, severity, fmat_type, argp);

	va_end(argp);
	return rc;
}

/****************************************************************************\
 * Name          : ncs_logmsg_int
 *
 * Description   : This function is use for Message Logging.
 *
 * Arguments     : svc_id       : Service ID.
 *                 fmat_id      : Format ID.
 *                 str_table_id : String tabel ID.
 *                 category     : Message category.
 *                 severity     : Message Severity.
 *                 fmat_type    : Format type to be used for logging.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
\*****************************************************************************/
uns32 ncs_logmsg_int(SS_SVC_ID svc_id,
		     uns32 inst_id,
		     uns8 fmat_id, uns8 str_table_id, uns32 category, uns8 severity, char *fmat_type, va_list argp)
{
	uns32 i = 0, length = 0;
	DTA_CB *inst = &dta_cb;
	DTSV_MSG *msg;
	NCSFL_HDR *hdr;
	REG_TBL_ENTRY *svc;
	NCS_UBAID *uba = NULL;
	uns8 *data;
	uns32 send_pri;
	int warning_rmval = 0;

    /*************************************************************************\
    * As different fields of the log-message are encoded, the minimum DTS     
    * required to understand the message may change.
    \*************************************************************************/
	uns32 act_dts_ver;	/* Active dts's version. */
	uns32 min_dts_ver;	/* Minimum DTS version that understands message */

	DTA_LOG_BUFFER *list = &inst->log_buffer;
	DTA_BUFFERED_LOG *buf = NULL;

	/* Check if DTA is created. Continue logging if DTA is created */
	if (inst->created == FALSE) {
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "ncs_logmsg: DTA does not exist. First create DTA before logging.", svc_id);
	}

	m_DTA_LK(&inst->lock);

	/* 
	 * Check whether DTS exist. If DTS does not exist then there is no 
	 * point in logging message. Return failure.
	 * Changed as of 15th June 2006. 
	 * If FLS doesn't exist & number of buffer msgs is more than DTA_BUF_LIMIT
	 * then drop the message and return failure.
	 */
	if ((inst->dts_exist == FALSE) && (list->num_of_logs >= DTA_BUF_LIMIT)) {
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
					  "ncs_logmsg: DTS does not exist & DTA log buffer is full. Log message is dropped.",
					  svc_id);
	}

	/*
	 * Check whether the Sevice is registered with the DTSv. If not already
	 * bound return failure.
	 */
	if (((svc = (REG_TBL_ENTRY *)ncs_patricia_tree_get(&inst->reg_tbl, (const uns8 *)&svc_id)) == NULL)) {
		m_DTA_UNLK(&inst->lock);
		return NCSCC_RC_FAILURE;
	}
#if (DTA_FLOW == 1)
	/* Check whether logging is enabled beyond default level for this service 
	 * i.e. INFO or DEBUG levels are enabled or not. 
	 * If logging levels are increased to INFO/DEBUG, don't apply DTA message
	 * thresholding. Otherwise DTA defines MAX threshold of 2000 messages after
	 * which all messages will be dropped. In such cases, check the rate of 
	 * logging for such processes.
	 */
	if (svc->severity_bit_map > 0xFC) {
		/* Don't apply thresholding of messages here */
	} else {
		/* Else apply thresholding */
		if (inst->msg_count > DTA_MAX_THRESHOLD) {
			m_DTA_UNLK(&inst->lock);
			warning_rmval =
			    m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
					   "ncs_logmsg: DTA queued msgs exceeds 2000. Message will be dropped.");
			return NCSCC_RC_FAILURE;
		}
	}

	/* Drop INFO/DEBUG messages at source during congestion */
	if ((inst->dts_congested == TRUE) && (severity < NCSFL_SEV_NOTICE)) {
		m_DTA_UNLK(&inst->lock);
		return NCSCC_RC_FAILURE;
	}
#endif

	msg = m_MMGR_ALLOC_DTSV_MSG;
	if (msg == NULL) {
		m_DTA_UNLK(&inst->lock);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_logmsg: Memory allocation failer for DTSV_MSG");
	}
	memset(msg, '\0', sizeof(DTSV_MSG));

	hdr = &msg->data.data.msg.log_msg.hdr;

	hdr->category = category;
	hdr->severity = severity;
	hdr->fmat_id = fmat_id;
	hdr->ss_id = svc_id;
	hdr->inst_id = inst_id;
	hdr->str_table_id = str_table_id;

	/* Apply the filter Policies. If the logging is disabled
	 * then return success. We are returning success here since it may 
	 * possible that logging is disabled by user.
	 */
	if (dta_svc_reg_log_en(svc, &msg->data.data.msg.log_msg) == NCSCC_RC_FAILURE) {
		m_DTA_UNLK(&inst->lock);
		m_MMGR_FREE_DTSV_MSG(msg);
		return NCSCC_RC_SUCCESS;
	}

	if (NCSCC_RC_SUCCESS != dta_copy_octets(&hdr->fmat_type, fmat_type, (uns16)(1 + strlen(fmat_type)))) {
		m_DTA_UNLK(&inst->lock);
		m_MMGR_FREE_DTSV_MSG(msg);
		return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_logmsg_int: Copy octet failed.");
	}

	/* Flexlog Agent fills in the TIME STAMP value */
	m_GET_MSEC_TIME_STAMP(&hdr->time.seconds, &hdr->time.millisecs);

	msg->vrid = inst->vrid;
	msg->msg_type = DTA_LOG_DATA;
	uba = &msg->data.data.msg.log_msg.uba;

	/* act_dts_ver needs to be recorded before inst->lock is unlocked  */
	act_dts_ver = inst->act_dts_ver;
	min_dts_ver = 1;	/* DTS should be at least this version to interpret the
				   encoded message. This directly determines the
				   message format vesion */

	m_DTA_UNLK(&inst->lock);

	memset(uba, '\0', sizeof(NCS_UBAID));

	if (ncs_enc_init_space(uba) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_OCT(hdr->fmat_type);
		m_MMGR_FREE_DTSV_MSG(msg);
		return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "ncs_logmsg: Unable to init user buff", svc_id);
	}

	while (fmat_type[i] != '\0') {
		switch (fmat_type[i]) {
		case 'T':
			{
				break;
			}
		case 'I':
			{
				uns32 idx;
				idx = m_NCSFL_MAKE_IDX((uns32)str_table_id, (uns32)va_arg(argp, uns32));
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, idx);
				ncs_enc_claim_space(uba, sizeof(uns32));
				break;
			}
		case 'L':
			{
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, va_arg(argp, uns32));
				ncs_enc_claim_space(uba, sizeof(uns32));
				break;
			}

		case 'C':
			{
				char *str = va_arg(argp, char *);

				if (NULL == str) {
					if (uba->start != NULL)
						m_MMGR_FREE_BUFR_LIST(uba->start);
					m_MMGR_FREE_OCT(hdr->fmat_type);
					m_MMGR_FREE_DTSV_MSG(msg);
					return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
								  "ncs_logmsg: NULL string passed for format type 'C'",
								  svc_id);
				}

				length = strlen(str) + 1;

				if (length > (DTS_MAX_SIZE_DATA * 3)) {
					if (uba->start != NULL)
						m_MMGR_FREE_BUFR_LIST(uba->start);
					m_MMGR_FREE_OCT(hdr->fmat_type);
					m_MMGR_FREE_DTSV_MSG(msg);
					return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
								  "ncs_logmsg: Can not log string with more than 1536 characters",
								  svc_id);
				}

				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, length);
				ncs_enc_claim_space(uba, sizeof(uns32));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)str, length);

				break;
			}
		case 'M':
			{
				data = ncs_enc_reserve_space(uba, sizeof(uns16));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_16bit(&data, (uns16)va_arg(argp, uns32));
				ncs_enc_claim_space(uba, sizeof(uns16));
				break;
			}
		case 'D':
			{
				NCSFL_MEM mem_d = va_arg(argp, NCSFL_MEM);

				if (mem_d.len > DTS_MAX_SIZE_DATA)
					mem_d.len = DTS_MAX_SIZE_DATA;

				/* Versioning & 64-bit compatibility changes - Encode the msg 
				 * for format 'D' after checking the version of the current 
				 * Active DTS.
				 * If Active DTS has MDS version 1 and DTA arch 64-bit then, 
				 * encode 32 bits with reserved bit-pattern 0x6464. 
				 * Else, if DTS has MDS version 2, then encode 64-bits of the 
				 * address.
				 * Also fill DTA_LOG_MSG structure with the DTS version as seen
				 * while encoding.
				 */

				/* The 'D' message is encoded at version 1 or 2 based on 
				   whether DTS is at version 1 or higher respectively. 

				 */
				/* Check for compatibility with receiving DTS version
				   and set the minimum DTS version required */
				if (act_dts_ver == 1) {
					/* Compatible DTS. Update minimum DTS version required */
					if (min_dts_ver < 1)
						min_dts_ver = 1;

					data = ncs_enc_reserve_space(uba, (sizeof(uns16) + sizeof(uns32)));
					if (data == NULL)
						goto reserve_error;
					ncs_encode_16bit(&data, mem_d.len);

					if (sizeof(inst) == 8) {
						/* Attempt to print 64-bit address on 32-bit DTS. 
						 * Print pre-defined bit pattern to indicate error. 
						 */
						ncs_encode_32bit(&data, (uns32)0x6464);
					} else if (sizeof(inst) == 4) {
						/* Do it the old way */
						ncs_encode_32bit(&data, NCS_PTR_TO_UNS32_CAST(mem_d.addr));
					}
					ncs_enc_claim_space(uba, (sizeof(uns16) + sizeof(uns32)));
				}
				/* Act DTS on version 2 or higher , then encode all 64-bits */
				else {
					/* Compatible DTS. Update minimum DTS version required */
					if (min_dts_ver < 2)
						min_dts_ver = 2;

					data = ncs_enc_reserve_space(uba, (sizeof(uns16) + sizeof(uns64)));
					if (data == NULL)
						goto reserve_error;
					ncs_encode_16bit(&data, mem_d.len);
					ncs_encode_64bit(&data, (uns64)(long)mem_d.addr);
					ncs_enc_claim_space(uba, (sizeof(uns16) + sizeof(uns64)));
				}

				ncs_encode_n_octets_in_uba(uba, (uns8 *)mem_d.dump, (uns32)mem_d.len);

				break;
			}
		case 'P':
			{
				NCSFL_PDU pdu = va_arg(argp, NCSFL_PDU);

				if (pdu.len > DTS_MAX_SIZE_DATA)
					pdu.len = DTS_MAX_SIZE_DATA;

				data = ncs_enc_reserve_space(uba, sizeof(uns16));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_16bit(&data, pdu.len);
				ncs_enc_claim_space(uba, sizeof(uns16));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)pdu.dump, (uns32)pdu.len);

				break;
			}
		case 'A':
			{
				encode_ip_address(uba, va_arg(argp, NCS_IP_ADDR));
				break;
			}
		case 'S':
			{
				data = ncs_enc_reserve_space(uba, sizeof(uns8));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_8bit(&data, (uns8)va_arg(argp, uns32));
				ncs_enc_claim_space(uba, sizeof(uns8));

				break;
			}
			/* Added code for handling float values */
		case 'F':
			{
				char str[DTS_MAX_DBL_DIGITS] = "";
				int num_chars = sprintf(str, "%f", va_arg(argp, double));
				if (num_chars == 0) {
					warning_rmval =
					    m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
							   "ncs_logmsg: Float to string conversion gives NULL");
					goto reserve_error;
				}
				length = strlen(str) + 1;
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, length);
				ncs_enc_claim_space(uba, sizeof(uns32));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)str, length);
				break;
			}
		case 'N':
			{
				char str[DTS_MAX_DBL_DIGITS] = "";
				int num_chars = 0;

				/* Check for compatibility with receiving DTS version
				   and set the minimum DTS version required */
				if (act_dts_ver == 1) {
					/* Incompatible DTS */
					goto reserve_error;
				} else {
					/* Compatible DTS. Update minimum DTS version required */
					if (min_dts_ver < 2)
						min_dts_ver = 2;
				}

				num_chars = sprintf(str, "%lld", va_arg(argp, long long));

                                if (num_chars == 0) {
					warning_rmval =
					    m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
							   "ncs_logmsg: long long to string conversion gives NULL");
					goto reserve_error;
				}

				length = strlen(str) + 1;
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, length);
				ncs_enc_claim_space(uba, sizeof(uns32));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)str, length);
				break;
			}
		case 'U':
			{
				char str[DTS_MAX_DBL_DIGITS] = "";
				int num_chars = 0;

				/* Check for compatibility with receiving DTS version
				   and set the minimum DTS version required */
				if (act_dts_ver == 1) {
					/* Incompatible DTS */
					goto reserve_error;
				} else {
					/* Compatible DTS. Update minimum DTS version required */
					if (min_dts_ver < 2)
						min_dts_ver = 2;
				}

				num_chars = sprintf(str, "%llu", va_arg(argp, unsigned long long));
                                if (num_chars == 0) {
					warning_rmval =
					    m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
							   "ncs_logmsg: unsigned long long to string conversion gives NULL");
					goto reserve_error;
				}

				length = strlen(str) + 1;
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, length);
				ncs_enc_claim_space(uba, sizeof(uns32));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)str, length);
				break;
			}
		case 'X':
			{
				char str[DTS_MAX_DBL_DIGITS] = "";
				int num_chars = 0;

				/* Check for compatibility with receiving DTS version
				   and set the minimum DTS version required */
				if (act_dts_ver == 1) {
					/* Incompatible DTS */
					goto reserve_error;
				} else {
					/* Compatible DTS. Update minimum DTS version required */
					if (min_dts_ver < 2)
						min_dts_ver = 2;
				}

				num_chars = sprintf(str, "Ox%016llx", va_arg(argp, long long));

                                if (num_chars == 0) {
					warning_rmval =
					    m_DTA_DBG_SINK(NCSCC_RC_FAILURE,
							   "ncs_logmsg: 64bit hex to string conversion gives NULL");
					goto reserve_error;
				}

				length = strlen(str) + 1;
				data = ncs_enc_reserve_space(uba, sizeof(uns32));
				if (data == NULL)
					goto reserve_error;
				ncs_encode_32bit(&data, length);
				ncs_enc_claim_space(uba, sizeof(uns32));

				ncs_encode_n_octets_in_uba(uba, (uns8 *)str, length);
				break;
			}
		default:
			{
				if (uba->start != NULL)
					m_MMGR_FREE_BUFR_LIST(uba->start);
				m_MMGR_FREE_OCT(hdr->fmat_type);
				m_MMGR_FREE_DTSV_MSG(msg);
				return m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE,
							  "ncs_logmsg: Format Type Not accounted for", svc_id);
			}

		}

		i++;
	}

	/* Buffer log messgages if DTS is not up _or_ registration is not confirmed */
	if ((inst->dts_exist == FALSE) || (svc->log_msg == FALSE)) {
		m_DTA_LK(&inst->lock);
		buf = m_MMGR_ALLOC_DTA_BUFFERED_LOG;
		if (!buf) {
			if (msg->data.data.msg.log_msg.uba.start != NULL)
				m_MMGR_FREE_BUFR_LIST(msg->data.data.msg.log_msg.uba.start);
			m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
			if (0 != msg)
				m_MMGR_FREE_DTSV_MSG(msg);
			m_DTA_UNLK(&inst->lock);
			return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "Failed to allocate memory for log buffering");
		}

		/* Set the msg format version based on how it is encoded above. 
		   Usually it will be 1, because during DTS initialization, until
		   active DTS shows up (i.e until inst->dts_exist becomes TRUE)
		   active DTS version is assumed to be 1.
		 */
		msg->data.data.msg.msg_fmat_ver = min_dts_ver;

		memset(buf, '\0', sizeof(DTA_BUFFERED_LOG));
		buf->buf_msg = msg;
		buf->next = NULL;

		if (!list->head)
			list->head = buf;
		else
			list->tail->next = buf;

		list->tail = buf;
		list->num_of_logs++;
		m_DTA_UNLK(&inst->lock);
	} else {
		send_pri = NCS_IPC_PRIORITY_LOW;

		/* Set the msg format version to final value of min_dts_ver */
		msg->data.data.msg.msg_fmat_ver = min_dts_ver;

		/* Smik - We don't send the msg to MDS but to DTA's msgbox */
		if (m_DTA_SND_MSG(&gl_dta_mbx, msg, send_pri) != NCSCC_RC_SUCCESS) {
			if (uba->start != NULL)
				m_MMGR_FREE_BUFR_LIST(uba->start);
			m_MMGR_FREE_OCT(hdr->fmat_type);
			m_MMGR_FREE_DTSV_MSG(msg);
			/*if (dta_mds_async_send(&msg, inst) != NCSCC_RC_SUCCESS) */
			return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_logmsg: send to DTA msgbox failed");
		}
	}

	return NCSCC_RC_SUCCESS;

 reserve_error:
	if (uba->start != NULL)
		m_MMGR_FREE_BUFR_LIST(uba->start);
	m_MMGR_FREE_OCT(hdr->fmat_type);
	m_MMGR_FREE_DTSV_MSG(msg);
	return m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "ncs_logmsg: Unable to reserve space in encode");
}

/*****************************************************************************

  The master DTA dispatch loop functions
  OVERVIEW:   These functions are used for processing the events in the DTA 
              mailbox and send them to MDS.

  PROCEDURE NAME: dts_do_evts & dts_do_evt

  DESCRIPTION:       

     dta_do_evts      Infinite loop services the passed SYSF_MBX
     dta_do_evt       Master Dispatch function and services off DTA work queue
*****************************************************************************/

/*****************************************************************************
   dta_do_evts
*****************************************************************************/
void dta_do_evts(SYSF_MBX *mbx)
{
	uns32 status;
	NCS_IPC_MSG *msg;
	int warning_rmval = 0;

	while ((msg = m_NCS_IPC_RECEIVE(mbx, NULL)) != NULL) {
		status = dta_do_evt((DTSV_MSG *)msg);
		if (status != NCSCC_RC_SUCCESS) {
			/* log the error */
			warning_rmval = m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_do_evts: Error returned");
		}
	}			/* end of while */

}

/****************************************************************************\
 * Name          : send_buffered_logs
 *
 * Description   : This function flushes buffered log messages to the DTS server
 *
 * Arguments     : -
 * Return Values : -
 *
 * Notes         : None.
\*****************************************************************************/
static void send_buffered_logs(void)
{
	DTA_CB *inst = &dta_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	SS_SVC_ID svc_id;
	DTA_LOG_BUFFER *list = &dta_cb.log_buffer;
	DTA_BUFFERED_LOG *buf = NULL;
	DTSV_MSG *bmsg = NULL;
	uns32 i, count;
	int warning_rmval = 0;

	count = list->num_of_logs;
	for (i = 0; i < count; i++) {
		if (!list->head) {
			list->tail = NULL;
			break;
		}
		buf = list->head;
		list->head = list->head->next;
		bmsg = buf->buf_msg;
		if (bmsg == NULL) {
			/*Shouldn't hit but still go to the next buffered log */
		} else {
			/* Send the buffered msg to DTS */
			svc_id = bmsg->data.data.msg.log_msg.hdr.ss_id;
			if (dta_mds_async_send(bmsg, inst) != NCSCC_RC_SUCCESS) {
				warning_rmval =
				    m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_do_evt: MDS async send failed", svc_id);
				rc = NCSCC_RC_FAILURE;
			}
			m_MMGR_FREE_OCT(bmsg->data.data.msg.log_msg.hdr.fmat_type);
			if ((rc == NCSCC_RC_FAILURE) && (bmsg->data.data.msg.log_msg.uba.start != NULL))
				m_MMGR_FREE_BUFR_LIST(bmsg->data.data.msg.log_msg.uba.start);
			if (0 != bmsg)
				m_MMGR_FREE_DTSV_MSG(bmsg);
		}
		/* Clear the log buffer datastructures */
		m_MMGR_FREE_DTA_BUFFERED_LOG(buf);
		list->num_of_logs--;
	}			/*end of for */
}

/*****************************************************************************
   dta_do_evt
*****************************************************************************/
uns32 dta_do_evt(DTSV_MSG *msg)
{
	DTA_CB *inst = &dta_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	SS_SVC_ID svc_id;
	int warning_rmval = 0;

	if (msg == NULL)
		return NCSCC_RC_SUCCESS;

	switch (msg->msg_type) {
	case DTS_UP_EVT:
		{
			send_buffered_logs();
			rc = NCSCC_RC_SUCCESS;
		}
		break;

	case DTA_LOG_DATA:
		{
#if (DTA_FLOW == 1)

			/* Check how many log messages have already been received.
			 * If more than dta congestion log limit, then do MDS_SEND to
			 * DTS to control the flow.
			 */
			inst->logs_received++;
			if (inst->logs_received > DTA_CONGESTION_LOG_LIMIT) {
				DTSV_MSG flow_msg;

				memset(&flow_msg, '\0', sizeof(DTSV_MSG));
				flow_msg.msg_type = DTA_FLOW_CONTROL;
				while (dta_mds_sync_send(&flow_msg, inst, 200, FALSE) != NCSCC_RC_SUCCESS) {
					/* Set congestion flag */
					inst->dts_congested = TRUE;
				}

				inst->dts_congested = FALSE;
				inst->logs_received = 0;
			}
#endif

			svc_id = msg->data.data.msg.log_msg.hdr.ss_id;
			if (dta_mds_async_send(msg, inst) != NCSCC_RC_SUCCESS) {
				warning_rmval =
				    m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_do_evt: MDS async send failed", svc_id);
				/*m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type); */
				rc = NCSCC_RC_FAILURE;
			}

			m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);

			if ((rc == NCSCC_RC_FAILURE) && (msg->data.data.msg.log_msg.uba.start != NULL))
				m_MMGR_FREE_BUFR_LIST(msg->data.data.msg.log_msg.uba.start);
		}
		break;

	case DTA_UNREGISTER_SVC:
		{
			svc_id = msg->data.data.unreg.svc_id;
			if (dta_mds_async_send(msg, inst) != NCSCC_RC_SUCCESS) {
				warning_rmval =
				    m_DTA_DBG_SINK_SVC(NCSCC_RC_FAILURE, "dta_do_evt: MDS async send failed", svc_id);
				rc = NCSCC_RC_FAILURE;
			}
		}
		break;

	case DTA_DESTROY_EVT:
		if (0 != msg)
			m_MMGR_FREE_DTSV_MSG(msg);

		return dta_cleanup_seq();

	case DTS_SVC_REG_CONF:
		dta_svc_reg_config(inst, msg);
		send_buffered_logs();
		break;

	default:
		{
			warning_rmval =
			    m_DTA_DBG_SINK(NCSCC_RC_FAILURE, "dta_do_evt: Invalid message type in DTA mailbox");
			rc = NCSCC_RC_FAILURE;
		}
		break;
	}

	if (0 != msg)
		m_MMGR_FREE_DTSV_MSG(msg);

	return rc;
}
