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

  MODULE : MQD_LOG.H

..............................................................................

  DESCRIPTION:

  This module contains logging/tracing defines for MQD.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef MQD_LOG_H
#define MQD_LOG_H

/******************************************************************************\
            Logging offset indexes for Headline logging
\******************************************************************************/
typedef enum mqd_hdln_flex {
	MQD_CREATE_SUCCESS,
	MQD_CREATE_FAILED,
	MQD_INIT_SUCCESS,
	MQD_INIT_FAILED,
	MQD_CREATE_HDL_SUCCESS,
	MQD_CREATE_HDL_FAILED,
	MQD_CB_HDL_TAKE_FAILED,
	MQD_AMF_INIT_SUCCESS,
	MQD_AMF_INIT_FAILED,
	MQD_LM_INIT_SUCCESS,
	MQD_LM_INIT_FAILED,
	MQD_MDS_INIT_SUCCESS,
	MQD_MDS_INIT_FAILED,
	MQD_REG_COMP_SUCCESS,
	MQD_REG_COMP_FAILED,
	MQD_ASAPi_BIND_SUCCESS,
	MQD_EDU_BIND_SUCCESS,

	MQD_EDU_UNBIND_SUCCESS,
	MQD_ASAPi_UNBIND_SUCCESS,
	MQD_DEREG_COMP_SUCCESS,
	MQD_MDS_SHUT_SUCCESS,
	MQD_MDS_SHUT_FAILED,
	MQD_LM_SHUT_SUCCESS,
	MQD_AMF_SHUT_SUCCESS,
	MQD_DESTROY_SUCCESS,

	MQD_COMP_NOT_INSERVICE,
	MQD_DONOT_EXIST,
	MQD_MEMORY_ALLOC_FAIL,

	MQD_ASAPi_REG_MSG_RCV,
	MQD_ASAPi_DEREG_MSG_RCV,
	MQD_ASAPi_NRESOLVE_MSG_RCV,
	MQD_ASAPi_GETQUEUE_MSG_RCV,
	MQD_ASAPi_TRACK_MSG_RCV,
	MQD_ASAPi_REG_RESP_MSG_SENT,
	MQD_ASAPi_DEREG_RESP_MSG_SENT,
	MQD_ASAPi_NRESOLVE_RESP_MSG_SENT,
	MQD_ASAPi_GETQUEUE_RESP_MSG_SENT,
	MQD_ASAPi_TRACK_RESP_MSG_SENT,
	MQD_ASAPi_TRACK_NTFY_MSG_SENT,
	MQD_ASAPi_REG_RESP_MSG_ERR,
	MQD_ASAPi_DEREG_RESP_MSG_ERR,
	MQD_ASAPi_NRESOLVE_RESP_MSG_ERR,
	MQD_ASAPi_GETQUEUE_RESP_MSG_ERR,
	MQD_ASAPi_TRACK_RESP_MSG_ERR,
	MQD_ASAPi_TRACK_NTFY_MSG_ERR,
	MQD_ASAPi_EVT_COMPLETE_STATUS,
	MQD_OBJ_NODE_GET_FAILED,
	MQD_DB_ADD_SUCCESS,
	MQD_DB_ADD_FAILED,
	MQD_DB_DEL_SUCCESS,
	MQD_DB_DEL_FAILED,
	MQD_DB_UPD_SUCCESS,
	MQD_DB_UPD_FAILED,
	MQD_DB_TRACK_ADD,
	MQD_DB_TRACK_DEL,
	MQD_CB_ALLOC_FAILED,
	MQD_A2S_EVT_ALLOC_FAILED,
	MQD_OBJ_NODE_ALLOC_FAILED,
	MQD_RED_DB_NODE_ALLOC_FAILED,
	NCS_ENC_RESERVE_SPACE_FAILED,
	MQD_RED_TRACK_OBJ_ALLOC_FAILED,
	MQD_RED_BAD_A2S_TYPE,
	MQD_RED_MBCSV_INIT_FAILED,
	MQD_RED_MBCSV_OPEN_FAILED,
	MQD_RED_MBCSV_FINALIZE_FAILED,
	MQD_RED_MBCSV_SELOBJGET_FAILED,
	MQD_RED_MBCSV_CHGROLE_FAILED,
	MQD_RED_MBCSV_CHGROLE_SUCCESS,
	MQD_RED_MBCSV_DISPATCH_FAILURE,
	MQD_RED_MBCSV_DISPATCH_SUCCESS,
	MQD_RED_MBCSV_ASYNCUPDATE_FAILURE,
	MQD_RED_MBCSV_ASYNCUPDATE_SUCCESS,
	MQD_RED_MBCSV_DATA_REQ_SEND_FAILED,
	MQD_RED_MBCSV_ASYNCUPDATE_REG_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_DEREG_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_TRACK_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_USEREVT_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_NDSTAT_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_NDTMREXP_ENC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_REG_DEC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_DEREG_DEC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_TRACK_DEC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_USEREVT_DEC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_NDSTAT_DEC_EDU_ERROR,
	MQD_RED_MBCSV_ASYNCUPDATE_NDTMREXP_DEC_EDU_ERROR,
	MQD_RED_STANDBY_REG_FAILED,
	MQD_RED_STANDBY_DEREG_FAILED,
	MQD_RED_STANDBY_TRACK_FAILED,
	MQD_RED_STANDBY_USEREVT_FAILED,
	MQD_RED_STANDBY_NDSTATINFO_FAILED,
	MQD_RED_STANDBY_NDTMREXP_FAILED,
	MQD_RED_STANDBY_PROCESSING_FAILED,
	MQD_RED_STANDBY_PROCESS_REG_SUCCESS,
	MQD_RED_STANDBY_QUEUE_NODE_NOT_PRESENT,
	MQD_RED_STANDBY_PROCESS_DEREG_SUCCESS,
	MQD_RED_STANDBY_PROCESS_DEREG_FAILURE,
	MQD_RED_STANDBY_PROCESS_TRACK_SUCCESS,
	MQD_RED_STANDBY_PROCESS_TRACK_FAILURE,
	MQD_RED_STANDBY_PROCESS_USEREVT_SUCCESS,
	MQD_RED_STANDBY_COLD_SYNC_RESP_DECODE_SUCCESS,
	MQD_RED_STANDBY_COLD_SYNC_RESP_DECODE_FAILURE,
	MQD_RED_ACTIVE_COLD_SYNC_RESP_ENCODE_SUCCESS,
	MQD_RED_ACTIVE_COLD_SYNC_RESP_ENCODE_FAILURE,
	MQD_RED_STANDBY_WARM_SYNC_RESP_DECODE_SUCCESS,
	MQD_RED_STANDBY_WARM_SYNC_RESP_DECODE_FAILURE,
	MQD_RED_ACTIVE_WARM_SYNC_RESP_ENCODE_SUCCESS,
	MQD_RED_ACTIVE_WARM_SYNC_RESP_ENCODE_FAILURE,
	MQD_RED_ACTIVE_DATA_RESP_ENCODE_SUCCESS,
	MQD_RED_ACTIVE_DATA_RESP_ENCODE_FAILURE,
	MQD_RED_STANDBY_DATA_RESP_DECODE_SUCCESS,
	MQD_RED_STANDBY_DATA_RESP_DECODE_FAILURE,
	MQD_AMF_HEALTH_CHECK_START_SUCCESS,
	MQD_AMF_HEALTH_CHECK_START_FAILED,
	MQD_MIB_TBL_REQ_SUCCESS,
	MQD_MIB_TBL_REQ_FAILED,
	MQD_MIB_SCALAR_TBL_REG_WITH_MAB_FAILED,
	MQD_MIB_SCALAR_TBL_REG_WITH_MAB_SUCCESS,
	MQD_MIB_SCALAR_TBL_ROW_OWNED_FAILED,
	MQD_MIB_SCALAR_TBL_ROW_OWNED_SUCCESS,
	MQD_MIB_QGROUP_TBL_OWNED_FAILED,
	MQD_MIB_QGROUP_TBL_OWNED_SUCCESS,
	MQD_MIB_QGROUP_TBL_ROW_OWNED_FAILED,
	MQD_MIB_QGROUP_TBL_ROW_OWNED_SUCCESS,
	MQD_MIB_QGROUP_MEMBERS_TBL_OWNED_FAILED,
	MQD_MIB_QGROUP_MEMBERS_TBL_OWNED_SUCCESS,
	MQD_MIB_QGROUP_MEMBERS_TBL_ROW_OWNED_FAILED,
	MQD_MIB_QGROUP_MEMBERS_TBL_ROW_OWNED_SUCCESS,
	MQD_MIB_SCALARTBL_UNREG_WITH_MAB_FAILED,
	MQD_MIB_SCALARTBL_UNREG_WITH_MAB_SUCCESS,
	MQD_MIB_GRPENTRYTBL_UNREG_WITH_MAB_FAILED,
	MQD_MIB_GRPENTRYTBL_UNREG_WITH_MAB_SUCCESS,
	MQD_MIB_GRPMEMBRSENTRYTBL_UNREG_WITH_MAB_FAILED,
	MQD_MIB_GRPMEMBRSENTRYTBL_UNREG_WITH_MAB_SUCCESS,
	MQD_MIB_SCALARTBL_REG_WITH_MIBLIB_FAILED,
	MQD_MIB_SCALARTBL_REG_WITH_MIBLIB_SUCCESS,
	MQD_MIB_GRPENTRYTBL_REG_WITH_MIBLIB_FAILED,
	MQD_MIB_GRPENTRYTBL_REG_WITH_MIBLIB_SUCCESS,
	MQD_MIB_GRPMEMBERSENTRYTBL_REG_WITH_MIBLIB_FAILED,
	MQD_MIB_GRPMEMBERSENTRYTBL_REG_WITH_MIBLIB_SUCCESS,
	MQD_MDS_INSTALL_FAILED,
	MQD_MDS_SUBSCRIPTION_FAILED,
	MQD_VDS_CREATE_FAILED,
	MQD_MDS_UNINSTALL_FAILED,
	MQD_VDEST_DESTROY_FAILED,
	MQD_MDS_ENCODE_FAILED,
	MQD_MDS_DECODE_FAILED,
	MQD_MDS_RCV_SEND_FAILED,
	MQD_MDS_SVC_EVT_MQA_DOWN_EVT_SEND_FAILED,
	MQD_MDS_SVC_EVT_MQND_DOWN_EVT_SEND_FAILED,
	MQD_MDS_SVC_EVT_MQND_UP_EVT_SEND_FAILED,
	MQD_MDS_SEND_API_FAILED,
	MQD_MDS_QUISCED_EVT_SEND_FAILED,
	MQD_VDEST_CHG_ROLE_FAILED,
	MQD_MDS_MSG_COMP_EVT_SEND_FAILED,
	MQD_QUISCED_VDEST_CHGROLE_FAILED,
	MQD_QUISCED_VDEST_CHGROLE_SUCCESS,
	MQD_CSI_SET_ROLE,
	MQD_CSI_REMOVE_CALLBK_CHGROLE_FAILED,
	MQD_CSI_REMOVE_CALLBK_SUCCESSFULL,
	MQD_REG_HDLR_DB_UPDATE_FAILED,
	MQD_REG_HDLR_DB_UPDATE_SUCCESS,
	MQD_DEREG_HDLR_DB_UPDATE_FAILED,
	MQD_DEREG_HDLR_DB_UPDATE_SUCCESS,
	MQD_GROUP_REMOVE_QUEUE_SUCCESS,
	MQD_GROUP_DELETE_SUCCESS,
	MQD_QUEUE_DELETE_SUCCESS,
	MQD_DEREG_HDLR_MIB_EVT_SEND_SUCCESS,
	MQD_DEREG_HDLR_MIB_EVT_SEND_FAILED,
	MQD_GROUP_TRACK_DB_UPDATE_SUCCESS,
	MQD_GROUP_TRACK_DB_UPDATE_FAILURE,
	MQD_GROUP_TRACKSTOP_DB_UPDATE_SUCCESS,
	MQD_GROUP_TRACKSTOP_DB_UPDATE_FAILURE,
	MQD_REG_HDLR_MIB_EVT_SEND_FAILED,
	MQD_REG_HDLR_MIB_EVT_SEND_SUCCESS,
	MQD_REG_DB_UPD_ERR_EXIST,
	MQD_REG_DB_QUEUE_UPDATE_SUCCESS,
	MQD_REG_DB_QUEUE_CREATE_SUCCESS,
	MQD_REG_DB_QUEUE_CREATE_FAILED,
	MQD_REG_DB_QUEUE_GROUP_CREATE_SUCCESS,
	MQD_REG_DB_QUEUE_GROUP_CREATE_FAILED,
	MQD_EVT_QUISCED_PROCESS_SUCCESS,
	MQD_EVT_QUISCED_PROCESS_MBCSVCHG_ROLE_FAILURE,
	MQD_EVT_UNSOLICITED_QUISCED_ACK,
	MQD_REG_DB_GRP_INSERT_SUCCESS,
	MQD_NRESOLV_HDLR_DB_ACCESS_FAILED,
	MQD_DB_UPD_MQND_DOWN,
	MQD_ASAPi_QUEUE_MAKE_SUCCESS,
	MQD_ASAPi_QUEUE_MAKE_FAILED,
	MQD_TMR_EXPIRED,
	MQD_TMR_START,
	MQD_TMR_STOPPED,
	MQD_MSG_FRMT_VER_INVALID
} MQD_HDLN_FLEX;

/******************************************************************************\
      Logging offset indexes for canned constant strings for the ASCII SPEC
\******************************************************************************/
typedef enum mqd_flex_sets {
	MQD_FC_HDLN,
} MQD_FLEX_SETS;

typedef enum mqd_log_ids {
	MQD_LID_HDLN,
} MQD_LOG_IDS;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                          MQSVMQD Logging Control
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

EXTERN_C void mqd_flx_log_reg(void);
EXTERN_C void mqd_flx_log_dereg(void);

#if((NCS_DTA == 1) && (NCS_MQSV_LOG == 1))
EXTERN_C void mqd_log(uns8, uns32, uns8, uns32, char *, uns32);
#define m_LOG_MQSV_D(id,category,sev,rc,fname,fno)  mqd_log(id,category,sev,rc,fname,fno)
#else
#define m_LOG_MQSV_D(id,category,sev,rc,fname,fno)
#endif   /* (NCS_DTA == 1) && (NCS_MQSV_LOG == 1) */

#endif   /* MQD_LOG_H */
