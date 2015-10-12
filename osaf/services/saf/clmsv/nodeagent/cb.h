/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010,2015 The OpenSAF Foundation
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
 * Author(s):  GoAhead Software
 *             Ericsson AB
 *
 */

#ifndef CLMNA_CB_H
#define CLMNA_CB_H

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <configmake.h>
#include <daemon.h>
#include <nid_api.h>
#include <mds_papi.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tmr.h>


/* Self node information */
typedef struct node_detail_t {
	SaUint32T node_id;
	SaNameT node_name;
} NODE_INFO;

/* CLM Server control block */
typedef struct clmna_cb_t {
	/* CLMS CLMNA sync params */
	int clms_sync_awaited;
	NCS_SEL_OBJ clms_sync_sel;
	MDS_HDL mds_hdl;
	MDS_DEST clms_mds_dest;		/* CLMS absolute/virtual address */
	SYSF_MBX mbx;		/* CLMNA's Mail Box */
	NCS_SEL_OBJ mbx_fd;
	SaNameT comp_name;	/* My AMF name */
	SaAmfHandleT amf_hdl;	/* Handle obtained from AMF */
	SaSelectionObjectT amf_sel_obj;	/* AMF provided selection object */
	SaAmfHAStateT ha_state;	/* My current AMF HA state */
	SaBoolT server_synced;
	NODE_INFO node_info;
	tmr_t scale_out_retry_tmr;
	bool is_scale_out_retry_tmr_running;
	bool nid_started;	/**< true if started by NID */
} CLMNA_CB;


#endif 

