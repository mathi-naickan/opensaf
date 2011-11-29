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

  FILE NAME: mqd_clm.h

..............................................................................

  DESCRIPTION:

  Contains the declarations and prototypes of the CLM callback functions

******************************************************************************/
#ifndef MQD_CLM_H
#define MQD_CLM_H

#include <saClm.h>

EXTERN_C void mqd_clm_cluster_track_callback(const SaClmClusterNotificationBufferT *notificationBuffer,
					     SaUint32T numberOfMembers, SaAisErrorT error);
EXTERN_C void mqd_del_node_down_info(MQD_CB *pMqd, NODE_ID nodeid);

#endif   /* MQD_CLM_H */
