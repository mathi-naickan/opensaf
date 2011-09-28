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
 *
 */

#include <logtrace.h>
#include <nid_start_util.h>

#include "rde_cb.h"

static RDE_AMF_CB *rde_amf_get_cb(void)
{
	RDE_CONTROL_BLOCK *rde_cb = rde_get_control_block();
	return &rde_cb->rde_amf_cb;
}

static void rde_saf_CSI_set_callback(SaInvocationT invocation,
	const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	RDE_AMF_CB *rde_amf_cb = rde_amf_get_cb();

	TRACE_ENTER();

	(void) saAmfResponse(rde_amf_cb->amf_hdl, invocation, SA_AIS_OK);
}

static void rde_saf_health_chk_callback(SaInvocationT invocation,
	const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	RDE_AMF_CB *rde_amf_cb = rde_amf_get_cb();

	TRACE_ENTER();

	(void) saAmfResponse(rde_amf_cb->amf_hdl, invocation, SA_AIS_OK);
}

void rde_saf_CSI_rem_callback(SaInvocationT invocation,
			      const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	RDE_AMF_CB *rde_amf_cb = rde_amf_get_cb();

	TRACE_ENTER();

	(void) saAmfResponse(rde_amf_cb->amf_hdl, invocation, SA_AIS_OK);
}

void rde_saf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	RDE_AMF_CB *rde_amf_cb = rde_amf_get_cb();

	TRACE_ENTER();

	(void) saAmfResponse(rde_amf_cb->amf_hdl, invocation, SA_AIS_OK);

	TRACE_LEAVE();
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : rde_amf_healthcheck_start
 *
 * Description   : RDE informs AMF to stat healthcheck 
 *
 * Arguments     : rde_amf_cb  - RDE control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 rde_amf_healthcheck_start(RDE_AMF_CB *rde_amf_cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_error;
	SaAmfHealthcheckKeyT Healthy;
	SaNameT SaCompName;
	char *phlth_ptr;
	char hlth_str[256];

	TRACE_ENTER();

	/*
	 ** Start the AMF health check 
	 */
	memset(&SaCompName, 0, sizeof(SaCompName));
	strcpy((char*)SaCompName.value, rde_amf_cb->comp_name);
	SaCompName.length = strlen(rde_amf_cb->comp_name);

	memset(&Healthy, 0, sizeof(Healthy));
	phlth_ptr = getenv("RDE_HA_ENV_HEALTHCHECK_KEY");
	if (phlth_ptr == NULL) {
		/*
		 ** default health check key 
		 */
		strcpy(hlth_str, "BAD10");
	} else {
		strcpy(hlth_str, phlth_ptr);
	}
	strcpy((char*)Healthy.key, hlth_str);
	Healthy.keyLen = strlen((char*)Healthy.key);

	amf_error = saAmfHealthcheckStart(rde_amf_cb->amf_hdl, &SaCompName, &Healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfHealthcheckStart FAILED %u", amf_error);
		rc = NCSCC_RC_FAILURE;
	}

	return rc;
}

uns32 rde_amf_init(RDE_AMF_CB *rde_amf_cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_error = SA_AIS_OK;
	SaNameT sname;
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;

	TRACE_ENTER();

	if (amf_comp_name_get_set_from_file("RDE_COMP_NAME_FILE", &sname) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	amfCallbacks.saAmfHealthcheckCallback = rde_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = rde_saf_CSI_set_callback;
	amfCallbacks.saAmfCSIRemoveCallback = rde_saf_CSI_rem_callback;
	amfCallbacks.saAmfComponentTerminateCallback = rde_saf_comp_terminate_callback;

	m_RDE_GET_AMF_VER(amf_version);

	amf_error = saAmfInitialize(&rde_amf_cb->amf_hdl, &amfCallbacks, &amf_version);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize FAILED %u", amf_error);
		return NCSCC_RC_FAILURE;
	}

	memset(&sname, 0, sizeof(sname));
	amf_error = saAmfComponentNameGet(rde_amf_cb->amf_hdl, &sname);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet FAILED %u", amf_error);
		return NCSCC_RC_FAILURE;
	}

	strcpy((char*)rde_amf_cb->comp_name, (char*)sname.value);

	amf_error = saAmfSelectionObjectGet(rde_amf_cb->amf_hdl, &rde_amf_cb->amf_fd);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet FAILED %u", amf_error);
		return NCSCC_RC_FAILURE;
	}

	amf_error = saAmfComponentRegister(rde_amf_cb->amf_hdl, &sname, (SaNameT *)NULL);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister FAILED %u", amf_error);
		return NCSCC_RC_FAILURE;
	}

	rc = rde_amf_healthcheck_start(rde_amf_cb);
	if (rc != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	TRACE_LEAVE2("AMF Initialization SUCCESS......");
	return(rc);
}

