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
        
This include file contains the message definitions for EDSV checkpoint
updates between the ACTIVE and STANDBY EDS peers.
          
******************************************************************************
*/

#ifndef EVT_EVTD_EDS_CKPT_H_
#define EVT_EVTD_EDS_CKPT_H_

#include "eds.h"

#define EDS_MBCSV_VERSION 1
#define EDS_MBCSV_VERSION_MIN 1

/* Checkpoint message types(Used as 'reotype' w.r.t mbcsv)  */

/* Checkpoint update messages are processed similair to edsv internal
 * messages handling,  so the types are defined such that the values 
 * can be mapped straight with edsv message types
 */

typedef enum edsv_ckpt_rec_type {
	EDS_CKPT_MSG_BASE,
	EDS_CKPT_INITIALIZE_REC = EDS_CKPT_MSG_BASE,
	EDS_CKPT_FINALIZE_REC,
	EDS_CKPT_CHAN_REC,
	EDS_CKPT_CHAN_OPEN_REC,
	EDS_CKPT_ASYNC_CHAN_OPEN_REC,
	EDS_CKPT_CHAN_CLOSE_REC,
	EDS_CKPT_CHAN_UNLINK_REC,
	EDS_CKPT_RETEN_REC,
	EDS_CKPT_SUBSCRIBE_REC,
	EDS_CKPT_UNSUBSCRIBE_REC,
	EDS_CKPT_RETENTION_TIME_CLR_REC,
	EDS_CKPT_AGENT_DOWN,
	EDS_CKPT_WARM_SYNC_CSUM,
	EDS_CKPT_MSG_MAX
} EDS_CKPT_DATA_TYPE;
/* CHAN_OPEN is used only in cold sync, while CHAN is used in async for both, synch,asynchronous chan_open api */

/* Structures for Checkpoint data(to be replicated at the standby) */

/* Registrationlist checkpoint record, used in cold/warm/async checkpoint updates */
typedef struct edsv_ckpt_reg_msg {
	uint32_t reg_id;		/* Registration Id at Active */
	MDS_DEST eda_client_dest;	/* Handy when an EDA instance goes away */
} EDS_CKPT_REG_MSG;

/* finalize checkpoint record, used in cold/warm/async checkpoint updates */
typedef struct edsv_ckpt_finalize_msg {
	uint32_t reg_id;		/* Registration Id at Active */
} EDS_CKPT_FINALIZE_MSG;

/* Channel records */
typedef struct edsv_ckpt_chan_msg {
	uint32_t reg_id;
	uint32_t chan_id;
	uint32_t last_copen_id;	/* Last assigned chan_open_id */
	uint32_t chan_attrib;	/* Attributes of this channel */
	uint16_t cname_len;	/* Length of channel name */
	uint8_t cname[SA_MAX_NAME_LENGTH];	/* Channel name. NULL terminated if ascii */
	MDS_DEST chan_opener_dest;
	SaTimeT chan_create_time;
	/* Is it chan_name_len or MAX_SANAME_LEN? */
} EDS_CKPT_CHAN_MSG;

/* channel open list checkpoint record */
typedef struct edsv_ckpt_chan_open_msg {
	uint32_t reg_id;
	uint32_t chan_id;
	uint32_t chan_open_id;
	uint32_t chan_attrib;	/* Attributes of this channel */
	MDS_DEST chan_opener_dest;
	uint16_t cname_len;	/* Length of channel name */
	uint8_t cname[SA_MAX_NAME_LENGTH];	/* Channel name. NULL terminated if ascii */
	SaTimeT chan_create_time;
} EDS_CKPT_CHAN_OPEN_MSG;

/* channel close checkpoint record */
typedef struct edsv_ckpt_chan_close_msg {
	uint32_t reg_id;
	uint32_t chan_id;
	uint32_t chan_open_id;
} EDS_CKPT_CHAN_CLOSE_MSG;

