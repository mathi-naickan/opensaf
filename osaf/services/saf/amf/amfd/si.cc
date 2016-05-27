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

AmfDb<std::string, AVD_SI> *si_db = NULL;

/**
 * @brief Checks if the dependencies configured leads to loop
 *	  If loop is detected amf will just osafassert
 *
 * @param name 
 * @param csi 
 */
static void osafassert_if_loops_in_csideps(SaNameT *csi_name, AVD_CSI* csi)
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

void AVD_SI::arrange_dep_csi(AVD_CSI* csi)
{
	AVD_CSI *temp_csi = NULL;

	TRACE_ENTER2("%s", csi->name.value);
	
	osafassert(csi->si == this);

	/* Check whether any of the CSI's in the existing CSI list is dependant on the newly added CSI */
	for (temp_csi = list_of_csi; temp_csi; temp_csi = temp_csi->si_list_of_csi_next) {
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
					temp_csi->rank = csi->rank + 1;
					remove_csi(temp_csi);
					add_csi_db(temp_csi);
					/* We need to check whether any other CSI is dependent on temp_csi.
					 * This recursive logic is required to update the ranks of the
					 * CSIs which are dependant on the temp_csi
					 */
					arrange_dep_csi(temp_csi);
				}
			}
		}
	}
	TRACE_LEAVE();
	return;
}

