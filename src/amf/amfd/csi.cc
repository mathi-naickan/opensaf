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

#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amf_util.h"
#include "amf/amfd/csi.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/proc.h"

AmfDb<std::string, AVD_CSI> *csi_db = nullptr;

//
AVD_COMP* AVD_CSI::find_assigned_comp(const std::string& cstype,
                                        const AVD_SU_SI_REL *sisu,
                                        const std::vector<AVD_COMP*> &list_of_comp) {
  auto iter = list_of_comp.begin();
  AVD_COMP* comp = nullptr;
  for (; iter != list_of_comp.end(); ++iter) {
    comp = *iter;
    AVD_COMPCS_TYPE *cst;
    if (nullptr != (cst = avd_compcstype_find_match(cstype, comp))) {
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
	TRACE_ENTER2("%s", csi->name.c_str());

	/* Delete CSI attributes */
	temp = csi->list_attributes;
	while (temp != nullptr) {
		avd_csi_remove_csiattr(csi, temp);
		temp = csi->list_attributes;
	}

	avd_cstype_remove_csi(csi);
	csi->si->remove_csi(csi);

	csi_db->erase(csi->name);

	if (csi->saAmfCSIDependencies != nullptr) {
		AVD_CSI_DEPS *csi_dep;
		AVD_CSI_DEPS *next_csi_dep;
		
		csi_dep = csi->saAmfCSIDependencies;
		while (csi_dep != nullptr) {
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
	TRACE_ENTER2("%s", csi->name.c_str());
	if (!ckpt) {
		/* inform the avnds that track this csi */
		for (curr = (AVD_PG_CSI_NODE *)m_NCS_DBLIST_FIND_FIRST(&csi->pg_node_list);
				curr != nullptr; curr = (AVD_PG_CSI_NODE *)m_NCS_DBLIST_FIND_NEXT(&curr->csi_dll_node)) {
		   avd_snd_pg_upd_msg(avd_cb, curr->node, 0, static_cast<SaAmfProtectionGroupChangesT>(0), csi->name);
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
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	std::string parent;
	unsigned int values_number;

	TRACE_ENTER2("%s", dn.c_str());

	parent = avd_getparent(dn);
	if (parent.empty() == true ) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		TRACE_LEAVE();
		return 0;
	}

	if (parent.compare(0, 6, "safSi=") != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent.c_str(), dn.c_str());
		TRACE_LEAVE();
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (cstype_db->find(Amf::to_string(&aname)) == nullptr) {
		/* CS type does not exist in current model, check CCB if passed as param */
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(&aname));
			TRACE_LEAVE();
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", osaf_extended_name_borrow(&aname));
			TRACE_LEAVE();
			return 0;
		}
	}

    	/* Verify that all (if any) CSI dependencies are valid */
	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, &values_number) == SA_AIS_OK) &&
		(values_number > 0)) {

		unsigned int i;
		std::string csi_dependency;
		std::string dep_parent;

		for (i = 0; i < values_number; i++) {
			SaNameT tmp_dependency;
			rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, i, &tmp_dependency);
			osafassert(rc == SA_AIS_OK);
			csi_dependency = Amf::to_string(&tmp_dependency);

			if (dn.compare(csi_dependency) == 0) {
				report_ccb_validation_error(opdata, "'%s' validation failed - dependency configured to"
						" itself!", dn.c_str());
				TRACE_LEAVE();
				return 0;
			}

			if (csi_db->find(csi_dependency) == nullptr) {
				/* CSI does not exist in current model, check CCB if passed as param */
				if (opdata == nullptr) {
					/* initial loading, check IMM */
					if (object_exist_in_imm(csi_dependency) == false) {
						report_ccb_validation_error(opdata, "'%s' validation failed - '%s' does not"
								" exist",
								dn.c_str(), csi_dependency.c_str());
						TRACE_LEAVE();
						return 0;
					}
				} else if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &tmp_dependency) == nullptr) {
					report_ccb_validation_error(opdata, "'%s' validation failed - '%s' does not exist"
							" in existing model or in CCB",
							dn.c_str(), csi_dependency.c_str());
					TRACE_LEAVE();
					return 0;
				}
			}

			dep_parent = avd_getparent(csi_dependency);
			if (dep_parent.empty() == true) {
				report_ccb_validation_error(opdata, "'%s' validation failed - invalid "
						"saAmfCSIDependency '%s'", dn.c_str(), csi_dependency.c_str());
				TRACE_LEAVE();
				return 0;
			}

			if (parent.compare(dep_parent) != 0) {
				report_ccb_validation_error(opdata, "'%s' validation failed - dependency to CSI in other"
						" SI is not allowed", dn.c_str());
				TRACE_LEAVE();
				return 0;
			}
		}
	}

	/* Verify that the SI can contain this CSI */
	{
		AVD_SI *avd_si;
		std::string si_name;

		avsv_sanamet_init(dn, si_name, "safSi");

		if (nullptr != (avd_si = avd_si_get(si_name))) {
			/* Check for any admin operations undergoing. This is valid during dyn add*/
			if((opdata != nullptr) && (AVD_SG_FSM_STABLE != avd_si->sg_of_si->sg_fsm_state)) {
				report_ccb_validation_error(opdata, "SG('%s') fsm state('%u') is not in "
						"AVD_SG_FSM_STABLE(0)", 
						avd_si->sg_of_si->name.c_str(), avd_si->sg_of_si->sg_fsm_state);
				TRACE_LEAVE();
				return 0;
			}
		} else {
			if (opdata == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist in model", si_name.c_str());
				TRACE_LEAVE();
				return 0;
			}

			const SaNameTWrapper si(si_name);
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, si) == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB",
						si_name.c_str());
				TRACE_LEAVE();
				return 0;
			}
		}
