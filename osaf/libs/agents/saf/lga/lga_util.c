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
 * Author(s): Ericsson AB
 *
 */

#include <stdlib.h>
#include <syslog.h>
#include "lga.h"

/* Variables used during startup/shutdown only */
static pthread_mutex_t lga_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int lga_use_count;

/**
 * 
 * 
 * @return unsigned int
 */
static unsigned int lga_create(void)
{
	unsigned int timeout = 3000;
	NCS_SEL_OBJ_SET set;
	unsigned int rc = NCSCC_RC_SUCCESS;

	/* create and init sel obj for mds sync */
	m_NCS_SEL_OBJ_CREATE(&lga_cb.lgs_sync_sel);
	m_NCS_SEL_OBJ_ZERO(&set);
	m_NCS_SEL_OBJ_SET(lga_cb.lgs_sync_sel, &set);
	lga_cb.lgs_sync_awaited = 1;

	/* register with MDS */
	if ((NCSCC_RC_SUCCESS != (rc = lga_mds_init(&lga_cb)))) {
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	/* Block and wait for indication from MDS meaning LGS is up */
	m_NCS_SEL_OBJ_SELECT(lga_cb.lgs_sync_sel, &set, 0, 0, &timeout);

	pthread_mutex_lock(&lga_cb.cb_lock);
	lga_cb.lgs_sync_awaited = 0;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	/* No longer needed */
	m_NCS_SEL_OBJ_DESTROY(lga_cb.lgs_sync_sel);

	return rc;

 error:
	/* delete the lga init instances */
	lga_hdl_list_del(&lga_cb.client_list);

	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
static void lga_destroy(void)
{
	TRACE_ENTER();

	/* delete the hdl db */
	lga_hdl_list_del(&lga_cb.client_list);

	/* unregister with MDS */
	lga_mds_finalize(&lga_cb);
}

/****************************************************************************
 * Name          : lga_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
static bool lga_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	lgsv_msg_t *cbk, *pnext;

	pnext = cbk = (lgsv_msg_t *)msg;
	while (pnext) {
		pnext = cbk->next;
		lga_msg_destroy(cbk);
		cbk = pnext;
	}
	return true;
}

/****************************************************************************
  Name          : lga_log_stream_hdl_rec_list_del
 
  Description   : This routine deletes a list of log stream records.
 
  Arguments     : pointer to the list of log stream records anchor.
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void lga_log_stream_hdl_rec_list_del(lga_log_stream_hdl_rec_t **plstr_hdl)
{
	lga_log_stream_hdl_rec_t *lstr_hdl;
	while ((lstr_hdl = *plstr_hdl) != NULL) {
		*plstr_hdl = lstr_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, lstr_hdl->log_stream_hdl);
		free(lstr_hdl);
		lstr_hdl = NULL;
	}
}

/****************************************************************************
  Name          : lga_hdl_cbk_rec_prc
 
  Description   : This routine invokes the registered callback routine & frees
                  the callback record resources.
 
  Arguments     : cb      - ptr to the LGA control block
                  msg     - ptr to the callback message
                  reg_cbk - ptr to the registered callbacks
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void lga_hdl_cbk_rec_prc(lga_cb_t *cb, lgsv_msg_t *msg, SaLogCallbacksT *reg_cbk)
{
	lgsv_cbk_info_t *cbk_info = &msg->info.cbk_info;

	/* invoke the corresponding callback */
	switch (cbk_info->type) {
	case LGSV_WRITE_LOG_CALLBACK_IND:
		{

			if (reg_cbk->saLogWriteLogCallback)
				reg_cbk->saLogWriteLogCallback(cbk_info->inv, cbk_info->write_cbk.error);
		}
		break;
	default:
		TRACE("unknown callback type: %d", cbk_info->type);
		break;
	}
}

/****************************************************************************
  Name          : lga_hdl_cbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the LGA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static SaAisErrorT lga_hdl_cbk_dispatch_one(lga_cb_t *cb, lga_client_hdl_rec_t *hdl_rec)
{
	lgsv_msg_t *cbk_msg;
	SaAisErrorT rc = SA_AIS_OK;

	/* Nonblk receive to obtain the message from priority queue */
	while (NULL != (cbk_msg = (lgsv_msg_t *)
			m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
		if (cbk_msg->info.cbk_info.type == LGSV_WRITE_LOG_CALLBACK_IND) {
			lga_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
			lga_msg_destroy(cbk_msg);
			break;
		} else {
			TRACE("Unsupported callback type = %d", cbk_msg->info.cbk_info.type);
			rc = SA_AIS_ERR_LIBRARY;
			lga_msg_destroy(cbk_msg);
		}
	}

	return rc;
}

/****************************************************************************
  Name          : lga_hdl_cbk_dispatch_all
 
  Description   : This routine dispatches all the pending callbacks.
 
  Arguments     : cb      - ptr to the LGA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t lga_hdl_cbk_dispatch_all(lga_cb_t *cb, lga_client_hdl_rec_t *hdl_rec)
{
	lgsv_msg_t *cbk_msg;
	uint32_t rc = SA_AIS_OK;

	/* Recv all the cbk notifications from the queue & process them */
	do {
		if (NULL == (cbk_msg = (lgsv_msg_t *)m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg)))
			break;
		if (cbk_msg->info.cbk_info.type == LGSV_WRITE_LOG_CALLBACK_IND) {
			TRACE_2("LGSV_LGS_DELIVER_EVENT");
			lga_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
		} else {
			TRACE("unsupported callback type %d", cbk_msg->info.cbk_info.type);
		}
		/* now that we are done with this rec, free the resources */
		lga_msg_destroy(cbk_msg);
	} while (1);

	return rc;
}

