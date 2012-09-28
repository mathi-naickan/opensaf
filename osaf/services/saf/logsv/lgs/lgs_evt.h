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

#ifndef LGS_EVT_H
#define LGS_EVT_H

#include <rda_papi.h>

typedef enum lgsv_lgs_evt_type {
	LGSV_LGS_LGSV_MSG = 0,
	LGSV_LGS_EVT_LGA_UP = 1,
	LGSV_LGS_EVT_LGA_DOWN = 2,
	LGSV_EVT_QUIESCED_ACK = 3,
	LGSV_EVT_NO_OP = 4,
	LGSV_EVT_RDA = 5,
	LGSV_LGS_EVT_MAX
} LGSV_LGS_EVT_TYPE;

typedef struct lgsv_lgs_mds_info {
	uint32_t node_id;
	MDS_DEST mds_dest_id;
} lgsv_lgs_mds_info_t;

typedef struct {
	PCS_RDA_ROLE io_role;
} lgsv_rda_info_t;

typedef struct lgsv_lgs_evt {
	struct lgsv_lgs_evt *next;
	struct timespec entered_at;
	uint32_t cb_hdl;
	MDS_SYNC_SND_CTXT mds_ctxt;	/* Relevant when this event has to be responded to
					 * in a synchronous fashion.
					 */
	MDS_DEST fr_dest;
	NODE_ID fr_node_id;
	MDS_SEND_PRIORITY_TYPE rcvd_prio;	/* Priority of the recvd evt */
	LGSV_LGS_EVT_TYPE evt_type;
	union {
		lgsv_msg_t msg;
		lgsv_lgs_mds_info_t mds_info;
		lgsv_rda_info_t rda_info;
	} info;
} lgsv_lgs_evt_t;

/* These are the function prototypes for event handling */
typedef uint32_t (*LGSV_LGS_LGA_API_MSG_HANDLER) (lgs_cb_t *, lgsv_lgs_evt_t *evt);
typedef uint32_t (*LGSV_LGS_EVT_HANDLER) (lgsv_lgs_evt_t *evt);

extern int lgs_client_stream_add(uint32_t client_id, uint32_t stream_id);
extern int lgs_client_stream_rmv(uint32_t client_id, uint32_t stream_id);
extern log_client_t *lgs_client_new(MDS_DEST mds_dest, uint32_t client_id, lgs_stream_list_t *stream_list);
extern log_client_t *lgs_client_get_by_id(uint32_t client_id);
extern int lgs_client_add_stream(log_client_t *client, uint32_t stream_id);
extern int lgs_client_delete(uint32_t client_id);
extern int lgs_client_delete_by_mds_dest(MDS_DEST mds_dest);
extern bool lgs_lga_entry_valid(lgs_cb_t *cb, MDS_DEST mds_dest);
extern uint32_t lgs_remove_lga_down_rec(lgs_cb_t *cb, MDS_DEST mds_dest);
extern void lgs_send_write_log_ack(uint32_t client_id, SaInvocationT invocation, SaAisErrorT error, MDS_DEST mds_dest);
extern void lgs_free_write_log(const lgsv_write_log_async_req_t *param);

#endif   /*!LGS_EVT_H */