/* channel unlink checkpoint record */
typedef struct edsv_ckpt_chan_unlink_msg {
	uint32_t reg_id;
	SaNameT chan_name;
} EDS_CKPT_CHAN_UNLINK_MSG;

/* retained event list checkpoint record */
typedef struct edsv_ckpt_retain_evt_msg {
	SaTimeT pubtime;
	EDSV_EDA_PUBLISH_PARAM data;	/* Set retention_time -= elapsed time */
} EDS_CKPT_RETAIN_EVT_MSG;
/* A retention event shall be stored in retention event list.
 * After expiry of the timer, in the active, a retention_clear is sent to standby
 * shall delete this event from the retention list.
 */

/* Subscriptionlist checkpoint record */
typedef struct edsv_ckpt_subscribe_msg {
	EDSV_EDA_SUBSCRIBE_PARAM data;
} EDS_CKPT_SUBSCRIBE_MSG;

typedef struct edsv_ckpt_unsubscribe_msg {
	EDSV_EDA_UNSUBSCRIBE_PARAM data;
} EDS_CKPT_UNSUBSCRIBE_MSG;

/* Retention-time clear record */
typedef struct edsv_ckpt_retention_time_clear_msg {
	EDSV_EDA_RETENTION_TIME_CLR_PARAM data;
} EDS_CKPT_RETENTION_TIME_CLEAR_MSG;

/* Checksum structure sent to the STANDBY during a warm-sync.
 * The STANDBY computes checksum on the individual databases and
 * in the case of a mismatch shall notify an cscum mismatch error,
 * following which the ACTIVE sends that specific database again
 * with a checksum.
*/
typedef struct eds_ckpt_data_csum {
	uint16_t reg_csum;
	uint16_t copen_csum;
/*  uint16_t reten_csum; */
	uint16_t subsc_csum;
} EDS_CKPT_DATA_CHECKSUM;

/* Checkpoint message containing eds data of a particular type.
 * Used during cold, warm sync, and async updates.
 */
typedef struct edsv_ckpt_header {
/* SaVersionT          version;  Active peer's Version */
	EDS_CKPT_DATA_TYPE ckpt_rec_type;	/* Type of eds data carried in this checkpoint */
	uint32_t num_ckpt_records;	/* =1 for async updates,>=1 for cold/warm sync */
	uint32_t data_len;		/* Total length of encoded checkpoint data of this type */
/* uint16_t               checksum;  Checksum calculated on the message */
} EDS_CKPT_HEADER;

typedef struct edsv_ckpt_msg {
	EDS_CKPT_HEADER header;
	union {
		EDS_CKPT_REG_MSG reg_rec;
		EDS_CKPT_FINALIZE_MSG finalize_rec;
		EDS_CKPT_CHAN_MSG chan_rec;
		EDS_CKPT_CHAN_OPEN_MSG chan_open_rec;
		EDS_CKPT_CHAN_CLOSE_MSG chan_close_rec;
		EDS_CKPT_CHAN_UNLINK_MSG chan_unlink_rec;
		EDS_CKPT_RETAIN_EVT_MSG retain_evt_rec;
		EDS_CKPT_SUBSCRIBE_MSG subscribe_rec;
		EDS_CKPT_UNSUBSCRIBE_MSG unsubscribe_rec;
		EDS_CKPT_RETENTION_TIME_CLEAR_MSG reten_time_clr_rec;
		EDS_CKPT_DATA_CHECKSUM warm_sync_csum;
		MDS_DEST agent_dest;
	} ckpt_rec;
/*   EDS_CKPT_REC        *next;  This may not be needed */
} EDS_CKPT_DATA;

/* NOTE: All Statistical data should be checkpointed as well. 
 * To maintain count for published messages, an empty CKPT_MSG shall be sent
 * for all EDS_CKPT_PUBLISH_REC types 
 */
