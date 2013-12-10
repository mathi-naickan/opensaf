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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This file contains the AMF API implementation.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"

/****************************************************************************
  Name	  : saAmfInitialize
 
  Description   : This function initializes the AMF for the invoking process
		  and registers the various callback functions.
 
  Arguments     : o_hdl    - ptr to the AMF handle
		  reg_cbks - ptr to a SaAmfCallbacksT structure
		  io_ver   - Version of the AMF implementation being used 
			     by the invoking process.
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfInitialize(SaAmfHandleT *o_hdl, const SaAmfCallbacksT *reg_cbks, SaVersionT *io_ver)
{
	AVA_CB *cb = 0;
	AVA_HDL_DB *hdl_db = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	if (!o_hdl || !io_ver) {
		TRACE_LEAVE2("NULL arguments being passed: SaAmfHandleT and SaVersionT arguments should be non NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* validate the version */
	if (!m_AVA_VER_IS_VALID(io_ver)) {
		TRACE_2("Invalid AMF version specified, supported version is: ReleaseCode = 'B', \
						majorVersion = 0x01, minorVersion = 0x01");
		rc = SA_AIS_ERR_VERSION;
	}

	/* fill the supported version no */
	io_ver->releaseCode = 'B';
	io_ver->majorVersion = 1;
	io_ver->minorVersion = 1;

	if (SA_AIS_OK != rc)
		goto done;

	/* Initialize the environment */
	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("Agents startup failed");
		return SA_AIS_ERR_LIBRARY;
	}

	/* Create AVA/CLA  CB */
	if (ncs_ava_startup() != NCSCC_RC_SUCCESS) {
		ncs_agents_shutdown();
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb write lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);


	/* set version used by client/application */
	cb->version.releaseCode = io_ver->releaseCode;
	cb->version.majorVersion = io_ver->majorVersion;
	cb->version.minorVersion = io_ver->minorVersion;

	/* get the ptr to the hdl db */
	hdl_db = &cb->hdl_db;

	/* create the hdl record & store the callbacks */
	if (!(hdl_rec = ava_hdl_rec_add(cb, hdl_db, reg_cbks))) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Initialize the ipc mailbox for the client for processing pending callbacks */
	if (NCSCC_RC_SUCCESS != ava_callback_ipc_init(hdl_rec)) {
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* pass the handle value to the appl */
	if (SA_AIS_OK == rc) {
		TRACE_1("saAmfHandle returned to application is: %llx", *o_hdl);
		*o_hdl = hdl_rec->hdl;
	}

 done:
	/* free the hdl rec if there's some error */
	if (hdl_rec && SA_AIS_OK != rc)
		ava_hdl_rec_del(cb, hdl_db, &hdl_rec);

	/* release cb read lock and return handle */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}

	if (SA_AIS_OK != rc) {
		ncs_ava_shutdown();
		ncs_agents_shutdown();
	}

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfSelectionObjectGet
 
  Description   : This function creates & returns the operating system handle 
		  associated with the AMF Handle.
 
  Arguments     : hdl       - AMF handle      - AMF handle
		  o_sel_obj - ptr to the selection object
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfSelectionObjectGet(SaAmfHandleT hdl, SaSelectionObjectT *o_sel_obj)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!o_sel_obj) {
		TRACE_LEAVE2("NULL argument passed for SaSelectionObject");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* everything's fine.. pass the sel obj to the appl */
	*o_sel_obj = (SaSelectionObjectT)
            m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&hdl_rec->callbk_mbx));

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfDispatch
 
  Description   : This function invokes, in the context of the calling thread,
		  the next pending callback for the AMF handle.
 
  Arguments     : hdl   - AMF handle
		  flags - flags that specify the callback execution behavior
		  of the saAmfDispatch() function,
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfDispatch(SaAmfHandleT hdl, SaDispatchFlagsT flags)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t pend_fin = 0;
	uint32_t pend_dis = 0;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!m_AVA_DISPATCH_FLAG_IS_VALID(flags)) {
		TRACE_LEAVE2("Invalid SaDispatchFlagsT passed");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Increment Dispatch usgae count */
	cb->pend_dis++;

	rc = ava_hdl_cbk_dispatch(&cb, &hdl_rec, flags);

	/* Decrement Dispatch usage count */
	cb->pend_dis--;

	/* can't use cb after we do agent shutdown, so copy all counts */
	pend_dis = cb->pend_dis;
	pend_fin = cb->pend_fin;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* see if we are still in any dispact context */
	if (pend_dis == 0)
		while (pend_fin != 0) {
			/* call agent shutdown,for each finalize done before */
			ncs_ava_shutdown();
			ncs_agents_shutdown();
			pend_fin--;
		}

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfFinalize
 
  Description   : This function closes the association, represented by the 
		  AMF handle, between the invoking process and the AMF.
 
  Arguments     : hdl - AMF handle
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfFinalize(SaAmfHandleT hdl)
{
	AVA_CB *cb = 0;
	AVA_HDL_DB *hdl_db = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	SaAisErrorT rc = SA_AIS_OK;
	bool agent_flag = false;	/* flag = false, we should not call agent shutdown */
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

	/* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* get the ptr to the hdl db */
	hdl_db = &cb->hdl_db;

	/* get the hdl rec */
	m_AVA_HDL_REC_FIND(hdl_db, hdl, hdl_rec);
	if (!hdl_rec) {
		TRACE("SaAmfHandle passed in argument is not found in AVA DB");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the finalize message */
	m_AVA_AMF_FINALIZE_MSG_FILL(msg, cb->ava_dest, hdl, cb->comp_name);
	rc = ava_mds_send(cb, &msg, 0);
	if (NCSCC_RC_SUCCESS == rc) {
		ncshm_give_hdl(hdl);
		ava_hdl_rec_del(cb, hdl_db, &hdl_rec);
	}
	else {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* Fialize the environment */
	if (cb->pend_dis == 0)
		agent_flag = true;
	else if (cb->pend_dis > 0)
		cb->pend_fin++;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request message */
	avsv_nda_ava_msg_content_free(&msg);

	/* we are not in any dispatch context, we can do agent shutdown */
	if (agent_flag == true) {
		ncs_ava_shutdown();
		ncs_agents_shutdown();
	}

	TRACE_LEAVE2("rc:%u", rc);

	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentRegister
 
  Description   : This function registers the component with the AMF.
 
  Arguments     : hdl	     - AMF handle
		  comp_name       - ptr to the comp name
		  proxy_comp_name - ptr to the proxy comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfComponentRegister(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *proxy_comp_name)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaNameT pcomp_name = {0};
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) ||
	    (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    (proxy_comp_name && (!proxy_comp_name->length || proxy_comp_name->length > SA_MAX_NAME_LENGTH))) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl)) || !m_AVA_FLAG_IS_COMP_NAME(cb)) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (!m_AVA_FLAG_IS_COMP_NAME(cb)) {
		if (getenv("SA_AMF_COMPONENT_NAME")) {
			if (strlen(getenv("SA_AMF_COMPONENT_NAME")) < SA_MAX_NAME_LENGTH) {
				strcpy((char *)cb->comp_name.value, getenv("SA_AMF_COMPONENT_NAME"));
				cb->comp_name.length = (uint16_t)strlen((char *)cb->comp_name.value);
				m_AVA_FLAG_SET(cb, AVA_FLAG_COMP_NAME);
			} else {
				TRACE_2("Length of SA_AMF_COMPONENT_NAME exceeds SA_MAX_NAME_LENGTH bytes");
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
		} else {
			TRACE_2("The SA_AMF_COMPONENT_NAME environment variable is NULL");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
	}
	if (cb)
		ncshm_give_hdl(gl_ava_hdl);

	/* non-proxied, component Length part of component name should be OK */
	if (!proxy_comp_name && (comp_name->length != cb->comp_name.length)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* proxy component, Length part of proxy component name should be Ok */
	if (proxy_comp_name && (proxy_comp_name->length != cb->comp_name.length)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* non-proxied component should not forge its name while registering */
	if (!proxy_comp_name && strncmp((char *)comp_name->value, (char *)cb->comp_name.value, comp_name->length)) {
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/* proxy component should not forge its name while registering its proxied */
	if (proxy_comp_name &&
	    strncmp((char *)proxy_comp_name->value, (char *)cb->comp_name.value, proxy_comp_name->length)) {
		TRACE("proxy component should not forge its name while registering its proxied");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/* check if the required callbacks were supplied during saAmfInitialize */
	if (!m_AVA_HDL_ARE_REG_CBKS_PRESENT(hdl_rec, proxy_comp_name)) {
		TRACE("Required callbacks were not specified in saAmfInitialize");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* populate & send the register message */
	if (proxy_comp_name)
		pcomp_name = *proxy_comp_name;
	m_AVA_COMP_REG_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, pcomp_name);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_COMP_REG == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

	/* TODO: msg_resp should include info regarding comp category.
	 * Then check supplied callbacks (different req depending on comp cat, check spec)
	 * during init and send SA_AIS_ERR_UNAIVALABLE if not the correct callbacks are supplied.
	 */

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentUnregister
 
  Description   : This function unregisters the component with the AMF.
 
  Arguments     : hdl	     - AMF handle
		  comp_name       - ptr to the comp name
		  proxy_comp_name - ptr to the proxy comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfComponentUnregister(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *proxy_comp_name)
{
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaNameT pcomp_name = {0};
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) ||
	    (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    (proxy_comp_name && (!proxy_comp_name->length || proxy_comp_name->length > SA_MAX_NAME_LENGTH))) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl)) || !m_AVA_FLAG_IS_COMP_NAME(cb)) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* non-proxied, component Length part of component name should be OK */
	if (!proxy_comp_name && (comp_name->length != cb->comp_name.length)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* proxy component, Length part of proxy component name should be Ok */
	if (proxy_comp_name && (proxy_comp_name->length != cb->comp_name.length)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* non-proxied component should not forge its name while unregistering */
	if (!proxy_comp_name && strncmp((char *)comp_name->value, (char *)cb->comp_name.value, comp_name->length)) {
		TRACE("non-proxied component should not forge its name while unregistering");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/* proxy component should not forge its name while unregistering proxied */
	if (proxy_comp_name &&
	    strncmp((char *)proxy_comp_name->value, (char *)cb->comp_name.value, proxy_comp_name->length)) {
		TRACE("proxy component should not forge its name while unregistering proxied");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/* populate & send the unregister message */
	if (proxy_comp_name)
		pcomp_name = *proxy_comp_name;
	m_AVA_COMP_UNREG_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, pcomp_name);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_COMP_UNREG == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfHealthcheckStart
 
  Description   : This function allows a process (as part of a comp) to start
		  a specific configured healthcheck.
 
  Arguments     : hdl	 - AMF handle
		  comp_name   - ptr to the comp name
		  hc_key      - ptr to the healthcheck type
		  inv	 - indicates whether the healthcheck is
				AMF initiated / comp initiated
		  rec_rcvr    - recommended recovery

 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckStart(SaAmfHandleT hdl,
				  const SaNameT *comp_name,
				  const SaAmfHealthcheckKeyT *hc_key,
				  SaAmfHealthcheckInvocationT inv, SaAmfRecommendedRecoveryT rec_rcvr)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* verify input inv  */
	if((SA_AMF_HEALTHCHECK_AMF_INVOKED != inv) && (SA_AMF_HEALTHCHECK_COMPONENT_INVOKED != inv)) {
		TRACE_LEAVE2("Incorrect argument specified for SaAmfHealthcheckInvocationT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* input validation of Recomended recovery */
	if(ava_B4_ver_used(cb)) {
		if (rec_rcvr < SA_AMF_NO_RECOMMENDATION || rec_rcvr > SA_AMF_CONTAINER_RESTART) {
			TRACE_LEAVE2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
			rc =  SA_AIS_ERR_ACCESS;
			goto done;
		}
	}
	else {
		if (rec_rcvr < SA_AMF_NO_RECOMMENDATION || rec_rcvr > SA_AMF_CLUSTER_RESET) {
			TRACE_LEAVE2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
			rc =  SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	/* check if hc cbk was supplied during saAmfInitialize (for AMF invoked healthcheck) */
	if ((SA_AMF_HEALTHCHECK_AMF_INVOKED == inv) && (!m_AVA_HDL_IS_HC_CBK_PRESENT(hdl_rec))) {
		rc = SA_AIS_ERR_INIT;
		TRACE("Required callbacks are not registered during saAmfInitialize");
		goto done;
	}

	/* populate & send the healthcheck start message */
	m_AVA_HC_START_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key, inv, rec_rcvr);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_HC_START == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfHealthcheckStop
 
  Description   : This function allows a process to stop an already started
		  healthcheck.
 
  Arguments     : hdl       - AMF handle
		  comp_name - ptr to the comp name
		  hc_key    - ptr to the healthcheck type
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckStop(SaAmfHandleT hdl, const SaNameT *comp_name, const SaAmfHealthcheckKeyT *hc_key)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the healthcheck stop message */
	m_AVA_HC_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_HC_STOP == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfHealthcheckConfirm
 
  Description   : This function allows a process (as part of a component) to 
		  inform the AMF that it has performed the healthcheck.
 
  Arguments     : hdl       - AMF handle
		  comp_name - ptr to the comp name
		  hc_key    - ptr to the healthcheck type
		  hc_result - healthcheck result
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckConfirm(SaAmfHandleT hdl,
				    const SaNameT *comp_name, const SaAmfHealthcheckKeyT *hc_key, SaAisErrorT hc_result)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX) ||
	    !m_AVA_AMF_RESP_ERR_CODE_IS_VALID(hc_result)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the healthcheck confirm message */
	m_AVA_HC_CONFIRM_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key, hc_result);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_HC_CONFIRM == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfPmStart
 
  Description   : This function allows a process (as part of a comp) to start
		  Passive Monitoring.
 
  Arguments     : hdl	    - AMF handle
		  comp_name      - ptr to the comp name
		  processId      - Identifier of a process to be monitored 
		  desc_TreeDepth - Depth of the tree of descendents of the process
		  pmErr	  - Specifies the type of process errors to monitor
		  rec_Recovery   - recommended recovery

 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfPmStart(SaAmfHandleT hdl,
			 const SaNameT *comp_name,
			 SaUint64T processId,
			 SaInt32T desc_TreeDepth, SaAmfPmErrorsT pmErr, SaAmfRecommendedRecoveryT rec_Recovery)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* input validation of component name */
	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of descendant tree depth */
	if (desc_TreeDepth < AVA_PM_START_ALL_DESCENDENTS) {
		TRACE_LEAVE2("Incorrect Descendants tree depth");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of pmError */
	if (pmErr != SA_AMF_PM_NON_ZERO_EXIT && pmErr != SA_AMF_PM_ZERO_EXIT &&
	    pmErr != (SA_AMF_PM_NON_ZERO_EXIT | SA_AMF_PM_ZERO_EXIT)) {
		TRACE_LEAVE2("Incorrect argument specified for SaAmfPmErrorsT ");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of Recomended recovery */
	if (rec_Recovery < SA_AMF_NO_RECOMMENDATION || rec_Recovery > SA_AMF_CONTAINER_RESTART) {
		TRACE_LEAVE2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of Process ID */
	if (processId == 0) {
		TRACE_LEAVE2("Incorrect argument specified for PID");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the passive monitor start message */
	m_AVA_PM_START_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, processId, desc_TreeDepth, pmErr, rec_Recovery);

	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_PM_START == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfPmStop
 
  Description   : This function allows a process to stop an already started
		  passive monitor.
 
  Arguments     : hdl	- AMF handle
		  comp_name  - ptr to the comp name
		  stopQual   - Qualifies which processes should stop being monitored 
		  processId  - Identifier of a process to be monitored
		  pmErr      - type of process errors that the Availability 
				  Management Framework should stop monitoring

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfPmStop(SaAmfHandleT hdl,
			const SaNameT *comp_name,
			SaAmfPmStopQualifierT stopQual, SaInt64T processId, SaAmfPmErrorsT pmErr)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* input validation of component name */
	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect Arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of StopQualifier */
	if (stopQual != SA_AMF_PM_PROC && stopQual != SA_AMF_PM_PROC_AND_DESCENDENTS
	    && stopQual != SA_AMF_PM_ALL_PROCESSES) {
		TRACE_LEAVE2("Incorrect argument specified for SaAmfPmStopQualifierT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of pmError */
	if (pmErr != SA_AMF_PM_NON_ZERO_EXIT && pmErr != SA_AMF_PM_ZERO_EXIT &&
	    pmErr != (SA_AMF_PM_NON_ZERO_EXIT | SA_AMF_PM_ZERO_EXIT)) {
		TRACE_LEAVE2("Incorrect argument specified for SaAmfPmErrorsT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* input validation of Process ID */
	if (processId == 0) {
		TRACE_LEAVE2("Incorrect argument specified for processId");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the Passive Monitoring stop message */
	m_AVA_PM_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, stopQual, processId, pmErr);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_PM_STOP == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentNameGet
 
  Description   : This function returns the name of the component to which the
		  process belongs.
 
  Arguments     : hdl	 - AMF handle
		  o_comp_name - ptr to the comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfComponentNameGet(SaAmfHandleT hdl, SaNameT *o_comp_name)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!o_comp_name) {
		TRACE_LEAVE2("Out param component name is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	memset(o_comp_name, '\0', sizeof(SaNameT));

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* fetch the comp name */
	if (m_AVA_FLAG_IS_COMP_NAME(cb)) {
		*o_comp_name = cb->comp_name;
		o_comp_name->length = cb->comp_name.length;
	} else {
		TRACE_2("component name does not exist");
		rc = SA_AIS_ERR_NOT_EXIST;
	}

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfCSIQuiescingComplete
 
  Description   : This functions allows a component to inform the AMF that it
		  has successfully completed its ongoing work and is now idle.
 
  Arguments     : hdl   - AMF handle
		  inv   - invocation value (used to match the corresponding 
			  callback)
		  error - status of the operation
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfCSIQuiescingComplete(SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVA_PEND_RESP *list_resp = 0;
	AVA_PEND_CBK_REC *rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if ((!m_AVA_AMF_RESP_ERR_CODE_IS_VALID(error)) || (!inv)) {
		TRACE_LEAVE2("Incorrect value specified for SaAisErrorT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl)) || !m_AVA_FLAG_IS_COMP_NAME(cb)) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* get the list of pending responses for this handle */
	list_resp = &hdl_rec->pend_resp;
	if (!list_resp) {
		TRACE_2("No pendingresponses for this handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* get the pending response rec from list */
	m_AVA_HDL_PEND_RESP_GET(list_resp, rec, inv);
	if (!rec) {
		TRACE_2("Pending Response record not found");
		if(ava_B4_ver_used(cb))
			rc = SA_AIS_ERR_NOT_EXIST;
		else
			rc = SA_AIS_ERR_INVALID_PARAM;

		goto done;
	}

	/* This API should only be called after a quiescing callback from AMF */
	if (rec->cbk_info->type != AVSV_AMF_CSI_SET || rec->cbk_info->param.csi_set.ha != SA_AMF_HA_QUIESCING) {
		TRACE_2("This API should only be called after a quiescing callback from AMF");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* populate & send the 'Quiescing Complete' message */
	m_AVA_CSI_QUIESCING_COMPL_MSG_FILL(msg, cb->ava_dest, hdl, inv, error, cb->comp_name);
	rc = ava_mds_send(cb, &msg, 0);
	if (NCSCC_RC_SUCCESS != rc)
		rc = SA_AIS_ERR_TRY_AGAIN;

	/* if we are done with this rec, free it */
	if ((rec->cbk_info->type == AVSV_AMF_CSI_SET) && !m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(rec)) {
		m_AVA_HDL_PEND_RESP_POP(list_resp, rec, inv);
		ava_hdl_cbk_rec_del(rec);
	} else {
		m_AVA_HDL_CBK_RESP_DONE_SET(rec);
	}

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the message */
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfHAStateGet
 
  Description   : This function returns the HA state of the CSI assigned to 
		  the component.
 
  Arguments     : hdl       - AMF handle
		  comp_name - ptr to the comp name
		  csi_name  - ptr to the CSI name
		  o_ha      - ptr to the CSI HA state
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfHAStateGet(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *csi_name, SaAmfHAStateT *o_ha)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
	    !csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH) || !o_ha) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the 'ha state get' message */
	m_AVA_HA_STATE_GET_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, *csi_name);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_HA_STATE_GET == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

	/* return the ha state */
	if (SA_AIS_OK == rc)
		*o_ha = msg_rsp->info.api_resp_info.param.ha_get.ha;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfProtectionGroupTrack
 
  Description   : This fuction requests the AMF to start tracking the changes
		  in the PG associated with the specified CSI.
 
  Arguments     : hdl       - AMF handle
		  csi_name  - ptr to the CSI name
		  flags     - flag that determines when the PG callback is 
			      called
		  buf       - ptr to the linear buffer provided by the 
			      application
		  num       - buffer size
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupTrack(SaAmfHandleT hdl,
				      const SaNameT *csi_name,
				      SaUint8T flags, SaAmfProtectionGroupNotificationBufferT *buf)
{
	SaAmfProtectionGroupNotificationBufferT *rsp_buf = 0;
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	bool is_syn = false, create_memory = false;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (!m_AVA_PG_FLAG_IS_VALID(flags)) {
		TRACE_LEAVE2("Incorrect PG tracking flags");
		return SA_AIS_ERR_BAD_FLAGS;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* check if pg cbk was supplied during saAmfInitialize (for change-only & change track) 
	   and
	   check if track flag is TRACK_CURRENT and neither buffer nor callback is provided */
	if ((((flags & SA_TRACK_CHANGES) || (flags & SA_TRACK_CHANGES_ONLY)) &&
	     (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec))) ||
	    ((flags & SA_TRACK_CURRENT) && (!buf) && (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec)))) {
		TRACE_2("PG tracking callback for CHANGES-ONLY and CHANGES was not registered during saAmfInitialize");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* validate the notify buffer */
	if ((flags & SA_TRACK_CURRENT) && buf && buf->notification) {
		if (!buf->numberOfItems) {
			TRACE_2("numberOfItems should not be zero when passing non NULL notification");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
		is_syn = true;
	}

	/* Check whether we have to allocate the memory */
	if ((flags & SA_TRACK_CURRENT) && buf) {
		if (buf->notification == NULL) {
			/* This means that we have to allocate the memory. In this case we will ignore buf->numberOfItems */
			is_syn = true;
			create_memory = true;

		}
	}

	/* populate & send the pg start message */
	m_AVA_PG_START_MSG_FILL(msg, cb->ava_dest, hdl, *csi_name, flags, is_syn);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		/* the resp may contain only the oper result or the curr pg members */
		if (AVSV_AVND_AMF_CBK_MSG == msg_rsp->type) {
			/* => resp msg contains the curr pg mems */
			osafassert(AVSV_AMF_PG_TRACK == msg_rsp->info.cbk_info->type);

			/* get the err code */
			rc = msg_rsp->info.cbk_info->param.pg_track.err;
			if (SA_AIS_OK != rc)
				goto done;

			/* get the notify buffer from the resp msg */
			rsp_buf = &msg_rsp->info.cbk_info->param.pg_track.buf;
			if (create_memory == false) {
				/* now copy the msg-resp buffer contents to appl provided buffer */
				if (rsp_buf->numberOfItems <= buf->numberOfItems) {
					/* user supplied buffer is sufficient.. copy all the members */
					memcpy(buf->notification, rsp_buf->notification,
					       rsp_buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
					buf->numberOfItems = rsp_buf->numberOfItems;

				} else {
					/* user supplied buffer isnt sufficient.. copy whatever is possible */
					memcpy(buf->notification, rsp_buf->notification,
					       buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
					rc = SA_AIS_ERR_NO_SPACE;
					buf->numberOfItems = rsp_buf->numberOfItems;

				}
			} else {	/* if(create_memory == false) */

				/* create_momory is true, so let us create the memory for the Use, User has to free it. */
				buf->numberOfItems = rsp_buf->numberOfItems;
				if (buf->numberOfItems != 0) {
					buf->notification =
					    malloc(buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
					if (buf->notification != NULL) {
						memcpy(buf->notification, rsp_buf->notification,
						       buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));

					} else {
						rc = SA_AIS_ERR_NO_MEMORY;
						buf->numberOfItems = 0;
					}
				} else {	/* if(buf->numberOfItems != 0) */

					/* buf->numberOfItems is zero. Nothing to be done. */

				}

			}	/* else of if(create_memory == false) */
		} else {
			/* => it's a regular resp msg */
			osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
			osafassert(AVSV_AMF_PG_START == msg_rsp->info.api_resp_info.type);
			rc = msg_rsp->info.api_resp_info.rc;
		}
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfProtectionGroupTrackStop
 
  Description   : This fuction requests the AMF to stop tracking the changes
		  in the PG associated with the CSI.
 
  Arguments     : hdl       - AMF handle
		  csi_name  - ptr to the CSI name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupTrackStop(SaAmfHandleT hdl, const SaNameT *csi_name)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the pg stop message */
	m_AVA_PG_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *csi_name);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_PG_STOP == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentErrorReport
 
  Description   : This function reports an error and a recovery recommendation
		  to the AMF.
 
  Arguments     : hdl       - AMF handle
		  err_comp - ptr to the erroneous comp name
		  err_time - error detection time
		  rec_rcvr - recommended recovery
		  ntf_id   - notification identifier sent to SysMan
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT saAmfComponentErrorReport(SaAmfHandleT hdl,
				      const SaNameT *err_comp,
				      SaTimeT err_time, SaAmfRecommendedRecoveryT rec_rcvr, SaNtfIdentifierT ntf_id)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* validate the component */
	if (!err_comp || !(err_comp->length) || (err_comp->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* validate the recommended recovery */
	if((cb->version.releaseCode == 'B') && (cb->version.majorVersion == 0x01)){
		if(rec_rcvr < SA_AMF_NO_RECOMMENDATION || rec_rcvr > SA_AMF_CLUSTER_RESET) {
			TRACE_LEAVE2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);

	/* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the 'error report' message */
	m_AVA_ERR_REP_MSG_FILL(msg, cb->ava_dest, hdl, *err_comp, err_time, rec_rcvr);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_ERR_REP == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentErrorClear
 
  Description   : This function clears the previous errors reported about 
		  the component.
 
  Arguments     : hdl       - AMF handle
		  comp_name - ptr to the comp name
		  ntf_id    - notification identifier sent to SysMan
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT saAmfComponentErrorClear(SaAmfHandleT hdl, const SaNameT *comp_name, SaNtfIdentifierT ntf_id)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if (!comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* populate & send the 'error clear' message */
	m_AVA_ERR_CLEAR_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
		osafassert(AVSV_AMF_ERR_CLEAR == msg_rsp->info.api_resp_info.type);
		rc = msg_rsp->info.api_resp_info.rc;
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfResponse
 
  Description   : The component responds to the AMF with the result of its 
		  execution of a particular AMF request.
 
  Arguments     : hdl   - AMF handle
		  inv   - invocation value (used to match the corresponding 
			  callback)
		  error - status of the operation
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfResponse(SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error)
{
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg = {0};
	AVA_PEND_RESP *list_resp = 0;
	AVA_PEND_CBK_REC *rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	AVSV_NDA_AVA_MSG *msg_rsp = NULL;

	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	if ((!m_AVA_AMF_RESP_ERR_CODE_IS_VALID(error)) || (!inv)) {
		TRACE_LEAVE2("Incorrect argument specified for SaAisErrorT");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* get the list of pending responses for this handle */
	list_resp = &hdl_rec->pend_resp;
	if (!list_resp) {
		TRACE_2("No pending responses for this handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* get the pending response rec from list */
	m_AVA_HDL_PEND_RESP_GET(list_resp, rec, inv);
	if (!rec) {
		TRACE_2("No pending response records found for this handle");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* populate & send the 'AMF response' message */
	m_AVA_AMF_RESP_MSG_FILL(msg, cb->ava_dest, hdl, inv, error, cb->comp_name);

	if (rec->cbk_info->type == AVSV_AMF_COMP_TERM)
		rc = ava_mds_send(cb, &msg, &msg_rsp);
	else
		rc = ava_mds_send(cb, &msg, NULL);

	if (NCSCC_RC_SUCCESS != rc)
		rc = SA_AIS_ERR_TRY_AGAIN;

	if (rec->cbk_info->type == AVSV_AMF_COMP_TERM) {
		if (msg_rsp->type != AVSV_AVND_AMF_API_RESP_MSG) {
			TRACE_2("ERR_LIBRARY: wrong type");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		if (msg_rsp->info.api_resp_info.type != AVSV_AMF_COMP_TERM_RSP) {
			TRACE_2("ERR_LIBRARY: wrong msg type");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		rc = msg_rsp->info.api_resp_info.rc;
	}

	/* if we are done with this rec, free it */
	if ((rec->cbk_info->type != AVSV_AMF_PG_TRACK) && !m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(rec)) {
		/* In case of Quiescing callback dont free the rec,
		   free it after app makes a QuiescingComplete */
		if (rec->cbk_info->type == AVSV_AMF_CSI_SET && rec->cbk_info->param.csi_set.ha == SA_AMF_HA_QUIESCING)
			goto done;
		TRACE("Freeing the callback info recordRecord");
		m_AVA_HDL_PEND_RESP_POP(list_resp, rec, inv);
		ava_hdl_cbk_rec_del(rec);
	} else {
		/* In case of Quiescing callback dont set the response done */
		if (rec->cbk_info->type == AVSV_AMF_CSI_SET && rec->cbk_info->param.csi_set.ha == SA_AMF_HA_QUIESCING)
			goto done;

		TRACE("Callback resonse completed");
		m_AVA_HDL_CBK_RESP_DONE_SET(rec);
	}

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request message */
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}


/******************
 * AMF-B04.01 API *
 ******************/

/****************************************************************************
  Name	  : saAmfInitialize_4

  Description   : This function initializes the AMF for the invoking process
		  and registers the various callback functions.

  Arguments     : o_hdl    - ptr to the AMF handle
		  reg_cbks - ptr to a SaAmfCallbacksT structure
		  io_ver   - Version of the AMF implementation being used
			     by the invoking process.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfInitialize_4(SaAmfHandleT *o_hdl, const SaAmfCallbacksT_4 *reg_cbks, SaVersionT *io_ver)
{
	AVA_CB *cb = 0;
	AVA_HDL_DB *hdl_db = 0;
	AVA_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */

	if (!o_hdl || !io_ver) {
		TRACE_2("NULL arguments being passed: SaAmfHandleT and SaVersionT arguments should be non NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* validate the version */
	if((io_ver->releaseCode != 'B') || (io_ver->majorVersion != 0x04)) {
		TRACE_2("Invalid AMF version specified, supported version is: ReleaseCode = 'B', \
						majorVersion = 0x04, minorVersion = 0x01");
		rc = SA_AIS_ERR_VERSION;
	}

	/* fill the supported version no */
	io_ver->releaseCode = 'B';
	io_ver->majorVersion = 4;
	io_ver->minorVersion = 1;

	if (SA_AIS_OK != rc)
		goto done;

	/* Initialize the environment */
	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		TRACE_2("Agents startup failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* Create AVA/CLA  CB */
	if (ncs_ava_startup() != NCSCC_RC_SUCCESS) {
		ncs_agents_shutdown();
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb write lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

	/* set version used by client/application */
	cb->version.releaseCode = io_ver->releaseCode;
	cb->version.majorVersion = io_ver->majorVersion;
	cb->version.minorVersion = io_ver->minorVersion;

	/* get the ptr to the hdl db */
	hdl_db = &cb->hdl_db;

	/* create the hdl record & store the callbacks */

	/* TODO: This cast will remove possibilities for container comp callbacks(last two in SaAmfCallbacksT_4 struct).
	 * But on the other hand they are not supported in ava_hdl_cbk_rec_prc, message from avnd.SaAmfCallbacksT
	 * should be replaced with SaAmfCallbacksT_4 everywhere in ava when SaAmfCallbacksT_4 messages are supported
	 * from avnd.
	 */
	if ((reg_cbks != NULL) &&
	    ((reg_cbks->saAmfContainedComponentCleanupCallback != 0) ||
	     (reg_cbks->saAmfContainedComponentInstantiateCallback != 0))) {
		TRACE_4("SA_AIS_ERR_NOT_SUPPORTED: unsupported callbacks");
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		goto done;
	}

	/* create the hdl record & store the callbacks */
	if (!(hdl_rec = ava_hdl_rec_add(cb, hdl_db, (SaAmfCallbacksT*)reg_cbks))) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Initialize the ipc mailbox for the client for processing pending callbacks */
	if (NCSCC_RC_SUCCESS != ava_callback_ipc_init(hdl_rec)) {
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* pass the handle value to the appl */
	if (SA_AIS_OK == rc) {
		TRACE_1("saAmfHandle returned to application is: %llx", *o_hdl);
		*o_hdl = hdl_rec->hdl;
	}

 done:
	/* free the hdl rec if there's some error */
	if (hdl_rec && SA_AIS_OK != rc)
		ava_hdl_rec_del(cb, hdl_db, &hdl_rec);

	/* release cb read lock and return handle */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}

	if (SA_AIS_OK != rc) {
		ncs_ava_shutdown();
		ncs_agents_shutdown();
	}

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfPmStart_3

  Description   : This function allows a process (as part of a comp) to start
		  Passive Monitoring.

  Arguments     : hdl	    	  - AMF handle
		   comp_name      - ptr to the comp name
		   processId      - Identifier of a process to be monitored
		   desc_TreeDepth - Depth of the tree of descendents of the process
		   pmErr	  - Specifies the type of process errors to monitor
		   rec_Recovery   - recommended recovery


  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfPmStart_3(SaAmfHandleT hdl,
			 const SaNameT *comp_name,
			 SaInt64T processId,
			 SaInt32T desc_TreeDepth, SaAmfPmErrorsT pmErr, SaAmfRecommendedRecoveryT rec_Recovery)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(0)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* input validation of Recomended recovery */
	if (rec_Recovery < SA_AMF_NO_RECOMMENDATION || rec_Recovery > SA_AMF_CONTAINER_RESTART) {
		TRACE_2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
		rc = SA_AIS_ERR_ACCESS;
		goto done;
	}

	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if comp_name exists in AMF configuration, if not: SA_AIS_ERR_NOT_EXIST */
	/* TODO: check if process id is executing on local node. if not: SA_AIS_ERR_NOT_EXIST */

	/* invalid process ID */
	if( processId <= 0) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	rc = saAmfPmStart(hdl, comp_name, processId, desc_TreeDepth, pmErr, rec_Recovery);

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfHAReadinessStateSet

  Description	: This function sets the HA state of the pre-instantiable
		  component identified by the component and CSI names.

  Arguments	: hdl		- AMF handle
		  comp_name	- ptr to the comp name
		  csi_name	- ptr to the csi_name
		  ha_state	- HA state to set
		  corr_ids	- pointer to Notification correlation identifiers.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/

SaAisErrorT saAmfHAReadinessStateSet(SaAmfHandleT hdl,
					const SaNameT *comp_name,
					const SaNameT *csi_name,
					SaAmfHAReadinessStateT ha_state,
					SaNtfCorrelationIdsT *corr_ids)
{
	TRACE_2("Not implemented yet");

	return SA_AIS_ERR_NOT_SUPPORTED;
}

/****************************************************************************
  Name	  : saAmfProtectionGroupTrack_4

  Description   : This fuction requests the AMF to start tracking the changes
		  in the PG associated with the specified CSI.

  Arguments     : hdl       - AMF handle
		   csi_name  - ptr to the CSI name
		   flags     - flag that determines when the PG callback is
			      called
		   buf       - ptr to the linear buffer provided by the
			      application

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupTrack_4(SaAmfHandleT hdl,
				      const SaNameT *csi_name,
				      SaUint8T flags, SaAmfProtectionGroupNotificationBufferT_4 *buf)

{
	SaAmfProtectionGroupNotificationBufferT *rsp_buf = 0;
	AVA_CB *cb = 0;
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_NDA_AVA_MSG msg;
	AVSV_NDA_AVA_MSG *msg_rsp = 0;
	bool is_syn = false, create_memory = false;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb read lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
    /* retrieve hdl rec */
	if ( !(hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)) ) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(cb)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* TODO: check if csiName exists in AMF configuration. If not, SA_AIS_ERR_NOT_EXIST */
	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */

	/* initialize the msg */
	memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

	if (!csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE2("Incorrect arguments");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (!m_AVA_PG_FLAG_IS_VALID(flags)) {
		TRACE_2("Incorrect PG tracking flags");
		rc = SA_AIS_ERR_BAD_FLAGS;
		goto done;
	}

	/* check if pg cbk was supplied during saAmfInitialize (for change-only & change track)
	   and
	   check if track flag is TRACK_CURRENT and neither buffer nor callback is provided */
	if ((((flags & SA_TRACK_CHANGES) || (flags & SA_TRACK_CHANGES_ONLY)) &&
	     (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec))) ||
	    ((flags & SA_TRACK_CURRENT) && (!buf) && (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec)))) {
		TRACE_2("PG tracking callback for CHANGES-ONLY and CHANGES was not registered during saAmfInitialize");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* validate the notify buffer */
	if ((flags & SA_TRACK_CURRENT) && buf && buf->notification) {
		if (!buf->numberOfItems) {
			TRACE_2("numberOfItems should not be zero when passing non NULL notification");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
		is_syn = true;
	}

	/* Check whether we have to allocate the memory */
	if ((flags & SA_TRACK_CURRENT) && buf) {
		if (buf->notification == NULL) {
			/* This means that we have to allocate the memory. In this case we will ignore buf->numberOfItems */
			is_syn = true;
			create_memory = true;

		}
	}

	/* populate & send the pg start message */
	m_AVA_PG_START_MSG_FILL(msg, cb->ava_dest, hdl, *csi_name, flags, is_syn);
	rc = ava_mds_send(cb, &msg, &msg_rsp);
	if (NCSCC_RC_SUCCESS == rc) {
		/* the resp may contain only the oper result or the curr pg members */
		if (AVSV_AVND_AMF_CBK_MSG == msg_rsp->type) {
			/* => resp msg contains the curr pg mems */
			osafassert(AVSV_AMF_PG_TRACK == msg_rsp->info.cbk_info->type);

			/* get the err code */
			rc = msg_rsp->info.cbk_info->param.pg_track.err;
			if (SA_AIS_OK != rc)
				goto done;

			/* get the notify buffer from the resp msg */
			rsp_buf = &msg_rsp->info.cbk_info->param.pg_track.buf;
			if (create_memory == false) {
				/* now copy the msg-resp buffer contents to appl provided buffer */
				if (rsp_buf->numberOfItems <= buf->numberOfItems) {
					/* user supplied buffer is sufficient.. copy all the members */
					ava_cpy_protection_group_ntf(buf->notification, rsp_buf->notification,
							rsp_buf->numberOfItems, SA_AMF_HARS_READY_FOR_ASSIGNMENT);

					buf->numberOfItems = rsp_buf->numberOfItems;
				} else {
					/* user supplied buffer isnt sufficient.. copy whatever is possible */
					ava_cpy_protection_group_ntf(buf->notification, rsp_buf->notification,
							buf->numberOfItems, SA_AMF_HARS_READY_FOR_ASSIGNMENT);
					rc = SA_AIS_ERR_NO_SPACE;
					buf->numberOfItems = rsp_buf->numberOfItems;
				}
			} else {	/* if(create_memory == false) */

				/* create_momory is true, so let us create the memory for the Use, User has to free it. */
				buf->numberOfItems = rsp_buf->numberOfItems;
				if (buf->numberOfItems != 0) {
					buf->notification =
					    malloc(buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT_4));
					if (buf->notification != NULL) {
						ava_cpy_protection_group_ntf(buf->notification, rsp_buf->notification,
								buf->numberOfItems, SA_AMF_HARS_READY_FOR_ASSIGNMENT);
					} else {
						rc = SA_AIS_ERR_NO_MEMORY;
						buf->numberOfItems = 0;
					}
				} else {	/* if(buf->numberOfItems != 0) */

					/* buf->numberOfItems is zero. Nothing to be done. */

				}

			}	/* else of if(create_memory == false) */
		} else {
			/* => it's a regular resp msg */
			osafassert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
			osafassert(AVSV_AMF_PG_START == msg_rsp->info.api_resp_info.type);
			rc = msg_rsp->info.api_resp_info.rc;
		}
	} else if (NCSCC_RC_FAILURE == rc)
		rc = SA_AIS_ERR_TRY_AGAIN;
	else if (NCSCC_RC_REQ_TIMOUT == rc)
		rc = SA_AIS_ERR_TIMEOUT;

 done:
	/* release cb read lock and return handles */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}
	if (hdl_rec)
		ncshm_give_hdl(hdl);

	/* free the contents of the request/response message */
	if (msg_rsp)
		avsv_nda_ava_msg_free(msg_rsp);
	avsv_nda_ava_msg_content_free(&msg);

	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfProtectionGroupNotificationFree_4

  Description   : This function frees the memory to which notification points
		   and which was allocated by the AMF previously in
		   saAmfProtectionGroupTrack_4() function.

  Arguments     : hdl		- AMF handle
		   notification	- pointer to memory that was previously allocated
				by AMF and is to be released.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupNotificationFree_4(SaAmfHandleT hdl, SaAmfProtectionGroupNotificationT_4 *notification)

{
	AVA_CB *cb = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Verifying the input Handle & global handle */
	if(!gl_ava_hdl || hdl > AVSV_UNS32_HDL_MAX) {
		TRACE_2("Invalid SaAmfHandle passed by component: %llx",hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	/* retrieve AvA CB */
	if (!(cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl))) {
		TRACE_4("SA_AIS_ERR_LIBRARY: Unable to retrieve cb handle");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	/* acquire cb write lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(cb)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */

	/* free memory */
	if(notification)
		free(notification);
	else
		rc = SA_AIS_ERR_INVALID_PARAM;

done:
	/* release cb read lock and return handler */
	if (cb) {
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_ava_hdl);
	}
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfComponentErrorReport_4

  Description   : This function reports an error and a recovery recommendation
		  to the AMF.

  Arguments     : hdl       - AMF handle
		   err_comp - ptr to the erroneous comp name
		   err_time - error detection time
		   rec_rcvr - recommended recovery
		   corr_ids - pointer to Notification correlation identifiers.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT saAmfComponentErrorReport_4(SaAmfHandleT hdl,
				      const SaNameT *err_comp,
				      SaTimeT err_time, SaAmfRecommendedRecoveryT rec_rcvr, SaNtfCorrelationIdsT *corr_ids)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(0)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* input validation of Recomended recovery */
	if (rec_rcvr < SA_AMF_NO_RECOMMENDATION || rec_rcvr > SA_AMF_CONTAINER_RESTART) {
		TRACE_2("Incorrect argument specified for SaAmfRecommendedRecoveryT");
		rc = SA_AIS_ERR_ACCESS;
		goto done;
	}

	/* TODO: check if comp name exists in AMF conf otherwise => SA_AIS_ERR_NOT_EXIST */
	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */

	if(corr_ids != NULL) {
		/* Support for SaNtfCorrelationIdsT is not yet implemented */
		if(!((corr_ids->rootCorrelationId == SA_NTF_IDENTIFIER_UNUSED) && (corr_ids->parentCorrelationId == SA_NTF_IDENTIFIER_UNUSED))) {
			TRACE_2("Value other then SA_NTF_IDENTIFIER_UNUSED for SaNtfIdentifierT and SaNtfIdentifierT is not yet supported");
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
		}
	}
	else {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	rc = saAmfComponentErrorReport(hdl, err_comp, err_time, rec_rcvr, corr_ids->notificationId);

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfCorrelationIdsGet

  Description	: This function is used in order to generate a notification.

  Arguments	: hdl		- AMF handle
		  inv		- invocation value (used to match the corresponding
		  corr_ids	- pointer to Notification correlation identifiers.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/

SaAisErrorT saAmfCorrelationIdsGet(SaAmfHandleT hdl, SaInvocationT inv, SaNtfCorrelationIdsT *corr_ids)
{
	TRACE_2("Not implemented yet");

	return SA_AIS_ERR_NOT_SUPPORTED;
}

/****************************************************************************
  Name	  : saAmfComponentErrorClear_4

  Description   : This function clears the previous errors reported about
		  the component.

  Arguments     : hdl       - AMF handle
		   comp_name - ptr to the comp name
		   corr_ids - pointer to Notification correlation identifiers.

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT saAmfComponentErrorClear_4(SaAmfHandleT hdl, const SaNameT *comp_name, SaNtfCorrelationIdsT *corr_ids)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(0)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* TODO: check if comp name exists in AMF conf otherwise => SA_AIS_ERR_NOT_EXIST */
	/* TODO: check if op state enabled, no effect => SA_AIS_ERR_NO_OP */
	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */

	if(corr_ids != NULL) {
		/* Support for SaNtfCorrelationIdsT is not yet implemented */
		if(!((corr_ids->rootCorrelationId == SA_NTF_IDENTIFIER_UNUSED) && (corr_ids->parentCorrelationId == SA_NTF_IDENTIFIER_UNUSED))) {
			TRACE_2("Value other then SA_NTF_IDENTIFIER_UNUSED for SaNtfIdentifierT and SaNtfIdentifierT is not yet supported");
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
		}
	}
	else {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	rc = saAmfComponentErrorClear(hdl, comp_name, corr_ids->notificationId);

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/****************************************************************************
  Name	  : saAmfResponse_4

  Description   : The component responds to the AMF with the result of its
		  execution of a particular AMF request.

  Arguments     : hdl   - AMF handle
		   inv   - invocation value (used to match the corresponding
			  callback)
		   corr_ids - pointer to Notification correlation identifiers.
		   error - status of the operation

  Return Values : Refer to SAI-AIS specification for various return values.

  Notes	 : None.
******************************************************************************/
SaAisErrorT saAmfResponse_4(SaAmfHandleT hdl, SaInvocationT inv, SaNtfCorrelationIdsT *corr_ids, SaAisErrorT error)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("SaAmfHandleT passed is %llx", hdl);

	/* Version is previously set in in initialize function */
	if(!ava_B4_ver_used(0)) {
		TRACE_2("Invalid AMF version, set correct AMF version using saAmfInitialize_4. "
				"Required version is: ReleaseCode = 'B', majorVersion = 0x04");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* TODO: check cluster membership, if node is not a member answer back with SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if handle is "old", due to node rejoin as member in cluster. If not: SA_AIS_ERR_UNAVAILABLE */
	/* TODO: check if invocation belong to an existing callback with outstanding response => SA_AIS_ERR_NOT_EXIST */

	/* corr_ids should always be NULL if response is not reporting an error. */
	if( (error == SA_AIS_OK) && (corr_ids != NULL)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}
	else if(corr_ids != NULL) {
		/* Support for SaNtfCorrelationIdsT is not yet implemented */
		if(!((corr_ids->rootCorrelationId == SA_NTF_IDENTIFIER_UNUSED) && (corr_ids->parentCorrelationId == SA_NTF_IDENTIFIER_UNUSED))) {
			TRACE_2("Value other then SA_NTF_IDENTIFIER_UNUSED for SaNtfIdentifierT and SaNtfIdentifierT is not yet supported");
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
		}
		if (error == SA_AIS_ERR_NOT_READY) {
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
		}
	}

	rc = saAmfResponse(hdl, inv, error);

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}
