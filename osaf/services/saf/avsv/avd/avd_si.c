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
 *            Ericsson AB
 *
 */

#include <logtrace.h>
#include <saflog.h>

#include <avd_util.h>
#include <avd_si.h>
#include <avd_app.h>
#include <avd_imm.h>
#include <avd_csi.h>
#include <avd_proc.h>

static NCS_PATRICIA_TREE si_db;
static void avd_si_add_csi_db(struct avd_csi_tag* csi);
static void si_update_ass_state(AVD_SI *si);

static void avd_si_arrange_dep_csi(struct avd_csi_tag* csi)
{
	AVD_CSI *temp_csi = NULL;
	AVD_SI *temp_si = NULL;
revisit:
        temp_csi = csi->si->list_of_csi;
        while (temp_csi) {
                if ((FALSE == temp_csi->dep_csi_added) && (0 == memcmp(&temp_csi->saAmfCSIDependencies, &csi->name, sizeof(SaNameT)))) {
                        temp_csi->rank = csi->rank + 1;
                        /* We need to rearrange Dep CSI rank as per modified. */
                        /* Store the SI pointer as avd_si_remove_csi makes it NULL in the end */
                        temp_si = temp_csi->si;
                        avd_si_remove_csi(temp_csi);
                        temp_csi->si = temp_si;
                        avd_si_add_csi_db(temp_csi);
                        temp_csi->dep_csi_added = TRUE;
                        /* We need to check whether any other CSI is dependent on temp_csi.*/
                        avd_si_arrange_dep_csi(temp_csi);
                        /* We need to revisit again as the list has been modified. */
                        goto revisit;
                }
                temp_csi = temp_csi->si_list_of_csi_next;
        }
	return;
}

void avd_si_add_csi(struct avd_csi_tag* avd_csi)
{
	AVD_CSI *temp_csi = NULL;
        bool found = FALSE;

        /* Find whether csi (avd_csi->saAmfCSIDependencies) is already in the DB. */
        if (avd_csi->rank != 1) /* (rank != 1) ==> (rank is 0). i.e. avd_csi is dependent on another CSI. */ {
                temp_csi = avd_csi->si->list_of_csi;
                while (temp_csi) {
                        if (0 == memcmp(&temp_csi->name, &avd_csi->saAmfCSIDependencies, sizeof(SaNameT))) {
				/* The CSI is dependent on other CSI, so its rank will be one more than Dep CSI */
				avd_csi->rank = temp_csi->rank + 1;
				found = TRUE;
                        }
			temp_csi = temp_csi->si_list_of_csi_next;
                }
                if (found == FALSE) {
			/* Not found, means saAmfCSIDependencies is yet to come. So put it in the end with rank 0. 
                           There may be existing CSIs which will be dependent on avd_csi, but don't run 
                           avd_si_arrange_dep_csi() as of now. All dependent CSI's rank will be changed when 
			   CSI on which avd_csi(avd_csi->saAmfCSIDependencies) will come.*/
                        goto add_csi;

                }
        }

        /* We need to check whether any other previously added CSI(with rank = 0) was dependent on this CSI. */
        avd_si_arrange_dep_csi(avd_csi);
add_csi:
        avd_si_add_csi_db(avd_csi);

	return;
}

static void avd_si_add_csi_db(struct avd_csi_tag* csi)
{
	AVD_CSI *i_csi = NULL;
	AVD_CSI *prev_csi = NULL;

	assert((csi != NULL) && (csi->si != NULL));

	i_csi = csi->si->list_of_csi;

	while ((i_csi != NULL) && (csi->rank <= i_csi->rank)) {
		prev_csi = i_csi;
		i_csi = i_csi->si_list_of_csi_next;
	}

	if (prev_csi == NULL) {
		csi->si_list_of_csi_next = csi->si->list_of_csi;
		csi->si->list_of_csi = csi;
	} else {
		prev_csi->si_list_of_csi_next = csi;
		csi->si_list_of_csi_next = i_csi;
	}

//	csi->si->num_csi++;
}

