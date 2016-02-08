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

#include <logtrace.h>

#include <util.h>
#include <amf_util.h>
#include <csi.h>
#include <imm.h>
#include <proc.h>

AmfDb<std::string, AVD_CSI> *csi_db = NULL;

//
AVD_COMP* AVD_CSI::find_assigned_comp(const SaNameT *cstype,
                                        const AVD_SU_SI_REL *sisu,
                                        const std::vector<AVD_COMP*> &list_of_comp) {
  auto iter = list_of_comp.begin();
  AVD_COMP* comp = nullptr;
  for (; iter != list_of_comp.end(); ++iter) {
    comp = *iter;
    AVD_COMPCS_TYPE *cst;
    if (NULL != (cst = avd_compcstype_find_match(cstype, comp))) {
      if (SA_AMF_HA_ACTIVE == sisu->state) {
        if (cst->saAmfCompNumCurrActiveCSIs < cst->saAmfCompNumMaxActiveCSIs) {
          break;
        } else { /* We can't assign this csi to this comp, so check for another comp */
          continue ;
        }
      } else {
        if (cst->saAmfCompNumCurrStandbyCSIs < cst->saAmfCompNumMaxStandbyCSIs) {
          break;
        } else { /* We can't assign this csi to this comp, so check for another comp */
          continue ;
        }
      }
    }
  }
  if (iter == list_of_comp.end()) {
    return nullptr;
  } else {
    return comp;
  }
}

void avd_csi_delete(AVD_CSI *csi)
{
	AVD_CSI_ATTR *temp;
	TRACE_ENTER2("%s", csi->name.value);

	/* Delete CSI attributes */
	temp = csi->list_attributes;
	while (temp != NULL) {
		avd_csi_remove_csiattr(csi, temp);
		temp = csi->list_attributes;
	}

	avd_cstype_remove_csi(csi);
	csi->si->remove_csi(csi);

	csi_db->erase(Amf::to_string(&csi->name));
	
	if (csi->saAmfCSIDependencies != NULL) {
		AVD_CSI_DEPS *csi_dep;
		AVD_CSI_DEPS *next_csi_dep;
		
		csi_dep = csi->saAmfCSIDependencies;
		while (csi_dep != NULL) {
			next_csi_dep = csi_dep->csi_dep_next;
			delete csi_dep;
			csi_dep = next_csi_dep;
		}
	}

	delete csi;
	TRACE_LEAVE2();
}

void csi_cmplt_delete(AVD_CSI *csi, bool ckpt)
{
	AVD_PG_CSI_NODE *curr;
	TRACE_ENTER2("%s", csi->name.value);
	if (!ckpt) {
		/* inform the avnds that track this csi */
		for (curr = (AVD_PG_CSI_NODE *)m_NCS_DBLIST_FIND_FIRST(&csi->pg_node_list);
				curr != NULL; curr = (AVD_PG_CSI_NODE *)m_NCS_DBLIST_FIND_NEXT(&curr->csi_dll_node)) {
		   avd_snd_pg_upd_msg(avd_cb, curr->node, 0, static_cast<SaAmfProtectionGroupChangesT>(0), &csi->name);
		}
	}

        /* delete the pg-node list */
        avd_pg_csi_node_del_all(avd_cb, csi);

        /* free memory and remove from DB */
        avd_csi_delete(csi);
	TRACE_LEAVE2();
}

/**
 * Validate configuration attributes for an AMF CSI object
 * @param csi
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	const char *parent;
	unsigned int values_number;

	if ((parent = avd_getparent((const char*)dn->value)) == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(parent, "safSi=", 6) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (cstype_db->find(Amf::to_string(&aname)) == NULL) {
		/* CS type does not exist in current model, check CCB if passed as param */
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

    	/* Verify that all (if any) CSI dependencies are valid */
	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, &values_number) == SA_AIS_OK) &&
		(values_number > 0)) {

		unsigned int i;
		SaNameT saAmfCSIDependency;
		const char *dep_parent;

		for (i = 0; i < values_number; i++) {
			rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, i, &saAmfCSIDependency);
			osafassert(rc == SA_AIS_OK);

			if (strncmp((char*)dn->value, (char*)saAmfCSIDependency.value, sizeof(dn->value)) == 0) {
				report_ccb_validation_error(opdata, "'%s' validation failed - dependency configured to"
						" itself!", dn->value);
				return 0;
			}

			if (csi_db->find(Amf::to_string(&saAmfCSIDependency)) == NULL) {
				/* CSI does not exist in current model, check CCB if passed as param */
				if (opdata == NULL) {
					/* initial loading, check IMM */
					if (!object_exist_in_imm(&saAmfCSIDependency)) {
						report_ccb_validation_error(opdata, "'%s' validation failed - '%s' does not"
								" exist",
								dn->value, saAmfCSIDependency.value);
						return 0;
					}
				} else if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfCSIDependency) == NULL) {
					report_ccb_validation_error(opdata, "'%s' validation failed - '%s' does not exist"
							" in existing model or in CCB",
							dn->value, saAmfCSIDependency.value);
					return 0;
				}
			}

			if ((dep_parent = avd_getparent((const char*)saAmfCSIDependency.value)) == NULL) {
				report_ccb_validation_error(opdata, "'%s' validation failed - invalid "
						"saAmfCSIDependency '%s'", dn->value, saAmfCSIDependency.value);
				return 0;
			}

			if (strncmp(parent, dep_parent, sizeof(dn->value)) != 0) {
				report_ccb_validation_error(opdata, "'%s' validation failed - dependency to CSI in other"
						" SI is not allowed", dn->value);
				return 0;
			}
		}
	}

	/* Verify that the SI can contain this CSI */
	{
		AVD_SI *avd_si;
		SaNameT si_name;

		avsv_sanamet_init(dn, &si_name, "safSi");

		if (NULL != (avd_si = avd_si_get(&si_name))) {
			/* Check for any admin operations undergoing. This is valid during dyn add*/
			if((opdata != NULL) && (AVD_SG_FSM_STABLE != avd_si->sg_of_si->sg_fsm_state)) {
				report_ccb_validation_error(opdata, "SG('%s') fsm state('%u') is not in "
						"AVD_SG_FSM_STABLE(0)", 
						avd_si->sg_of_si->name.value, avd_si->sg_of_si->sg_fsm_state);
				return 0;
			}
		} else {
			if (opdata == NULL) {
				report_ccb_validation_error(opdata, "'%s' does not exist in model", si_name.value);
				return 0;
			}

			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &si_name) == NULL) {
				report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB",
						si_name.value);
				return 0;
			}
		}
