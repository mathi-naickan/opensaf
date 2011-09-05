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

   Vinay Khanna

..............................................................................

  DESCRIPTION:
   This module deals with the creation, accessing and deletion of the component
   database in the AVND.

..............................................................................

  FUNCTIONS:

  
******************************************************************************
*/

#include <stdbool.h>

#include <saImmOm.h>
#include <avsv_util.h>
#include <immutil.h>
#include <logtrace.h>

#include <avnd.h>

static int get_string_attr_from_imm(SaImmOiHandleT immOmHandle, SaImmAttrNameT attrName, const SaNameT *dn, SaStringT *str);
/* AMF Class SaAmfCompGlobalAttributes */
typedef struct {
	SaUint32T saAmfNumMaxInstantiateWithoutDelay;
	SaUint32T saAmfNumMaxInstantiateWithDelay;
	SaUint32T saAmfNumMaxAmStartAttempts;
	SaUint32T saAmfNumMaxAmStopAttempts;
	SaTimeT saAmfDelayBetweenInstantiateAttempts;
} amf_comp_global_attr_t;

static amf_comp_global_attr_t comp_global_attrs;

/* AMF Class SaAmfCompType */
typedef struct amf_comp_type {
	NCS_PATRICIA_NODE tree_node;	/* name is key */
	SaNameT    name;
	SaAmfCompCategoryT saAmfCtCompCategory;
	SaNameT    saAmfCtSwBundle;
	SaStringT *saAmfCtDefCmdEnv;
	SaTimeT    saAmfCtDefClcCliTimeout;
	SaTimeT    saAmfCtDefCallbackTimeout;
	SaStringT  saAmfCtRelPathInstantiateCmd;
	SaStringT *saAmfCtDefInstantiateCmdArgv;
	SaUint32T  saAmfCtDefInstantiationLevel;
	SaStringT  saAmfCtRelPathTerminateCmd;
	SaStringT *saAmfCtDefTerminateCmdArgv;
	SaStringT  saAmfCtRelPathCleanupCmd;
	SaStringT *saAmfCtDefCleanupCmdArgv;
	SaStringT  saAmfCtRelPathAmStartCmd;
	SaStringT *saAmfCtDefAmStartCmdArgv;
	SaStringT  saAmfCtRelPathAmStopCmd;
	SaStringT *saAmfCtDefAmStopCmdArgv;
	SaTimeT    saAmfCompQuiescingCompleteTimeout;
	SaAmfRecommendedRecoveryT saAmfCtDefRecoveryOnError;
	SaBoolT saAmfCtDefDisableRestart;
} amf_comp_type_t;

/* We should only get the config attributes from IMM to avoid problems
   with a pure runtime attributes */
static SaImmAttrNameT compConfigAttributes[] = {
	"saAmfCompType",
	"saAmfCompCmdEnv",
	"saAmfCompInstantiateCmdArgv",
	"saAmfCompInstantiateTimeout",
	"saAmfCompInstantiationLevel",
	"saAmfCompNumMaxInstantiateWithoutDelay",
	"saAmfCompNumMaxInstantiateWithDelay",
	"saAmfCompDelayBetweenInstantiateAttempts",
	"saAmfCompTerminateCmdArgv",
	"saAmfCompTerminateTimeout",
	"saAmfCompCleanupCmdArgv",
	"saAmfCompCleanupTimeout",
	"saAmfCompAmStartCmdArgv",
	"saAmfCompAmStartTimeout",
	"saAmfCompNumMaxAmStartAttempts",
	"saAmfCompAmStopCmdArgv",
	"saAmfCompAmStopTimeout",
	"saAmfCompNumMaxAmStopAttempts",
	"saAmfCompCSISetCallbackTimeout",
	"saAmfCompCSIRmvCallbackTimeout",
	"saAmfCompQuiescingCompleteTimeout",
	"saAmfCompRecoveryOnError",
	"saAmfCompDisableRestart",
	NULL
};

/*****************************************************************************
 ****  Component Part of AVND AMF Configuration Database Layout           **** 
 *****************************************************************************
 
                   AVND_COMP
                   ---------------- 
   AVND_CB        | Stuff...       |
   -----------    | Attrs          |
  | COMP-Tree |-->|                |
  | ....      |   |                |
  | ....      |   |                |
   -----------    |                |
                  |                |
                  |                |
                  | Proxy ---------|-----> AVND_COMP (Proxy)
   AVND_SU        |                |
   -----------    |                |
  | Child     |-->|-SU-Comp-Next---|-----> AVND_COMP (Next)
  | Comp-List |   |                |
  |           |<--|-Parent SU      |       AVND_COMP_CSI_REC
   -----------    |                |       -------------
                  | CSI-Assign-List|----->|             |
                  |                |      |             |
                   ----------------        -------------

****************************************************************************/

static SaAisErrorT avnd_compglobalattrs_config_get(void)
{
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	SaNameT dn = {.value = "safRdn=compGlobalAttributes,safApp=safAmfService" };

	dn.length = strlen((char *)dn.value);

	immutil_saImmOmAccessorInitialize(avnd_cb->immOmHandle, &accessorHandle);
	rc = immutil_saImmOmAccessorGet_2(accessorHandle, &dn, NULL, (SaImmAttrValuesT_2 ***)&attributes);
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmAccessorGet_2 FAILED %u", rc);
		goto done;
	}

	avnd_log(NCSFL_SEV_NOTICE, "'%s'", dn.value);

	if (immutil_getAttr("saAmfNumMaxInstantiateWithoutDelay", attributes, 0,
			    &comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay) != SA_AIS_OK) {
		comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay = 2;
	}

	if (immutil_getAttr("saAmfNumMaxInstantiateWithDelay", attributes, 0,
			    &comp_global_attrs.saAmfNumMaxInstantiateWithDelay) != SA_AIS_OK) {
		comp_global_attrs.saAmfNumMaxInstantiateWithDelay = 0;
	}

	if (immutil_getAttr("saAmfNumMaxAmStartAttempts", attributes, 0,
			    &comp_global_attrs.saAmfNumMaxAmStartAttempts) != SA_AIS_OK) {
		comp_global_attrs.saAmfNumMaxAmStartAttempts = 2;
	}

	if (immutil_getAttr("saAmfNumMaxAmStopAttempts", attributes, 0,
			    &comp_global_attrs.saAmfNumMaxAmStopAttempts) != SA_AIS_OK) {
		comp_global_attrs.saAmfNumMaxAmStopAttempts = 2;
	}

	if (immutil_getAttr("saAmfDelayBetweenInstantiateAttempts", attributes, 0,
			    &comp_global_attrs.saAmfDelayBetweenInstantiateAttempts) != SA_AIS_OK) {
		comp_global_attrs.saAmfDelayBetweenInstantiateAttempts = 0;
	}

	immutil_saImmOmAccessorFinalize(accessorHandle);

	rc = SA_AIS_OK;

