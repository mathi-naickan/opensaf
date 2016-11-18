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

#include "amf/amfnd/avnd.h"
#include "osaf/immutil/immutil.h"
#include "base/osaf_utility.h"

static pthread_mutex_t sudb_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	const std::string info_name = Amf::to_string(&info->name);

	TRACE_ENTER2("SU'%s'", info_name.c_str());
	/* verify if this su is already present in the db */
	if (avnd_sudb_rec_get(cb->sudb, info_name)) {
		*rc = AVND_ERR_DUP_SU;
		goto err;
	}

	/* a fresh su... */
	su = new AVND_SU();

	/*
	 * Update the config parameters.
	 */
	/* update the su-name (patricia key) */
	su->name = info_name;

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
	su->si_list.cmp_cookie = avsv_dblist_sastring_cmp;
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
	 * Add to db.
	 */
	osaf_mutex_lock_ordie(&sudb_mutex);
	*rc = cb->sudb.insert(su->name, su);
	osaf_mutex_unlock_ordie(&sudb_mutex);
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

	LOG_CR("SU DB record %s add failed", info_name.c_str());
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
uint32_t avnd_sudb_rec_del(AVND_CB *cb, const std::string& name)
{
	AVND_SU *su = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP *comp;

	TRACE_ENTER2("%s", name.c_str());

	/* get the su record */
	su = avnd_sudb_rec_get(cb->sudb, name);
	if (!su) {
		LOG_NO("%s: %s not found", __FUNCTION__, su->name.c_str());
		rc = AVND_ERR_NO_SU;
		goto done;
	}

	/* Delete all components */
	while ((comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list)))) {
		rc = avnd_compdb_rec_del(cb, comp->name);
		if (rc != NCSCC_RC_SUCCESS)
			goto done;
	}

	/* SU should not have any comp or SI attached to it */
	osafassert(su->comp_list.n_nodes == 0);
	osafassert(su->si_list.n_nodes == 0);

	osaf_mutex_lock_ordie(&sudb_mutex);
	/* remove from container */
	cb->sudb.erase(su->name);
	osaf_mutex_unlock_ordie(&sudb_mutex);
	/* remove the association with hdl mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, su->su_hdl);

	/* free the memory */
	delete su;

done:
	if (rc == NCSCC_RC_SUCCESS)
		LOG_IN("Deleted '%s'", name.c_str());
	else
		LOG_ER("Delete of '%s' failed", name.c_str());

	TRACE_LEAVE();
	return rc;
}

uint32_t avnd_su_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	AVND_SU *su;
	uint32_t rc = NCSCC_RC_FAILURE;
	const std::string param_name = Amf::to_string(&param->name);

	TRACE_ENTER2("'%s'", param_name.c_str());

	su = avnd_sudb_rec_get(cb->sudb, param_name);
	if (!su) {
		LOG_ER("%s: failed to get %s", __FUNCTION__, param_name.c_str());
		goto done;
	}

	switch (param->act) {
	case AVSV_OBJ_OPR_DEL:
		/* delete the record */
		rc = avnd_sudb_rec_del(cb, param_name);
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

/**
 * This function return SU rec.
 * @param Pointer to SUDB.
 * @param Pointer to SU name.
 * @return Pointer to SU.
 */
AVND_SU *avnd_sudb_rec_get(AmfDb<std::string, AVND_SU>& sudb, const std::string& name) {
	osaf_mutex_lock_ordie(&sudb_mutex);
	AVND_SU *su = sudb.find(name);
	osaf_mutex_unlock_ordie(&sudb_mutex);
	return su;
}

/**
 * This function return next SU rec.
 * @param Pointer to SUDB.
 * @param Pointer to SU name.
 * @return Pointer to SU.
 */
AVND_SU *avnd_sudb_rec_get_next(AmfDb<std::string, AVND_SU>& sudb, const std::string& name) {
	osaf_mutex_lock_ordie(&sudb_mutex);
	AVND_SU *su = nullptr;

	if (sudb.size() == 0)
		goto done;

	if (name == "") {
		std::map<std::string, AVND_SU*>::iterator it = sudb.begin();
		su = it->second;
	} else {
		su = sudb.findNext(name);
	}
	
done:
	osaf_mutex_unlock_ordie(&sudb_mutex);
	return su;
}

/**
 * This function adds comp to the comp-list
 * @param Pointer to SUDB.
 * @param Pointer to SU name.
 * @return Pointer to SU.
 */
void sudb_rec_comp_add(AVND_SU *su, AVND_COMP *comp, uint32_t *rc) {
	osaf_mutex_lock_ordie(&sudb_mutex);
	comp->su_dll_node.key = (uint8_t *)&((comp)->inst_level);
	*rc = ncs_db_link_list_add(&(su)->comp_list, &(comp)->su_dll_node);
	osaf_mutex_unlock_ordie(&sudb_mutex);
}