#if 0
		svctypecstype = avd_svctypecstypes_get(&svctypecstype_name);
		if (svctypecstype == NULL) {
			LOG_ER("Not found '%s'", svctypecstype_name.value);
			return -1;
		}

		if (svctypecstype->curr_num_csis == svctypecstype->saAmfSvctMaxNumCSIs) {
			LOG_ER("SI '%s' cannot contain more CSIs of this type '%s*",
				csi->si->name.value, csi->saAmfCSType.value);
			return -1;
		}
#endif
	}
	return 1;
}
/**
 * @brief	Check whether the CSI dependency is already existing in the existing list
 * 		if not adds to the dependencies list 
 *
 * @param[in]	csi - csi to which the dependency is added
 * @param[in]	new_csi_dep - csi dependency to be added
 *
 * @return	true/false
 */
static bool csi_add_csidep(AVD_CSI *csi,AVD_CSI_DEPS *new_csi_dep)
{
	AVD_CSI_DEPS *temp_csi_dep;
	bool csi_added = false;

	/* Check whether the CSI dependency is already existing in the existing list.
	 * If yes, it should not get added again
	 */
	for (temp_csi_dep = csi->saAmfCSIDependencies; temp_csi_dep != NULL;
		temp_csi_dep = temp_csi_dep->csi_dep_next) {
		if (0 == memcmp(&new_csi_dep->csi_dep_name_value,
				&temp_csi_dep->csi_dep_name_value, sizeof(SaNameT))) {
			csi_added = true;
		}
	}
	if (!csi_added) {
		/* Add into the CSI dependency list */
		new_csi_dep->csi_dep_next =  csi->saAmfCSIDependencies;
		csi->saAmfCSIDependencies = new_csi_dep; 
	}	 

	return csi_added;
}

/**
 * Removes a CSI dep from the saAmfCSIDependencies list and free the memory
 */
static void csi_remove_csidep(AVD_CSI *csi, const SaNameT *required_dn)
{
	AVD_CSI_DEPS *prev = NULL;
	AVD_CSI_DEPS *curr;

	for (curr = csi->saAmfCSIDependencies; curr != NULL; curr = curr->csi_dep_next) {
		if (memcmp(required_dn, &curr->csi_dep_name_value, sizeof(SaNameT)) == 0) {
			break;
		}
		prev = curr;
	}

	if (curr != NULL) {
		if (prev == NULL) {
			csi->saAmfCSIDependencies = curr->csi_dep_next;
		} else {
			prev->csi_dep_next = curr->csi_dep_next;
		}
	}

	delete curr;
}

//
AVD_CSI::AVD_CSI(const SaNameT* csi_name) {
  memcpy(&name.value, csi_name->value, csi_name->length);
  name.length = csi_name->length;
}
/**
 * @brief	creates new csi and adds csi node to the csi_db 
 *
 * @param[in]	csi_name 
 *
 * @return	pointer to AVD_CSI	
 */
AVD_CSI *csi_create(const SaNameT *csi_name)
{
	AVD_CSI *csi;

	TRACE_ENTER2("'%s'", csi_name->value);

	csi = new AVD_CSI(csi_name);
	
	if (csi_db->insert(Amf::to_string(&csi->name), csi) != NCSCC_RC_SUCCESS)
		osafassert(0);

	TRACE_LEAVE();
	return csi;
}
/**
 * @brief	Reads csi attributes  from imm and csi to the model
 *
 * @param[in]	csi_name 
 *
 * @return	pointer to AVD_CSI	
 */
static void csi_get_attr_and_add_to_model(AVD_CSI *csi, const SaImmAttrValuesT_2 **attributes, const SaNameT *si_name)
{
	int rc = -1;
	unsigned int values_number = 0;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", csi->name.value);

	/* initialize the pg node-list */
	csi->pg_node_list.order = NCS_DBLIST_ANY_ORDER;
	csi->pg_node_list.cmp_cookie = avsv_dblist_uns32_cmp;
	csi->pg_node_list.free_cookie = 0;

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), attributes, 0, &csi->saAmfCSType);
	osafassert(error == SA_AIS_OK);

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, &values_number) == SA_AIS_OK)) {
		if (values_number == 0) {
			/* No Dependency Configured. Mark rank as 1.*/
			csi->rank = 1;
		} else {
			/* Dependency Configured. Decide rank when adding it in si list.*/
			unsigned int i;
			bool found;
			AVD_CSI_DEPS *new_csi_dep = NULL;

			for (i = 0; i < values_number; i++) {
				new_csi_dep = new AVD_CSI_DEPS();
				if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, i,
					&new_csi_dep->csi_dep_name_value) != SA_AIS_OK) {
					LOG_ER("Get saAmfCSIDependencies FAILED for '%s'", csi->name.value);
					// make sure we don't leak any memory if
					// saAmfCSIDependencies can't be read
					delete new_csi_dep;
					goto done;
				}
				found = csi_add_csidep(csi,new_csi_dep);
				if (found == true)
					delete new_csi_dep;
			}
		}
	} else {
		csi->rank = 1;
		TRACE_ENTER2("DEP not configured, marking rank 1. Csi'%s', Rank'%u'",csi->name.value,csi->rank);
	}

	csi->cstype = cstype_db->find(Amf::to_string(&csi->saAmfCSType));
	csi->si = avd_si_get(si_name);

	avd_cstype_add_csi(csi);
	csi->si->add_csi(csi);

	rc = 0;

 done:
	if (rc != 0) {
		delete csi;
		osafassert(0);
	}
	TRACE_LEAVE(); 
}