void avd_si_remove_csi(AVD_CSI* csi)
{
	AVD_CSI *i_csi = NULL;
	AVD_CSI *prev_csi = NULL;

	if (csi->si != AVD_SI_NULL) {
		/* remove CSI from the SI */
		prev_csi = NULL;
		i_csi = csi->si->list_of_csi;

		while ((i_csi != NULL) && (i_csi != csi)) {
			prev_csi = i_csi;
			i_csi = i_csi->si_list_of_csi_next;
		}

		if (i_csi != csi) {
			/* Log a fatal error */
			assert(0);
		} else {
			if (prev_csi == NULL) {
				csi->si->list_of_csi = csi->si_list_of_csi_next;
			} else {
				prev_csi->si_list_of_csi_next = csi->si_list_of_csi_next;
			}
		}
#if 0
		assert(csi->si->num_csi);
		csi->si->num_csi--;
#endif
		csi->si_list_of_csi_next = NULL;
		csi->si = AVD_SI_NULL;
	}			/* if (csi->si != AVD_SI_NULL) */
}

AVD_SI *avd_si_new(const SaNameT *dn)
{
	AVD_SI *si;

	if ((si = calloc(1, sizeof(AVD_SI))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}

	memcpy(si->name.value, dn->value, dn->length);
	si->name.length = dn->length;
	si->tree_node.key_info = (uns8 *)&si->name;
	si->si_switch = AVSV_SI_TOGGLE_STABLE;
	si->saAmfSIAdminState = SA_AMF_ADMIN_UNLOCKED;
	si->si_dep_state = AVD_SI_NO_DEPENDENCY;
	si->saAmfSIAssignmentState = SA_AMF_ASSIGNMENT_UNASSIGNED;
	si->alarm_sent = FALSE;
	si->num_dependents = 0;

	return si;
}

static void si_delete_csis(AVD_SI *si)
{
	AVD_CSI *csi, *temp;

	csi = si->list_of_csi; 
	while (csi != NULL) {
		temp = csi;
		csi = csi->si_list_of_csi_next;
		avd_csi_delete(temp);
	}

	si->list_of_csi = NULL;
	si->num_csi = 0;
}

void avd_si_delete(AVD_SI *si)
{
	unsigned int rc;

	TRACE_ENTER2("%s", si->name.value);

	/* All CSI under this should have been deleted by now on the active 
	   director and on standby all the csi should be deleted because 
	   csi delete is not checkpointed as and when it happens */
	si_delete_csis(si);

	/* All the SI Dependencies should have been unconfigured or deleted */
	assert(si->spons_si_list == 0);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, si, AVSV_CKPT_AVD_SI_CONFIG);
	avd_svctype_remove_si(si);
	avd_app_remove_si(si->app, si);
	avd_sg_remove_si(si->sg_of_si, si);
	rc = ncs_patricia_tree_del(&si_db, &si->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
	free(si);
}

void avd_si_db_add(AVD_SI *si)
{
	unsigned int rc;

	if (avd_si_get(&si->name) == NULL) {
		rc = ncs_patricia_tree_add(&si_db, &si->tree_node);
		assert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_SI *avd_si_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SI *)ncs_patricia_tree_get(&si_db, (uns8 *)&tmp);
}

AVD_SI *avd_si_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SI *)ncs_patricia_tree_getnext(&si_db, (uns8 *)&tmp);
}

static void si_add_to_model(AVD_SI *si)
{
	SaNameT dn;

	TRACE_ENTER2("%s", si->name.value);

	/* Check parent link to see if it has been added already */
	if (si->svc_type != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&si->name, &dn, "safApp");
	si->app = avd_app_get(&dn);

	avd_si_db_add(si);

	si->svc_type = avd_svctype_get(&si->saAmfSvcType);

	if (si->saAmfSIProtectedbySG.length > 0)
		si->sg_of_si = avd_sg_get(&si->saAmfSIProtectedbySG);

	avd_svctype_add_si(si);
	avd_app_add_si(si->app, si);
	avd_sg_add_si(si->sg_of_si, si);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, si, AVSV_CKPT_AVD_SI_CONFIG);
	avd_saImmOiRtObjectUpdate(&si->name, "saAmfSIAssignmentState",
		SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAssignmentState);

done:
	TRACE_LEAVE();
}