/****************************************************************************
  Name          : lga_hdl_cbk_dispatch_block
 
  Description   : This routine blocks forever for receiving indications from 
                  LGS. The routine returns when saEvtFinalize is executed on 
                  the handle.
 
  Arguments     : cb      - ptr to the LGA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t lga_hdl_cbk_dispatch_block(lga_cb_t *cb, lga_client_hdl_rec_t *hdl_rec)
{
	lgsv_msg_t *cbk_msg;
	uint32_t rc = SA_AIS_OK;

	for (;;) {
		if (NULL != (cbk_msg = (lgsv_msg_t *)
			     m_NCS_IPC_RECEIVE(&hdl_rec->mbx, cbk_msg))) {

			if (cbk_msg->info.cbk_info.type == LGSV_WRITE_LOG_CALLBACK_IND) {
				TRACE_2("LGSV_LGS_DELIVER_EVENT");
				lga_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
			} else {
				TRACE("unsupported callback type %d", cbk_msg->info.cbk_info.type);
			}
			lga_msg_destroy(cbk_msg);
		} else
			return rc;	/* FIX to handle finalize clean up of mbx */
	}
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int lga_startup(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;
	pthread_mutex_lock(&lga_lock);

	TRACE_ENTER2("lga_use_count: %u", lga_use_count);
	if (lga_use_count > 0) {
		/* Already created, just increment the use_count */
		lga_use_count++;
		goto done;
	} else {
		if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
			TRACE("ncs_agents_startup FAILED");
			goto done;
		}

		if ((rc = lga_create()) != NCSCC_RC_SUCCESS) {
			ncs_agents_shutdown();
			goto done;
		} else
			lga_use_count = 1;
	}

 done:
	pthread_mutex_unlock(&lga_lock);

	TRACE_LEAVE2("rc: %u, lga_use_count: %u", rc, lga_use_count);
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int lga_shutdown(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("lga_use_count: %u", lga_use_count);
	pthread_mutex_lock(&lga_lock);

	if (lga_use_count > 1) {
		/* Users still exist, just decrement the use count */
		lga_use_count--;
	} else if (lga_use_count == 1) {
		lga_destroy();
		rc = ncs_agents_shutdown();
		lga_use_count = 0;
	}

	pthread_mutex_unlock(&lga_lock);

	TRACE_LEAVE2("rc: %u, lga_use_count: %u", rc, lga_use_count);
	return rc;
}

/****************************************************************************
 * Name          : lga_msg_destroy
 *
 * Description   : This is the function which is called to destroy an LGSV msg.
 *
 * Arguments     : LGSV_MSG *.
 *
 * Return Values : NONE
 *
 * Notes         : None.
 *****************************************************************************/
void lga_msg_destroy(lgsv_msg_t *msg)
{
	if (LGSV_LGA_API_MSG == msg->type) {
		TRACE("LGSV_LGA_API_MSG");
	}

  /** There are no other pointers 
   ** off the evt, so free the evt
   **/
	free(msg);
}