done:
	return rc;
}

/****************************************************************************
  Name          : avnd_compdb_init
 
  Description   : This routine initializes the component database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_compdb_init(AVND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));

	if (avnd_compglobalattrs_config_get() != SA_AIS_OK)
		return NCSCC_RC_FAILURE;

	params.key_size = sizeof(SaNameT);
	rc = ncs_patricia_tree_init(&cb->compdb, &params);
	if (NCSCC_RC_SUCCESS == rc)
		m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_CREATE, AVND_LOG_COMP_DB_SUCCESS, 0, 0, NCSFL_SEV_INFO);
	else
		m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_CREATE, AVND_LOG_COMP_DB_FAILURE, 0, 0, NCSFL_SEV_CRITICAL);

	return rc;
}

/****************************************************************************
  Name          : avnd_compdb_destroy
 
  Description   : This routine destroys the component database. It deletes 
                  all the component records in the database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_compdb_destroy(AVND_CB *cb)
{
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* scan & delete each comp */
	while (0 != (comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uns8 *)0))) {
		/* delete the record 
		   m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, comp, AVND_CKPT_COMP_CONFIG);
		   AvND is going down, but don't send any async update even for 
		   external components, otherwise external components will be deleted
		   from ACT. */
		rc = avnd_compdb_rec_del(cb, &comp->name);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;
	}

	/* finally destroy patricia tree */
	rc = ncs_patricia_tree_destroy(&cb->compdb);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_DESTROY, AVND_LOG_COMP_DB_SUCCESS, 0, 0, NCSFL_SEV_INFO);
	return rc;

 err:
	m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_DESTROY, AVND_LOG_COMP_DB_FAILURE, 0, 0, NCSFL_SEV_CRITICAL);
	return rc;
}

/****************************************************************************
  Name          : avnd_compdb_rec_add
 
  Description   : This routine adds a component record to the component 
                  database. If a component is already present, nothing is 
                  done.
 
  Arguments     : cb   - ptr to the AvND control block
                  info - ptr to the component params (comp-name -> nw order)
                  rc   - ptr to the operation result
 
  Return Values : ptr to the component record, if success
                  0, otherwise
 
  Notes         : None.
******************************************************************************/
AVND_COMP *avnd_compdb_rec_add(AVND_CB *cb, AVND_COMP_PARAM *info, uns32 *rc)
{
	AVND_COMP *comp = 0;
	AVND_SU *su = 0;
	SaNameT su_name;

	*rc = NCSCC_RC_SUCCESS;

	/* verify if this component is already present in the db */
	if (0 != m_AVND_COMPDB_REC_GET(cb->compdb, info->name)) {
		*rc = AVND_ERR_DUP_COMP;
		goto err;
	}

	/*
	 * Determine if the SU is present
	 */
	/* extract the su-name from comp dn */
	memset(&su_name, 0, sizeof(SaNameT));
	avsv_cpy_SU_DN_from_DN(&su_name, &info->name);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, su_name);
	if (!su) {
		*rc = AVND_ERR_NO_SU;
		goto err;
	}

	/* a fresh comp... */
	comp = calloc(1, sizeof(AVND_COMP));
	if (!comp) {
		*rc = AVND_ERR_NO_MEMORY;
		goto err;
	}

	/*
	 * Update the config parameters.
	 */
	/* update the comp-name (patricia key) */
	memcpy(&comp->name, &info->name, sizeof(SaNameT));

	/* update the component attributes */
	comp->inst_level = info->inst_level;

	comp->is_am_en = info->am_enable;

	switch (info->category) {
	case AVSV_COMP_TYPE_SA_AWARE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		break;

	case AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_PROXIED_LOCAL_NON_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_NON_SAF:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		break;

	default:
		break;
	}			/* switch */

	m_AVND_COMP_RESTART_EN_SET(comp, (info->comp_restart == TRUE) ? FALSE : TRUE);

	comp->cap = info->cap;
	comp->node_id = cb->node_info.nodeId;

	/* update CLC params */
	comp->clc_info.inst_retry_max = info->max_num_inst;
	comp->clc_info.am_start_retry_max = info->max_num_amstart;

	/* instantiate cmd params */
	memcpy(comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].cmd, info->init_info, info->init_len);
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].len = info->init_len;
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout = info->init_time;

	/* terminate cmd params */
	memcpy(comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].cmd, info->term_info, info->term_len);
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].len = info->term_len;
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].timeout = info->term_time;

	/* cleanup cmd params */
	memcpy(comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1].cmd, info->clean_info, info->clean_len);
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1].len = info->clean_len;
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1].timeout = info->clean_time;

	/* am-start cmd params */
	memcpy(comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1].cmd, info->amstart_info, info->amstart_len);
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1].len = info->amstart_len;
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1].timeout = info->amstart_time;

	/* am-stop cmd params */
	memcpy(comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1].cmd, info->amstop_info, info->amstop_len);
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1].len = info->amstop_len;
	comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1].timeout = info->amstop_time;

	/* update the callback response time out values */
	if (info->terminate_callback_timeout)
		comp->term_cbk_timeout = info->terminate_callback_timeout;
	else
		comp->term_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	if (info->csi_set_callback_timeout)
		comp->csi_set_cbk_timeout = info->csi_set_callback_timeout;
	else
		comp->csi_set_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	if (info->quiescing_complete_timeout)
		comp->quies_complete_cbk_timeout = info->quiescing_complete_timeout;
	else
		comp->quies_complete_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	if (info->csi_rmv_callback_timeout)
		comp->csi_rmv_cbk_timeout = info->csi_rmv_callback_timeout;
	else
		comp->csi_rmv_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	if (info->proxied_inst_callback_timeout)
		comp->pxied_inst_cbk_timeout = info->proxied_inst_callback_timeout;
	else
		comp->pxied_inst_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	if (info->proxied_clean_callback_timeout)
		comp->pxied_clean_cbk_timeout = info->proxied_clean_callback_timeout;
	else
		comp->pxied_clean_cbk_timeout = ((SaTimeT)AVND_COMP_CBK_RESP_TIME) * 1000000;

	/* update the default error recovery param */
	comp->err_info.def_rec = info->def_recvr;

	/*
	 * Update the rest of the parameters with default values.
	 */
	if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
	else
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);

	comp->avd_updt_flag = FALSE;

	/* synchronize comp oper state */
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, *rc);
	if (NCSCC_RC_SUCCESS != *rc)
		goto err;

	comp->pres = SA_AMF_PRESENCE_UNINSTANTIATED;

	/* create the association with hdl-mngr */
	if ((0 == (comp->comp_hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_AVND, (NCSCONTEXT)comp)))) {
		*rc = AVND_ERR_HDL;
		goto err;
	}

	/* 
	 * Initialize the comp-hc list.
	 */
	comp->hc_list.order = NCS_DBLIST_ANY_ORDER;
	comp->hc_list.cmp_cookie = avnd_dblist_hc_rec_cmp;
	comp->hc_list.free_cookie = 0;

	/* 
	 * Initialize the comp-csi list.
	 */
	comp->csi_list.order = NCS_DBLIST_ASSCEND_ORDER;
	comp->csi_list.cmp_cookie = avsv_dblist_saname_cmp;
	comp->csi_list.free_cookie = 0;

	/* 
	 * Initialize the pm list.
	 */
	avnd_pm_list_init(comp);

	/*
	 * initialize proxied list
	 */
	avnd_pxied_list_init(comp);

	/*
	 * Add to the patricia tree.
	 */
	comp->tree_node.bit = 0;
	comp->tree_node.key_info = (uns8 *)&comp->name;
	*rc = ncs_patricia_tree_add(&cb->compdb, &comp->tree_node);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_TREE;
		goto err;
	}

	/*
	 * Add to the comp-list (maintained by su)
	 */
	m_AVND_SUDB_REC_COMP_ADD(*su, *comp, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Update su bk ptr.
	 */
	comp->su = su;

	if (TRUE == su->su_is_external) {
		m_AVND_COMP_TYPE_SET_EXT_CLUSTER(comp);
	} else
		m_AVND_COMP_TYPE_SET_LOCAL_NODE(comp);

	m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_REC_ADD, AVND_LOG_COMP_DB_SUCCESS, &info->name, 0, NCSFL_SEV_NOTICE);
	avnd_hc_config_get(comp);
	return comp;

 err:
	if (AVND_ERR_DLL == *rc)
		ncs_patricia_tree_del(&cb->compdb, &comp->tree_node);

	if (comp) {
		if (comp->comp_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, comp->comp_hdl);

		free(comp);
	}

	m_AVND_LOG_COMP_DB(AVND_LOG_COMP_DB_REC_ADD, AVND_LOG_COMP_DB_FAILURE, &info->name, 0, NCSFL_SEV_CRITICAL);
	return 0;
}

