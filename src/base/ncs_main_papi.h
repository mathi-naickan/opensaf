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

  DESCRIPTION:  This file declares the main entry point into NCS.

  ******************************************************************************
  */

#ifndef BASE_NCS_MAIN_PAPI_H_
#define BASE_NCS_MAIN_PAPI_H_

#include "base/ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

/***********************************************************************\
   ncs_agents_startup: This function initializes all service agents for
                    a process. It attempts a symbol lookup on the
                    service-agent entry points. It starts all agents
                    that it can lookup this way.

                    At a minimum it starts the following services
                    - leap
                    - mds

                    The following services are started only if a symbol
                    look up on their entry-points is successful.
                    - ava
                    - gla
                    - mqa
                    (etc.)

\***********************************************************************/
unsigned int ncs_agents_startup(void);

/***********************************************************************\
   ncs_agents_shutdown: This function shutdown agents.
\***********************************************************************/
unsigned int ncs_agents_shutdown(void);

/***********************************************************************\
   Individual agents startup and shutdown functions
\***********************************************************************/
unsigned int ncs_core_agents_startup(void);
unsigned int ncs_mbca_startup(void);

unsigned int ncs_leap_startup(void);
unsigned int ncs_mds_startup(void);

unsigned int ncs_mbca_shutdown(void);
unsigned int ncs_core_agents_shutdown(void);

void ncs_mds_shutdown(void);
void ncs_leap_shutdown(void);


/***********************************************************************\
   m_NCS_GET_NODE_ID: This function returns a node-id (in the SAF sense).
                      Only the macro should be used, the function
                      prototyped below should not be used.

   Example usage :    NODE_ID node_id = m_NCS_GET_NODE_ID;
   Note : This Macro should be called only after calling the function
          ncs_agents_startup or function ncs_core_agents_startup only

\***********************************************************************/
NCS_NODE_ID ncs_get_node_id(void);
#define m_NCS_GET_NODE_ID ncs_get_node_id()

static inline NCS_CHASSIS_ID GetChassisIdFromNodeId(NCS_NODE_ID node_id) {
  uint32_t kChassisMask = 0xff;
  uint32_t chassis_id = (((uint32_t) node_id) >> 16) & kChassisMask;
  return chassis_id;
}

static inline SlotSubslotId GetSlotSubslotIdFromNodeId(NCS_NODE_ID node_id) {
  uint32_t kSubslotMask = 0xf;
  uint32_t kSlotMask = 0xff;
  uint32_t subslot_id = ((uint32_t) node_id) & kSubslotMask;
  uint32_t slot_id = (((uint32_t) node_id) >> 8) & kSlotMask;
  return ((subslot_id ^ kSubslotMask) << 8) | slot_id;
}

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_MAIN_PAPI_H_
