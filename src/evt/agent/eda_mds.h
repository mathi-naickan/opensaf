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

  EDA-EDS communication related definitions.
  
******************************************************************************
*/

#ifndef EVT_AGENT_EDA_MDS_H_
#define EVT_AGENT_EDA_MDS_H_

#include "evt/agent/eda.h"

/* 
 * Wrapper structure that encapsulates communication 
 * semantics for communication with EDS
 */

#define EDA_SVC_PVT_SUBPART_VERSION  1
#define EDA_WRT_EDS_SUBPART_VER_AT_MIN_MSG_FMT 1
#define EDA_WRT_EDS_SUBPART_VER_AT_MAX_MSG_FMT 1
#define EDA_WRT_EDS_SUBPART_VER_RANGE             \
        (EDA_WRT_EDS_SUBPART_VER_AT_MAX_MSG_FMT - \
         EDA_WRT_EDS_SUBPART_VER_AT_MIN_MSG_FMT +1)

/*****************************************************************************
                 Macros to fill the MDS message structure
*****************************************************************************/
/* Macro to populate the 'EVT Initialize' message */
#define m_EDA_EDSV_INIT_MSG_FILL(m, ver) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_INITIALIZE; \
   (m).info.api_info.param.init.version = (ver); \
} while (0);

/*Macro to populate the 'Lost Event' message */
#define m_EDA_EDSV_LOST_EVT_MSG_FILL(m,regid) \
do { \
   memset(&(m),0,sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_LOST_EVENT; \
} while (0);

/* Macro to populate the 'EVT Finalize' message */
#define m_EDA_EDSV_FINALIZE_MSG_FILL(m, regid) \
do { \
    memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_FINALIZE; \
   (m).info.api_info.param.finalize.reg_id = (regid); \
} while (0);

/* Macro to populate the 'EVT Channel Open' sync message */
#define m_EDA_EDSV_CHAN_OPEN_SYNC_MSG_FILL(m, regid, copenflags, cn) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_CHAN_OPEN_SYNC; \
   (m).info.api_info.param.chan_open_sync.reg_id = (regid); \
   (m).info.api_info.param.chan_open_sync.chan_open_flags = (copenflags); \
   (m).info.api_info.param.chan_open_sync.chan_name = (cn); \
} while (0);

/* Macro to populate the 'EVT Channel Open' aync message */
#define m_EDA_EDSV_CHAN_OPEN_ASYNC_MSG_FILL(m, i, regid, \
                                            copenflags, cn) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_CHAN_OPEN_ASYNC; \
   (m).info.api_info.param.chan_open_async.inv = (i); \
   (m).info.api_info.param.chan_open_async.reg_id = (regid); \
   (m).info.api_info.param.chan_open_async.chan_open_flags = (copenflags); \
   (m).info.api_info.param.chan_open_async.chan_name = (cn); \
} while (0);

/* Macro to populate the 'EVT Channel CLOSE' message */
#define m_EDA_EDSV_CHAN_CLOSE_MSG_FILL(m, regid, cid, copenid) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_CHAN_CLOSE; \
   (m).info.api_info.param.chan_close.reg_id = (regid); \
   (m).info.api_info.param.chan_close.chan_id = (cid); \
   (m).info.api_info.param.chan_close.chan_open_id = (copenid); \
} while (0);

/* Macro to populate the 'EVT Channel UNLINK' message */
#define m_EDA_EDSV_CHAN_UNLINK_MSG_FILL(m, regid, cn) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_CHAN_UNLINK; \
   (m).info.api_info.param.chan_unlink.reg_id = (regid); \
   (m).info.api_info.param.chan_unlink.chan_name = (cn); \
} while (0);

