/*      OpenSAF
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

#ifndef SMF_SMFND_SMFND_H_
#define SMF_SMFND_SMFND_H_

#include "base/ncsgl_defs.h"
#include "base/ncssysf_ipc.h"
#include "mds/mds_papi.h"
#include "base/ncs_hdl_pub.h"
#include "base/ncs_util.h"
#include "base/logtrace.h"

#include "smf/smfsv_defs.h"
#include "smf/smfsv_evt.h"

#define SMFND_HA_INIT_STATE 0
/* Added CMD_REQ_ASYNCH message to protocol */
#define SMFND_MDS_PVT_SUBPART_VERSION 2

/*
This structure represents the mapping between the inv_id and all the agents need to
respond to the callback. Every invid represents a callback. The place holder for this
is smfnd_cb_t.
*/
typedef struct smfnd_smfa_adest_invid_map{
	SaInvocationT				inv_id;
	uint32_t					no_of_cbk;
	/* List of inv_id.*/
	struct smfnd_smfa_adest_invid_map	*next_invid;
}SMFND_SMFA_ADEST_INVID_MAP;

/*
 * The LGA control block is the master anchor structure for all LGA
 * instantiations within a process.
 */
typedef struct {
	SYSF_MBX mbx;		                /* SMFND mailbox                             */
	V_DEST_RL mds_role;	                /* Current MDS role - ACTIVE/STANDBY         */
	uint32_t mds_handle;	                /* My MDS handle                             */
	MDS_DEST mds_dest;                     	/* My destination in MDS                     */
	SaVersionT smf_version;	                /* The version currently supported           */
	SaNameT comp_name;	                /* Components's name                         */
	SaAmfHandleT amf_hdl;	                /* AMF handle, obtained thru AMF init        */
	SaInvocationT amf_invocation_id;        /* AMF InvocationID - needed to handle Quiesed state */
	bool is_quisced_set;
	SaSelectionObjectT amfSelectionObject;	/* Selection Object to wait for amf events   */
	SaAmfHAStateT ha_state;	                /* present AMF HA state of the component     */
	SMFND_SMFA_ADEST_INVID_MAP *cbk_list;	/* Mapping between inv_id and all the agents */
	uint32_t agent_cnt;			/* Count of SMF Agents */
	MDS_DEST smfd_dest;			/* MDS DEST of SMFD */
} smfnd_cb_t;

extern smfnd_cb_t *smfnd_cb;

#ifdef __cplusplus
extern "C" {
#endif

/* smfnd_amf.c */
	extern uint32_t smfnd_amf_init(smfnd_cb_t * cb);

/* smfnd_mds.c */
	extern uint32_t smfnd_mds_init(smfnd_cb_t * cb);
	extern uint32_t smfnd_mds_finalize(smfnd_cb_t * cb);
	extern uint32_t smfnd_mds_change_role(smfnd_cb_t * cb);

/* smfnd_evt.c */
	extern void smfnd_process_mbx(SYSF_MBX *);

/* smfnd_main.c */
#if 0
	extern uint32_t smfnd_amf_disconnected(smfnd_cb_t * cb);
#endif
	extern void smfnd_msg_destroy(SMFSV_EVT * evt);

#ifdef __cplusplus
}
#endif
#endif  // SMF_SMFND_SMFND_H_