/****************************************************************************
  Name          : avnd_compdb_rec_del
 
  Description   : This routine deletes a component record from the component 
                  database. 
 
  Arguments     : cb       - ptr to the AvND control block
                  name - ptr to the comp-name (in n/w order)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This routine expects a NULL comp-csi list.
******************************************************************************/
uns32 avnd_compdb_rec_del(AVND_CB *cb, SaNameT *name)
{
	AVND_COMP *comp;
	AVND_SU *su = 0;
	SaNameT su_name;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", name->value);

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, *name);
	if (!comp) {
		LOG_ER("%s: %s not found", __FUNCTION__, name->value);
		rc = AVND_ERR_NO_COMP;
		goto done;
	}

	/* comp should not be attached to any csi when it is being deleted */
	assert(comp->csi_list.n_nodes == 0);

	/*
	 * Determine if the SU is present
	 */
	/* extract the su-name from comp dn */
	memset(&su_name, 0, sizeof(SaNameT));
	avsv_cpy_SU_DN_from_DN(&su_name, name);
	su_name.length = su_name.length;

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, su_name);
	if (!su) {
		LOG_ER("%s: %s not found", __FUNCTION__, su_name.value);
		rc = AVND_ERR_NO_SU;
		goto done;
	}

	/* 
	 * Remove from the comp-list (maintained by su).
	 */
	rc = m_AVND_SUDB_REC_COMP_REM(*su, *comp);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("%s: %s remove failed", __FUNCTION__, name->value);
		rc = AVND_ERR_DLL;
		goto done;
	}

	/* 
	 * Remove from the patricia tree.
	 */
	rc = ncs_patricia_tree_del(&cb->compdb, &comp->tree_node);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("%s: %s tree del failed", __FUNCTION__, name->value);
		rc = AVND_ERR_TREE;
		goto done;
	}

	/* 
	 * Delete the various lists (hc, pm, pg, cbk etc) maintained by this comp.
	 */
	avnd_comp_hc_rec_del_all(cb, comp);
	avnd_comp_cbq_del(cb, comp, FALSE);
	avnd_comp_pm_rec_del_all(cb, comp);

	/* remove the association with hdl mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, comp->comp_hdl);

	/* comp should not be attached to any hc when it is being deleted */
	assert(comp->hc_list.n_nodes == 0);

	/* free the memory */
	free(comp);

done:
	if (rc == NCSCC_RC_SUCCESS)
		LOG_IN("Deleted '%s'", name->value);
	else
		LOG_ER("Delete of '%s' failed", name->value);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_compdb_csi_rec_get
 
  Description   : This routine gets the comp-csi relationship record from 
                  the csi-list (maintained on comp).
 
  Arguments     : cb            - ptr to AvND control block
                  comp_name - ptr to the comp-name (n/w order)
                  csi_name      - ptr to the CSI name
 
  Return Values : ptr to the comp-csi record (if any)
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_compdb_csi_rec_get(AVND_CB *cb, SaNameT *comp_name, SaNameT *csi_name)
{
	AVND_COMP_CSI_REC *csi_rec = 0;
	AVND_COMP *comp = 0;

	/* get the comp & csi records */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, *comp_name);
	if (comp)
		csi_rec = m_AVND_COMPDB_REC_CSI_GET(*comp, *csi_name);

	return csi_rec;
}

