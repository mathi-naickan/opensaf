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

  This file contains important header file includes to be used by rest of EDS.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

*******************************************************************************/
#ifndef EVT_EVTD_EDS_DL_API_H_
#define EVT_EVTD_EDS_DL_API_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"
#include "eds_cb.h"

uint32_t initialize_for_assignment(EDS_CB *cb, SaAmfHAStateT ha_state);
uint32_t ncs_edsv_eds_lib_req(NCS_LIB_REQ_INFO *);

#endif  // EVT_EVTD_EDS_DL_API_H_
