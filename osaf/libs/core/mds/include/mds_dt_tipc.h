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

  DESCRIPTION:  MDS DT headers

******************************************************************************
*/
#ifndef _MDS_DT_TIPC_H
#define _MDS_DT_TIPC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/param.h>
#include <sys/poll.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "mds_dt.h"

/******************************/
/* TIPC Service Name specific */
/******************************/

#define MDS_TIPC_NM_PREFIX 0x4d44	/* "M" = ASCII 0x4d */

typedef enum {
	MDS_TIPC_NM_TYPE_SVC_INST = 0x01000000,
	MDS_TIPC_NM_TYPE_VDEST,
	MDS_TIPC_NM_TYPE_PCON_ID_DETAILS,
	MDS_TIPC_NM_TYPE_NODE_ID_DETAILS,
} MDS_TIPC_NM_TYPE;

typedef struct mdtm_tipc_tx_info {
	int blah;
} MDTM_TIPC_TX_INFO;

typedef struct mds_dt_tipc_cb {
	int BSRSockfd;		/* Bind, send, recv socket */
	int DSockfd;		/* Discovery socket - Do we
				   need a separate socket */
} MDS_DT_TIPC_CB;

extern uns32 mdtm_find_adest(struct tipc_portid id, MDS_DEST *adest);
				   /* TIPC-ID is given in return adest is provided */
extern uns32 mdtm_tipc_init(NODE_ID node_id, uns32 *mds_tipc_ref);
extern uns32 mdtm_tipc_destroy(void);

extern uns32 mds_mdtm_svc_install_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				       V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
				       MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
extern uns32 mds_mdtm_svc_uninstall_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
					 V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
					 MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
extern uns32 mds_mdtm_svc_subscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
					 MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);
extern uns32 mds_mdtm_svc_unsubscribe_tipc(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, 
			NCSMDS_SCOPE_TYPE install_scope, MDS_SUBTN_REF_VAL subtn_ref_val);
extern uns32 mds_mdtm_vdest_install_tipc(MDS_VDEST_ID vdest_id);
extern uns32 mds_mdtm_vdest_uninstall_tipc(MDS_VDEST_ID vdest_id);
extern uns32 mds_mdtm_vdest_subscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val);
extern uns32 mds_mdtm_vdest_unsubscribe_tipc(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val);
extern uns32 mds_mdtm_tx_hdl_register_tipc(MDS_DEST adest);
extern uns32 mds_mdtm_tx_hdl_unregister_tipc(MDS_DEST adest);

extern uns32 mds_mdtm_send_tipc(MDTM_SEND_REQ *req);

extern uns32 mds_mdtm_node_subscribe_tipc(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val); 
extern uns32 mds_mdtm_node_unsubscribe_tipc(MDS_SUBTN_REF_VAL subtn_ref_val); 

#endif