/**
 * Get configuration for all AMF CSI objects from IMM and create
 * AVD internal objects.
 * 
 * @param si_name
 * @param si
 * 
 * @return int
 */
SaAisErrorT avd_csi_config_get(const SaNameT *si_name, AVD_SI *si)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT csi_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCSI";
	AVD_CSI *csi;

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, si_name, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize_2 failed");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &csi_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&csi_name, attributes, NULL))
			goto done2;

		if ((csi = csi_db->find(Amf::to_string(&csi_name))) == NULL)
		{
			csi = csi_create(&csi_name);

			csi_get_attr_and_add_to_model(csi, attributes, si_name);
		}

		if (avd_csiattr_config_get(&csi_name, csi) != SA_AIS_OK) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:

	return error;
}

/*****************************************************************************
 * Function: csi_ccb_completed_create_hdlr
 * 
 * Purpose: This routine validates create CCB operations on SaAmfCSI objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static SaAisErrorT csi_ccb_completed_create_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	SaNameT si_name;
	AVD_SI *avd_si;
	AVD_COMP *t_comp;
	AVD_SU_SI_REL *t_sisu;
	AVD_COMP_CSI_REL *compcsi;
	SaNameT cstype_name;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	if (!is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata)) 
		goto done;

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), opdata->param.create.attrValues, 0, &cstype_name);
	osafassert(rc == SA_AIS_OK);

	avsv_sanamet_init(&opdata->objectName, &si_name, "safSi");
	avd_si = avd_si_get(&si_name);

	if (NULL != avd_si) {
		/* Check whether si has been assigned to any SU. */
		if (NULL != avd_si->list_of_sisu) {
			t_sisu = avd_si->list_of_sisu;
			while(t_sisu) {
				if (t_sisu->csi_add_rem == true) {
					LOG_NO("CSI create of '%s' in queue: pending assignment"
							" for '%s'", 
							opdata->objectName.value, t_sisu->su->name.value);
				}
				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */

			t_sisu = avd_si->list_of_sisu;
			while(t_sisu) {
				AVD_SU *su = t_sisu->su;
				/* We need to assign this csi if an extra component exists, which is unassigned.*/

				su->reset_all_comps_assign_flag();

				compcsi = t_sisu->list_of_csicomp;
				while (compcsi != NULL) {
					compcsi->comp->set_assigned(true);
					compcsi = compcsi->susi_csicomp_next;
				}

				t_comp = su->find_unassigned_comp_that_provides_cstype(&cstype_name);

				/* Component not found.*/
				if (NULL == t_comp) {
					/* This means that all the components are assigned, let us assigned it to assigned
					   component.*/
					t_comp = AVD_CSI::find_assigned_comp(&cstype_name, t_sisu, su->list_of_comp);
				}
				if (NULL == t_comp) {
					report_ccb_validation_error(opdata, "Compcsi doesn't exist or "
							"MaxActiveCSI/MaxStandbyCSI have reached for csi '%s'",
							opdata->objectName.value);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */
		}/* if (NULL != si->list_of_sisu) */
	}

	rc = SA_AIS_OK;
done:
	TRACE_LEAVE(); 
	return rc;
}

/*****************************************************************************
 * Function: csi_ccb_completed_modify_hdlr
 * 
 * Purpose: This routine validates modify CCB operations on SaAmfCSI objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static SaAisErrorT csi_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_CSI *csi = csi_db->find(Amf::to_string(&opdata->objectName));

	assert(csi);
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSType")) {
			SaNameT cstype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			if(SA_AMF_ADMIN_LOCKED != csi->si->saAmfSIAdminState) {
				report_ccb_validation_error(opdata, "Parent SI is not in locked state, SI state '%d'",
						csi->si->saAmfSIAdminState);
				goto done;
			}
			if (cstype_db->find(Amf::to_string(&cstype_name)) == NULL) {
				report_ccb_validation_error(opdata, "CS Type not found '%s'", cstype_name.value);
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSIDependencies")) {
			//Reject replacement of CSI deps, only deletion and addition are supported.	
			if (attr_mod->modType == SA_IMM_ATTR_VALUES_REPLACE) {
				report_ccb_validation_error(opdata,
					"'%s' - replacement of CSI dependency is not supported",
					opdata->objectName.value);
				goto done;
				
			}
			const SaNameT *required_dn = (SaNameT*) attr_mod->modAttr.attrValues[0];
			const AVD_CSI *required_csi = csi_db->find(Amf::to_string(required_dn));

			// Required CSI must exist in current model
			if (required_csi == NULL) {
				report_ccb_validation_error(opdata,
						"CSI '%s' does not exist", required_dn->value);
				goto done;
			}

			// Required CSI must be contained in the same SI
			const char *si_dn = strchr((char*)required_dn->value, ',') + 1;
			if (strstr((char*)opdata->objectName.value, si_dn) == NULL) {
				report_ccb_validation_error(opdata,
						"'%s' is not in the same SI as '%s'",
						opdata->objectName.value, required_dn->value);
				goto done;
			}

			if (attr_mod->modType == SA_IMM_ATTR_VALUES_ADD) {
				AVD_CSI_DEPS *csi_dep;

				if (attr_mod->modAttr.attrValuesNumber > 1) {
					report_ccb_validation_error(opdata, "only one dep can be added at a time");
					goto done;
				}

				// check cyclic dependencies by scanning the deps of the required CSI
				for (csi_dep = required_csi->saAmfCSIDependencies; csi_dep; csi_dep = csi_dep->csi_dep_next) {
					if (strcmp((char*)csi_dep->csi_dep_name_value.value,
							(char*)opdata->objectName.value) == 0) {
						// the required CSI requires this CSI
						report_ccb_validation_error(opdata,
								"cyclic dependency between '%s' and '%s'",
								opdata->objectName.value, required_dn->value);
						goto done;
					}
				}

				// don't allow adding the same dep again
				for (csi_dep = csi->saAmfCSIDependencies; csi_dep; csi_dep = csi_dep->csi_dep_next) {
					if (strcmp((char*)csi_dep->csi_dep_name_value.value,
							(char*)required_dn->value) == 0) {
						// dep already exist, should we return OK instead?
						report_ccb_validation_error(opdata,
								"dependency between '%s' and '%s' already exist",
								opdata->objectName.value, required_dn->value);
						goto done;
					}
				}

				// disallow dep between same CSIs
				if (strcmp((char*)csi->name.value, (char*)required_dn->value) == 0) {
					report_ccb_validation_error(opdata,
						"dependency for '%s' to itself", csi->name.value);
					goto done;
				}
			} else if (attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) {
				if (attr_mod->modAttr.attrValuesNumber > 1) {
					report_ccb_validation_error(opdata, "only one dep can be removed at a time");
					goto done;
				}
			} 
		} else {
			report_ccb_validation_error(opdata, "Modification of attribute '%s' not supported",
					attr_mod->modAttr.attrName);
			goto done;
		}
	}

	rc = SA_AIS_OK;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: csi_ccb_completed_delete_hdlr
 * 
 * Purpose: This routine validates delete CCB operations on SaAmfCSI objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static SaAisErrorT csi_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_CSI *csi;
	AVD_SU_SI_REL *t_sisu;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	csi = csi_db->find(Amf::to_string(&opdata->objectName));

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		if (csi == NULL) {
			/* This means that csi has been deleted during checkpointing at STDBY and completed callback
			   has arrived delayed.*/
			TRACE("CSI delete completed (STDBY): '%s' does not exist", opdata->objectName.value);
		}
		//IMM honors response of completed callback only from active amfd, so reply ok from standby amfd.
		rc = SA_AIS_OK;
		opdata->userData = csi;	/* Save for later use in apply */
		goto done;
	}

	if(AVD_SG_FSM_STABLE != csi->si->sg_of_si->sg_fsm_state) {
		report_ccb_validation_error(opdata, "SG('%s') fsm state('%u') is not in AVD_SG_FSM_STABLE(0)",
				csi->si->sg_of_si->name.value, csi->si->sg_of_si->sg_fsm_state);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if (csi->si->saAmfSIAdminState != SA_AMF_ADMIN_LOCKED) {
		if (NULL == csi->si->list_of_sisu) {
			/* UnLocked but not assigned. Safe to delete.*/
		} else {/* Assigned to some SU, check whether the last csi. */
			/* SI is unlocked and this is the last csi to be deleted, then donot allow it. */
			if (csi->si->list_of_csi->si_list_of_csi_next == NULL) {
				report_ccb_validation_error(opdata, " csi('%s') is the last csi in si('%s'). Lock SI and"
						" then delete csi.", csi->name.value, csi->si->name.value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			t_sisu = csi->si->list_of_sisu;
			while(t_sisu) {
				if (t_sisu->csi_add_rem == true) {
					LOG_NO("CSI remove of '%s' rejected: pending "
							"assignment for '%s'", 
							csi->name.value, t_sisu->su->name.value);
					if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done; 
					}
				}
				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */
		}
	} else {
		if (csi->list_compcsi != NULL) {
			report_ccb_validation_error(opdata, "SaAmfCSI '%s' is in use", csi->name.value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	rc = SA_AIS_OK;
	opdata->userData = csi;	/* Save for later use in apply */
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT csi_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = csi_ccb_completed_create_hdlr(opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = csi_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = csi_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
	return rc;
}

static void ccb_apply_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SU_SI_REL *t_sisu;
	AVD_COMP_CSI_REL *t_csicomp;
	AVD_CSI *csi = static_cast<AVD_CSI*>(opdata->userData);
	AVD_CSI *csi_in_db;

	bool first_sisu = true;

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) { 
		/* A double check whether csi has been deleted from DB or not and whether pointer stored userData 
		   is still valid. */
		csi_in_db =  csi_db->find(Amf::to_string(&opdata->objectName));
		if ((csi == NULL) || (csi_in_db == NULL)) {
			/* This means that csi has been deleted during checkpointing at STDBY and delete callback
			   has arrived delayed.*/
			LOG_WA("CSI delete apply (STDBY): csi does not exist");
			goto done;
		}
		if (csi->list_compcsi == NULL ) {
			/* delete the pg-node list */
			avd_pg_csi_node_del_all(avd_cb, csi);

			/* free memory and remove from DB */
			avd_csi_delete(csi);
		}
		goto done;
	}

        TRACE_ENTER2("'%s'", csi ? csi->name.value : NULL);

	/* Check whether si has been assigned to any SU. */
	if ((NULL != csi->si->list_of_sisu) && 
			(csi->compcsi_cnt != 0)) {
		TRACE("compcsi_cnt'%u'", csi->compcsi_cnt);
		/* csi->compcsi_cnt == 0 ==> This means that there is no comp_csi related to this csi in the SI. It may
		   happen this csi is not assigned to any CSI because of no compcstype match, but its si may
		   have SUSI. This will happen in case of deleting one comp from SU in upgrade case 
                   Scenario : Add one comp1-csi1, comp2-csi2 in upgrade procedure, then delete 
		   comp1-csi1 i.e. in the end call immcfg -d csi1. Since csi1 will not be assigned
		   to anybody because of unique comp-cstype configured and since comp1 is deleted
		   so, there wouldn't be any assignment.
		   So, Just delete csi.*/
		t_sisu = csi->si->list_of_sisu;
		while(t_sisu) {
			/* Find the relevant comp-csi to send susi delete. */
			for (t_csicomp = t_sisu->list_of_csicomp; t_csicomp; t_csicomp = t_csicomp->susi_csicomp_next)
				if (t_csicomp->csi == csi)
					break;
			osafassert(t_csicomp);
			/* Mark comp-csi and sisu to be under csi add/rem.*/
			/* Send csi assignment for act susi first to the corresponding amfnd. */
			if ((SA_AMF_HA_ACTIVE == t_sisu->state) && (true == first_sisu)) {
				first_sisu = false;
				if (avd_snd_susi_msg(avd_cb, t_sisu->su, t_sisu, AVSV_SUSI_ACT_DEL, true, t_csicomp) != NCSCC_RC_SUCCESS) {
					LOG_ER("susi send failure for su'%s' and si'%s'", t_sisu->su->name.value, t_sisu->si->name.value);
					goto done;
				}

			}
			t_sisu->csi_add_rem = static_cast<SaBoolT>(true);
			t_sisu->comp_name = t_csicomp->comp->comp_info.name;
			t_sisu->csi_name = t_csicomp->csi->name;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_sisu, AVSV_CKPT_AVD_SI_ASS);
			t_sisu = t_sisu->si_next;
		}/* while(t_sisu) */

	} else { /* if (NULL != csi->si->list_of_sisu) */
		csi_cmplt_delete(csi, false);
	}

	/* Send pg update and delete csi after all csi gets removed. */
done:
	TRACE_LEAVE();

}

/*****************************************************************************
 * Function: csi_ccb_apply_modify_hdlr
 * 
 * Purpose: This routine handles modify operations on SaAmfCSI objects.
 * 
 *
 * Input  : Ccb Util Oper Data. 
 *              
 * Returns: None.
 *  
 * NOTES  : None.
 *              
 *      
 **************************************************************************/
static void csi_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{               
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_CSI *csi = NULL;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);
 
	csi = csi_db->find(Amf::to_string(&opdata->objectName));
	assert(csi != NULL);
	AVD_SI *si = csi->si;
	assert(si != NULL);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSType")) {
			AVD_CS_TYPE *csi_type;
			SaNameT cstype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			TRACE("saAmfCSType modified from '%s' to '%s' for Csi'%s'", csi->saAmfCSType.value,
					cstype_name.value, csi->name.value);
			csi_type = cstype_db->find(Amf::to_string(&cstype_name));
			avd_cstype_remove_csi(csi);
			csi->saAmfCSType = cstype_name;
			csi->cstype = csi_type;
			avd_cstype_add_csi(csi);
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSIDependencies")) {
			if (attr_mod->modType == SA_IMM_ATTR_VALUES_ADD) {
				assert(attr_mod->modAttr.attrValuesNumber == 1);
				si->remove_csi(csi);
				AVD_CSI_DEPS *new_csi_dep = new AVD_CSI_DEPS();
				new_csi_dep->csi_dep_name_value = *((SaNameT*) attr_mod->modAttr.attrValues[0]);
				bool already_exist = csi_add_csidep(csi, new_csi_dep);
				if (already_exist)
					delete new_csi_dep;
				csi->rank = 0; // indicate that there is a dep to another CSI
				si->add_csi(csi);
			} else if (attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) {
				assert(attr_mod->modAttr.attrValuesNumber == 1);
				const SaNameT *required_dn = (SaNameT*) attr_mod->modAttr.attrValues[0];
				csi_remove_csidep(csi, required_dn);
				
				//Mark rank of all the CSIs to 0.
                                for (AVD_CSI *tmp_csi = csi->si->list_of_csi; tmp_csi;
                                                tmp_csi = tmp_csi->si_list_of_csi_next) {
					tmp_csi->rank = 0;// indicate that there is a dep to another CSI
				}
				//Rearrange Rank of all the CSIs now.
                                for (AVD_CSI *tmp_csi = csi->si->list_of_csi; tmp_csi;
                                                tmp_csi = tmp_csi->si_list_of_csi_next) {
					tmp_csi->si->arrange_dep_csi(tmp_csi);
				}
			} else
				assert(0);
		} else {
			osafassert(0);
		}
	}
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: csi_ccb_apply_create_hdlr
 * 
 * Purpose: This routine handles create operations on SaAmfCSI objects.
 * 
 *
 * Input  : Ccb Util Oper Data. 
 *              
 * Returns: None.
 *  
 * NOTES  : None.
 *              
 *      
 **************************************************************************/
static void csi_ccb_apply_create_hdlr(struct CcbUtilOperationData *opdata)
{
	AVD_CSI *csi = NULL;
	if ((csi = csi_db->find(Amf::to_string(&opdata->objectName))) == NULL) {
		/* this check is added because, some times there is
		   possibility that before getting ccb apply callback
		   we might get compcsi create checkpoint and csi will
		   be created as part of checkpoint processing */
		csi = csi_create(&opdata->objectName);
	} 
	csi_get_attr_and_add_to_model(csi, opdata->param.create.attrValues,
			opdata->param.create.parentName);

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		goto done;

	csi_assign_hdlr(csi);

done:
	TRACE_LEAVE();
}

/**
 * @brief       Assign csi to component as per compcsi configurations.
 *
 * @param[in]   csi pointer.
 *
 * @return      OK if csi is assigned else NO_OP.
 */
SaAisErrorT csi_assign_hdlr(AVD_CSI *csi)
{
	AVD_COMP *t_comp;
	AVD_SU_SI_REL *t_sisu;
	bool first_sisu = true;
	AVD_COMP_CSI_REL *compcsi;
	SaAisErrorT rc = SA_AIS_ERR_NO_OP;

	/* Check whether csi assignment is already in progress and if yes, then return.
	   This csi will be assigned after the undergoing csi assignment gets over.*/
	if (csi->si->list_of_sisu != NULL) {
		for(t_sisu = csi->si->list_of_sisu; t_sisu != NULL; t_sisu = t_sisu->si_next) {
			if (t_sisu->csi_add_rem == true) {
				LOG_NO("CSI create '%s' delayed: pending assignment for '%s'",
						csi->name.value, t_sisu->su->name.value);
				goto done;
			}
		}
	}

	/* Check whether si has been assigned to any SU. */
	if (NULL != csi->si->list_of_sisu) {
		t_sisu = csi->si->list_of_sisu;
		while(t_sisu) {
			/* We need to assign this csi if an extra component exists, which is unassigned.*/

			t_sisu->su->reset_all_comps_assign_flag();

			compcsi = t_sisu->list_of_csicomp;
			while (compcsi != NULL) {
				compcsi->comp->set_assigned(true);
				compcsi = compcsi->susi_csicomp_next;
			}

			t_comp = t_sisu->su->find_unassigned_comp_that_provides_cstype(&csi->saAmfCSType);

			/* Component not found.*/
			if (NULL == t_comp) {
				/* This means that all the components are assigned, let us assigned it to assigned 
				   component.*/
				t_comp = AVD_CSI::find_assigned_comp(&csi->saAmfCSType, t_sisu, t_sisu->su->list_of_comp);
			}
			if (NULL == t_comp) {
				LOG_ER("Compcsi doesn't exist or MaxActiveCSI/MaxStandbyCSI have reached for csi '%s'",
						csi->name.value);
				goto done;
			}

			if ((compcsi = avd_compcsi_create(t_sisu, csi, t_comp, true)) == NULL) {
				/* free all the CSI assignments and end this loop */
				avd_compcsi_delete(avd_cb, t_sisu, true);
				break;
			}
			/* Mark comp-csi and sisu to be under csi add/rem.*/
			/* Send csi assignment for act susi first to the corresponding amfnd. */
			if ((SA_AMF_HA_ACTIVE == t_sisu->state) && (true == first_sisu)) {
				first_sisu = false;
				if (avd_snd_susi_msg(avd_cb, t_sisu->su, t_sisu, AVSV_SUSI_ACT_ASGN, true, compcsi) != NCSCC_RC_SUCCESS) {
					/* free all the CSI assignments and end this loop */
					avd_compcsi_delete(avd_cb, t_sisu, true); 
					/* Unassign the SUSI */
					avd_susi_update_assignment_counters(t_sisu, AVSV_SUSI_ACT_DEL, static_cast<SaAmfHAStateT>(0), static_cast<SaAmfHAStateT>(0));
					avd_susi_delete(avd_cb, t_sisu, true);
					goto done;
				}
				rc = SA_AIS_OK;

			}
			t_sisu->csi_add_rem = static_cast<SaBoolT>(true);
			t_sisu->comp_name = compcsi->comp->comp_info.name;
			t_sisu->csi_name = compcsi->csi->name;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_sisu, AVSV_CKPT_AVD_SI_ASS);
			t_sisu = t_sisu->si_next;
		}/* while(t_sisu) */

	}/* if (NULL != csi->si->list_of_sisu) */
	else if (csi->si->saAmfSIAdminState == SA_AMF_ADMIN_UNLOCKED) {
		/* CSI has been added into an SI, now SI can be assigned */
		csi->si->sg_of_si->si_assign(avd_cb, csi->si);
	}
done:
	return rc;
	TRACE_LEAVE();
}