void AVD_SI::add_csi(AVD_CSI* avd_csi)
{
	AVD_CSI *temp_csi = NULL;
        bool found = false;

	TRACE_ENTER2("%s", avd_csi->name.value);
	
	osafassert(avd_csi->si == this);

	/* Find whether csi (avd_csi->saAmfCSIDependencies) is already in the DB. */
	if (avd_csi->rank != 1) /* (rank != 1) ==> (rank is 0). i.e. avd_csi is dependent on another CSI. */ {
		/* Check if the new CSI has any dependency on the CSI's of existing CSI list */
		for (temp_csi = list_of_csi; temp_csi; temp_csi = temp_csi->si_list_of_csi_next) {
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
        arrange_dep_csi(avd_csi);
add_csi:
        add_csi_db(avd_csi);

	TRACE_LEAVE();
	return;
}

void AVD_SI::add_csi_db(AVD_CSI* csi)
{
	TRACE_ENTER2("%s", csi->name.value);

	AVD_CSI *i_csi = NULL;
	AVD_CSI *prev_csi = NULL;
	bool found_pos =  false;

	osafassert((csi != NULL) && (csi->si != NULL));
	osafassert(csi->si == this);

	i_csi = list_of_csi;
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
		csi->si_list_of_csi_next = list_of_csi;
		list_of_csi = csi;
	} else {
		prev_csi->si_list_of_csi_next = csi;
		csi->si_list_of_csi_next = i_csi;
	}

	TRACE_LEAVE();
}

/**
 * @brief Add a SIranked SU to the SI list
 *
 * @param si
 * @param suname
 * @param saAmfRank
 */
void AVD_SI::add_rankedsu(const SaNameT *suname, uint32_t saAmfRank)
{
	avd_sirankedsu_t *tmp;
	avd_sirankedsu_t *prev = NULL;
	avd_sirankedsu_t *ranked_su;
	TRACE_ENTER();

	ranked_su = new avd_sirankedsu_t;
	ranked_su->suname = *suname;
	ranked_su->saAmfRank = saAmfRank;

	for (tmp = rankedsu_list_head; tmp != NULL; tmp = tmp->next) {
		if (tmp->saAmfRank >= saAmfRank)
			break;
		else
			prev = tmp;
	}

	if (prev == NULL) {
		ranked_su->next = rankedsu_list_head;
		rankedsu_list_head = ranked_su;
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
void AVD_SI::remove_rankedsu(const SaNameT *suname)
{
	avd_sirankedsu_t *tmp;
	avd_sirankedsu_t *prev = NULL;
	TRACE_ENTER();

	for (tmp = rankedsu_list_head; tmp != NULL; tmp = tmp->next) {
		if (memcmp(&tmp->suname, suname, sizeof(*suname)) == 0)
			break;
		prev = tmp;
	}

	if (prev == NULL)
		rankedsu_list_head = NULL;
	else
		prev->next = tmp->next;

	delete tmp;
	TRACE_LEAVE();
}

void AVD_SI::remove_csi(AVD_CSI* csi)
{
	AVD_CSI *i_csi = NULL;
	AVD_CSI *prev_csi = NULL;
	
	osafassert(csi->si == this);

	/* remove CSI from the SI */
	prev_csi = NULL;
	i_csi = list_of_csi;

	// find 'csi'
	while ((i_csi != NULL) && (i_csi != csi)) {
		prev_csi = i_csi;
		i_csi = i_csi->si_list_of_csi_next;
	}

	if (i_csi != csi) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (prev_csi == NULL) {
			list_of_csi = csi->si_list_of_csi_next;
		} else {
			prev_csi->si_list_of_csi_next = csi->si_list_of_csi_next;
		}
	}

	csi->si_list_of_csi_next = NULL;
}


AVD_SI::AVD_SI() :
	saAmfSIRank(0),
	saAmfSIPrefActiveAssignments(0),
	saAmfSIPrefStandbyAssignments(0),
	saAmfSIAdminState(SA_AMF_ADMIN_UNLOCKED),
	saAmfSIAssignmentState(SA_AMF_ASSIGNMENT_UNASSIGNED),
	saAmfSINumCurrActiveAssignments(0),
	saAmfSINumCurrStandbyAssignments(0),
	si_switch(AVSV_SI_TOGGLE_STABLE),
	sg_of_si(NULL),
	list_of_csi(NULL),
	list_of_sisu(NULL),
	si_dep_state(AVD_SI_NO_DEPENDENCY),
	spons_si_list(NULL),
	num_dependents(0),
	tol_timer_count(0),
	svc_type(NULL),
	app(NULL),
	si_list_app_next(NULL),
	list_of_sus_per_si_rank(NULL),
	rankedsu_list_head(NULL),
	invocation(0),
	alarm_sent(false)
{
	memset(&name, 0, sizeof(SaNameT));
	memset(&saAmfSvcType, 0, sizeof(SaNameT));
	memset(&saAmfSIProtectedbySG, 0, sizeof(SaNameT));
}

AVD_SI *avd_si_new(const SaNameT *dn)
{
	AVD_SI *si;

	si = new AVD_SI();

	memcpy(si->name.value, dn->value, dn->length);
	si->name.length = dn->length;

	return si;
}

/**
 * Deletes all the CSIs under this AMF SI object
 * 
 * @param si
 */
void AVD_SI::delete_csis()
{
	AVD_CSI *csi, *temp;

	csi = list_of_csi; 
	while (csi != NULL) {
		temp = csi;
		csi = csi->si_list_of_csi_next;
		avd_csi_delete(temp);
	}

	list_of_csi = NULL;
}

void avd_si_delete(AVD_SI *si)
{
	TRACE_ENTER2("%s", si->name.value);

	/* All CSI under this should have been deleted by now on the active 
	   director and on standby all the csi should be deleted because 
	   csi delete is not checkpointed as and when it happens */
	si->delete_csis();
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, si, AVSV_CKPT_AVD_SI_CONFIG);
	avd_svctype_remove_si(si);
	si->app->remove_si(si);
	si->sg_of_si->remove_si(si);

	// clear any pending alarms for this SI
	if ((si->alarm_sent == true) &&
			(avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)) {
		avd_alarm_clear(&si->name, SA_AMF_NTFID_SI_UNASSIGNED,
				SA_NTF_SOFTWARE_ERROR);
	}

	si_db->erase(Amf::to_string(&si->name));
	
	delete si;
}
/**
 * @brief    Deletes all assignments
 *
 * @param[in] cb - the AvD control block
 *
 * @return    NULL
 *
 */
void AVD_SI::delete_assignments(AVD_CL_CB *cb)
{
	AVD_SU_SI_REL *sisu = list_of_sisu;
	TRACE_ENTER2(" '%s'", name.value);

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

	if (si_db->find(Amf::to_string(&si->name)) == NULL) {
		rc = si_db->insert(Amf::to_string(&si->name), si);
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

	return si_db->find(Amf::to_string(dn)); 
}



void AVD_SI::si_add_to_model()
{
	SaNameT dn;

	TRACE_ENTER2("%s", name.value);

	/* Check parent link to see if it has been added already */
	if (svc_type != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&name, &dn, "safApp");
	app = app_db->find(Amf::to_string(&dn));

	avd_si_db_add(this);

	svc_type = svctype_db->find(Amf::to_string(&saAmfSvcType));

	if (saAmfSIProtectedbySG.length > 0)
		sg_of_si = sg_db->find(Amf::to_string(&saAmfSIProtectedbySG));

	avd_svctype_add_si(this);
	app->add_si(this);
	sg_of_si->add_si(this);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, this, AVSV_CKPT_AVD_SI_CONFIG);
	avd_saImmOiRtObjectUpdate(&name, "saAmfSIAssignmentState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSIAssignmentState);

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
	char *parent, *app;

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

	if (svctype_db->find(Amf::to_string(&aname)) == NULL) {
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

	if (sg_db->find(Amf::to_string(&aname)) == NULL) {
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

	/* saAmfSIProtectedbySG and SI should belong to the same applicaion. */
	if ((app = strchr((char*)aname.value, ',')) == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", aname.value);
		return 0;
	}
	if (strcmp(parent, ++app)) {
		report_ccb_validation_error(opdata, "SI '%s' and SG '%s' belong to different application",
				dn->value, aname.value);
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSIAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
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
	if ((si = si_db->find(Amf::to_string(si_name))) == NULL) {
		if ((si = avd_si_new(si_name)) == NULL)
			goto done;
	} else {
		TRACE("already created, refreshing config...");
		/* here delete the whole csi configuration to refresh 
		   since csi deletes are not checkpointed there may be 
		   some CSIs that are deleted from previous data sync up */
		si->delete_csis();

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
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIActiveWeight.push_back(
				std::string(immutil_getStringAttr(attributes, "saAmfSIActiveWeight", i))
			);
		}
	} else {
		/*  TODO */
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSIStandbyWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		for (i = 0; i < attrValuesNumber; i++) {
			si->saAmfSIStandbyWeight.push_back(
				std::string(immutil_getStringAttr(attributes, "saAmfSIStandbyWeight", i))
			);
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

	TRACE_LEAVE();
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

		si->si_add_to_model();

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
	bool value_is_deleted;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	si = avd_si_get(&opdata->objectName);
	osafassert(si != NULL);

	/* Modifications can only be done for these attributes. */
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		//void *value = NULL;

		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL)) {
			/* Attribute value is deleted, revert to default value */
			value_is_deleted = true;
		} else {
			/* Attribute value is modified */
			value_is_deleted = false;
			//value = attribute->attrValues[0];
		}

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
		} else if (!strcmp(attribute->attrName, "saAmfSIRank")) {
			if (value_is_deleted == true)
				continue;
				
			SaUint32T sirank = *(SaUint32T*)attribute->attrValues[0];

			if (!si->is_sirank_valid(sirank)) {
				report_ccb_validation_error(opdata, "saAmfSIRank(%u) is invalid due to SI Dependency rules", sirank);
				rc = SA_AIS_ERR_BAD_OPERATION;
				break;
			}

			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				report_ccb_validation_error(opdata, "SG'%s' is not stable (%u)", si->sg_of_si->name.value,
						si->sg_of_si->sg_fsm_state);
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
	
	if ((operationId != SA_AMF_ADMIN_SI_SWAP) && (si->sg_of_si->sg_ncs_spec == true)) {
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
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"Single Csi assignment undergoing on (sg'%s')", si->sg_of_si->name.value);
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

		si->set_admin_state(SA_AMF_ADMIN_UNLOCKED);

		if (avd_cb->init_state == AVD_INIT_DONE) {
			/* Assignments will be delivered once cluster timer expires.*/
			rc = SA_AIS_OK;
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
			goto done;
		}

		err = si->sg_of_si->si_assign(avd_cb, si);
		if (si->list_of_sisu == NULL) {
			LOG_NO("'%s' could not be assigned to any SU", si->name.value);
			rc = SA_AIS_OK;
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
			goto done;
		}

		if ((err != NCSCC_RC_SUCCESS) && (si->list_of_sisu == NULL)) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"SI unlock of %s failed", objectName->value);
			si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			goto done;
		}
		si->invocation = invocation;
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

		if ((si->list_of_sisu == AVD_SU_SI_REL_NULL) || (avd_cb->init_state == AVD_INIT_DONE)) {
			si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			/* This may happen when SUs are locked before SI is locked. */
			LOG_WA("SI lock of %s, has no assignments", objectName->value);
			rc = SA_AIS_OK;
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
			goto done;
		}

		/* Check if other semantics are happening for other SUs. If yes
		 * return an error.
		 */
		if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
			if ((si->sg_of_si->sg_fsm_state != AVD_SG_FSM_SI_OPER) ||
			    (si->saAmfSIAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) ||
			    (adm_state != SA_AMF_ADMIN_LOCKED)) {
				report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
						"SI lock of '%s' failed, SG not stable", objectName->value);
				goto done;
			} else {
				report_admin_op_error(immOiHandle, si->invocation, SA_AIS_ERR_INTERRUPT, NULL,
						"SI lock has been issued '%s'", objectName->value);
				si->invocation = 0;
			}
		}

		back_val = si->saAmfSIAdminState;
		si->set_admin_state(adm_state);

		err = si->sg_of_si->si_admin_down(avd_cb, si);
		if (err != NCSCC_RC_SUCCESS) {
			si->set_admin_state(back_val);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"SI shutdown/lock of %s failed", objectName->value);
			goto done;
		}
		si->invocation = invocation;

		break;

	case SA_AMF_ADMIN_SI_SWAP:
		rc = si->sg_of_si->si_swap(si, invocation);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(immOiHandle, invocation,
				rc, NULL,
				"SI Swap of %s failed, syslog has details", objectName->value);
			goto done;
		} else
			;  // response done later
		break;

	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"SI Admin operation %llu not supported", operationId);
		goto done;
	}

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
		} else {
			LOG_ER("Ignoring unknown attribute '%s'", attributeName);
		}
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
 * @return void 
 */
