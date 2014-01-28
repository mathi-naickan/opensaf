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

#include <util.h>
#include <si.h>
#include <app.h>
#include <imm.h>
#include <csi.h>
#include <proc.h>
#include <si_dep.h>

static NCS_PATRICIA_TREE si_db;
static void avd_si_add_csi_db(struct avd_csi_tag* csi);
static void si_update_ass_state(AVD_SI *si);

//
// TODO(HANO) Temporary use this function instead of strdup which uses malloc.
// Later on remove this function and use std::string instead
#include <cstring>
static char *StrDup(const char *s)
{
	char *c = new char[strlen(s) + 1];
	std::strcpy(c,s);
	return c;
}

/**
 * @brief Checks if the dependencies configured leads to loop
 *	  If loop is detected amf will just osafassert
 *
 * @param name 
 * @param csi 
 */
static void osafassert_if_loops_in_csideps(SaNameT *csi_name, struct avd_csi_tag* csi)
{         
	AVD_CSI *temp_csi = NULL;
	AVD_CSI_DEPS *csi_dep_ptr;
         
	TRACE_ENTER2("%s", csi->name.value);

	/* Check if the CSI has any dependency on the csi_name
	 * if yes then loop is there and osafassert
	 */
	for(csi_dep_ptr = csi->saAmfCSIDependencies; csi_dep_ptr; csi_dep_ptr = csi_dep_ptr->csi_dep_next) {
		if (0 == memcmp(csi_name, &csi_dep_ptr->csi_dep_name_value, sizeof(SaNameT))) {
                        LOG_ER("%s: %u: Looping detected in the CSI dependencies configured for csi:%s, osafasserting",
                                __FILE__, __LINE__, csi->name.value);
                        osafassert(0);
                }
	}

	/* Check if any of the dependents of CSI has dependency on csi_name */
	for (csi_dep_ptr = csi->saAmfCSIDependencies; csi_dep_ptr; csi_dep_ptr = csi_dep_ptr->csi_dep_next) {
		for (temp_csi = csi->si->list_of_csi; temp_csi; temp_csi = temp_csi->si_list_of_csi_next) {
			if(0 == memcmp(&temp_csi->name, &csi_dep_ptr->csi_dep_name_value, sizeof(SaNameT))) {
				/* Again call the loop detection function to check whether this temp_csi
				 * has the dependency on the given csi_name
				 */
				osafassert_if_loops_in_csideps(csi_name, temp_csi);
			}
		}
	}
	TRACE_LEAVE();
}

static void avd_si_arrange_dep_csi(struct avd_csi_tag* csi)
{
	AVD_CSI *temp_csi = NULL;
	AVD_SI *temp_si = NULL;

	TRACE_ENTER2("%s", csi->name.value);

	/* Check whether any of the CSI's in the existing CSI list is dependant on the newly added CSI */
	for (temp_csi = csi->si->list_of_csi; temp_csi; temp_csi = temp_csi->si_list_of_csi_next) {
		AVD_CSI_DEPS *csi_dep_ptr;

		/* Go through the all the dependencies of exising CSI */
		for (csi_dep_ptr=temp_csi->saAmfCSIDependencies; csi_dep_ptr; csi_dep_ptr = csi_dep_ptr->csi_dep_next) {
			if ((0 == memcmp(&csi_dep_ptr->csi_dep_name_value, &csi->name, sizeof(SaNameT)))) {
				/* Try finding out any loops in the dependency configuration with this temp_csi
				 * and osafassert if any loop found
				 */
				osafassert_if_loops_in_csideps(&temp_csi->name, csi);

				/* Existing CSI is dependant on the new CSI, so its rank should be more
				 * than one of the new CSI. But increment the rank only if its rank is
				 * less than or equal the new CSI, since it  can depend on multiple CSIs
				 */
				if(temp_csi->rank <= csi->rank) {
					/* We need to rearrange Dep CSI rank as per modified. */
					/* Store the SI pointer as avd_si_remove_csi makes it NULL in the end */
					temp_csi->rank = csi->rank + 1;
					temp_si = temp_csi->si;
					avd_si_remove_csi(temp_csi);
					temp_csi->si = temp_si;
					avd_si_add_csi_db(temp_csi);
					/* We need to check whether any other CSI is dependent on temp_csi.
					 * This recursive logic is required to update the ranks of the
					 * CSIs which are dependant on the temp_csi
					 */
					avd_si_arrange_dep_csi(temp_csi);
				}
			}
		}
	}
	TRACE_LEAVE();
	return;
}

