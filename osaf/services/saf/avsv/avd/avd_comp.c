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
 *            Ericsson
 *
 */

/*****************************************************************************

  DESCRIPTION:This module deals with the creation, accessing and deletion of
  the component database on the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>
#include <avsv_util.h>
#include <avd_util.h>
#include <avd_comp.h>
#include <avd_imm.h>
#include <avd_node.h>
#include <avd_csi.h>
#include <avd_proc.h>
#include <avd_ckpt_msg.h>

static NCS_PATRICIA_TREE comp_db;

void avd_comp_db_add(AVD_COMP *comp)
{
	unsigned int rc;

	if (avd_comp_get(&comp->comp_info.name) == NULL) {
		rc = ncs_patricia_tree_add(&comp_db, &comp->tree_node);
		assert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_COMP *avd_comp_new(const SaNameT *dn)
{
	AVD_COMP *comp;

	if ((comp = calloc(1, sizeof(AVD_COMP))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}
	
	memcpy(comp->comp_info.name.value, dn->value, dn->length);
	comp->comp_info.name.length = dn->length;
	comp->tree_node.key_info = (uns8 *)&(comp->comp_info.name);
	comp->comp_info.cap = SA_AMF_COMP_ONE_ACTIVE_OR_ONE_STANDBY;
	comp->comp_info.category = AVSV_COMP_TYPE_NON_SAF;
	comp->comp_info.def_recvr = SA_AMF_COMPONENT_RESTART;
	comp->comp_info.inst_level = 1;
	comp->comp_info.comp_restart = TRUE;
	comp->nodefail_cleanfail = FALSE;
	comp->saAmfCompOperState = SA_AMF_OPERATIONAL_DISABLED;
	comp->saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;
	comp->saAmfCompPresenceState = SA_AMF_PRESENCE_UNINSTANTIATED;

	return comp;
}

void avd_comp_pres_state_set(AVD_COMP *comp, SaAmfPresenceStateT pres_state)
{
	assert(pres_state <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value, avd_pres_state_name[comp->saAmfCompPresenceState],
		avd_pres_state_name[pres_state]);

	comp->saAmfCompPresenceState = pres_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		"saAmfCompPresenceState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_PRES_STATE);

	/* alarm & notifications */
	if(comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
		avd_send_comp_inst_failed_alarm(&comp->comp_info.name, &comp->su->su_on_node->name);
	else if(comp->saAmfCompPresenceState ==  SA_AMF_PRESENCE_TERMINATION_FAILED)
		avd_send_comp_clean_failed_alarm(&comp->comp_info.name, &comp->su->su_on_node->name);
}

void avd_comp_oper_state_set(AVD_COMP *comp, SaAmfOperationalStateT oper_state)
{
	assert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value, avd_oper_state_name[comp->saAmfCompOperState], avd_oper_state_name[oper_state]);

	comp->saAmfCompOperState = oper_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		"saAmfCompOperState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_OPER_STATE);
}

void avd_comp_readiness_state_set(AVD_COMP *comp, SaAmfReadinessStateT readiness_state)
{
	if (comp->saAmfCompReadinessState == readiness_state)
		return;

	assert(readiness_state <= SA_AMF_READINESS_STOPPING);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value,
		avd_readiness_state_name[comp->saAmfCompReadinessState], avd_readiness_state_name[readiness_state]);
	comp->saAmfCompReadinessState = readiness_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		 "saAmfCompReadinessState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_READINESS_STATE);
}

void avd_comp_proxy_status_change(AVD_COMP *comp, SaAmfProxyStatusT proxy_status)
{
	assert(proxy_status <= SA_AMF_PROXY_STATUS_PROXIED);
	TRACE_ENTER2("'%s' ProxyStatus is now %s", comp->comp_info.name.value, avd_proxy_status_name[proxy_status]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s ProxyStatus is now %s", 
			comp->comp_info.name.value, avd_proxy_status_name[proxy_status]);

	/* alarm & notifications */
	if(proxy_status == SA_AMF_PROXY_STATUS_UNPROXIED)
		avd_send_comp_proxy_status_unproxied_alarm(&comp->comp_info.name);
	else if(proxy_status == SA_AMF_PROXY_STATUS_PROXIED)
		avd_send_comp_proxy_status_proxied_ntf(&comp->comp_info.name, 
		                                       SA_AMF_PROXY_STATUS_UNPROXIED, 
		                                       SA_AMF_PROXY_STATUS_PROXIED);

}

AVD_COMP *avd_comp_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP *)ncs_patricia_tree_get(&comp_db, (uns8 *)&tmp);
}

AVD_COMP *avd_comp_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP *)ncs_patricia_tree_getnext(&comp_db, (uns8 *)&tmp);
}

void avd_comp_delete(AVD_COMP *comp)
{
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
	avd_su_remove_comp(comp);
	avd_comptype_remove_comp(comp);
	(void)ncs_patricia_tree_del(&comp_db, &comp->tree_node);
	free(comp);
}

/*****************************************************************************
 * Function: avd_comp_ack_msg
 *
 * Purpose:  This function processes the acknowledgment received for
 *          a non proxied component or proxied component related message 
 *          from AVND for operator related changes. If the message
 *          acknowledges success nothing to be done. If failure is
 *          reported a error is logged.
 *
 * Input: cb - the AVD control block
 *        ack_msg - The acknowledgement message received from the AVND.
 *
 * Returns: none.
 *
 * NOTES:
 *
 * 
 **************************************************************************/
