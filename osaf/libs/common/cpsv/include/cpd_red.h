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

#ifndef CPD_RED_H
#define CPD_RED_H

typedef enum cpd_mbcsv_msg_type {
	CPD_A2S_MSG_BASE = 1,
	CPD_A2S_MSG_CKPT_CREATE = CPD_A2S_MSG_BASE,
	CPD_A2S_MSG_CKPT_DEST_ADD,
	CPD_A2S_MSG_CKPT_DEST_DEL,
	CPD_A2S_MSG_CKPT_RDSET,
	CPD_A2S_MSG_CKPT_AREP_SET,
	CPD_A2S_MSG_CKPT_UNLINK,
	CPD_A2S_MSG_CKPT_USR_INFO,
	CPD_A2S_MSG_CKPT_DEST_DOWN,
	CPD_A2S_MSG_MAX_EVT
} CPD_MBCSV_MSG_TYPE;

typedef struct cpd_a2s_ckpt_create {
	SaNameT ckpt_name;
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointCreationAttributesT ckpt_attrib;
	bool is_unlink_set;
	bool is_active_exists;
	MDS_DEST active_dest;
	SaTimeT create_time;
	uint32_t num_users;
	uint32_t num_writers;
	uint32_t num_readers;
	uint32_t num_sections;
	uint32_t dest_cnt;
	CPSV_CPND_DEST_INFO *dest_list;
} CPD_A2S_CKPT_CREATE;

typedef struct cpd_a2s_ckpt_unlink {
	SaNameT ckpt_name;
	bool is_unlink_set;

} CPD_A2S_CKPT_UNLINK;

typedef struct cpd_a2s_ckpt_usr_info {
	SaCkptCheckpointHandleT ckpt_id;
	uint32_t num_user;
	uint32_t num_writer;
	uint32_t num_reader;
	uint32_t num_sections;
	uint32_t ckpt_on_scxb1;
	uint32_t ckpt_on_scxb2;

} CPD_A2S_CKPT_USR_INFO;

typedef struct cpd_a2s_ckpt_usr_info_2 {
	SaCkptCheckpointHandleT ckpt_id;
	uint32_t num_user;
	uint32_t num_writer;
	uint32_t num_reader;
	uint32_t num_sections;
	uint32_t ckpt_on_scxb1;
	uint32_t ckpt_on_scxb2;
	uint32_t node_users_cnt;
	CPD_NODE_USER_INFO *node_list;
} CPD_A2S_CKPT_USR_INFO_2;

typedef struct cpd_mbcsv_msg {
	CPD_MBCSV_MSG_TYPE type;
	union {
		/* Messages for async updates  */
		CPD_A2S_CKPT_CREATE ckpt_create;
		CPSV_CKPT_DEST_INFO dest_add;
		CPSV_CKPT_DEST_INFO dest_del;
		CPSV_CKPT_RDSET rd_set;
		CPSV_CKPT_DEST_INFO arep_set;
		CPD_A2S_CKPT_UNLINK ckpt_ulink;
		CPD_A2S_CKPT_USR_INFO usr_info;
		CPSV_CKPT_DEST_INFO dest_down;
		CPD_A2S_CKPT_USR_INFO_2 usr_info_2;
	} info;
} CPD_MBCSV_MSG;

#endif