void avd_si_add_csi(struct avd_csi_tag* avd_csi)
{
	AVD_CSI *temp_csi = NULL;
        bool found = false;

	TRACE_ENTER2("%s", avd_csi->name.value);

	/* Find whether csi (avd_csi->saAmfCSIDependencies) is already in the DB. */
	if (avd_csi->rank != 1) /* (rank != 1) ==> (rank is 0). i.e. avd_csi is dependent on another CSI. */ {
		/* Check if the new CSI has any dependency on the CSI's of existing CSI list */
		for (temp_csi = avd_csi->si->list_of_csi; temp_csi; temp_csi = temp_csi->si_list_of_csi_next) {
			AVD_CSI_DEPS *csi_dep_ptr;

			/* Go through all the dependencies of new CSI */
			for (csi_dep_ptr = avd_csi->saAmfCSIDependencies; csi_dep_ptr; csi_dep_ptr = csi_dep_ptr->csi_dep_next) {
				if (0 == memcmp(&temp_csi->name, &csi_dep_ptr->csi_dep_name_value, sizeof(SaNameT))) {
					/* The new CSI is dependent on existing CSI, so its rank will be one more than
					 * the existing CSI. But increment the rank only if the new CSI rank is 
					 * less than or equal to the existing CSIs, since it can depend on multiple CSIs 
					 */
					if(avd_csi->rank <= temp_csi->rank)
						avd_csi->rank = temp_csi->rank + 1;
					found = true;
				}
			}

		}
                if (found == false) {
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

	TRACE_LEAVE();
	return;
}

static void avd_si_add_csi_db(struct avd_csi_tag* csi)
{
	AVD_CSI *i_csi = NULL;
	AVD_CSI *prev_csi = NULL;
	bool found_pos =  false;

	osafassert((csi != NULL) && (csi->si != NULL));
	i_csi = csi->si->list_of_csi;
	while ((i_csi != NULL) && (csi->rank <= i_csi->rank)) {
		while ((i_csi != NULL) && (csi->rank == i_csi->rank)) {

			if (m_CMP_HORDER_SANAMET(csi->name, i_csi->name) < 0){
				found_pos = true;
				break;
			}
			prev_csi = i_csi;
			i_csi = i_csi->si_list_of_csi_next;

			if ((i_csi != NULL) && (i_csi->rank < csi->rank)) {
				found_pos = true;
				break;
			}
		}

		if (found_pos || i_csi == NULL)
			break;
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
}

/**
 * @brief Add a SIranked SU to the SI list
 *
 * @param si
 * @param suname
 * @param saAmfRank
 */
void avd_si_add_rankedsu(AVD_SI *si, const SaNameT *suname, uint32_t saAmfRank)
{
	avd_sirankedsu_t *tmp;
	avd_sirankedsu_t *prev = NULL;
	avd_sirankedsu_t *ranked_su;
	TRACE_ENTER();

	ranked_su = new avd_sirankedsu_t;
	ranked_su->suname = *suname;
	ranked_su->saAmfRank = saAmfRank;

	for (tmp = si->rankedsu_list_head; tmp != NULL; tmp = tmp->next) {
		if (tmp->saAmfRank >= saAmfRank)
			break;
		else
			prev = tmp;
	}

	if (prev == NULL) {
		ranked_su->next = si->rankedsu_list_head;
		si->rankedsu_list_head = ranked_su;
	} else {
		ranked_su->next = prev->next;
		prev->next = ranked_su;
	}
	TRACE_LEAVE();
}
/**
 * @brief Remove a SIranked SU from the SI list
 *
 * @param si
 * @param suname
 */
void avd_si_remove_rankedsu(AVD_SI *si, const SaNameT *suname)
{
	avd_sirankedsu_t *tmp;
	avd_sirankedsu_t *prev = NULL;
	TRACE_ENTER();

	for (tmp = si->rankedsu_list_head; tmp != NULL; tmp = tmp->next) {
		if (memcmp(&tmp->suname, suname, sizeof(*suname)) == 0)
			break;
		prev = tmp;
	}

	if (prev == NULL)
		si->rankedsu_list_head = NULL;
	else
		prev->next = tmp->next;

	delete tmp;
	TRACE_LEAVE();
}

/**
 * @brief Get next SI ranked SU with lower rank than specified
 *
 * @param si
 * @param saAmfRank, 0 means get highest rank
 * 
 * @return avd_sirankedsu_t*
 */
avd_sirankedsu_t *avd_si_getnext_rankedsu(const AVD_SI *si, uint32_t saAmfRank)
{
	avd_sirankedsu_t *tmp;

	for (tmp = si->rankedsu_list_head; tmp != NULL; tmp = tmp->next) {
		if (tmp->saAmfRank > saAmfRank)
			return tmp;
	}

	return NULL;
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
			osafassert(0);
		} else {
			if (prev_csi == NULL) {
				csi->si->list_of_csi = csi->si_list_of_csi_next;
			} else {
				prev_csi->si_list_of_csi_next = csi->si_list_of_csi_next;
			}
		}

		csi->si_list_of_csi_next = NULL;
		csi->si = AVD_SI_NULL;
	}			/* if (csi->si != AVD_SI_NULL) */
}

AVD_SI *avd_si_new(const SaNameT *dn)
{
	AVD_SI *si;

	si = new AVD_SI();

	memcpy(si->name.value, dn->value, dn->length);
	si->name.length = dn->length;
	si->tree_node.key_info = (uint8_t *)&si->name;
	si->si_switch = AVSV_SI_TOGGLE_STABLE;
	si->saAmfSIAdminState = SA_AMF_ADMIN_UNLOCKED;
	si->si_dep_state = AVD_SI_NO_DEPENDENCY;
	si->saAmfSIAssignmentState = SA_AMF_ASSIGNMENT_UNASSIGNED;
	si->alarm_sent = false;
	si->num_dependents = 0;
	si->tol_timer_count = 0;

	return si;
}

/**
 * Deletes all the CSIs under this AMF SI object
 * 
 * @param si
 */
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
}

