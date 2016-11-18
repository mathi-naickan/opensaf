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
 * licensing terms.z
 *
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
..............................................................................

  $Header: 

..............................................................................

  DESCRIPTION:

 _Public_ IMMA abstractions and function prototypes

*******************************************************************************/

#ifndef IMM_AGENT_IMMA_MDS_H_
#define IMM_AGENT_IMMA_MDS_H_

uint32_t imma_mds_callback(struct ncsmds_callback_info *info);
uint32_t imma_mds_register(IMMA_CB *cb);
void imma_mds_unregister(IMMA_CB *cb);
uint32_t imma_mds_callback(struct ncsmds_callback_info *info);
/* uint32_t imma_mds_msg_sync_send (NCSCONTEXT imma_mds_hdl, */
uint32_t imma_mds_msg_sync_send(uint32_t imma_mds_hdl,
				      MDS_DEST *destination, IMMSV_EVT *i_evt, IMMSV_EVT **o_evt, SaTimeT timeout);
/* uint32_t imma_mds_msg_send (NCSCONTEXT imma_mds_hdl,  */
uint32_t imma_mds_msg_send(uint32_t imma_mds_hdl, MDS_DEST *destination, IMMSV_EVT *i_evt, uint32_t to_svc);

#endif  // IMM_AGENT_IMMA_MDS_H_