#if 0
		svctypecstype = avd_svctypecstypes_get(&svctypecstype_name);
		if (svctypecstype == nullptr) {
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
	TRACE_LEAVE();
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
	for (temp_csi_dep = csi->saAmfCSIDependencies; temp_csi_dep != nullptr;
		temp_csi_dep = temp_csi_dep->csi_dep_next) {
		if (new_csi_dep->csi_dep_name_value.compare(temp_csi_dep->csi_dep_name_value) == 0) {
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
static void csi_remove_csidep(AVD_CSI *csi, const std::string& required_dn)
{
	AVD_CSI_DEPS *prev = nullptr;
	AVD_CSI_DEPS *curr;

	for (curr = csi->saAmfCSIDependencies; curr != nullptr; curr = curr->csi_dep_next) {
		if (required_dn.compare(curr->csi_dep_name_value) == 0) {
			break;
		}
		prev = curr;
	}

	if (curr != nullptr) {
		if (prev == nullptr) {
			csi->saAmfCSIDependencies = curr->csi_dep_next;
		} else {
			prev->csi_dep_next = curr->csi_dep_next;
		}
	}

	delete curr;
}

//
AVD_CSI::AVD_CSI(const std::string& csi_name) :
	name(csi_name) {
}
/**
 * @brief	creates new csi and adds csi node to the csi_db 
 *
 * @param[in]	csi_name 
 *
 * @return	pointer to AVD_CSI	
 */
AVD_CSI *csi_create(const std::string& csi_name)
{
	AVD_CSI *csi;

	TRACE_ENTER2("'%s'", csi_name.c_str());

	csi = new AVD_CSI(csi_name);
	
	if (csi_db->insert(csi->name, csi) != NCSCC_RC_SUCCESS)
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
static void csi_get_attr_and_add_to_model(AVD_CSI *csi, const SaImmAttrValuesT_2 **attributes, const std::string& si_name)
{
	int rc = -1;
	unsigned int values_number = 0;
	SaAisErrorT error;
	SaNameT cs_type;

	TRACE_ENTER2("'%s'", csi->name.c_str());

	/* initialize the pg node-list */
	csi->pg_node_list.order = NCS_DBLIST_ANY_ORDER;
	csi->pg_node_list.cmp_cookie = avsv_dblist_uns32_cmp;
	csi->pg_node_list.free_cookie = 0;

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), attributes, 0, &cs_type);
	osafassert(error == SA_AIS_OK);
	csi->saAmfCSType = Amf::to_string(&cs_type);


	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, &values_number) == SA_AIS_OK)) {
		if (values_number == 0) {
			/* No Dependency Configured. Mark rank as 1.*/
			csi->rank = 1;
		} else {
			/* Dependency Configured. Decide rank when adding it in si list.*/
			unsigned int i;
			bool found;
			AVD_CSI_DEPS *new_csi_dep = nullptr;
			
			TRACE("checking dependencies");

			for (i = 0; i < values_number; i++) {
				TRACE("i %u", i);
				new_csi_dep = new AVD_CSI_DEPS();
				SaNameT temp_name;
				if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSIDependencies"), attributes, i,
					&temp_name) != SA_AIS_OK) {
					LOG_ER("Get saAmfCSIDependencies FAILED for '%s'", csi->name.c_str());
					// make sure we don't leak any memory if
					// saAmfCSIDependencies can't be read
					delete new_csi_dep;
					goto done;
				}
				new_csi_dep->csi_dep_name_value = Amf::to_string(&temp_name);
				found = csi_add_csidep(csi,new_csi_dep);
				if (found == true)
					delete new_csi_dep;
			}
		}
	} else {
		csi->rank = 1;
		TRACE_ENTER2("DEP not configured, marking rank 1. Csi'%s', Rank'%u'",csi->name.c_str(),csi->rank);
	}

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("osafAmfCSICommunicateCsiAttributeChange"),
					attributes, &values_number) != SA_AIS_OK)) {
		TRACE("Default for osafAmfCSICommunicateCsiAttributeChange set to false");
		csi->osafAmfCSICommunicateCsiAttributeChange = false; //Default value is always 0.
	}

	TRACE("find %s", csi->saAmfCSType.c_str());
	csi->cstype = cstype_db->find(csi->saAmfCSType);
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
SaAisErrorT avd_csi_config_get(const std::string& si_name, AVD_SI *si)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT temp_csi_name;
	
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCSI";
	AVD_CSI *csi;

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, si_name.c_str(), SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		nullptr, &searchHandle) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize_2 failed");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &temp_csi_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		const std::string csi_name(Amf::to_string(&temp_csi_name));
		if (!is_config_valid(csi_name, attributes, nullptr))
			goto done2;

		if ((csi = csi_db->find(csi_name)) == nullptr)
		{
			csi = csi_create(csi_name);

			csi_get_attr_and_add_to_model(csi, attributes, si_name);
		}

		if (avd_csiattr_config_get(csi_name, csi) != SA_AIS_OK) {
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
	std::string si_name;
	AVD_SI *avd_si;
	AVD_COMP *t_comp;
	AVD_SU_SI_REL *t_sisu;
	AVD_COMP_CSI_REL *compcsi;
	SaNameT cstype_name;
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	if (!is_config_valid(object_name, opdata->param.create.attrValues, opdata)) 
		goto done;

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), opdata->param.create.attrValues, 0, &cstype_name);
	osafassert(rc == SA_AIS_OK);

	avsv_sanamet_init(object_name, si_name, "safSi");
	avd_si = avd_si_get(si_name);

	if (nullptr != avd_si) {
		/* Check whether si has been assigned to any SU. */
		if (nullptr != avd_si->list_of_sisu) {
			t_sisu = avd_si->list_of_sisu;
			while(t_sisu) {
				if (t_sisu->csi_add_rem == true) {
					LOG_NO("CSI create of '%s' in queue: pending assignment"
							" for '%s'", 
							object_name.c_str(), t_sisu->su->name.c_str());
				}
				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */

			t_sisu = avd_si->list_of_sisu;
			while(t_sisu) {
				AVD_SU *su = t_sisu->su;
				/* We need to assign this csi if an extra component exists, which is unassigned.*/

				su->reset_all_comps_assign_flag();

				compcsi = t_sisu->list_of_csicomp;
				while (compcsi != nullptr) {
					compcsi->comp->set_assigned(true);
					compcsi = compcsi->susi_csicomp_next;
				}

				t_comp = su->find_unassigned_comp_that_provides_cstype(Amf::to_string(&cstype_name));

				/* Component not found.*/
				if (nullptr == t_comp) {
					/* This means that all the components are assigned, let us assigned it to assigned
					   component.*/
					t_comp = AVD_CSI::find_assigned_comp(Amf::to_string(&cstype_name), t_sisu, su->list_of_comp);
				}
				if (nullptr == t_comp) {
					report_ccb_validation_error(opdata, "Compcsi doesn't exist or "
							"MaxActiveCSI/MaxStandbyCSI have reached for csi '%s'",
							object_name.c_str());
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */
		}/* if (nullptr != si->list_of_sisu) */
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
	const std::string object_name(Amf::to_string(&opdata->objectName));

	assert(csi);
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSType")) {
			SaNameT cstype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			if(SA_AMF_ADMIN_LOCKED != csi->si->saAmfSIAdminState) {
				report_ccb_validation_error(opdata, "Parent SI is not in locked state, SI state '%d'",
						csi->si->saAmfSIAdminState);
				goto done;
			}
			if (cstype_db->find(Amf::to_string(&cstype_name)) == nullptr) {
				report_ccb_validation_error(opdata, "CS Type not found '%s'", osaf_extended_name_borrow(&cstype_name));
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "osafAmfCSICommunicateCsiAttributeChange")) {
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
                                (attr_mod->modAttr.attrValues == nullptr)) {
                                report_ccb_validation_error(opdata,
                                     "Invalid modification of osafAmfCSICommunicateCsiAttributeChange, valid (0 or 1)");
                                goto done;
			}
                        uint32_t val = static_cast<uint32_t>(*(uint32_t*)attr_mod->modAttr.attrValues[0]);
                        if ((val != true) && (val != false)) {
                                report_ccb_validation_error(opdata,
                                      "Modification of osafAmfCSICommunicateCsiAttributeChange fails," 
				      " Invalid Input %d",val);
                                goto done;
                        }
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSIDependencies")) {
			//Reject replacement of CSI deps, only deletion and addition are supported.	
			if (attr_mod->modType == SA_IMM_ATTR_VALUES_REPLACE) {
				report_ccb_validation_error(opdata,
					"'%s' - replacement of CSI dependency is not supported",
					object_name.c_str());
				goto done;
				
			}
			const std::string required_dn(Amf::to_string(static_cast<SaNameT *>(attr_mod->modAttr.attrValues[0])));
			const AVD_CSI *required_csi = csi_db->find(required_dn);

			// Required CSI must exist in current model
			if (required_csi == nullptr) {
				report_ccb_validation_error(opdata,
						"CSI '%s' does not exist", required_dn.c_str());
				goto done;
			}

			// Required CSI must be contained in the same SI
			std::string si_name;
			avsv_sanamet_init(required_dn, si_name, "safSi");
			if (object_name.find(si_name) == std::string::npos) {
				report_ccb_validation_error(opdata,
						"'%s' is not in the same SI as '%s'",
						object_name.c_str(), required_dn.c_str());
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
					if (csi_dep->csi_dep_name_value.compare(object_name) == 0) {
						// the required CSI requires this CSI
						report_ccb_validation_error(opdata,
								"cyclic dependency between '%s' and '%s'",
								object_name.c_str(), required_dn.c_str());
						goto done;
					}
				}

				// don't allow adding the same dep again
				for (csi_dep = csi->saAmfCSIDependencies; csi_dep; csi_dep = csi_dep->csi_dep_next) {
					if (csi_dep->csi_dep_name_value.compare(required_dn.c_str()) == 0) {
						// dep already exist, should we return OK instead?
						report_ccb_validation_error(opdata,
								"dependency between '%s' and '%s' already exist",
								object_name.c_str(), required_dn.c_str());
						goto done;
					}
				}

				// disallow dep between same CSIs
				if (csi->name.compare(required_dn.c_str()) == 0) {
					report_ccb_validation_error(opdata,
						"dependency for '%s' to itself", csi->name.c_str());
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
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	csi = csi_db->find(object_name);

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		if (csi == nullptr) {
			/* This means that csi has been deleted during checkpointing at STDBY and completed callback
			   has arrived delayed.*/
			TRACE("CSI delete completed (STDBY): '%s' does not exist", osaf_extended_name_borrow(&opdata->objectName));
		}
		//IMM honors response of completed callback only from active amfd, so reply ok from standby amfd.
		rc = SA_AIS_OK;
		opdata->userData = csi;	/* Save for later use in apply */
		goto done;
	}

	if(AVD_SG_FSM_STABLE != csi->si->sg_of_si->sg_fsm_state) {
		report_ccb_validation_error(opdata, "SG('%s') fsm state('%u') is not in AVD_SG_FSM_STABLE(0)",
				csi->si->sg_of_si->name.c_str(), csi->si->sg_of_si->sg_fsm_state);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if (csi->si->saAmfSIAdminState != SA_AMF_ADMIN_LOCKED) {
		if (nullptr == csi->si->list_of_sisu) {
			/* UnLocked but not assigned. Safe to delete.*/
		} else {/* Assigned to some SU, check whether the last csi. */
			/* SI is unlocked and this is the last csi to be deleted, then donot allow it. */
			if (csi->si->list_of_csi->si_list_of_csi_next == nullptr) {
				report_ccb_validation_error(opdata, " csi('%s') is the last csi in si('%s'). Lock SI and"
						" then delete csi.", csi->name.c_str(), csi->si->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			t_sisu = csi->si->list_of_sisu;
			while(t_sisu) {
				if (t_sisu->csi_add_rem == true) {
					LOG_NO("CSI remove of '%s' rejected: pending "
							"assignment for '%s'", 
							csi->name.c_str(), t_sisu->su->name.c_str());
					if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done; 
					}
				}
				t_sisu = t_sisu->si_next;
			}/*  while(t_sisu) */
		}
	} else {
		if (csi->list_compcsi != nullptr) {
			report_ccb_validation_error(opdata, "SaAmfCSI '%s' is in use", csi->name.c_str());
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

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
		if ((csi == nullptr) || (csi_in_db == nullptr)) {
			/* This means that csi has been deleted during checkpointing at STDBY and delete callback
			   has arrived delayed.*/
			LOG_WA("CSI delete apply (STDBY): csi does not exist");
			goto done;
		}
		if (csi->list_compcsi == nullptr ) {
			/* delete the pg-node list */
			avd_pg_csi_node_del_all(avd_cb, csi);

			/* free memory and remove from DB */
			avd_csi_delete(csi);
		}
		goto done;
	}

        TRACE_ENTER2("'%s'", csi ? csi->name.c_str() : nullptr);

	/* Check whether si has been assigned to any SU. */
	if ((nullptr != csi->si->list_of_sisu) && 
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
					LOG_ER("susi send failure for su'%s' and si'%s'", t_sisu->su->name.c_str(), t_sisu->si->name.c_str());
					goto done;
				}

			}
			t_sisu->csi_add_rem = static_cast<SaBoolT>(true);
			t_sisu->comp_name = Amf::to_string(&t_csicomp->comp->comp_info.name);
			t_sisu->csi_name = t_csicomp->csi->name;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_sisu, AVSV_CKPT_AVD_SI_ASS);
			t_sisu = t_sisu->si_next;
		}/* while(t_sisu) */

	} else { /* if (nullptr != csi->si->list_of_sisu) */
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
	AVD_CSI *csi = nullptr;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));
 
	csi = csi_db->find(Amf::to_string(&opdata->objectName));
	assert(csi != nullptr);
	AVD_SI *si = csi->si;
	assert(si != nullptr);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSType")) {
			AVD_CS_TYPE *csi_type;
			SaNameT cstype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			TRACE("saAmfCSType modified from '%s' to '%s' for Csi'%s'", csi->saAmfCSType.c_str(),
					osaf_extended_name_borrow(&cstype_name), csi->name.c_str());
			csi_type = cstype_db->find(Amf::to_string(&cstype_name));
			avd_cstype_remove_csi(csi);
			csi->saAmfCSType = Amf::to_string(&cstype_name);
			csi->cstype = csi_type;
			avd_cstype_add_csi(csi);
		} else if (!strcmp(attr_mod->modAttr.attrName, "osafAmfCSICommunicateCsiAttributeChange")) {
			csi->osafAmfCSICommunicateCsiAttributeChange = static_cast<bool>(*((bool*)attr_mod->modAttr.attrValues[0]));
			LOG_NO("Modified osafAmfCSICommunicateCsiAttributeChange for '%s' to '%u'",
					csi->name.c_str(), csi->osafAmfCSICommunicateCsiAttributeChange);
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfCSIDependencies")) {
			if (attr_mod->modType == SA_IMM_ATTR_VALUES_ADD) {
				assert(attr_mod->modAttr.attrValuesNumber == 1);
				si->remove_csi(csi);
				AVD_CSI_DEPS *new_csi_dep = new AVD_CSI_DEPS();
				new_csi_dep->csi_dep_name_value = Amf::to_string(static_cast<SaNameT*>(attr_mod->modAttr.attrValues[0]));
				bool already_exist = csi_add_csidep(csi, new_csi_dep);
				if (already_exist)
					delete new_csi_dep;
				csi->rank = 0; // indicate that there is a dep to another CSI
				si->add_csi(csi);
			} else if (attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) {
				assert(attr_mod->modAttr.attrValuesNumber == 1);
				const SaNameT *required_dn = (SaNameT*) attr_mod->modAttr.attrValues[0];
				csi_remove_csidep(csi, Amf::to_string(required_dn));
				
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
	AVD_CSI *csi = nullptr;
	if ((csi = csi_db->find(Amf::to_string(&opdata->objectName))) == nullptr) {
		/* this check is added because, some times there is
		   possibility that before getting ccb apply callback
		   we might get compcsi create checkpoint and csi will
		   be created as part of checkpoint processing */
		csi = csi_create(Amf::to_string(&opdata->objectName));
	} 
	csi_get_attr_and_add_to_model(csi, opdata->param.create.attrValues,
			Amf::to_string(opdata->param.create.parentName));

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
	if (csi->si->list_of_sisu != nullptr) {
		for(t_sisu = csi->si->list_of_sisu; t_sisu != nullptr; t_sisu = t_sisu->si_next) {
			if (t_sisu->csi_add_rem == true) {
				LOG_NO("CSI create '%s' delayed: pending assignment for '%s'",
						csi->name.c_str(), t_sisu->su->name.c_str());
				goto done;
			}
		}
	}

	/* Check whether si has been assigned to any SU. */
	if (nullptr != csi->si->list_of_sisu) {
		t_sisu = csi->si->list_of_sisu;
		while(t_sisu) {
			/* We need to assign this csi if an extra component exists, which is unassigned.*/

			t_sisu->su->reset_all_comps_assign_flag();

			compcsi = t_sisu->list_of_csicomp;
			while (compcsi != nullptr) {
				compcsi->comp->set_assigned(true);
				compcsi = compcsi->susi_csicomp_next;
			}

			t_comp = t_sisu->su->find_unassigned_comp_that_provides_cstype(csi->saAmfCSType);

			/* Component not found.*/
			if (nullptr == t_comp) {
				/* This means that all the components are assigned, let us assigned it to assigned 
				   component.*/
				t_comp = AVD_CSI::find_assigned_comp(csi->saAmfCSType, t_sisu, t_sisu->su->list_of_comp);
			}
			if (nullptr == t_comp) {
				LOG_ER("Compcsi doesn't exist or MaxActiveCSI/MaxStandbyCSI have reached for csi '%s'",
						csi->name.c_str());
				goto done;
			}

			if ((compcsi = avd_compcsi_create(t_sisu, csi, t_comp, true)) == nullptr) {
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
			t_sisu->comp_name = Amf::to_string(&compcsi->comp->comp_info.name);
			t_sisu->csi_name = compcsi->csi->name;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_sisu, AVSV_CKPT_AVD_SI_ASS);
			t_sisu = t_sisu->si_next;
		}/* while(t_sisu) */

	}/* if (nullptr != csi->si->list_of_sisu) */
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

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
       const std::string& csi_dn, const std::string& _comp_dn)
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
			nullptr
	};

	const SaNameTWrapper comp_dn(_comp_dn);
	avsv_create_association_class_dn(comp_dn, nullptr, "safCSIComp", &dn);

	TRACE("Adding %s", osaf_extended_name_borrow(&dn));
	avd_saImmOiRtObjectCreate("SaAmfCSIAssignment",	csi_dn, attrValues);
	osaf_extended_name_free(&dn);
}

