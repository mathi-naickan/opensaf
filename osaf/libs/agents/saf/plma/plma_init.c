/*      -*- OpenSAF  -*-
 *
 * (C)  Copyright 2010 The OpenSAF Foundation
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

/*************************************************************************//** 
 * @file	plma_init.c
 * @brief	This file contains the initialization and destroy routines for 
 *		PLMA library.
 *
 * @author	Emerson Network Power
*****************************************************************************/

#include "ncssysf_def.h"

#include <plma.h>
#include <pthread.h>
#include "ncsgl_defs.h"

static PLMA_CB _plma_cb;
PLMA_CB   *plma_ctrlblk;

uint32_t plma_use_count = 0;
void plma_sync_with_plms(void);

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Lock the mutex protecting agent startup and shutdown.
 *
 * This function locks the agent startup mutex. The mutex is static and requires
 * neither initialisation nor finalisation. After a thread has called
 * plma_agent_startup_mutex_lock(), that same thread must call
 * plma_agent_startup_mutex_unlock() to release the lock. The thread is not
 * allowed to call plma_agent_startup_mutex_lock() again without first calling
 * plma_agent_startup_mutex_unlock() to unlock it.
 */
static void plma_agent_startup_mutex_lock(void)
{
	int result = pthread_mutex_lock(&s_agent_startup_mutex);
	/* Should never fail. If it does, it indicates a serious fault,
	   e.g. memory corruption. */
	osafassert(result == 0);
}

/**
 * @brief Unlock the mutex protecting agent startup and shutdown.
 *
 * This function unlocks the agent startup mutex so that other threads can take
 * it and call the agent startup and shutdown functions. Only the thread that
 * locked the mutex is allowed to unlock it, and it is illegal to unlock a mutex
 * that is not locked.
 */
static void plma_agent_startup_mutex_unlock(void)
{
	int result = pthread_mutex_unlock(&s_agent_startup_mutex);
	/* Should never fail. If it does, it indicates a serious fault,
	   e.g. memory corruption or trying to unlock the mutex when it
	   wasn't locked by the calling thread. */
	osafassert(result == 0);
}

