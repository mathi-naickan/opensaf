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

  This file contains the initialization and destroy routines for EDA library.
..............................................................................

  FUNCTIONS INCLUDED in this module:

******************************************************************************
*/

#include "eda.h"
#include <pthread.h>
#include "ncsgl_defs.h"
#include "ncssysf_mem.h"

/* global cb handle */
uint32_t gl_eda_hdl = 0;
static uint32_t eda_use_count = 0;

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Lock the mutex protecting agent startup and shutdown.
 *
 * This function locks the agent startup mutex. The mutex is static and requires
 * neither initialisation nor finalisation. After a thread has called
 * eda_agent_startup_mutex_lock(), that same thread must call
 * eda_agent_startup_mutex_unlock() to release the lock. The thread is not
 * allowed to call eda_agent_startup_mutex_lock() again without first calling
 * eda_agent_startup_mutex_unlock() to unlock it.
 */
static void eda_agent_startup_mutex_lock(void)
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
static void eda_agent_startup_mutex_unlock(void)
{
	int result = pthread_mutex_unlock(&s_agent_startup_mutex);
	/* Should never fail. If it does, it indicates a serious fault,
	   e.g. memory corruption or trying to unlock the mutex when it
	   wasn't locked by the calling thread. */
	osafassert(result == 0);
}

/*
 * Enable tracing early - GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
        char *value;
        /* Initialize trace system first of all so we can see what is going. */
        if ((value = getenv("EDA_TRACE_PATH_FILENAME")) != NULL) {
                if (logtrace_init("eda", value, CATEGORY_ALL) != 0) {
                        /* error, we cannot do anything */
                        return;
                }
        }
}

/****************************************************************************
  Name          : ncs_eda_lib_req
 
  Description   : This routine is exported to the external entities & is used
                  to create & destroy the EDA library.
 
  Arguments     : req_info - ptr to the request info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t ncs_eda_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = eda_create(&req_info->info.create);
		break;

	case NCS_LIB_REQ_DESTROY:
		eda_destroy(&req_info->info.destroy);
		break;

	default:
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : eda_create
 
  Description   : This routine creates & initializes the EDA control block.
 
  Arguments     : create_info - ptr to the create info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t eda_create(NCS_LIB_CREATE *create_info)
{
	EDA_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (NULL == create_info) {
		TRACE_LEAVE2("create_info is NULL");
		return NCSCC_RC_FAILURE;
	}

	/* allocate EDA cb */
	if (NULL == (cb = m_MMGR_ALLOC_EDA_CB)) {
		TRACE_4("malloc failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	memset(cb, 0, sizeof(EDA_CB));

	/* assign the EDA pool-id (used by hdl-mngr) */
	cb->pool_id = NCS_HM_POOL_ID_COMMON;

	/* create the association with hdl-mngr */
	if (0 == (cb->cb_hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_EDA, (NCSCONTEXT)cb))) {
		TRACE_4("create handle failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	/* get the process id */
	cb->prc_id = getpid();

	/* initialize the eda cb lock */
	m_NCS_LOCK_INIT(&cb->cb_lock);
	m_NCS_LOCK_INIT(&cb->eds_sync_lock);

	/* Store the cb hdl in the global variable */
	gl_eda_hdl = cb->cb_hdl;

	TRACE_1("global eda library handle is: %u", gl_eda_hdl);

	/* register with MDS */
	if ((NCSCC_RC_SUCCESS != (rc = eda_mds_init(cb)))) {
		TRACE_4("mds init failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	eda_sync_with_eds(cb);
	cb->node_status = SA_CLM_NODE_JOINED;
	TRACE_LEAVE2("Default local node membership status: %u", cb->node_status);
	return rc;

 error:
	if (cb) {
		/* remove the association with hdl-mngr */
		if (cb->cb_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, cb->cb_hdl);

		/* delete the eda init instances */
		eda_hdl_list_del(&cb->eda_init_rec_list);

		/* destroy the lock */
		m_NCS_LOCK_DESTROY(&cb->cb_lock);

		/* free the control block */
		m_MMGR_FREE_EDA_CB(cb);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : eda_destroy
 
  Description   : This routine destroys the EDA control block.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void eda_destroy(NCS_LIB_DESTROY *destroy_info)
{
	EDA_CB *cb = 0;
	TRACE_ENTER();

	/* retrieve EDA CB */
	cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl);
	if (!cb) {
		TRACE_LEAVE2("global take handle failed: %u", gl_eda_hdl);
		return;
	}
	/* delete the hdl db */
	eda_hdl_list_del(&cb->eda_init_rec_list);

	/* unregister with MDS */
	eda_mds_finalize(cb);

	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

	/* return EDA CB */
	ncshm_give_hdl(gl_eda_hdl);

	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, cb->cb_hdl);

	/* free the control block */
	m_MMGR_FREE_EDA_CB(cb);

	/* reset the global cb handle */
	gl_eda_hdl = 0;

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          :  ncs_eda_startup

  Description   :  This routine creates a EDSv agent infrastructure to interface
                   with EDSv service. Once the infrastructure is created from
                   then on use_count is incremented for every startup request.

  Arguments     :  - NIL-

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_eda_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;
	TRACE_ENTER();

	eda_agent_startup_mutex_lock();
	if (eda_use_count > 0) {
		/* Already created, so just increment the use_count */
		eda_use_count++;
		eda_agent_startup_mutex_unlock();
		TRACE_LEAVE2("Library use count: %u", eda_use_count);
		return NCSCC_RC_SUCCESS;
	}

   /*** Init EDA ***/
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	if (ncs_eda_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		eda_agent_startup_mutex_unlock();
		return NCSCC_RC_FAILURE;
	} else {
		eda_use_count = 1;
		TRACE("EDA agent library initialized");
	}

	eda_agent_startup_mutex_unlock();
	TRACE_LEAVE2("Library use count: %u", eda_use_count);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  ncs_eda_shutdown 

  Description   :  This routine destroys the EDSv agent infrastructure created 
                   to interface EDSv service. If the registered users are > 1, 
                   it just decrements the use_count.   

  Arguments     :  - NIL -

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_eda_shutdown(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	eda_agent_startup_mutex_lock();
	if (eda_use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		eda_use_count--;
	} else if (eda_use_count == 1) {
		NCS_LIB_REQ_INFO lib_destroy;

		memset(&lib_destroy, 0, sizeof(lib_destroy));
		lib_destroy.i_op = NCS_LIB_REQ_DESTROY;

		rc = ncs_eda_lib_req(&lib_destroy);

		eda_use_count = 0;
	}

	eda_agent_startup_mutex_unlock();
	TRACE_LEAVE2("Library use count: %u", eda_use_count);
	return rc;
}