/****************************************************************************
  Name          : avnd_compdb_csi_rec_get_next
 
  Description   : This routine gets the next comp-csi relationship record from 
                  the csi-list (maintained on comp).
 
  Arguments     : cb            - ptr to AvND control block
                  comp_name - ptr to the comp-name (n/w order)
                  csi_name      - ptr to the CSI name
 
  Return Values : ptr to the comp-csi record (if any)
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_compdb_csi_rec_get_next(AVND_CB *cb, SaNameT *comp_name, SaNameT *csi_name)
{
	AVND_COMP_CSI_REC *csi = 0;
	AVND_COMP *comp = 0;

	/* get the comp  & the next csi */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, *comp_name);
	if (comp) {
		if (csi_name->length)
			for (csi = m_AVND_COMPDB_REC_CSI_GET_FIRST(*comp);
			     csi && !(m_CMP_HORDER_SANAMET(*csi_name, csi->name) < 0);
			     csi = m_AVND_COMPDB_REC_CSI_NEXT(*comp, *csi)) ;
		else
			csi = m_AVND_COMPDB_REC_CSI_GET_FIRST(*comp);
	}

	/* found the csi */
	if (csi)
		goto done;

	/* find the csi in the remaining comp recs */
	for (comp = m_AVND_COMPDB_REC_GET_NEXT(cb->compdb, *comp_name); comp;
	     comp = m_AVND_COMPDB_REC_GET_NEXT(cb->compdb, comp->name)) {
		csi = m_AVND_COMPDB_REC_CSI_GET_FIRST(*comp);
		if (csi)
			break;
	}

 done:
	return csi;
}

uns32 avnd_comp_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	uns32 rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("Op %u, %s", param->act, param->name.value);

	switch (param->act) {
	case AVSV_OBJ_OPR_MOD: {
			AVND_COMP *comp = 0;

			comp = m_AVND_COMPDB_REC_GET(cb->compdb, param->name);
			if (!comp) {
				LOG_ER("failed to get %s", param->name.value);
				goto done;
			}

			switch (param->attr_id) {
			case saAmfCompInstantiateCmd_ID:
			case saAmfCompTerminateCmd_ID:
			case saAmfCompCleanupCmd_ID:
			case saAmfCompAmStartCmd_ID:
			case saAmfCompAmStopCmd_ID:
				comp->config_is_valid = 0;
				break;
			case saAmfCompInstantiateTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout =
				    m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_TIMEOUT);
				break;

			case saAmfCompDelayBetweenInstantiateAttempts_ID:
				break;

			case saAmfCompTerminateTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].timeout =
				    m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_TERM_TIMEOUT);
				break;

			case saAmfCompCleanupTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1].timeout =
				    m_NCS_OS_NTOHLL_P(param->value);
				break;

			case saAmfCompAmStartTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1].timeout =
				    m_NCS_OS_NTOHLL_P(param->value);
				break;

			case saAmfCompAmStopTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1].timeout =
				    m_NCS_OS_NTOHLL_P(param->value);
				break;

			case saAmfCompTerminateCallbackTimeOut_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->term_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_TERM_CBK_TIMEOUT);
				break;

			case saAmfCompCSISetCallbackTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->csi_set_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_CSI_SET_CBK_TIMEOUT);
				break;

			case saAmfCompQuiescingCompleteTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->quies_complete_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_QUIES_CMPLT_CBK_TIMEOUT);
				break;

			case saAmfCompCSIRmvCallbackTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->csi_rmv_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_CSI_RMV_CBK_TIMEOUT);
				break;

			case saAmfCompProxiedCompInstantiateCallbackTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->pxied_inst_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_PXIED_INST_CBK_TIMEOUT);
				break;

			case saAmfCompProxiedCompCleanupCallbackTimeout_ID:
				assert(sizeof(SaTimeT) == param->value_len);
				comp->pxied_clean_cbk_timeout = m_NCS_OS_NTOHLL_P(param->value);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_PXIED_CLEAN_CBK_TIMEOUT);
				break;

			case saAmfCompNodeRebootCleanupFail_ID:
				break;

			case saAmfCompRecoveryOnError_ID:
				assert(sizeof(uns32) == param->value_len);
				comp->err_info.def_rec = m_NCS_OS_NTOHL(*(uns32 *)(param->value));
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_DEFAULT_RECVR);
				break;

			case saAmfCompNumMaxInstantiate_ID:
				assert(sizeof(uns32) == param->value_len);
				comp->clc_info.inst_retry_max =
				    m_NCS_OS_NTOHL(*(uns32 *)(param->value));
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp,
								 AVND_CKPT_COMP_INST_RETRY_MAX);
				break;

			case saAmfCompAMEnable_ID:
				assert(1 == param->value_len);
				comp->is_am_en = (NCS_BOOL)param->value[0];
				comp->clc_info.am_start_retry_cnt = 0;
				rc = avnd_comp_am_oper_req_process(cb, comp);
				break;

			case saAmfCompNumMaxInstantiateWithDelay_ID:
				break;
			case saAmfCompNumMaxAmStartAttempts_ID:
				break;
			case saAmfCompNumMaxAmStopAttempts_ID:
				break;
			case saAmfCompType_ID: {
				comp->saAmfCompType = param->name_sec;
				/* 
				** Indicate that comp config is no longer valid and have to be
				** refreshed from IMM. We cannot refresh here since it is probably
				** not yet in IMM.
				*/
				comp->config_is_valid = 0;
				LOG_NO("saAmfCompType changed to '%s' for '%s'",
					comp->saAmfCompType.value, comp->name.value);
				break;
			}
			default:
				LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
				goto done;
			}
		}
		break;

	case AVSV_OBJ_OPR_DEL:
		{
			AVND_COMP *comp = 0;

			/* get the record */
			comp = m_AVND_COMPDB_REC_GET(cb->compdb, param->name);
			if (comp) {
				/* delete the record */
				m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, comp, AVND_CKPT_COMP_CONFIG);
				rc = avnd_compdb_rec_del(cb, &param->name);
			}
		}
		break;

	default:
		LOG_NO("%s: Unsupported action %u", __FUNCTION__, param->act);
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}

