/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amfd/sutype.h"
#include "amf/amfd/sutcomptype.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/ntf.h"
#include "amf/amfd/proc.h"
#include <algorithm>

AmfDb<std::string, AVD_SUTYPE> *sutype_db = nullptr;

//
AVD_SUTYPE::AVD_SUTYPE(const std::string& dn) :
	name(dn)
{
}

static AVD_SUTYPE *sutype_new(const std::string& dn)
{
	AVD_SUTYPE *sutype = new AVD_SUTYPE(dn);

	return sutype;
}

static void sutype_delete(AVD_SUTYPE **sutype)
{
	osafassert(true == (*sutype)->list_of_su.empty());
	delete *sutype;
	*sutype = nullptr;
}

static void sutype_db_add(AVD_SUTYPE *sutype)
{
	unsigned int rc = sutype_db->insert(sutype->name,sutype);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

static AVD_SUTYPE *sutype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	const SaImmAttrValuesT_2 *attr;
	AVD_SUTYPE *sutype;
	int rc = 0;
	unsigned i = 0;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn.c_str());

	if ((sutype = sutype_new(dn)) == nullptr) {
		LOG_ER("avd_sutype_new failed");
		return nullptr;
	}

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutIsExternal"), attributes, 0, &sutype->saAmfSutIsExternal);
	osafassert(error == SA_AIS_OK);

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutDefSUFailover"), attributes, 0, &sutype->saAmfSutDefSUFailover);
	osafassert(error == SA_AIS_OK);

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfSutProvidesSvcTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	sutype->number_svc_types = attr->attrValuesNumber;
	osafassert(sutype->saAmfSutProvidesSvcTypes.empty() == true);

	for (i = 0; i < sutype->number_svc_types; i++) {
		SaNameT svc_type;
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutProvidesSvcTypes"), attributes, i, &svc_type) != SA_AIS_OK) {
			LOG_ER("Get saAmfSutProvidesSvcTypes FAILED for '%s'", dn.c_str());
			osafassert(0);
		}
		sutype->saAmfSutProvidesSvcTypes.push_back(Amf::to_string(&svc_type));
		TRACE("%s", sutype->saAmfSutProvidesSvcTypes.back().c_str());
	}

	rc = 0;

	if (rc != 0)
		sutype_delete(&sutype);

	TRACE_LEAVE();

	return sutype;
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaBoolT abool;
	/* int i = 0; */
	std::string::size_type pos;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* Should be children to the SU Base type */
	if (dn.compare(pos + 1, 10, "safSuType=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos +1).c_str(), dn.c_str());
		return 0;
	}
#if 0
	/*  TODO we need proper Svc Type handling for this, revisit */

	/* Make sure all Svc types exist */
	for (i = 0; i < su_type->number_svc_types; i++) {
		AVD_AMF_SVC_TYPE *svc_type =
		    avd_svctype_find(avd_cb, su_type->saAmfSutProvidesSvcTypes[i], true);
		if (svc_type == nullptr) {
			/* Svc type does not exist in current model, check CCB */
			if ((opdata != nullptr) &&
			    (ccbutil_getCcbOpDataByDN(opdata->ccbId, &su_type->saAmfSutProvidesSvcTypes[i]) == nullptr)) {
				LOG_ER("Svc type '%s' does not exist either in model or CCB",
					su_type->saAmfSutProvidesSvcTypes[i]);
				return SA_AIS_ERR_BAD_OPERATION;
			}
			LOG_ER("AMF Svc type '%s' does not exist",
				su_type->saAmfSutProvidesSvcTypes[i].value);
			return -1;
		}
	}
#endif
	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutDefSUFailover"), attributes, 0, &abool);
	osafassert(rc == SA_AIS_OK);

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutDefSUFailover"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSutDefSUFailover %u for '%s'", abool, dn.c_str());
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutIsExternal"), attributes, 0, &abool);
	osafassert(rc == SA_AIS_OK);

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutIsExternal"), attributes, 0, &abool) == SA_AIS_OK) &&
			(abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSutIsExternal %u for '%s'", abool, dn.c_str());
		return 0;
	}

	return 1;
}

SaAisErrorT avd_sutype_config_get(void)
{
	AVD_SUTYPE *sut;
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSUType";

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		nullptr, &searchHandle);
	
	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		const std::string temp_dn(Amf::to_string(&dn));
		if (!is_config_valid(temp_dn, attributes, nullptr))
		    goto done2;

		if (( sut = sutype_db->find(temp_dn)) == nullptr) {

			if ((sut = sutype_create(temp_dn, attributes)) == nullptr) {
				error = SA_AIS_ERR_FAILED_OPERATION;
				goto done2;
			}

			sutype_db_add(sut);
		}

		if (avd_sutcomptype_config_get(temp_dn, sut) != SA_AIS_OK) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