AVD_COMP_CSI_REL *avd_compcsi_create(AVD_SU_SI_REL *susi, AVD_CSI *csi,
	AVD_COMP *comp, bool create_in_imm)
{
	AVD_COMP_CSI_REL *compcsi = nullptr;

	if ((csi == nullptr) && (comp == nullptr)) {
		LOG_ER("Either csi or comp is nullptr");
                return nullptr;
	}
	TRACE_ENTER2("Comp'%s' and Csi'%s'", osaf_extended_name_borrow(&comp->comp_info.name), csi->name.c_str());

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
	if (csi->list_compcsi == nullptr) {
		csi->list_compcsi = compcsi;
	} else {
		compcsi->csi_csicomp_next = csi->list_compcsi;
		csi->list_compcsi = compcsi;
	}
	csi->compcsi_cnt++;

	/* Add to the SUSI owned list */
	if (susi->list_of_csicomp == nullptr) {
		susi->list_of_csicomp = compcsi;
	} else {
		compcsi->susi_csicomp_next = susi->list_of_csicomp;
		susi->list_of_csicomp = compcsi;
	}
	if (create_in_imm)
		avd_create_csiassignment_in_imm(susi->state, csi->name, Amf::to_string(&comp->comp_info.name));
done:
	TRACE_LEAVE();
	return compcsi;
}

