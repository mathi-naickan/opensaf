/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#include "usrbuf.h"
#include "dtm.h"

/**
 * Function to add new node
 *
 * @param new_node
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_new(DTM_NODE_DB * new_node)
{

	DTM_NODE_DB *node = NULL;

	TRACE_ENTER();

	node = calloc(1, sizeof(DTM_NODE_DB));

	if (node == NULL) {
		LOG_ER("Calloc failed");
		goto done;
	}

	/* memset(node, 0, sizeof(DTM_NODE_DB)); */

	/*Initialize some attributes of the node like */
	memcpy(node, new_node, sizeof(DTM_NODE_DB));

 done:
	TRACE_LEAVE();
	return node;

}

/**
 * This function initializes the DTMS_CB including the Patricia trees
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_cb_init(DTM_INTERNODE_CB * dtms_cb)
{
	NCS_PATRICIA_PARAMS nodeid_param;
	NCS_PATRICIA_PARAMS comm_socket_param;
	NCS_PATRICIA_PARAMS ipaddr_param;

	TRACE_ENTER();

	memset(&nodeid_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&comm_socket_param, 0, sizeof(NCS_PATRICIA_PARAMS));
	memset(&ipaddr_param, 0, sizeof(NCS_PATRICIA_PARAMS));

	nodeid_param.key_size = sizeof(uint32_t);
	comm_socket_param.key_size = sizeof(uint32_t);
	ipaddr_param.key_size = INET6_ADDRSTRLEN;

	/* Initialize patricia tree for nodeid list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&dtms_cb->nodeid_tree, &nodeid_param)) {
		LOG_ER("DTM: ncs_patricia_tree_init FAILED");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize comm_socket patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&dtms_cb->comm_sock_tree, &comm_socket_param)) {
		LOG_ER("DTM:ncs_patricia_tree_init FAILED");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize comm_socket patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&dtms_cb->ip_addr_tree, &ipaddr_param)) {
		LOG_ER("DTM:ncs_patricia_tree_init FAILED");
		return NCSCC_RC_FAILURE;
	}

	if (m_NCS_IPC_CREATE(&dtms_cb->mbx) != NCSCC_RC_SUCCESS) {
		/* Mail box creation failed */
		LOG_ER("DTM:IPC create FAILED");
		return NCSCC_RC_FAILURE;
	} else {

		NCS_SEL_OBJ obj;

		if (NCSCC_RC_SUCCESS != m_NCS_IPC_ATTACH(&dtms_cb->mbx)) {
			m_NCS_IPC_RELEASE(&dtms_cb->mbx, NULL);
			LOG_ER("DTM: Internode Mailbox  Attach failed");
			return NCSCC_RC_FAILURE;
		}

		obj = m_NCS_IPC_GET_SEL_OBJ(&dtms_cb->mbx);

		/* retreive the corresponding fd for mailbox and fill it in cb */
		dtms_cb->mbx_fd = m_GET_FD_FROM_SEL_OBJ(obj);	/* extract and fill value needs to be extracted */
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Retrieve node from node db by nodeid
 *
 * @param nodeid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_get_by_id(uint32_t nodeid)
{
	DTM_NODE_DB *node = NULL;
	TRACE_ENTER();
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;

	node = (DTM_NODE_DB *) ncs_patricia_tree_get(&dtms_cb->nodeid_tree, (uint8_t *)&nodeid);
	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_nodeid))
					   - ((char *)((DTM_NODE_DB *) 0))));
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Retrieve next node from node db by nodeid
 *
 * @param nodeid
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_getnext_by_id(uint32_t node_id)
{
	DTM_NODE_DB *node = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();

	if (node_id == 0) {

		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->nodeid_tree, (uint8_t *)0);
	} else
		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->nodeid_tree, (uint8_t *)&node_id);

	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_nodeid))
					   - ((char *)((DTM_NODE_DB *) 0))));
		TRACE("DTM:Node found %d", node->node_id);
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Retrieve node from node db by comm_socket
 *
 * @param comm_socket
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_get_by_comm_socket(uint32_t comm_socket)
{
	DTM_NODE_DB *node = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();

	node = (DTM_NODE_DB *) ncs_patricia_tree_get(&dtms_cb->comm_sock_tree, (uint8_t *)&comm_socket);
	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_comm_socket))
					   - ((char *)((DTM_NODE_DB *) 0))));
		TRACE("DTM:Node found %d", node->comm_socket);
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Retrieve next node from node db by comm_socket
 *
 * @param comm_socket
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_getnext_by_comm_socket(uint32_t comm_socket)
{
	DTM_NODE_DB *node = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();

	if (comm_socket == 0) {

		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->comm_sock_tree, (uint8_t *)0);
	} else
		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->comm_sock_tree, (uint8_t *)&comm_socket);

	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_comm_socket))
					   - ((char *)((DTM_NODE_DB *) 0))));
		TRACE("DTM:Node found %d", node->comm_socket);
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Retrieve node from node db by node_ip
 *
 * @param comm_socket
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_get_by_node_ip(uint8_t *node_ip)
{
	DTM_NODE_DB *node = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();

	node = (DTM_NODE_DB *) ncs_patricia_tree_get(&dtms_cb->ip_addr_tree, (uint8_t *)node_ip);
	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_ip_address))
					   - ((char *)((DTM_NODE_DB *) 0))));
		TRACE("DTM:Node found %s", node_ip);
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Retrieve next node from node db by node_ip
 *
 * @param node_ip
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
DTM_NODE_DB *dtm_node_getnext_by_nodeaddr(uint8_t *node_ip)
{
	DTM_NODE_DB *node = NULL;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();

	if (node_ip == NULL) {

		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->ip_addr_tree, (uint8_t *)0);
	} else {
		node = (DTM_NODE_DB *) ncs_patricia_tree_getnext(&dtms_cb->ip_addr_tree, (uint8_t *)node_ip);
	}
	if (node != (DTM_NODE_DB *) NULL) {
		/* Adjust the pointer */
		node = (DTM_NODE_DB *) (((char *)node)
					- (((char *)&(((DTM_NODE_DB *) 0)->pat_ip_address))
					   - ((char *)((DTM_NODE_DB *) 0))));
		TRACE("DTM:Node found %s", node_ip);
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Adds the node to patricia tree
 *
 * @param node
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_node_add(DTM_NODE_DB * node, int i)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER();
	TRACE("DTM:value of i %d", i);

	osafassert(node != NULL);

	switch (i) {

	case 0:
		TRACE("DTM:Adding node_id to the patricia tree with node_id :%u as key", node->node_id);
		node->pat_nodeid.key_info = (uint8_t *)&(node->node_id);
		rc = ncs_patricia_tree_add(&dtms_cb->nodeid_tree, &node->pat_nodeid);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("DTM:ncs_patricia_tree_add for node_id  FAILED for :%d :%u", node->node_id, rc);
			node->pat_nodeid.key_info = NULL;
			goto done;
		}
		break;
	case 1:
		TRACE("DTM:Adding comm_socket to the patricia tree with comm_socket :%u as key", node->comm_socket);
		node->pat_comm_socket.key_info = (uint8_t *)&(node->comm_socket);
		rc = ncs_patricia_tree_add(&dtms_cb->comm_sock_tree, &node->pat_comm_socket);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("DTM:ncs_patricia_tree_add for comm_socket  FAILED for :%d :%u", node->comm_socket, rc);
			node->pat_comm_socket.key_info = NULL;
			goto done;
		}
		break;

	case 2:
		TRACE("DTM:Adding node_ip to the patricia tree with node_ip :%s as key", node->node_ip);
		node->pat_ip_address.key_info = (uint8_t *)&(node->node_ip);
		rc = ncs_patricia_tree_add(&dtms_cb->ip_addr_tree, &node->pat_ip_address);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("DTM:ncs_patricia_tree_add for node_ip  FAILED for :%s :%u", node->node_ip, rc);
			node->pat_comm_socket.key_info = NULL;
			goto done;
		}
		break;

	default:
		TRACE("DTM:Invalid Patricia add");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	TRACE_LEAVE2("rc : %d", rc);
	return rc;
}