typedef struct eds_ckpt_message {
	uint32_t tot_data_len;	/* Combined length of all record types */
	EDS_CKPT_DATA *ckpt_data;	/* Multiple instances of ckpt_data for different eds records */
} EDS_CKPT_MSG;
/* Error codes sent by STANDBY during checkpoint updates */
typedef enum eds_ckpt_csum_err {
	EDS_REG_REC_CSUM_MISMATCH = 0,
	EDS_CHAN_REC_CSUM_MISMATCH,
	EDS_RETEN_REC_CSUM_MISMATCH,
	EDS_SUBSC_REC_CSUM_MISMATCH,
	EDS_MISC_REC_CSUM_MISMATCH
} EDS_CKPT_CSUM_MISMATCH;

typedef enum eds_ckpt_enc_err {
	EDS_REG_ENC_ERROR = 0,
	EDS_CHAN_OPEN_ENC_ERROR,
	EDS_RETEN_ENC_ERROR,
	EDS_SUBSC_ENC_ERROR,
	EDS_CHAN_CLOSE_ENC_ERROR,
	EDS_CHAN_UNLINK_ENC_ERROR,
	EDS_FINALIZE_ENC_ERROR,
	EDS_RETEN_CLEAR_ENC_ERROR
} EDS_CKPT_ENC_ERROR;

typedef enum eds_ckpt_dec_err {
	EDS_REG_DEC_ERROR = 0,
	EDS_CHAN_OPEN_DEC_ERROR,
	EDS_RETEN_DEC_ERROR,
	EDS_SUBSC_DEC_ERROR,
	EDS_CHAN_CLOSE_DEC_ERROR,
	EDS_CHAN_UNLINK_DEC_ERROR,
	EDS_FINALIZE_DEC_ERROR,
	EDS_RETEN_CLEAR_DEC_ERROR
} EDS_CKPT_DEC_ERROR;

#define m_EDS_FILL_VERSION(rec) { \
 rec.version.releaseCode='A'; \
 rec.version.majorVersion=0x01; \
 rec.versionminorVersion=0x01; \
}

#define m_EDS_COPY_REG_REC(rec,list) { \
 rec->reg_id=list->reg_id; \
 rec->eda_client_dest=list->eda_client_dest; \
}

#define m_EDS_COPY_CHAN_REC(chan,wl) { \
 chan->chan_id=wl->chan_id; \
 chan->last_copen_id=wl->last_copen_id; \
 chan->chan_attrib |=SA_EVT_CHANNEL_CREATE; \
 chan->chan_create_time = wl->chan_row.create_time; \
 chan->cname_len=wl->cname_len; \
 memcpy(chan->cname,wl->cname,wl->cname_len); \
}
/*Here cname should be appended with '\0', TBD */

#define m_EDS_COPY_CHAN_OPEN_REC(rec,list,wl) { \
 rec->reg_id=list->reg_id; \
 rec->chan_id=list->chan_id; \
 rec->chan_open_id=list->chan_open_id; \
 rec->chan_attrib = list->chan_open_flags; \
 memcpy(&rec->chan_opener_dest,&list->chan_opener_dest,sizeof(MDS_DEST)); \
 rec->cname_len=wl->cname_len; \
 memcpy(rec->cname,wl->cname,wl->cname_len); \
}

/* rec.reg_id=list.reg_id \ */

#define m_EDS_COPY_SUBSC_REC(rec,s) {\
 rec->data.sub_id=s->subscript_id; \
 rec->data.chan_id=s->chan_id; \
 rec->data.chan_open_id=s->chan_open_id; \
 rec->data.filter_array=s->filters; \
}