static void avnd_comptype_delete(amf_comp_type_t *compt)
{
	int arg_counter;
	char *argv;

	if (!compt)
		return;

	free(compt->saAmfCtDefCmdEnv);
	free(compt->saAmfCtRelPathInstantiateCmd);

	/* Free saAmfCtDefInstantiateCmdArgv[i] before freeing saAmfCtDefInstantiateCmdArgv */
	arg_counter = 0;
	while ((argv = compt->saAmfCtDefInstantiateCmdArgv[arg_counter++]) != NULL)
		free(argv);
	free(compt->saAmfCtDefInstantiateCmdArgv);

	free(compt->saAmfCtRelPathTerminateCmd);

	/* Free saAmfCtDefTerminateCmdArgv[i] before freeing saAmfCtDefTerminateCmdArgv */
	arg_counter = 0;
	while ((argv = compt->saAmfCtDefTerminateCmdArgv[arg_counter++]) != NULL)
		free(argv);
	free(compt->saAmfCtDefTerminateCmdArgv);

	free(compt->saAmfCtRelPathCleanupCmd);
	/* Free saAmfCtDefCleanupCmdArgv[i] before freeing saAmfCtDefCleanupCmdArgv */
	arg_counter = 0;
	while ((argv = compt->saAmfCtDefCleanupCmdArgv[arg_counter++]) != NULL)
		free(argv);
	free(compt->saAmfCtDefCleanupCmdArgv);

	free(compt->saAmfCtRelPathAmStartCmd);
	/* Free saAmfCtDefAmStartCmdArgv[i] before freeing saAmfCtDefAmStartCmdArgv */
	arg_counter = 0;
	while ((argv = compt->saAmfCtDefAmStartCmdArgv[arg_counter++]) != NULL)
		free(argv);
	free(compt->saAmfCtDefAmStartCmdArgv);

	free(compt->saAmfCtRelPathAmStopCmd);
	/* Free saAmfCtDefAmStopCmdArgv[i] before freeing saAmfCtDefAmStopCmdArgv */
	arg_counter = 0;
	while ((argv = compt->saAmfCtDefAmStopCmdArgv[arg_counter++]) != NULL)
		free(argv);
	free(compt->saAmfCtDefAmStopCmdArgv);

	free(compt);
}

static amf_comp_type_t *avnd_comptype_create(const SaNameT *dn)
{
	SaImmAccessorHandleT accessorHandle;
	amf_comp_type_t *compt;
	int rc = -1, i;
	unsigned int j;
	const char *str;
	const SaImmAttrValuesT_2 **attributes;

	if ((compt = calloc(1, sizeof(amf_comp_type_t))) == NULL) {
		LOG_ER("%s: calloc FAILED for '%s'", __FUNCTION__, dn->value);
		LOG_ER("out of memory will exit now");
		exit(1);
	}

	(void)immutil_saImmOmAccessorInitialize(avnd_cb->immOmHandle, &accessorHandle);

	if (immutil_saImmOmAccessorGet_2(accessorHandle, dn, NULL, (SaImmAttrValuesT_2 ***)&attributes) != SA_AIS_OK) {
		LOG_ER("saImmOmAccessorGet_2 FAILED for '%s'", dn->value);
		goto done;
	}

	memcpy(compt->name.value, dn->value, dn->length);
	compt->name.length = dn->length;
	compt->tree_node.key_info = (uns8 *)&(compt->name);

	if (immutil_getAttr("saAmfCtCompCategory", attributes, 0, &compt->saAmfCtCompCategory) != SA_AIS_OK)
		assert(0);

	if (IS_COMP_LOCAL(compt->saAmfCtCompCategory) && 
	    immutil_getAttr("saAmfCtSwBundle", attributes, 0, &compt->saAmfCtSwBundle) != SA_AIS_OK) {
		assert(0);
		return NULL;	
	}

#if 0
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCmdEnv", 0)) != NULL)
		compt->saAmfCtDefCmdEnv = strdup(str);
#endif
	(void)immutil_getAttr("saAmfCtDefClcCliTimeout", attributes, 0, &compt->saAmfCtDefClcCliTimeout);

	(void)immutil_getAttr("saAmfCtDefCallbackTimeout", attributes, 0, &compt->saAmfCtDefCallbackTimeout);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathInstantiateCmd", 0)) != NULL)
		compt->saAmfCtRelPathInstantiateCmd = strdup(str);

	immutil_getAttrValuesNumber("saAmfCtDefInstantiateCmdArgv", attributes, &j);
	compt->saAmfCtDefInstantiateCmdArgv = calloc(j + 1, sizeof(SaStringT));
	assert(compt->saAmfCtDefInstantiateCmdArgv);
	for (i = 0; i < j; i++) {
		str = immutil_getStringAttr(attributes, "saAmfCtDefInstantiateCmdArgv", i);
		assert(str);
		compt->saAmfCtDefInstantiateCmdArgv[i] = strdup(str);
		assert(compt->saAmfCtDefInstantiateCmdArgv[i]);
	}

	if (immutil_getAttr("saAmfCtDefInstantiationLevel", attributes, 0, &compt->saAmfCtDefInstantiationLevel) != SA_AIS_OK)
		compt->saAmfCtDefInstantiationLevel = 0;

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathTerminateCmd", 0)) != NULL)
		compt->saAmfCtRelPathTerminateCmd = strdup(str);

	immutil_getAttrValuesNumber("saAmfCtDefTerminateCmdArgv", attributes, &j);
	compt->saAmfCtDefTerminateCmdArgv = calloc(j + 1, sizeof(SaStringT));
	assert(compt->saAmfCtDefTerminateCmdArgv);
	for (i = 0; i < j; i++) {
		str = immutil_getStringAttr(attributes, "saAmfCtDefTerminateCmdArgv", i);
		assert(str);
		compt->saAmfCtDefTerminateCmdArgv[i] = strdup(str);
		assert(compt->saAmfCtDefTerminateCmdArgv[i]);
	}

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathCleanupCmd", 0)) != NULL)
		compt->saAmfCtRelPathCleanupCmd = strdup(str);

	immutil_getAttrValuesNumber("saAmfCtDefCleanupCmdArgv", attributes, &j);
	compt->saAmfCtDefCleanupCmdArgv = calloc(j + 1, sizeof(SaStringT));
	assert(compt->saAmfCtDefCleanupCmdArgv);
	for (i = 0; i < j; i++) {
		str = immutil_getStringAttr(attributes, "saAmfCtDefCleanupCmdArgv", i);
		assert(str);
		compt->saAmfCtDefCleanupCmdArgv[i] = strdup(str);
		assert(compt->saAmfCtDefCleanupCmdArgv[i]);
	}

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStartCmd", 0)) != NULL)
		compt->saAmfCtRelPathAmStartCmd = strdup(str);

	immutil_getAttrValuesNumber("saAmfCtDefAmStartCmdArgv", attributes, &j);
	compt->saAmfCtDefAmStartCmdArgv = calloc(j + 1, sizeof(SaStringT));
	assert(compt->saAmfCtDefAmStartCmdArgv);
	for (i = 0; i < j; i++) {
		str = immutil_getStringAttr(attributes, "saAmfCtDefAmStartCmdArgv", i);
		assert(str);
		compt->saAmfCtDefAmStartCmdArgv[i] = strdup(str);
		assert(compt->saAmfCtDefAmStartCmdArgv[i]);
	}

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStopCmd", 0)) != NULL)
		compt->saAmfCtRelPathAmStopCmd = strdup(str);

	immutil_getAttrValuesNumber("saAmfCtDefAmStopCmdArgv", attributes, &j);
	compt->saAmfCtDefAmStopCmdArgv = calloc(j + 1, sizeof(SaStringT));
	assert(compt->saAmfCtDefAmStopCmdArgv);
	for (i = 0; i < j; i++) {
		str = immutil_getStringAttr(attributes, "saAmfCtDefAmStopCmdArgv", i);
		assert(str);
		compt->saAmfCtDefAmStopCmdArgv[i] = strdup(str);
		assert(compt->saAmfCtDefAmStopCmdArgv[i]);
	}

	(void)immutil_getAttr("saAmfCtDefQuiescingCompleteTimeout", attributes, 0,
			      &compt->saAmfCompQuiescingCompleteTimeout);

	if (immutil_getAttr("saAmfCtDefRecoveryOnError", attributes, 0, &compt->saAmfCtDefRecoveryOnError) != SA_AIS_OK)
		assert(0);

	if (immutil_getAttr("saAmfCtDefDisableRestart", attributes, 0, &compt->saAmfCtDefDisableRestart) != SA_AIS_OK)
		compt->saAmfCtDefDisableRestart = SA_FALSE;

	rc = 0;

 done:
	if (rc != 0) {
		avnd_comptype_delete(compt);
		compt = NULL;
	}

	(void)immutil_saImmOmAccessorFinalize(accessorHandle);

	return compt;
}

