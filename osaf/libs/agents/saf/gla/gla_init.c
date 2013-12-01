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

  This file contains the initialization and destroy routines for GLA library.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  
******************************************************************************
*/

#include "gla.h"
#include <pthread.h>
#include "glnd.h"
#include "osaf_utility.h"
#include "osaf_poll.h"

/* global cb handle */
uint32_t gl_gla_hdl = 0;
static uint32_t gla_use_count = 0;

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

static void gla_sync_with_glnd(GLA_CB *cb);
static uint32_t gla_resource_info_send(GLA_CB *gla_cb, SaLckHandleT hdl_id);
static uint32_t gla_client_res_tree_cleanup(GLA_CLIENT_INFO *client_info);

/****************************************************************************
  Name          : gla_lib_req
 
  Description   : This routine is exported to the external entities & is used
                  to create & destroy the AvA library.
 
  Arguments     : req_info - ptr to the request info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = gla_create(&req_info->info.create);
		if (rc == NCSCC_RC_SUCCESS)
			TRACE_1("GLA se lib api create success");
		else
			TRACE_2("GLA se lib api create failed");
		break;

	case NCS_LIB_REQ_DESTROY:
		rc = gla_destroy(&req_info->info.destroy);
		if (rc == NCSCC_RC_SUCCESS)
			TRACE_1("GLA se lib api destroy success");
		else
			TRACE_2("GLA se lib api destroy failed");
		break;

	default:
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/********************************************************************
 Name    :  gla_sync_with_glnd

 Description : This is for GLA to sync with GLND when it gets MDS callback
 
**********************************************************************/
void gla_sync_with_glnd(GLA_CB *cb)
{
	TRACE_ENTER();

	m_NCS_LOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);

	if (cb->glnd_svc_up) {
		m_NCS_UNLOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);
		return;
	}

	cb->glnd_sync_awaited = true;
	m_NCS_SEL_OBJ_CREATE(&cb->glnd_sync_sel);
	m_NCS_UNLOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);

	/* Await indication from MDS saying GLND is up */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(cb->glnd_sync_sel), 30000);

	/* Destroy the sync - object */
	m_NCS_LOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);

	cb->glnd_sync_awaited = false;
	m_NCS_SEL_OBJ_DESTROY(cb->glnd_sync_sel);

	m_NCS_UNLOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : gla_create
 
  Description   : This routine creates & initializes the GLA control block.
 
  Arguments     : create_info - ptr to the create info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_create(NCS_LIB_CREATE *create_info)
{
	GLA_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* validate create info */
	if (create_info == NULL){
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	/* allocate GLA cb */
	if (!(cb = m_MMGR_ALLOC_GLA_CB)) {
		TRACE_4("Control block alloc failed");
		rc = NCSCC_RC_FAILURE;
		goto error1;
	}

	memset(cb, 0, sizeof(GLA_CB));

	/* assign the GLA pool-id (used by hdl-mngr) */
	cb->pool_id = NCS_HM_POOL_ID_COMMON;

	/* create the association with hdl-mngr */
	if (!(cb->agent_handle_id = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_GLA, (NCSCONTEXT)cb))) {
		TRACE_2("GLA handle creation failed");
		rc = NCSCC_RC_FAILURE;
		goto error2;
	}

	/* get the process id */
	cb->process_id = getpid();

	/* initialize the gla cb lock */
	if (m_NCS_LOCK_INIT(&cb->cb_lock) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLA cb lock init failed");
		rc = NCSCC_RC_FAILURE;
		goto error3;
	}

	/* initialize the client tree */
	if (gla_client_tree_init(cb) != NCSCC_RC_SUCCESS) {
		TRACE_4("GLA client tree init failed");
		rc = NCSCC_RC_FAILURE;
		goto error4;
	}

	/* register with MDS */
	if (gla_mds_register(cb) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLA mds register failed");
		rc = NCSCC_RC_FAILURE;
		goto error5;
	}

	gla_sync_with_glnd(cb);

	/* initialize the resource id tree */
	if (gla_res_tree_init(cb) != NCSCC_RC_SUCCESS) {
		/* TBD: Log message */
		rc = NCSCC_RC_FAILURE;
		goto error6;
	}

	/* initialize the lock id tree */
	if (gla_lock_tree_init(cb) != NCSCC_RC_SUCCESS) {
		/* TBD: Log message */
		rc = NCSCC_RC_FAILURE;
		goto error7;
	}

	TRACE_1("GLA mds register success");

	/* everything went off well.. store the hdl in the global variable */
	gl_gla_hdl = cb->agent_handle_id;

	goto end;

 error7:
	/* delete the lock tree */
	ncs_patricia_tree_destroy(&cb->gla_lock_id_tree);
 error6:
	/* delete the res tree */
	ncs_patricia_tree_destroy(&cb->gla_resource_id_tree);
 error5:
	/* delete the tree */
	ncs_patricia_tree_destroy(&cb->gla_client_tree);
 error4:
	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

 error3:
	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, cb->agent_handle_id);
 error2:
	/* free the control block */
	m_MMGR_FREE_GLA_CB(cb);
 error1:

 end:	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_destroy
 
  Description   : This routine destroys the GLA control block.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_destroy(NCS_LIB_DESTROY *destroy_info)
{
	GLA_CB *cb = 0;
	TRACE_ENTER();

	/* validate the CB */
	cb = (GLA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLA, gl_gla_hdl);
	if (!cb) 
		return NCSCC_RC_FAILURE;

	/* Send the unreg info to the GLND */
	gla_agent_unregister(cb);

	/* unregister with MDS */
	gla_mds_unregister(cb);

	/* delete all the client info */
	gla_client_tree_destroy(cb);

	/* delete the resource tree */
	gla_lock_tree_destroy(cb);

	/* delete the resource tree */
	gla_res_tree_destroy(cb);

	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);


	/* return GLA CB */
	ncshm_give_hdl(gl_gla_hdl);

	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, cb->agent_handle_id);

	/* free the control block */
	m_MMGR_FREE_GLA_CB(cb);

	/* reset the global cb handle */
	gl_gla_hdl = 0;
	
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_client_tree_init
  
  Description   : This routine is used to initialize the client tree
 
  Arguments     : cb - pointer to the GLA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_client_tree_init(GLA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaLckHandleT);
	if (ncs_patricia_tree_init(&cb->gla_client_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_client_tree_destroy
 
  Description   : This routine destroys the GLA client tree.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_client_tree_destroy(GLA_CB *gla_cb)
{
	/* take the cb lock */
	m_NCS_LOCK(&gla_cb->cb_lock, NCS_LOCK_WRITE);

	/* cleanup the client tree */
	gla_client_tree_cleanup(gla_cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&gla_cb->gla_client_tree);

	/* giveup the cb lock */
	m_NCS_UNLOCK(&gla_cb->cb_lock, NCS_LOCK_WRITE);

	return;
}

/****************************************************************************
  Name          : gla_client_tree_cleanup
 
  Description   : This routine cleansup the GLA Client tree
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_client_tree_cleanup(GLA_CB *gla_cb)
{
	GLA_CLIENT_INFO *client_info;

	/* scan the entire handle db & delete each record */
	while ((client_info = (GLA_CLIENT_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_client_tree, 0))) {
		/* delete the client info */
		if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, client_info->lcl_lock_handle_id))
			gla_client_tree_delete_node(gla_cb, client_info, true);
	}

	return;
}

