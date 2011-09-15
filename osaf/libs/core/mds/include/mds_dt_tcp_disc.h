/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#ifndef _MDS_DT_TCP_DISC_H
#define _MDS_DT_TCP_DISC_H

#include "mds_dt.h"
#include "mds_log.h"
#include "ncssysf_def.h"
#include "ncssysf_tsk.h"
#include "ncssysf_mem.h"
#include "mds_dt_tcp_disc.h"
#include "sys/poll.h"

/*  mds_indentifire + mds_version   + msg_type + scope_type + server_type +  server_instance_lower +
server_instance_upper + sub_ref_val + sub_ref_val  + node_id  +process_id */
/* 4 + 1 + 1 + 1 + 4 + 4 + 4 + 8 + 4 + 4 */
#define MDS_MDTM_DTM_SUBSCRIBE_SIZE 35

/*   mds_indentifire + mds_version +   msg_type + sub_ref_val +node_id + process_id*/
#define MDS_MDTM_DTM_UNSUBSCRIBE_SIZE 22	/*4 + 1 + 1 + 8 + 4 + 4 */

/*  mds_indentifire + mds_version +   msg_type + sub_ref_val + node_id + process_id*/
#define MDS_MDTM_DTM_NODE_SUBSCRIBE_SIZE 22	/*4 + 1+ 1 + 8 + 4 + 4 */

/* mds_indentifire + mds_version +   msg_type + sub_ref_val + node_id + process_id*/
#define MDS_MDTM_DTM_NODE_UNSUBSCRIBE_SIZE 22	/*4 + 1 + 1 + 8 + 4 + 4 */

/*  mds_indentifire + mds_version +   msg_type+ install_scope + server_type
     server_instance_lower + server_instance_upper + node_id + process_id + install_scope */
#define MDS_MDTM_DTM_SVC_INSTALL_SIZE 27	/*4 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + 4 */

/* mds_indentifire + mds_version +   msg_type+ install_scope + server_type + server_instance_lower + 
   server_instance_upper  + node_id + process_id + install_scope*/
#define MDS_MDTM_DTM_SVC_UNINSTALL_SIZE 27	/*4 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + 4 */

/* Send_buffer_size + MDS_MDTM_DTM_SUBSCRIBE_SIZE*/
#define MDS_MDTM_DTM_SUBSCRIBE_BUFFER_SIZE  (2 + MDS_MDTM_DTM_SUBSCRIBE_SIZE)

/* Send_buffer_size + MDS_MDTM_DTM_UNSUBSCRIBE_SIZE*/
#define MDS_MDTM_DTM_UNSUBSCRIBE_BUFFER_SIZE (2 + MDS_MDTM_DTM_UNSUBSCRIBE_SIZE)

/* Send_buffer_size + MDS_MDTM_DTM_NODE_SUBSCRIBE_SIZE*/
#define MDS_MDTM_DTM_NODE_SUBSCRIBE_BUFFER_SIZE (2 + MDS_MDTM_DTM_NODE_SUBSCRIBE_SIZE)

/* Send_buffer_size + MDS_MDTM_DTM_NODE_UNSUBSCRIBE_SIZE*/
#define MDS_MDTM_DTM_NODE_UNSUBSCRIBE_BUFFER_SIZE (2+ + MDS_MDTM_DTM_NODE_UNSUBSCRIBE_SIZE)

/* Send_buffer_size + MDS_MDTM_DTM_SVC_INSTALL_SIZE*/
#define MDS_MDTM_DTM_SVC_INSTALL_BUFFER_SIZE (2 + MDS_MDTM_DTM_SVC_INSTALL_SIZE)

/* Send_buffer_size + MDS_MDTM_DTM_SVC_UNINSTALL_SIZE*/
#define MDS_MDTM_DTM_SVC_UNINSTALL_BUFFER_SIZE (2 + MDS_MDTM_DTM_SVC_UNINSTALL_SIZE)

#define MDS_IDENTIFIRE 0x56123456
#define MDS_SND_VERSION 1
#define MDS_RCV_IDENTIFIRE 0x56123456
#define MDS_RCV_VERSION 1
#define DTM_INTRANODE_MSG_SIZE 1500
#define DTM_INTRANODE_UNSENT_MSG 200

#define MDS_MDTM_LOWER_INSTANCE 0x00000000
#define MDS_MDTM_UPPER_INSTANCE 0xffffffff

typedef struct dtm_intranode_unsent_msgs {
	uns16 len;
	uns8 *buffer;
	struct dtm_intranode_unsent_msgs *next;
} MDTM_INTRANODE_UNSENT_MSGS;

typedef struct mds_mdtm_bind_msg {
	uns8 install_scope;
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	NODE_ID node_id;
	uns32 process_id;
} MDS_MDTM_BIND_MSG;

typedef struct mds_mdtm_unbind_msg {
	uns8 install_scope;
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	NODE_ID node_id;
	uns32 process_id;
} MDS_MDTM_UNBIND_MSG;

typedef struct mds_mdtm_subscribe_msg {
	NCSMDS_SCOPE_TYPE scope_type;
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	MDS_SUBTN_REF_VAL sub_ref_val;
	NODE_ID node_id;
	uns32 process_id;
} MDS_MDTM_SUBSCRIBE_MSG;

typedef struct mds_mdtm_unsubscribe_msg {
	NCSMDS_SCOPE_TYPE scope_type;
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	MDS_SUBTN_REF_VAL sub_ref_val;
	NODE_ID node_id;
	uns32 process_id;
} MDS_MDTM_UNSUBSCRIBE_MSG;

typedef struct mds_mdtm_node_subscribe_msg {
	NODE_ID node_id;
	uns32 process_id;
	MDS_SUBTN_REF_VAL sub_ref_val;
} MDS_MDTM_NODE_SUBSCRIBE_MSG;

typedef struct mds_mdtm_node_unsubscribe_msg {
	NODE_ID node_id;
	uns32 process_id;
	MDS_SUBTN_REF_VAL sub_ref_val;
} MDS_MDTM_NODE_UNSUBSCRIBE_MSG;

uns32 mds_mdtm_svc_install_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
			       V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
			       MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
uns32 mds_mdtm_svc_uninstall_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				 V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
				 MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);
uns32 mds_mdtm_svc_subscribe_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				 MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);
uns32 mds_mdtm_svc_unsubscribe_tcp(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope, MDS_SUBTN_REF_VAL subtn_ref_val);
uns32 mds_mdtm_vdest_install_tcp(MDS_VDEST_ID vdest_id);
uns32 mds_mdtm_vdest_uninstall_tcp(MDS_VDEST_ID vdest_id);
uns32 mds_mdtm_vdest_subscribe_tcp(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val);
uns32 mds_mdtm_vdest_unsubscribe_tcp(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val);
uns32 mds_mdtm_tx_hdl_register_tcp(MDS_DEST adest);
uns32 mds_mdtm_tx_hdl_unregister_tcp(MDS_DEST adest);

uns32 mds_mdtm_node_subscribe_tcp(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);
uns32 mds_mdtm_node_unsubscribe_tcp(MDS_SUBTN_REF_VAL subtn_ref_val);

#endif
