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

#include <stdlib.h>
#include <syslog.h>
#include "clma.h"

static void clma_hdl_list_del(clma_client_hdl_rec_t ** list);
/* Variables used during startup/shutdown only */
static pthread_mutex_t clma_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int clma_use_count;
static void clma_destroy(void);

/**
 * 
 * 
 * @return unsigned int
 */
uint32_t clma_validate_version(SaVersionT *version)
{
	TRACE_ENTER();
	if (version->releaseCode == 'B' && version->majorVersion == 1 && version->minorVersion == 1) {
		TRACE_LEAVE();
		return 1;
	}
	if (version->releaseCode == 'B' && version->majorVersion == 4 && version->minorVersion == 1) {
		TRACE_LEAVE();
		return 0;
	}

	return 0;
}

/**
 * 
 * 
 * @return unsigned int
 */
static unsigned int clma_create(void)
{
	unsigned int timeout = 3000;
	NCS_SEL_OBJ_SET set;
	unsigned int rc = NCSCC_RC_SUCCESS;

	/* create and init sel obj for mds sync */
	m_NCS_SEL_OBJ_CREATE(&clma_cb.clms_sync_sel);
	m_NCS_SEL_OBJ_ZERO(&set);
	m_NCS_SEL_OBJ_SET(clma_cb.clms_sync_sel, &set);
	pthread_mutex_lock(&clma_cb.cb_lock);
	clma_cb.clms_sync_awaited = 1;
	pthread_mutex_unlock(&clma_cb.cb_lock);

	/* register with MDS */
	if ((NCSCC_RC_SUCCESS != (rc = clma_mds_init(&clma_cb)))) {	/* need to do */
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	/* Block and wait for indication from MDS meaning CMLS is up */
	m_NCS_SEL_OBJ_SELECT(clma_cb.clms_sync_sel, &set, 0, 0, &timeout);

	pthread_mutex_lock(&clma_cb.cb_lock);
	clma_cb.clms_sync_awaited = 0;
	pthread_mutex_unlock(&clma_cb.cb_lock);

	/* No longer needed */
	m_NCS_SEL_OBJ_DESTROY(clma_cb.clms_sync_sel);

	return rc;

 error:
	/* delete the clma init instances */	/*need to do */
	clma_hdl_list_del(&clma_cb.client_list);

	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int clma_startup(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;
	pthread_mutex_lock(&clma_lock);

	TRACE_ENTER2("clma_use_count: %u", clma_use_count);
	if (clma_use_count > 0) {
		/* Already created, just increment the use_count */
		clma_use_count++;
		goto done;
	} else {
		if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
			TRACE("ncs_agents_startup FAILED");
			goto done;
		}

		if ((rc = clma_create()) != NCSCC_RC_SUCCESS) {
			ncs_agents_shutdown();
			goto done;
		} else
			clma_use_count = 1;
	}

 done:
	pthread_mutex_unlock(&clma_lock);
	TRACE_LEAVE2("rc: %u, clma_use_count: %u", rc, clma_use_count);
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int clma_shutdown(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("clma_use_count: %u", clma_use_count);
	pthread_mutex_lock(&clma_lock);

	if (clma_use_count > 1) {
		/* Users still exist, just decrement the use count */
		clma_use_count--;
	} else if (clma_use_count == 1) {
		clma_destroy();	/*need to do */
		rc = ncs_agents_shutdown();
		clma_use_count = 0;
	}

	pthread_mutex_unlock(&clma_lock);
	TRACE_LEAVE2("rc: %u, clma_use_count: %u", rc, clma_use_count);
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
static void clma_destroy(void)
{
	/* delete the hdl db */
	clma_hdl_list_del(&clma_cb.client_list);

	/* unregister with MDS */
	clma_mds_finalize(&clma_cb);
}

/****************************************************************************
  Name          : clma_find_hdl_rec_by_client_id
 
  Description   : This routine looks up a clma_client_hdl_rec by client_id
 
  Arguments     : cb      - ptr to the CLMA control block
                  client_id  - cluster wide unique allocated by CLMS

  Return Values : CLMA_CLIENT_HDL_REC * or NULL
 
  Notes         : None
******************************************************************************/
clma_client_hdl_rec_t *clma_find_hdl_rec_by_client_id(clma_cb_t * clma_cb, uint32_t client_id)
{
	clma_client_hdl_rec_t *clma_hdl_rec;

	for (clma_hdl_rec = clma_cb->client_list; clma_hdl_rec != NULL; clma_hdl_rec = clma_hdl_rec->next) {
		if (clma_hdl_rec->clms_client_id == client_id)
			return clma_hdl_rec;
	}

	return NULL;
}

/****************************************************************************
 * Name          : clma_clear_mbx
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
static bool clma_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	CLMSV_MSG *cbk, *pnext;

	pnext = cbk = (CLMSV_MSG *) msg;
	while (pnext) {
		pnext = cbk->next;
		clma_msg_destroy(cbk);	/*need to do */
		cbk = pnext;
	}
	return true;
}

/****************************************************************************
  Name          : clma_hdl_rec_add
 
  Description   : This routine adds the handle record to the clma cb.
 
  Arguments     : cb       - ptr tot he CLMA control block
                  reg_cbks - ptr to the set of registered callbacks
                  client_id   - obtained from CLMS.
         
  Return Values : ptr to the clma handle record
 
  Notes         : None
******************************************************************************/
clma_client_hdl_rec_t *clma_hdl_rec_add(clma_cb_t * cb, const SaClmCallbacksT *reg_cbks_1,
					const SaClmCallbacksT_4 * reg_cbks_4, SaVersionT *version, uint32_t client_id)
{
	clma_client_hdl_rec_t *rec = calloc(1, sizeof(clma_client_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		goto done;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->local_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_CLMA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		goto err_free;
	}

	/* store the registered callbacks */
	if (clma_validate_version(version)) {
		if (reg_cbks_1)
			memcpy((void *)&rec->cbk_param.reg_cbk, (void *)reg_cbks_1, sizeof(SaClmCallbacksT));
	} else {
		if (reg_cbks_4)
			memcpy((void *)&rec->cbk_param.reg_cbk_4, (void *)reg_cbks_4, sizeof(SaClmCallbacksT_4));
	}

	/* Store verison in clma_client_hdl_rec_t */
	rec->version = calloc(1, sizeof(SaVersionT));
	if (rec->version == NULL) {
		TRACE("calloc failed");
		goto err_destroy_hdl;
	}

	rec->version->releaseCode = version->releaseCode;
	rec->version->majorVersion = version->majorVersion;
	rec->version->minorVersion = version->minorVersion;

	/** Associate with the client_id obtained from CLMS
     	**/
	rec->clms_client_id = client_id;
	rec->is_member = SA_TRUE;	
	rec->is_configured = SA_TRUE;

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
     	** CLIENT_HDL_RECORDS for this CLMA_CB 
     	**/

	pthread_mutex_lock(&cb->cb_lock);
	/* add this to the start of the list */
	rec->next = cb->client_list;
	cb->client_list = rec;
	pthread_mutex_unlock(&cb->cb_lock);

	goto done;

 err_ipc_release:
	(void)m_NCS_IPC_RELEASE(&rec->mbx, NULL);

 err_destroy_hdl:
	ncshm_destroy_hdl(NCS_SERVICE_ID_CLMA, rec->local_hdl);

 err_free:
	free(rec);
	rec = NULL;

 done:
	return rec;
}

/****************************************************************************
  Name          : clma_hdl_rec_del
 
  Description   : This routine deletes the a client handle record from
                  a list of client hdl records. 
 
  Arguments     : clma_client_hdl_rec_t **list_head
                  clma_client_hdl_rec_t *rm_node
 
  Return Values : None
 
  Notes         : The selection object is destroyed after all the means to 
                  access the handle record (ie. hdl db tree or hdl mngr) is 
                  removed. This is to disallow the waiting thread to access 
                  the hdl rec while other thread executes saAmfFinalize on it.
******************************************************************************/
uint32_t clma_hdl_rec_del(clma_client_hdl_rec_t ** list_head, clma_client_hdl_rec_t * rm_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	clma_client_hdl_rec_t *list_iter = *list_head;

	TRACE_ENTER();
	/* TODO: free all resources allocated by the client */

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;

	/** detach & release the IPC 
         **/
		m_NCS_IPC_DETACH(&rm_node->mbx, clma_clear_mbx, NULL);
		m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

		ncshm_give_hdl(rm_node->local_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_CLMA, rm_node->local_hdl);
	/** free the hdl rec 
         **/
		free(rm_node->version);
		free(rm_node);
		rc = NCSCC_RC_SUCCESS;
		goto out;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;

		/** detach & release the IPC */
				m_NCS_IPC_DETACH(&rm_node->mbx, clma_clear_mbx, NULL);
				m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

				ncshm_give_hdl(rm_node->local_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_CLMA, rm_node->local_hdl);

		/** free the hdl rec */
				free(rm_node->version);
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
  Name          : clma_hdl_cbk_rec_prc
 
  Description   : This routine invokes the registered callback routine & frees
                  the callback record resources.
 
  Arguments     : cb      - ptr to the CLMA control block
                  msg     - ptr to the callback message
                  reg_cbk - ptr to the registered callbacks
 
  Return Values : Error code
 
  Notes         : None
******************************************************************************/
static SaAisErrorT clma_hdl_cbk_rec_prc(clma_cb_t * cb, CLMSV_MSG * msg, clma_client_hdl_rec_t * hdl_rec)
{
	SaClmCallbacksT *reg_cbk = NULL;
	SaClmCallbacksT_4 *reg_cbk_4 = NULL;
	CLMSV_CBK_INFO *cbk_info = &msg->info.cbk_info;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	if (clma_validate_version(hdl_rec->version))
		reg_cbk = &hdl_rec->cbk_param.reg_cbk;
	else
		reg_cbk_4 = &hdl_rec->cbk_param.reg_cbk_4;

	TRACE_ENTER2("callback type: %d", cbk_info->type);
	/* invoke the corresponding callback */
	switch (cbk_info->type) {
	case CLMSV_TRACK_CBK:
		{
			if (clma_validate_version(hdl_rec->version)) {
				if (reg_cbk->saClmClusterTrackCallback) {
					SaClmClusterNotificationBufferT *buf = (SaClmClusterNotificationBufferT *)
					    malloc(sizeof(SaClmClusterNotificationBufferT));
					buf->viewNumber = cbk_info->param.track.buf_info.viewNumber;
					buf->numberOfItems = cbk_info->param.track.buf_info.numberOfItems;
					buf->notification =
					    (SaClmClusterNotificationT *)malloc(sizeof(SaClmClusterNotificationT) *
										buf->numberOfItems);
					clma_fill_clusterbuf_from_buf_4(buf, &cbk_info->param.track.buf_info);
					reg_cbk->saClmClusterTrackCallback(buf,
									   cbk_info->param.track.mem_num,
									   cbk_info->param.track.err);
					if(buf->numberOfItems)
						free(buf->notification);
					free(buf);
				}
			} else {
				if (reg_cbk_4->saClmClusterTrackCallback) {
					reg_cbk_4->saClmClusterTrackCallback(&cbk_info->param.track.buf_info,
									     cbk_info->param.track.mem_num,
									     cbk_info->param.track.inv,
									     cbk_info->param.track.root_cause_ent,
									     cbk_info->param.track.cor_ids,
									     cbk_info->param.track.step,
									     cbk_info->param.track.time_super,
									     cbk_info->param.track.err);
				}
			}
		}
		rc = SA_AIS_OK;
		break;
	case CLMSV_NODE_ASYNC_GET_CBK:
		{
			if (clma_validate_version(hdl_rec->version)) {
				if (reg_cbk->saClmClusterNodeGetCallback) {
					SaClmClusterNodeT *node =
					    (SaClmClusterNodeT *)malloc(sizeof(SaClmClusterNodeT));
					clma_fill_node_from_node4(node, cbk_info->param.node_get.info);
					reg_cbk->saClmClusterNodeGetCallback(cbk_info->param.node_get.inv,
									     node, cbk_info->param.node_get.err);
					free(node);
				}
			} else {
				if (reg_cbk_4->saClmClusterNodeGetCallback) {
					reg_cbk_4->saClmClusterNodeGetCallback(cbk_info->param.node_get.inv,
									       &cbk_info->param.node_get.info,
									       cbk_info->param.node_get.err);
				}
			}
		}
		rc = SA_AIS_OK;
		break;
	default:
		TRACE("unknown callback type: %d", cbk_info->type);
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_hdl_cbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the CLMA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static SaAisErrorT clma_hdl_cbk_dispatch_one(clma_cb_t * cb, clma_client_hdl_rec_t * hdl_rec)
{
	CLMSV_MSG *cbk_msg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	/* Nonblk receive to obtain the message from priority queue */
	while (NULL != (cbk_msg = (CLMSV_MSG *)
			m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
		TRACE_1("In While loop type = %d", (int)cbk_msg->info.cbk_info.type);
		if (cbk_msg->info.cbk_info.type == CLMSV_TRACK_CBK ||
		    cbk_msg->info.cbk_info.type == CLMSV_NODE_ASYNC_GET_CBK) {
			rc = clma_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);	/*need to do */
			clma_msg_destroy(cbk_msg);	/* need to do */
			break;
		} else {
			TRACE("Unsupported callback type = %d", cbk_msg->info.cbk_info.type);
			rc = SA_AIS_ERR_LIBRARY;
		}
		clma_msg_destroy(cbk_msg);	/* need to do */
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_hdl_cbk_dispatch_all
 
  Description   : This routine dispatches all the pending callbacks.
 
  Arguments     : cb      - ptr to the CLMA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t clma_hdl_cbk_dispatch_all(clma_cb_t * cb, clma_client_hdl_rec_t * hdl_rec)
{
	CLMSV_MSG *cbk_msg = NULL;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	/* Recv all the cbk notifications from the queue & process them */
	do {
		if (NULL == (cbk_msg = (CLMSV_MSG *) m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg)))
			break;
		if (cbk_msg->info.cbk_info.type == CLMSV_TRACK_CBK ||
		    cbk_msg->info.cbk_info.type == CLMSV_NODE_ASYNC_GET_CBK) {
			TRACE_2("CLMSV_CLMS_DELIVER_EVENT");
			TRACE_2("cbk_msg->evt_type = %d", cbk_msg->evt_type);
			TRACE_2("cbk_msg->info.cbk_info.type = %d", cbk_msg->info.cbk_info.type);
			rc = clma_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);
		} else {
			TRACE("unsupported callback type %d", cbk_msg->info.cbk_info.type);
		}
		/* now that we are done with this rec, free the resources */
		clma_msg_destroy(cbk_msg);
	} while (1);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_hdl_cbk_dispatch_block
 
  Description   : This routine blocks forever for receiving indications from 
                  CLMS. The routine returns when saEvtFinalize is executed on 
                  the handle.
 
  Arguments     : cb      - ptr to the CLMA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t clma_hdl_cbk_dispatch_block(clma_cb_t * cb, clma_client_hdl_rec_t * hdl_rec)
{
	CLMSV_MSG *cbk_msg;
	uint32_t rc = SA_AIS_OK;

	for (;;) {
		if (NULL != (cbk_msg = (CLMSV_MSG *)
			     m_NCS_IPC_RECEIVE(&hdl_rec->mbx, cbk_msg))) {

			if (cbk_msg->info.cbk_info.type == CLMSV_TRACK_CBK ||
			    cbk_msg->info.cbk_info.type == CLMSV_NODE_ASYNC_GET_CBK) {
				TRACE_2("CLMSV_NODE_ASYNC_GET_CBK");
				rc = clma_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);
			} else {
				TRACE("unsupported callback type %d", cbk_msg->info.cbk_info.type);
			}
			/* now that we are done with this rec, free the resources */
			clma_msg_destroy(cbk_msg);
		} else
			return rc;	/* FIX to handle finalize clean up of mbx */
	}
	return rc;
}

/****************************************************************************
  Name          : clma_hdl_cbk_dispatch
 
  Description   : This routine dispatches the pending callbacks as per the 
                  dispatch flags.
 
  Arguments     : cb      - ptr to the CLMA control block
                  hdl_rec - ptr to the handle record
                  flags   - dispatch flags
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
SaAisErrorT clma_hdl_cbk_dispatch(clma_cb_t * cb, clma_client_hdl_rec_t * hdl_rec, SaDispatchFlagsT flags)
{
	SaAisErrorT rc;
	TRACE_ENTER();
	switch (flags) {
	case SA_DISPATCH_ONE:
		rc = clma_hdl_cbk_dispatch_one(cb, hdl_rec);	/*need to do */
		break;

	case SA_DISPATCH_ALL:
		rc = clma_hdl_cbk_dispatch_all(cb, hdl_rec);	/* need to do */
		break;

	case SA_DISPATCH_BLOCKING:
		rc = clma_hdl_cbk_dispatch_block(cb, hdl_rec);	/* need to do */
		break;

	default:
		TRACE("dispatch flag not valid");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

	TRACE_LEAVE();
	return rc;
}

void clma_add_to_async_cbk_msg_list(CLMSV_MSG ** head, CLMSV_MSG * new_node)
{

	TRACE_ENTER();

	CLMSV_MSG *temp;

	if (*head == NULL) {
		new_node->next = NULL;
		*head = new_node;
		TRACE("in the head");
	} else {
		temp = *head;
		while (temp->next != NULL)
			temp = temp->next;

		TRACE("in the tail");
		new_node->next = NULL;
		temp->next = new_node;
	}
	TRACE_LEAVE();
}

/*just cross check everything is fine.*/
void clma_msg_destroy(CLMSV_MSG * msg)
{

	TRACE_ENTER();
	if (!msg)
		return;

	if (msg->evt_type == CLMSV_CLMS_TO_CLMA_API_RESP_MSG) {
		if (msg->info.api_resp_info.type == CLMSV_TRACK_CURRENT_RESP) {
			if (msg->info.api_resp_info.param.track.notify_info->numberOfItems)
				free(msg->info.api_resp_info.param.track.notify_info->notification);

			free(msg->info.api_resp_info.param.track.notify_info);
		}
	} else if (msg->evt_type == CLMSV_CLMS_TO_CLMA_CBK_MSG) {
		if (msg->info.cbk_info.type == CLMSV_TRACK_CBK) {
			if (msg->info.cbk_info.param.track.buf_info.numberOfItems)
				free(msg->info.cbk_info.param.track.buf_info.notification);

			free(msg->info.cbk_info.param.track.root_cause_ent);
			free(msg->info.cbk_info.param.track.cor_ids);
		}
	}

	/* Now free the message */
	free(msg);

	TRACE_LEAVE();
}

static void clma_hdl_list_del(clma_client_hdl_rec_t ** list)
{
	clma_client_hdl_rec_t *client_hdl;

	while ((client_hdl = *list) != NULL) {
		*list = client_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_CLMA, client_hdl->local_hdl);

		free(client_hdl);
		client_hdl = 0;
	}
}

/*
 * To enable tracing early in saClmInitialize, use a GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
	char *value;

	/* Initialize trace system first of all so we can see what is going. */
	if ((value = getenv("CLMSV_TRACE_PATHNAME")) != NULL) {
		if (logtrace_init("clma", value, CATEGORY_ALL) != 0) {
			/* error, we cannot do anything */
			return;
		}
	}
}
