/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

#ifndef CLM_CLMD_CLMS_NTF_H_
#define CLM_CLMD_CLMS_NTF_H_
extern void clms_node_reconfigured_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node);
extern void clms_node_admin_state_change_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node, SaUint32T newState);
extern SaAisErrorT clms_ntf_init(CLMS_CB * cb);
extern void clms_node_exit_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node);
extern void clms_node_join_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node);

#endif  // CLM_CLMD_CLMS_NTF_H_