static void csi_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
                csi_ccb_apply_create_hdlr(opdata);
		break;
        case CCBUTIL_MODIFY:
                csi_ccb_apply_modify_hdlr(opdata);
                break;
	case CCBUTIL_DELETE:
		ccb_apply_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Create an SaAmfCSIAssignment runtime object in IMM.
 * @param ha_state
 * @param csi_dn
 * @param comp_dn
 */
static void avd_create_csiassignment_in_imm(SaAmfHAStateT ha_state,
       const SaNameT *csi_dn, const SaNameT *comp_dn)
{
	SaNameT dn;
	SaAmfHAReadinessStateT saAmfCSICompHAReadinessState = SA_AMF_HARS_READY_FOR_ASSIGNMENT;
	void *arr1[] = { &dn };
	void *arr2[] = { &ha_state };
	void *arr3[] = { &saAmfCSICompHAReadinessState };
	const SaImmAttrValuesT_2 attr_safCSIComp = {
			const_cast<SaImmAttrNameT>("safCSIComp"),
			SA_IMM_ATTR_SANAMET, 1, arr1
	};
	const SaImmAttrValuesT_2 attr_saAmfCSICompHAState = {
			const_cast<SaImmAttrNameT>("saAmfCSICompHAState"),
			SA_IMM_ATTR_SAUINT32T, 1, arr2
	};
	const SaImmAttrValuesT_2 attr_saAmfCSICompHAReadinessState = {
			const_cast<SaImmAttrNameT>("saAmfCSICompHAReadinessState"),
			SA_IMM_ATTR_SAUINT32T, 1, arr3
	};
	const SaImmAttrValuesT_2 *attrValues[] = {
			&attr_safCSIComp,
			&attr_saAmfCSICompHAState,
			&attr_saAmfCSICompHAReadinessState,
			NULL
	};

	avsv_create_association_class_dn(comp_dn, NULL, "safCSIComp", &dn);

	TRACE("Adding %s", dn.value);
	avd_saImmOiRtObjectCreate("SaAmfCSIAssignment",	csi_dn, attrValues);
}