void avd_comp_ack_msg(AVD_CL_CB *cb, AVD_DND_MSG *ack_msg)
{
	AVD_COMP *comp, *i_comp;
	AVSV_PARAM_INFO param;
	AVD_AVND *avnd;
	uns32 min_si = 0;
	NCS_BOOL isPre;
	AVD_AVND *su_node_ptr = NULL;

	/* check the ack message for errors. If error find the component that
	 * has error.
	 */

	if (ack_msg->msg_info.n2d_reg_comp.error != NCSCC_RC_SUCCESS) {
		/* Find the component */
		comp = avd_comp_get(&ack_msg->msg_info.n2d_reg_comp.comp_name);
		if (comp == NULL) {
			/* The component has already been deleted. there is nothing
			 * that can be done.
			 */

			/* Log an information error that the component is
			 * deleted.
			 */
			return;
		}

		/* log an fatal error as normally this shouldnt happen.
		 */
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ack_msg->msg_info.n2d_reg_comp.error);

		/* Make the row status as not in service to indicate that. It is
		 * not active.
		 */

		/* verify if the max ACTIVE and STANDBY SIs of the SU 
		 ** need to be changed
		 */
		if (comp->max_num_csi_actv == comp->su->si_max_active) {
			/* find the number and set it */
			min_si = 0;
			i_comp = comp->su->list_of_comp;
			while (i_comp) {
				if (i_comp != comp) {
					if (min_si > i_comp->max_num_csi_actv)
						min_si = i_comp->max_num_csi_actv;
					else if (min_si == 0)
						min_si = i_comp->max_num_csi_actv;
				}
				i_comp = i_comp->su_comp_next;
			}
			/* Now we have the min value. set it */
			comp->su->si_max_active = min_si;
		}

		/* FOR STANDBY count */
		if (comp->max_num_csi_stdby == comp->su->si_max_standby) {
			/* find the number and set it */
			min_si = 0;
			i_comp = comp->su->list_of_comp;
			while (i_comp) {
				if (i_comp != comp) {
					if (min_si > i_comp->max_num_csi_stdby)
						min_si = i_comp->max_num_csi_stdby;
					else if (min_si == 0)
						min_si = i_comp->max_num_csi_stdby;
				}
				i_comp = i_comp->su_comp_next;
			}
			/* Now we have the min value. set it */
			comp->su->si_max_standby = min_si;
		}

		/* Verify if the SUs preinstan value need to be changed */
		if ((AVSV_COMP_TYPE_SA_AWARE == comp->comp_info.category) ||
		    (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == comp->comp_info.category) ||
		    (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == comp->comp_info.category)) {
			isPre = FALSE;
			i_comp = comp->su->list_of_comp;
			while (i_comp) {
				if (((AVSV_COMP_TYPE_SA_AWARE == i_comp->comp_info.category) ||
				     (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == i_comp->comp_info.category) ||
				     (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == i_comp->comp_info.category)) &&
				    (i_comp != comp)) {
					isPre = TRUE;
					break;
				}
				i_comp = i_comp->su_comp_next;
			}	/* end while */

			if (isPre == TRUE) {
				comp->su->saAmfSUPreInstantiable = TRUE;
			} else {
				comp->su->saAmfSUPreInstantiable = FALSE;
			}
		}

		if (comp->su->curr_num_comp == 1) {
			/* This comp will be deleted so revert these to def val */
			comp->su->si_max_active = 0;
			comp->su->si_max_standby = 0;
			comp->su->saAmfSUPreInstantiable = TRUE;
		}

		/* Set runtime cached attributes. */
		avd_saImmOiRtObjectUpdate(&comp->su->name,
				"saAmfSUPreInstantiable", SA_IMM_ATTR_SAUINT32T,
				&comp->su->saAmfSUPreInstantiable);

		/* decrement the active component number of this SU */
		comp->su->curr_num_comp--;

		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, (comp->su), AVSV_CKPT_AVD_SU_CONFIG);

		avd_su_remove_comp(comp);

		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
		return;
	}

	/* Log an information error that the node was updated with the
	 * information of the component.
	 */
	/* Find the component */
	comp = avd_comp_get(&ack_msg->msg_info.n2d_reg_comp.comp_name);
	if (comp == NULL) {
		/* The comp has already been deleted. there is nothing
		 * that can be done.
		 */

		/* Log an information error that the comp is
		 * deleted.
		 */
		/* send a delete message to the AvND for the comp. */
		memset(((uns8 *)&param), '\0', sizeof(AVSV_PARAM_INFO));
		param.act = AVSV_OBJ_OPR_DEL;
		param.name = ack_msg->msg_info.n2d_reg_comp.comp_name;
		param.class_id = AVSV_SA_AMF_COMP;
		avnd = avd_node_find_nodeid(ack_msg->msg_info.n2d_reg_comp.node_id);
		avd_snd_op_req_msg(cb, avnd, &param);
		return;
	}

	if (comp->su->curr_num_comp == comp->su->num_of_comp) {
		if (comp->su->saAmfSUPreInstantiable == TRUE) {
			avd_sg_app_su_inst_func(cb, comp->su->sg_of_su);
		} else {
			comp->saAmfCompOperState = SA_AMF_OPERATIONAL_ENABLED;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVSV_CKPT_COMP_OPER_STATE);
			avd_su_pres_state_set(comp->su, SA_AMF_OPERATIONAL_ENABLED);

			m_AVD_GET_SU_NODE_PTR(cb, comp->su, su_node_ptr);

			if (m_AVD_APP_SU_IS_INSVC(comp->su, su_node_ptr)) {
				avd_su_readiness_state_set(comp->su, SA_AMF_READINESS_IN_SERVICE);
				switch (comp->su->sg_of_su->sg_redundancy_model) {
				case SA_AMF_2N_REDUNDANCY_MODEL:
					avd_sg_2n_su_insvc_func(cb, comp->su);
					break;

				case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
					avd_sg_nacvred_su_insvc_func(cb, comp->su);
					break;

				case SA_AMF_N_WAY_REDUNDANCY_MODEL:
					avd_sg_nway_su_insvc_func(cb, comp->su);
					break;

				case SA_AMF_NPM_REDUNDANCY_MODEL:
					avd_sg_npm_su_insvc_func(cb, comp->su);
					break;

				case SA_AMF_NO_REDUNDANCY_MODEL:
				default:
					avd_sg_nored_su_insvc_func(cb, comp->su);
					break;
				}
			}
		}
	}
}

/**
 * Add comp to model
 * @param comp
 */
