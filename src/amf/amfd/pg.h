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

  This file includes PG related declarations.
  
******************************************************************************
*/

#ifndef AMF_AMFD_PG_H_
#define AMF_AMFD_PG_H_

#include "amf/amfd/node.h"
#include "amf/amfd/csi.h"
#include "base/ncsdlib.h"
#include "amf/amfd/susi.h"

/* csi pg node wrapper structure (maintained by csi) */
typedef struct avd_pg_csi_node {
	NCS_DB_LINK_LIST_NODE csi_dll_node;	/* csi dll node (key is node ptr) */

	AVD_AVND *node;		/* ptr to the node */
} AVD_PG_CSI_NODE;

/* node pg csi wrapper structure (maintained by node) */
typedef struct avd_pg_node_csi {
	NCS_DB_LINK_LIST_NODE node_dll_node;	/* avnd dll node (key is csi ptr) */

	AVD_CSI *csi;		/* ptr to the csi */
} AVD_PG_NODE_CSI;

void avd_pg_trk_act_evh(AVD_CL_CB *, struct AVD_EVT *);

uint32_t avd_pg_susi_chg_prc(AVD_CL_CB *, AVD_SU_SI_REL *);
uint32_t avd_pg_compcsi_chg_prc(AVD_CL_CB *, struct avd_comp_csi_rel_tag *, bool);

uint32_t avd_pg_csi_node_add(AVD_CL_CB *, AVD_CSI *, AVD_AVND *);
void avd_pg_csi_node_del(AVD_CL_CB *, AVD_CSI *, AVD_AVND *);
void avd_pg_csi_node_del_all(AVD_CL_CB *, AVD_CSI *);
void avd_pg_node_csi_del_all(AVD_CL_CB *, AVD_AVND *);

#endif  // AMF_AMFD_PG_H_
