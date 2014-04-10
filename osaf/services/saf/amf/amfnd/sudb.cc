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

  DESCRIPTION:
   This module deals with the creation, accessing and deletion of
   the SU database on the AVND.

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"
#include <immutil.h>

/****************************************************************************
  Name          : avnd_sudb_init
 
  Description   : This routine initializes the SU database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_sudb_init(AVND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	params.key_size = sizeof(SaNameT);
	rc = ncs_patricia_tree_init(&cb->sudb, &params);
	if (NCSCC_RC_SUCCESS == rc)
		TRACE("SU DB create success");
	else
		LOG_CR("SU DB create failed");

	return rc;
}

/****************************************************************************
  Name          : avnd_sudb_rec_add
 
  Description   : This routine adds a SU record to the SU database. If a SU 
                  is already present, nothing is done.
 
  Arguments     : cb   - ptr to the AvND control block
                  info - ptr to the SU parameters
                  rc   - ptr to the operation result
 
  Return Values : ptr to the SU record, if success
                  0, otherwise
 
  Notes         : None.
******************************************************************************/
AVND_SU *avnd_sudb_rec_add(AVND_CB *cb, AVND_SU_PARAM *info, uint32_t *rc)
{
	AVND_SU *su = 0;
	TRACE_ENTER2("SU'%s'", info->name.value);
	/* verify if this su is already present in the db */
	if (0 != m_AVND_SUDB_REC_GET(cb->sudb, info->name)) {
		*rc = AVND_ERR_DUP_SU;
		goto err;
	}

	/* a fresh su... */
	su = new AVND_SU();

	/*
	 * Update the config parameters.
	 */
	/* update the su-name (patricia key) */
	memcpy(&su->name, &info->name, sizeof(SaNameT));

	/* update error recovery escalation parameters */
	su->comp_restart_prob = info->comp_restart_prob;
	su->comp_restart_max = info->comp_restart_max;
	su->su_restart_prob = info->su_restart_prob;
	su->su_restart_max = info->su_restart_max;
	su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;

	/* update the NCS flag */
	su->is_ncs = info->is_ncs;
	su->su_is_external = info->su_is_external;
	su->sufailover = info->su_failover;

	/*
	 * Update the rest of the parameters with default values.
	 */
	m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
	su->pres = SA_AMF_PRESENCE_UNINSTANTIATED;
	su->avd_updt_flag = false;

	/* 
	 * Initialize the comp-list.
	 */
	su->comp_list.order = NCS_DBLIST_ASSCEND_ORDER;
	su->comp_list.cmp_cookie = avsv_dblist_uns32_cmp;
	su->comp_list.free_cookie = 0;

	/* 
	 * Initialize the si-list.
	 */
	su->si_list.order = NCS_DBLIST_ASSCEND_ORDER;
	su->si_list.cmp_cookie = avsv_dblist_saname_cmp;
	su->si_list.free_cookie = 0;

	/* 
	 * Initialize the siq.
	 */
	su->siq.order = NCS_DBLIST_ANY_ORDER;
	su->siq.cmp_cookie = 0;
	su->siq.free_cookie = 0;

	/* create the association with hdl-mngr */
	if ((0 == (su->su_hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_AVND, (NCSCONTEXT)su)))) {
		*rc = AVND_ERR_HDL;
		goto err;
	}

	/* 
	 * Add to the patricia tree.
	 */
	su->tree_node.bit = 0;
	su->tree_node.key_info = (uint8_t *)&su->name;
	*rc = ncs_patricia_tree_add(&cb->sudb, &su->tree_node);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_TREE;
		goto err;
	}

	return su;

 err:
	if (su) {
		if (su->su_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, su->su_hdl);
		delete su;
	}

	LOG_CR("SU DB record %s add failed", info->name.value);
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_sudb_rec_del
 
  Description   : This routine deletes a SU record from the SU database.
 
  Arguments     : cb       - ptr to the AvND control block
                  name - ptr to the su-name (in n/w order)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : All the components belonging to this SU should have been 
                  deleted before deleting the SU record. Also no SIs should be
                  attached to this record.
******************************************************************************/
uint32_t avnd_sudb_rec_del(AVND_CB *cb, SaNameT *name)
{
	AVND_SU *su = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP *comp;

	TRACE_ENTER2("%s", name->value);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *name);
	if (!su) {
		LOG_NO("%s: %s not found", __FUNCTION__, su->name.value);
		rc = AVND_ERR_NO_SU;
		goto done;
	}

	/* Delete all components */
	while ((comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list)))) {
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, comp, AVND_CKPT_COMP_CONFIG);
		rc = avnd_compdb_rec_del(cb, &comp->name);
		if (rc != NCSCC_RC_SUCCESS)
			goto done;
	}

	/* SU should not have any comp or SI attached to it */
	osafassert(su->comp_list.n_nodes == 0);
	osafassert(su->si_list.n_nodes == 0);

	/* remove from the patricia tree */
	rc = ncs_patricia_tree_del(&cb->sudb, &su->tree_node);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_NO("%s: %s tree del failed", __FUNCTION__, su->name.value);
		rc = AVND_ERR_TREE;
		goto done;
	}

	/* remove the association with hdl mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, su->su_hdl);

	/* free the memory */
	delete su;

done:
	if (rc == NCSCC_RC_SUCCESS)
		LOG_IN("Deleted '%s'", name->value);
	else
		LOG_ER("Delete of '%s' failed", name->value);

	TRACE_LEAVE();
	return rc;
}

uint32_t avnd_su_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	AVND_SU *su;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", param->name.value);

	su = m_AVND_SUDB_REC_GET(cb->sudb, param->name);
	if (!su) {
		LOG_ER("%s: failed to get %s", __FUNCTION__, param->name.value);
		goto done;
	}

	switch (param->act) {
	case AVSV_OBJ_OPR_DEL:
		/* delete the record */
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVND_CKPT_SU_CONFIG);
		rc = avnd_sudb_rec_del(cb, &param->name);
		break;
	case AVSV_OBJ_OPR_MOD: {
		switch (param->attr_id) {
		case saAmfSUFailOver_ID:
			osafassert(sizeof(uint32_t) == param->value_len);
			su->sufailover = m_NCS_OS_NTOHL(*(uint32_t *)(param->value));
			break;
		default:
			LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
			goto done;
		}
		break;
	}
	default:
		LOG_NO("%s: Unsupported action %u", __FUNCTION__, param->act);
		goto done;
		break;
	}

	rc = NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}