#define m_EDS_COPY_RETEN_REC(rec,list) {\
rec->pubtime=list->publishTime; \
rec->data.event_id=list->event_id;\
rec->data.reg_id=list->reg_id;\
rec->data.chan_id=list->chan_id;\
rec->data.chan_open_id=list->retd_evt_chan_open_id;\
rec->data.pattern_array=list->patternArray;\
rec->data.priority=list->priority;\
m_NCS_TMR_MSEC_REMAINING(list->ret_tmr.tmr_id,&remaining_time);\
rec->data.retention_time=(SaTimeT)remaining_time; \
rec->data.retention_time=rec->data.retention_time*100000; /*Revisit this */ \
rec->data.publisher_name.length=list->publisherName.length;\
memcpy(rec->data.publisher_name.value,list->publisherName.value,list->publisherName.length);\
rec->data.data_len=list->data_len;\
rec->data.data=list->data;\
}

#define m_MMGR_ALLOC_EDSV_CKPT_DATA        (EDS_CKPT_DATA *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_DATA), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_CKPT_DATA(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_REG_MSG        (EDS_CKPT_REG_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_REG_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_REG_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_FINALIZE_MSG        (EDS_CKPT_FINALIZE_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_FINALIZE_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_FINALIZE_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_CHAN_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_CHAN_MSG        (EDS_CKPT_CHAN_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_CHAN_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_CHAN_OPEN_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_CHAN_OPEN_MSG        (EDS_CKPT_CHAN_OPEN_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_CHAN_OPEN_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_CHAN_UNLINK_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_CHAN_UNLINK_MSG        (EDS_CKPT_CHAN_UNLINK_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_CHAN_UNLINK_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_CHAN_CLOSE_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_CHAN_CLOSE_MSG        (EDS_CKPT_CHAN_CLOSE_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_CHAN_CLOSE_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_RETAIN_EVT_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_RETAIN_EVT_MSG        (EDS_CKPT_RETAIN_EVT_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_RETAIN_EVT_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_SUBSCRIBE_EVT_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_SUBSCRIBE_EVT_MSG        (EDS_CKPT_SUBSCRIBE_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_SUBSCRIBE_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_UNSUBSCRIBE_EVT_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_UNSUBSCRIBE_EVT_MSG        (EDS_CKPT_UNSUBSCRIBE_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_UNSUBSCRIBE_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_RETENTION_TIME_CLEAR_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_RETENTION_TIME_CLEAR_MSG        (EDS_CKPT_RETENTION_TIME_CLEAR_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_RETENTION_TIME_CLEAR_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_FREE_EDSV_DATA_CHECKSUM_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_MMGR_ALLOC_EDSV_DATA_CHECKSUM_MSG        (EDS_CKPT_DATA_CHECKSUM_MSG *)m_NCS_MEM_ALLOC(sizeof(EDS_CKPT_DATA_CHECKSUM_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG)

#define m_EDSV_FILL_ASYNC_UPDATE_REG(ckpt,regid,dest) {\
  ckpt.header.ckpt_rec_type=EDS_CKPT_INITIALIZE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.reg_rec.reg_id = regid;\
  ckpt.ckpt_rec.reg_rec.eda_client_dest = dest;\
}

#define m_EDSV_FILL_ASYNC_UPDATE_CHAN(ckpt,evt,cid,coid,dest,chan_create_time){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_ASYNC_CHAN_OPEN_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.chan_rec.reg_id = evt->reg_id; \
  ckpt.ckpt_rec.chan_rec.chan_id = cid;\
  ckpt.ckpt_rec.chan_rec.last_copen_id = coid;\
  ckpt.ckpt_rec.chan_rec.chan_attrib = evt->chan_open_flags; \
  ckpt.ckpt_rec.chan_rec.chan_opener_dest = dest;\
  ckpt.ckpt_rec.chan_rec.cname_len = evt->chan_name.length; \
  ckpt.ckpt_rec.chan_rec.chan_create_time = chan_create_time; \
  memcpy(ckpt.ckpt_rec.chan_rec.cname,evt->chan_name.value,ckpt.ckpt_rec.chan_rec.cname_len);\
}