/****************************************************************************
  Name          : gla_client_tree_find_and_add
 
  Description   : This routine adds the new client to the client tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  hdl_id : the handle id.
                  flag   : true/false if true create the new node
 
  Return Values : returns the client_info node.
 
  Notes         : None
******************************************************************************/
GLA_CLIENT_INFO *gla_client_tree_find_and_add(GLA_CB *gla_cb, SaLckHandleT hdl_id, bool flag)
{
	GLA_CLIENT_INFO *client_info = NULL;
	TRACE_ENTER();

	/* take the cb lock */
	m_NCS_LOCK(&gla_cb->cb_lock, NCS_LOCK_READ);
	client_info = (GLA_CLIENT_INFO *)ncs_patricia_tree_get(&gla_cb->gla_client_tree, (uint8_t *)&hdl_id);
	m_NCS_UNLOCK(&gla_cb->cb_lock, NCS_LOCK_READ);

	if (flag == true) {
		/* create and allocate the memory */
		if (!client_info) {
			client_info = (GLA_CLIENT_INFO *)m_MMGR_ALLOC_GLA_CLIENT_INFO;
			if (!client_info) {
				TRACE_4("GLA client node alloc failed");
				client_info = NULL;
				goto end;
			}
			memset(client_info, 0, sizeof(GLA_CLIENT_INFO));

			NCS_PATRICIA_PARAMS param;
			memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
			param.key_size = sizeof(SaLckResourceIdT);
			if (ncs_patricia_tree_init(&client_info->client_res_tree, &param) != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_GLA_CLIENT_INFO(client_info);
				client_info = NULL;
				goto end;
			}
			/* create the association with hdl-mngr */
			if (0 ==
			    (client_info->lcl_lock_handle_id =
			     ncshm_create_hdl(NCS_HM_POOL_ID_EXTERNAL1, NCS_SERVICE_ID_GLA, (NCSCONTEXT)client_info))) {
				m_MMGR_FREE_GLA_CLIENT_INFO(client_info);
				client_info = NULL;
				goto end;
			}

			client_info->lock_handle_id = hdl_id;
			client_info->patnode.key_info = (uint8_t *)&client_info->lock_handle_id;

			/* initialize the callbk mailbox */
			if (glsv_gla_callback_queue_init(client_info) != NCSCC_RC_SUCCESS) {
				ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, client_info->lcl_lock_handle_id);
				m_MMGR_FREE_GLA_CLIENT_INFO(client_info);
				client_info = NULL;
				goto end;
			}

			/* take the lock */
			m_NCS_LOCK(&gla_cb->cb_lock, NCS_LOCK_WRITE);

			if (ncs_patricia_tree_add(&gla_cb->gla_client_tree, &client_info->patnode) != NCSCC_RC_SUCCESS) {
				TRACE_4("GLA client tree add failed");
				ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, client_info->lcl_lock_handle_id);
				m_MMGR_FREE_GLA_CLIENT_INFO(client_info);
				m_NCS_UNLOCK(&gla_cb->cb_lock, NCS_LOCK_WRITE);
				client_info = NULL;
				goto end;
			}

			/* give up the lock */
			m_NCS_UNLOCK(&gla_cb->cb_lock, NCS_LOCK_WRITE);
		}
	}
 end:	
	TRACE_LEAVE();
	return client_info;
}

