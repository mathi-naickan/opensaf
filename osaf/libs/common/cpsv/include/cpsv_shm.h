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

#ifndef CPSV_SHM_H
#define CPSV_SHM_H

#define MAX_CLIENTS 1000
#define MAX_CKPTS  2000
#define MAX_SIZE  30
#define CPND_ERR -2
#define SHM_NEXT -3
#define SHM_INIT -1

#define CPSV_CPND_SHM_VERSION    1

typedef struct cpsv_ckpt_hdr {
	SaCkptCheckpointHandleT ckpt_id;	/* Index for identifying the checkpoint */
	SaNameT ckpt_name;
	SaCkptCheckpointCreationAttributesT create_attrib;
	SaCkptCheckpointOpenFlagsT open_flags;
	uint32_t ckpt_lcl_ref_cnt;
	bool is_unlink;
	bool is_close;
	SaUint32T n_secs;
	SaUint32T mem_used;
	bool cpnd_rep_create;
	bool is_active_exist;
	MDS_DEST active_mds_dest;
	uint32_t cpnd_lcl_wr;
	uint32_t cpnd_oth_state;
} CPSV_CKPT_HDR;

typedef struct cpsv_sect_hdr {
	uint32_t lcl_sec_id;
	uint16_t idLen;
	uint8_t id[MAX_SIZE];
	SaCkptSectionStateT sec_state;
	SaSizeT sec_size;
	SaTimeT exp_tmr;
	SaTimeT lastUpdate;
} CPSV_SECT_HDR;

typedef struct ckpt_info {
	SaNameT ckpt_name;
	SaCkptCheckpointHandleT ckpt_id;
	uint32_t maxSections;
	SaSizeT maxSecSize;
	NODE_ID node_id;
	int32_t offset;
	uint32_t client_bitmap;
	int32_t is_valid;
	uint32_t bm_offset;
	bool is_unlink;
	bool is_close;
	bool cpnd_rep_create;
	bool is_first;
	SaTimeT close_time;
	int32_t next;
} CKPT_INFO;

typedef struct client_info {
	SaCkptHandleT ckpt_app_hdl;
	uint32_t ckpt_open_ref_cnt;
	uint32_t open_reader_flags_cnt;
	uint32_t open_writer_flags_cnt;
	MDS_DEST agent_mds_dest;
	SaVersionT version;
	uint16_t cbk_reg_info;
	bool is_valid;
	uint32_t offset;
	bool arr_flag;
} CLIENT_INFO;

typedef struct gbl_shm_ptr {
	void *base_addr;
	void *cli_addr;
	void *ckpt_addr;
	int32_t n_clients;
	int32_t n_ckpts;
} GBL_SHM_PTR;

typedef struct cpnd_shm_version {
	uint16_t shm_version;	/* Added to provide support for SAF Inservice upgrade facilty */
	uint16_t dummy_version1;	/* Not in use */
	uint16_t dummy_version2;	/* Not in use */
	uint16_t dummy_version3;	/* Not in use */
} CPND_SHM_VERSION;

typedef struct client_hdr {
	int32_t num_clients;
} CLIENT_HDR;

typedef struct ckpt_hdr {
	int32_t num_ckpts;
} CKPT_HDR;

typedef enum cpnd_type_info {
	CPND_BASE,
	CPND_CLIENT_INFO,
	CPND_CKPT_INFO
} CPND_TYPE_INFO;

#endif