void AVD_SI::adjust_si_assignments(const uint32_t mod_pref_assignments)
{
	AVD_SU_SI_REL *sisu, *tmp_sisu;
	uint32_t no_of_sisus_to_delete;
	uint32_t i = 0;

	TRACE_ENTER2("for SI:%s ", name.value);
	
	if( sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL ) {
		if( mod_pref_assignments > saAmfSINumCurrActiveAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if ( avd_sg_nacvred_su_chose_asgn(avd_cb, sg_of_si ) != NULL ) {
				/* New assignments are been done in the SG.
                                   change the SG FSM state to AVD_SG_FSM_SG_REALIGN */
				m_AVD_SET_SG_FSM(avd_cb, sg_of_si, AVD_SG_FSM_SG_REALIGN);
			} else {
				/* No New assignments are been done in the SG
				   reason might be no more inservice SUs to take new assignments or
				   SUs are already assigned to saAmfSGMaxActiveSIsperSU capacity */
				update_ass_state();
				TRACE("No New assignments are been done SI:%s", name.value);
			}
		} else {
			no_of_sisus_to_delete = saAmfSINumCurrActiveAssignments -
						mod_pref_assignments;

			/* Get the sisu pointer from the  si->list_of_sisu list from which 
			no of sisus need to be deleted based on SI ranked SU */
			sisu = tmp_sisu = list_of_sisu;
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
			m_AVD_SET_SG_FSM(avd_cb,  sg_of_si, AVD_SG_FSM_SG_REALIGN);
		}
	} 
	if( sg_of_si->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL ) {
		if( mod_pref_assignments > saAmfSINumCurrStandbyAssignments ) {
			/* SI assignment is not yet complete, choose and assign to appropriate SUs */
			if( avd_sg_nway_si_assign(avd_cb, sg_of_si ) == NCSCC_RC_FAILURE ) {
				LOG_ER("SI new assignmemts failed  SI:%s", name.value);
			} 
		} else {
			no_of_sisus_to_delete = 0;
			no_of_sisus_to_delete = saAmfSINumCurrStandbyAssignments -
						mod_pref_assignments; 

			/* Get the sisu pointer from the  si->list_of_sisu list from which 
			no of sisus need to be deleted based on SI ranked SU */
			sisu = tmp_sisu = list_of_sisu;
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
			m_AVD_SET_SG_FSM(avd_cb, sg_of_si, AVD_SG_FSM_SG_REALIGN);
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

			if (value_is_deleted) {
				mod_pref_assignments = 1;
			} else {
				mod_pref_assignments = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);
			}

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
				si->adjust_si_assignments(mod_pref_assignments);
			} else if (mod_pref_assignments < si->saAmfSINumCurrActiveAssignments) {
				si->adjust_si_assignments(mod_pref_assignments);
				si->saAmfSIPrefActiveAssignments = mod_pref_assignments;
			}
			TRACE("Modified saAmfSIPrefActiveAssignments is '%u'", si->saAmfSIPrefActiveAssignments);
			si->update_ass_state();
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
				si->adjust_si_assignments(mod_pref_assignments);
			} else if (mod_pref_assignments < si->saAmfSINumCurrStandbyAssignments) {
				si->adjust_si_assignments(mod_pref_assignments);
				si->saAmfSIPrefStandbyAssignments = mod_pref_assignments;
			}
			TRACE("Modified saAmfSINumCurrStandbyAssignments is '%u'", si->saAmfSINumCurrStandbyAssignments);
			si->update_ass_state();
		} else if (!strcmp(attribute->attrName, "saAmfSIRank")) {
			if (value_is_deleted == true)
				si->update_sirank(0);
			else {
				/* Ignore the modification with the same value. */
				if (si->saAmfSIRank != *((SaUint32T *)attr_mod->modAttr.attrValues[0]))
					si->update_sirank(*((SaUint32T *)attr_mod->modAttr.attrValues[0]));
			}
			TRACE("Modified saAmfSIRank is '%u'", si->saAmfSIRank);
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
		si->si_add_to_model();
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
 */
void AVD_SI::update_ass_state()
{
	const SaAmfAssignmentStateT oldState = saAmfSIAssignmentState;
	SaAmfAssignmentStateT newState = SA_AMF_ASSIGNMENT_UNASSIGNED;

	/*
	** Note: for SA_AMF_2N_REDUNDANCY_MODEL, SA_AMF_NPM_REDUNDANCY_MODEL &
	** SA_AMF_N_WAY_REDUNDANCY_MODEL it is not possible to check:
	** osafassert(si->saAmfSINumCurrActiveAssignments == 1);
	** since AMF temporarily goes above the limit during fail over
	 */
	switch (sg_of_si->sg_type->saAmfSgtRedundancyModel) {
	case SA_AMF_2N_REDUNDANCY_MODEL:
		/* fall through */
	case SA_AMF_NPM_REDUNDANCY_MODEL:
		if ((saAmfSINumCurrActiveAssignments == 0) &&
				(saAmfSINumCurrStandbyAssignments == 0)) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else if ((saAmfSINumCurrActiveAssignments == 1) &&
				(saAmfSINumCurrStandbyAssignments == 1)) {
			newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		} else
			newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;

		break;
	case SA_AMF_N_WAY_REDUNDANCY_MODEL:
		if ((saAmfSINumCurrActiveAssignments == 0) &&
				(saAmfSINumCurrStandbyAssignments == 0)) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else if ((saAmfSINumCurrActiveAssignments == 1) &&
		           (saAmfSINumCurrStandbyAssignments == saAmfSIPrefStandbyAssignments)) {
		        newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		} else
		        newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
                break;
	case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
		if (saAmfSINumCurrActiveAssignments == 0) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else {
			osafassert(saAmfSINumCurrStandbyAssignments == 0);
			osafassert(saAmfSINumCurrActiveAssignments <= saAmfSIPrefActiveAssignments);
			if (saAmfSINumCurrActiveAssignments == saAmfSIPrefActiveAssignments)
				newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
			else
				newState = SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
		}
		break;
	case SA_AMF_NO_REDUNDANCY_MODEL:
		if (saAmfSINumCurrActiveAssignments == 0) {
			newState = SA_AMF_ASSIGNMENT_UNASSIGNED;
		} else {
			osafassert(saAmfSINumCurrActiveAssignments == 1);
			osafassert(saAmfSINumCurrStandbyAssignments == 0);
			newState = SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
		}
		break;
	default:
		osafassert(0);
	}

	if (newState != saAmfSIAssignmentState) {
		TRACE("'%s' %s => %s", name.value,
				   avd_ass_state[saAmfSIAssignmentState], avd_ass_state[newState]);
		#if 0
		saflog(LOG_NOTICE, amfSvcUsrName, "%s AssignmentState %s => %s", si->name.value,
			   avd_ass_state[si->saAmfSIAssignmentState], avd_ass_state[newState]);
		#endif
		
		saAmfSIAssignmentState = newState;

		/* alarm & notifications */
		if (saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_UNASSIGNED) {
			avd_send_si_unassigned_alarm(&name);
			alarm_sent = true;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_ALARM_SENT);
		}
		else {
			avd_send_si_assigned_ntf(&name, oldState, saAmfSIAssignmentState);
			
			/* Clear of alarm */
			if ((oldState == SA_AMF_ASSIGNMENT_UNASSIGNED) && alarm_sent) {
				avd_alarm_clear(&name, SA_AMF_NTFID_SI_UNASSIGNED, SA_NTF_SOFTWARE_ERROR);
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_ALARM_SENT);
			}

			/* always reset in case the SI has been recycled */
			alarm_sent = false;
		}

		avd_saImmOiRtObjectUpdate(&name, "saAmfSIAssignmentState",
			SA_IMM_ATTR_SAUINT32T, &saAmfSIAssignmentState);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_ASSIGNMENT_STATE);
	}
}