void avd_si_delete(AVD_SI *si)
{
	unsigned int rc;

	TRACE_ENTER2("%s", si->name.value);

	/* All CSI under this should have been deleted by now on the active 
	   director and on standby all the csi should be deleted because 
	   csi delete is not checkpointed as and when it happens */
	si_delete_csis(si);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, si, AVSV_CKPT_AVD_SI_CONFIG);
	avd_svctype_remove_si(si);
	avd_app_remove_si(si->app, si);
	avd_sg_remove_si(si->sg_of_si, si);

	// clear any pending alarms for this SI
	if ((si->alarm_sent == true) &&
			(avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)) {
		avd_alarm_clear(&si->name, SA_AMF_NTFID_SI_UNASSIGNED,
				SA_NTF_SOFTWARE_ERROR);
	}

	rc = ncs_patricia_tree_del(&si_db, &si->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
	delete si;
}
/**
 * @brief    Deletes all assignments on a particular SI
 *
 * @param[in] cb - the AvD control block
 * @param[in] si - SI for which susi need to be deleted
 *
 * @return    NULL
 *
 */
void avd_si_assignments_delete(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *sisu = si->list_of_sisu;
	TRACE_ENTER2(" '%s'", si->name.value);

	for (; sisu != NULL; sisu = sisu->si_next) {
		if(sisu->fsm != AVD_SU_SI_STATE_UNASGN)
			if (avd_susi_del_send(sisu) == NCSCC_RC_SUCCESS)
				avd_sg_su_oper_list_add(cb, sisu->su, false);
	}
	TRACE_LEAVE();
}

