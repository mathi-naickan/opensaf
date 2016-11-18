/*      OpenSAF
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
 * Author(s): Ericsson AB
 *
 */

#ifndef SMF_SMFD_SMFD_SMFND_H_
#define SMF_SMFD_SMFD_SMFND_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "base/ncssysf_ipc.h"
#include "base/ncsgl_defs.h"
#include "clm/saf/saClm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

typedef enum SmfNdStateT {
        ndUp,
        ndDown
} SmfNdStateT;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

typedef struct SmfndNodeDest {
	MDS_DEST dest;
        MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver;
        uint32_t nd_up_cntr;
} SmfndNodeDest;

typedef struct SmfndNodeT {
	struct SmfndNodeT *next;
	SaClmClusterNodeT clmInfo;
	MDS_DEST dest;
        MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver;
        SmfNdStateT nd_state;
        uint32_t nd_up_cntr;
} SmfndNodeT;

typedef struct smfd_smfnd_adest_invid_map{
        SaInvocationT                           inv_id;
        uint32_t                                   no_of_cbks;
	SYSF_MBX				*cbk_mbx;
        struct smfd_smfnd_adest_invid_map       *next_invid;
}SMFD_SMFND_ADEST_INVID_MAP;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

	uint32_t smfnd_up(SaClmNodeIdT node_id, MDS_DEST smfnd_dest, MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver);
	uint32_t smfnd_down(SaClmNodeIdT node_id);
	bool smfnd_for_name(const char *i_nodeName, SmfndNodeDest* o_nodeDest);
	uint32_t smfnd_exec_remote_cmd(const char *i_cmd, const SmfndNodeDest* i_smfnd,
                                       uint32_t i_timeout, uint32_t i_localTimeout);

#ifdef __cplusplus
}
#endif
#endif  // SMF_SMFD_SMFD_SMFND_H_