/** Delete an SaAmfCSIAssignment from IMM
 * 
 * @param comp_dn
 * @param csi_dn
 */
static void avd_delete_csiassignment_from_imm(const std::string& comp_dn, const std::string& csi_dn)
{
       SaNameT dn;
       const SaNameTWrapper comp(comp_dn);
       const SaNameTWrapper csi(csi_dn);

       avsv_create_association_class_dn(comp, csi, "safCSIComp", &dn);
       TRACE("Deleting %s", osaf_extended_name_borrow(&dn));

       avd_saImmOiRtObjectDelete(Amf::to_string(&dn));
       osaf_extended_name_free(&dn);
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
	AVD_COMP_CSI_REL *i_compcsi, *prev_compcsi = nullptr;

	TRACE_ENTER();
	while (susi->list_of_csicomp != nullptr) {
		lcomp_csi = susi->list_of_csicomp;

		i_compcsi = lcomp_csi->csi->list_compcsi;
		while ((i_compcsi != nullptr) && (i_compcsi != lcomp_csi)) {
			prev_compcsi = i_compcsi;
			i_compcsi = i_compcsi->csi_csicomp_next;
		}
		if (i_compcsi != lcomp_csi) {
			/* not found */
		} else {
			if (prev_compcsi == nullptr) {
				lcomp_csi->csi->list_compcsi = i_compcsi->csi_csicomp_next;
			} else {
				prev_compcsi->csi_csicomp_next = i_compcsi->csi_csicomp_next;
			}
			lcomp_csi->csi->compcsi_cnt--;

			/* trigger pg upd */
			if (!ckpt) {
				avd_pg_compcsi_chg_prc(cb, lcomp_csi, true);
			}

			i_compcsi->csi_csicomp_next = nullptr;
		}

		susi->list_of_csicomp = lcomp_csi->susi_csicomp_next;
		lcomp_csi->susi_csicomp_next = nullptr;
		prev_compcsi = nullptr;
		avd_delete_csiassignment_from_imm(Amf::to_string(&lcomp_csi->comp->comp_info.name), lcomp_csi->csi->name);
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
	AVD_COMP_CSI_REL *t_compcsi, *t_compcsi_susi, *prev_compcsi = nullptr;

	TRACE_ENTER2("Csi'%s', compcsi_cnt'%u'", comp_csi->csi->name.c_str(), comp_csi->csi->compcsi_cnt);

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
	if (nullptr == prev_compcsi)
		susi->list_of_csicomp = t_compcsi_susi->susi_csicomp_next;
	else {
		prev_compcsi->susi_csicomp_next = t_compcsi_susi->susi_csicomp_next;
		t_compcsi_susi->susi_csicomp_next = nullptr;
	}

	prev_compcsi =  nullptr;
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
	if (nullptr == prev_compcsi)
		comp_csi->csi->list_compcsi = t_compcsi->csi_csicomp_next;
	else {
		prev_compcsi->csi_csicomp_next = t_compcsi->csi_csicomp_next;
		t_compcsi->csi_csicomp_next = nullptr;
	}

	osafassert(t_compcsi == t_compcsi_susi);
	comp_csi->csi->compcsi_cnt--;

	if (!ckpt)
		avd_snd_pg_upd_msg(avd_cb, comp_csi->comp->su->su_on_node, comp_csi, SA_AMF_PROTECTION_GROUP_REMOVED, std::string(""));
	avd_delete_csiassignment_from_imm(Amf::to_string(&comp_csi->comp->comp_info.name), comp_csi->csi->name);
	delete comp_csi;

	TRACE_LEAVE();
}

void avd_csi_remove_csiattr(AVD_CSI *csi, AVD_CSI_ATTR *attr)
{
	AVD_CSI_ATTR *i_attr = nullptr;
	AVD_CSI_ATTR *p_attr = nullptr;

	TRACE_ENTER();
	/* remove ATTR from CSI list */
	i_attr = csi->list_attributes;

	while ((i_attr != nullptr) && (i_attr != attr)) {
		p_attr = i_attr;
		i_attr = i_attr->attr_next;
	}

	if (i_attr != attr) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (p_attr == nullptr) {
			csi->list_attributes = i_attr->attr_next;
		} else {
			p_attr->attr_next = i_attr->attr_next;
			osaf_extended_name_free(&attr->name_value.name);
			osaf_extended_name_free(&attr->name_value.value);
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
	while (ptr != nullptr) {
		cnt++;
		if (ptr->attr_next != nullptr)
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
	avd_class_impl_set("SaAmfCSI", nullptr, nullptr, csi_ccb_completed_cb,
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
        for (AVD_CSI_DEPS *spons_csi = csi->saAmfCSIDependencies; spons_csi != nullptr;
                spons_csi = spons_csi->csi_dep_next) {
		bool is_sponsor_assigned = false;
		
		AVD_CSI *tmp_csi =  csi_db->find(spons_csi->csi_dep_name_value);

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

/**
 * Clean up COMPCSI objects by searching for SaAmfCSIAssignment instances in IMM
 * @return SA_AIS_OK when OK
 */
/**
 * Re-create csi assignment and update comp related states, which are
 * collected after headless
 * Update relevant runtime attributes
 * @return SA_AIS_OK when OK
 */
SaAisErrorT avd_compcsi_recreate(AVSV_N2D_ND_CSICOMP_STATE_MSG_INFO *info)
{
	AVD_SU_SI_REL *susi;
	const AVD_SI *si;
	AVD_CSI *csi;
	AVD_COMP *comp;
	const AVSV_CSICOMP_STATE_MSG *csicomp;
	const AVSV_COMP_STATE_MSG *comp_state;

	TRACE_ENTER();

	for (csicomp = info->csicomp_list; csicomp != nullptr; csicomp=csicomp->next) {
		csi = csi_db->find(Amf::to_string(&csicomp->safCSI));
		osafassert(csi);

		comp = comp_db->find(Amf::to_string(&csicomp->safComp));
		osafassert(comp);

		TRACE("Received CSICOMP state msg: csi %s, comp %s",
			osaf_extended_name_borrow(&csicomp->safCSI), osaf_extended_name_borrow(&csicomp->safComp));

		si = csi->si;
		osafassert(si);

		susi = avd_susi_find(avd_cb, comp->su->name, si->name);
		if (susi == 0) {
			LOG_ER("SU_SI_REL record for SU '%s' and SI '%s' was not found",
                         comp->su->name.c_str(), si->name.c_str());
			return SA_AIS_ERR_NOT_EXIST;
		}

		AVD_COMP_CSI_REL *compcsi = avd_compcsi_create(susi, csi, comp, true);
		osafassert(compcsi);
	}

	for (comp_state = info->comp_list; comp_state != nullptr; comp_state = comp_state->next) {
		comp = comp_db->find(Amf::to_string(&comp_state->safComp));
		osafassert(comp);

		// operation state
		comp->avd_comp_oper_state_set(static_cast<SaAmfOperationalStateT>(comp_state->comp_oper_state));

		// . update saAmfCompReadinessState after SU:saAmfSuReadinessState
		// . saAmfCompCurrProxyName and saAmfCompCurrProxiedNames wouldn't change during headless
		//   so they need not to update

		// presense state
		comp->avd_comp_pres_state_set(static_cast<SaAmfPresenceStateT>(comp_state->comp_pres_state));

		// restart count
		comp->saAmfCompRestartCount = comp_state->comp_restart_cnt;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_RESTART_COUNT);
		avd_saImmOiRtObjectUpdate(Amf::to_string(&comp->comp_info.name),
			const_cast<SaImmAttrNameT>("saAmfCompRestartCount"), SA_IMM_ATTR_SAUINT32T,
			&comp->saAmfCompRestartCount);
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}

void avd_compcsi_cleanup_imm_object(AVD_CL_CB *cb)
{

	SaAisErrorT rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;

	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	AVD_SU_SI_REL *susi;

	const char *className = "SaAmfCSIAssignment";
	const SaImmAttrNameT siass_attributes[] = {
		const_cast<SaImmAttrNameT>("safCSIComp"),
		NULL
	};

	TRACE_ENTER();

	osafassert(cb->scs_absence_max_duration > 0);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(cb->immOmHandle, NULL, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		  siass_attributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &dn,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		AVD_SI *si = si_db->find(strstr(osaf_extended_name_borrow(&dn), "safSi"));
		osafassert(si);

		AVD_CSI *csi = csi_db->find(strstr(osaf_extended_name_borrow(&dn), "safCsi"));
		osafassert(csi);
		SaNameT comp_name;
		avsv_sanamet_init_from_association_dn(&dn, &comp_name, "safComp", csi->name.c_str());
		AVD_COMP *comp = comp_db->find(Amf::to_string(&comp_name));
		if (comp == nullptr) {
			LOG_WA("Component %s not found in comp_db", osaf_extended_name_borrow(&comp_name));
			osaf_extended_name_free(&comp_name);
			continue;
		}
		osaf_extended_name_free(&comp_name);

		susi = avd_susi_find(avd_cb, comp->su->name, si->name);
		if (susi == nullptr || (susi->fsm == AVD_SU_SI_STATE_ABSENT)) {
			avd_saImmOiRtObjectDelete(Amf::to_string(&dn));
		}
	}

	(void)immutil_saImmOmSearchFinalize(searchHandle);

done:
    TRACE_LEAVE();

}