static void comp_add_to_model(AVD_COMP *comp)
{
	SaNameT dn;
	AVD_COMP *i_comp = NULL;
	AVD_AVND *su_node_ptr = NULL;
	NCS_BOOL isPre;

	TRACE_ENTER2("%s", comp->comp_info.name.value);

	/* Check parent link to see if it has been added already */
	if (comp->su != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&comp->comp_info.name, &dn, "safSu");
	comp->su = avd_su_get(&dn);

	avd_comp_db_add(comp);
	comp->comp_type = avd_comptype_get(&comp->saAmfCompType);
	assert(comp->comp_type);
	avd_comptype_add_comp(comp);
	avd_su_add_comp(comp);

	/* check if the
	 * corresponding node is UP send the component information
	 * to the Node.
	 */
	if (FALSE == comp->su->su_is_external) {
		su_node_ptr = comp->su->su_on_node;
	} else {
		/* This is an external SU, so there is no node assigned to it.
		   For some purpose of validations and sending SU/Comps info to
		   hosting node (Controllers), we can take use of the hosting
		   node. */
		if ((AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE ==
		     comp->comp_info.category) ||
		    (AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE == comp->comp_info.category)) {
			/* This is a valid external component. Ext comp is in ext
			   SU. */
		} else {
			/* This is not a valid external component. External SU has 
			   been assigned a cluster component. */
			comp->su->curr_num_comp--;
			avd_su_remove_comp(comp);
			LOG_ER("Not A Valid External Component: Category '%u'configured",
				comp->comp_info.category);
			return;
		}
		su_node_ptr = avd_cb->ext_comp_info.local_avnd_node;
	}			/* Else of if(FALSE == comp->su->su_is_external). */
#if 0
	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		if (avd_snd_comp_msg(avd_cb, comp) != NCSCC_RC_SUCCESS) {
			/* the SU will never get to readiness state in service */
			/* Log an internal error */
			comp->su->curr_num_comp--;
			avd_su_remove_comp(comp);
			LOG_ER("Sending Comp Info to AvND failed: '%s'",
				comp->comp_info.name.value);
			return;
		}
	}
#endif
	/* Verify if the SUs preinstan value need to be changed */
	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE)) {
		comp->su->saAmfSUPreInstantiable = TRUE;
	} else {
		isPre = FALSE;
		i_comp = comp->su->list_of_comp;
		while (i_comp) {
			if ((i_comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) ||
			    (i_comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
			    (i_comp->comp_info.category == AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE)) {
				isPre = TRUE;
				break;
			}
			i_comp = i_comp->su_comp_next;
		}
		if (isPre == FALSE) {
			comp->su->saAmfSUPreInstantiable = FALSE;
		}
		comp->max_num_csi_actv = 1;
		comp->max_num_csi_stdby = 1;
	}

	if ((comp->max_num_csi_actv < comp->su->si_max_active) || (comp->su->si_max_active == 0)) {
		comp->su->si_max_active = comp->max_num_csi_actv;
	}

	if ((comp->max_num_csi_stdby < comp->su->si_max_standby) || (comp->su->si_max_standby == 0)) {
		comp->su->si_max_standby = comp->max_num_csi_stdby;
	}

	/* Set runtime cached attributes. */
	avd_saImmOiRtObjectUpdate(&comp->su->name, "saAmfSUPreInstantiable",
		SA_IMM_ATTR_SAUINT32T, &comp->su->saAmfSUPreInstantiable);

	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompReadinessState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);

	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompOperState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);
	
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompPresenceState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);

done:
	TRACE_LEAVE();
}

/**
 * Validate configuration attributes for an AMF Comp object
 * @param comp
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	char *parent;
	SaUint32T value;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safSu=", 6) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfCompType", attributes, 0, &aname);
	assert(rc == SA_AIS_OK);

	if (avd_comptype_get(&aname) == NULL) {
		/* Comp type does not exist in current model, check CCB */
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	rc = immutil_getAttr("saAmfCompRecoveryOnError", attributes, 0, &value);
	if (rc == SA_AIS_OK) {
		if ((value < SA_AMF_NO_RECOMMENDATION) || (value > SA_AMF_NODE_FAILFAST)) {
			LOG_ER("Illegal/unsupported saAmfCompRecoveryOnError value %u for '%s'",
				   value, dn->value);
			return 0;
		}

		if (value == SA_AMF_NO_RECOMMENDATION)
			LOG_NO("Invalid configuration, saAmfCompRecoveryOnError=NO_RECOMMENDATION(%u) for '%s'",
				   value, dn->value);
	}

	rc = immutil_getAttr("saAmfCompDisableRestart", attributes, 0, &value);
	if ((rc == SA_AIS_OK) && (value > SA_TRUE)) {
		LOG_ER("Illegal saAmfCompDisableRestart value %u for '%s'",
			   value, dn->value);
		return 0;
	}