static void init_comp_category(AVND_COMP *comp, SaAmfCompCategoryT category)
{
	AVSV_COMP_TYPE_VAL comptype = avsv_amfcompcategory_to_avsvcomptype(category);

	assert(comptype != AVSV_COMP_TYPE_INVALID);

	switch (comptype) {
	case AVSV_COMP_TYPE_SA_AWARE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		break;

	case AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_PROXIED_LOCAL_NON_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_SAAWARE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PREINSTANTIABLE);
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_PROXIED);
		break;

	case AVSV_COMP_TYPE_NON_SAF:
		m_AVND_COMP_TYPE_SET(comp, AVND_COMP_TYPE_LOCAL);
		break;

	default:
		assert(0);
		break;
	}
}

static int get_string_attr_from_imm(SaImmOiHandleT immOmHandle, SaImmAttrNameT attrName, const SaNameT *dn, SaStringT *str)
{
	int rc = -1;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrNameT attributeNames[2] = {attrName, NULL};
	const char *s;
	SaAisErrorT error;

	immutil_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);

	if ((error = immutil_saImmOmAccessorGet_2(accessorHandle, dn, attributeNames, (SaImmAttrValuesT_2 ***)&attributes)) != SA_AIS_OK) {
		TRACE("saImmOmAccessorGet FAILED %u for %s", error, dn->value);
		goto done;
	}

	if ((s = immutil_getStringAttr(attributes, attrName, 0)) == NULL) {
		TRACE("Get %s FAILED for '%s'", attrName, dn->value);
		goto done;
	}

	*str = strdup(s);
	rc = 0;

done:
	immutil_saImmOmAccessorFinalize(accessorHandle);

	return rc;
}

/**
 * Initialize the bundle dependent attributes in comp. Especially the CLC-CLI commands.
 * @param comp
 * @param comptype
 * @param path_prefix
 * @param attributes
 */
static void init_bundle_dependent_attributes(AVND_COMP *comp, const amf_comp_type_t *comptype,
	const char *path_prefix, const SaImmAttrValuesT_2 **attributes)
{
	int i, j;
	AVND_COMP_CLC_CMD_PARAM *cmd;
	const char *argv;

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1];
	if (comptype->saAmfCtRelPathInstantiateCmd != NULL) {
		i = 0;
		i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, "%s/%s",
			path_prefix, comptype->saAmfCtRelPathInstantiateCmd);

		j = 0;
		while ((argv = comptype->saAmfCtDefInstantiateCmdArgv[j++]) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		j = 0;
		while ((argv = immutil_getStringAttr(attributes, "saAmfCompInstantiateCmdArgv", j++)) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		cmd->len = i;

		/* Check for truncation, should alloc these strings dynamically instead */
		assert((cmd->len > 0) && (cmd->len < sizeof(cmd->cmd)));
		TRACE("cmd=%s", cmd->cmd);
	}

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1];
	if (comptype->saAmfCtRelPathTerminateCmd != NULL) {
		i = 0;
		i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, "%s/%s",
			path_prefix, comptype->saAmfCtRelPathTerminateCmd);

		j = 0;
		while ((argv = comptype->saAmfCtDefTerminateCmdArgv[j++]) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		j = 0;
		while ((argv = immutil_getStringAttr(attributes, "saAmfCompTerminateCmdArgv", j++)) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		cmd->len = i;

		/* Check for truncation, should alloc these strings dynamically instead */
		assert((cmd->len > 0) && (cmd->len < sizeof(cmd->cmd)));
		TRACE("cmd=%s", cmd->cmd);
	}

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1];
	if (comptype->saAmfCtRelPathCleanupCmd != NULL) {
		i = 0;
		i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, "%s/%s",
			path_prefix, comptype->saAmfCtRelPathCleanupCmd);

		j = 0;
		while ((argv = comptype->saAmfCtDefCleanupCmdArgv[j++]) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		j = 0;
		while ((argv = immutil_getStringAttr(attributes, "saAmfCompCleanupCmdArgv", j++)) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		cmd->len = i;

		/* Check for truncation, should alloc these strings dynamically instead */
		assert((cmd->len > 0) && (cmd->len < sizeof(cmd->cmd)));
		TRACE("cmd=%s", cmd->cmd);
	}

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1];
	if (comptype->saAmfCtRelPathAmStartCmd != NULL) {
		i = 0;
		i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, "%s/%s",
			path_prefix, comptype->saAmfCtRelPathAmStartCmd);

		j = 0;
		while ((argv = comptype->saAmfCtDefAmStartCmdArgv[j++]) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		j = 0;
		while ((argv = immutil_getStringAttr(attributes, "saAmfCompAmStartCmdArgv", j++)) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		cmd->len = i;

		comp->is_am_en = TRUE;

		/* Check for truncation, should alloc these strings dynamically instead */
		assert((cmd->len > 0) && (cmd->len < sizeof(cmd->cmd)));
		TRACE("cmd=%s", cmd->cmd);
	}

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1];
	if (comptype->saAmfCtRelPathAmStopCmd != NULL) {
		i = 0;
		i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, "%s/%s",
			path_prefix, comptype->saAmfCtRelPathAmStopCmd);

		j = 0;
		while ((argv = comptype->saAmfCtDefAmStopCmdArgv[j++]) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		j = 0;
		while ((argv = immutil_getStringAttr(attributes, "saAmfCompAmStopCmdArgv", j++)) != NULL)
			i += snprintf(&cmd->cmd[i], sizeof(cmd->cmd) - i, " %s", argv);

		cmd->len = i;

		/* Check for truncation, should alloc these strings dynamically instead */
		assert((cmd->len > 0) && (cmd->len < sizeof(cmd->cmd)));
		TRACE("cmd=%s", cmd->cmd);
	}
}

