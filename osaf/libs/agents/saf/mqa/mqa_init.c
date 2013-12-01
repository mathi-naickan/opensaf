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

  This file contains the initialization and destroy routines for MQA library.
..............................................................................

  FUNCTIONS INCLUDED in this module:

******************************************************************************
*/

#include "mqa.h"
#include <pthread.h>
#include "osaf_utility.h"
#include "osaf_poll.h"

/* global cb handle */
uint32_t gl_mqa_hdl = 0;
MQA_CB mqa_cb;
static uint32_t mqa_use_count = 0;

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

static void mqa_sync_with_mqnd(MQA_CB *cb);
static void mqa_sync_with_mqd(MQA_CB *cb);
static void mqa_asapi_unregister(MQA_CB *cb);
static void mqa_client_tree_destroy(MQA_CB *mqa_cb);
static void mqa_client_tree_cleanup(MQA_CB *mqa_cb);
static void mqa_queue_tree_destroy(MQA_CB *mqa_cb);
static void mqa_queue_tree_cleanup(MQA_CB *mqa_cb);
static uint32_t mqa_create(NCS_LIB_CREATE *create_info);
static uint32_t mqa_destroy(NCS_LIB_DESTROY *destroy_info);
static uint32_t mqa_asapi_register(MQA_CB *cb);
static uint32_t mqa_client_tree_init(MQA_CB *cb);
static uint32_t mqa_queue_tree_init(MQA_CB *cb);

/****************************************************************************
  Name          : mqa_lib_req
 
  Description   : This routine is exported to the external entities & is used
                  to create & destroy the MQSv library.
 
  Arguments     : req_info - ptr to the request info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqa_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = mqa_create(&req_info->info.create);
		if (rc == NCSCC_RC_SUCCESS)
			TRACE_1("MsgQ Svc Registration Success");
		else
			TRACE_2("FAILURE: MsgQ Svc Registration Failed");
		break;

	case NCS_LIB_REQ_DESTROY:
		rc = mqa_destroy(&req_info->info.destroy);
		if (rc == NCSCC_RC_SUCCESS)
			TRACE_1("MsgQ Svc Deregistration Success");
		else
			TRACE_2("FAILURE: MsgQ Svc Deregistration Failed");
		break;

	default:
		TRACE_2("MsgQ Svc Req unknown");
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/********************************************************************
 Name    :  mqa_sync_with_mqd

 Description : This is for MQA to sync with MQD when it gets MDS callback

**********************************************************************/
static void mqa_sync_with_mqd(MQA_CB *cb)
{
	TRACE_ENTER();

	m_NCS_LOCK(&cb->mqd_sync_lock, NCS_LOCK_WRITE);

	if (cb->is_mqd_up) {
		TRACE_1("MQD is already up with the MQA");
		m_NCS_UNLOCK(&cb->mqd_sync_lock, NCS_LOCK_WRITE);
		return;
	}

	cb->mqd_sync_awaited = true;
	m_NCS_SEL_OBJ_CREATE(&cb->mqd_sync_sel);
	m_NCS_UNLOCK(&cb->mqd_sync_lock, NCS_LOCK_WRITE);

	/* Await indication from MDS saying MQND is up */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(cb->mqd_sync_sel), 30000);

	/* Destroy the sync - object */
	m_NCS_LOCK(&cb->mqd_sync_lock, NCS_LOCK_WRITE);

	cb->mqd_sync_awaited = false;
	m_NCS_SEL_OBJ_DESTROY(cb->mqd_sync_sel);

	m_NCS_UNLOCK(&cb->mqd_sync_lock, NCS_LOCK_WRITE);
	TRACE_1("MQD synced up with the MQA");
	TRACE_LEAVE();
	return;
}