void AVD_SI::inc_curr_act_ass()
{
	saAmfSINumCurrActiveAssignments++;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_ACTIVE);
	TRACE("%s saAmfSINumCurrActiveAssignments=%u", name.value, saAmfSINumCurrActiveAssignments);
	update_ass_state();
}

void AVD_SI::dec_curr_act_ass()
{
	osafassert(saAmfSINumCurrActiveAssignments > 0);
	saAmfSINumCurrActiveAssignments--;
	TRACE("%s saAmfSINumCurrActiveAssignments=%u", name.value, saAmfSINumCurrActiveAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_ACTIVE);
	update_ass_state();
}

void AVD_SI::inc_curr_stdby_ass()
{
	saAmfSINumCurrStandbyAssignments++;
	TRACE("%s saAmfSINumCurrStandbyAssignments=%u", name.value, saAmfSINumCurrStandbyAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_STBY);
	update_ass_state();
}

void AVD_SI::dec_curr_stdby_ass()
{
	osafassert(saAmfSINumCurrStandbyAssignments > 0);
	saAmfSINumCurrStandbyAssignments--;
	TRACE("%s saAmfSINumCurrStandbyAssignments=%u", name.value, saAmfSINumCurrStandbyAssignments);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_STBY);
	update_ass_state();
}

