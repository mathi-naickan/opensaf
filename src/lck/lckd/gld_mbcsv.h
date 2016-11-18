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

#ifndef LCK_LCKD_GLD_MBCSV_H_
#define LCK_LCKD_GLD_MBCSV_H_

#include "lck/lckd/gld_evt.h"
#include "gld_cb.h"

#define MAX_NO_OF_RSC_INFO_RECORDS  10

#define GLSV_GLD_MBCSV_VERSION    1
#define GLSV_GLD_MBCSV_VERSION_MIN 1

/*****************************************************************************
 * The messages used by GLD
 *****************************************************************************/

typedef struct glsv_a2s_node_list_tag {
	MDS_DEST dest_id;
	uint32_t node_id;
	uint32_t status;
	struct glsv_a2s_node_list_tag *next;
} GLSV_A2S_NODE_LIST;

typedef struct glsv_gld_a2s_rsc_details {
	SaNameT resource_name;	/* Cluster-wide unique lock name             */
	SaLckResourceIdT rsc_id;	/* unique resource id - Index for            */
	bool can_orphan;	/* is this resource allocated in orphan mode */
	SaLckLockModeT orphan_lck_mode;	/* related to orphan mode */
	GLSV_A2S_NODE_LIST *node_list;	/* Nodes on which this resource is reffered  */
} GLSV_GLD_A2S_RSC_DETAILS;

typedef struct glsv_a2s_rsc_open_info {
	SaNameT rsc_name;
	SaLckResourceIdT rsc_id;
	MDS_DEST mdest_id;
	SaTimeT rsc_creation_time;
} GLSV_A2S_RSC_OPEN_INFO;

typedef struct glsv_a2s_rsc_details {
	SaLckResourceIdT rsc_id;
	bool orphan;
	SaLckLockModeT lck_mode;
	MDS_DEST mdest_id;
	uint32_t lcl_ref_cnt;
} GLSV_A2S_RSC_DETAILS;

typedef struct glsv_a2s_glnd_mds_info_tag {
	MDS_DEST mdest_id;
} GLSV_A2S_GLND_MDS_INFO;

typedef struct glsv_gld_a2s_ckpt_evt_tag {

	GLSV_GLD_EVT_TYPE evt_type;
	union {
		GLSV_A2S_RSC_OPEN_INFO rsc_open_info;
		GLSV_A2S_RSC_DETAILS rsc_details;
		GLSV_A2S_GLND_MDS_INFO glnd_mds_info;
	} info;
} GLSV_GLD_A2S_CKPT_EVT;

/* This is the function prototype for event handling */
typedef uint32_t (*GLSV_GLD_A2S_EVT_HANDLER) (struct glsv_gld_a2s_ckpt_evt_tag * evt);

uint32_t gld_process_standby_evt(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_CKPT_EVT *evt);

uint32_t gld_sb_proc_data_rsp(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_RSC_DETAILS *rsc_details);

GLSV_GLD_GLND_DETAILS *gld_add_glnd_node(GLSV_GLD_CB *gld_cb, MDS_DEST glnd_mds_dest);
void glsv_gld_a2s_ckpt_resource(GLSV_GLD_CB *gld_cb, SaNameT *rsc_name, SaLckResourceIdT rsc_id,
					 MDS_DEST mdest_id, SaTimeT creation_time);

void glsv_gld_a2s_ckpt_node_details(GLSV_GLD_CB *gld_cb, MDS_DEST mdest_id, uint32_t evt_type);
void glsv_gld_a2s_ckpt_rsc_details(GLSV_GLD_CB *gld_cb, GLSV_GLD_EVT_TYPE evt_type,
					    GLSV_RSC_DETAILS rsc_details, MDS_DEST mdest_id, uint32_t lcl_ref_cnt);
uint32_t glsv_gld_mbcsv_async_update(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_CKPT_EVT *a2s_evt);
uint32_t glsv_gld_mbcsv_chgrole(GLSV_GLD_CB *gld_cb);

#endif  // LCK_LCKD_GLD_MBCSV_H_
