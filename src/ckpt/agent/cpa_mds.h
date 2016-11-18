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

 _Public_ CPA abstractions and function prototypes

*******************************************************************************/

/*
 * Module Inclusion Control...
 */

#ifndef CKPT_AGENT_CPA_MDS_H_
#define CKPT_AGENT_CPA_MDS_H_

uint32_t cpa_mds_callback(struct ncsmds_callback_info *info);
uint32_t cpa_mds_register(CPA_CB *cb);
void cpa_mds_unregister(CPA_CB *cb);
uint32_t cpa_mds_msg_sync_send(uint32_t cpa_mds_hdl,
				     MDS_DEST *destination, CPSV_EVT *i_evt, CPSV_EVT **o_evt, SaTimeT timeout);
uint32_t cpa_mds_msg_send(uint32_t cpa_mds_hdl, MDS_DEST *destination, CPSV_EVT *i_evt, uint32_t to_svc);

#endif  // CKPT_AGENT_CPA_MDS_H_
