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

  DESCRIPTION: This file contains internode & external proxy component's
               data base creation, accessing and deletion of functions.

  FUNCTIONS INCLUDED in this module:
  
****************************************************************************/
#include "avnd.h"

/******************************************************************************
  Name          : avnd_nodeid_mdsdest_rec_add
 
  Description   : This routine adds NODE_ID to MDS_DEST record for a particular
                  AvND.
 
  Arguments     : cb  - ptr to the AvND control block.
                  mds_dest - Mds dest of AvND coming up.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_nodeid_mdsdest_rec_add(AVND_CB *cb, MDS_DEST mds_dest)
{
	AVND_NODEID_TO_MDSDEST_MAP *rec = NULL;
	NODE_ID node_id = 0;
	uint32_t res = NCSCC_RC_SUCCESS;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(mds_dest);

	rec = (AVND_NODEID_TO_MDSDEST_MAP *)ncs_patricia_tree_get(&cb->nodeid_mdsdest_db, (uint8_t *)&(node_id));
	if (rec != NULL) {
		LOG_ER("nodeid_mdsdest rec already exists, Rec Add Failed: MdsDest:%" PRId64 ", NodeId:%u",
				    mds_dest, node_id);
		return NCSCC_RC_FAILURE;
	} else {
		rec = new AVND_NODEID_TO_MDSDEST_MAP;

		rec->node_id = node_id;
		rec->mds_dest = mds_dest;
		rec->tree_node.bit = 0;
		rec->tree_node.key_info = (uint8_t *)&(rec->node_id);

		res = ncs_patricia_tree_add(&cb->nodeid_mdsdest_db, &rec->tree_node);

		if (NCSCC_RC_SUCCESS != res) {
			LOG_ER("Couldn't add nodeid_mdsdest rec, patricia add failed:MdsDest:%" PRId64 ", NodeId:%u",
			     mds_dest, node_id);
			delete rec;
			return res;
		}

	}			/* Else of if(rec != NULL)  */

	return res;

}

/******************************************************************************
  Name          : avnd_nodeid_mdsdest_rec_del
 
  Description   : This routine delete NODE_ID to MDS_DEST record for a particular
                  AvND.
 
  Arguments     : cb  - ptr to the AvND control block.
                  mds_dest - Mds dest of AvND coming up.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_nodeid_mdsdest_rec_del(AVND_CB *cb, MDS_DEST mds_dest)
{
	AVND_NODEID_TO_MDSDEST_MAP *rec = NULL;
	NODE_ID node_id = 0;
	uint32_t res = NCSCC_RC_SUCCESS;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(mds_dest);

	rec = (AVND_NODEID_TO_MDSDEST_MAP *)ncs_patricia_tree_get(&cb->nodeid_mdsdest_db, (uint8_t *)&(node_id));
	if (rec == NULL) {
		LOG_ER("nodeid_mdsdest rec doesn't exist, Rec del failed: MdsDest:%" PRId64 " NodeId:%u",
				    mds_dest, node_id);
		return NCSCC_RC_FAILURE;
	} else {
		res = ncs_patricia_tree_del(&cb->nodeid_mdsdest_db, &rec->tree_node);

		if (NCSCC_RC_SUCCESS != res) {
			LOG_ER("Couldn't del nodeid_mdsdest rec, patricia del failed: MdsDest:%" PRId64 ",NodeId:%u, res%u",
			     mds_dest, node_id, res);
			return res;
		}

	}			/* Else of if(rec == NULL) */

	delete rec;

	return res;
}

/******************************************************************************
  Name          : avnd_get_mds_dest_from_nodeid
 
  Description   : This routine return mds dest of AvND for a particular node id.
 
  Arguments     : cb  - ptr to the AvND control block.
                  mds_dest - Mds dest of AvND coming up.
 
  Return Values : 0/mds dest.
 
  Notes         : None
******************************************************************************/
MDS_DEST avnd_get_mds_dest_from_nodeid(AVND_CB *cb, NODE_ID node_id)
{
	AVND_NODEID_TO_MDSDEST_MAP *rec = NULL;

	rec = (AVND_NODEID_TO_MDSDEST_MAP *)ncs_patricia_tree_get(&cb->nodeid_mdsdest_db, (uint8_t *)&(node_id));
	if (rec == NULL) {
		LOG_ER("nodeid_mdsdest rec doesn't exist, Rec get failed: NodeId:%u",node_id);
		return 0;
	}

	return rec->mds_dest;
}

