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
#ifndef AVD_CLUSTER_H
#define AVD_CLUSTER_H

#include <saAmf.h>
#include <avd_node.h>

typedef struct avd_cluster_tag {
	SaNameT saAmfCluster;
	SaNameT saAmfClusterClmCluster;
	SaTimeT saAmfClusterStartupTimeout;
	SaAmfAdminStateT saAmfClusterAdminState;
} AVD_CLUSTER;

extern AVD_CLUSTER *avd_cluster;

extern SaAisErrorT avd_cluster_config_get(void);
extern void avd_cluster_tmr_init_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
extern void avd_cluster_constructor(void);

#endif