/**
 * Validate configuration attributes for an AMF SI object
 * @param si
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	SaAmfAdminStateT admstate;
	char *parent;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safApp=", 7) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfSvcType", attributes, 0, &aname);
	assert(rc == SA_AIS_OK);

	if (avd_svctype_get(&aname) == NULL) {
		/* SVC type does not exist in current model, check CCB if passed as param */
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if (immutil_getAttr("saAmfSIProtectedbySG", attributes, 0, &aname) == SA_AIS_OK) {
		if (avd_sg_get(&aname) == NULL) {
			if (opdata == NULL) {
				LOG_ER("'%s' does not exist", aname.value);
				return 0;
			}

			/* SG does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
				LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
				return 0;
			}
		}
	}

	if ((immutil_getAttr("saAmfSIAdminState", attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate)) {
		LOG_ER("Invalid saAmfSIAdminState %u for '%s'", admstate, dn->value);
		return 0;
	}

	return 1;
}

static AVD_SI *si_create(SaNameT *si_name, const SaImmAttrValuesT_2 **attributes)
{
	int i, rc = -1;
	AVD_SI *si;
	SaUint32T attrValuesNumber;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", si_name->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if (NULL == (si = avd_si_get(si_name))) {
		if ((si = avd_si_new(si_name)) == NULL)
			goto done;
	} else {
		TRACE("already created, refreshing config...");
		/* here delete the whole csi configuration to refresh 
		   since csi deletes are not checkpointed there may be 
		   some CSIs that are deleted from previous data sync up */
		si_delete_csis(si);
	}

	error = immutil_getAttr("saAmfSvcType", attributes, 0, &si->saAmfSvcType);
	assert(error == SA_AIS_OK);

	/* Optional, strange... */
	(void)immutil_getAttr("saAmfSIProtectedbySG", attributes, 0, &si->saAmfSIProtectedbySG);

	if (immutil_getAttr("saAmfSIRank", attributes, 0, &si->saAmfSIRank) != SA_AIS_OK) {
		/* Empty, assign default value */
		si->saAmfSIRank = 0;
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber("saAmfSIActiveWeight", attributes, &attrValuesNumber) == SA_AIS_OK) {
		si->saAmfSIActiveWeight = malloc(attrValuesNumber * sizeof(char *));
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIActiveWeight[i] =
			    strdup(immutil_getStringAttr(attributes, "saAmfSIActiveWeight", i));
		}
	} else {
		/*  TODO */
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber("saAmfSIStandbyWeight", attributes, &attrValuesNumber) == SA_AIS_OK) {
		si->saAmfSIStandbyWeight = malloc(attrValuesNumber * sizeof(char *));
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIStandbyWeight[i] =
			    strdup(immutil_getStringAttr(attributes, "saAmfSIStandbyWeight", i));
		}
	} else {
		/*  TODO */
	}

	if (immutil_getAttr("saAmfSIPrefActiveAssignments", attributes, 0,
			    &si->saAmfSIPrefActiveAssignments) != SA_AIS_OK) {

		/* Empty, assign default value */
		si->saAmfSIPrefActiveAssignments = 1;
	}

	if (immutil_getAttr("saAmfSIPrefStandbyAssignments", attributes, 0,
			    &si->saAmfSIPrefStandbyAssignments) != SA_AIS_OK) {

		/* Empty, assign default value */
		si->saAmfSIPrefStandbyAssignments = 1;
	}

	if (immutil_getAttr("saAmfSIAdminState", attributes, 0, &si->saAmfSIAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		si->saAmfSIAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	rc = 0;

done:
	if (rc != 0) {
		avd_si_delete(si);
		si = NULL;
	}

	return si;
}

/**
 * Get configuration for all AMF SI objects from IMM and create
 * AVD internal objects.
 * 
 * @param cb
 * @param app
 * 
 * @return int
 */
SaAisErrorT avd_si_config_get(AVD_APP *app)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT si_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSI";
	AVD_SI *si;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, &app->name, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
	      NULL, &searchHandle) != SA_AIS_OK) {

		LOG_ER("No objects found (1)");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &si_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&si_name, attributes, NULL))
			goto done2;

		if ((si = si_create(&si_name, attributes)) == NULL)
			goto done2;

		si_add_to_model(si);

		if (avd_sirankedsu_config_get(&si_name, si) != SA_AIS_OK)
			goto done2;

		if (avd_csi_config_get(&si_name, si) != SA_AIS_OK)
			goto done2;
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static SaAisErrorT si_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_SI *si;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	si = avd_si_get(&opdata->objectName);
	assert(si != NULL);

	/* Modifications can only be done for these attributes. */
	i = 0;
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

		if (!strcmp(attribute->attrName, "saAmfSIPrefActiveAssignments")) {
			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				LOG_ER("SG'%s' is not stable (%u)", si->sg_of_si->name.value, 
						si->sg_of_si->sg_fsm_state);
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
			if (si->sg_of_si->sg_redundancy_model != SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) {
				LOG_ER("Invalid modification,saAmfSIPrefActiveAssignments can be updated only for" 
						" N_WAY_ACTIVE_REDUNDANCY_MODEL");
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
		} else if (!strcmp(attribute->attrName, "saAmfSIPrefStandbyAssignments")) {
			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				LOG_ER("SG'%s' is not stable (%u)", si->sg_of_si->name.value, 
						si->sg_of_si->sg_fsm_state);
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
			if( si->sg_of_si->sg_redundancy_model != SA_AMF_N_WAY_REDUNDANCY_MODEL ) {
				LOG_ER("Invalid modification,saAmfSIPrefStandbyAssignments can be updated only for" 
						" N_WAY_REDUNDANCY_MODEL");
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
		} else {
			LOG_ER("Modification of attribute %s is not supported", attribute->attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			break;
		}
	}

	return rc;
}

static void si_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *objectName, SaImmAdminOperationIdT operationId,
	const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;
	uns32 err = NCSCC_RC_FAILURE;
	AVD_SI *si;
	SaAmfAdminStateT adm_state = 0;
	SaAmfAdminStateT back_val;

	TRACE_ENTER2("%s op=%llu", objectName->value, operationId);

	si = avd_si_get(objectName);
	
	if ((operationId != SA_AMF_ADMIN_SI_SWAP) && (si->sg_of_si->sg_ncs_spec == SA_TRUE)) {
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		LOG_WA("Admin operation %llu on MW SI is not allowed", operationId);
		goto done;
	}

	switch (operationId) {
	case SA_AMF_ADMIN_UNLOCK:
		if (SA_AMF_ADMIN_UNLOCKED == si->saAmfSIAdminState) {
			LOG_WA("SI unlock of %s failed, already unlocked", objectName->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
			LOG_WA("SI unlock of %s failed, SG not stable", objectName->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);

		switch (si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			err = avd_sg_2n_si_func(avd_cb, si);
			break;
		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			err = avd_sg_nacvred_si_func(avd_cb, si);
			break;
		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			err = avd_sg_nway_si_func(avd_cb, si);
			break;
		case SA_AMF_NPM_REDUNDANCY_MODEL:
			err = avd_sg_npm_si_func(avd_cb, si);
			break;
		case SA_AMF_NO_REDUNDANCY_MODEL:
			err = avd_sg_nored_si_func(avd_cb, si);
			break;
		default:
			assert(0);
		}

		if (err != NCSCC_RC_SUCCESS) {
			LOG_WA("SI unlock of %s failed", objectName->value);
			avd_si_admin_state_set(si, SA_AMF_ADMIN_LOCKED);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}

		break;

	case SA_AMF_ADMIN_SHUTDOWN:
		if (SA_AMF_ADMIN_SHUTTING_DOWN == si->saAmfSIAdminState) {
			LOG_WA("SI unlock of %s failed, already shutting down", objectName->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if ((SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_LOCKED_INSTANTIATION == si->saAmfSIAdminState)) {
			LOG_WA("SI unlock of %s failed, is locked (instantiation)", objectName->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		adm_state = SA_AMF_ADMIN_SHUTTING_DOWN;

		/* Don't break */

	case SA_AMF_ADMIN_LOCK:
		if (0 == adm_state)
			adm_state = SA_AMF_ADMIN_LOCKED;

		if (SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) {
			LOG_WA("SI lock of %s failed, already locked", objectName->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
			avd_si_admin_state_set(si, SA_AMF_ADMIN_LOCKED);
			LOG_WA("SI lock of %s, has no assignments", objectName->value);
			rc = SA_AIS_OK;
			goto done;
		}

		/* Check if other semantics are happening for other SUs. If yes
		 * return an error.
		 */
		if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
			LOG_WA("SI lock of %s failed, SG not stable", objectName->value);
			if ((si->sg_of_si->sg_fsm_state != AVD_SG_FSM_SI_OPER) ||
			    (si->saAmfSIAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) ||
			    (adm_state != SA_AMF_ADMIN_LOCKED)) {
				LOG_WA("'%s' other semantics...", objectName->value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}

		back_val = si->saAmfSIAdminState;
		avd_si_admin_state_set(si, (adm_state));

		switch (si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			err = avd_sg_2n_si_admin_down(avd_cb, si);
			break;
		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			err = avd_sg_nway_si_admin_down(avd_cb, si);
			break;
		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			err = avd_sg_nacvred_si_admin_down(avd_cb, si);
			break;
		case SA_AMF_NPM_REDUNDANCY_MODEL:
			err = avd_sg_npm_si_admin_down(avd_cb, si);
			break;
		case SA_AMF_NO_REDUNDANCY_MODEL:
			err = avd_sg_nored_si_admin_down(avd_cb, si);
			break;
		default:
			assert(0);
		}

		if (err != NCSCC_RC_SUCCESS) {
			LOG_WA("SI shutdown/lock of %s failed", objectName->value);
			avd_si_admin_state_set(si, back_val);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}

		break;

	case SA_AMF_ADMIN_SI_SWAP:
		switch (si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			rc = avd_sg_2n_siswap_func(si, invocation);
			break;
		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
		case SA_AMF_NPM_REDUNDANCY_MODEL:
			/* TODO , Will be done when the SI SWAP is implemented */
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
			break;
			
		case SA_AMF_NO_REDUNDANCY_MODEL:
		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
		default:
			LOG_WA("SI lock of %s failed, SI SWAP not Supported on redundancy model=%u",
				objectName->value, si->sg_of_si->sg_redundancy_model);
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
			break;
		}
		break;
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		LOG_WA("SI Admin operation %llu not supported", operationId);
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}

done:
	if ((operationId != SA_AMF_ADMIN_SI_SWAP) || (rc != SA_AIS_OK))
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
	TRACE_LEAVE();
}

static SaAisErrorT si_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_SI *si = avd_si_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("%s", objectName->value);
	assert(si != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfSINumCurrActiveAssignments", attributeName)) {
			(void)avd_saImmOiRtObjectUpdate(objectName, attributeName,
						       SA_IMM_ATTR_SAUINT32T, &si->saAmfSINumCurrActiveAssignments);
		} else if (!strcmp("saAmfSINumCurrStandbyAssignments", attributeName)) {
			(void)avd_saImmOiRtObjectUpdate(objectName, attributeName,
						       SA_IMM_ATTR_SAUINT32T, &si->saAmfSINumCurrStandbyAssignments);
		} else
			assert(0);
	}

	return SA_AIS_OK;
}

static SaAisErrorT si_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SI *si;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = si_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		si = avd_si_get(&opdata->objectName);
		if (NULL != si->list_of_sisu) {
			LOG_ER("SaAmfSI is in use '%s'", si->name.value);
			goto done;
		}
		/* check for any SI-SI dependency configurations */
		if (0 != si->num_dependents) {
			LOG_ER("Dependents Exist; Cannot delete '%s'", si->name.value);
			goto done;
		}
		rc = SA_AIS_OK;
		opdata->userData = si;	/* Save for later use in apply */
		break;
	default:
		assert(0);
		break;
	}
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
/*
 * @brief  Readjust the SI assignments whenever PrefActiveAssignments  & PrefStandbyAssignments is updated
 * 	   using IMM interface 
 * 
 * @param[in]  SI 
 * 
 * @return void 
 */
static void avd_si_adjust_si_assignments(AVD_SI *si)
{
	AVD_SU_SI_REL *sisu, *tmp_sisu;
	uns32 no_of_sisus_to_delete;
	AVD_SU_SI_STATE old_susi_state = AVD_SU_SI_STATE_ASGN;
	uns32 i = 0;

	TRACE_ENTER2("for SI:%s ", si->name.value);
	
	if( si->sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL ) {
		if( si->saAmfSIPrefActiveAssignments > si->saAmfSINumCurrActiveAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if ( avd_sg_nacvred_su_chose_asgn(avd_cb, si->sg_of_si ) != NULL ) {
				/* New assignments are been done in the SG.
                                   change the SG FSM state to AVD_SG_FSM_SG_REALIGN */
				m_AVD_SET_SG_FSM(avd_cb, si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
			} else {
				/* No New assignments are been done in the SG
				   reason might be no more inservice SUs to take new assignments or
				   SUs are already assigned to saAmfSGMaxActiveSIsperSU capacity */
				TRACE("No New assignments are been done SI:%s",si->name.value);
			}
		} else {
			no_of_sisus_to_delete = si->saAmfSINumCurrActiveAssignments -
						si->saAmfSIPrefActiveAssignments;

			/* Get the sisu pointer from the  si->list_of_sisu list from which 
			no of sisus need to be deleted based on SI ranked SU */
			sisu = tmp_sisu = si->list_of_sisu;
			for( i = 0; i < no_of_sisus_to_delete && NULL != tmp_sisu; i++ ) {
				tmp_sisu = tmp_sisu->si_next;
			}
			while( tmp_sisu && (tmp_sisu->si_next != NULL) ) {
				sisu = sisu->si_next;
				tmp_sisu = tmp_sisu->si_next;
			}

			for( i = 0; i < no_of_sisus_to_delete && (NULL != sisu); i++ ) {
				/* Send quiesced request for the sisu that needs tobe deleted */
				old_susi_state = sisu->fsm;
				sisu->state = SA_AMF_HA_QUIESCED;
			        sisu->fsm = AVD_SU_SI_STATE_MODIFY;

				if ( avd_snd_susi_msg( avd_cb, sisu->su, sisu, AVSV_SUSI_ACT_MOD ) == 
										NCSCC_RC_FAILURE ) {
					LOG_ER("susi msg send failed  SU:%s SI:%s",sisu->su->name.value,
										sisu->si->name.value);
					sisu->state = SA_AMF_HA_ACTIVE;
					sisu->fsm = old_susi_state;
				} else {
					/* Add SU to su_opr_list */
                                	avd_sg_su_oper_list_add(avd_cb, sisu->su, FALSE);

					/* Checkpoint the sisu state change and send notification */
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(avd_cb, sisu);
				}
				sisu = sisu->si_next;
			}
			/* Change the SG FSM to AVD_SG_FSM_SG_REALIGN SG to Stable state */
			m_AVD_SET_SG_FSM(avd_cb,  si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
		}
	} 
	if( si->sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL ) {
		if( si->saAmfSIPrefStandbyAssignments > si->saAmfSINumCurrStandbyAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if( avd_sg_nway_si_assign(avd_cb, si->sg_of_si ) == NCSCC_RC_FAILURE ) {
				LOG_ER("SI new assignmemts failed  SI:%s",si->name.value);
			} 
		} else {
			no_of_sisus_to_delete = 0;
			no_of_sisus_to_delete = si->saAmfSINumCurrStandbyAssignments -
						si->saAmfSIPrefStandbyAssignments; 

			/* Get the sisu pointer from the  si->list_of_sisu list from which 
			no of sisus need to be deleted based on SI ranked SU */
			sisu = tmp_sisu = si->list_of_sisu;
			for(i = 0; i < no_of_sisus_to_delete && (NULL != tmp_sisu); i++) {
				tmp_sisu = tmp_sisu->si_next;
			}
			while( tmp_sisu && (tmp_sisu->si_next != NULL) ) {
				sisu = sisu->si_next;
				tmp_sisu = tmp_sisu->si_next;
			}

			for( i = 0; i < no_of_sisus_to_delete && (NULL != sisu); i++ ) {
				/* Delete Standby SI assignment & move it to Realign state */
				old_susi_state = sisu->fsm;
				sisu->fsm = AVD_SU_SI_STATE_UNASGN;
				if(avd_snd_susi_msg( avd_cb,sisu->su, sisu, AVSV_SUSI_ACT_DEL ) == 
											NCSCC_RC_FAILURE){
					LOG_ER("sisu msg send failed  SU:%s SI:%s",sisu->su->name.value,
									sisu->si->name.value);
					sisu->state = SA_AMF_HA_STANDBY;
					sisu->fsm = old_susi_state;
				} else {
					/* Add SU to su_opr_list */
                                	avd_sg_su_oper_list_add(avd_cb, sisu->su, FALSE);

					/* Checkpoint the sisu state change and send notification */
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(avd_cb, sisu);
				}
				sisu = sisu->si_next;
			}
			/* Change the SG FSM to AVD_SG_FSM_SG_REALIGN */ 
			m_AVD_SET_SG_FSM(avd_cb,  si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
		}
	}
	TRACE_LEAVE();	
}
static void si_ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SI *si;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	si = avd_si_get(&opdata->objectName);
	assert(si != NULL);

	/* Modifications can be done for any parameters. */
	i = 0;
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		void *value;
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfSIPrefActiveAssignments")) {
			si->saAmfSIPrefActiveAssignments = *((SaUint32T *)value);
			/* Check if we need to readjust the SI assignments as PrefActiveAssignments
				got changed */
			if ( si->saAmfSIPrefActiveAssignments == si->saAmfSINumCurrActiveAssignments ) {
				TRACE("Assignments are equal updating the SI status ");
				si_update_ass_state(si);
			} else {
				avd_si_adjust_si_assignments(si);
			}
		} else if (!strcmp(attribute->attrName, "saAmfSIPrefStandbyAssignments")) {
			si->saAmfSIPrefStandbyAssignments = *((SaUint32T *)value);
			/* Check if we need to readjust the SI assignments as PrefStandbyAssignments
			   got changed */
			if ( si->saAmfSIPrefStandbyAssignments == si->saAmfSINumCurrStandbyAssignments ) {
				TRACE("Assignments are equal updating the SI status ");
				si_update_ass_state(si);
			} else {
				avd_si_adjust_si_assignments(si);
			}
		} else {
			assert(0);
		}
	}
	TRACE_LEAVE();	
}

static void si_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SI *si;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		si = si_create(&opdata->objectName, opdata->param.create.attrValues);
		assert(si);
		si_add_to_model(si);
		break;
	case CCBUTIL_DELETE:
		avd_si_delete(opdata->userData);
		break;
	case CCBUTIL_MODIFY:
		si_ccb_apply_modify_hdlr(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Update the SI assignment state. See table 11 p.88 in AMF B.04
 * IMM is updated aswell as the standby peer.
 * @param si
 */
static void si_update_ass_state(AVD_SI *si)
{
	SaAmfAssignmentStateT oldState = si->saAmfSIAssignmentState;
	SaAmfAssignmentStateT newState;

	if ((si->saAmfSINumCurrActiveAssignments == 0) && (si->saAmfSINumCurrStandbyAssignments == 0))
		newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
	else {
		switch (si->sg_of_si->sg_type->saAmfSgtRedundancyModel) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			/* fall through */
		case SA_AMF_NPM_REDUNDANCY_MODEL:
			if (si->saAmfSINumCurrActiveAssignments == 1 && si->saAmfSINumCurrStandbyAssignments == 1)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
			break;
		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			if (si->saAmfSINumCurrActiveAssignments == 1 &&
				si->saAmfSINumCurrStandbyAssignments == si->saAmfSIPrefStandbyAssignments)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
			break;
		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			if (si->saAmfSINumCurrActiveAssignments == si->saAmfSIPrefActiveAssignments &&
				si->saAmfSINumCurrStandbyAssignments == 0)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
			break;
		case SA_AMF_NO_REDUNDANCY_MODEL:
			if (si->saAmfSINumCurrActiveAssignments == 1 && si->saAmfSINumCurrStandbyAssignments == 0)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
			break;
		default:
			assert(0);
		}
	}

	if (newState != si->saAmfSIAssignmentState) {
		TRACE("'%s' %s => %s", si->name.value,
				   avd_ass_state[si->saAmfSIAssignmentState], avd_ass_state[newState]);
		saflog(LOG_NOTICE, amfSvcUsrName, "%s AssignmentState %s => %s", si->name.value,
			   avd_ass_state[si->saAmfSIAssignmentState], avd_ass_state[newState]);
		
		si->saAmfSIAssignmentState = newState;

		/* alarm & notifications */
		if (si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_UNASSIGNED) {
			avd_send_si_unassigned_alarm(&si->name);
			si->alarm_sent = TRUE;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_ALARM_SENT);
		}
		else {
			avd_send_si_assigned_ntf(&si->name, oldState, si->saAmfSIAssignmentState);
			
			/* Clear of alarm */
			if ((oldState == SA_AMF_ASSIGNMENT_UNASSIGNED) && si->alarm_sent) {
				avd_alarm_clear(&si->name, SA_AMF_NTFID_SI_UNASSIGNED, SA_NTF_SOFTWARE_ERROR);
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_ALARM_SENT);
			}

			/* always reset in case the SI has been recycled */
			si->alarm_sent = FALSE;
		}

		avd_saImmOiRtObjectUpdate(&si->name, "saAmfSIAssignmentState",
			SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAssignmentState);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_ASSIGNMENT_STATE);
	}
}

void avd_si_inc_curr_act_ass(AVD_SI *si)
{
	si->saAmfSINumCurrActiveAssignments++;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_ACTIVE);
	TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);
	si_update_ass_state(si);
}

