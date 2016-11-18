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

  AvA - AvND communication related definitions.
  
******************************************************************************
*/

#ifndef AMF_AGENT_AVA_MDS_H_
#define AMF_AGENT_AVA_MDS_H_

#ifdef  __cplusplus
extern "C" {
#endif

/* In Service upgrade support */
#define AVA_MDS_SUB_PART_VERSION   2

#define AVA_AVND_SUBPART_VER_MIN   1
#define AVA_AVND_SUBPART_VER_MAX   2

/*****************************************************************************
                 Function to fill the MDS message structure
*****************************************************************************/
void ava_fill_finalize_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name);
void ava_fill_comp_reg_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT proxy_comp_name,
		SaVersionT *version);
void ava_fill_comp_unreg_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT proxy_comp_name);
void ava_fill_hc_start_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT proxy_comp_name,
		SaAmfHealthcheckKeyT hc_key, SaAmfHealthcheckInvocationT hc_inv,
		SaAmfRecommendedRecoveryT rcv);
void ava_fill_hc_stop_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT proxy_comp_name,
		SaAmfHealthcheckKeyT hc_key);
void ava_fill_hc_confirm_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT proxy_comp_name,
		SaAmfHealthcheckKeyT hc_key, SaAisErrorT res);
void ava_fill_pm_start_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaUint64T processId,
		SaInt32T depth, SaAmfPmErrorsT pmErr, SaAmfRecommendedRecoveryT rcv);
void ava_fill_pm_stop_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaAmfPmStopQualifierT stop,
		SaUint64T processId, SaAmfPmErrorsT pmErr);
void ava_fill_ha_state_get_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaNameT csi_name);
void ava_fill_pg_start_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT csi_name, SaUint8T flag, bool syn);
void ava_fill_pg_stop_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT csi_name);
void ava_fill_error_report_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name, SaTimeT time,
		SaAmfRecommendedRecoveryT rcv);
void ava_fill_err_clear_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaNameT comp_name);
void ava_fill_response_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error,
		SaNameT comp_name);
void ava_fill_csi_quiescing_complete_msg(AVSV_NDA_AVA_MSG* msg, MDS_DEST dst,
		SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error,
		SaNameT comp_name);

/*** Extern function declarations ***/
struct ava_cb_tag;
uint32_t ava_mds_reg(struct ava_cb_tag *);

uint32_t ava_mds_unreg(struct ava_cb_tag *);

uint32_t ava_mds_cbk(NCSMDS_CALLBACK_INFO *);

uint32_t ava_mds_send(struct ava_cb_tag *, AVSV_NDA_AVA_MSG *, AVSV_NDA_AVA_MSG **);

#ifdef  __cplusplus
}
#endif
#endif  // AMF_AGENT_AVA_MDS_H_