/**
 * This function handles modify operations on SaAmfSUType objects. 
 * @param ptr to opdata 
 */
static void sutype_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));
	AVD_SUTYPE *sut = sutype_db->find(Amf::to_string(&opdata->objectName));

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSutDefSUFailover")) {
			uint32_t old_value = sut->saAmfSutDefSUFailover;
			sut->saAmfSutDefSUFailover = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);
			TRACE("Modified saAmfSutDefSUFailover from '%u' to '%u'", old_value,
					sut->saAmfSutDefSUFailover);
			/* Modify saAmfSUFailover for SUs which had inherited saAmfSutDefSUFailover.
				Modification will not be done for the NPI SU */
			if (old_value != sut->saAmfSutDefSUFailover) {
				for (const auto& su : sut->list_of_su) {
					if ((!su->saAmfSUFailover_configured) && (su->saAmfSUPreInstantiable)) {
						su->set_su_failover(static_cast<bool>(sut->saAmfSutDefSUFailover));
					}
				}
			}
		} else {
			osafassert(0);
		}
	}

	TRACE_LEAVE();
}

static void sutype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SUTYPE *sut;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sut = sutype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(sut);
		sutype_db_add(sut);
		break;
	case CCBUTIL_MODIFY:
		sutype_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		sut = sutype_db->find(Amf::to_string(&opdata->objectName));
		sutype_db->erase(sut->name);
		sutype_delete(&sut);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * This routine validates modify CCB operations on SaAmfSUType objects.
 * @param   Ccb Util Oper Data
 */

static SaAisErrorT sutype_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_SUTYPE *sut = sutype_db->find(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == nullptr)) {
			report_ccb_validation_error(opdata, "Attributes cannot be deleted in SaAmfSUType");
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSutDefSUFailover")) {
			uint32_t sut_failover = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);

			if (sut_failover > SA_TRUE) {
				report_ccb_validation_error(opdata, "invalid saAmfSutDefSUFailover in:'%s'", sut->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}	
	
			for (const auto& su : sut->list_of_su) {
				if (su->saAmfSUFailover_configured)
					continue;

				/* If SG is not in stable state and amfnd is already busy in the handling of some fault,
				   modification of this attribute in an unstable SG can affects the recovery at amfd though 
				   amfnd part of recovery had been done without the modified value of this attribute.
				   So modification should be allowed only in the stable state of SG.
				 */
				if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
					rc = SA_AIS_ERR_TRY_AGAIN;
					report_ccb_validation_error(opdata, "SG state is not stable"); 
					goto done;
				}
			}
		} else {
			report_ccb_validation_error(opdata, "attribute is non writable:'%s' ", attr_mod->modAttr.attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT sutype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SUTYPE *sut;
	bool su_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = sutype_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		sut = sutype_db->find(Amf::to_string(&opdata->objectName));

		/* check whether there exists a delete operation for 
		 * each of the SU in the su_type list in the current CCB 
		 */                      

		for (const auto& su : sut->list_of_su) {
			const SaNameTWrapper su_name(su->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, su_name);
			if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
				su_exist = true;
				break;
			}
		}

		if (su_exist == true) {
			report_ccb_validation_error(opdata, "SaAmfSUType '%s'is in use",sut->name.c_str());
			goto done;
		}
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

	/* Add SU to list in SU Type */
void avd_sutype_add_su(AVD_SU* su)
{
	AVD_SUTYPE *sut = su->su_type;
	sut->list_of_su.push_back(su);
}

void avd_sutype_remove_su(AVD_SU* su) {
  AVD_SUTYPE *su_type = su->su_type;

  if (su_type != nullptr) {
    su_type->list_of_su.erase(std::remove(su_type->list_of_su.begin(),
                                          su_type->list_of_su.end(), su), su_type->list_of_su.end());
  }
}

void avd_sutype_constructor(void)
{

	sutype_db = new AmfDb<std::string, AVD_SUTYPE>;
	avd_class_impl_set("SaAmfSUBaseType", nullptr, nullptr,
		avd_imm_default_OK_completed_cb, nullptr);
	avd_class_impl_set("SaAmfSUType", nullptr, nullptr,
		sutype_ccb_completed_cb, sutype_ccb_apply_cb);
}
