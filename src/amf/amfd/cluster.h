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
/****************************************************************************

  DESCRIPTION:

  This module is the include file for Sa Amf cluster object management.
  
****************************************************************************/
#ifndef AMF_AMFD_CLUSTER_H_
#define AMF_AMFD_CLUSTER_H_

#include "amf/saf/saAmf.h"
#include "amf/amfd/node.h"

typedef struct avd_cluster_tag {
	std::string saAmfCluster;
	std::string saAmfClusterClmCluster;
	SaTimeT saAmfClusterStartupTimeout;
	SaAmfAdminStateT saAmfClusterAdminState;
} AVD_CLUSTER;

extern AVD_CLUSTER *avd_cluster;

extern SaAisErrorT avd_cluster_config_get(void);
extern void avd_cluster_tmr_init_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
extern void avd_node_sync_tmr_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
extern void avd_cluster_constructor(void);

#endif  // AMF_AMFD_CLUSTER_H_