/****************************************************************************
  Name          : lga_find_hdl_rec_by_regid
 
  Description   : This routine looks up a lga_client_hdl_rec by client_id
 
  Arguments     : cb      - ptr to the LGA control block
                  client_id  - cluster wide unique allocated by LGS

  Return Values : LGA_CLIENT_HDL_REC * or NULL
 
  Notes         : None
******************************************************************************/
lga_client_hdl_rec_t *lga_find_hdl_rec_by_regid(lga_cb_t *lga_cb, uint32_t client_id)
{
	lga_client_hdl_rec_t *lga_hdl_rec;

	for (lga_hdl_rec = lga_cb->client_list; lga_hdl_rec != NULL; lga_hdl_rec = lga_hdl_rec->next) {
		if (lga_hdl_rec->lgs_client_id == client_id)
			return lga_hdl_rec;
	}

	return NULL;
}

/****************************************************************************
  Name          : lga_hdl_list_del
 
  Description   : This routine deletes all handles for this library.
 
  Arguments     : cb  - ptr to the LGA control block
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void lga_hdl_list_del(lga_client_hdl_rec_t **p_client_hdl)
{
	lga_client_hdl_rec_t *client_hdl;

	while ((client_hdl = *p_client_hdl) != NULL) {
		*p_client_hdl = client_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, client_hdl->local_hdl);
	/** clean up the channel records for this lga-client
         **/
		lga_log_stream_hdl_rec_list_del(&client_hdl->stream_list);
	/** remove the association with hdl-mngr 
         **/
		free(client_hdl);
		client_hdl = 0;
	}
}

