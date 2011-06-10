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

  This module is the main include file for Cluster Membership Agent (CLMA).
  
******************************************************************************
*/

#ifndef CLMA_H
#define CLMA_H

/* CLMA CB global handle declaration */

#include <pthread.h>
#include <assert.h>
#include <limits.h>
#include <saClm.h>

#include <ncs_main_papi.h>
#include <ncssysf_ipc.h>
#include <mds_papi.h>
#include <ncs_hdl_pub.h>
#include <ncsencdec_pub.h>
#include <ncs_util.h>
#include <logtrace.h>

#include "clmsv_msg.h"
#include "clmsv_defs.h"

#define CLMA_SVC_PVT_SUBPART_VERSION  1
#define CLMA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT 1
#define CLMA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT 1
#define CLMA_WRT_CLMS_SUBPART_VER_RANGE             \
        (CLMA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT - \
         CLMA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT + 1)
#define m_CLMA_CONVERT_SATIME_TEN_MILLI_SEC(t)    (t)/(10000000)

/* CLMA client record */
typedef struct clma_client_hdl_rec {
	unsigned int clms_client_id;	/* handle value returned by CLMS for this client */
	unsigned int local_hdl;	/* LOG handle (derived from hdl-mngr) */
	SaVersionT *version;
	union {
		SaClmCallbacksT_4 reg_cbk_4;
		SaClmCallbacksT reg_cbk;
	} cbk_param;
	SYSF_MBX mbx;		/* priority q mbx b/w MDS & Library */
	SaBoolT is_member;
	SaBoolT is_configured;
	struct clma_client_hdl_rec *next;	/* next pointer for the list in clma_cb_t */
} clma_client_hdl_rec_t;

/*
 * The CLMA control block is the master anchor structure for all CLMA
 * instantiations within a process.
 */
typedef struct {
	pthread_mutex_t cb_lock;	/* CB lock */
	clma_client_hdl_rec_t *client_list;	/* CLMA client handle database */
	MDS_HDL mds_hdl;	/* MDS handle */
	MDS_DEST clms_mds_dest;	/* CLMS absolute/virtual address */
	int clms_up;		/* Indicate that MDS subscription
				 * is complete */
	/* CLMS CLMA sync params */
	int clms_sync_awaited;
	NCS_SEL_OBJ clms_sync_sel;
} clma_cb_t;

/* clma_api.c */
extern clma_cb_t clma_cb;
extern uns32 clma_validate_version(SaVersionT *version);

extern uns32 clma_mds_init(clma_cb_t * cb);
extern uns32 clma_mds_msg_sync_send(clma_cb_t * cb, CLMSV_MSG * i_msg, CLMSV_MSG ** o_msg, uns32 timeout);
extern uns32 clma_mds_msg_async_send(clma_cb_t * cb, CLMSV_MSG * i_msg, uns32 prio);

extern unsigned int clma_startup(void);
extern unsigned int clma_shutdown(void);
extern void clma_msg_destroy(CLMSV_MSG * msg);
extern void clma_mds_finalize(clma_cb_t * cb);

extern clma_client_hdl_rec_t *clma_find_hdl_rec_by_client_id(clma_cb_t * clma_cb, uns32 client_id);
extern clma_client_hdl_rec_t *clma_hdl_rec_add(clma_cb_t * cb, const SaClmCallbacksT *reg_cbks_1,
					       const SaClmCallbacksT_4 * reg_cbks_4,
					       SaVersionT *version, uns32 client_id);
extern uns32 clma_hdl_rec_del(clma_client_hdl_rec_t ** list_head, clma_client_hdl_rec_t * rm_node);
extern SaAisErrorT clma_hdl_cbk_dispatch(clma_cb_t * cb, clma_client_hdl_rec_t * hdl_rec, SaDispatchFlagsT flags);

extern uns32 clma_clms_msg_proc(clma_cb_t * cb, CLMSV_MSG * clmsv_msg, MDS_SEND_PRIORITY_TYPE prio);
extern void clma_add_to_async_cbk_msg_list(CLMSV_MSG ** head, CLMSV_MSG * new_node);
extern void clma_fill_clusterbuf_from_buf_4(SaClmClusterNotificationBufferT *buf,
					    SaClmClusterNotificationBufferT_4 * buf_4);
extern void clma_fill_node_from_node4(SaClmClusterNodeT *clusterNode, SaClmClusterNodeT_4 clusterNode_4);

#endif   /* !CLMA_H */