void avd_si_db_add(AVD_SI *si)
{
	unsigned int rc;

	if (avd_si_get(&si->name) == NULL) {
		rc = ncs_patricia_tree_add(&si_db, &si->tree_node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_SI *avd_si_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	if (dn->length > SA_MAX_NAME_LENGTH)
		return NULL;

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SI *)ncs_patricia_tree_get(&si_db, (uint8_t *)&tmp);
}

AVD_SI *avd_si_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	if (dn->length > SA_MAX_NAME_LENGTH)
		return NULL;

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SI *)ncs_patricia_tree_getnext(&si_db, (uint8_t *)&tmp);
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
	avd_saImmOiRtObjectUpdate(&si->name, const_cast<SaImmAttrNameT>("saAmfSIAssignmentState"),
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
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safApp=", 7) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSvcType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (avd_svctype_get(&aname) == NULL) {
		/* SVC type does not exist in current model, check CCB if passed as param */
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIProtectedbySG"), attributes, 0, &aname) != SA_AIS_OK) {
		report_ccb_validation_error(opdata, "saAmfSIProtectedbySG not specified for '%s'", dn->value);
		return 0;
	}

	if (avd_sg_get(&aname) == NULL) {
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist", aname.value);
			return 0;
		}
		
		/* SG does not exist in current model, check CCB */
		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSIAdminState %u for '%s'", admstate, dn->value);
		return 0;
	}

	return 1;
}

