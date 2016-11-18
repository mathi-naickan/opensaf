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

  This file contains routines for miscellaneous operations.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"

/****************************************************************************
  Name          : ava_avnd_msg_prc
 
  Description   : This routine process the messages from AvND.
 
  Arguments     : cb  - ptr to the AvA control block
                  msg - ptr to the AvND message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_avnd_msg_prc(AVA_CB *cb, AVSV_NDA_AVA_MSG *msg)
{
	AVA_HDL_REC *hdl_rec = 0;
	AVSV_AMF_CBK_INFO *cbk_info = 0;
	uint32_t hdl = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* 
	 * AvA receives either AVSV_AVND_AMF_CBK_MSG or AVSV_AVND_AMF_API_RESP_MSG 
	 * from AvND. Response to APIs is handled by synchronous blocking calls. 
	 * Hence, in this flow, the message type can only be AVSV_AVND_AMF_CBK_MSG.
	 */
	osafassert(msg->type == AVSV_AVND_AMF_CBK_MSG);

	/* get the callbk info */
	cbk_info = msg->info.cbk_info;

	/* convert csi attributes into the form that amf-cbk understands */
	if ((AVSV_AMF_CSI_SET == cbk_info->type) && (SA_AMF_CSI_ADD_ONE == cbk_info->param.csi_set.csi_desc.csiFlags)) {
		rc = avsv_amf_csi_attr_convert(cbk_info);
		if (NCSCC_RC_SUCCESS != rc) {
			TRACE_2("csi attributes convertion failed");
			goto done;
		}
	}
	/* For CSI attribute change callback, convert AVSV_CSI_ATTRS to SaAmfCSIAttributeListT. */
	if (cbk_info->type == AVSV_AMF_CSI_ATTR_CHANGE) {
		rc = avsv_attrs_to_amf_attrs(&cbk_info->param.csi_attr_change.csiAttr,
				&cbk_info->param.csi_attr_change.attrs);
		if (NCSCC_RC_SUCCESS != rc) {
			TRACE_2("csi attributes convertion failed in CSI_ATTR_CHANGE_CBK failed");
			goto done;
		}
	}

	/* retrieve the handle record */
	hdl = cbk_info->hdl;
	hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl);
	if (hdl_rec) {
		/* push the callbk parameters in the pending callbk list */
		rc = ava_hdl_cbk_param_add(cb, hdl_rec, cbk_info);
		if (NCSCC_RC_SUCCESS == rc) {
			/* => the callbk info ptr is used in the pend callbk list */
			msg->info.cbk_info = 0;
		} else
			TRACE_2("Handle callback param add failed");

		/* return the handle record */
		ncshm_give_hdl(hdl);
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_B4_ver_used

  Description   : This routine checks if AMF B04 version is used by
                  client/application.

  Arguments     : in_cb   - ptr to the AvA control block. If zero,
                            global handle will be taken & released.

  Return Values : true if B4 version is used

  Notes         : None
******************************************************************************/
bool ava_B4_ver_used(AVA_CB *in_cb)
{
	AVA_CB *cb = 0;
	bool rc = false;

	if(in_cb) {
		if((in_cb->version.releaseCode == 'B') && (in_cb->version.majorVersion == 0x04))
			rc = true;
	}
	else {

		cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl);

		if(cb) {
			if((cb->version.releaseCode == 'B') && (cb->version.majorVersion == 0x04))
				rc = true;

			ncshm_give_hdl(gl_ava_hdl);
		}
	}

	return rc;
}

/*
 * Temporary function in order to copy data from SaAmfProtectionGroupNotificationT to
 * SaAmfProtectionGroupNotificationT_4. The difference is the additional
 * haReadinessState element in SaAmfProtectionGroupMemberT_4.
 *
 * When haReadinessState is fully supported in the interface between avnd and ava, this function
 * should be removed and standard memcpy could be used instead.
 */
void ava_cpy_protection_group_ntf(SaAmfProtectionGroupNotificationT_4  *to_ntf,
				const SaAmfProtectionGroupNotificationT *from_ntf,
				SaUint32T items,
				SaAmfHAReadinessStateT ha_read_state)
{
	unsigned int i;

	memset(to_ntf, 0, items * sizeof(*to_ntf));
	for(i = 0; i < items; i++) {
		to_ntf[i].change = from_ntf[i].change;
		osaf_extended_name_alloc(osaf_extended_name_borrow(&from_ntf[i].member.compName),
								 &to_ntf[i].member.compName);
		to_ntf[i].member.haReadinessState = ha_read_state;
		to_ntf[i].member.haState = from_ntf[i].member.haState;
		to_ntf[i].member.rank = from_ntf[i].member.rank;
	}
}

/**
 *  @Brief: Check SaNameT is a valid formation
 *
 */
