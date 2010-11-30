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

  This file contains EDS's MDS interaction logic.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

*******************************************************************************/
#ifndef EDS_MDS_H
#define EDS_MDS_H

#define EDS_SVC_PVT_SUBPART_VERSION  1
#define EDS_WRT_EDA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define EDS_WRT_EDA_SUBPART_VER_AT_MAX_MSG_FMT 1
#define EDS_WRT_EDA_SUBPART_VER_RANGE             \
        (EDS_WRT_EDA_SUBPART_VER_AT_MAX_MSG_FMT - \
         EDS_WRT_EDA_SUBPART_VER_AT_MIN_MSG_FMT +1)

EXTERN_C uns32 eds_mds_init(EDS_CB *);
EXTERN_C uns32 eds_mds_vdest_create(EDS_CB *);
EXTERN_C uns32 eds_mds_finalize(EDS_CB *cb);
EXTERN_C uns32 eds_mds_vdest_destroy(EDS_CB *cb);
EXTERN_C uns32 eds_mds_change_role(EDS_CB *cb);
EXTERN_C uns32 eds_mds_msg_send(EDS_CB *cb,
				EDSV_MSG *msg,
				MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio);

EXTERN_C uns32 eds_mds_ack_send(EDS_CB *cb, EDSV_MSG *msg, MDS_DEST dest, uns32 timeout, MDS_SEND_PRIORITY_TYPE prio);

EXTERN_C uns32 eds_dec_subscribe_msg(NCS_UBAID *uba, long msg_hdl, uns8 ckpt_flag);

EXTERN_C uns32 eds_dec_publish_msg(NCS_UBAID *uba, long msg_hdl, uns8 ckpt_flag);

/*****************************************************************************
                 Macros to fill the MDS message structure
*****************************************************************************/
/* Macro to populate the 'EVT Initialize' response message */
#define m_EDS_EDSV_INIT_MSG_FILL(m, rs, regid) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_RESP_MSG; \
   (m).info.api_resp_info.type.api_rsp = EDSV_EDA_INITIALIZE_RSP_MSG; \
   (m).info.api_resp_info.rc = (rs); \
   if (rs == SA_AIS_OK);             \
   {                                 \
      (m).info.api_resp_info.param.init_rsp.reg_id = (regid); \
   } \
} while (0);

/* Macro to populate the limit get response message */
#define m_EDS_EDSV_LIMIT_GET_MSG_FILL(m, rs) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_RESP_MSG; \
   (m).info.api_resp_info.type.api_rsp = EDSV_EDA_LIMIT_GET_RSP_MSG; \
   (m).info.api_resp_info.rc = (rs); \
   if (rs == SA_AIS_OK);             \
   {                                 \
      (m).info.api_resp_info.param.limit_get_rsp.max_chan = EDSV_MAX_CHANNELS;\
      (m).info.api_resp_info.param.limit_get_rsp.max_evt_size = SA_EVENT_DATA_MAX_SIZE;\
      (m).info.api_resp_info.param.limit_get_rsp.max_ptrn_size = EDSV_MAX_PATTERN_SIZE;\
      (m).info.api_resp_info.param.limit_get_rsp.max_num_ptrns = EDSV_MAX_PATTERNS;\
      (m).info.api_resp_info.param.limit_get_rsp.max_ret_time = EDSV_MAX_RETENTION_TIME;\
   } \
} while (0);

/* Macro to populate the 'EVT Channel Open' response message */
#define m_EDS_EDSV_CHAN_OPEN_SYNC_MSG_FILL(m, rs, chanid, copenid) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_RESP_MSG; \
   (m).info.api_resp_info.type.api_rsp = EDSV_EDA_CHAN_OPEN_SYNC_RSP_MSG; \
   (m).info.api_resp_info.rc = (rs); \
   (m).info.api_resp_info.param.chan_open_rsp.chan_id = (chanid); \
   (m).info.api_resp_info.param.chan_open_rsp.chan_open_id = (copenid); \
} while (0);

