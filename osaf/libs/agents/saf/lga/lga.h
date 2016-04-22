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
 * Author(s): Ericsson AB
 *
 */

#ifndef LGA_H
#define LGA_H

#include <pthread.h>
#include <assert.h>
#include <saLog.h>

#include <ncs_main_papi.h>
#include <ncssysf_ipc.h>
#include <mds_papi.h>
#include <ncs_hdl_pub.h>
#include <ncsencdec_pub.h>
#include <ncs_util.h>
#include <logtrace.h>
#include <osaf_time.h>

#include "lgsv_msg.h"
#include "lgsv_defs.h"

#define LGA_SVC_PVT_SUBPART_VERSION  1
#define LGA_WRT_LGS_SUBPART_VER_AT_MIN_MSG_FMT 1
#define LGA_WRT_LGS_SUBPART_VER_AT_MAX_MSG_FMT 1
#define LGA_WRT_LGS_SUBPART_VER_RANGE             \
        (LGA_WRT_LGS_SUBPART_VER_AT_MAX_MSG_FMT - \
         LGA_WRT_LGS_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* Log Stream Handle Definition */
typedef struct lga_log_stream_hdl_rec {
	unsigned int log_stream_hdl;	/* Log stream HDL from handle mgr */
	SaNameT log_stream_name;	/* log stream name mentioned during open log stream */
	unsigned int open_flags;	/* log stream open flags as defined in AIS.02.01 */
	unsigned int log_header_type;	/* log header type */
	unsigned int lgs_log_stream_id;	/* server reference for this log stream */
	struct lga_log_stream_hdl_rec *next;	/* next pointer for the list in lga_client_hdl_rec_t */
	struct lga_client_hdl_rec *parent_hdl;	/* Back Pointer to the client instantiation */
	/* This flag is used with recovery handling. It is valid only
	 * after server down has happened (will be initiated when server down
	 * event occurs). It's not valid in LGA_NORMAL state.
	 */
	bool recovered_flag;
} lga_log_stream_hdl_rec_t;

/* LGA client record */
typedef struct lga_client_hdl_rec {
	unsigned int lgs_client_id;	/* handle value returned by LGS for this client */
	unsigned int local_hdl;	/* LOG handle (derived from hdl-mngr) */
	SaLogCallbacksT reg_cbk;	/* callbacks registered by the application */
	lga_log_stream_hdl_rec_t *stream_list;	/* List of open streams per client */
	SYSF_MBX mbx;		/* priority q mbx b/w MDS & Library */
	struct lga_client_hdl_rec *next;	/* next pointer for the list in lga_cb_t */
	/* These flags are used with recovery handling. They are valid only
	 * after server down has happened (will be initiated when server down
	 * event occurs). They are not valid in LGA_NORMAL state
	 */
	bool initialized_flag;      /* Used with "headless" recovery handling
				     * Set when client is initialized. Streams
				     * may not have been recovered
				     */
	bool recovered_flag;        /* Used with "headless" recovery handling
				     * Set when client is initialized an all
				     * streams are recovered
				     */
} lga_client_hdl_rec_t;

/* States of the server */
typedef enum {
	LGS_START,      /* The state before agent is started
			 */
	LGS_DOWN,       /* Server is down (headless)
			 */
	LGS_NO_ACTIVE,  /* No active server (switch/fail - over)
			 */
	LGS_UP          /* Server is up
			 */
} lgs_state_t;

/* Agent internal states */
typedef enum {
	LGA_NORMAL,     /* Server is up and no recovery is ongoing
			 */
	LGA_NO_SERVER,  /* No Server (Server down) state
			 */
	LGA_RECOVERY1,  /* Server is up. Recover clients and streams when
			 * request from client.
			 * Recovery1 timer is running
			 */
	LGA_RECOVERY2   /* Auto recover remaining clients and streams
			 * After recovery1 timeout
			 */
} lga_state_t;
/*
 * The LGA control block is the master anchor structure for all LGA
 * instantiations within a process.
 */
typedef struct {
	pthread_mutex_t cb_lock;	/* CB lock */
	lga_client_hdl_rec_t *client_list;	/* LGA client handle database */
	MDS_HDL mds_hdl;	/* MDS handle */
	MDS_DEST lgs_mds_dest;	/* LGS absolute/virtual address */
	lgs_state_t lgs_state;	/* Indicate current server MDS state */
	/* LGS LGA sync params */
	int lgs_sync_awaited;
	NCS_SEL_OBJ lgs_sync_sel;
} lga_cb_t;

/* lga_saf_api.c */
extern lga_cb_t lga_cb;

/* lga_mds.c */
extern uint32_t lga_mds_init(lga_cb_t *cb);
extern void lga_mds_finalize(lga_cb_t *cb);
extern uint32_t lga_mds_msg_sync_send(lga_cb_t *cb, lgsv_msg_t *i_msg, lgsv_msg_t **o_msg, uint32_t timeout,uint32_t prio);
extern uint32_t lga_mds_msg_async_send(lga_cb_t *cb, lgsv_msg_t *i_msg, uint32_t prio);
extern void lgsv_lga_evt_free(struct lgsv_msg *);

/* lga_init.c */
unsigned int lga_startup(lga_cb_t *cb);
extern unsigned int lga_shutdown_after_last_client(void);
extern unsigned int lga_force_shutdown(void);

/* lga_hdl.c */
extern SaAisErrorT lga_hdl_cbk_dispatch(lga_cb_t *, lga_client_hdl_rec_t *, SaDispatchFlagsT);
extern lga_client_hdl_rec_t *lga_hdl_rec_add(lga_cb_t *lga_cb, const SaLogCallbacksT *reg_cbks, uint32_t client_id);
extern lga_log_stream_hdl_rec_t *lga_log_stream_hdl_rec_add(lga_client_hdl_rec_t **hdl_rec,
							    uint32_t log_stream_id,
							    uint32_t log_stream_open_flags,
							    const SaNameT *logStreamName, uint32_t log_header_type);
extern void lga_hdl_list_del(lga_client_hdl_rec_t **);
extern uint32_t lga_hdl_rec_del(lga_client_hdl_rec_t **, lga_client_hdl_rec_t *);
extern uint32_t lga_log_stream_hdl_rec_del(lga_log_stream_hdl_rec_t **, lga_log_stream_hdl_rec_t *);
extern bool lga_validate_lga_client_hdl(lga_cb_t *lga_cb, lga_client_hdl_rec_t *find_hdl_rec);

/* lga_util.c */
extern lga_client_hdl_rec_t *lga_find_hdl_rec_by_regid(lga_cb_t *lga_cb, uint32_t client_id);
extern void lga_msg_destroy(lgsv_msg_t *msg);

#endif   /* !LGA_H */