/**
 * Delete the node from the nodedb
 *
 * @param node
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_node_delete(DTM_NODE_DB * node, int i)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;
	TRACE_ENTER2("DTM:value of i %d", i);

	osafassert(node != NULL);

	switch (i) {

	case 0:
		if (node->node_id != 0) {
			TRACE("DTM:Deleting node_id from the patricia tree with node_id :%u as key", node->node_id);
			if ((rc = ncs_patricia_tree_del(&dtms_cb->nodeid_tree, &node->pat_nodeid)) != NCSCC_RC_SUCCESS) {
				TRACE("DTM:ncs_patricia_tree_del FAILED for node_id :%u rc :%d", node->node_id, rc);
				goto done;
			}
		}
		break;
	case 1:
		if (node->comm_socket != 0) {
			TRACE("DTM:Deleting comm_socket  from  the patricia tree with comm_socket :%u as key", node->comm_socket);
			if ((rc =
			     ncs_patricia_tree_del(&dtms_cb->comm_sock_tree,
						   &node->pat_comm_socket)) != NCSCC_RC_SUCCESS) {
				TRACE("DTM:ncs_patricia_tree_del  FAILED for comm_socket :%d rc :%u", node->comm_socket, rc);
				goto done;
			}
		}
		break;

	case 2:
		if (node->node_ip != NULL) {
			TRACE("DTM:Deletinging node_ip from the patricia tree with node_ip :%s as key", node->node_ip);
			if ((rc =
			     ncs_patricia_tree_del(&dtms_cb->ip_addr_tree,
						   &node->pat_ip_address)) != NCSCC_RC_SUCCESS) {
				TRACE("DTM:ncs_patricia_tree_del FAILED for node_ip :%s rc :%u", node->node_ip, rc);
				goto done;
			}
		}
		break;

	default:
		osafassert(0);
	}

 done:
	TRACE_LEAVE();
	return rc;
}