/* Macro to populate the 'EVT Channel PUBLISH' message */
#define m_EDA_EDSV_PUBLISH_MSG_FILL(m, regid, cid, copenid, pat, prio, \
                                    rettime, pubname, evtid, dlen, d) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_PUBLISH; \
   (m).info.api_info.param.publish.reg_id = (regid); \
   (m).info.api_info.param.publish.chan_id = (cid); \
   (m).info.api_info.param.publish.chan_open_id = (copenid); \
   (m).info.api_info.param.publish.pattern_array = (pat); \
   (m).info.api_info.param.publish.priority = (prio); \
   (m).info.api_info.param.publish.retention_time = (rettime); \
   (m).info.api_info.param.publish.publisher_name = (pubname); \
   (m).info.api_info.param.publish.event_id = (evtid); \
   (m).info.api_info.param.publish.data_len = (dlen); \
   (m).info.api_info.param.publish.data = (d); \
} while (0);

/* Macro to populate the 'EVT Channel SUBSCRIBE' message */
#define m_EDA_EDSV_SUBSCRIBE_MSG_FILL(m, regid, cid, copenid, subid, filt) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_SUBSCRIBE; \
   (m).info.api_info.param.subscribe.reg_id = (regid); \
   (m).info.api_info.param.subscribe.chan_id = (cid); \
   (m).info.api_info.param.subscribe.chan_open_id = (copenid); \
   (m).info.api_info.param.subscribe.sub_id = (subid); \
   (m).info.api_info.param.subscribe.filter_array = (filt); \
} while (0);

/* Macro to populate the 'EVT Channel UNSUBSCRIBE' message */
#define m_EDA_EDSV_UNSUBSCRIBE_MSG_FILL(m, regid, cid, copenid, subid) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_UNSUBSCRIBE; \
   (m).info.api_info.param.unsubscribe.reg_id = (regid); \
   (m).info.api_info.param.unsubscribe.chan_id = (cid); \
   (m).info.api_info.param.unsubscribe.chan_open_id = (copenid); \
   (m).info.api_info.param.unsubscribe.sub_id = (subid); \
} while (0);

/* Macro to populate the 'EVT Channel 'RETENTION TIME CLEAR' message */
#define m_EDA_EDSV_CHAN_RETENTION_TIME_CLR_MSG_FILL(m, regid, cid, \
                                                    copenid, evtid) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_RETENTION_TIME_CLR; \
   (m).info.api_info.param.rettimeclr.reg_id = (regid); \
   (m).info.api_info.param.rettimeclr.chan_id = (cid); \
   (m).info.api_info.param.rettimeclr.chan_open_id = (copenid); \
   (m).info.api_info.param.rettimeclr.event_id = (evtid); \
} while (0);

/* macro to copy incoming event data to event handle record */
#define m_CPY_EVTDATA_TO_EVT_HDLREC(m,n)\
do { \
   (m)->priority = (n)->priority; \
   (m)->publisher_name = (n)->publisher_name; \
   (m)->publish_time = (n)->publish_time; \
   (m)->retention_time = (n)->retention_time; \
   (m)->event_data_size = (n)->data_len; \
} while (0);

/* Macro to populate the 'Limit Get' message */
#define m_EDA_EDSV_LIMIT_GET_MSG_FILL(m) \
do { \
   memset(&(m), 0, sizeof(EDSV_MSG)); \
   (m).type = EDSV_EDA_API_MSG; \
   (m).info.api_info.type = EDSV_EDA_LIMIT_GET; \
} while (0);

/*** Extern function declarations ***/

uint32_t eda_mds_init(EDA_CB *cb);
void eda_sync_with_eds(EDA_CB *cb);
void eda_mds_finalize(EDA_CB *cb);
uint32_t eda_mds_msg_sync_send(struct eda_cb_tag *cb,
				     struct edsv_msg *i_msg, struct edsv_msg **o_msg, SaTimeT timeout);

uint32_t eda_mds_msg_async_send(struct eda_cb_tag *cb, struct edsv_msg *i_msg, uint32_t prio);
void edsv_eda_evt_free(struct edsv_msg *);

#endif  // EVT_AGENT_EDA_MDS_H_