void AVD_SI::inc_curr_act_dec_std_ass()
{
        saAmfSINumCurrActiveAssignments++;
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_ACTIVE);
        TRACE("%s saAmfSINumCurrActiveAssignments=%u", name.value, saAmfSINumCurrActiveAssignments);

        osafassert(saAmfSINumCurrStandbyAssignments > 0);
        saAmfSINumCurrStandbyAssignments--;
        TRACE("%s saAmfSINumCurrStandbyAssignments=%u", name.value, saAmfSINumCurrStandbyAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_STBY);

        update_ass_state();
}

void AVD_SI::inc_curr_stdby_dec_act_ass()
{
        saAmfSINumCurrStandbyAssignments++;
        TRACE("%s saAmfSINumCurrStandbyAssignments=%u", name.value, saAmfSINumCurrStandbyAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_STBY);

        osafassert(saAmfSINumCurrActiveAssignments > 0);
        saAmfSINumCurrActiveAssignments--;
        TRACE("%s saAmfSINumCurrActiveAssignments=%u", name.value, saAmfSINumCurrActiveAssignments);
        m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_SU_CURR_ACTIVE);

        update_ass_state();
}

void avd_si_constructor(void)
{
	si_db = new AmfDb<std::string, AVD_SI>;
	avd_class_impl_set("SaAmfSI", si_rt_attr_cb, si_admin_op_cb,
		si_ccb_completed_cb, si_ccb_apply_cb);
}

