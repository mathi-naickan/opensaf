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
  FILE NAME: GLND_CB.C

  DESCRIPTION: API's used to handle GLND CB.

  FUNCTIONS INCLUDED in this module:
  glnd_cb_create ...........GLND CB init.
  glnd_cb_destroy ......... Destroy GLND CB.

******************************************************************************/

#include "glnd.h"

uns32 gl_glnd_hdl;
NCSCONTEXT gl_glnd_task_hdl;

/****************************************************************************
 * Name          : glnd_cb_create
 *
 * Description   : This will create the CB and create the internal structures
 *
 * Arguments     : pool id - pool id for the handle manager
 *
 * Return Values : GLND CB Pointer
 *
 * Notes         : None.
 *****************************************************************************/
GLND_CB *glnd_cb_create(uns32 pool_id)
{
	GLND_CB *glnd_cb = NULL;
	NCS_PATRICIA_PARAMS params = { 0 };
	SaAmfHealthcheckKeyT healthy;
	int8 *health_key = NULL;
	SaAisErrorT amf_error;

	/* register with the Log service */
	glnd_flx_log_reg();

	/* allocate the memory */
	glnd_cb = m_MMGR_ALLOC_GLND_CB;

	if (!glnd_cb) {
		m_LOG_GLND_MEMFAIL(GLND_CB_ALLOC_FAILED, __FILE__, __LINE__);
		glnd_flx_log_dereg();
		return NULL;
	}

	memset(glnd_cb, 0, sizeof(GLND_CB));
	glnd_cb->pool_id = pool_id;

	/* create the handle */
	glnd_cb->cb_hdl_id = ncshm_create_hdl((uns8)pool_id, NCS_SERVICE_ID_GLND, (NCSCONTEXT)glnd_cb);
	if (!glnd_cb->cb_hdl_id) {
		m_LOG_GLND_HEADLINE(GLND_CB_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto hdl_err;
	}

	/* create the internal strucutures */
	/* create the client Tree */
	params.key_size = sizeof(SaLckHandleT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&glnd_cb->glnd_client_tree, &params)) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_CLIENT_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto client_err;
	}

	/* create the agent tree */
	params.key_size = sizeof(MDS_DEST);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&glnd_cb->glnd_agent_tree, &params)) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_AGENT_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto agent_err;
	}

	/* create the Resource tree */
	params.key_size = sizeof(SaLckResourceIdT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&glnd_cb->glnd_res_tree, &params)) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_RSC_TREE_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto res_err;
	}

	/* create the mail box and attach it */
	if (m_NCS_IPC_CREATE(&glnd_cb->glnd_mbx) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_IPC_CREATE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto mbx_create_err;
	}

	if (m_NCS_IPC_ATTACH(&glnd_cb->glnd_mbx) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_IPC_ATTACH_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto mbx_attach_err;
	}

	/* EDU initialisation */
	m_NCS_EDU_HDL_INIT(&glnd_cb->glnd_edu_hdl);

	/* resigter with the MDS */
	if (glnd_mds_register(glnd_cb) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_MDS_REGISTER_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto mds_err;
	} else
		m_LOG_GLND_HEADLINE(GLND_MDS_REGISTER_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

	/* Initialise with the AMF service */
	if (glnd_amf_init(glnd_cb) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_AMF_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto amf_init_err;
	} else
		m_LOG_GLND_HEADLINE(GLND_AMF_INIT_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

	/* register with the AMF service */
	if (glnd_amf_register(glnd_cb) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_HEADLINE(GLND_AMF_REGISTER_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		goto amf_reg_err;
	} else
		m_LOG_GLND_HEADLINE(GLND_AMF_REGISTER_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

	/* everything went off well.. store the hdl in the global variable */
	gl_glnd_hdl = glnd_cb->cb_hdl_id;

	/*   start the AMF Health Check  */

	memset(&healthy, 0, sizeof(healthy));

	health_key = (int8 *)getenv("GLSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		if (strlen("A1B2") < sizeof(healthy.key))
			strncpy((char *)healthy.key, "A1B2", sizeof(healthy.key));
		/* TBD Log the info */
	} else {
		if (strlen((char *)health_key) <= sizeof(healthy.key))
			strncpy((char *)healthy.key, (char *)health_key, SA_AMF_HEALTHCHECK_KEY_MAX - 1);
	}
	healthy.keyLen = strlen((char *)healthy.key);

	amf_error = saAmfHealthcheckStart(glnd_cb->amf_hdl, &glnd_cb->comp_name, &healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		m_LOG_GLND_HEADLINE(GLND_AMF_HEALTHCHECK_START_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	} else
		m_LOG_GLND_HEADLINE(GLND_AMF_HEALTHCHECK_START_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

	if (glnd_cb->node_state != GLND_CLIENT_INFO_GET_STATE) {
		TRACE("setting the state as  GLND_OPERATIONAL_STATE");
		/* GLND HAS STRTED */
		glnd_cb->node_state = GLND_OPERATIONAL_STATE;

	}

	/*create a shared memory segment to Checkpint Resource info, lck_info & backup_event info */
	if (glnd_shm_create(glnd_cb) != NCSCC_RC_SUCCESS)
		goto glnd_shm_create_fail;

	return glnd_cb;
 glnd_shm_create_fail:
	glnd_amf_deregister(glnd_cb);
 amf_reg_err:
	glnd_amf_de_init(glnd_cb);
 amf_init_err:
	glnd_mds_unregister(glnd_cb);
 mds_err:
	m_NCS_EDU_HDL_FLUSH(&glnd_cb->glnd_edu_hdl);
	m_NCS_IPC_DETACH(&glnd_cb->glnd_mbx, glnd_cleanup_mbx, glnd_cb);
 mbx_attach_err:
	m_NCS_IPC_RELEASE(&glnd_cb->glnd_mbx, NULL);
 mbx_create_err:
	ncs_patricia_tree_destroy(&glnd_cb->glnd_res_tree);
 res_err:
	ncs_patricia_tree_destroy(&glnd_cb->glnd_agent_tree);
 agent_err:
	ncs_patricia_tree_destroy(&glnd_cb->glnd_client_tree);
 client_err:
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLND, glnd_cb->cb_hdl_id);
 hdl_err:
	glnd_flx_log_dereg();
	/* free the control block */
	m_MMGR_FREE_GLND_CB(glnd_cb);

	return NULL;
}

/****************************************************************************
 * Name          : glnd_cb_destroy
 *
 * Description   : Destroy the CB 
 *
 * Arguments     : glnd_cb  - GLND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 glnd_cb_destroy(GLND_CB *glnd_cb)
{

	GLND_AGENT_INFO *agent_info;

	/* destroy the handle */
	if (glnd_cb->cb_hdl_id) {
		m_GLND_GIVEUP_GLND_CB;
		ncshm_destroy_hdl(NCS_SERVICE_ID_GLND, glnd_cb->cb_hdl_id);
	}

	/* detach the mail box */
	if (m_NCS_IPC_DETACH(&glnd_cb->glnd_mbx, glnd_cleanup_mbx, glnd_cb) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	/* delete the mailbox */
	if (m_NCS_IPC_RELEASE(&glnd_cb->glnd_mbx, NULL) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	/* delete all the internal structures */
	/* delete the trees */
	for (agent_info = (GLND_AGENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_agent_tree, (uns8 *)0);
	     agent_info != NULL;
	     agent_info = (GLND_AGENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_agent_tree, (uns8 *)0)) {
		glnd_agent_node_del(glnd_cb, agent_info);
	}
	ncs_patricia_tree_destroy(&glnd_cb->glnd_agent_tree);
	ncs_patricia_tree_destroy(&glnd_cb->glnd_client_tree);
	ncs_patricia_tree_destroy(&glnd_cb->glnd_res_tree);

	/* deresigter with the MDS */
	glnd_mds_unregister(glnd_cb);

	/* EDU cleanup */
	m_NCS_EDU_HDL_FLUSH(&glnd_cb->glnd_edu_hdl);

	/* free the control block */
	m_MMGR_FREE_GLND_CB(glnd_cb);

	/* deregister with the log service */
	glnd_flx_log_dereg();
	/* reset the global cb handle */
	gl_glnd_hdl = 0;
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : glnd_cleanup_mbx
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
NCS_BOOL glnd_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	GLSV_GLND_EVT *pEvt = (GLSV_GLND_EVT *)msg;
	GLSV_GLND_EVT *pnext;
	pnext = pEvt;
	while (pnext) {
		pnext = pEvt->next;
		glnd_evt_destroy(pEvt);
		pEvt = pnext;
	}
	return TRUE;
}

/****************************************************************************
 * Name          : glnd_dump_cb
 *
 * Description   : This is the function dumps the contents of the control block.
 *
 * Arguments     : glnd_cb     -  Pointer to the control block
 *                 
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
void glnd_dump_cb()
{
	GLND_CB *glnd_cb;
	GLND_CLIENT_INFO *client_info;
	GLND_AGENT_INFO *agent_info;
	MDS_DEST agent_mds_dest;
	SaLckHandleT handle_id = 0;
	GLND_CLIENT_LIST_RESOURCE *resource_list;
	GLND_RESOURCE_INFO *res_info = NULL;
	SaLckResourceIdT res_id = 0;

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		m_LOG_GLND_HEADLINE(GLND_CB_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}

	memset(&agent_mds_dest, 0, sizeof(MDS_DEST));

	/* display the handles */
	TRACE("GLND Node id - %d", m_NCS_NODE_ID_FROM_MDS_DEST(glnd_cb->glnd_mdest_id));
	if (glnd_cb->gld_card_up == TRUE)
		TRACE("GLD is UP  ");
	else
		TRACE("GLD is DOWN ");

	/* display Agent data */
	agent_info = (GLND_AGENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_agent_tree, (uns8 *)&agent_mds_dest);
	while (agent_info) {
		agent_mds_dest = agent_info->agent_mds_id;
		TRACE("************ Agent info *************** ");
		TRACE("Agent Node id - %d", m_NCS_NODE_ID_FROM_MDS_DEST(agent_info->agent_mds_id));

		while ((client_info = glnd_client_node_find_next(glnd_cb, handle_id, agent_info->agent_mds_id))) {
			handle_id = client_info->app_handle_id;
			TRACE("Client Handle id - %d", (uns32)client_info->app_handle_id);

			/*display the resource list */
			for (resource_list = client_info->res_list; resource_list != NULL;
			     resource_list = resource_list->next) {
				TRACE("Resource id - %d", (uns32)resource_list->rsc_info->resource_id);
			}
		}
		TRACE("******************************************************** ");
		agent_info =
		    (GLND_AGENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_agent_tree, (uns8 *)&agent_mds_dest);
	}

	/* display the resource treee */
	res_info = (GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree, (uns8 *)&res_id);
	while (res_info) {
		res_id = res_info->resource_id;
		TRACE("************ Resource info *************** ");
		TRACE("resource id - %d \t Resource Name - %s ", (uns32)res_info->resource_id,
		      res_info->resource_name.value);
		TRACE("local ref count - %d \t Mds Node id - %d ", res_info->lcl_ref_cnt,
		      m_NCS_NODE_ID_FROM_MDS_DEST(res_info->master_mds_dest));
		TRACE("PR orphaned - %d \t EX Orphaned - %d ", res_info->lck_master_info.pr_orphaned,
		      res_info->lck_master_info.ex_orphaned);
		if (res_info->status == GLND_RESOURCE_ACTIVE_MASTER) {
			GLND_RES_LOCK_LIST_INFO *list;
			TRACE("############ Master info #############");
			TRACE("PR orphaned - %d \t EX Orphaned - %d ", res_info->lck_master_info.pr_orphaned,
			      res_info->lck_master_info.ex_orphaned);

			TRACE("Grant list : ");
			list = res_info->lck_master_info.grant_list;
			while (list) {
				TRACE("Lock Id : %d   Node Id : %d  App Handle : %d ",
				      (uns32)list->lock_info.lockid, m_NCS_NODE_ID_FROM_MDS_DEST(list->req_mdest_id),
				      (uns32)list->lock_info.handleId);
				list = list->next;
			}
			TRACE("Wait Ex list : ");
			list = res_info->lck_master_info.wait_exclusive_list;
			while (list) {
				TRACE("Lock Id : %d   Node Id : %d  App Handle : %d ",
				      (uns32)list->lock_info.lockid, m_NCS_NODE_ID_FROM_MDS_DEST(list->req_mdest_id),
				      (uns32)list->lock_info.handleId);
				list = list->next;
			}
			TRACE("Wait PR list : ");
			list = res_info->lck_master_info.wait_read_list;
			while (list) {
				TRACE("Lock Id : %d   Node Id : %d  App Handle : %d ",
				      (uns32)list->lock_info.lockid, m_NCS_NODE_ID_FROM_MDS_DEST(list->req_mdest_id),
				      (uns32)list->lock_info.handleId);
				list = list->next;
			}
		} else {
			GLND_RES_LOCK_LIST_INFO *lock_list = res_info->lcl_lck_req_info;
			TRACE("############ Non-Master info #############");
			while (lock_list) {
				TRACE("Lock Id : %d  App Handle : %d ", (uns32)lock_list->lock_info.lockid,
				      (uns32)lock_list->lock_info.handleId);
				lock_list = lock_list->next;
			}
		}
		TRACE("******************************************************** ");
		res_info = (GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree, (uns8 *)&res_id);
	}

	/* giveup the handle */
	m_GLND_GIVEUP_GLND_CB;

}