#if 0
	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) && (comp->comp_info.init_len == 0)) {
		LOG_ER("Sa Aware Component: instantiation command not configured");
		return -1;
	} else if ((comp->comp_info.category == AVSV_COMP_TYPE_NON_SAF) &&
		   ((comp->comp_info.init_len == 0) || (comp->comp_info.term_len == 0))) {
		LOG_ER("Non-SaAware Component: instantiation or termination command not configured");
		return -1;
	}

	if ((AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE == comp->comp_info.category)) {
		if ((comp->comp_info.init_len == 0) ||
		    (comp->comp_info.term_len == 0) || (comp->comp_info.clean_len == 0)) {
			/* For external component, the following fields should not be 
			   filled. */
		} else {
			LOG_ER("External Component: instantiation or termination not configured");
			return -1;
		}
	} else {
		if (comp->comp_info.clean_len == 0) {
			LOG_ER("Cluster Component: Cleanup script not configured");
			return -1;
		}

		if (comp->comp_info.max_num_inst == 0) {
			LOG_ER("Cluster Component: Max num inst not configured");
			return -1;
		}
	}

	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_NON_PRE_INSTANTIABLE)) {

		if (comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE_OR_Y_STANDBY) {
			comp->max_num_csi_actv = 1;
		} else if ((comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE_OR_1_STANDBY) ||
			   (comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE)) {
			comp->max_num_csi_actv = 1;
			comp->max_num_csi_stdby = 1;
		} else if (comp->comp_info.cap == NCS_COMP_CAPABILITY_X_ACTIVE) {
			comp->max_num_csi_stdby = comp->max_num_csi_actv;
		}

		if ((comp->max_num_csi_actv == 0) || (comp->max_num_csi_stdby == 0)) {
			LOG_ER("Max Act Csi or Max Stdby Csi not configured");
			return -1;
		}
	}

	sg_red_model = su->sg_of_su->sg_redundancy_model;

	/* Check illegal component capability/category wrt SG red model */
	if (((sg_red_model == SA_AMF_N_WAY_REDUNDANCY_MODEL) &&
	     ((comp->comp_info.cap != NCS_COMP_CAPABILITY_X_ACTIVE_AND_Y_STANDBY) ||
	      (comp->comp_info.category == AVSV_COMP_TYPE_NON_SAF)))) {
		LOG_ER("Illegal category %u or cap %u for SG red model %u",
			comp->comp_info.category, comp->comp_info.cap, sg_red_model);
		return -1;
	}

	/* Check illegal component capability wrt SG red model */
	if ((sg_red_model == SA_AMF_NPM_REDUNDANCY_MODEL) &&
	    (comp->comp_info.cap != NCS_COMP_CAPABILITY_1_ACTIVE_OR_1_STANDBY)) {
		LOG_ER("Illegal capability %u for SG red model %u",
			comp->comp_info.cap, sg_red_model);
		return -1;
	}

	/* Verify that the SU can contain this component */
	{
		AVD_SUTCOMP_TYPE *sutcomptype;
		SaNameT sutcomptype_name;

		avd_create_association_class_dn(&comp->saAmfCompType, &su->saAmfSUType,
			"safMemberCompType", &sutcomptype_name);
		sutcomptype = avd_sutcomptype_get(&sutcomptype_name);
		if (sutcomptype == NULL) {
			LOG_ER("Not found '%s'", sutcomptype_name.value);
			return -1;
		}

		if (sutcomptype->curr_num_components == sutcomptype->saAmfSutMaxNumComponents) {
			LOG_ER("SU '%s' cannot contain more components of this type '%s*",
				su->name.value, comp->saAmfCompType.value);
			return -1;
		}
	}

	if (TRUE == su->su_is_external) {
		if ((TRUE == comp->comp_info.am_enable) ||
		    (0 != comp->comp_info.amstart_len) || (0 != comp->comp_info.amstop_len)) {
			LOG_ER("External Component: Active monitoring configured");
			return -1;
		} else {
			/* There are default values assigned to amstart_time, 
			   amstop_time and clean_time. Since these values are not 
			   used for external components, so we will reset it. */
			comp->comp_info.amstart_time = 0;
			comp->comp_info.amstop_time = 0;
			comp->comp_info.clean_time = 0;
			comp->comp_info.max_num_amstart = 0;
		}
	}
#endif
	return 1;
}

static AVD_COMP *comp_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_COMP *comp;
	char *cmd_argv;
	const char *str;
	const AVD_COMP_TYPE *comptype;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if (NULL == (comp = avd_comp_get(dn))) {
		if ((comp = avd_comp_new(dn)) == NULL)
			goto done;
	}
	else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr("saAmfCompType", attributes, 0, &comp->saAmfCompType);
	assert(error == SA_AIS_OK);

	if ((comptype = avd_comptype_get(&comp->saAmfCompType)) == NULL) {
		LOG_ER("saAmfCompType '%s' does not exist", comp->saAmfCompType.value);
		goto done;
	}

	/*  TODO clean this up! */
	comp->comp_info.category = avsv_amfcompcategory_to_avsvcomptype(comptype->saAmfCtCompCategory);
	if (comp->comp_info.category == AVSV_COMP_TYPE_INVALID) {
		LOG_ER("Comp category %x invalid for '%s'", comp->comp_info.category, comp->saAmfCompType.value);
		goto done;
	}

	if (strlen(comptype->saAmfCtRelPathInstantiateCmd) > 0) {
		strcpy(comp->comp_info.init_info, comptype->saAmfCtRelPathInstantiateCmd);
		cmd_argv = comp->comp_info.init_info + strlen(comp->comp_info.init_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompInstantiateCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefInstantiateCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.init_len = strlen(comp->comp_info.init_info);
	}

	if (immutil_getAttr("saAmfCompInstantiateTimeout", attributes, 0, &comp->comp_info.init_time) != SA_AIS_OK)
		comp->comp_info.init_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompInstantiationLevel", attributes, 0, &comp->comp_info.inst_level) != SA_AIS_OK)
		comp->comp_info.inst_level = comptype->saAmfCtDefInstantiationLevel;

	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithoutDelay", attributes,
			    0, &comp->comp_info.max_num_inst) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

	/*  TODO what is implemented? With or without delay? */

#if 0
	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithDelay", attributes,
			    0, &comp->max_num_inst_delay) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay;