/****************************************************************************
  Name          : gla_client_info_send

  Description   :  

  Arguments     :

  Return Values : 

  Notes         : None
******************************************************************************/
uint32_t gla_client_info_send(GLA_CB *gla_cb)
{
	GLA_CLIENT_INFO *client_info;
	GLSV_GLND_EVT restart_client_info_evt;
	uint32_t ret = NCSCC_RC_SUCCESS;
	uint16_t lck_clbk = 0;
	TRACE_ENTER();

	/* take the cb lock */
	client_info = (GLA_CLIENT_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_client_tree, (uint8_t *)0);
	while (client_info) {
		/* populate the evt */
		memset(&restart_client_info_evt, 0, sizeof(GLSV_GLND_EVT));

		restart_client_info_evt.type = GLSV_GLND_EVT_CLIENT_INFO;
		restart_client_info_evt.info.restart_client_info.client_handle_id = client_info->lock_handle_id;
		restart_client_info_evt.info.restart_client_info.app_proc_id = gla_cb->process_id;
		restart_client_info_evt.info.restart_client_info.agent_mds_dest = gla_cb->gla_mds_dest;
		/* compute the reg_info */
		lck_clbk |= (uint16_t)((client_info->lckCallbk.saLckResourceOpenCallback) ? GLSV_LOCK_OPEN_CBK_REG : 0);
		lck_clbk |= (uint16_t)((client_info->lckCallbk.saLckLockGrantCallback) ? GLSV_LOCK_GRANT_CBK_REG : 0);
		lck_clbk |= (uint16_t)((client_info->lckCallbk.saLckLockWaiterCallback) ? GLSV_LOCK_WAITER_CBK_REG : 0);
		lck_clbk |=
		    (uint16_t)((client_info->lckCallbk.saLckResourceUnlockCallback) ? GLSV_LOCK_UNLOCK_CBK_REG : 0);

		restart_client_info_evt.info.restart_client_info.cbk_reg_info = lck_clbk;

		/* send the event */
		if ((ret = gla_mds_msg_async_send(gla_cb, &restart_client_info_evt)) != NCSCC_RC_SUCCESS) {
			TRACE_2("GLA mds send failure: from mds_dest: %d event_type: %d", m_NCS_NODE_ID_FROM_MDS_DEST(gla_cb->gla_mds_dest),
					    GLSV_GLND_EVT_CLIENT_INFO);
			ret = NCSCC_RC_FAILURE;
			goto end;
		}

		/* Send resource info for this client */
		gla_resource_info_send(gla_cb, client_info->lock_handle_id);
		client_info =
		    (GLA_CLIENT_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_client_tree,
								 (uint8_t *)&client_info->lock_handle_id);
	}
 end:	
	TRACE_LEAVE();
	return ret;
}

