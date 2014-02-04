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

#ifndef FM_CB_H
#define FM_CB_H

uint32_t gl_fm_hdl;

typedef enum {
	FM_TMR_TYPE_MIN,
	FM_TMR_PROMOTE_ACTIVE,
	FM_TMR_TYPE_MAX
} FM_TMR_TYPE;

typedef enum {
	FM_TMR_STATUS_MIN,
	FM_TMR_RUNNING,
	FM_TMR_STOPPED,
	FM_TMR_STATUS_MAX
} FM_TMR_STATUS;

typedef struct fm_tmr {
	tmr_t tmr_id;
	FM_TMR_TYPE type;
	FM_TMR_STATUS status;
} FM_TMR;

/* FM control block */
typedef struct fm_cb {

	uint32_t cb_hdl;
	SYSF_MBX mbx;

/* FM AMF CB */
	FM_AMF_CB fm_amf_cb;
	NODE_ID node_id;
	SaNameT node_name;

	SaNameT peer_node_name;
	NODE_ID peer_node_id;
	MDS_DEST peer_adest;	/* will be 0 if peer is not up */

/* Holds own role. */
	PCS_RDA_ROLE role;

/* AMF HA state for FM */
	SaAmfHAStateT amf_state;
	
/* MDS handles. */
	MDS_DEST adest;
	MDS_HDL adest_hdl;
	MDS_HDL adest_pwe1_hdl;

/* Timers */
	FM_TMR promote_active_tmr;

/* Time in terms of one hundredth of seconds (500 for 5 secs.) */
	uint32_t active_promote_tmr_val;
	bool csi_assigned;
/* Variable to indicate OpenSAF control of TIPC transport */
	bool control_tipc;
/* Booleans to mark service down events of critical Osaf Services */
	bool immd_down;
	bool immnd_down;
	bool amfnd_down;
	bool amfd_down;
} FM_CB;

extern char *role_string[];
extern FM_CB *fm_cb;

/*****************************************************************
*         Prototypes for extern functions                       *
*****************************************************************/
uint32_t fm_rda_init(FM_CB *);
uint32_t fm_rda_finalize(FM_CB *);
uint32_t fm_rda_set_role(FM_CB *, PCS_RDA_ROLE);

#endif