/**
 * Initialize the members of the comp object with the configuration attributes from IMM.
 * 
 * @param comp
 * @param attributes
 * @param bundle_missing_is_error if true skip further initialization and return error code.
 * if false continue initialization to get most attributes correct.
 * 
 * @return int
 */
static int comp_init(AVND_COMP *comp, const SaImmAttrValuesT_2 **attributes,
	bool bundle_missing_is_error)
{
	int res = -1;
	AVND_COMP_CLC_CMD_PARAM *cmd;
	amf_comp_type_t *comptype;
	SaNameT nodeswbundle_name;
	SaBoolT disable_restart;
	char *path_prefix = NULL;

	TRACE_ENTER2("%s", comp->name.value);

	if ((comptype = avnd_comptype_create(&comp->saAmfCompType)) == NULL) {
		LOG_ER("%s: avnd_comptype_create FAILED for '%s'", __FUNCTION__,
			comp->saAmfCompType.value);
		goto done;
	}

	avsv_create_association_class_dn(&comptype->saAmfCtSwBundle,
		&avnd_cb->amf_nodeName, "safInstalledSwBundle", &nodeswbundle_name);

	res = get_string_attr_from_imm(avnd_cb->immOmHandle, "saAmfNodeSwBundlePathPrefix",
		&nodeswbundle_name, &path_prefix);

	if (res == 0) {
		init_bundle_dependent_attributes(comp, comptype, path_prefix, attributes);
	} else {
		if (bundle_missing_is_error) {
			LOG_NO("%s: '%s'", __FUNCTION__, comp->name.value);
			LOG_ER("%s: FAILED to read '%s'", __FUNCTION__, nodeswbundle_name.value);
			goto done;
		} else {
			TRACE("%s is missing, will refresh attributes later", nodeswbundle_name.value);
			/* Continue initialization to get the correct type set. Especially
			 * pre-instantiable or not is important when instantion request comes. */
		}
	}

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1];
	if (immutil_getAttr("saAmfCompInstantiateTimeout", attributes, 0, &cmd->timeout) != SA_AIS_OK) {
		cmd->timeout = comptype->saAmfCtDefClcCliTimeout;
	}

	if (immutil_getAttr("saAmfCompInstantiationLevel", attributes, 0, &comp->inst_level) != SA_AIS_OK)
		comp->inst_level = comptype->saAmfCtDefInstantiationLevel;

	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithoutDelay", attributes,
			    0, &comp->clc_info.inst_retry_max) != SA_AIS_OK)
		comp->clc_info.inst_retry_max = comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

#if 0
	//  TODO
	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithDelay", attributes,
			    0, &comp->max_num_inst_delay) != SA_AIS_OK)
		comp->comp_info.max_num_inst = comp_global_attrs.saAmfNumMaxInstantiateWithDelay;

	if (immutil_getAttr("saAmfCompDelayBetweenInstantiateAttempts", attributes,
			    0, &comp->inst_retry_delay) != SA_AIS_OK)
		comp->inst_retry_delay = comp_global_attrs.saAmfDelayBetweenInstantiateAttempts;
#endif

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1];
	if (immutil_getAttr("saAmfCompTerminateTimeout", attributes, 0, &cmd->timeout) != SA_AIS_OK)
		cmd->timeout = comptype->saAmfCtDefCallbackTimeout;

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_CLEANUP - 1];
	if (immutil_getAttr("saAmfCompCleanupTimeout", attributes, 0, &cmd->timeout) != SA_AIS_OK)
		cmd->timeout = comptype->saAmfCtDefClcCliTimeout;

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTART - 1];
	if (immutil_getAttr("saAmfCompAmStartTimeout", attributes, 0, &cmd->timeout) != SA_AIS_OK)
		cmd->timeout = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStartAttempts", attributes,
			    0, &comp->clc_info.am_start_retry_max) != SA_AIS_OK)
		comp->clc_info.am_start_retry_max = comp_global_attrs.saAmfNumMaxAmStartAttempts;

	cmd = &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_AMSTOP - 1];
	if (immutil_getAttr("saAmfCompAmStopTimeout", attributes, 0, &cmd->timeout) != SA_AIS_OK)
		cmd->timeout = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStopAttempts", attributes,
			    0, &comp->clc_info.saAmfCompNumMaxAmStopAttempts) != SA_AIS_OK)
		comp->clc_info.saAmfCompNumMaxAmStopAttempts = comp_global_attrs.saAmfNumMaxAmStopAttempts;

	if (immutil_getAttr("saAmfCompCSISetCallbackTimeout", attributes,
			    0, &comp->csi_set_cbk_timeout) != SA_AIS_OK)
		comp->csi_set_cbk_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompCSIRmvCallbackTimeout", attributes,
			    0, &comp->csi_rmv_cbk_timeout) != SA_AIS_OK)
		comp->csi_rmv_cbk_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompQuiescingCompleteTimeout", attributes,
			    0, &comp->quies_complete_cbk_timeout) != SA_AIS_OK)
		comp->quies_complete_cbk_timeout = comptype->saAmfCompQuiescingCompleteTimeout;

	if (immutil_getAttr("saAmfCompRecoveryOnError", attributes, 0, &comp->err_info.def_rec) != SA_AIS_OK)
		comp->err_info.def_rec = comptype->saAmfCtDefRecoveryOnError;

	if (immutil_getAttr("saAmfCompDisableRestart", attributes, 0, &disable_restart) != SA_AIS_OK)
		disable_restart = comptype->saAmfCtDefDisableRestart;

	comp->is_restart_en = (disable_restart == SA_TRUE) ? FALSE : TRUE;

	init_comp_category(comp, comptype->saAmfCtCompCategory);

	if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
	else
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);

	/* if we are missing path_prefix we need to refresh the config later */
	if (path_prefix != NULL)
		comp->config_is_valid = 1;

	res = 0;