/****************************************************************************
  Name          : gla_resource_info_send 

  Description   :

  Arguments     :

  Return Values :

  Notes         : None
******************************************************************************/
static uint32_t gla_resource_info_send(GLA_CB *gla_cb, SaLckHandleT hdl_id)
{
	GLA_RESOURCE_ID_INFO *res_info;
	GLSV_GLND_EVT restart_res_info_evt;
	uint32_t ret;
	TRACE_ENTER();

	/* take the cb lock */
	res_info = (GLA_RESOURCE_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_resource_id_tree, (uint8_t *)0);
	while (res_info) {
		if (res_info->lock_handle_id == hdl_id) {
			/* populate the evt */
			memset(&restart_res_info_evt, 0, sizeof(GLSV_GLND_EVT));
			restart_res_info_evt.type = GLSV_GLND_EVT_CLIENT_INFO;
			restart_res_info_evt.info.restart_client_info.resource_id = res_info->gbl_res_id;
			restart_res_info_evt.info.restart_client_info.client_handle_id = hdl_id;

			/* send the event */
			if ((ret = gla_mds_msg_async_send(gla_cb, &restart_res_info_evt)) != NCSCC_RC_SUCCESS) {
				TRACE_2("GLA mds send failure: from mds_dest: %d event_type: %d", 
						    m_NCS_NODE_ID_FROM_MDS_DEST(gla_cb->gla_mds_dest),
						    GLSV_GLND_EVT_CLIENT_INFO);
				ret = NCSCC_RC_FAILURE;
				goto end;
			}
		}
		res_info =
		    (GLA_RESOURCE_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_resource_id_tree,
								      (uint8_t *)&res_info->lcl_res_id);

	}
 end:
	TRACE_LEAVE();
	return ret;
}

/****************************************************************************
  Name          : gla_client_tree_delete_node
 
  Description   : This routine adds the deletes the client from the client tree
 
  Arguments     : client_info - pointer to the client node.
                  
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t gla_client_tree_delete_node(GLA_CB *gla_cb, GLA_CLIENT_INFO *client_info, bool give_hdl)
{
	/* clean the queue */
	glsv_gla_callback_queue_destroy(client_info);

	/* delete from the tree */
	if (ncs_patricia_tree_del(&gla_cb->gla_client_tree, &client_info->patnode) != NCSCC_RC_SUCCESS)
		TRACE_4("GLA client tree del failed");

	if (give_hdl)
		ncshm_give_hdl(client_info->lcl_lock_handle_id);
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, client_info->lcl_lock_handle_id);

	/* free the mem */
	m_MMGR_FREE_GLA_CLIENT_INFO(client_info);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_res_tree_init
  
  Description   : This routine is used to initialize the Resource tree
 
  Arguments     : cb - pointer to the GLA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_res_tree_init(GLA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaLckResourceHandleT);
	if (ncs_patricia_tree_init(&cb->gla_resource_id_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_res_tree_destroy
 
  Description   : This routine destroys the GLA resource tree.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_res_tree_destroy(GLA_CB *gla_cb)
{
	/* cleanup the client tree */
	gla_res_tree_cleanup(gla_cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&gla_cb->gla_resource_id_tree);

	return;
}

/****************************************************************************
  Name          : gla_client_res_tree_destroy

  Description   : This routine destroys the GLA resource tree.

  Arguments     : destroy_info - ptr to the destroy info

  Return Values : None

  Notes         : None
******************************************************************************/
uint32_t gla_client_res_tree_destroy(GLA_CLIENT_INFO *client_info)
{
	/* cleanup the client tree */
	gla_client_res_tree_cleanup(client_info);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&client_info->client_res_tree);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_res_tree_cleanup
 
  Description   : This routine cleansup the GLA resource tree
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_res_tree_cleanup(GLA_CB *gla_cb)
{
	GLA_RESOURCE_ID_INFO *res_id_info;

	/* scan the entire handle db & delete each record */
	while ((res_id_info = (GLA_RESOURCE_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_resource_id_tree, 0))) {
		/* delete the client info */

		if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, res_id_info->lcl_res_id))
			gla_res_tree_delete_node(gla_cb, res_id_info);
	}

	return;
}

