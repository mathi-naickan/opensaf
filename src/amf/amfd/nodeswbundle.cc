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

#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"
#include "amf/amfd/amfd.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"

/**
 * Validate configuration attributes for an AMF node Sw bundle object.
 * Validate the naming tree.
 * @param ng
 * @param opdata
 * 
 * @return int
 */
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	std::string parent;
	const char *path_prefix;

	parent = avd_getparent(dn);
	if (parent.empty() == true) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* Should be children to nodes */
	if (parent.compare(0, 11, "safAmfNode=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent.c_str(), dn.c_str());
		return 0;
	}

	path_prefix = immutil_getStringAttr(attributes, "saAmfNodeSwBundlePathPrefix", 0);
	osafassert(path_prefix);

	if (path_prefix[0] != '/') {
		report_ccb_validation_error(opdata, "Invalid absolute path '%s' for '%s' ", path_prefix, dn.c_str());
		return 0;
	}

	return 1;
}

/**
 * Check the SU list for components that reference the bundle
 * @param bundle_dn_to_delete
 * @param node_dn
 * @param su_list
 * 
 * @return int
 */
static int is_swbdl_delete_ok_for_node(const std::string& bundle_dn_to_delete,
	const std::string& node_dn, const std::vector<AVD_SU*> &su_list, CcbUtilOperationData_t *opdata)
{
	SaNameT bundle_dn;
	const SaNameTWrapper node(node_dn);

	for (const auto& su : su_list) {
		for (const auto& comp : su->list_of_comp) {
			const SaNameTWrapper sw_bundle(comp->comp_type->saAmfCtSwBundle);
			avsv_create_association_class_dn(sw_bundle, 
				node, "safInstalledSwBundle", &bundle_dn);

			if (bundle_dn_to_delete.compare(Amf::to_string(&bundle_dn)) == 0) {
				if (su->su_on_node->node_state == AVD_AVND_STATE_ABSENT ||
				    (!su->sg_of_su->sg_ncs_spec &&
				     (comp->su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION))) {
					continue;
				} else {
					report_ccb_validation_error(opdata, "'%s' admin state is not locked instantiaion",
							su->name.c_str());
					return 0;
				}
			}
			osaf_extended_name_free(&bundle_dn);
		}
	}

	return 1;
}

/**
 * Check all SUs on this node for bundle references
 * @param dn
 * 
 * @return int
 */
static int is_swbdl_delete_ok(const std::string& bundle_dn, CcbUtilOperationData_t *opdata)
{
	const AVD_AVND *node;
	std::string node_dn;

	/* Check if any comps are referencing this bundle */
	avsv_sanamet_init(bundle_dn, node_dn, "safAmfNode=");
	node = avd_node_get(node_dn);

	if (!is_swbdl_delete_ok_for_node(bundle_dn, node_dn, node->list_of_ncs_su, opdata))
		return 0;

	if (!is_swbdl_delete_ok_for_node(bundle_dn, node_dn, node->list_of_su, opdata))
		return 0;

	return 1;
}

static SaAisErrorT nodeswbdl_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfNodeSwBundle not supported");
		break;
	case CCBUTIL_DELETE:
		if (is_swbdl_delete_ok(Amf::to_string(&opdata->objectName), opdata))
			rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void nodeswbdl_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		break;
	case CCBUTIL_DELETE:
		break;
	default:
		osafassert(0);
	}

	TRACE_LEAVE();
}

void avd_nodeswbundle_constructor(void)
{
	avd_class_impl_set("SaAmfNodeSwBundle", nullptr, nullptr,
		nodeswbdl_ccb_completed_cb, nodeswbdl_ccb_apply_cb);
}
