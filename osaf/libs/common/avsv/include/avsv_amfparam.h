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

  This file includes definitions related to AMF APIs.
  
******************************************************************************
*/

#ifndef AVSV_AMFPARAM_H
#define AVSV_AMFPARAM_H

/* AMF API enums */
typedef enum avsv_amf_api_type {
	AVSV_AMF_FINALIZE = 1,
	AVSV_AMF_COMP_REG,
	AVSV_AMF_COMP_UNREG,
	AVSV_AMF_PM_START,
	AVSV_AMF_PM_STOP,
	AVSV_AMF_HC_START,
	AVSV_AMF_HC_STOP,
	AVSV_AMF_HC_CONFIRM,
	AVSV_AMF_CSI_QUIESCING_COMPLETE,
	AVSV_AMF_HA_STATE_GET,
	AVSV_AMF_PG_START,
	AVSV_AMF_PG_STOP,
	AVSV_AMF_ERR_REP,
	AVSV_AMF_ERR_CLEAR,
	AVSV_AMF_RESP,
	AVSV_AMF_INITIALIZE,
	AVSV_AMF_SEL_OBJ_GET,
	AVSV_AMF_DISPATCH,
	AVSV_AMF_COMP_NAME_GET,
	AVSV_AMF_COMP_TERM_RSP,
	AVSV_AMF_API_MAX
} AVSV_AMF_API_TYPE;

/* AMF Callback API enums */
typedef enum avsv_amf_cbk_type {
	AVSV_AMF_HC = 1,
	AVSV_AMF_COMP_TERM,
	AVSV_AMF_CSI_SET,
	AVSV_AMF_CSI_REM,
	AVSV_AMF_PG_TRACK,
	AVSV_AMF_PXIED_COMP_INST,
	AVSV_AMF_PXIED_COMP_CLEAN,
	AVSV_AMF_CBK_MAX
} AVSV_AMF_CBK_TYPE;

/* 
 * API & callback API parameter definitions.
 * These definitions are used for 
 * 1) encoding & decoding messages between AvND & AvA.
 * 2) storing the callback parameters in the pending callback 
 *    list (on AvA & AvND) & overlapping request list (on AvND).
 */

/*** API Parameter definitions ***/

/* initialize - not reqd */
typedef struct avsv_amf_init_param_tag {
	void *dummy;
} AVSV_AMF_INIT_PARAM;

/* selection object get - not reqd */
typedef struct avsv_amf_sel_obj_get_param_tag {
	void *dummy;
} AVSV_AMF_SEL_OBJ_GET_PARAM;

/* dispatch  - not reqd */
typedef struct avsv_amf_dispatch_param_tag {
	void *dummy;
} AVSV_AMF_DISPATCH_PARAM;

/* finalize */
typedef struct avsv_amf_finalize_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp-name (extra param) */
} AVSV_AMF_FINALIZE_PARAM;

/* component register */
typedef struct avsv_amf_comp_reg_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT proxy_comp_name;	/* proxy comp name */
} AVSV_AMF_COMP_REG_PARAM;

/* component unregister */
typedef struct avsv_amf_comp_unreg_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT proxy_comp_name;	/* proxy comp name */
} AVSV_AMF_COMP_UNREG_PARAM;

/* passive monitor start */
typedef struct avsv_amf_pm_start_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaUint64T pid;		/* prc id */
	SaInt32T desc_tree_depth;	/* descendent tree depth */
	SaAmfPmErrorsT pm_err;	/* pm errors */
	union {
		uint32_t raw;
		AVSV_ERR_RCVR avsv_ext;
		SaAmfRecommendedRecoveryT saf_amf;
	} rec_rcvr;
} AVSV_AMF_PM_START_PARAM;

/* passive monitor stop */
typedef struct avsv_amf_pm_stop_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaAmfPmStopQualifierT stop_qual;	/* stop qualifier */
	SaUint64T pid;		/* prc id */
	SaAmfPmErrorsT pm_err;	/* pm errors */
} AVSV_AMF_PM_STOP_PARAM;

/* healthcheck start */
typedef struct avsv_amf_hc_start_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT proxy_comp_name;	/* proxy comp name */
	SaAmfHealthcheckKeyT hc_key;	/* healthcheck key */
	SaAmfHealthcheckInvocationT inv_type;	/* invocation type */
	union {
		uint32_t raw;
		AVSV_ERR_RCVR avsv_ext;
		SaAmfRecommendedRecoveryT saf_amf;
	} rec_rcvr;
} AVSV_AMF_HC_START_PARAM;

/* healthcheck stop */
typedef struct avsv_amf_hc_stop_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT proxy_comp_name;	/* proxy comp name */
	SaAmfHealthcheckKeyT hc_key;	/* healthcheck key */
} AVSV_AMF_HC_STOP_PARAM;