/****************************************************************************
  Name          : gla_client_res_tree_cleanup 

  Description   : This routine cleansup the GLA resource tree

  Arguments     : destroy_info - ptr to the destroy info

  Return Values : None

  Notes         : None
******************************************************************************/
static uint32_t gla_client_res_tree_cleanup(GLA_CLIENT_INFO *client_info)
{
	GLA_CLIENT_RES_INFO *client_res_info = NULL;
	SaLckResourceIdT res_id = 0;

	/* scan the entire handle db & delete each record */
	while ((client_res_info =
		(GLA_CLIENT_RES_INFO *)ncs_patricia_tree_getnext(&client_info->client_res_tree, (uint8_t *)&res_id))) {
		/* delete the client info */
		ncs_patricia_tree_del(&client_info->client_res_tree, &client_res_info->patnode);

		res_id = client_res_info->gbl_res_id;
		/* free the mem */
		m_MMGR_FREE_GLA_CLIENT_RES_INFO(client_res_info);

	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_res_tree_find_and_add
 
  Description   : This routine adds the new res to the resource tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  res_id : the res handle id.
                  flag   : true/false if true create the new node
 
  Return Values : returns the client_info node.
 
  Notes         : None
******************************************************************************/
GLA_RESOURCE_ID_INFO *gla_res_tree_find_and_add(GLA_CB *gla_cb, SaLckResourceHandleT res_id, bool flag)
{
	GLA_RESOURCE_ID_INFO *res_id_info = NULL;

	if (flag == true) {
		/* create and allocate the memory */
		res_id_info = (GLA_RESOURCE_ID_INFO *)m_MMGR_ALLOC_GLA_RES_ID_INFO;
		if (!res_id_info) {
			/* TBD log message */
			return NULL;
		}
		memset(res_id_info, 0, sizeof(GLA_RESOURCE_ID_INFO));
		/* create the association with hdl-mngr */
		if (0 ==
		    (res_id_info->lcl_res_id =
		     ncshm_create_hdl(NCS_HM_POOL_ID_EXTERNAL2, NCS_SERVICE_ID_GLA, (NCSCONTEXT)res_id_info))) {
			m_MMGR_FREE_GLA_RES_ID_INFO(res_id_info);
			return NULL;
		}

		res_id_info->patnode.key_info = (uint8_t *)&res_id_info->lcl_res_id;

		if (ncs_patricia_tree_add(&gla_cb->gla_resource_id_tree, &res_id_info->patnode) != NCSCC_RC_SUCCESS) {
			ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, res_id_info->lcl_res_id);
			m_MMGR_FREE_GLA_RES_ID_INFO(res_id_info);
			return NULL;
		}
	}
	return res_id_info;
}

/****************************************************************************
  Name          : gla_client_res_tree_find_and_add 

  Description   : This routine adds the new res to the resource tree

  Arguments     :
                  gla_cb : pointer to the gla control block.
                  res_id : the res handle id.
                  flag   : true/false if true create the new node

  Return Values : returns the client_info node.

  Notes         : None
******************************************************************************/
GLA_CLIENT_RES_INFO *gla_client_res_tree_find_and_add(GLA_CLIENT_INFO *client_info,
						      SaLckResourceIdT res_id, bool flag)
{
	GLA_CLIENT_RES_INFO *res_id_info = NULL;

	if (flag == true) {
		/* create and allocate the memory */
		res_id_info = (GLA_CLIENT_RES_INFO *)m_MMGR_ALLOC_GLA_CLIENT_RES_INFO;
		if (!res_id_info) {
			/* TBD log message */
			return NULL;
		}
		memset(res_id_info, 0, sizeof(GLA_CLIENT_RES_INFO));
		res_id_info->gbl_res_id = res_id;
		res_id_info->patnode.key_info = (uint8_t *)&res_id_info->gbl_res_id;

		if (ncs_patricia_tree_add(&client_info->client_res_tree, &res_id_info->patnode) != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_GLA_CLIENT_RES_INFO(res_id_info);
			return NULL;
		}
	} else {
		res_id_info = (GLA_CLIENT_RES_INFO *)ncs_patricia_tree_get(&client_info->client_res_tree,
									   (uint8_t *)&res_id);
	}
	return res_id_info;
}

/****************************************************************************
  Name          : gla_res_tree_delete_node
 
  Description   : This routine deletes the res from the res tree
 
  Arguments     : res_id - handle to the resource id.
                  
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t gla_res_tree_delete_node(GLA_CB *gla_cb, GLA_RESOURCE_ID_INFO *res_info)
{
	/* delete from the tree */
	if (ncs_patricia_tree_del(&gla_cb->gla_resource_id_tree, &res_info->patnode) != NCSCC_RC_SUCCESS) {
		/* TBD log message */
	}

	ncshm_give_hdl(res_info->lcl_res_id);
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, res_info->lcl_res_id);

	gla_stop_tmr(&res_info->res_async_tmr);
	/* free the mem */
	m_MMGR_FREE_GLA_RES_ID_INFO(res_info);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_res_tree_reverse_find
 
  Description   : This routine adds the new res to the resource tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  
  Return Values : returns the res_id_info node.
 
  Notes         : None
