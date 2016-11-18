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

  This file contains functions related to the Agent structure handling.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************/

#include "lck/lcknd/glnd.h"

/*****************************************************************************
  PROCEDURE NAME : glnd_agent_node_find

  DESCRIPTION    : Finds the Agent info node from the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  mds_handle_id  - vcard id of the agent.

  RETURNS        :The pointer to the agent info node

  NOTES         : None
*****************************************************************************/
GLND_AGENT_INFO *glnd_agent_node_find(GLND_CB *glnd_cb, MDS_DEST agent_mds_dest)
{
	GLND_AGENT_INFO *agent_info;

	/* search for the agent id */
	agent_info = (GLND_AGENT_INFO *)ncs_patricia_tree_get(&glnd_cb->glnd_agent_tree, (uint8_t *)&agent_mds_dest);
	return agent_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_agent_node_add

  DESCRIPTION    : Adds the Agent node to the Agent tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  agent_mds_dest   - mds dest id for the agent.
                  process_id
                 

  RETURNS        :The pointer to the agent info node on success.
                  else returns NULL.

  NOTES         : None
*****************************************************************************/
GLND_AGENT_INFO *glnd_agent_node_add(GLND_CB *glnd_cb, MDS_DEST agent_mds_dest, uint32_t process_id)
{
	GLND_AGENT_INFO *agent_info;
	TRACE_ENTER();

	agent_info = glnd_agent_node_find(glnd_cb, agent_mds_dest);

	if (!agent_info) {
		/* create new agent info and put it into the tree */
		if ((agent_info = m_MMGR_ALLOC_GLND_AGENT_INFO) == NULL) {
			LOG_CR("GLND agent alloc failed: agent_mds_dest %" PRIx64 "Error %s", agent_mds_dest, strerror(errno));
			assert(0);
		}
		agent_info->agent_mds_id = agent_mds_dest;
		agent_info->process_id = process_id;
		agent_info->patnode.key_info = (uint8_t *)&agent_info->agent_mds_id;
		if (ncs_patricia_tree_add(&glnd_cb->glnd_agent_tree, &agent_info->patnode) != NCSCC_RC_SUCCESS) {
			LOG_ER("GLND agent tree add failed: agent_mds_dest %" PRIx64, agent_mds_dest);
			/* free and return */
			m_MMGR_FREE_GLND_AGENT_INFO(agent_info);
			agent_info = NULL;
			goto end;
		}
		goto end;
	}
end:
	TRACE_LEAVE();
	return agent_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_agent_node_del

  DESCRIPTION    : Deletes the Resource node from the resource tree.

  ARGUMENTS      : 
                  glnd_cb      - ptr to the GLND control block
                  agent_info - ptr to the agent Info.

  RETURNS        : NCSCC_RC_SUCCESS/NCS_RC_FAILURE

  NOTES         : 
*****************************************************************************/
void glnd_agent_node_del(GLND_CB *glnd_cb, GLND_AGENT_INFO *agent_info)
{
	GLND_CLIENT_INFO *client_info;
	SaLckHandleT handle_id = 0;
	TRACE_ENTER();

	/* detach it from the tree */
	if (ncs_patricia_tree_del(&glnd_cb->glnd_agent_tree, (NCS_PATRICIA_NODE *)&agent_info->patnode)
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND agent tree del failed: agent_mds_id %" PRIx64, agent_info->agent_mds_id);
		goto end;
	}

	/* clean up all the client info that has been part of the agent */
	while ((client_info = glnd_client_node_find_next(glnd_cb, handle_id, agent_info->agent_mds_id))) {
		handle_id = client_info->app_handle_id;
		/* delete the client info */
		glnd_client_node_del(glnd_cb, client_info);
	}

	/* free the memory */
	m_MMGR_FREE_GLND_AGENT_INFO(agent_info);
end:
	TRACE_LEAVE();
}