AVD_COMP_CSI_REL *avd_compcsi_create(AVD_SU_SI_REL *susi, AVD_CSI *csi,
	AVD_COMP *comp, bool create_in_imm)
{
	AVD_COMP_CSI_REL *compcsi = NULL;

	if ((csi == NULL) && (comp == NULL)) {
		LOG_ER("Either csi or comp is NULL");
                return NULL;
	}

	TRACE_ENTER2("Comp'%s' and Csi'%s'", comp->comp_info.name.value, csi->name.value);

	/* do not add if already in there */
	for (compcsi = susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
		if ((compcsi->comp == comp) && (compcsi->csi == csi))
			goto done;
	}

	compcsi = new AVD_COMP_CSI_REL();

	compcsi->comp = comp;
	compcsi->csi = csi;
	compcsi->susi = susi;

	/* Add to the CSI owned list */
	if (csi->list_compcsi == NULL) {
		csi->list_compcsi = compcsi;
	} else {
		compcsi->csi_csicomp_next = csi->list_compcsi;
		csi->list_compcsi = compcsi;
	}
	csi->compcsi_cnt++;

	/* Add to the SUSI owned list */
	if (susi->list_of_csicomp == NULL) {
		susi->list_of_csicomp = compcsi;
	} else {
		compcsi->susi_csicomp_next = susi->list_of_csicomp;
		susi->list_of_csicomp = compcsi;
	}
	if (create_in_imm)
		avd_create_csiassignment_in_imm(susi->state, &csi->name, &comp->comp_info.name);
done:
	TRACE_LEAVE();
	return compcsi;
}