bool ava_sanamet_is_valid(const SaNameT* pName)
{
	if (!osaf_is_extended_name_valid(pName)) {
		LOG_WA("Environment variable SA_ENABLE_EXTENDED_NAMES "
			"is not set, or not using extended name api");
		return false;
	}
	if (osaf_extended_name_length(pName) > kOsafMaxDnLength) {
		LOG_ER("Exceeding maximum of extended name length(%u)"
			,kOsafMaxDnLength);
		return false;
	}
	return true;
}

/**
 * @brief  Copies callbacks passed by application in saAmfInitialize() API into
	   AMF internal callback strucutre OsafAmfCallbacksT.
 *
 * @param  osaf_cbk  (ptr to OsafAmfCallbacksT).
 * @param  cbk (ptr to SaAmfCallbacksT).
 *
 */
void amf_copy_from_SaAmfCallbacksT_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk,
       const SaAmfCallbacksT *cbk) {
  osaf_cbk->saAmfHealthcheckCallback = cbk->saAmfHealthcheckCallback;
  osaf_cbk->saAmfComponentTerminateCallback = cbk->saAmfComponentTerminateCallback;
  osaf_cbk->saAmfCSISetCallback = cbk->saAmfCSISetCallback;
  osaf_cbk->saAmfCSIRemoveCallback = cbk->saAmfCSIRemoveCallback;
  osaf_cbk->saAmfProtectionGroupTrackCallback = cbk->saAmfProtectionGroupTrackCallback;
  osaf_cbk->saAmfProxiedComponentInstantiateCallback = cbk->saAmfProxiedComponentInstantiateCallback;
  osaf_cbk->saAmfProxiedComponentCleanupCallback = cbk->saAmfProxiedComponentCleanupCallback; 
}

/*
 * @brief  Copies callbacks passed by application in saAmfInitialize_4() API into
           AMF internal callback strucutre OsafAmfCallbacksT.
 *
 * @param  osaf_cbk  (ptr to OsafAmfCallbacksT).
 * @param  cbk (ptr to SaAmfCallbacksT_4).
 */
void amf_copy_from_SaAmfCallbacksT_4_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk, const SaAmfCallbacksT_4 *cbk) {
  osaf_cbk->saAmfHealthcheckCallback = cbk->saAmfHealthcheckCallback;
  osaf_cbk->saAmfComponentTerminateCallback = cbk->saAmfComponentTerminateCallback;
  osaf_cbk->saAmfCSISetCallback = cbk->saAmfCSISetCallback;
  osaf_cbk->saAmfCSIRemoveCallback = cbk->saAmfCSIRemoveCallback;
  osaf_cbk->saAmfProtectionGroupTrackCallback_4 = cbk->saAmfProtectionGroupTrackCallback;
  osaf_cbk->saAmfProxiedComponentInstantiateCallback = cbk->saAmfProxiedComponentInstantiateCallback;
  osaf_cbk->saAmfProxiedComponentCleanupCallback = cbk->saAmfProxiedComponentCleanupCallback; 
  osaf_cbk->saAmfContainedComponentInstantiateCallback = cbk->saAmfContainedComponentInstantiateCallback; 
  osaf_cbk->saAmfContainedComponentInstantiateCallback = cbk->saAmfContainedComponentInstantiateCallback; 
}

/*
 * @brief  Copies callbacks passed by application in saAmfInitialize_5() API into
           AMF internal callback strucutre OsafAmfCallbacksT.
 *
 * @param  osaf_cbk  (ptr to OsafAmfCallbacksT).
 * @param  cbk (ptr to SaAmfCallbacksT_o4).
 */
void amf_copy_from_SaAmfCallbacksT_o4_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk, const SaAmfCallbacksT_o4 *cbk) {
  osaf_cbk->saAmfHealthcheckCallback = cbk->saAmfHealthcheckCallback;
  osaf_cbk->saAmfComponentTerminateCallback = cbk->saAmfComponentTerminateCallback;
  osaf_cbk->saAmfCSISetCallback = cbk->saAmfCSISetCallback;
  osaf_cbk->saAmfCSIRemoveCallback = cbk->saAmfCSIRemoveCallback;
  osaf_cbk->saAmfProtectionGroupTrackCallback_4 = cbk->saAmfProtectionGroupTrackCallback;
  osaf_cbk->saAmfProxiedComponentInstantiateCallback = cbk->saAmfProxiedComponentInstantiateCallback;
  osaf_cbk->saAmfProxiedComponentCleanupCallback = cbk->saAmfProxiedComponentCleanupCallback; 
  osaf_cbk->saAmfContainedComponentInstantiateCallback = cbk->saAmfContainedComponentInstantiateCallback; 
  osaf_cbk->saAmfContainedComponentInstantiateCallback = cbk->saAmfContainedComponentInstantiateCallback; 
  osaf_cbk->saAmfContainedComponentInstantiateCallback = cbk->saAmfContainedComponentInstantiateCallback; 
  osaf_cbk->osafCsiAttributeChangeCallback = cbk->osafCsiAttributeChangeCallback;
}