#define m_EDSV_FILL_ASYNC_UPDATE_CCLOSE(ckpt,regid,cid,coid){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_CHAN_CLOSE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.chan_close_rec.reg_id = regid; \
  ckpt.ckpt_rec.chan_close_rec.chan_id = cid;\
  ckpt.ckpt_rec.chan_close_rec.chan_open_id = coid;\
}

#define m_EDSV_FILL_ASYNC_UPDATE_CUNLINK(ckpt,cname,clen){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_CHAN_UNLINK_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.chan_unlink_rec.chan_name.length = clen;\
  memcpy(ckpt.ckpt_rec.chan_unlink_rec.chan_name.value,cname,clen);\
}

#define m_EDSV_FILL_ASYNC_UPDATE_FINALIZE(ckpt,regid){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_FINALIZE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.finalize_rec.reg_id= regid;\
}

#define m_EDSV_FILL_ASYNC_UPDATE_RETAIN_EVT(ckpt,evt,pub){\
  ckpt.header.ckpt_rec_type=EDS_CKPT_RETEN_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.retain_evt_rec.pubtime=pub;\
  ckpt.ckpt_rec.retain_evt_rec.data.event_id=evt->event_id;\
  ckpt.ckpt_rec.retain_evt_rec.data.reg_id=evt->reg_id;\
  ckpt.ckpt_rec.retain_evt_rec.data.chan_id=evt->chan_id;\
  ckpt.ckpt_rec.retain_evt_rec.data.chan_open_id=evt->chan_open_id;\
  ckpt.ckpt_rec.retain_evt_rec.data.priority=evt->priority;\
  ckpt.ckpt_rec.retain_evt_rec.data.retention_time=evt->retention_time;\
  ckpt.ckpt_rec.retain_evt_rec.data.publisher_name.length=evt->publisher_name.length;\
  memcpy(ckpt.ckpt_rec.retain_evt_rec.data.publisher_name.value,evt->publisher_name.value,evt->publisher_name.length);\
  ckpt.ckpt_rec.retain_evt_rec.data.pattern_array=evt->pattern_array;\
  ckpt.ckpt_rec.retain_evt_rec.data.data=evt->data;\
  ckpt.ckpt_rec.retain_evt_rec.data.data_len=evt->data_len; \
}

#define m_EDSV_FILL_ASYNC_UPDATE_SUBSCRIBE(ckpt,evt){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_SUBSCRIBE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.subscribe_rec.data.reg_id=evt->reg_id;\
  ckpt.ckpt_rec.subscribe_rec.data.sub_id=evt->sub_id;\
  ckpt.ckpt_rec.subscribe_rec.data.chan_id=evt->chan_id;\
  ckpt.ckpt_rec.subscribe_rec.data.chan_open_id=evt->chan_open_id;\
  ckpt.ckpt_rec.subscribe_rec.data.filter_array=evt->filter_array;\
}

#define m_EDSV_FILL_ASYNC_UPDATE_UNSUBSCRIBE(ckpt,evt){ \
  ckpt.header.ckpt_rec_type=EDS_CKPT_UNSUBSCRIBE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.unsubscribe_rec.data.reg_id=evt->reg_id;\
  ckpt.ckpt_rec.unsubscribe_rec.data.sub_id=evt->sub_id;\
  ckpt.ckpt_rec.unsubscribe_rec.data.chan_id=evt->chan_id;\
  ckpt.ckpt_rec.unsubscribe_rec.data.chan_open_id=evt->chan_open_id;\
}