done:
	free(path_prefix);
	avnd_comptype_delete(comptype);
	TRACE_LEAVE();
	return res;
}

/**
 * Create an avnd component object.
 * Validation has been done by avd => simple error handling (asserts).
 * Comp type argv and comp argv augments each other.
 * @param comp_name
 * @param attributes
 * @param su
 * 
 * @return AVND_COMP*
 */
static AVND_COMP *avnd_comp_create(const SaNameT *comp_name, const SaImmAttrValuesT_2 **attributes, AVND_SU *su)
{
	int rc = -1;
	AVND_COMP *comp;
	SaAisErrorT error;

	TRACE_ENTER2("%s", comp_name->value);

	if ((comp = calloc(1, sizeof(*comp))) == NULL) {
		LOG_ER("%s: calloc FAILED for '%s'", __FUNCTION__, comp_name->value);
		LOG_ER("out of memory will exit now");
		exit(1);
	}

	memcpy(&comp->name, comp_name, sizeof(comp->name));
	comp->name.length = comp_name->length;

	error = immutil_getAttr("saAmfCompType", attributes, 0, &comp->saAmfCompType);
	assert(error == SA_AIS_OK);

	if (comp_init(comp, attributes, false) != 0)
		goto done;

	/* create the association with hdl-mngr */
	comp->comp_hdl = ncshm_create_hdl(avnd_cb->pool_id, NCS_SERVICE_ID_AVND, comp);
	if (0 == comp->comp_hdl) {
		LOG_ER("%s: ncshm_create_hdl FAILED for '%s'", __FUNCTION__, comp_name->value);
		goto done;
	}

	comp->avd_updt_flag = FALSE;

	/* synchronize comp oper state */
	m_AVND_COMP_OPER_STATE_AVD_SYNC(avnd_cb, comp, rc);

//	comp->cap = info->cap;
	comp->node_id = avnd_cb->node_info.nodeId;
	comp->pres = SA_AMF_PRESENCE_UNINSTANTIATED;

	/* Initialize the comp-hc list. */
	comp->hc_list.order = NCS_DBLIST_ANY_ORDER;
	comp->hc_list.cmp_cookie = avnd_dblist_hc_rec_cmp;
	comp->hc_list.free_cookie = 0;

	/* Initialize the comp-csi list. */
	comp->csi_list.order = NCS_DBLIST_ASSCEND_ORDER;
	comp->csi_list.cmp_cookie = avsv_dblist_saname_cmp;
	comp->csi_list.free_cookie = 0;

	avnd_pm_list_init(comp);

	/* initialize proxied list */
	avnd_pxied_list_init(comp);

	/* Add to the patricia tree. */
	comp->tree_node.key_info = (uns8 *)&comp->name;
	if(ncs_patricia_tree_add(&avnd_cb->compdb, &comp->tree_node) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_patricia_tree_add FAILED for '%s'", comp_name->value);
		goto done;
	}

	/* Add to the comp-list (maintained by su) */
	m_AVND_SUDB_REC_COMP_ADD(*su, *comp, rc);

	comp->su = su;

	if (TRUE == su->su_is_external) {
		m_AVND_COMP_TYPE_SET_EXT_CLUSTER(comp);
	} else
		m_AVND_COMP_TYPE_SET_LOCAL_NODE(comp);

	m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(avnd_cb, comp, AVND_CKPT_COMP_CONFIG);

	/* determine if su is pre-instantiable */
	if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		m_AVND_SU_PREINSTANTIABLE_SET(comp->su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(avnd_cb, comp->su, AVND_CKPT_SU_FLAG_CHANGE);
//			m_AVND_SU_OPER_STATE_SET_AND_SEND_NTF(avnd_cb, comp->su, SA_AMF_OPERATIONAL_DISABLED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(avnd_cb, comp->su, AVND_CKPT_SU_OPER_STATE);
	}

	/* determine if su is restart capable */
	if (m_AVND_COMP_IS_RESTART_DIS(comp)) {
		m_AVND_SU_RESTART_DIS_SET(comp->su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(avnd_cb, comp->su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	rc = 0;
done:
	if (rc != 0) {
		/* remove the association with hdl mngr */
		ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, comp->comp_hdl);
		free(comp);
		comp = NULL;
	}
	return comp;
}

/**
 * Get configuration for all AMF Comp objects related to the 
 * specified SU from IMM and create internal objects. 
 * 
 * @param su 
 * 
 * @return SaAisErrorT 
 */
unsigned int avnd_comp_config_get_su(AVND_SU *su)
{
	unsigned int rc = NCSCC_RC_FAILURE;
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT comp_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfComp";
	AVND_COMP *comp;

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((error = immutil_saImmOmSearchInitialize_2(avnd_cb->immOmHandle, &su->name,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR,
		&searchParam, compConfigAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &comp_name,
		(SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		avnd_log(NCSFL_SEV_NOTICE, "'%s'", comp_name.value);
		if(0 == m_AVND_COMPDB_REC_GET(avnd_cb->compdb, comp_name)) {
			if ((comp = avnd_comp_create(&comp_name, attributes, su)) == NULL)
				goto done2;

			avnd_hc_config_get(comp);
		}
	}

	rc = NCSCC_RC_SUCCESS;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	return rc;
}

/**
 * Reinitialize a comp object with configuration data from IMM.
 * 
 * @param comp
 * 
 * @return int
 */
int avnd_comp_config_reinit(AVND_COMP *comp)
{
	int res = -1;
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;

	/*
	** If the component configuration is not valid (e.g. comptype has been
	** changed by an SMF upgrade), refresh it from IMM.
	** At first time instantiation of OpenSAF components we cannot go
	** to IMM since we would deadloack.
	*/
	if (comp->config_is_valid)
		return 0;

	TRACE_ENTER2("%s", comp->name.value);

	(void)immutil_saImmOmAccessorInitialize(avnd_cb->immOmHandle, &accessorHandle);

	if (immutil_saImmOmAccessorGet_2(accessorHandle, &comp->name, NULL,
		(SaImmAttrValuesT_2 ***)&attributes) != SA_AIS_OK) {

		LOG_ER("saImmOmAccessorGet_2 FAILED for '%s'", comp->name.value);
		goto done;
	}

	res = comp_init(comp, attributes, true);
	if (res == 0)
		TRACE("'%s' configuration reread from IMM", comp->name.value);

	(void)immutil_saImmOmAccessorFinalize(accessorHandle);

	/* need to get HC type configuration also if that has been recently created */
	avnd_hctype_config_get(&comp->saAmfCompType);

done:
	TRACE_LEAVE();
	return res;
}