#endif

	if (immutil_getAttr("saAmfCompDelayBetweenInstantiateAttempts", attributes,
			    0, &comp->inst_retry_delay) != SA_AIS_OK)
		comp->inst_retry_delay = avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts;

	if (strlen(comptype->saAmfCtRelPathTerminateCmd) > 0) {
		strcpy(comp->comp_info.term_info, comptype->saAmfCtRelPathTerminateCmd);
		cmd_argv = comp->comp_info.term_info + strlen(comp->comp_info.term_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompTerminateCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefTerminateCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.term_len = strlen(comp->comp_info.term_info);
	}

	if (immutil_getAttr("saAmfCompTerminateTimeout", attributes,
			    0, &comp->comp_info.terminate_callback_timeout) != SA_AIS_OK)
		comp->comp_info.terminate_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (strlen(comptype->saAmfCtRelPathCleanupCmd) > 0) {
		strcpy(comp->comp_info.clean_info, comptype->saAmfCtRelPathCleanupCmd);
		cmd_argv = comp->comp_info.clean_info + strlen(comp->comp_info.clean_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompCleanupCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefCleanupCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.clean_len = strlen(comp->comp_info.clean_info);
	}

	if (immutil_getAttr("saAmfCompCleanupTimeout", attributes, 0, &comp->comp_info.clean_time) != SA_AIS_OK)
		comp->comp_info.clean_time = comptype->saAmfCtDefClcCliTimeout;

	if (strlen(comptype->saAmfCtRelPathAmStartCmd) > 0) {
		strcpy(comp->comp_info.amstart_info, comptype->saAmfCtRelPathAmStartCmd);
		cmd_argv = comp->comp_info.amstart_info + strlen(comp->comp_info.amstart_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompAmStartCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefAmStartCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.amstart_len = strlen(comp->comp_info.amstart_info);
	}

	if (immutil_getAttr("saAmfCompAmStartTimeout", attributes, 0, &comp->comp_info.amstart_time) != SA_AIS_OK)
		comp->comp_info.amstart_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStartAttempts", attributes,
			    0, &comp->comp_info.max_num_amstart) != SA_AIS_OK)
		comp->comp_info.max_num_amstart = avd_comp_global_attrs.saAmfNumMaxAmStartAttempts;

	if (strlen(comptype->saAmfCtRelPathAmStopCmd) > 0) {
		strcpy(comp->comp_info.amstop_info, comptype->saAmfCtRelPathAmStopCmd);
		cmd_argv = comp->comp_info.amstop_info + strlen(comp->comp_info.amstop_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompAmStopCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefAmStopCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);
	}

	if (immutil_getAttr("saAmfCompAmStopTimeout", attributes, 0, &comp->comp_info.amstop_time) != SA_AIS_OK)
		comp->comp_info.amstop_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStopAttempts", attributes,
			    0, &comp->comp_info.max_num_amstop) != SA_AIS_OK)
		comp->comp_info.max_num_amstop = avd_comp_global_attrs.saAmfNumMaxAmStopAttempts;

	if (immutil_getAttr("saAmfCompCSISetCallbackTimeout", attributes,
			    0, &comp->comp_info.csi_set_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_set_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompCSIRmvCallbackTimeout", attributes,
			    0, &comp->comp_info.csi_rmv_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_rmv_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompQuiescingCompleteTimeout", attributes,
			    0, &comp->comp_info.quiescing_complete_timeout) != SA_AIS_OK)
		comp->comp_info.quiescing_complete_timeout = comptype->saAmfCtDefQuiescingCompleteTimeout;

	if (immutil_getAttr("saAmfCompRecoveryOnError", attributes, 0, &comp->comp_info.def_recvr) != SA_AIS_OK)
		comp->comp_info.def_recvr = comptype->saAmfCtDefRecoveryOnError;

	if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
		comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
		LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
			   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
	}

	if (immutil_getAttr("saAmfCompDisableRestart", attributes, 0, &comp->comp_info.comp_restart) != SA_AIS_OK)
		comp->comp_info.comp_restart = comptype->saAmfCtDefDisableRestart;

	comp->max_num_csi_actv = -1;	// TODO
	comp->max_num_csi_stdby = -1;	// TODO

	rc = 0;
done:
	if (rc != 0) {
		avd_comp_delete(comp);
		comp = NULL;
	}

	return comp;
}

/**
 * Get configuration for all SaAmfComp objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_comp_config_get(const SaNameT *su_name, AVD_SU *su)
{
	SaAisErrorT rc, error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT comp_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfComp";
	AVD_COMP *comp;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, su_name,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, NULL, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize_2 failed: %u", rc);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &comp_name,
		(SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		if (!is_config_valid(&comp_name, attributes, NULL))
			goto done2;

		if ((comp = comp_create(&comp_name, attributes)) == NULL)
			goto done2;

		comp_add_to_model(comp);

		if (avd_compcstype_config_get(&comp_name, comp) != SA_AIS_OK)
			goto done2;
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

/**
 * Handle admin operations on SaAmfComp objects.
 *      
 * @param immOiHandle             
 * @param invocation
 * @param objectName
 * @param operationId
 * @param params
 */