#define m_EDSV_FILL_ASYNC_UPDATE_RETEN_CLEAR(ckpt,evt) {\
  ckpt.header.ckpt_rec_type=EDS_CKPT_RETENTION_TIME_CLR_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.reten_time_clr_rec.data.event_id=evt->event_id;\
  ckpt.ckpt_rec.reten_time_clr_rec.data.chan_id=evt->chan_id;\
  ckpt.ckpt_rec.reten_time_clr_rec.data.chan_open_id=evt->chan_open_id;\
}
#define m_EDSV_FILL_ASYNC_UPDATE_AGENT_DOWN(ckpt,dest) {\
  ckpt.header.ckpt_rec_type=EDS_CKPT_AGENT_DOWN; \
  ckpt.header.num_ckpt_records=1;\
  ckpt.header.data_len=1;\
  ckpt.ckpt_rec.agent_dest=dest;\
}
typedef uint32_t (*EDS_CKPT_HDLR) (EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_mbcsv_init(EDS_CB *eds_cb);
uint32_t eds_mbcsv_open_ckpt(EDS_CB *cb);
uint32_t eds_mbcsv_change_HA_state(EDS_CB *cb);
uint32_t eds_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl);

uint32_t eds_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);	/* Common Callback interface to mbcsv */
uint32_t eds_ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_ckpt_enc_cold_sync_data(EDS_CB *eds_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req);
uint32_t eds_edu_enc_reg_list(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_edu_enc_chan_rec(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_edu_enc_chan_open_list(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_edu_enc_subsc_list(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_edu_enc_reten_list(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_ckpt_encode_async_update(EDS_CB *eds_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_ckpt_decode_async_update(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_ckpt_decode_cold_sync(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_process_ckpt_data(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_reg_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_finalize_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_chan_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_chan_open_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_chan_close_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_chan_unlink_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_reten_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_subscribe_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_unsubscribe_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_ret_time_clr_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_proc_agent_down_rec(EDS_CB *cb, EDS_CKPT_DATA *data);
uint32_t eds_ckpt_warm_sync_csum_dec_hdlr(EDS_CB *cb, NCS_UBAID *uba);
uint32_t eds_ckpt_warm_sync_csum_enc_hdlr(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t eds_ckpt_enc_warm_sync_csum(EDS_CB *cb, NCS_UBAID *uba);
uint32_t compute_reg_csum(EDS_CB *cb, uint16_t *cksum);
uint32_t compute_copen_csum(EDS_CB *cb, uint16_t *cksum);
uint32_t compute_subsc_csum(EDS_CB *cb, uint16_t *cksum);
uint32_t send_async_update(EDS_CB *cb, EDS_CKPT_DATA *ckpt_rec, uint32_t action);
uint32_t edsv_ckpt_send_cold_sync(EDS_CB *eds_cb);
uint32_t eds_ckpt_send_async_update(EDS_CB *eds_cb);
uint32_t eds_ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg);
uint32_t eds_ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg);
uint32_t eds_ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg);
uint32_t eds_edp_ed_ckpt_hdr(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_reg_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			 NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_chan_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_chan_open_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_chan_close_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_chan_ulink_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_ret_clr_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_csum_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_usubsc_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_finalize_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len,
			      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t eds_edp_ed_header_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
int32_t eds_ckpt_msg_test_type(NCSCONTEXT arg);
uint32_t eds_edp_ed_ckpt_msg(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
void eds_enc_ckpt_header(uint8_t *pdata, EDS_CKPT_HEADER header);
uint32_t eds_enc_ckpt_reten_msg(NCS_UBAID *uba, EDS_CKPT_RETAIN_EVT_MSG *msg);
uint32_t eds_ckpt_enc_subscribe_msg(NCS_UBAID *uba, EDS_CKPT_SUBSCRIBE_MSG *msg);
uint32_t eds_ckpt_enc_reten_msg(NCS_UBAID *uba, EDS_CKPT_RETAIN_EVT_MSG *msg);

uint32_t eds_ckpt_enc_warm_sync_data(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t mbcsv_cold_sync_driver(EDS_CB *cb);
uint32_t eds_dec_ckpt_header(NCS_UBAID *uba, EDS_CKPT_HEADER *header);

#endif  // EVT_EVTD_EDS_CKPT_H_