/********************************************************************
 Name    :  mqa_sync_with_mqnd

 Description : This is for MQA to sync with MQND when it gets MDS callback
 
**********************************************************************/
static void mqa_sync_with_mqnd(MQA_CB *cb)
{
	TRACE_ENTER();

	m_NCS_LOCK(&cb->mqnd_sync_lock, NCS_LOCK_WRITE);

	if (cb->is_mqnd_up) {
		TRACE_1("MQND is already up with the MQA");
		m_NCS_UNLOCK(&cb->mqnd_sync_lock, NCS_LOCK_WRITE);
		return;
	}

	cb->mqnd_sync_awaited = true;
	m_NCS_SEL_OBJ_CREATE(&cb->mqnd_sync_sel);
	m_NCS_UNLOCK(&cb->mqnd_sync_lock, NCS_LOCK_WRITE);

	/* Await indication from MDS saying MQND is up */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(cb->mqnd_sync_sel), 30000);

	/* Destroy the sync - object */
	m_NCS_LOCK(&cb->mqnd_sync_lock, NCS_LOCK_WRITE);

	cb->mqnd_sync_awaited = false;
	m_NCS_SEL_OBJ_DESTROY(cb->mqnd_sync_sel);

	m_NCS_UNLOCK(&cb->mqnd_sync_lock, NCS_LOCK_WRITE);
	TRACE_1("MQND synced up with the MQA");
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_create
 
  Description   : This routine creates & initializes the MQA control block.
 
  Arguments     : create_info - ptr to the create info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t mqa_create(NCS_LIB_CREATE *create_info)
{
	MQA_CB *cb = &mqa_cb;

	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* validate create info */
	if (create_info == NULL)
		return NCSCC_RC_FAILURE;

	memset(cb, 0, sizeof(MQA_CB));

	/* assign the MQA pool-id (used by hdl-mngr) */
	cb->pool_id = NCS_HM_POOL_ID_COMMON;

	/* create the association with hdl-mngr */
	if (!(cb->agent_handle_id = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_MQA, (NCSCONTEXT)cb))) {
		TRACE_2("Handle Registration Failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_1("Handle Registration Success");
	/* everything went off well.. store the hdl in the global variable */
	gl_mqa_hdl = cb->agent_handle_id;

	/* get the process id */
	cb->process_id = getpid();

	/* initialize the mqa cb lock */
	if ((rc = m_NCS_LOCK_INIT(&cb->cb_lock)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Cotnrol Block Lock Initialisation Failed eith return value %d", rc);
		goto error1;
	}
	TRACE_1("Control Block lock initialization Success");

	/* initialize the client tree */
	if ((rc = mqa_client_tree_init(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Client database Initialization Failed");
		goto error2;
	}

	TRACE_1("Client Database Initialization Success");
	/* initialize the queue tree */
	if ((rc = mqa_queue_tree_init(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Queue database Initialization Failed");
		goto error3;
	}
	TRACE_1("Queue Database Initialization Success");

	/* EDU initialisation */
	if ((rc = m_NCS_EDU_HDL_INIT(&cb->edu_hdl)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Edu Handle Initialization Failed");
		goto error4;
	}

	TRACE_1("EDU Handle Initialization Success");
	/* register with MDS */
	if ((rc = mqa_mds_register(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: MDS registration Failed");
		goto error5;
	} else
		TRACE_1("MDS Registration Success");

	/* Update clm_node_joined flag to 1 */

	cb->clm_node_joined = 1;

	mqa_sync_with_mqd(cb);

	mqa_sync_with_mqnd(cb);

	/* initialize the timeout linked list */
	if ((rc = mqa_timer_table_init(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Initialization Failed");
		goto error6;
	}

	if ((rc = mqa_asapi_register(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Registration with ASAPi Failed");
		goto error7;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 error7:
	mqa_timer_table_destroy(cb);

 error6:
	/* MDS unregister. */
	mqa_mds_unregister(cb);

 error5:
	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);

 error4:
	/* delete the tree */
	mqa_queue_tree_destroy(cb);

 error3:
	/* delete the tree */
	mqa_client_tree_destroy(cb);
 error2:
	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

 error1:
	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_MQA, cb->agent_handle_id);

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : mqa_destroy
 
  Description   : This routine destroys the MQA control block.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t mqa_destroy(NCS_LIB_DESTROY *destroy_info)
{
	MQA_CB *cb = 0;
	TRACE_ENTER();

	/* validate the CB */
	cb = (MQA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_MQA, gl_mqa_hdl);
	if (!cb)
		return NCSCC_RC_FAILURE;

	/* return MQA CB */
	ncshm_give_hdl(gl_mqa_hdl);

	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_MQA, cb->agent_handle_id);

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS)	{
		TRACE_4("FAILURE: mqa_destroy Failed to acquire lock");
		return NCSCC_RC_FAILURE;
	}

	mqa_timer_table_destroy(cb);

	/* Unregister with ASAPi */
	mqa_asapi_unregister(cb);

	/* MDS unregister. */
	mqa_mds_unregister(cb);

	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);

	mqa_queue_tree_destroy(cb);

	/* delete all the client info */
	mqa_client_tree_destroy(cb);

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

	/* reset the global cb handle */
	gl_mqa_hdl = 0;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_asapi_register
 
  Description   : This routine registers with ASAPi library.
 
  Arguments     : cb - MQA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t mqa_asapi_register(MQA_CB *cb)
{
	ASAPi_OPR_INFO asapi_or;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Register with ASAPi library */

	asapi_or.type = ASAPi_OPR_BIND;
	asapi_or.info.bind.i_indhdlr = mqa_asapi_msghandler;
	asapi_or.info.bind.i_mds_hdl = cb->mqa_mds_hdl;
	asapi_or.info.bind.i_mds_id = NCSMDS_SVC_ID_MQA;
	asapi_or.info.bind.i_my_id = NCS_SERVICE_ID_MQA;
	asapi_or.info.bind.i_mydest = cb->mqa_mds_dest;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: ASAPi Bind Failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_asapi_unregister
 
  Description   : This routine registers with ASAPi library.
 
  Arguments     : cb - MQA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static void mqa_asapi_unregister(MQA_CB *cb)
{
	ASAPi_OPR_INFO asapi_or;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Register with ASAPi library */

	asapi_or.type = ASAPi_OPR_UNBIND;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != NCSCC_RC_SUCCESS) {
		TRACE_2("ASAPi Deregisteration Failed with returncode %d", rc);
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_client_tree_init
  
  Description   : This routine is used to initialize the client tree
 
  Arguments     : cb - pointer to the MQA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t mqa_client_tree_init(MQA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	TRACE_ENTER();

	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaMsgHandleT);
	if (ncs_patricia_tree_init(&cb->mqa_client_tree, &param) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: initialization of the client tree failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_client_tree_destroy
 
  Description   : This routine destroys the MQA client tree.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void mqa_client_tree_destroy(MQA_CB *mqa_cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* take the cb lock */
	if ((rc = m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Client database Finalization Failed");
		return;
	}
	/* cleanup the client tree */
	mqa_client_tree_cleanup(mqa_cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&mqa_cb->mqa_client_tree);

	/* giveup the cb lock */
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_client_tree_cleanup
 
  Description   : This routine cleansup the MQA Client tree
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void mqa_client_tree_cleanup(MQA_CB *mqa_cb)
{
	MQA_CLIENT_INFO *client_info;
	SaMsgHandleT *temp_ptr = 0;
	SaMsgHandleT temp_hdl = 0;
	TRACE_ENTER();

	/* scan the entire handle db & delete each record */
	while ((client_info = (MQA_CLIENT_INFO *)
		ncs_patricia_tree_getnext(&mqa_cb->mqa_client_tree, (uint8_t *const)temp_ptr))) {
		/* delete the client info */
		temp_hdl = client_info->msgHandle;
		temp_ptr = &temp_hdl;
		mqsv_mqa_callback_queue_destroy(client_info);
		mqa_client_tree_delete_node(mqa_cb, client_info);
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_is_track_enabled
 
  Description   : This routine checks if there is any tracking
                  enabled for a group
 
  Arguments     : mqa_cb - MQA Control block.
                  queueGroupName - Group name
 
  Return Values : true if this is no tracking enabled for this group. false
                  otherwise
 
  Notes         : None
******************************************************************************/
bool mqa_is_track_enabled(MQA_CB *mqa_cb, SaNameT *queueGroupName)
{
	MQA_CLIENT_INFO *client_info;
	MQA_TRACK_INFO *track_info;
	SaMsgHandleT *temp_ptr = 0;
	SaMsgHandleT temp_hdl = 0;

	/* scan the entire handle db & delete each record */
	while ((client_info = (MQA_CLIENT_INFO *)
		ncs_patricia_tree_getnext(&mqa_cb->mqa_client_tree, (uint8_t *const)temp_ptr))) {
		track_info = mqa_track_tree_find_and_add(client_info, queueGroupName, false);
		if (!track_info) {
			temp_hdl = client_info->msgHandle;
			temp_ptr = &temp_hdl;
			continue;
		} else
			return true;
	}

	return false;
}

/****************************************************************************
  Name          : mqa_client_tree_delete_node
 
  Description   : This routine adds the deletes the client from the client tree
 
  Arguments     : client_info - pointer to the client node.
                  
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t mqa_client_tree_delete_node(MQA_CB *mqa_cb, MQA_CLIENT_INFO *client_info)
{
	MQA_TRACK_INFO *track_info;
	SaNameT *temp_ptr = 0;
	SaNameT temp_name;
	uint8_t *value = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* scan the entire group track db & delete each record */
	while ((track_info = (MQA_TRACK_INFO *)ncs_patricia_tree_getnext(&client_info->mqa_track_tree, value))) {
		/* delete the track info */
		temp_name = track_info->queueGroupName;
		temp_ptr = &temp_name;
		value = temp_ptr->value;
		/* delete from the tree */
		if ((rc = ncs_patricia_tree_del(&client_info->mqa_track_tree,
						&track_info->patnode)) != NCSCC_RC_SUCCESS)
			TRACE_2("Track Database Deregistration Failed");

		/* free the mem */
		if (track_info->notificationBuffer.notification)
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
		m_MMGR_FREE_MQA_TRACK_INFO(track_info);
	}

	/* destroy the tree */
	ncs_patricia_tree_destroy(&client_info->mqa_track_tree);

	/* delete from the tree */
	if ((rc = ncs_patricia_tree_del(&mqa_cb->mqa_client_tree, &client_info->patnode)) != NCSCC_RC_SUCCESS)
		TRACE_2("Client database Deregistration Failed");

	/* free the mem */
	m_MMGR_FREE_MQA_CLIENT_INFO(client_info);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_client_tree_find_and_add
 
  Description   : This routine adds the new client to the client tree and creates
                  the selection object for that client.
 
  Arguments     : 
                  mqa_cb : pointer to the mqa control block.
                  hdl_id : the handle id.
                  flag   : true/false if true create the new node if node 
                           doesn't exist.FALSE -> search for an existing node.
 
  Return Values : returns the MQA_CLIENT_INFO node.
 
  Notes         : The caller takes the cb lock before calling this function
                  
******************************************************************************/
MQA_CLIENT_INFO *mqa_client_tree_find_and_add(MQA_CB *mqa_cb, SaMsgHandleT hdl_id, bool flag)
{
	MQA_CLIENT_INFO *client_info = NULL;
	NCS_PATRICIA_PARAMS param;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	client_info = (MQA_CLIENT_INFO *)ncs_patricia_tree_get(&mqa_cb->mqa_client_tree, (uint8_t *)&hdl_id);

	if (flag == true) {
		/* create and allocate the memory */
		if (!client_info) {
			client_info = (MQA_CLIENT_INFO *)m_MMGR_ALLOC_MQA_CLIENT_INFO;
			if (!client_info) {
				TRACE_4("Client database creation failed");
				return NULL;
			}
			memset(client_info, 0, sizeof(MQA_CLIENT_INFO));
			/* Store the client_info pointer as msghandle. */
			client_info->msgHandle = NCS_PTR_TO_UNS64_CAST(client_info);
			client_info->patnode.key_info = (uint8_t *)&client_info->msgHandle;

			if ((rc =
			     ncs_patricia_tree_add(&mqa_cb->mqa_client_tree,
						   &client_info->patnode)) != NCSCC_RC_SUCCESS) {
				TRACE_2("Client database Registration Failed");
				m_MMGR_FREE_MQA_CLIENT_INFO(client_info);
				return NULL;
			}

			/* Create the group track tree */
			memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
			param.key_size = SA_MAX_NAME_LENGTH;
			if (ncs_patricia_tree_init(&client_info->mqa_track_tree, &param) != NCSCC_RC_SUCCESS) {
				TRACE_2("Initialization of the client tree failed");
				m_MMGR_FREE_MQA_CLIENT_INFO(client_info);
				return NULL;
			}

		}
	}
	TRACE_LEAVE();
	return client_info;
}

/****************************************************************************
  Name          : mqa_track_tree_find_and_add

  Description   : This routine adds/searches the new group track element to the
                  client tree.

  Arguments     :
                  client_info : pointer to the mqa client.
                  hdl_id : the handle id.
                  flag   : true/false if true create the new node if node
                           doesn't exist.FALSE -> search for an existing node.

  Return Values : returns the MQA_TRACK_INFO node.

  Notes         : The caller takes the cb lock before calling this function

******************************************************************************/
MQA_TRACK_INFO *mqa_track_tree_find_and_add(MQA_CLIENT_INFO *client_info, SaNameT *group, bool flag)
{
	MQA_TRACK_INFO *track_info = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	track_info = (MQA_TRACK_INFO *)ncs_patricia_tree_get(&client_info->mqa_track_tree, (uint8_t *)group->value);

	if (flag == true) {
		/* create and allocate the memory */
		if (!track_info) {
			track_info = (MQA_TRACK_INFO *)m_MMGR_ALLOC_MQA_TRACK_INFO;
			if (!track_info) {
				TRACE_2("Track database creation failed");
				return NULL;
			}
			memset(track_info, 0, sizeof(MQA_TRACK_INFO));
			track_info->queueGroupName = *group;
			track_info->patnode.key_info = (uint8_t *)track_info->queueGroupName.value;
			if ((rc = ncs_patricia_tree_add(&client_info->mqa_track_tree,
							&track_info->patnode)) != NCSCC_RC_SUCCESS) {
				TRACE_2("Track Database Registration Failed");
				if (track_info->notificationBuffer.notification)
					m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
				m_MMGR_FREE_MQA_TRACK_INFO(track_info);
				return NULL;
			}

		}
	}
	TRACE_LEAVE();
	return track_info;

}

/****************************************************************************
  Name          : mqa_track_tree_find_and_del

  Description   : This routine deletes a new group track element to the
                  client tree.

  Arguments     :
                  client_info : pointer to the mqa client.
                  group : the group name to delete from track list.

  Return Values : returns status of deletion, true if deleted, false otherwise

  Notes         : The caller takes the cb lock before calling this function

******************************************************************************/
bool mqa_track_tree_find_and_del(MQA_CLIENT_INFO *client_info, SaNameT *group)
{
	MQA_TRACK_INFO *track_info = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	track_info = (MQA_TRACK_INFO *)ncs_patricia_tree_get(&client_info->mqa_track_tree, (uint8_t *)group->value);

	if (!track_info)
		return false;

	if ((rc = ncs_patricia_tree_del(&client_info->mqa_track_tree, &track_info->patnode)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Track Database Deregistration Failed");
		if (track_info->notificationBuffer.notification)
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
		m_MMGR_FREE_MQA_TRACK_INFO(track_info);
		return false;
	}

	if (track_info->notificationBuffer.notification)
		m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
	m_MMGR_FREE_MQA_TRACK_INFO(track_info);

	TRACE_LEAVE();
	return true;
}

/****************************************************************************
  Name          : mqa_queue_tree_init
  
  Description   : This routine is used to initialize the queue tree
 
  Arguments     : cb - pointer to the MQA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t mqa_queue_tree_init(MQA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	TRACE_ENTER();

	uint32_t rc = NCSCC_RC_SUCCESS;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaMsgQueueHandleT);
	if ((rc = ncs_patricia_tree_init(&cb->mqa_queue_tree, &param)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Initialization of the queue tree failed");
		return rc;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_client_queue_destroy
 
  Description   : This routine destroys the MQA queue tree.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void mqa_queue_tree_destroy(MQA_CB *mqa_cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* take the cb lock */
	if ((rc = (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE))) != NCSCC_RC_SUCCESS) {
		TRACE_2("Queue database Finalization Failed");
		return;
	}

	/* cleanup the client tree */
	mqa_queue_tree_cleanup(mqa_cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&mqa_cb->mqa_queue_tree);

	/* giveup the cb lock */
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_queue_tree_cleanup
 
  Description   : This routine cleansup the MQA queue tree
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void mqa_queue_tree_cleanup(MQA_CB *mqa_cb)
{
	MQA_QUEUE_INFO *queue_info;
	SaMsgQueueHandleT *temp_ptr = NULL;
	SaMsgQueueHandleT temp_hdl;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* scan the entire handle db & delete each record */
	while ((queue_info = (MQA_QUEUE_INFO *)
		ncs_patricia_tree_getnext(&mqa_cb->mqa_queue_tree, (uint8_t *const)temp_ptr))) {
		temp_hdl = queue_info->queueHandle;
		temp_ptr = &temp_hdl;

		/* delete the client info */
		if ((rc = ncs_patricia_tree_del(&mqa_cb->mqa_queue_tree, &queue_info->patnode)) != NCSCC_RC_SUCCESS) {
			TRACE_2("Queue database Deregistration Failed");
		} else {
			/* free the mem */
			m_MMGR_FREE_MQA_QUEUE_INFO(queue_info);
		}

	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : mqa_queue_tree_find_and_add
 
  Description   : This routine adds the new queue to the queue tree
 
  Arguments     : 
                  mqa_cb : pointer to the mqa control block.
                  hdl_id : the handle id.
                  flag   : true/false if true create the new node if node 
                           doesn't exist.FALSE -> search for an existing node.
                 MQA_CLIENT_INFO *client_info : Info about the client.
                 SaMsgQueueOpenFlagsT openFlags : 
                          If flag == true, openFlags is IN parameter.
                          If flag != true, openFlags is ignored.
  Return Values : returns the MQA_QUEUE_INFO node.
 
  Notes         : None
******************************************************************************/
MQA_QUEUE_INFO *mqa_queue_tree_find_and_add(MQA_CB *mqa_cb,
					    SaMsgQueueHandleT hdl_id,
					    bool flag, MQA_CLIENT_INFO *client_info, SaMsgQueueOpenFlagsT openFlags)
{
	MQA_QUEUE_INFO *queue_info = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* read lock taken by the caller. */
	queue_info = (MQA_QUEUE_INFO *)ncs_patricia_tree_get(&mqa_cb->mqa_queue_tree, (uint8_t *)&hdl_id);

	if (flag == true) {
		/* create and allocate the memory */
		if (!queue_info) {
			queue_info = (MQA_QUEUE_INFO *)m_MMGR_ALLOC_MQA_QUEUE_INFO;
			if (!queue_info) {
				TRACE_2("Queue database creation failed");
				return NULL;
			}
			memset(queue_info, 0, sizeof(MQA_QUEUE_INFO));
			queue_info->queueHandle = hdl_id;
			queue_info->openFlags = openFlags;
			queue_info->client_info = client_info;
			queue_info->patnode.key_info = (uint8_t *)&queue_info->queueHandle;
			queue_info->msg_get_count = 0;

			if ((rc =
			     ncs_patricia_tree_add(&mqa_cb->mqa_queue_tree,
						   &queue_info->patnode)) != NCSCC_RC_SUCCESS) {
				TRACE_2("Queue database Registration Failed");
				m_MMGR_FREE_MQA_QUEUE_INFO(queue_info);

				return NULL;
			}

		}
	}
	TRACE_LEAVE();
	return queue_info;
}

/****************************************************************************
  Name          : mqa_queue_tree_delete_node
 
  Description   : This routine adds the deletes the queue from the queue tree
 
  Arguments     : queue_info - pointer to the queue node.
                  
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t mqa_queue_tree_delete_node(MQA_CB *mqa_cb, SaMsgQueueHandleT hdl_id)
{

	MQA_QUEUE_INFO *queue_info = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	queue_info = (MQA_QUEUE_INFO *)ncs_patricia_tree_get(&mqa_cb->mqa_queue_tree, (uint8_t *)&hdl_id);
	if (!queue_info) {
		TRACE_2("FAILURE: Adding the queue in the queue tree is failed");
		return NCSCC_RC_FAILURE;
	}

	/* delete from the tree */
	if ((rc = ncs_patricia_tree_del(&mqa_cb->mqa_queue_tree, &queue_info->patnode)) != NCSCC_RC_SUCCESS)
		TRACE_2("Queue database Deregistration Failed");

	if (queue_info->task_handle != 0) {
		/* m_NCS_TASK_STOP(queue_info->task_handle);
		   m_NCS_TASK_RELEASE(queue_info->task_handle); */
	}

	/* free the mem */
	m_MMGR_FREE_MQA_QUEUE_INFO(queue_info);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  ncs_mqa_startup

  Description   :  This routine creates a MQSv agent infrastructure to interface
                   with MQSv service. Once the infrastructure is created from
                   then on use_count is incremented for every startup request.

  Arguments     :  - NIL-

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_mqa_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;
	char *value = NULL;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);
	if (mqa_use_count > 0) {
		/* Already created, so just increment the use_count */
		mqa_use_count++;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_SUCCESS;
	}

   /*** Init MQA ***/
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	if (mqa_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	} else {
		m_NCS_DBG_PRINTF("\nMQSV:MQA:ON");
		mqa_use_count = 1;
	}

	/* Initialize trace system first of all so we can see what is going. */
	if ((value = getenv("MQA_TRACE_PATHNAME")) != NULL) {
		logtrace_init("mqa", value, CATEGORY_ALL);
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  ncs_mqa_shutdown 

  Description   :  This routine destroys the MQSv agent infrastructure created 
                   to interface MQSv service. If the registered users are > 1, 
                   it just decrements the use_count.   

  Arguments     :  - NIL -

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_mqa_shutdown(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);
	if (mqa_use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		mqa_use_count--;
	} else if (mqa_use_count == 1) {
		NCS_LIB_REQ_INFO lib_destroy;

		memset(&lib_destroy, 0, sizeof(lib_destroy));
		lib_destroy.i_op = NCS_LIB_REQ_DESTROY;

		rc = mqa_lib_req(&lib_destroy);

		mqa_use_count = 0;
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
	TRACE_LEAVE();
	return rc;
}