static AVD_SI *si_create(SaNameT *si_name, const SaImmAttrValuesT_2 **attributes)
{
	unsigned int i;
	int rc = -1;
	AVD_SI *si;
	AVD_SU_SI_REL *sisu;
	AVD_COMP_CSI_REL *compcsi, *temp;
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

		/* delete the corresponding compcsi configuration also */
		sisu = si->list_of_sisu;
		while (sisu != NULL) {
			compcsi = sisu->list_of_csicomp;
			while (compcsi != NULL) {
				temp = compcsi;
				compcsi = compcsi->susi_csicomp_next;
				delete temp;
			}
			sisu->list_of_csicomp = NULL;
			sisu = sisu->si_next;
		}
	}

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSvcType"), attributes, 0, &si->saAmfSvcType);
	osafassert(error == SA_AIS_OK);

	/* Optional, strange... */
	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIProtectedbySG"), attributes, 0, &si->saAmfSIProtectedbySG);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIRank"), attributes, 0, &si->saAmfSIRank) != SA_AIS_OK) {
		/* Empty, assign default value (highest number => lowest rank) */
		si->saAmfSIRank = ~0U;
	}

	/* If 0 (zero), treat as lowest possible rank. Should be a positive integer */
	if (si->saAmfSIRank == 0) {
		si->saAmfSIRank = ~0U;
		TRACE("'%s' saAmfSIRank auto-changed to lowest", si->name.value);
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSIActiveWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		si->saAmfSIActiveWeight = new char*[attrValuesNumber];
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIActiveWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSIActiveWeight", i));
		}
	} else {
		/*  TODO */
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSIStandbyWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		si->saAmfSIStandbyWeight = new char*[attrValuesNumber];
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIStandbyWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSIStandbyWeight", i));
		}
	} else {
		/*  TODO */
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIPrefActiveAssignments"), attributes, 0,
			    &si->saAmfSIPrefActiveAssignments) != SA_AIS_OK) {

		/* Empty, assign default value */
		si->saAmfSIPrefActiveAssignments = 1;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIPrefStandbyAssignments"), attributes, 0,
			    &si->saAmfSIPrefStandbyAssignments) != SA_AIS_OK) {

		/* Empty, assign default value */
		si->saAmfSIPrefStandbyAssignments = 1;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIAdminState"), attributes, 0, &si->saAmfSIAdminState) != SA_AIS_OK) {
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
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT si_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSI";
	AVD_SI *si;
	SaImmAttrNameT configAttributes[] = {
		const_cast<SaImmAttrNameT>("saAmfSvcType"),
		const_cast<SaImmAttrNameT>("saAmfSIProtectedbySG"),
		const_cast<SaImmAttrNameT>("saAmfSIRank"),
		const_cast<SaImmAttrNameT>("saAmfSIActiveWeight"),
		const_cast<SaImmAttrNameT>("saAmfSIStandbyWeight"),
		const_cast<SaImmAttrNameT>("saAmfSIPrefActiveAssignments"),
		const_cast<SaImmAttrNameT>("saAmfSIPrefStandbyAssignments"),
		const_cast<SaImmAttrNameT>("saAmfSIAdminState"),
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, &app->name, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
	      configAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &si_name,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
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

	osafassert(rc == SA_AIS_ERR_NOT_EXIST);
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	si = avd_si_get(&opdata->objectName);
	osafassert(si != NULL);

	/* Modifications can only be done for these attributes. */
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

		if (!strcmp(attribute->attrName, "saAmfSIPrefActiveAssignments")) {
			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				report_ccb_validation_error(opdata, "SG'%s' is not stable (%u)", si->sg_of_si->name.value,
						si->sg_of_si->sg_fsm_state);
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
			if (si->sg_of_si->sg_redundancy_model != SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) {
				report_ccb_validation_error(opdata, "Invalid modification,saAmfSIPrefActiveAssignments can"
						" be updated only for N_WAY_ACTIVE_REDUNDANCY_MODEL");
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}

		} else if (!strcmp(attribute->attrName, "saAmfSIPrefStandbyAssignments")) {
			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				report_ccb_validation_error(opdata, "SG'%s' is not stable (%u)", si->sg_of_si->name.value,
						si->sg_of_si->sg_fsm_state);
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
			if( si->sg_of_si->sg_redundancy_model != SA_AMF_N_WAY_REDUNDANCY_MODEL ) {
				report_ccb_validation_error(opdata, "Invalid modification,saAmfSIPrefStandbyAssignments"
						" can be updated only for N_WAY_REDUNDANCY_MODEL");
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}
		} else {
			report_ccb_validation_error(opdata, "Modification of attribute %s is not supported",
					attribute->attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			break;
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void si_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *objectName, SaImmAdminOperationIdT operationId,
	const SaImmAdminOperationParamsT_2 **params)
{
	uint32_t err = NCSCC_RC_FAILURE;
	AVD_SI *si;
	SaAmfAdminStateT adm_state = static_cast<SaAmfAdminStateT>(0);
	SaAmfAdminStateT back_val;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("%s op=%llu", objectName->value, operationId);

	si = avd_si_get(objectName);
	
	if ((operationId != SA_AMF_ADMIN_SI_SWAP) && (si->sg_of_si->sg_ncs_spec == SA_TRUE)) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"Admin operation %llu on MW SI is not allowed", operationId);
		goto done;
	}
	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
        if (sg_is_tolerance_timer_running_for_any_si(si->sg_of_si)) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"Tolerance timer is running for some of the SI's in the SG '%s', "
				"so differing admin opr",si->sg_of_si->name.value);
		goto done;
        }

        /* Avoid if any single Csi assignment is undergoing on SG. */
        if (csi_assignment_validate(si->sg_of_si) == true) {
                rc = SA_AIS_ERR_TRY_AGAIN;
                LOG_WA("Single Csi assignment undergoing on (sg'%s')", si->sg_of_si->name.value);
                goto done;
        }

	switch (operationId) {
	case SA_AMF_ADMIN_UNLOCK:
		if (SA_AMF_ADMIN_UNLOCKED == si->saAmfSIAdminState) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"SI unlock of %s failed, already unlocked", objectName->value);
			goto done;
		}

		if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"SI unlock of %s failed, SG not stable", objectName->value);
			goto done;
		}

		avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);

		err = si->sg_of_si->si_func(avd_cb, si);
		if (err != NCSCC_RC_SUCCESS) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"SI unlock of %s failed", objectName->value);
			avd_si_admin_state_set(si, SA_AMF_ADMIN_LOCKED);
			goto done;
		}

		break;

	case SA_AMF_ADMIN_SHUTDOWN:
		if (SA_AMF_ADMIN_SHUTTING_DOWN == si->saAmfSIAdminState) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"SI unlock of %s failed, already shutting down", objectName->value);
			goto done;
		}

		if ((SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_LOCKED_INSTANTIATION == si->saAmfSIAdminState)) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"SI unlock of %s failed, is locked (instantiation)", objectName->value);
			goto done;
		}
		adm_state = SA_AMF_ADMIN_SHUTTING_DOWN;

		/* Don't break */

	case SA_AMF_ADMIN_LOCK:
		if (0 == adm_state)
			adm_state = SA_AMF_ADMIN_LOCKED;

		if (SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"SI lock of %s failed, already locked", objectName->value);
			goto done;
		}

		if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
			avd_si_admin_state_set(si, SA_AMF_ADMIN_LOCKED);
			/* This may happen when SUs are locked before SI is locked. */
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
				report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
						"'%s' other semantics...", objectName->value);
				goto done;
			}
		}

		back_val = si->saAmfSIAdminState;
		avd_si_admin_state_set(si, (adm_state));

		err = si->sg_of_si->si_admin_down(avd_cb, si);
		if (err != NCSCC_RC_SUCCESS) {
			avd_si_admin_state_set(si, back_val);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"SI shutdown/lock of %s failed", objectName->value);
			goto done;
		}

		break;

	case SA_AMF_ADMIN_SI_SWAP:
		if (si->sg_of_si->si_swap != NULL) {
			rc = si->sg_of_si->si_swap(si, invocation);
			if (rc != SA_AIS_OK) {
				report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
						"SI Swap of %s failed", objectName->value);
				goto done;
			}
		} else {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
					"SI SWAP of %s failed, not supported for redundancy model",
					objectName->value);
			goto done;
		}
		break;

	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"SI Admin operation %llu not supported", operationId);
		goto done;
	}

	if ((operationId != SA_AMF_ADMIN_SI_SWAP))
		avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