******************************************************************************/
GLA_RESOURCE_ID_INFO *gla_res_tree_reverse_find(GLA_CB *gla_cb, SaLckHandleT handle, SaLckResourceIdT gbl_res)
{
	GLA_RESOURCE_ID_INFO *res_id_info;
	SaLckResourceHandleT res_id = 0;
	/* scan the entire handle db & delete each record */
	while ((res_id_info = (GLA_RESOURCE_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_resource_id_tree,
										(uint8_t *)&res_id))) {
		res_id = res_id_info->lcl_res_id;
		if (res_id_info->lock_handle_id == handle && res_id_info->gbl_res_id == gbl_res)
			return res_id_info;
	}

	return NULL;
}

/****************************************************************************
  Name          : gla_res_tree_cleanup_client_down
 
  Description   : This routine adds the new res to the resource tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  
  Return Values : returns the res_id_info node.
 
  Notes         : None
******************************************************************************/
void gla_res_tree_cleanup_client_down(GLA_CB *gla_cb, SaLckHandleT handle)
{
	GLA_RESOURCE_ID_INFO *res_id_info;
	SaLckResourceHandleT res_id = 0;
	/* scan the entire handle db & delete each record */
	while ((res_id_info = (GLA_RESOURCE_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_resource_id_tree,
										(uint8_t *)&res_id))) {
		res_id = res_id_info->lcl_res_id;
		if (res_id_info->lock_handle_id == handle) {
			if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, res_id_info->lcl_res_id))
				gla_res_tree_delete_node(gla_cb, res_id_info);
		}
	}
	return;
}

/****************************************************************************
  Name          : gla_lock_tree_init
  
  Description   : This routine is used to initialize the Lock tree
 
  Arguments     : cb - pointer to the GLA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t gla_lock_tree_init(GLA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaLckLockIdT);
	if (ncs_patricia_tree_init(&cb->gla_lock_id_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_lock_tree_destroy
 
  Description   : This routine destroys the GLA lock tree.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_lock_tree_destroy(GLA_CB *gla_cb)
{
	/* cleanup the client tree */
	gla_lock_tree_cleanup(gla_cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&gla_cb->gla_lock_id_tree);
	return;
}