/***********************************************************************//**
* @brief	This routine is used to initialize the client tree.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_client_tree_init()
{
	NCS_PATRICIA_PARAMS param;
	PLMA_CB *cb = plma_ctrlblk;
	TRACE_ENTER();
	
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaPlmHandleT);
	if (ncs_patricia_tree_init(&cb->client_info, &param) 
						!= NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: CLIENT TREE INIT FAILED");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}


/***********************************************************************//**
* @brief	This routine is used to initialize the group tree.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.	
***************************************************************************/
uint32_t plma_group_tree_init()
{
	NCS_PATRICIA_PARAMS param;
	PLMA_CB *cb = plma_ctrlblk;
	TRACE_ENTER();
	
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaPlmEntityGroupHandleT);
	if (ncs_patricia_tree_init(&cb->entity_group_info, &param) 
						!= NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: GROUP TREE INIT FAILED");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************//**
* @brief	This routine is used to destroy the client tree.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_client_tree_destroy()
{
	/** cleanup the client tree */
	PLMA_CLIENT_INFO *client_node;
	PLMA_CB *cb = plma_ctrlblk;
	SaPlmHandleT *temp_ptr = 0;
	SaPlmHandleT temp_hdl = 0;
	uint32_t    rc = NCSCC_RC_SUCCESS;


	TRACE_ENTER();	
	/** scan the entire handle db & delete each record */
	while ((client_node = (PLMA_CLIENT_INFO *)
		ncs_patricia_tree_getnext(&cb->client_info, (uint8_t *)temp_ptr)))
	{		
		/** delete the client info */
		temp_hdl = client_node->plm_handle;
		temp_ptr = &temp_hdl;

		/** Destroy the IPC attached to this client */
		plma_callback_ipc_destroy(client_node); 

		 if (client_node != NULL){
			/** Remove the Node from the client tree */
			if (ncs_patricia_tree_del(&cb->client_info, 
				&client_node->pat_node) != NCSCC_RC_SUCCESS){
				LOG_ER("PLMA: CLIENT NODE DELETE FAILED");
		                rc = NCSCC_RC_FAILURE;
	        	}
			clean_client_info_node(client_node);
			free(client_node);
		 }
							
			
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This routine is used to destroy the group info tree.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_group_tree_destroy()
{
	/* cleanup the client tree */
	PLMA_CB *cb = plma_ctrlblk;
	PLMA_ENTITY_GROUP_INFO *grp_info_node;
	SaPlmEntityGroupHandleT *temp_ptr = 0;
	SaPlmEntityGroupHandleT temp_hdl = 0;
	uint32_t    rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();	
	/* scan the entire handle db & delete each record */
	while ((grp_info_node = (PLMA_ENTITY_GROUP_INFO *)
		ncs_patricia_tree_getnext(&cb->entity_group_info, (uint8_t *)temp_ptr)))
	{		
		/* delete the client info */
		temp_hdl = grp_info_node->entity_group_handle;
		temp_ptr = &temp_hdl;

		 if (grp_info_node != NULL){
			/* Remove the Node from the client tree */
			if (ncs_patricia_tree_del(&cb->entity_group_info, &grp_info_node->pat_node) != NCSCC_RC_SUCCESS){
				LOG_ER("PLMA: GROUP_INFO NODE DELETE FAILED");
		                rc = NCSCC_RC_FAILURE;
	        	}
			clean_group_info_node(grp_info_node);
			free(grp_info_node);
		 }
							
			
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This routine creates and initializes the PLMA Control block.
*
* @param[in]	create_info - A pointer to the structure that has creation 
*			      parameters, if any, are provided to libraries 
*			      in a command-line arguments style.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_create(NCS_LIB_CREATE *create_info)
{
	
	PLMA_CB *plma_cb;
	uint32_t    rc = NCSCC_RC_SUCCESS;
	if(!plma_ctrlblk){
		plma_ctrlblk = &_plma_cb;
	}

	plma_cb = plma_ctrlblk;
	
	TRACE_ENTER();
	/** validate create info */
	if (create_info == NULL){
		LOG_ER("PLMA : INVALID create_info RECEIVED");
		rc =  NCSCC_RC_FAILURE;
		goto end;
	}
	
	
	memset(plma_cb, 0, sizeof(PLMA_CB));


	/** Initialize the PLMA_CB lock */
	if (m_NCS_LOCK_INIT(&plma_cb->cb_lock) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: cb_lock INIT FAILED");
		rc = NCSCC_RC_FAILURE;
		goto lock_fail;
	}
	
	/** initialize the client tree */
	if (plma_client_tree_init() != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: CLIENT TREE INIT FAILED");
		rc = NCSCC_RC_FAILURE;
		goto cl_tree_init_fail;
	}
	
	/** initialize the group tree */
	if (plma_group_tree_init() != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: GROUP INFO TREE INIT FAILED");
		rc = NCSCC_RC_FAILURE;
		goto gr_tree_init_fail;
	}
	
	/** register with MDS */
	if (plma_mds_register() != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: MDS REGISTER FAILED");
		rc = NCSCC_RC_FAILURE;
		goto mds_reg_fail;
	}

	/** EDU initialisation */
	if (m_NCS_EDU_HDL_INIT(&plma_cb->edu_hdl) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: EDU HDL INIT FAILED");
		rc = NCSCC_RC_FAILURE;
		goto edu_init_fail;
	}

	
	plma_sync_with_plms();
	
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
	

edu_init_fail:
	plma_mds_unregister();
mds_reg_fail:
	plma_group_tree_destroy();
gr_tree_init_fail:
	plma_client_tree_destroy();
cl_tree_init_fail:
	/** destroy the lock */
	m_NCS_LOCK_DESTROY(&plma_cb->cb_lock);
lock_fail:
	/** make reference to plma_cb obsolete */ 
	plma_ctrlblk = NULL;
end:
	TRACE_LEAVE();
	return rc;
	

}

/***********************************************************************//**
* @brief	This routine destroys the PLMA Control block.
*
* @param[in]	destroy_info - A pointer to the NCS_LIB_DESTROY structure.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_destroy(NCS_LIB_DESTROY *destroy_info)
{
	PLMA_CB *plma_cb = plma_ctrlblk;
	
	TRACE_ENTER();

	/* MDS unregister. */
	plma_mds_unregister();

	/** flush the EDU handle */
	m_NCS_EDU_HDL_FLUSH(&plma_cb->edu_hdl);	

	/** destroy the client tree */
	plma_client_tree_destroy();

	/** destroy the lock */
	m_NCS_LOCK_DESTROY(&plma_cb->cb_lock);

	/** make reference to plma_cb obsolete */ 
	plma_ctrlblk = NULL;


	TRACE_LEAVE();
	
	return NCSCC_RC_SUCCESS;
}

	
/***********************************************************************//**
* @brief	This routine is exported to the external entities & is used
*		to create & destroy the PLMA library.
*
* @param[in]	req_info - ptr to the request info
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = plma_create(&req_info->info.create);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("PLMA : LIB CREATE FAILED");	
		}
		break;
	case NCS_LIB_REQ_DESTROY:
		rc = plma_destroy(&req_info->info.destroy);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("PLMA : LIB DESTROY FAILED");	
		}
		break;
	default:
		break;
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This routine creates a PLMSv agent infrastructure to interface
*		with PLMSv service. Once the infrastructure is created from
*		then on use_count is incremented for every startup request.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t ncs_plma_startup()
{
	NCS_LIB_REQ_INFO lib_create;
		
	TRACE_ENTER();
	plma_agent_startup_mutex_lock();
	if (plma_use_count > 0) {
		/** Already created, so just increment the use_count */
		plma_use_count++;
		plma_agent_startup_mutex_unlock();
		return NCSCC_RC_SUCCESS;
	}
	/** Initialize PLMA library */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	if (plma_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		plma_agent_startup_mutex_unlock();
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}else{
		/** Initialize the library for the first time */
		m_NCS_DBG_PRINTF("\nPLMSV:PLMA:ON");
		plma_use_count = 1;
	}
	plma_agent_startup_mutex_unlock();

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************//**
* @brief	This routine destroys the  PLMSv agent infrastructure created 
*		to nterface with PLMSv service. If the registered users are >1,
*		it just decrements the use_count.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t ncs_plma_shutdown()
{
	NCS_LIB_REQ_INFO lib_destroy;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	
	plma_agent_startup_mutex_lock();


	if (plma_use_count > 1) {
		/** Still users exists, so just decrement the use_count */
		plma_use_count--;
	} else if (plma_use_count == 1) {
		/** last user, destroy the library */
		memset(&lib_destroy, 0, sizeof(lib_destroy));
		lib_destroy.i_op = NCS_LIB_REQ_DESTROY;
		rc = plma_lib_req(&lib_destroy);
		plma_use_count = 0;
	}

	plma_agent_startup_mutex_unlock();

	TRACE_LEAVE();
	return rc;
}
