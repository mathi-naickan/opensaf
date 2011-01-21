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

#include <avd_util.h>
#include <avd_cluster.h>
#include <avd_imm.h>
#include <avd_evt.h>
#include <avd_proc.h>

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
	SaNameT lsg_name;
	AVD_SG *i_sg;

	TRACE_ENTER();
	saflog(LOG_NOTICE, amfSvcUsrName, "Cluster startup timeout, assigning SIs to SUs");

	assert(evt->info.tmr.type == AVD_TMR_CL_INIT);

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

	lsg_name.length = 0;
	for (i_sg = avd_sg_getnext(&lsg_name); i_sg != NULL; i_sg = avd_sg_getnext(&lsg_name)) {
		lsg_name = i_sg->name;

		if ((i_sg->list_of_su == NULL) || (i_sg->sg_ncs_spec == SA_TRUE)) {
			continue;
		}

		switch (i_sg->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			avd_sg_2n_realign_func(cb, i_sg);
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			avd_sg_nway_realign_func(cb, i_sg);
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			avd_sg_nacvred_realign_func(cb, i_sg);
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			avd_sg_npm_realign_func(cb, i_sg);
			break;

		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			avd_sg_nored_realign_func(cb, i_sg);
			break;
		}
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
				LOG_ER("Invalid saAmfClusterStartupTimeout %llu",
					avd_cluster->saAmfClusterStartupTimeout);
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
		LOG_ER("SaAmfCluster objects cannot be created");
		goto done;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		LOG_ER("SaAmfCluster objects cannot be deleted");
		goto done;
		break;
	default:
		assert(0);
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
		assert(0);
		break;
	}
}

static void cluster_admin_op_cb(SaImmOiHandleT immOiHandle,
	SaInvocationT invocation, const SaNameT *object_name,
	SaImmAdminOperationIdT op_id, const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;

	switch (op_id) {
	case SA_AMF_ADMIN_SHUTDOWN:
	case SA_AMF_ADMIN_UNLOCK:
	case SA_AMF_ADMIN_LOCK:
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}

	(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
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

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
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
		return -1;
	}

	TRACE("'%s'", dn.value);

	if (immutil_getAttr("saAmfClusterStartupTimeout", attributes,
			    0, &avd_cluster->saAmfClusterStartupTimeout) == SA_AIS_OK) {
		/* If zero, use default value */
		if (avd_cluster->saAmfClusterStartupTimeout == 0)
			avd_cluster->saAmfClusterStartupTimeout = AVSV_CLUSTER_INIT_INTVL;
	} else {
		LOG_ER("Get saAmfClusterStartupTimeout FAILED for '%s'", dn.value);
		goto done;
	}

	if (immutil_getAttr("saAmfClusterAdminState", attributes, 0, &avd_cluster->saAmfClusterAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		avd_cluster->saAmfClusterAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	if (!avd_admin_state_is_valid(avd_cluster->saAmfClusterAdminState)) {
		LOG_ER("Invalid saAmfClusterAdminState %u", avd_cluster->saAmfClusterAdminState);
		return -1;
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