void AVD_SI::set_admin_state(SaAmfAdminStateT state)
{
	const SaAmfAdminStateT old_state = saAmfSIAdminState;

	osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("%s AdmState %s => %s", name.value,
		avd_adm_state_name[saAmfSIAdminState], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", name.value,
		avd_adm_state_name[saAmfSIAdminState], avd_adm_state_name[state]);
	saAmfSIAdminState = state;
	avd_saImmOiRtObjectUpdate(&name, "saAmfSIAdminState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSIAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SI_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(&name, SA_AMF_NTFID_SI_ADMIN_STATE, old_state, saAmfSIAdminState);
	TRACE_LEAVE();
}

void AVD_SI::set_si_switch(AVD_CL_CB *cb, const SaToggleState state)
{
	si_switch = state;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, this, AVSV_CKPT_SI_SWITCH);
}

uint32_t AVD_SI::pref_active_assignments() const
{
	return saAmfSIPrefActiveAssignments;
}

uint32_t AVD_SI::curr_active_assignments() const
{
	return saAmfSINumCurrActiveAssignments;
}

uint32_t AVD_SI::pref_standby_assignments() const
{
	return saAmfSIPrefStandbyAssignments;
}

uint32_t AVD_SI::curr_standby_assignments() const
{
	return saAmfSINumCurrStandbyAssignments;
}