/****************************************************************************
  Name          : avnd_nodeid_to_mdsdest_map_db_init
 
  Description   : This routine initializes the node_id to mds dest map database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_nodeid_to_mdsdest_map_db_init(AVND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	params.key_size = sizeof(NODE_ID);
	rc = ncs_patricia_tree_init(&cb->nodeid_mdsdest_db, &params);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("nodeid_mdsdest_db initialization failed");
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_internode_avail_comp_db_init
 
  Description   : This routine initializes the available internode components database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_internode_avail_comp_db_init(AVND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	params.key_size = sizeof(SaNameT);
	rc = ncs_patricia_tree_init(&cb->internode_avail_comp_db, &params);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("internode_avail_comp_db initialization failed");
	}

	return rc;
}

/******************************************************************************
  Name          : avnd_internode_comp_add
 
  Description   : This routine adds an internode component in 
                  internode_avail_comp_db.
 
  Arguments     : ptree   - ptr to the patricia tree of data base.
                  name    - ptr to the component name.
                  node id - node id of the component.
                  rc      - out param for operation result
                  pxy_for_ext_comp - Whether this is a proxy for external
                  component.
                  comp_is_proxy - Whether internode component is proxy or
                                  proxied. 1 is for proxy, 0 is for proxied.
 
  Return Values : Pointer to AVND_COMP data structure
 
  Notes         : None
******************************************************************************/
AVND_COMP *avnd_internode_comp_add(NCS_PATRICIA_TREE *ptree, SaNameT *name,
				   NODE_ID node_id, uint32_t *rc, bool pxy_for_ext_comp, bool comp_is_proxy)
{
	AVND_COMP *comp = 0;

	*rc = SA_AIS_OK;

	/* verify if this component is already present in the db */
	if (NULL != (comp = m_AVND_COMPDB_REC_GET(*ptree, *name))) {
		/* This is a proxy and already proxying at least one component. 
		   So, no problem. */
		*rc = SA_AIS_ERR_EXIST;
		TRACE_1("avnd_internode_comp_add already exists. %s and NodeId:%u",
				      name->value, node_id);
		return comp;
	}

	/* a fresh comp... */
	comp = new AVND_COMP();
	comp->use_comptype_attr = new std::bitset<NumAttrs>;

	/* update the comp-name (patricia key) */
	memcpy(&comp->name, name, sizeof(SaNameT));

	comp->pres = SA_AMF_PRESENCE_UNINSTANTIATED;

	if (0 == node_id) {
		/* This means this is an external component. */
		m_AVND_COMP_TYPE_SET_EXT_CLUSTER(comp);
	}

	m_AVND_COMP_TYPE_SET_INTER_NODE(comp);

	if (true == pxy_for_ext_comp) {
		m_AVND_PROXY_FOR_EXT_COMP_SET(comp);
	}

	if (true == comp_is_proxy) {
		m_AVND_COMP_TYPE_PROXY_SET(comp);
	} else if (false == comp_is_proxy) {
		m_AVND_COMP_TYPE_PROXIED_SET(comp);
	}

	comp->node_id = node_id;

	/* initialize proxied list */
	avnd_pxied_list_init(comp);

	/* Add to the patricia tree. */
	comp->tree_node.bit = 0;
	comp->tree_node.key_info = (uint8_t *)&comp->name;
	*rc = ncs_patricia_tree_add(ptree, &comp->tree_node);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = SA_AIS_ERR_NO_MEMORY;
		goto err;
	}

	TRACE_1("avnd_internode_comp_add:%s nodeid:%u, pxy_for_ext_comp:%u,comp_is_proxy:%u",
			      comp->name.value, node_id, pxy_for_ext_comp, comp_is_proxy);
	return comp;

 err:

	if (comp) {
		avnd_comp_delete(comp);
	}

	LOG_ER("avnd_internode_comp_add failed.%s: NodeId:%u", name->value, node_id);
	return 0;

}

/******************************************************************************
  Name          : avnd_internode_comp_del
 
  Description   : This routine deletes an internode component from internode_avail_comp_db.
 
  Arguments     : ptree  - ptr to the patricia tree of data base.
                  name - ptr to the component name.
                        
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_internode_comp_del(AVND_CB *cb, NCS_PATRICIA_TREE *ptree, SaNameT *name)
{
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CBK *cbk_rec = NULL, *temp_cbk_ptr = NULL;

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(*ptree, *name);
	if (!comp) {
		rc = AVND_ERR_NO_COMP;
		LOG_ER("internode_comp_del failed. Rec doesn't exist :%s", name->value);
		goto err;
	}
	TRACE("avnd_internode_comp_del:%s: nodeid:%u, comp_type:%u",
			      comp->name.value, comp->node_id, comp->comp_type);

/*  Delete the callbacks if any. */
	cbk_rec = comp->cbk_list;
	while (cbk_rec) {
		temp_cbk_ptr = cbk_rec->next;
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, cbk_rec, AVND_CKPT_COMP_CBK_REC);
		avnd_comp_cbq_rec_del(cb, comp, cbk_rec);
		cbk_rec = temp_cbk_ptr;
	}

	/* 
	 * Remove from the patricia tree.
	 */
	rc = ncs_patricia_tree_del(ptree, &comp->tree_node);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = AVND_ERR_TREE;
		goto err;
	}

	/* free the memory */
	if (comp)
		avnd_comp_delete(comp);
	return rc;

 err:

	/* free the memory */
	if (comp)
		avnd_comp_delete(comp);

	LOG_ER("internode_comp_del failed: %s ,rc=%u", name->value, rc);

	return rc;

}
