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

  This file contains the initialization and destroy routines for AvA library.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"
#include <pthread.h>
#include "osaf_utility.h"
#include "osaf_poll.h"

/* global cb handle */
uint32_t gl_ava_hdl = 0;
static uint32_t ava_use_count = 0;

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * To enable tracing early in saAmfInitialize, use a GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
	char *value;

	/* Initialize trace system first of all so we can see what is going. */
	if ((value = getenv("AVA_TRACE_PATHNAME")) != NULL) {
		if (logtrace_init("ava", value, CATEGORY_ALL) != 0) {
			/* error, we cannot do anything */
			return;
		}
	}
}

/****************************************************************************
  Name          : ava_lib_req
 
  Description   : This routine is exported to the external entities & is used
                  to create & destroy the AvA library.
 
  Arguments     : req_info - ptr to the request info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t ava_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = ava_create(&req_info->info.create);
		if (NCSCC_RC_SUCCESS == rc) {
			TRACE_1("AVA creation success");
		} else {
			TRACE_4("AVA creation failed");
		}
		break;

	case NCS_LIB_REQ_DESTROY:
		ava_destroy(&req_info->info.destroy);
		if (NCSCC_RC_SUCCESS == rc) {
			TRACE_1("AVA creation success");
		} else {
			TRACE_4("AVA creation failed");
		}
		break;

	default:
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_create
 
  Description   : This routine creates & initializes the AvA control block.
 
  Arguments     : create_info - ptr to the create info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t ava_create(NCS_LIB_CREATE *create_info)
{
	AVA_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR err;
	TRACE_ENTER();

	/* allocate AvA cb */
	if (!(cb = calloc(1, sizeof(AVA_CB)))) {
		LOG_ER("AVA Create: Calloc failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	/* fetch the comp name from the env variable */
	if (getenv("SA_AMF_COMPONENT_NAME")) {
		if (strlen(getenv("SA_AMF_COMPONENT_NAME")) < SA_MAX_NAME_LENGTH) {
			strcpy((char *)cb->comp_name.value, getenv("SA_AMF_COMPONENT_NAME"));
			cb->comp_name.length = (uint16_t)strlen((char *)cb->comp_name.value);
			m_AVA_FLAG_SET(cb, AVA_FLAG_COMP_NAME);
			TRACE("Component name = %s",cb->comp_name.value);
		} else {
			TRACE_2("Length of SA_AMF_COMPONENT_NAME exceeds SA_MAX_NAME_LENGTH bytes");
			rc = NCSCC_RC_FAILURE;
			goto error;
		}
	}

	/* assign the AvA pool-id (used by hdl-mngr) */
	cb->pool_id = NCS_HM_POOL_ID_COMMON;

	/* create the association with hdl-mngr */
	if (!(cb->cb_hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_AVA, (NCSCONTEXT)cb))) {
		LOG_ER("Unable to create handle for control block");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	TRACE("Created handle for the control block");

	/* initialize the AvA cb lock */
	m_NCS_LOCK_INIT(&cb->lock);
	TRACE("Initialized the AVA control block lock");

	/* EDU initialisation */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);
	TRACE("EDU Initialization success");

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_nda_msg, &err);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_4("EDU Compilation failed");
		goto error;
	}

	/* create the sel obj (for mds sync) */
	m_NCS_SEL_OBJ_CREATE(&cb->sel_obj);

	/* initialize the hdl db */
	if (NCSCC_RC_SUCCESS != ava_hdl_init(&cb->hdl_db)) {
		TRACE_4("AVA Handles DB initialization failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}
	TRACE("AVA Handles DB created successfully");

	m_AVA_FLAG_SET(cb, AVA_FLAG_FD_VALID);

	/* register with MDS */
	if ((NCSCC_RC_SUCCESS != ava_mds_reg(cb))) {
		LOG_ER("AVA MDS Registration failed");
		rc = NCSCC_RC_FAILURE;
		goto error;
	}
	TRACE("AVA MDS Registration success");

	TRACE_1("Waiting on select till AMF Node Director is up");
	/* block until mds detects avnd */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(cb->sel_obj), 3000);

	/* reset the fd validity flag  */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
	m_AVA_FLAG_RESET(cb, AVA_FLAG_FD_VALID);
	m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);

	/* This sel obj is no more used */
	m_NCS_SEL_OBJ_DESTROY(cb->sel_obj);

	/* everything went off well.. store the cb hdl in the global variable */
	gl_ava_hdl = cb->cb_hdl;

	TRACE_LEAVE();
	return rc;

 error:
	if (cb) {
		/* remove the association with hdl-mngr */
		if (cb->cb_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_AVA, cb->cb_hdl);

		/* delete the hdl db */
		ava_hdl_del(cb);

		/* Flush the edu hdl */
		m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);

		/* destroy the lock */
		m_NCS_LOCK_DESTROY(&cb->lock);

		/* free the control block */
		free(cb);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_destroy
 
  Description   : This routine destroys the AvA control block.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void ava_destroy(NCS_LIB_DESTROY *destroy_info)
{
	AVA_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* retrieve AvA CB */
	cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl);
	if (!cb) {
		LOG_ER("Unable to take handle for control block");
		goto done;
	}

	/* delete the hdl db */
	ava_hdl_del(cb);
	TRACE_1("Deleted the handles DB");

	/* unregister with MDS */
	rc = ava_mds_unreg(cb);
	if (NCSCC_RC_SUCCESS != rc)
		TRACE_4("MDS unregistration failed");
	else
		TRACE_1("MDS unregistration success");

	/* EDU cleanup */
	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);
	TRACE_1("EDU cleanup failed");

	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->lock);
	TRACE_1("Destroying lock for control block failed");

	/* return AvA CB */
	ncshm_give_hdl(gl_ava_hdl);

	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_AVA, cb->cb_hdl);
	TRACE_1("Removing association with handle manager failed");

	/* free the control block */
	free(cb);

	/* reset the global cb handle */
	gl_ava_hdl = 0;

 done:
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          :  ncs_ava_startup

  Description   :  This routine creates a AVSv agent infrastructure to interface
                   with AVSv service. Once the infrastructure is created from
                   then on use_count is incremented for every startup request.

  Arguments     :  - NIL-

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_ava_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;
	TRACE_ENTER();

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (ava_use_count > 0) {
		/* Already created, so just increment the use_count */
		ava_use_count++;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		TRACE_LEAVE2("AVA use count = %d",ava_use_count);
		return NCSCC_RC_SUCCESS;
	}

   /*** Init AVA ***/
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	if (ava_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		TRACE_LEAVE2("AVA lib create failed");
		return NCSCC_RC_FAILURE;
	} else {
		ava_use_count = 1;
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	TRACE_LEAVE2("AVA Use count = %u",ava_use_count);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  ncs_ava_shutdown 

  Description   :  This routine destroys the AVSv agent infrastructure created 
                   to interface AVSv service. If the registered users are > 1, 
                   it just decrements the use_count.   

  Arguments     :  - NIL -

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int ncs_ava_shutdown(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (ava_use_count > 1) {
		/* Still users exists, so just decrement the use_count */
		ava_use_count--;
		TRACE("Library users still exist: AVA use count = %d",ava_use_count);
	} else if (ava_use_count == 1) {
		NCS_LIB_REQ_INFO lib_destroy;

		memset(&lib_destroy, 0, sizeof(lib_destroy));
		lib_destroy.i_op = NCS_LIB_REQ_DESTROY;
		rc = ava_lib_req(&lib_destroy);
		ava_use_count = 0;
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
	TRACE_LEAVE2("AVA use count = %d", ava_use_count);
	return rc;
}