done:
	TRACE_LEAVE();
}

static SaAisErrorT si_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_SI *si = avd_si_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("%s", objectName->value);
	osafassert(si != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfSINumCurrActiveAssignments", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &si->saAmfSINumCurrActiveAssignments);
		} else if (!strcmp("saAmfSINumCurrStandbyAssignments", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &si->saAmfSINumCurrStandbyAssignments);
		} else
			osafassert(0);
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
			report_ccb_validation_error(opdata, "SaAmfSI is in use '%s'", si->name.value);
			goto done;
		}
		/* check for any SI-SI dependency configurations */
		if (0 != si->num_dependents || si->spons_si_list != NULL) {
			report_ccb_validation_error(opdata, "Sponsors or Dependents Exist; Cannot delete '%s'",
					si->name.value);
			goto done;
		}
		rc = SA_AIS_OK;
		opdata->userData = si;	/* Save for later use in apply */
		break;
	default:
		osafassert(0);
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
static void avd_si_adjust_si_assignments(AVD_SI *si, uint32_t mod_pref_assignments)
{
	AVD_SU_SI_REL *sisu, *tmp_sisu;
	uint32_t no_of_sisus_to_delete;
	uint32_t i = 0;

	TRACE_ENTER2("for SI:%s ", si->name.value);
	
	if( si->sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL ) {
		if( mod_pref_assignments > si->saAmfSINumCurrActiveAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if ( avd_sg_nacvred_su_chose_asgn(avd_cb, si->sg_of_si ) != NULL ) {
				/* New assignments are been done in the SG.
                                   change the SG FSM state to AVD_SG_FSM_SG_REALIGN */
				m_AVD_SET_SG_FSM(avd_cb, si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
			} else {
				/* No New assignments are been done in the SG
				   reason might be no more inservice SUs to take new assignments or
				   SUs are already assigned to saAmfSGMaxActiveSIsperSU capacity */
				si_update_ass_state(si);
				TRACE("No New assignments are been done SI:%s",si->name.value);
			}
		} else {
			no_of_sisus_to_delete = si->saAmfSINumCurrActiveAssignments -
						mod_pref_assignments;

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
				if (avd_susi_mod_send(sisu, SA_AMF_HA_QUIESCED) == NCSCC_RC_SUCCESS) {
					/* Add SU to su_opr_list */
                                	avd_sg_su_oper_list_add(avd_cb, sisu->su, false);
				}
				sisu = sisu->si_next;
			}
			/* Change the SG FSM to AVD_SG_FSM_SG_REALIGN SG to Stable state */
			m_AVD_SET_SG_FSM(avd_cb,  si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
		}
	} 
	if( si->sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL ) {
		if( mod_pref_assignments > si->saAmfSINumCurrStandbyAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if( avd_sg_nway_si_assign(avd_cb, si->sg_of_si ) == NCSCC_RC_FAILURE ) {
				LOG_ER("SI new assignmemts failed  SI:%s",si->name.value);
			} 
		} else {
			no_of_sisus_to_delete = 0;
			no_of_sisus_to_delete = si->saAmfSINumCurrStandbyAssignments -
						mod_pref_assignments; 

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
				if (avd_susi_del_send(sisu) == NCSCC_RC_SUCCESS) {
					/* Add SU to su_opr_list */
                                	avd_sg_su_oper_list_add(avd_cb, sisu->su, false);
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
	bool value_is_deleted;
	uint32_t mod_pref_assignments;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	si = avd_si_get(&opdata->objectName);
	osafassert(si != NULL);

	/* Modifications can be done for any parameters. */
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == NULL))
			value_is_deleted = true;
		else
			value_is_deleted = false;

		if (!strcmp(attribute->attrName, "saAmfSIPrefActiveAssignments")) {

			if (value_is_deleted)
				mod_pref_assignments = si->saAmfSIPrefActiveAssignments = 1;
			else
				mod_pref_assignments = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);

			if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
				si->saAmfSIPrefActiveAssignments = mod_pref_assignments;
				continue;
			}

			/* Check if we need to readjust the SI assignments as PrefActiveAssignments
				got changed */
			if ( mod_pref_assignments == si->saAmfSINumCurrActiveAssignments ) {
				TRACE("Assignments are equal updating the SI status ");
				si->saAmfSIPrefActiveAssignments = mod_pref_assignments;
			} else if (mod_pref_assignments > si->saAmfSINumCurrActiveAssignments) {
				si->saAmfSIPrefActiveAssignments = mod_pref_assignments;
				avd_si_adjust_si_assignments(si, mod_pref_assignments);
			} else if (mod_pref_assignments < si->saAmfSINumCurrActiveAssignments) {
				avd_si_adjust_si_assignments(si, mod_pref_assignments);
				si->saAmfSIPrefActiveAssignments = mod_pref_assignments;
			}
			TRACE("Modified saAmfSIPrefActiveAssignments is '%u'", si->saAmfSIPrefActiveAssignments);
			si_update_ass_state(si);
		} else if (!strcmp(attribute->attrName, "saAmfSIPrefStandbyAssignments")) {
			if (value_is_deleted)
				mod_pref_assignments = si->saAmfSIPrefStandbyAssignments = 1;
			else
				mod_pref_assignments =
					*((SaUint32T *)attr_mod->modAttr.attrValues[0]);

			if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
				si->saAmfSIPrefStandbyAssignments = mod_pref_assignments;
				continue;
			}

			/* Check if we need to readjust the SI assignments as PrefStandbyAssignments
			   got changed */
			if ( mod_pref_assignments == si->saAmfSINumCurrStandbyAssignments ) {
				TRACE("Assignments are equal updating the SI status ");
				si->saAmfSIPrefStandbyAssignments = mod_pref_assignments;
			} else if (mod_pref_assignments > si->saAmfSINumCurrStandbyAssignments) {
				si->saAmfSIPrefStandbyAssignments = mod_pref_assignments;
				avd_si_adjust_si_assignments(si, mod_pref_assignments);
			} else if (mod_pref_assignments < si->saAmfSINumCurrStandbyAssignments) {
				avd_si_adjust_si_assignments(si, mod_pref_assignments);
				si->saAmfSIPrefStandbyAssignments = mod_pref_assignments;
			}
			TRACE("Modified saAmfSINumCurrStandbyAssignments is '%u'", si->saAmfSINumCurrStandbyAssignments);
			si_update_ass_state(si);
		} else {
			osafassert(0);
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
		osafassert(si);
		si_add_to_model(si);
		break;
	case CCBUTIL_DELETE:
		avd_si_delete(static_cast<AVD_SI*>(opdata->userData));
		break;
	case CCBUTIL_MODIFY:
		si_ccb_apply_modify_hdlr(opdata);
		break;
	default:
		osafassert(0);
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
	SaAmfAssignmentStateT newState = SA_AMF_ASSIGNMENT_UNASSIGNED;

	/*
	** Note: for SA_AMF_2N_REDUNDANCY_MODEL, SA_AMF_NPM_REDUNDANCY_MODEL &
	** SA_AMF_N_WAY_REDUNDANCY_MODEL it is not possible to check:
	** osafassert(si->saAmfSINumCurrActiveAssignments == 1);
	** since AMF temporarily goes above the limit during fail over
	 */
	switch (si->sg_of_si->sg_type->saAmfSgtRedundancyModel) {
	case SA_AMF_2N_REDUNDANCY_MODEL:
		/* fall through */
	case SA_AMF_NPM_REDUNDANCY_MODEL:
		if ((si->saAmfSINumCurrActiveAssignments == 0) &&
				(si->saAmfSINumCurrStandbyAssignments == 0)) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else if ((si->saAmfSINumCurrActiveAssignments == 1) &&
				(si->saAmfSINumCurrStandbyAssignments == 1)) {
			newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		} else
			newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;

		break;
	case SA_AMF_N_WAY_REDUNDANCY_MODEL:
		if ((si->saAmfSINumCurrActiveAssignments == 0) &&
				(si->saAmfSINumCurrStandbyAssignments == 0)) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else if ((si->saAmfSINumCurrActiveAssignments == 1) &&
		           (si->saAmfSINumCurrStandbyAssignments == si->saAmfSIPrefStandbyAssignments)) {
		        newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		} else
		        newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
                break;
	case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
		if (si->saAmfSINumCurrActiveAssignments == 0) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else {
			osafassert(si->saAmfSINumCurrStandbyAssignments == 0);
			osafassert(si->saAmfSINumCurrActiveAssignments <= si->saAmfSIPrefActiveAssignments);
			if (si->saAmfSINumCurrActiveAssignments == si->saAmfSIPrefActiveAssignments)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
		}
		break;
	case SA_AMF_NO_REDUNDANCY_MODEL:
		if (si->saAmfSINumCurrActiveAssignments == 0) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else {
			osafassert(si->saAmfSINumCurrActiveAssignments == 1);
			osafassert(si->saAmfSINumCurrStandbyAssignments == 0);
			newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		}
		break;
	default:
		osafassert(0);
	}

	if (newState != si->saAmfSIAssignmentState) {
		TRACE("'%s' %s => %s", si->name.value,
				   avd_ass_state[si->saAmfSIAssignmentState], avd_ass_state[newState]);
		#if 0
		saflog(LOG_NOTICE, amfSvcUsrName, "%s AssignmentState %s => %s", si->name.value,
			   avd_ass_state[si->saAmfSIAssignmentState], avd_ass_state[newState]);
		#endif
		
		si->saAmfSIAssignmentState = newState;

		/* alarm & notifications */
		if (si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_UNASSIGNED) {
			avd_send_si_unassigned_alarm(&si->name);
			si->alarm_sent = true;
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
			si->alarm_sent = false;
		}

		avd_saImmOiRtObjectUpdate(&si->name, const_cast<SaImmAttrNameT>("saAmfSIAssignmentState"),
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
	osafassert(si->saAmfSINumCurrActiveAssignments > 0);
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
	osafassert(si->saAmfSINumCurrStandbyAssignments > 0);
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

        osafassert(si->saAmfSINumCurrStandbyAssignments > 0);
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

        osafassert(si->saAmfSINumCurrActiveAssignments > 0);
        si->saAmfSINumCurrActiveAssignments--;
        TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_SU_CURR_ACTIVE);

        si_update_ass_state(si);
}

void avd_si_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&si_db, &patricia_params) == NCSCC_RC_SUCCESS);
	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfSI"), si_rt_attr_cb, si_admin_op_cb, si_ccb_completed_cb, si_ccb_apply_cb);
}

void avd_si_admin_state_set(AVD_SI* si, SaAmfAdminStateT state)
{
	   SaAmfAdminStateT old_state = si->saAmfSIAdminState;
	
       osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
       TRACE_ENTER2("%s AdmState %s => %s", si->name.value,
                  avd_adm_state_name[si->saAmfSIAdminState], avd_adm_state_name[state]);
       saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", si->name.value,
                  avd_adm_state_name[si->saAmfSIAdminState], avd_adm_state_name[state]);
       si->saAmfSIAdminState = state;
       avd_saImmOiRtObjectUpdate(&si->name,
				 const_cast<SaImmAttrNameT>("saAmfSIAdminState"), SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAdminState);
       m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_ADMIN_STATE);
       avd_send_admin_state_chg_ntf(&si->name, SA_AMF_NTFID_SI_ADMIN_STATE, old_state, si->saAmfSIAdminState);
}