void avd_si_dec_curr_act_ass(AVD_SI *si)
{
	assert(si->saAmfSINumCurrActiveAssignments > 0);
	si->saAmfSINumCurrActiveAssignments--;
	TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_ACTIVE);
	si_update_ass_state(si);
}

void avd_si_inc_curr_stdby_ass(AVD_SI *si)
{
	si->saAmfSINumCurrStandbyAssignments++;
	TRACE("%s saAmfSINumCurrStandbyAssignments=%u", si->name.value, si->saAmfSINumCurrStandbyAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_STBY);
	si_update_ass_state(si);
}

void avd_si_dec_curr_stdby_ass(AVD_SI *si)
{
	assert(si->saAmfSINumCurrStandbyAssignments > 0);
	si->saAmfSINumCurrStandbyAssignments--;
	TRACE("%s saAmfSINumCurrStandbyAssignments=%u", si->name.value, si->saAmfSINumCurrStandbyAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_STBY);
	si_update_ass_state(si);
}

void avd_si_inc_curr_act_dec_std_ass(AVD_SI *si)
{
        si->saAmfSINumCurrActiveAssignments++;
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_ACTIVE);
        TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);

        assert(si->saAmfSINumCurrStandbyAssignments > 0);
        si->saAmfSINumCurrStandbyAssignments--;
        TRACE("%s saAmfSINumCurrStandbyAssignments=%u", si->name.value, si->saAmfSINumCurrStandbyAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_STBY);

        si_update_ass_state(si);
}