/****************************************************************************
  Name          : gla_lock_tree_cleanup
 
  Description   : This routine cleansup the GLA lock tree
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void gla_lock_tree_cleanup(GLA_CB *gla_cb)
{
	GLA_LOCK_ID_INFO *lock_id_info;

	/* scan the entire handle db & delete each record */
	while ((lock_id_info = (GLA_LOCK_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_lock_id_tree, 0))) {
		/* delete the client info */
		if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, lock_id_info->lcl_lock_id))
			gla_lock_tree_delete_node(gla_cb, lock_id_info);
	}
	return;
}

/****************************************************************************
  Name          : gla_lock_tree_find_and_add
 
  Description   : This routine adds the new res to the lock tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  lock_id : the lock handle .
                  flag   : true/false if true create the new node
 
  Return Values : returns the lock id node.
 
  Notes         : None
******************************************************************************/
GLA_LOCK_ID_INFO *gla_lock_tree_find_and_add(GLA_CB *gla_cb, SaLckLockIdT lock_id, bool flag)
{
	GLA_LOCK_ID_INFO *lock_id_info = NULL;

	if (flag == true) {
		/* create and allocate the memory */
		lock_id_info = (GLA_LOCK_ID_INFO *)m_MMGR_ALLOC_GLA_LOCK_ID_INFO;
		if (!lock_id_info) {
			/* TBD log message */
			return NULL;
		}
		memset(lock_id_info, 0, sizeof(GLA_LOCK_ID_INFO));
		/* create the association with hdl-mngr */
		if (0 ==
		    (lock_id_info->lcl_lock_id =
		     ncshm_create_hdl(NCS_HM_POOL_ID_EXTERNAL3, NCS_SERVICE_ID_GLA, (NCSCONTEXT)lock_id_info))) {
			m_MMGR_FREE_GLA_RES_ID_INFO(lock_id_info);
			return NULL;
		}

		lock_id_info->patnode.key_info = (uint8_t *)&lock_id_info->lcl_lock_id;

		if (ncs_patricia_tree_add(&gla_cb->gla_lock_id_tree, &lock_id_info->patnode) != NCSCC_RC_SUCCESS) {
			ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, lock_id_info->lcl_lock_id);
			m_MMGR_FREE_GLA_LOCK_ID_INFO(lock_id_info);
			return NULL;
		}
	}
	return lock_id_info;
}

/****************************************************************************
  Name          : gla_lock_tree_delete_node
 
  Description   : This routine deletes the res from the lock tree
 
  Arguments     : lock_id - handle to the lock id.
                  
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t gla_lock_tree_delete_node(GLA_CB *gla_cb, GLA_LOCK_ID_INFO *lock_info)
{

	/* delete from the tree */
	if (ncs_patricia_tree_del(&gla_cb->gla_lock_id_tree, &lock_info->patnode) != NCSCC_RC_SUCCESS) {
		/* TBD log message */
		return NCSCC_RC_FAILURE;
	}

	ncshm_give_hdl(lock_info->lcl_lock_id);

	ncshm_destroy_hdl(NCS_SERVICE_ID_GLA, lock_info->lcl_lock_id);

	gla_stop_tmr(&lock_info->lock_async_tmr);
	gla_stop_tmr(&lock_info->unlock_async_tmr);
	/* free the mem */
	m_MMGR_FREE_GLA_LOCK_ID_INFO(lock_info);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_lock_tree_reverse_find
 
  Description   : This routine adds the new res to the resource tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  
  Return Values : returns the res_id_info node.
 
  Notes         : None
******************************************************************************/
GLA_LOCK_ID_INFO *gla_lock_tree_reverse_find(GLA_CB *gla_cb,
					     SaLckHandleT handle, SaLckResourceIdT gbl_res, SaLckLockIdT gbl_lock)
{
	GLA_LOCK_ID_INFO *lock_id_info;
	SaLckLockIdT lock_id = 0;
	/* scan the entire handle db each record */
	while ((lock_id_info = (GLA_LOCK_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_lock_id_tree,
									     (uint8_t *)&lock_id))) {
		lock_id = lock_id_info->lcl_lock_id;
		if (lock_id_info->lock_handle_id == handle &&
		    lock_id_info->gbl_res_id == gbl_res && lock_id_info->gbl_lock_id == gbl_lock)
			return lock_id_info;
	}

	return NULL;
}