/*
 * @brief Check whether @newSiRank is a valid value in term of si dependency
 *        "...The rank of a dependent service instance must not be higher
 *        than the ranks of the service instances on which it depends..."
 *        Lower value means higher rank.
 * @param [in] @newSiRank: rank of si to be checked
 * @return true if @newSiRank is valid value, otherwise false
 */
bool AVD_SI::is_sirank_valid(uint32_t newSiRank) const
{
	AVD_SPONS_SI_NODE *node;

	newSiRank = (newSiRank == 0) ? ~0U : newSiRank;
	/* Check with its sponsors SI */
	for (node = spons_si_list; node; node = node->next) {
		if (newSiRank < node->si->saAmfSIRank) {
			LOG_ER("Invalid saAmfSIRank, ('%s', rank: %u) is higher rank than "
					"sponsor si ('%s', rank: %u)", name.value, newSiRank, 
					node->si->name.value, node->si->saAmfSIRank);
			return false;
		}
	}

	/* Check with its dependent SI */
	std::list<AVD_SI*> depsi_list;
	get_dependent_si_list(name, depsi_list);
	for (std::list<AVD_SI*>::const_iterator it = depsi_list.begin();
			it != depsi_list.end(); it++) {
		if (newSiRank > (*it)->saAmfSIRank) {
			LOG_ER("Invalid saAmfSIRank, ('%s', rank: %u) is lower rank than "
					"dependent si ('%s', rank: %u)", name.value, newSiRank, 
					(*it)->name.value, (*it)->saAmfSIRank);
			return false;
		}
	}
	return true;
}

/*
 * @brief Update saAmfSIRank by new value of @newSiRank, and update the 
 *        the si list which is hold by the sg
 * @param [in] @newSiRank: rank of si to be updated
 */
void AVD_SI::update_sirank(uint32_t newSiRank)
{
	AVD_SG* sg = sg_of_si;
	
	/* Remove and add again to maintain the descending Si rank */
	sg->remove_si(this);
	saAmfSIRank = (newSiRank == 0) ? ~0U : newSiRank;
	sg->add_si(this);
}
/**
  + * @brief       Checks whether SI has atleast one valid active assignment.
  + * @return      true/false
  + **/
bool AVD_SI::is_active() const
{
	for (AVD_SU_SI_REL *sisu = list_of_sisu; sisu != nullptr; sisu = sisu->si_next) {
		if ((sisu->state == SA_AMF_HA_ACTIVE) && (sisu->fsm == AVD_SU_SI_STATE_ASGND) &&
				(sisu->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
			return true;
		}
	}
	return false;
}