static void comp_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *objectName, SaImmAdminOperationIdT opId,
	const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;

        TRACE_ENTER2("%llu, '%s', %llu", invocation, objectName->value, opId);

	AVD_COMP *comp = avd_comp_get(objectName);
	assert(comp != NULL);

	switch (opId) {
		/* Valid B.04 AMF comp admin operations */
	case SA_AMF_ADMIN_RESTART:
		if (comp->comp_info.comp_restart == TRUE) {
			LOG_WA("Component Restart disabled '%s'", objectName->value);
			rc = SA_AIS_ERR_NOT_SUPPORTED;
		}
		else if (comp->admin_pend_cbk.invocation != 0) {
			LOG_WA("Component undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->su->pend_cbk.invocation != 0) {
			LOG_WA("SU undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->su->su_on_node->admin_node_pend_cbk.invocation != 0) {
			LOG_WA("Node undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->saAmfCompPresenceState != SA_AMF_PRESENCE_INSTANTIATED) {
			LOG_WA("Component not instantiated '%s'", objectName->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
		else {
			/* prepare the admin op req message and queue it */
			if (avd_admin_op_msg_snd(&comp->comp_info.name, AVSV_SA_AMF_COMP,
				opId, comp->su->su_on_node) == NCSCC_RC_SUCCESS) {
				comp->admin_pend_cbk.admin_oper = opId;
				comp->admin_pend_cbk.invocation = invocation;
				rc = SA_AIS_OK;
			}
			else {
				LOG_WA("Admin op request send failed '%s'", objectName->value);
				rc = SA_AIS_ERR_TIMEOUT;
			}
		}
		break;

	case SA_AMF_ADMIN_EAM_START:
	case SA_AMF_ADMIN_EAM_STOP:
	default:
		LOG_WA("Unsupported admin operation '%llu'", opId);
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}

	if (rc != SA_AIS_OK)
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);

	TRACE_LEAVE2("(%u)",rc);
}

static SaAisErrorT comp_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_COMP *comp = avd_comp_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("'%s'", objectName->value);
	assert(comp != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfCompRestartCount", attributeName)) {
			(void)avd_saImmOiRtObjectUpdate(objectName, attributeName,
						       SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompRestartCount);
		} else if (!strcmp("saAmfCompCurrProxyName", attributeName)) {
			/* TODO */
		} else if (!strcmp("saAmfCompCurrProxiedNames", attributeName)) {
			/* TODO */
		} else
			assert(0);
	}

	return SA_AIS_OK;
}

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_COMP *comp;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER();

	comp = avd_comp_get(&opdata->objectName);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		void *value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfCompType")) {
			SaNameT dn = *((SaNameT*)value);
			if (NULL == avd_comptype_get(&dn)) {
				LOG_ER("saAmfCompType '%s' not found", dn.value);
				goto done;
			}

			if (comp->su->sg_of_su->sg_ncs_spec == TRUE) {
				LOG_ER("Modification of saAmfCompType for OpenSAF comp not supported");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompInstantiateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompInstantiateTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {
			uns32 num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompInstantiationLevel Fail, Zero InstantiationLevel");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {
			uns32 num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompNumMaxInstantiateWithoutDelay Fail, Zero withoutDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithDelay")) {
			uns32 num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompNumMaxInstantiateWithDelay Fail, Zero withDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDelayBetweenInstantiateAttempts")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompDelayBetweenInstantiateAttempts Fail, Zero Delay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompTerminateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompTerminateTimeout Fail, Zero timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompCleanupCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCleanupTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompAmStartCmdArgv Fail, NULL arg");
				goto done;
			}
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStartCmdArgv Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompAmStartTimeout Fail, Zero Timeout");
				goto done;
			}
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStartTimeout Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStartAttempt")) {
			uns32 num_am_start = *((SaUint32T *)value);
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompNumMaxAmStartAttempt Fail, Comp su_is_external");
				goto done;
			}
			if (num_am_start == 0) {
				LOG_ER("Modification of saAmfCompNumMaxAmStartAttempt Fail, Zero num_am_start");
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {
			char *param_val = *((char **)value);
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStopCmdArgv Fail, Comp su_is_external");
				goto done;
			}
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompAmStopCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopTimeout")) {
			SaTimeT timeout;
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStopTimeout Fail, Comp su_is_external");
				goto done;
			}
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompAmStopTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStopAttempt")) {	
			uns32 num_am_stop = *((SaUint32T *)value);
			if (TRUE == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompNumMaxAmStopAttempt Fail, Comp su_is_external");
				goto done;
			}
			if (num_am_stop == 0) {
				LOG_ER("Modification of saAmfCompNumMaxAmStopAttempt Fail, Zero num_am_stop");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSISetCallbackTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCSISetCallbackTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSIRmvCallbackTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCSIRmvCallbackTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompQuiescingCompleteTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompQuiescingCompleteTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompRecoveryOnError")) {
			uns32 recovery = *((SaUint32T *)value);
			if ((recovery < SA_AMF_NO_RECOMMENDATION) || (recovery > SA_AMF_CONTAINER_RESTART )) {
				LOG_ER("Modification of saAmfCompRecoveryOnError Fail, Invalid recovery =%d",recovery);
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			SaBoolT val = *((SaBoolT *)value);
			if ((val != SA_TRUE) && (val != SA_FALSE)) {
				LOG_ER("Modification of saAmfCompDisableRestart Fail, Invalid Input %d",val);
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompProxyCsi")) {
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				LOG_ER("Modification of saAmfCompProxyCsi Fail, Length Zero");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompContainerCsi")) {
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				LOG_ER("Modification of saAmfCompContainerCsi Fail, Length Zero");
				goto done;
			}
		} else {
			LOG_ER("Modification of attribute '%s' not supported", attribute->attrName);
			goto done;
		}
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT comp_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_COMP *comp;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER();

	comp = avd_comp_get(&opdata->objectName);

	if (comp->su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		LOG_ER("Rejecting deletion of '%s'", opdata->objectName.value);
		LOG_ER("SU admin state is not locked instantiation required for deletion");
		goto done;
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT comp_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = comp_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void comp_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_COMP *comp;
	NCS_BOOL node_present = FALSE;
	AVD_AVND *su_node_ptr = NULL;
	AVD_COMP_TYPE *comp_type;
	unsigned int rc = NCSCC_RC_SUCCESS;
	AVSV_PARAM_INFO param;

	TRACE_ENTER();

	memset(((uns8 *)&param), '\0', sizeof(AVSV_PARAM_INFO));
	param.class_id = AVSV_SA_AMF_COMP;
	param.act = AVSV_OBJ_OPR_MOD;

	comp = avd_comp_get(&opdata->objectName);
	param.name = comp->comp_info.name;
	comp_type = avd_comptype_get(&comp->saAmfCompType);

	m_AVD_GET_SU_NODE_PTR(avd_cb, comp->su, su_node_ptr);

	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		node_present = TRUE;
	}

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		char *cmd_argv;
		void *value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfCompType")) {
			SaNameT *dn = (SaNameT*) value;

			avd_comptype_remove_comp(comp);
			comp->saAmfCompType = *dn;
			comp->comp_type = avd_comptype_get(dn);
			avd_comptype_add_comp(comp);
			param.attr_id = saAmfCompType_ID;
			param.name_sec = *dn;
			TRACE("saAmfCompType changed to '%s' for '%s'", dn->value, opdata->objectName.value);
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {

			char *param_val = *((char **)value);
			param.attr_id = saAmfCompInstantiateCmd_ID;
			strcpy((char *)&param.value[0], (char *)&comp_type->saAmfCtRelPathInstantiateCmd);
			/* We need to append the arg with the command. */
			cmd_argv = (char *)(param.value + strlen((char *)param.value));
			*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */
			strncpy(cmd_argv, param_val, strlen(param_val));
			param.value_len = strlen((char *)param.value);

			memset(&comp->comp_info.init_cmd_arg_info, 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&comp->comp_info.init_cmd_arg_info, param_val);
			comp->comp_info.init_len = param.value_len;
			memset(&comp->comp_info.init_info, 0, AVSV_MISC_STR_MAX_SIZE);
			memcpy(comp->comp_info.init_info, &param.value[0], comp->comp_info.init_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateTimeout")) {
			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompInstantiateTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.init_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {
			AVD_SU *su = comp->su;
			param.attr_id = saAmfCompInstantiationLevel_ID;
			param.value_len = sizeof(uns32);
			memcpy(&param.value[0],(SaUint32T *)value , param.value_len);
			comp->comp_info.inst_level = *((SaUint32T *)value);
			
			avd_su_remove_comp(comp);
			comp->su = su;
			avd_su_add_comp(comp);
			
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {

			uns32 num_inst;
			num_inst = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxInstantiate_ID;
			param.value_len = sizeof(uns32);
			num_inst = htonl(num_inst);
			memcpy(&param.value[0], &num_inst, param.value_len);
			comp->comp_info.max_num_inst = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithDelay")) {

			uns32 num_inst;
			num_inst = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxInstantiateWithDelay_ID;
			param.value_len = sizeof(uns32);
			num_inst = htonl(num_inst);
			memcpy(&param.value[0], &num_inst, param.value_len);
			comp->max_num_inst_delay = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompDelayBetweenInstantiateAttempts")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompDelayBetweenInstantiateAttempts_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->inst_retry_delay = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {

			char *param_val = *((char **)value);
			param.attr_id = saAmfCompTerminateCmd_ID;
			strcpy((char *)&param.value[0], (char *)&comp_type->saAmfCtRelPathTerminateCmd);
			/* We need to append the arg with the command. */
			cmd_argv = (char *)(param.value + strlen((char *)param.value));
			*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */
			strncpy(cmd_argv, param_val, strlen(param_val));
			param.value_len = strlen((char *)param.value);

			memset(&comp->comp_info.term_cmd_arg_info, 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&comp->comp_info.term_cmd_arg_info, param_val);
			comp->comp_info.term_len = param.value_len;
			memset(&comp->comp_info.term_info, 0, AVSV_MISC_STR_MAX_SIZE);
			memcpy(comp->comp_info.term_info, &param.value[0], comp->comp_info.term_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompTerminateTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.term_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {

			char *param_val = *((char **)value);
			param.attr_id = saAmfCompCleanupCmd_ID;
			strcpy((char *)&param.value[0], (char *)&comp_type->saAmfCtRelPathCleanupCmd);
			/* We need to append the arg with the command. */
			cmd_argv = (char *)(param.value + strlen((char *)param.value));
			*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */
			strncpy(cmd_argv, param_val, strlen(param_val));
			param.value_len = strlen((char *)param.value);

			memset(&comp->comp_info.clean_cmd_arg_info, 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&comp->comp_info.clean_cmd_arg_info, param_val);
			comp->comp_info.clean_len = param.value_len;
			memset(&comp->comp_info.clean_info, 0, AVSV_MISC_STR_MAX_SIZE);
			memcpy(comp->comp_info.clean_info, &param.value[0], comp->comp_info.clean_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupTimeout")) {
			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCleanupTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.clean_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {

			char *param_val = *((char **)value);

			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			param.attr_id = saAmfCompAmStartCmd_ID;
			strcpy((char *)&param.value[0], (char *)&comp_type->saAmfCtRelPathAmStartCmd);
			/* We need to append the arg with the command. */
			cmd_argv = (char *)(param.value + strlen((char *)param.value));
			*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */
			strncpy(cmd_argv, param_val, strlen(param_val));
			param.value_len = strlen((char *)param.value);

			memset(&comp->comp_info.amstart_cmd_arg_info, 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&comp->comp_info.amstart_cmd_arg_info, param_val);
			comp->comp_info.amstart_len = param.value_len;
			memset(&comp->comp_info.amstart_info, 0, AVSV_MISC_STR_MAX_SIZE);
			memcpy(comp->comp_info.amstart_info, &param.value[0], comp->comp_info.amstart_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompAmStartTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.amstart_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStartAttempt")) {

			uns32 num_am_start;
			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			num_am_start = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxAmStartAttempts_ID;
			param.value_len = sizeof(uns32);
			num_am_start = htonl(num_am_start);
			memcpy(&param.value[0], &num_am_start, param.value_len);
			comp->comp_info.max_num_amstart = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {

			char *param_val = *((char **)value);
			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			memset(&(comp->comp_info.amstop_cmd_arg_info), 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&(comp->comp_info.amstop_cmd_arg_info), param_val);
			param.attr_id = saAmfCompAmStopCmd_ID;
			strcpy((char *)&param.value[0], (char *)&comp_type->saAmfCtRelPathAmStartCmd);
			/* We need to append the arg with the command. */
			cmd_argv = (char *)(param.value + strlen((char *)param.value));
			*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */
			strncpy(cmd_argv, param_val, strlen(param_val));
			param.value_len = strlen((char *)param.value);

			memset(&comp->comp_info.amstop_cmd_arg_info, 0, AVSV_MISC_STR_MAX_SIZE);
			strcpy((char *)&comp->comp_info.amstop_cmd_arg_info, param_val);
			comp->comp_info.amstop_len = param.value_len;
			memset(&comp->comp_info.amstop_info, 0, AVSV_MISC_STR_MAX_SIZE);
			memcpy((char *)&comp->comp_info.amstop_info, &param.value[0],
			       comp->comp_info.amstop_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompAmStopTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.amstop_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStopAttempt")) {

			uns32 num_am_stop;
			if (TRUE == comp->su->su_is_external) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}

			num_am_stop = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxAmStopAttempts_ID;
			param.value_len = sizeof(uns32);
			num_am_stop = htonl(num_am_stop);
			memcpy(&param.value[0], &num_am_stop, param.value_len);
			comp->comp_info.max_num_amstop = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCSISetCallbackTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCSISetCallbackTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.csi_set_callback_timeout = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCSIRmvCallbackTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCSIRmvCallbackTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.csi_rmv_callback_timeout = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompQuiescingCompleteTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;
			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompQuiescingCompleteTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			comp->comp_info.quiescing_complete_timeout = *((SaTimeT *)value);
		} else if (!strcmp(attribute->attrName, "saAmfCompRecoveryOnError")) {
			uns32 recovery;
			recovery = *((SaUint32T *)value);
			param.attr_id = saAmfCompRecoveryOnError_ID;
			param.value_len = sizeof(uns32);
			recovery = htonl(recovery);
			memcpy(&param.value[0], &recovery, param.value_len);
			comp->comp_info.def_recvr = *((SaUint32T *)value);
			if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
				comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
				LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
					   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			comp->comp_info.comp_restart = *((SaUint32T *)value);
		} else if (!strcmp(attribute->attrName, "saAmfCompProxyCsi")) {
			comp->comp_proxy_csi = *((SaNameT *)value);
		} else if (!strcmp(attribute->attrName, "saAmfCompContainerCsi")) {
			comp->comp_container_csi = *((SaNameT *)value);
		} else {
			assert(0);
		}

		if (TRUE == node_present)
			avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

done:
	TRACE_LEAVE();
	return;
}

static void comp_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata)
{
	AVD_COMP *comp = NULL, *i_comp = NULL;
	uns32 min_si = 0;
	NCS_BOOL isPre;
	AVD_AVND *su_node_ptr = NULL;
	AVSV_PARAM_INFO param;
	SaBoolT old_val;
	SaBoolT su_delete = SA_FALSE;
	struct CcbUtilOperationData *t_opData;

	TRACE_ENTER();

	comp = avd_comp_get(&opdata->objectName);
	/* comp should be found in the database even if it was 
	 * due to parent su delete the changes are applied in 
	 * bottom up order so all the component deletes are applied 
	 * first and then parent SU delete is applied
	 * just doing sanity check here
	 **/
	assert(comp != NULL);

	/* verify if the max ACTIVE and STANDBY SIs of the SU 
	 ** need to be changed
	 */
	if (comp->max_num_csi_actv == comp->su->si_max_active) {
		/* find the number and set it */
		min_si = 0;
		i_comp = comp->su->list_of_comp;
		while (i_comp) {
			if (i_comp != comp) {
				if (min_si > i_comp->max_num_csi_actv)
					min_si = i_comp->max_num_csi_actv;
				else if (min_si == 0)
					min_si = i_comp->max_num_csi_actv;
			}
			i_comp = i_comp->su_comp_next;
		}
		/* Now we have the min value. set it */
		comp->su->si_max_active = min_si;
	}

	/* FOR STANDBY count */
	if (comp->max_num_csi_stdby == comp->su->si_max_standby) {
		/* find the number and set it */
		min_si = 0;
		i_comp = comp->su->list_of_comp;
		while (i_comp) {
			if (i_comp != comp) {
				if (min_si > i_comp->max_num_csi_stdby)
					min_si = i_comp->max_num_csi_stdby;
				else if (min_si == 0)
					min_si = i_comp->max_num_csi_stdby;
			}
			i_comp = i_comp->su_comp_next;
		}
		/* Now we have the min value. set it */
		comp->su->si_max_standby = min_si;
	}

	old_val = comp->su->saAmfSUPreInstantiable;

	/* Verify if the SUs preinstan value need to be changed */
	if ((AVSV_COMP_TYPE_SA_AWARE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == comp->comp_info.category)) {
		isPre = FALSE;
		i_comp = comp->su->list_of_comp;
		while (i_comp) {
			if (((AVSV_COMP_TYPE_SA_AWARE == i_comp->comp_info.category) ||
			     (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == i_comp->comp_info.category) ||
			     (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == i_comp->comp_info.category))
			    && (i_comp != comp)) {
				isPre = TRUE;
				break;
			}
			i_comp = i_comp->su_comp_next;
		}		/* end while */

		if (isPre == TRUE) {
			comp->su->saAmfSUPreInstantiable = TRUE;
		} else {
			comp->su->saAmfSUPreInstantiable = FALSE;
		}
	}

	if (comp->su->curr_num_comp == 1) {
		/* This comp will be deleted so revert these to def val */
		comp->su->si_max_active = 0;
		comp->su->si_max_standby = 0;
		comp->su->saAmfSUPreInstantiable = TRUE;
	}

	/* check whether the SU is also undergoing delete operation */
	t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &comp->su->name);
	if (t_opData && t_opData->operationType == CCBUTIL_DELETE) {
		su_delete = SA_TRUE;
	}

	/* if SU is not being deleted and the PreInstantiable state has changed
	 * then update the IMM with the new value for saAmfSUPreInstantiable */
	if (su_delete == SA_FALSE && old_val != comp->su->saAmfSUPreInstantiable) {
		avd_saImmOiRtObjectUpdate(&comp->su->name, "saAmfSUPreInstantiable", SA_IMM_ATTR_SAUINT32T,
				&comp->su->saAmfSUPreInstantiable);
	}

	/* send a message to the AVND deleting the
	 * component.
	 */
	m_AVD_GET_SU_NODE_PTR(avd_cb, comp->su, su_node_ptr);
	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		memset(((uns8 *)&param), '\0', sizeof(AVSV_PARAM_INFO));
		param.act = AVSV_OBJ_OPR_DEL;
		param.name = comp->comp_info.name;
		param.class_id = AVSV_SA_AMF_COMP;
		avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

	avd_comp_delete(comp);
	TRACE_LEAVE();
}

static void comp_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_COMP *comp;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		comp = comp_create(&opdata->objectName, opdata->param.create.attrValues);
		assert(comp);
		comp_add_to_model(comp);
		break;
	case CCBUTIL_MODIFY:
		comp_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		comp_ccb_apply_delete_hdlr(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE();
}

void avd_comp_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&comp_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfComp", comp_rt_attr_cb, comp_admin_op_cb,
		comp_ccb_completed_cb, comp_ccb_apply_cb);
}

