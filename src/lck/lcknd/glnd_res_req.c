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
  FILE NAME: GLND_RES_REQ.C

  DESCRIPTION: API's used to handle Resource Request Queue

  FUNCTIONS INCLUDED in this module:
  glnd_resource_req_node_add  -     To add the node to the resource request list
  glnd_resource_req_node_find -     To Find the resource request node from the list.
  glnd_resource_req_node_del  -     To delete the node from the resource request list.

******************************************************************************/

#include "lck/lcknd/glnd.h"
#include<string.h>
/*****************************************************************************
  PROCEDURE NAME : glnd_resource_req_node_add

  DESCRIPTION    : Adds the Resource request node 

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  rsc_info     - ptr to the resource request info.

  RETURNS        :The pointer to the resource req node info on success.
                  else returns NULL.

  NOTES         : None
*****************************************************************************/
GLND_RESOURCE_REQ_LIST *glnd_resource_req_node_add(GLND_CB *glnd_cb,
						   GLSV_EVT_RSC_INFO *rsc_info,
						   MDS_SYNC_SND_CTXT *mds_ctxt, SaLckResourceIdT lcl_resource_id)
{

	GLND_RESOURCE_REQ_LIST *res_req_info;

	res_req_info = (GLND_RESOURCE_REQ_LIST *)m_MMGR_ALLOC_GLND_RESOURCE_REQ_LIST;

	if (!res_req_info) {
		LOG_CR("GLND Rsc req list alloc failed: Error %s", strerror(errno));
		assert(0);
	}

	memset(res_req_info, 0, sizeof(GLND_RESOURCE_REQ_LIST));
	res_req_info->res_req_hdl_id = ncshm_create_hdl((uint8_t)glnd_cb->pool_id,
							NCS_SERVICE_ID_GLND, (NCSCONTEXT)res_req_info);
	if (!res_req_info->res_req_hdl_id) {
		LOG_ER("GLND Rsc req create handle failed");
		m_MMGR_FREE_GLND_RESOURCE_REQ_LIST(res_req_info);
		return NULL;
	}
	memcpy(&res_req_info->resource_name, &rsc_info->resource_name, sizeof(SaNameT));
	res_req_info->client_handle_id = rsc_info->client_handle_id;
	res_req_info->invocation = rsc_info->invocation;
	res_req_info->agent_mds_dest = rsc_info->agent_mds_dest;
	res_req_info->call_type = rsc_info->call_type;
	res_req_info->glnd_res_mds_ctxt = *mds_ctxt;
	res_req_info->lcl_resource_id = lcl_resource_id;

	/* add it to the list */
	if (glnd_cb->res_req_list != NULL) {
		res_req_info->next = glnd_cb->res_req_list;
		glnd_cb->res_req_list->prev = res_req_info;
		glnd_cb->res_req_list = res_req_info;
	} else {
		glnd_cb->res_req_list = res_req_info;
	}

	/* start the timeout timer */
	if (rsc_info->call_type == GLSV_SYNC_CALL) {
		glnd_start_tmr(glnd_cb, &res_req_info->timeout,
			       GLND_TMR_RES_REQ_TIMEOUT, rsc_info->timeout, (uint32_t)res_req_info->res_req_hdl_id);
	} else {
		glnd_start_tmr(glnd_cb, &res_req_info->timeout,
			       GLND_TMR_RES_REQ_TIMEOUT,
			       GLSV_LOCK_DEFAULT_TIMEOUT, (uint32_t)res_req_info->res_req_hdl_id);

	}
	return res_req_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_req_node_find

  DESCRIPTION    : find the Resource request node 

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  

  RETURNS        :The pointer to the resource req node info on success.
                  else returns NULL.

  NOTES         : 
*****************************************************************************/
GLND_RESOURCE_REQ_LIST *glnd_resource_req_node_find(GLND_CB *glnd_cb, SaNameT *resource_name)
{
	GLND_RESOURCE_REQ_LIST *res_req_info;

	/* find it from the list */
	for (res_req_info = glnd_cb->res_req_list; res_req_info != NULL; res_req_info = res_req_info->next) {
		if (memcmp(resource_name, &res_req_info->resource_name, sizeof(SaNameT)) == 0) {
			break;
		}
	}

	return res_req_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_req_node_del

  DESCRIPTION    : deletes the Resource request node 

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  

  RETURNS        :The pointer to the resource req node info on success.
                  else returns NULL.

  NOTES         : Delete the returned pointer immediately.
*****************************************************************************/
void glnd_resource_req_node_del(GLND_CB *glnd_cb, uint32_t res_req_hdl)
{
	GLND_RESOURCE_REQ_LIST *res_req_info;
	res_req_info = (GLND_RESOURCE_REQ_LIST *)ncshm_take_hdl(NCS_SERVICE_ID_GLND, res_req_hdl);

	if (res_req_info != NULL) {
		/* delete it from the list and return the pointer */
		if (glnd_cb->res_req_list == res_req_info)
			glnd_cb->res_req_list = glnd_cb->res_req_list->next;
		if (res_req_info->prev)
			res_req_info->prev->next = res_req_info->next;
		if (res_req_info->next)
			res_req_info->next->prev = res_req_info->prev;

		glnd_stop_tmr(&res_req_info->timeout);

		ncshm_give_hdl(res_req_hdl);
		/* destroy the handle */
		ncshm_destroy_hdl(NCS_SERVICE_ID_GLND, res_req_info->res_req_hdl_id);

		/* free the memory */
		m_MMGR_FREE_GLND_RESOURCE_REQ_LIST(res_req_info);

	}
	return;
}