/** Delete an SaAmfCSIAssignment from IMM
 * 
 * @param comp_dn
 * @param csi_dn
 */
static void avd_delete_csiassignment_from_imm(const SaNameT *comp_dn, const SaNameT *csi_dn)
{
       SaNameT dn; 

       avsv_create_association_class_dn(comp_dn, csi_dn, "safCSIComp", &dn);
       TRACE("Deleting %s", dn.value);

       avd_saImmOiRtObjectDelete(&dn);
}

/*****************************************************************************
 * Function: avd_compcsi_delete
 *
 * Purpose:  This function will delete and free all the AVD_COMP_CSI_REL
 * structure from the list_of_csicomp in the SUSI relationship
 * 
 * Input: cb - the AVD control block
 *        susi - The SU SI relationship structure that encompasses this
 *               component CSI relationship.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE .
 *
 * NOTES:
 *
 * 
 **************************************************************************/

uint32_t avd_compcsi_delete(AVD_CL_CB *cb, AVD_SU_SI_REL *susi, bool ckpt)
{
	AVD_COMP_CSI_REL *lcomp_csi;
	AVD_COMP_CSI_REL *i_compcsi, *prev_compcsi = NULL;

	TRACE_ENTER();
	while (susi->list_of_csicomp != NULL) {
		lcomp_csi = susi->list_of_csicomp;

		i_compcsi = lcomp_csi->csi->list_compcsi;
		while ((i_compcsi != NULL) && (i_compcsi != lcomp_csi)) {
			prev_compcsi = i_compcsi;
			i_compcsi = i_compcsi->csi_csicomp_next;
		}
		if (i_compcsi != lcomp_csi) {
			/* not found */
		} else {
			if (prev_compcsi == NULL) {
				lcomp_csi->csi->list_compcsi = i_compcsi->csi_csicomp_next;
			} else {
				prev_compcsi->csi_csicomp_next = i_compcsi->csi_csicomp_next;
			}
			lcomp_csi->csi->compcsi_cnt--;

			/* trigger pg upd */
			if (!ckpt) {
				avd_pg_compcsi_chg_prc(cb, lcomp_csi, true);
			}

			i_compcsi->csi_csicomp_next = NULL;
		}

		susi->list_of_csicomp = lcomp_csi->susi_csicomp_next;
		lcomp_csi->susi_csicomp_next = NULL;
		prev_compcsi = NULL;
		avd_delete_csiassignment_from_imm(&lcomp_csi->comp->comp_info.name, &lcomp_csi->csi->name);
		delete lcomp_csi;

	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_compcsi_from_csi_and_susi_delete
 *
 * Purpose:  This function will delete and free AVD_COMP_CSI_REL
 * structure from the list_of_csicomp and SUSI.
 * 
 * Input: susi - SUSI from where comp-csi need to be deleted.
 *        compcsi - To be deleted.
 *        ckpt - whether this function has been called form checkpoint context.
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/
void avd_compcsi_from_csi_and_susi_delete(AVD_SU_SI_REL *susi, AVD_COMP_CSI_REL *comp_csi, bool ckpt)
{
	AVD_COMP_CSI_REL *t_compcsi, *t_compcsi_susi, *prev_compcsi = NULL;

	TRACE_ENTER2("Csi'%s', compcsi_cnt'%u'", comp_csi->csi->name.value, comp_csi->csi->compcsi_cnt);

	/* Find the comp-csi in susi. */
	t_compcsi_susi = susi->list_of_csicomp;
	while (t_compcsi_susi) {
		if (t_compcsi_susi == comp_csi)
			break;
		prev_compcsi = t_compcsi_susi;
		t_compcsi_susi = t_compcsi_susi->susi_csicomp_next;
	}
	osafassert(t_compcsi_susi);
	/* Delink the csi from this susi. */
	if (NULL == prev_compcsi)
		susi->list_of_csicomp = t_compcsi_susi->susi_csicomp_next;
	else {
		prev_compcsi->susi_csicomp_next = t_compcsi_susi->susi_csicomp_next;
		t_compcsi_susi->susi_csicomp_next = NULL;
	}

	prev_compcsi =  NULL;
	/* Find the comp-csi in csi->list_compcsi. */
	t_compcsi = comp_csi->csi->list_compcsi;
	while (t_compcsi) {
		if (t_compcsi == comp_csi)
			break;
		prev_compcsi = t_compcsi;
		t_compcsi = t_compcsi->csi_csicomp_next;
	}
	osafassert(t_compcsi);
	/* Delink the csi from csi->list_compcsi. */
	if (NULL == prev_compcsi)
		comp_csi->csi->list_compcsi = t_compcsi->csi_csicomp_next;
	else {
		prev_compcsi->csi_csicomp_next = t_compcsi->csi_csicomp_next;
		t_compcsi->csi_csicomp_next = NULL;
	}

	osafassert(t_compcsi == t_compcsi_susi);
	comp_csi->csi->compcsi_cnt--;

	if (!ckpt)
		avd_snd_pg_upd_msg(avd_cb, comp_csi->comp->su->su_on_node, comp_csi, SA_AMF_PROTECTION_GROUP_REMOVED, 0);
	avd_delete_csiassignment_from_imm(&comp_csi->comp->comp_info.name, &comp_csi->csi->name);
	delete comp_csi;

	TRACE_LEAVE();
}

void avd_csi_remove_csiattr(AVD_CSI *csi, AVD_CSI_ATTR *attr)
{
	AVD_CSI_ATTR *i_attr = NULL;
	AVD_CSI_ATTR *p_attr = NULL;

	TRACE_ENTER();
	/* remove ATTR from CSI list */
	i_attr = csi->list_attributes;

	while ((i_attr != NULL) && (i_attr != attr)) {
		p_attr = i_attr;
		i_attr = i_attr->attr_next;
	}

	if (i_attr != attr) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (p_attr == NULL) {
			csi->list_attributes = i_attr->attr_next;
		} else {
			p_attr->attr_next = i_attr->attr_next;
			delete [] attr->name_value.string_ptr;
			delete attr;
		}
	}

	osafassert(csi->num_attributes > 0);
	csi->num_attributes--;
	TRACE_LEAVE();
}

void avd_csi_add_csiattr(AVD_CSI *csi, AVD_CSI_ATTR *csiattr)
{
	int cnt = 0;
	AVD_CSI_ATTR *ptr;

	TRACE_ENTER();
	/* Count number of attributes (multivalue) */
	ptr = csiattr;
	while (ptr != NULL) {
		cnt++;
		if (ptr->attr_next != NULL)
			ptr = ptr->attr_next;
		else
			break;
	}

	ptr->attr_next = csi->list_attributes;
	csi->list_attributes = csiattr;
	csi->num_attributes += cnt;
	TRACE_LEAVE();
}

void avd_csi_constructor(void)
{
	csi_db = new AmfDb<std::string, AVD_CSI>;
	avd_class_impl_set("SaAmfCSI", NULL, NULL, csi_ccb_completed_cb,
		csi_ccb_apply_cb);
}

/**
 * @brief	Check whether the Single csi assignment is undergoing on the SG.
 *
 * @param[in]	sg - Pointer to the Service Group.
 *
 * @return	true if operation is undergoing else false.
 */
bool csi_assignment_validate(AVD_SG *sg)
{
	AVD_SU_SI_REL *temp_sisu;

	for (const auto& temp_si : sg->list_of_si)
		for (temp_sisu = temp_si->list_of_sisu; temp_sisu; temp_sisu = temp_sisu->si_next)
			if (temp_sisu->csi_add_rem == true)
				return true;
	return false;
}


/**
 * @brief       Checks if sponsor CSIs of any CSI are assigned to any comp in this SU.
 *
 * @param[in]   ptr to CSI.
 * @param[in]   ptr to SU.
 *
 * @return      true/false.
 */
bool are_sponsor_csis_assigned_in_su(AVD_CSI *csi, AVD_SU *su)
{
        for (AVD_CSI_DEPS *spons_csi = csi->saAmfCSIDependencies; spons_csi != NULL;
                spons_csi = spons_csi->csi_dep_next) {
		bool is_sponsor_assigned = false;
		
                AVD_CSI *tmp_csi =  csi_db->find(Amf::to_string(&spons_csi->csi_dep_name_value));

		//Check if this sponsor csi is assigned to any comp in this su.
		for (AVD_COMP_CSI_REL *compcsi = tmp_csi->list_compcsi; compcsi;
				compcsi = compcsi->csi_csicomp_next) {	
			if (compcsi->comp->su == su)
				is_sponsor_assigned = true;
		}
		//Return false if this sponsor is not assigned to this SU.
		if (is_sponsor_assigned == false)
			return false;
        }
        return true;
}

