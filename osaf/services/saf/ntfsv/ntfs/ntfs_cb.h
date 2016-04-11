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

#ifndef NTFS_CB_H
#define NTFS_CB_H

#include <stdbool.h>
#include <saNtf.h>
#include <saClm.h>

/* Default HA state assigned locally during ntfs initialization */
#define NTFS_HA_INIT_STATE 0

/* CHECKPOINT status */
typedef enum checkpoint_status {
	COLD_SYNC_IDLE = 0,
	COLD_SYNC_COMPLETE,
} CHECKPOINT_STATE;

typedef struct {
	NCS_PATRICIA_NODE pat_node;
	uint32_t client_id;
	uint32_t client_id_net;
	MDS_DEST mds_dest;
} ntf_client_t;

typedef struct ntfs_cb {
	SYSF_MBX mbx;		/* NTFS's mailbox                             */
	MDS_HDL mds_hdl;	/* PWE Handle for interacting with NTFAs      */
	V_DEST_RL mds_role;	/* Current MDS role - ACTIVE/STANDBY         */
	MDS_DEST vaddr;		/* My identification in MDS                  */
	SaVersionT ntf_version;	/* The version currently supported           */
	SaNameT comp_name;	/* Components's name NTFS                     */
	SaAmfHandleT amf_hdl;	/* AMF handle, obtained thru AMF init        */
	SaInvocationT amf_invocation_id;	/* AMF InvocationID - needed to handle Quiesed state */
	bool is_quisced_set;
	SaSelectionObjectT amfSelectionObject;	/* Selection Object to wait for amf events */
	SaSelectionObjectT logSelectionObject;	/* Selection Object to wait for log events */
	SaSelectionObjectT clmSelectionObject;	/* Selection Object to wait for clms events */
	SaAmfHAStateT ha_state;	/* present AMF HA state of the component     */
	uint32_t async_upd_cnt;	/* Async Update Count for Warmsync */
	CHECKPOINT_STATE ckpt_state;	/* State if cold synched */
	NCS_MBCSV_HDL mbcsv_hdl;	/* Handle obtained during mbcsv init */
	SaSelectionObjectT mbcsv_sel_obj;	/* Selection object to wait for MBCSv events */
	NCS_MBCSV_CKPT_HDL mbcsv_ckpt_hdl;	/* MBCSv handle obtained during checkpoint open */
	EDU_HDL edu_hdl;	/* Handle from EDU for encode/decode operations */
	bool fully_initialized;
	unsigned int cache_size; /* size of the reader cache */
	bool nid_started;	/**< true if started by NID */
	SaClmHandleT clm_hdl;	/* CLM handle, obtained through CLM init        */
	NCS_SEL_OBJ usr2_sel_obj; /* Selection object for CLM initialization.*/
	uint16_t peer_mbcsv_version; /*Remeber peer NTFS MBCSV version.*/
	bool clm_initialized; //For CLM init status;
} ntfs_cb_t;

extern uint32_t ntfs_cb_init(ntfs_cb_t *);
extern void ntfs_process_mbx(SYSF_MBX *mbx);

#endif