/* healthcheck confirm */
typedef struct avsv_amf_hc_confirm_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT proxy_comp_name;	/* proxy comp name */
	SaAmfHealthcheckKeyT hc_key;	/* healthcheck key */
	SaAisErrorT hc_res;	/* healthcheck result */
} AVSV_AMF_HC_CONFIRM_PARAM;

/* component name get */
typedef struct avsv_amf_comp_name_get_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
} AVSV_AMF_COMP_NAME_GET_PARAM;

/* csi quiescing complete */
typedef struct avsv_amf_csi_quiescing_compl_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaInvocationT inv;	/* invocation value */
	SaAisErrorT err;	/* error */
	SaNameT comp_name;	/* comp-name (extra param) */
} AVSV_AMF_CSI_QUIESCING_COMPL_PARAM;

/* ha state get */
typedef struct avsv_amf_ha_get_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
	SaNameT csi_name;	/* csi name */
	SaAmfHAStateT ha;	/* ha state */
} AVSV_AMF_HA_GET_PARAM;

/* pg start */
typedef struct avsv_amf_pg_start_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT csi_name;	/* csi name */
	SaUint8T flags;		/* track flags */
	bool is_syn;	/* indicates if the appln synchronously
				   waits for the pg members (extra param) */
} AVSV_AMF_PG_START_PARAM;

/* pg stop */
typedef struct avsv_amf_pg_stop_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT csi_name;	/* csi name */
} AVSV_AMF_PG_STOP_PARAM;

/* error report */
typedef struct avsv_amf_err_rep_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT err_comp;	/* erroneous comp */
	SaTimeT detect_time;	/* error detect time */
	union {
		uint32_t raw;
		AVSV_ERR_RCVR avsv_ext;
		SaAmfRecommendedRecoveryT saf_amf;
	} rec_rcvr;
} AVSV_AMF_ERR_REP_PARAM;

/* error clear */
typedef struct avsv_amf_err_clear_param_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaNameT comp_name;	/* comp name */
} AVSV_AMF_ERR_CLEAR_PARAM;

/* response */
typedef AVSV_AMF_CSI_QUIESCING_COMPL_PARAM AVSV_AMF_RESP_PARAM;

/*** Callback Parameter definitions ***/

/* healthcheck */
typedef struct avsv_amf_hc_param_tag {
	SaNameT comp_name;	/* comp name */
	SaAmfHealthcheckKeyT hc_key;	/* healthcheck key */
} AVSV_AMF_HC_PARAM;

/* component terminate */
typedef struct avsv_amf_comp_term_param_tag {
	SaNameT comp_name;	/* comp name */
} AVSV_AMF_COMP_TERM_PARAM;

/* csi set */
typedef struct avsv_amf_csi_set_param_tag {
	SaNameT comp_name;	/* comp name */
	SaAmfHAStateT ha;	/* ha state */
	SaAmfCSIDescriptorT csi_desc;	/* csi descriptor */
	AVSV_CSI_ATTRS attrs;	/* contains the csi-attr list */
} AVSV_AMF_CSI_SET_PARAM;

/* csi remove */
typedef struct avsv_amf_csi_rem_param_tag {
	SaNameT comp_name;	/* comp name */
	SaNameT csi_name;	/* csi name */
	SaAmfCSIFlagsT csi_flags;	/* csi flags */
} AVSV_AMF_CSI_REM_PARAM;

/* pg track */
typedef struct avsv_amf_pg_track_param_tag {
	SaNameT csi_name;	/* csi name */
	SaUint32T mem_num;	/* number of members */
	SaAisErrorT err;	/* error */
	SaAmfProtectionGroupNotificationBufferT buf;	/* notify buffer */
} AVSV_AMF_PG_TRACK_PARAM;

/* proxied component instantiate */
typedef struct avsv_amf_pxied_comp_inst_param_tag {
	SaNameT comp_name;	/* comp name */
} AVSV_AMF_PXIED_COMP_INST_PARAM;

/* proxied component cleanup */
typedef struct avsv_amf_pxied_comp_clean_param_tag {
	SaNameT comp_name;	/* comp name */
} AVSV_AMF_PXIED_COMP_CLEAN_PARAM;

/* wrapper structure for all the callbacks */
typedef struct avsv_amf_cbk_info_tag {
	SaAmfHandleT hdl;	/* AMF handle */
	SaInvocationT inv;	/* invocation value */
	AVSV_AMF_CBK_TYPE type;	/* callback type */
	union {
		AVSV_AMF_HC_PARAM hc;
		AVSV_AMF_COMP_TERM_PARAM comp_term;
		AVSV_AMF_CSI_SET_PARAM csi_set;
		AVSV_AMF_CSI_REM_PARAM csi_rem;
		AVSV_AMF_PG_TRACK_PARAM pg_track;
		AVSV_AMF_PXIED_COMP_INST_PARAM pxied_comp_inst;
		AVSV_AMF_PXIED_COMP_CLEAN_PARAM pxied_comp_clean;
	} param;
} AVSV_AMF_CBK_INFO;

#endif   /* !AVSV_AMFPARAM_H */