void avd_si_inc_curr_stdby_dec_act_ass(AVD_SI *si)
{
        si->saAmfSINumCurrStandbyAssignments++;
        TRACE("%s saAmfSINumCurrStandbyAssignments=%u", si->name.value, si->saAmfSINumCurrStandbyAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_STBY);

        assert(si->saAmfSINumCurrActiveAssignments > 0);
        si->saAmfSINumCurrActiveAssignments--;
        TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_ACTIVE);

        si_update_ass_state(si);
}

void avd_si_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&si_db, &patricia_params) == NCSCC_RC_SUCCESS);
	avd_class_impl_set("SaAmfSI", si_rt_attr_cb, si_admin_op_cb, si_ccb_completed_cb, si_ccb_apply_cb);
}

void avd_si_admin_state_set(AVD_SI* si, SaAmfAdminStateT state)
{
	   SaAmfAdminStateT old_state = si->saAmfSIAdminState;
	
       assert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
       TRACE_ENTER2("%s AdmState %s => %s", si->name.value,
                  avd_adm_state_name[si->saAmfSIAdminState], avd_adm_state_name[state]);
       saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", si->name.value,
                  avd_adm_state_name[si->saAmfSIAdminState], avd_adm_state_name[state]);
       si->saAmfSIAdminState = state;
       avd_saImmOiRtObjectUpdate(&si->name,
               "saAmfSIAdminState", SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAdminState);
       m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_ADMIN_STATE);
       avd_send_admin_state_chg_ntf(&si->name, SA_AMF_NTFID_SI_ADMIN_STATE, old_state, si->saAmfSIAdminState);
}