/****************************************************************************
  Name          : lga_log_stream_hdl_rec_del
 
  Description   : This routine deletes the a log stream handle record from
                  a list of log stream hdl records. 
 
  Arguments     : LGA_LOG_STREAM_HDL_REC **list_head
                  LGA_LOG_STREAM_HDL_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t lga_log_stream_hdl_rec_del(lga_log_stream_hdl_rec_t **list_head, lga_log_stream_hdl_rec_t *rm_node)
{
	/* Find the channel hdl record in the list of records */
	lga_log_stream_hdl_rec_t *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
	/** remove the association with hdl-mngr 
         **/
		ncshm_give_hdl(rm_node->log_stream_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, rm_node->log_stream_hdl);
		free(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
		/** remove the association with hdl-mngr 
                 **/
				ncshm_give_hdl(rm_node->log_stream_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, rm_node->log_stream_hdl);
				free(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
    /** The node couldn't be deleted **/
	TRACE("The node couldn't be deleted");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : lga_hdl_rec_del
 
  Description   : This routine deletes the a client handle record from
                  a list of client hdl records. 
 
  Arguments     : LGA_CLIENT_HDL_REC **list_head
                  LGA_CLIENT_HDL_REC *rm_node
 
  Return Values : None
 
  Notes         : The selection object is destroyed after all the means to 
                  access the handle record (ie. hdl db tree or hdl mngr) is 
                  removed. This is to disallow the waiting thread to access 
                  the hdl rec while other thread executes saAmfFinalize on it.
******************************************************************************/
uint32_t lga_hdl_rec_del(lga_client_hdl_rec_t **list_head, lga_client_hdl_rec_t *rm_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	lga_client_hdl_rec_t *list_iter = *list_head;

	TRACE_ENTER();

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;

	/** detach & release the IPC 
         **/
		m_NCS_IPC_DETACH(&rm_node->mbx, lga_clear_mbx, NULL);
		m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

		ncshm_give_hdl(rm_node->local_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, rm_node->local_hdl);
	/** Free the channel records off this hdl 
         **/
		lga_log_stream_hdl_rec_list_del(&rm_node->stream_list);

	/** free the hdl rec 
         **/
		free(rm_node);
		rc = NCSCC_RC_SUCCESS;
		goto out;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;

		/** detach & release the IPC */
				m_NCS_IPC_DETACH(&rm_node->mbx, lga_clear_mbx, NULL);
				m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

				ncshm_give_hdl(rm_node->local_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, rm_node->local_hdl);
		/** Free the channel records off this lga_hdl  */
				lga_log_stream_hdl_rec_list_del(&rm_node->stream_list);

		/** free the hdl rec */
				free(rm_node);

				rc = NCSCC_RC_SUCCESS;
				goto out;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
	TRACE("failed");

 out:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : lga_log_stream_hdl_rec_add
 
  Description   : This routine adds the logstream handle record to the list
                  of handles in the client hdl record.
 
  Arguments     : LGA_CLIENT_HDL_REC *hdl_rec
                  uint32_t               lstr_id
                  uint32_t               lstr_open_id
                  uint32_t               log_stream_open_flags
                  SaNameT             *logStreamName
 
  Return Values : ptr to the channel handle record
 
  Notes         : None
******************************************************************************/
lga_log_stream_hdl_rec_t *lga_log_stream_hdl_rec_add(lga_client_hdl_rec_t **hdl_rec,
						     uint32_t lstr_id,
						     uint32_t log_stream_open_flags,
						     const SaNameT *logStreamName, uint32_t log_header_type)
{
	lga_log_stream_hdl_rec_t *rec = calloc(1, sizeof(lga_log_stream_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		return NULL;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->log_stream_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_LGA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		free(rec);
		return NULL;
	}

    /** Initialize the known channel attributes at this point
     **/
	rec->lgs_log_stream_id = lstr_id;
	rec->open_flags = log_stream_open_flags;
	rec->log_stream_name.length = logStreamName->length;
	memcpy((void *)rec->log_stream_name.value, (void *)logStreamName->value, logStreamName->length);
	rec->log_header_type = log_header_type;
    /** Initialize the parent handle **/
	rec->parent_hdl = *hdl_rec;

    /** Insert this record into the list of channel hdl records
     **/
	rec->next = (*hdl_rec)->stream_list;
	(*hdl_rec)->stream_list = rec;

    /** Everything appears fine, so return the 
     ** steam hdl.
     **/
	return rec;
}

/****************************************************************************
  Name          : lga_hdl_rec_add
 
  Description   : This routine adds the handle record to the lga cb.
 
  Arguments     : cb       - ptr tot he LGA control block
                  reg_cbks - ptr to the set of registered callbacks
                  client_id   - obtained from LGS.
 
  Return Values : ptr to the lga handle record
 
  Notes         : None
******************************************************************************/
lga_client_hdl_rec_t *lga_hdl_rec_add(lga_cb_t *cb, const SaLogCallbacksT *reg_cbks, uint32_t client_id)
{
	lga_client_hdl_rec_t *rec = calloc(1, sizeof(lga_client_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		goto out;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->local_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_LGA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		goto err_free;
	}

	/* store the registered callbacks */
	if (reg_cbks != NULL)
		memcpy((void *)&rec->reg_cbk, (void *)reg_cbks, sizeof(SaLogCallbacksT));

    /** Associate with the client_id obtained from LGS
     **/
	rec->lgs_client_id = client_id;

    /** Initialize and attach the IPC/Priority queue
     **/
	if (m_NCS_IPC_CREATE(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_IPC_CREATE failed");
		goto err_destroy_hdl;
	}

	if (m_NCS_IPC_ATTACH(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_IPC_ATTACH failed");
		goto err_ipc_release;
	}

    /** Add this to the Link List of 
     ** CLIENT_HDL_RECORDS for this LGA_CB 
     **/

	pthread_mutex_lock(&cb->cb_lock);
	/* add this to the start of the list */
	rec->next = cb->client_list;
	cb->client_list = rec;
	pthread_mutex_unlock(&cb->cb_lock);

	goto out;

 err_ipc_release:
	(void)m_NCS_IPC_RELEASE(&rec->mbx, NULL);

 err_destroy_hdl:
	(void)ncshm_destroy_hdl(NCS_SERVICE_ID_LGA, rec->local_hdl);

 err_free:
	free(rec);
	rec = NULL;

 out:
	return rec;
}

/****************************************************************************
  Name          : lga_hdl_cbk_dispatch
 
  Description   : This routine dispatches the pending callbacks as per the 
                  dispatch flags.
 
  Arguments     : cb      - ptr to the LGA control block
                  hdl_rec - ptr to the handle record
                  flags   - dispatch flags
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
SaAisErrorT lga_hdl_cbk_dispatch(lga_cb_t *cb, lga_client_hdl_rec_t *hdl_rec, SaDispatchFlagsT flags)
{
	SaAisErrorT rc;

	switch (flags) {
	case SA_DISPATCH_ONE:
		rc = lga_hdl_cbk_dispatch_one(cb, hdl_rec);
		break;

	case SA_DISPATCH_ALL:
		rc = lga_hdl_cbk_dispatch_all(cb, hdl_rec);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = lga_hdl_cbk_dispatch_block(cb, hdl_rec);
		break;

	default:
		TRACE("dispatch flag not valid");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

	return rc;
}

/*
 * To enable tracing early in saLogInitialize, use a GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
	char *value;

	/* Initialize trace system first of all so we can see what is going. */
	if ((value = getenv("LOGSV_TRACE_PATHNAME")) != NULL) {
		if (logtrace_init("lga", value, CATEGORY_ALL) != 0) {
			/* error, we cannot do anything */
			return;
		}
	}
}