/****************************************************************************
  Name          : gla_lock_tree_cleanup_client_down
 
  Description   : This routine cleans up the lock tree
 
  Arguments     : 
                  gla_cb : pointer to the gla control block.
                  
  Return Values : returns the res_id_info node.
 
  Notes         : None
******************************************************************************/
void gla_lock_tree_cleanup_client_down(GLA_CB *gla_cb, SaLckHandleT handle)
{
	GLA_LOCK_ID_INFO *lock_id_info = NULL;
	SaLckLockIdT lock_id = 0;

	while ((lock_id_info = (GLA_LOCK_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_lock_id_tree,
									     (uint8_t *)&lock_id))) {
		if (lock_id_info->lock_handle_id == handle) {
			lock_id = lock_id_info->lcl_lock_id;
			if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, lock_id_info->lcl_lock_id))
				gla_lock_tree_delete_node(gla_cb, lock_id_info);
		} else {
			lock_id = lock_id_info->lcl_lock_id;
		}
	}

	return;
}

/****************************************************************************
  Name          :gla_res_lock_tree_cleanup_client_down 

  Description   : This routine cleans up the lock tree

  Arguments     :
                  gla_cb : pointer to the gla control block.

  Return Values : returns the res_id_info node.

  Notes         : None
******************************************************************************/
void gla_res_lock_tree_cleanup_client_down(GLA_CB *gla_cb, GLA_RESOURCE_ID_INFO *res_info, SaLckHandleT handle)
{
	GLA_LOCK_ID_INFO *lock_id_info = NULL;
	SaLckLockIdT lock_id = 0;

	/* scan the entire handle db & delete each record */
	while ((lock_id_info = (GLA_LOCK_ID_INFO *)ncs_patricia_tree_getnext(&gla_cb->gla_lock_id_tree,
									     (uint8_t *)&lock_id))) {
		if (lock_id_info->lock_handle_id == handle && lock_id_info->lcl_res_id == res_info->lcl_res_id) {
			lock_id = lock_id_info->lcl_lock_id;
			if (NULL != ncshm_take_hdl(NCS_SERVICE_ID_GLA, lock_id_info->lcl_lock_id))
				gla_lock_tree_delete_node(gla_cb, lock_id_info);
		} else {
			lock_id = lock_id_info->lcl_lock_id;
		}

	}

	return;
}

/****************************************************************************
  Name          :  ncs_gla_startup

  Description   :  This routine creates a GLSv agent infrastructure to interface
                   with GLSv service. Once the infrastructure is created from
                   then on use_count is incremented for every startup request.

  Arguments     :  - NIL-

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_gla_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;
	char *value = NULL;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);
	if (gla_use_count > 0) {
		/* Already created, so just increment the use_count */
		gla_use_count++;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_SUCCESS;
	}

   /*** Init GLA ***/
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	if (gla_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	} else {
		TRACE("GLSV:GLA:ON");
		gla_use_count = 1;
	}

        /* Initialize trace system first of all so we can see what is going. */
        if ((value = getenv("GLA_TRACE_PATHNAME")) != NULL) {
               logtrace_init("gla", value, CATEGORY_ALL);
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  ncs_gla_shutdown 

  Description   :  This routine destroys the GLSv agent infrastructure created 
                   to interface GLSv service. If the registered users are > 1, 
                   it just decrements the use_count.   

  Arguments     :  - NIL -

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_gla_shutdown(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);
	if (gla_use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		gla_use_count--;
	} else if (gla_use_count == 1) {
		NCS_LIB_REQ_INFO lib_destroy;

		memset(&lib_destroy, 0, sizeof(lib_destroy));
		lib_destroy.i_op = NCS_LIB_REQ_DESTROY;

		rc = gla_lib_req(&lib_destroy);

		gla_use_count = 0;
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
	return rc;
}
