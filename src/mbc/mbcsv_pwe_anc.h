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




   REVISION HISTORY:

   DESCRIPTION:

   This module contains the MBCSv MBX list structure and the corresponding functions.

*******************************************************************************/

/*
 * Module Inclusion Control...
 */
#ifndef MBC_MBCSV_PWE_ANC_H_
#define MBC_MBCSV_PWE_ANC_H_

#include "mbcsv.h"

/*
 * Peer list used for storing all the peers of this PWE. This is used
 * for brodcasting the message to all the peers.
 */
typedef struct {
  uint32_t pwe_hdl;               /* Handle supplied by application with OPEN call */
  MBCSV_ANCHOR anchor;
} MBCSV_PEER_KEY;

/*
 * Prototypes of PWE anchor.
 */
uint32_t mbcsv_destroy_peer_list(void);
uint32_t mbcsv_add_new_pwe_anc(uint32_t pwe_hdl, MBCSV_ANCHOR anchor);
uint32_t mbcsv_rmv_pwe_anc_entry(uint32_t pwe_hdl, MBCSV_ANCHOR anchor);
uint32_t mbcsv_get_next_anchor_for_pwe(uint32_t pwe_hdl, MBCSV_ANCHOR *anchor);
uint32_t mbcsv_send_brodcast_msg(uint32_t pwe_hdl, MBCSV_EVT *msg, CKPT_INST *ckpt);
uint32_t mbcsv_rmv_ancs_for_pwe(uint32_t pwe_hdl);
uint32_t mbcsv_initialize_peer_list(void);

#endif  // MBC_MBCSV_PWE_ANC_H_
