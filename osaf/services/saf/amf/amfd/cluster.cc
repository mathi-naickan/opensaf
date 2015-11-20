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

#include <immutil.h>
#include <logtrace.h>
#include <saflog.h>

#include <util.h>
#include <cluster.h>
#include <imm.h>
#include <evt.h>
#include <proc.h>

/* Singleton cluster object */
static AVD_CLUSTER _avd_cluster;

/* Reference to cluster object */
AVD_CLUSTER *avd_cluster = &_avd_cluster;

/****************************************************************************
 *  Name          : avd_tmr_cl_init_func
 * 
 *  Description   : This routine is the AMF cluster initialisation timer expiry
 *                  routine handler. This routine calls the SG FSM
 *                  handler for each of the application SG in the AvSv
 *                  database. At the begining of the processing the AvD state
 *                  is changed to AVD_APP_STATE, the stable state.
 * 
 *  Arguments     : cb         -  AvD cb .
 *                  evt        -  ptr to the received event
 * 
 *  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 * 
 *  Notes         : None.
 ***************************************************************************/

void avd_cluster_tmr_init_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	TRACE_ENTER();
	saflog(LOG_NOTICE, amfSvcUsrName, "Cluster startup timeout, assigning SIs to SUs");

	osafassert(evt->info.tmr.type == AVD_TMR_CL_INIT);

	if (avd_cluster->saAmfClusterAdminState != SA_AMF_ADMIN_UNLOCKED) {
		LOG_WA("Admin state of cluster is locked");
		goto done;
	}

	if (cb->init_state != AVD_INIT_DONE) {
		LOG_ER("wrong state %u", cb->init_state);
		goto done;
	}

	/* change state to application state. */
	cb->init_state = AVD_APP_STATE;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, cb, AVSV_CKPT_AVD_CB_CONFIG);

	/* call the realignment routine for each of the SGs in the
	 * system that are not NCS specific.
	 */

	for (std::map<std::string, AVD_SG*>::const_iterator it = sg_db->begin();
			it != sg_db->end(); it++) {
		AVD_SG *i_sg = it->second;
		if ((i_sg->list_of_su == NULL) || (i_sg->sg_ncs_spec == true)) {
			continue;
		}
		i_sg->realign(cb, i_sg);
	}

done:
	TRACE_LEAVE();
}

static void ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfClusterStartupTimeout")) {
			SaTimeT cluster_startup_timeout = *((SaTimeT *)attr_mod->modAttr.attrValues[0]);
			TRACE("saAmfClusterStartupTimeout modified from '%llu' to '%llu'", 
					avd_cluster->saAmfClusterStartupTimeout, cluster_startup_timeout);
			avd_cluster->saAmfClusterStartupTimeout = cluster_startup_timeout;
		}
	}
}

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfClusterStartupTimeout")) {
			SaTimeT cluster_startup_timeout = *((SaTimeT *)attr_mod->modAttr.attrValues[0]);
			if (0 == cluster_startup_timeout) {
				report_ccb_validation_error(opdata, "Invalid saAmfClusterStartupTimeout %llu",
						cluster_startup_timeout);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
	}

 done:
	return rc;
}

static SaAisErrorT cluster_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		report_ccb_validation_error(opdata, "SaAmfCluster objects cannot be created");
		goto done;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		report_ccb_validation_error(opdata, "SaAmfCluster objects cannot be deleted");
		goto done;
		break;
	default:
		osafassert(0);
		break;
	}

 done:
	return rc;
}

static void cluster_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_MODIFY:
		ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_CREATE:
	case CCBUTIL_DELETE:
		/* fall through */
	default:
		osafassert(0);
		break;
	}
}

static void cluster_admin_op_cb(SaImmOiHandleT immOiHandle,
	SaInvocationT invocation, const SaNameT *object_name,
	SaImmAdminOperationIdT op_id, const SaImmAdminOperationParamsT_2 **params)
{
	switch (op_id) {
	case SA_AMF_ADMIN_SHUTDOWN:
	case SA_AMF_ADMIN_UNLOCK:
	case SA_AMF_ADMIN_LOCK:
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL, "Not supported"); 
		break;
	}
}

/**
 * Get configuration for the AMF cluster object from IMM and
 * initialize the AVD internal object.
 * 
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_cluster_config_get(void)
{
	SaAisErrorT error, rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCluster";

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
						       SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
						       &searchParam, NULL, &searchHandle)) != SA_AIS_OK) {
		LOG_ER("saImmOmSearchInitialize failed: %u", error);
		goto done;
	}

	if ((error = immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes)) != SA_AIS_OK) {
		LOG_ER("No cluster object found");
		goto done;
	}

	avd_cluster->saAmfCluster = dn;

	/* Cluster should be root object */
	if (strchr((char *)dn.value, ',') != NULL) {
		LOG_ER("Parent to '%s' is not root", dn.value);
		return static_cast<SaAisErrorT>(-1);
	}

	TRACE("'%s'", dn.value);

	if (immutil_getAttr( const_cast<SaImmAttrNameT>("saAmfClusterStartupTimeout"), attributes,
			    0, &avd_cluster->saAmfClusterStartupTimeout) == SA_AIS_OK) {
		/* If zero, use default value */
		if (avd_cluster->saAmfClusterStartupTimeout == 0)
			avd_cluster->saAmfClusterStartupTimeout = AVSV_CLUSTER_INIT_INTVL;
	} else {
		LOG_ER("Get saAmfClusterStartupTimeout FAILED for '%s'", dn.value);
		goto done;
	}

	if (immutil_getAttr( const_cast<SaImmAttrNameT>("saAmfClusterAdminState"), attributes, 0, &avd_cluster->saAmfClusterAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		avd_cluster->saAmfClusterAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	if (!avd_admin_state_is_valid(avd_cluster->saAmfClusterAdminState, NULL)) {
		LOG_ER("Invalid saAmfClusterAdminState %u", avd_cluster->saAmfClusterAdminState);
		return static_cast<SaAisErrorT>(-1);
	}

	rc = SA_AIS_OK;

 done:
	(void)immutil_saImmOmSearchFinalize(searchHandle);

	return rc;
}

void avd_cluster_constructor(void)
{
   avd_class_impl_set("SaAmfCluster", NULL, cluster_admin_op_cb,
		cluster_ccb_completed_cb, cluster_ccb_apply_cb);
}