/* Macro to populate the 'EVT Channel Unlink' response message */
#define m_EDS_EDSV_CHAN_UNLINK_SYNC_MSG_FILL(m, rs, cname) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_RESP_MSG; \
   (m).info.api_resp_info.type.api_rsp = EDSV_EDA_CHAN_UNLINK_SYNC_RSP_MSG; \
   (m).info.api_resp_info.rc = (rs); \
   (m).info.api_resp_info.param.chan_unlink_rsp.chan_name = (cname); \
  } while (0);

/* Macro to populate the 'EVT Channel Unlink' response message */
#define m_EDS_EDSV_CHAN_RETENTION_TMR_CLEAR_SYNC_MSG_FILL(m, rs) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_RESP_MSG; \
   (m).info.api_resp_info.type.api_rsp = EDSV_EDA_CHAN_RETENTION_TIME_CLEAR_SYNC_RSP_MSG; \
   (m).info.api_resp_info.rc = (rs); \
  } while (0);
/* Macro to populate the 'EVT Channel Open' callback message */
#define m_EDS_EDSV_CHAN_OPEN_CB_MSG_FILL(m, regid, invocation, cname, chanid, \
                                         copenid, copenflags, err) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDS_CBK_MSG; \
   (m).info.cbk_info.type = EDSV_EDS_CHAN_OPEN; \
   (m).info.cbk_info.eds_reg_id = (regid); \
   (m).info.cbk_info.inv = (invocation); \
   (m).info.cbk_info.param.chan_open_cbk.chan_name = (cname); \
   (m).info.cbk_info.param.chan_open_cbk.chan_id = (chanid); \
   (m).info.cbk_info.param.chan_open_cbk.chan_open_id = (copenid); \
   (m).info.cbk_info.param.chan_open_cbk.chan_open_flags = (copenflags); \
   (m).info.cbk_info.param.chan_open_cbk.eda_chan_hdl = (0); \
   (m).info.cbk_info.param.chan_open_cbk.error = (err); \
} while (0);

/* Macro to populate the 'EVT Event Deliver' callback message */
#define m_EDS_EDSV_DELIVER_EVENT_CB_MSG_FILL(m, regid, subid, chanid,\
                                            copenid, ptrns, prio, \
                                            pub_name, pub_time, ret_time, \
                                            evid, recoid, buf_len, buf) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDS_CBK_MSG; \
   (m).info.cbk_info.type = EDSV_EDS_DELIVER_EVENT; \
   (m).info.cbk_info.eds_reg_id = (regid); \
   (m).info.cbk_info.param.evt_deliver_cbk.sub_id = (subid); \
   (m).info.cbk_info.param.evt_deliver_cbk.chan_id = (chanid); \
   (m).info.cbk_info.param.evt_deliver_cbk.chan_open_id = (copenid); \
   (m).info.cbk_info.param.evt_deliver_cbk.pattern_array = (ptrns); \
   (m).info.cbk_info.param.evt_deliver_cbk.priority = (prio); \
   (m).info.cbk_info.param.evt_deliver_cbk.publisher_name = (pub_name); \
   (m).info.cbk_info.param.evt_deliver_cbk.publish_time = (pub_time); \
   (m).info.cbk_info.param.evt_deliver_cbk.retention_time = (ret_time); \
   (m).info.cbk_info.param.evt_deliver_cbk.eda_event_id = (evid); \
   (m).info.cbk_info.param.evt_deliver_cbk.ret_evt_ch_oid = (recoid); \
   (m).info.cbk_info.param.evt_deliver_cbk.data_len =    (buf_len); \
   (m).info.cbk_info.param.evt_deliver_cbk.data =        (buf); \
} while (0);

/* Macro to populate the 'CLM Cluster Node Status' callback message */
#define m_EDS_EDSV_CLM_STATUS_CB_MSG_FILL(m, cluster_change) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDS_CBK_MSG; \
   (m).info.cbk_info.type = EDSV_EDS_CLMNODE_STATUS; \
   (m).info.cbk_info.param.clm_status_cbk.node_status = (cluster_change); \
} while (0);

#define m_EDS_GET_NODE_ID_FROM_ADEST(adest) (NODE_ID) ((uns64)adest >> 32)

#endif   /* !EDS_MDS_H */
