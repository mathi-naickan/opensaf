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
  FILE NAME: cpnd_res.c

  DESCRIPTION: CPND Redundancy Processing Routines

******************************************************************************/

#include "cpnd.h"

#define m_CPND_CKPT_HDR_UPDATE(ckpt_hdr,addr,offset)  memcpy(&ckpt_hdr,addr+offset,sizeof(CPSV_CKPT_HDR))

#define m_CPND_SEC_HDR_UPDATE(sect_hdr,addr,offset)  memcpy(&sect_hdr,addr+offset,sizeof(CPSV_SECT_HDR))

#define m_CPND_CLIHDR_INFO_READ(cli_hdr,addr,offset)  memcpy(&cli_hdr,addr+offset,sizeof(CLIENT_HDR))

#define m_CPND_CLIHDR_INFO_WRITE(addr,cli_hdr,offset)  memcpy(addr+offset,&cli_hdr,sizeof(CLIENT_HDR))

#define m_CPND_CKPTHDR_READ(ckpt_hdr,addr,offset)  memcpy(&ckpt_hdr,addr+offset,sizeof(CKPT_HDR))

#define m_CPND_CLINFO_READ(cli_info,addr,offset)  memcpy(&cli_info,addr+offset,sizeof(CLIENT_INFO))

#define m_CPND_CLINFO_UPDATE(addr,cli_info,offset) memcpy(addr+offset,&cli_info,sizeof(CLIENT_INFO))

#define m_CPND_CKPTINFO_READ(ckpt_info,addr,offset) memcpy(&ckpt_info,addr+offset,sizeof(CKPT_INFO))

#define m_CPND_CKPTINFO_UPDATE(addr,ckpt_info,offset) memcpy(addr+offset,&ckpt_info,sizeof(CKPT_INFO))

#define m_CPND_CKPTHDR_UPDATE(ckpt_hdr,offset)  memcpy(offset,&ckpt_hdr,sizeof(CKPT_HDR))

static uint32_t cpnd_res_ckpt_sec_add(CPND_CKPT_SECTION_INFO *pSecPtr, CPND_CKPT_NODE *cp_node);
static bool cpnd_find_exact_ckptinfo(CPND_CB *cb, CKPT_INFO *ckpt_info, uint32_t bitmap_offset,
					 uint32_t *offset, uint32_t *prev_offset);
static void cpnd_clear_ckpt_info(CPND_CB *cb, CPND_CKPT_NODE *cp_node, uint32_t curr_offset, uint32_t prev_offset);

/******************************************************************************* *
 * Name           : cpnd_client_extract_bits
 *
 * Description    : To extract the bit set in the client bitmap value 
 *
 * Arguments      : bitmap_value
 *
 * Return Values  : bit set position
 * Notes          : None
*********************************************************************************/

uint32_t cpnd_client_extract_bits(uint32_t bitmap_value, uint32_t *bit_position)
{
	uint32_t counter = *bit_position;
	uint32_t mask = 0x1;
	uint32_t nbits = 8 * sizeof(int32_t);

	mask = mask << counter;
	do {
		if (bitmap_value & mask) {
			counter++;
			break;
		}
		mask = 0x1;
		counter++;
		mask = mask << counter;
	} while (counter <= (nbits - 1));
	*bit_position = counter;
	return counter - 1;
}

/******************************************************************************************
 * Name            : cpnd_res_ckpt_sec_add
 *
 * Description     : Add the section info to the ckpt replica 
 *
 * Arguments       : CPND_CKPT_SECTION_INFO - section info , CPND_CKPT_NODE - cpnd node
 *
 * Return Values   : Success / Error
 * Notes           : 
*********************************************************************************************/

uint32_t cpnd_res_ckpt_sec_add(CPND_CKPT_SECTION_INFO *pSecPtr, CPND_CKPT_NODE *cp_node)
{
	if (cp_node->replica_info.section_info != NULL) {
		pSecPtr->next = cp_node->replica_info.section_info;
		cp_node->replica_info.section_info->prev = pSecPtr;
		cp_node->replica_info.section_info = pSecPtr;
	} else {
		cp_node->replica_info.section_info = pSecPtr;
	}
	return NCSCC_RC_SUCCESS;

}

/******************************************************************************************
 * Name            : cpnd_res_ckpt_sec_del
 *
 * Description     : Delete the section info from the ckpt replica
 *
 * Arguments       : CPND_CKPT_NODE - cpnd node
 *
 * Return Values   : Success / Error
 * Notes           :
*********************************************************************************************/
uint32_t cpnd_res_ckpt_sec_del(CPND_CKPT_NODE *cp_node)
{
	CPND_CKPT_SECTION_INFO *pSecPtr = NULL, *nextPtr = NULL;

	pSecPtr = cp_node->replica_info.section_info;
	while (pSecPtr != NULL) {
		nextPtr = pSecPtr;
		pSecPtr = pSecPtr->next;
		if ((nextPtr->sec_id.id != NULL) && (nextPtr->sec_id.idLen != 0)) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(nextPtr->sec_id.id, NCS_SERVICE_ID_CPND);
		}
		m_CPND_FREE_CKPT_SECTION(nextPtr);
	}
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************************************************
 * Name           : cpnd_ckpt_replica_create_res
 *
 * Description    : To read the data from the checkpoint replica shared memory and fill up the data structures
 *
 * Arguments      : NCS_OS_POSIX_SHM_REQ_INFO *open_req -  Shared Memory Request Info pointer
 *                  uint8_t* buf  - Name of the shared memory
 *                  CPND_CKPT_NODE *cp_node - CPND_CKPT_NODE pointer
 *                  ref_cnt
 *
 * Return Values  :
 * Notes          : None
**************************************************************************************************************/

/*
 |-----------|--------- |---------- |--------|----------|-------------|----------|---------|
 | CKPT_HDR  | SEC_HDR  | SEC_INFO  |SEC_HDR | SEC_INFO |.............|  SEC_HDR |SEC_INFO |
 |           |          |           |        |          |             |          |         |
 |-----------|----------|-----------|--------|----------|------------ |----------|---------|        
*/

uint32_t cpnd_ckpt_replica_create_res(NCS_OS_POSIX_SHM_REQ_INFO *open_req, char *buf, CPND_CKPT_NODE **cp_node,
				   uint32_t ref_cnt, CKPT_INFO *cp_info)
{
/*   NCS_OS_POSIX_SHM_REQ_INFO read_req,shm_read; */
	CPSV_CKPT_HDR ckpt_hdr;
	CPSV_SECT_HDR sect_hdr;
	uint32_t counter = 0, sec_cnt = 0, rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_SECTION_INFO *pSecPtr = NULL;
	NCS_OS_POSIX_SHM_REQ_INFO read_req;

	TRACE_ENTER();

	memset(&ckpt_hdr, '\0', sizeof(CPSV_CKPT_HDR));
	open_req->type = NCS_OS_POSIX_SHM_REQ_OPEN;
	open_req->info.open.i_size =
	    sizeof(CPSV_CKPT_HDR) + (cp_info->maxSections * ((sizeof(CPSV_SECT_HDR) + cp_info->maxSecSize)));
	open_req->info.open.i_offset = 0;
	open_req->info.open.i_name = buf;
	open_req->info.open.i_map_flags = MAP_SHARED;
	open_req->info.open.o_addr = NULL;
	open_req->info.open.i_flags = O_RDWR;
	rc = ncs_os_posix_shm(open_req);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd shm open request failed %s",buf);
		/*   assert(0); */
		return rc;
	}

	m_CPND_CKPT_HDR_UPDATE(ckpt_hdr, (char *)open_req->info.open.o_addr, 0);
	(*cp_node)->create_attrib = ckpt_hdr.create_attrib;
	(*cp_node)->open_flags = ckpt_hdr.open_flags;
	(*cp_node)->is_active_exist = ckpt_hdr.is_active_exist;
	(*cp_node)->active_mds_dest = ckpt_hdr.active_mds_dest;
	(*cp_node)->ckpt_lcl_ref_cnt = ref_cnt;
	(*cp_node)->replica_info.n_secs = ckpt_hdr.n_secs;
	(*cp_node)->cpnd_rep_create = ckpt_hdr.cpnd_rep_create;
	(*cp_node)->replica_info.open = *open_req;

	if ((*cp_node)->create_attrib.maxSections == 0)
		return rc;

	(*cp_node)->replica_info.shm_sec_mapping =
	    (uint32_t *)m_MMGR_ALLOC_CPND_DEFAULT(sizeof(uint32_t) * ((*cp_node)->create_attrib.maxSections));

	if ((*cp_node)->replica_info.shm_sec_mapping == NULL) {
		LOG_ER("cpnd default memory alloc failed");
		/*  assert(0); */
		return NCSCC_RC_FAILURE;
	}

	/* The below for loop  indicates all are free */
	for (; sec_cnt < (*cp_node)->create_attrib.maxSections; sec_cnt++)
		(*cp_node)->replica_info.shm_sec_mapping[sec_cnt] = 1;
	sec_cnt = 0;

	while (counter < ckpt_hdr.n_secs) {
		memset(&read_req, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));
		memset(&sect_hdr, '\0', sizeof(CPSV_SECT_HDR));
		read_req.type = NCS_OS_POSIX_SHM_REQ_READ;
		read_req.info.read.i_addr = (void *)((char *)open_req->info.open.o_addr + sizeof(CPSV_CKPT_HDR));
		read_req.info.read.i_read_size = sizeof(CPSV_SECT_HDR);
		read_req.info.read.i_offset =
		    counter * (sizeof(CPSV_SECT_HDR) + (*cp_node)->create_attrib.maxSectionSize);
		read_req.info.read.i_to_buff = (CPSV_SECT_HDR *)&sect_hdr;
		rc = ncs_os_posix_shm(&read_req);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("cpnd sect HDR read failed");
			/*   assert(0); */
			return rc;
		}

		/*  macro for reading the section header information  */
		/*    offset = counter * (sizeof(CPSV_SECT_HDR)+(*cp_node)->create_attrib.maxSectionSize);
		   m_CPND_SEC_HDR_UPDATE(sect_hdr,open_req->info.open.o_addr+sizeof(CPSV_CKPT_HDR),offset); */
		(*cp_node)->replica_info.shm_sec_mapping[sec_cnt] = 0;
		sec_cnt++;
		counter++;
		pSecPtr = m_MMGR_ALLOC_CPND_CKPT_SECTION_INFO;
		if (pSecPtr == NULL) {
			TRACE_4("cpnd ckpt section info memory allocation failed");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}

		memset(pSecPtr, '\0', sizeof(CPND_CKPT_SECTION_INFO));
		pSecPtr->lcl_sec_id = sect_hdr.lcl_sec_id;
		pSecPtr->sec_id.idLen = sect_hdr.idLen;
		if (pSecPtr->sec_id.idLen != 0) {
			pSecPtr->sec_id.id = m_MMGR_ALLOC_CPND_DEFAULT(pSecPtr->sec_id.idLen);
			if (pSecPtr->sec_id.id == NULL) {
				TRACE_4("cpnd default allocation failed for sec_id length");
				rc = NCSCC_RC_FAILURE;
				goto end;
			}
		}

		memcpy(pSecPtr->sec_id.id, sect_hdr.id, sect_hdr.idLen);
		pSecPtr->sec_state = sect_hdr.sec_state;
		pSecPtr->sec_size = sect_hdr.sec_size;
		pSecPtr->exp_tmr = sect_hdr.exp_tmr;
		pSecPtr->lastUpdate = sect_hdr.lastUpdate;

		cpnd_res_ckpt_sec_add(pSecPtr, *cp_node);
		memset(&sect_hdr, '\0', sizeof(CPSV_SECT_HDR));

		(*cp_node)->replica_info.mem_used += pSecPtr->sec_size;

	}
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;

 end:
	if ((*cp_node)->replica_info.shm_sec_mapping != NULL)
		m_MMGR_FREE_CPND_DEFAULT((*cp_node)->replica_info.shm_sec_mapping);
	cpnd_res_ckpt_sec_del(*cp_node);
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;
}

void cpnd_restart_update_timer(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaTimeT closetime)
{
	CKPT_INFO ckpt_info;

	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
	if (cp_node->offset >= 0) {
		m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				     cp_node->offset * sizeof(CKPT_INFO));
		ckpt_info.close_time = closetime;
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				       ckpt_info, cp_node->offset * sizeof(CKPT_INFO));
	}
	return;
}

/****************************************************************************************************
 * Name           : cpnd_restart_shm_create
 *
 * Description    : To create the shared memory for CPND restart
 *
 * Arguments      : NCS_OS_POSIX_SHM_REQ_INFO *open_req - Shared Memory Request Info pointer
 *                  CPND_CB *cb  - CPND CB pointer
 *
 * Return Values  : void * - Returns the starting address of the shared memory
 * Notes          : If the shared memory is present - CPND has restarted , so CPND will update its database by 
                    reading the information from the shared memory
                    If the shared memory is not present - CPND is coming up for the first time , so create a new 
                    shared memory and update the shared memory as and when the database gets updated
 * TBD            : TO CHECK THE ERROR CONDITIONS
****************************************************************************************************/

void *cpnd_restart_shm_create(NCS_OS_POSIX_SHM_REQ_INFO *cpnd_open_req, CPND_CB *cb, SaClmNodeIdT nodeid)
{
	uint32_t counter = 0, count, num_bitset = 0, n_clients, rc = NCSCC_RC_SUCCESS, i_offset, bit_position;
	int32_t next_offset;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	CLIENT_INFO cl_info;
	CLIENT_HDR cli_hdr;
	CKPT_INFO cp_info, tmp_cp_info;
	SaCkptHandleT client_hdl;
	char *buf = NULL, *buffer = NULL;
	uint8_t size = 0, total_length;
	GBL_SHM_PTR gbl_shm_addr = {0, 0, 0, 0, 0};
	memset(&cp_info, '\0', sizeof(CKPT_INFO));
	NCS_OS_POSIX_SHM_REQ_INFO ckpt_rep_open;
	SaTimeT presentTime, timeout = 0;
	int64_t now, diff_time, giga_sec;
	uint32_t max_client_hdl = 0;
	SaTimeT tmpTime = 0;
	CPND_SHM_VERSION cpnd_shm_version;

	TRACE_ENTER();
	/* Initializing shared memory version */
	memset(&cpnd_shm_version, '\0', sizeof(cpnd_shm_version));
	cpnd_shm_version.shm_version = CPSV_CPND_SHM_VERSION;

	size = strlen("CPND_CHECKPOINT_INFO");
	total_length = size + sizeof(nodeid) + 5;
	buffer = m_MMGR_ALLOC_CPND_DEFAULT(total_length);
	if (buffer == NULL) {
		LOG_ER("cpnd default memory allocation failed in cpnd_open in resart shm create");
		return NULL;
	}
	cb->cpnd_res_shm_name = (uint8_t*)buffer;
	memset(buffer, '\0', total_length);
	strncpy(buffer, "CPND_CHECKPOINT_INFO", total_length);
	sprintf(buffer + size, "_%d", (uint32_t)nodeid);

	/* 1. FIRST TRYING TO OPEN IN RDWR MODE */
	cpnd_open_req->type = NCS_OS_POSIX_SHM_REQ_OPEN;
	cpnd_open_req->info.open.i_size =
	    sizeof(CLIENT_HDR) + (MAX_CLIENTS * sizeof(CLIENT_INFO)) + sizeof(CKPT_HDR) +
	    (MAX_CKPTS * sizeof(CKPT_INFO));
	cpnd_open_req->info.open.i_offset = 0;
	cpnd_open_req->info.open.i_name = buffer;
	cpnd_open_req->info.open.i_map_flags = MAP_SHARED;
	cpnd_open_req->info.open.o_addr = NULL;
	cpnd_open_req->info.open.i_flags = O_RDWR;

	rc = ncs_os_posix_shm(cpnd_open_req);

	if (rc == NCSCC_RC_FAILURE) {	/* INITIALLY IT FAILS SO CREATE A SHARED MEMORY */
		TRACE_1("cpnd comming up first time");
		cpnd_open_req->info.open.i_flags = O_CREAT | O_RDWR;
		rc = ncs_os_posix_shm(cpnd_open_req);
		if (NCSCC_RC_FAILURE == rc) {
			LOG_ER("cpnd open request fail for RDWR mode %s",buf);
			m_MMGR_FREE_CPND_DEFAULT(buffer);
			return NULL;
		}
		cb->cpnd_first_time = true;

		memset(cpnd_open_req->info.open.o_addr, 0,
		       sizeof(CLIENT_HDR) + (MAX_CLIENTS * sizeof(CLIENT_INFO)) + sizeof(CKPT_HDR) +
		       (MAX_CKPTS * sizeof(CKPT_INFO)));
		TRACE_1("cpnd new shm create request success");
		return cpnd_open_req->info.open.o_addr;
	}

	/*

	   |- ------- ---  |--------------------------------|------------ |------------------|
	   |               |                                |             |                  |
	   | CLIENT_HDR    |  CLIENT_INFO                   |  CKPT_HDR   |  CKPT_INFO       |
	   |No. of clients |                                |             |                  |
	   --------------------------------------------------------------------------------    
	 */

	/* Already the shared memory exists */
	else {
		TRACE_1("cpnd restart already shared memory exits");
		gbl_shm_addr.cli_addr = cpnd_open_req->info.open.o_addr + sizeof(cpnd_shm_version);	/* Starting address of the shared memory */
		gbl_shm_addr.ckpt_addr = (void *)((char *)gbl_shm_addr.cli_addr + sizeof(CLIENT_HDR) +
						  (MAX_CLIENTS * sizeof(CLIENT_INFO)));
		cb->shm_addr = gbl_shm_addr;

		/* READ FROM THE SHARED MEMORY */

		TRACE("CPND IS RESTARTING ");
		/* Read the number of clients from the header */
		memset(&cli_hdr, '\0', sizeof(CLIENT_HDR));
		m_CPND_CLIHDR_INFO_READ(cli_hdr, (char *)gbl_shm_addr.cli_addr, 0);

		n_clients = cli_hdr.num_clients;
		TRACE_1("cpnd num clients read ");
		/* ( DO - WHILE )-  READ THE CLIENT INFO AND FILL THE DATABASE OF CLIENT INFO */
		if (n_clients != 0) {
			while (counter < MAX_CLIENTS) {
				memset(&cl_info, '\0', sizeof(CLIENT_INFO));
				i_offset = counter * sizeof(CLIENT_INFO);
				m_CPND_CLINFO_READ(cl_info, (char *)gbl_shm_addr.cli_addr + sizeof(CLIENT_HDR),
						   i_offset);

				if (cl_info.ckpt_app_hdl == 0) {
					counter++;
					continue;
				}

				cl_node = m_MMGR_ALLOC_CPND_CKPT_CLIENT_NODE;
				if (cl_node == NULL) {
					LOG_ER("cpnd ckpt client node memory alloc failed ");
					rc = SA_AIS_ERR_NO_MEMORY;
					goto memfail;
				}
				memset(cl_node, '\0', sizeof(CPND_CKPT_CLIENT_NODE));
				cl_node->ckpt_app_hdl = cl_info.ckpt_app_hdl;
				cl_node->ckpt_open_ref_cnt = cl_info.ckpt_open_ref_cnt;
				cl_node->open_reader_flags_cnt = cl_info.open_reader_flags_cnt;
				cl_node->open_writer_flags_cnt = cl_info.open_writer_flags_cnt;
				cl_node->agent_mds_dest = cl_info.agent_mds_dest;
				cl_node->offset = cl_info.offset;
				cl_node->version = cl_info.version;
				cl_node->arrival_cb_flag = cl_info.arr_flag;
				cl_node->ckpt_list = NULL;

				if (cpnd_client_node_add(cb, cl_node) != NCSCC_RC_SUCCESS) {
					TRACE_4("cpnd client nonde tree add failed cpkpt_app_hdl %llx ",cl_node->ckpt_app_hdl);
					rc = SA_AIS_ERR_NO_MEMORY;
					goto node_add_fail;
				}
				counter++;
				if (cl_info.ckpt_app_hdl > max_client_hdl) {
					max_client_hdl = cl_info.ckpt_app_hdl;
					cb->cli_id_gen = cl_info.ckpt_app_hdl + 1;
				}
				TRACE_1("cpnd client info read success");
			}
		}
		counter = 0;

		/* TO READ THE NUMBER OF CHECKPOINTS FROM THE HEADER */
		while (counter < MAX_CKPTS) {
			memset(&cp_info, '\0', sizeof(CKPT_INFO));
			i_offset = counter * sizeof(CKPT_INFO);
			m_CPND_CKPTINFO_READ(cp_info, (char *)gbl_shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);

			if (cp_info.is_valid == 0) {
				counter++;
				continue;
			}
			if (cp_info.is_first) {
				cp_node = m_MMGR_ALLOC_CPND_CKPT_NODE;
				if (cp_node == NULL) {
					LOG_ER("cpnd ckpt node memory allocation failed");
					goto memfail;
				}

				memset(cp_node, '\0', sizeof(CPND_CKPT_NODE));
				cp_node->ckpt_name = cp_info.ckpt_name;
				cp_node->ckpt_id = cp_info.ckpt_id;
				cp_node->offset = cp_info.offset;
				cp_node->is_close = cp_info.is_close;
				cp_node->is_unlink = cp_info.is_unlink;
				cp_node->close_time = cp_info.close_time;
				cp_node->cpnd_rep_create = cp_info.cpnd_rep_create;
				/* Non-collocated Differentiator flag */
				if (cp_info.cpnd_rep_create) {
					/* OPEN THE SHARED MEMORY ALREADY CREATED FOR CHECKPOINT REPLICA */
					/* size=cp_node->ckpt_name.length; */
					size = cp_node->ckpt_name.length;
					total_length = size + sizeof(cp_node->ckpt_id) + sizeof(NODE_ID) + 5;
					buf = m_MMGR_ALLOC_CPND_DEFAULT(total_length);
					memset(buf, '\0', total_length);
					strncpy(buf, (char *)cp_node->ckpt_name.value, size);
					sprintf(buf + size - 1, "_%d_%d", (uint32_t)nodeid, (uint32_t)cp_node->ckpt_id);
					rc = cpnd_ckpt_replica_create_res(&ckpt_rep_open, buf, &cp_node, 0, &cp_info);
					if (rc != NCSCC_RC_SUCCESS) {
						/*   assert(0); */
						TRACE_4("cpnd ckpt replica create failed with return value %d",rc);

						counter++;
						continue;
					}
					cb->num_rep++;
				}
				if (cp_node->is_unlink)
					cp_node->ckpt_name.length = 0;

				memset(&tmp_cp_info, '\0', sizeof(CKPT_INFO));
				memcpy(&tmp_cp_info, &cp_info, sizeof(CKPT_INFO));
				next_offset = cp_info.offset;
				while (next_offset >= 0) {
					num_bitset = client_bitmap_isset(tmp_cp_info.client_bitmap);	/* To check which clients opened this checkpoint */
					cp_node->ckpt_lcl_ref_cnt = cp_node->ckpt_lcl_ref_cnt + num_bitset;
					bit_position = 0;
					for (count = 1; count <= num_bitset; count++) {
						client_hdl = cpnd_client_extract_bits(tmp_cp_info.client_bitmap, &bit_position);	/* This will return the client which opened this checkpoint */
						TRACE_1("cpnd client handle extracted ");
						client_hdl = (tmp_cp_info.bm_offset * 32) + client_hdl;
						cpnd_client_node_get(cb, client_hdl, &cl_node);	/* already in the above do-while , we added client node to client tree */
						if (cl_node == NULL) {
							/* this should not have happened , quit */
							/*  assert(0); */
							TRACE_4("cpnd client node get failed client hdl: %llx",client_hdl);
							continue;
							/* goto end; */
						}
						cpnd_ckpt_client_add(cp_node, cl_node);
					}
					next_offset = tmp_cp_info.next;
					if (next_offset >= 0) {
						memset(&tmp_cp_info, '\0', sizeof(CKPT_INFO));
						i_offset = next_offset * sizeof(CKPT_INFO);
						m_CPND_CKPTINFO_READ(tmp_cp_info,
								     (char *)gbl_shm_addr.ckpt_addr + sizeof(CKPT_HDR),
								     i_offset);
					}

				}	/* End of clients processing for this cp_node */

				cpnd_ckpt_node_add(cb, cp_node);

				if (cp_info.is_close) {
					/* start the timer if exists */
					now = m_GET_TIME_STAMP(tmpTime);
					giga_sec = 1000000000;
					diff_time = now - cp_node->close_time;
					/* if((cp_node->create_attrib.retentionDuration) > (SA_TIME_ONE_SECOND*(presentTime - cp_node->close_time))) */
					if ((cp_node->create_attrib.retentionDuration) > (giga_sec * diff_time)) {
						/*  timeout = cp_node->create_attrib.retentionDuration - (SA_TIME_ONE_SECOND*(presentTime - cp_node->close_time)); */
						timeout =
						    cp_node->create_attrib.retentionDuration - (giga_sec * diff_time);
						timeout = m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);
					}
					if (timeout) {
						/* for restart shared memory updation */
						m_GET_TIME_STAMP(presentTime);
						cpnd_restart_update_timer(cb, cp_node, presentTime);
						if (!m_CPND_IS_COLLOCATED_ATTR_SET
								(cp_node->create_attrib.creationFlags)) {
							cp_node->ret_tmr.type = CPND_TMR_TYPE_NON_COLLOC_RETENTION;
						} else {
							cp_node->ret_tmr.type = CPND_TMR_TYPE_RETENTION;
						}
						cp_node->ret_tmr.uarg = cb->cpnd_cb_hdl_id;
						cp_node->ret_tmr.ckpt_id = cp_node->ckpt_id;
						cpnd_tmr_start(&cp_node->ret_tmr, timeout);
					} else {
						if (!m_CPND_IS_COLLOCATED_ATTR_SET
								(cp_node->create_attrib.creationFlags)) {
							cpnd_proc_non_colloc_rt_expiry(cb, cp_node->ckpt_id);
						} else {
							cpnd_proc_rt_expiry(cb, cp_node->ckpt_id);
						}
					}
				}

			}	/* End of one cp_node processing */
			counter++;
		}		/* End of while  after processing all 2000 ckpt structs */
	}			/* End of else  CPND after restart */
	TRACE_LEAVE();
	return cpnd_open_req->info.open.o_addr;
 memfail:
 node_add_fail:
	if (cl_node)
		cpnd_client_node_tree_cleanup(cb);
	if (cp_node)
		cpnd_ckpt_node_tree_cleanup(cb);
	TRACE_LEAVE();
	return cpnd_open_req->info.open.o_addr;
}

/* TO FIND THE FREE BLOCK */
/************************************************************************************
 * Name        :  cpnd_find_free_loc
 *
 * Description : To find the free block in the client info if case 1 & ckpt_info if case 2
 *               it will detect if there is any hole in between and allocate that memory,
 *               this is done by checking the is_valid flag
 *
 * Arguments   : type - which will decide for which case it has to find free block
 *
 * Return Values : Return the free block number 
 ***********************************************************************************/
int32_t cpnd_find_free_loc(CPND_CB *cb, CPND_TYPE_INFO type)
{
	int32_t counter = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	CLIENT_INFO cl_info;
	CKPT_INFO ckpt_info;
	NCS_OS_POSIX_SHM_REQ_INFO read_req;

	TRACE_ENTER();
	memset(&read_req, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));
	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));

	switch (type) {
	case CPND_CLIENT_INFO:
		do {
			read_req.type = NCS_OS_POSIX_SHM_REQ_READ;
			read_req.info.read.i_addr = (void *)((char *)cb->shm_addr.cli_addr + sizeof(CLIENT_HDR));
			read_req.info.read.i_read_size = sizeof(CLIENT_INFO);
			read_req.info.read.i_offset = counter * sizeof(CLIENT_INFO);
			read_req.info.read.i_to_buff = (CLIENT_INFO *)&cl_info;
			rc = ncs_os_posix_shm(&read_req);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("cpnd client info read shm failed");
				return -2;
			}
			if (1 == ((CLIENT_INFO *)read_req.info.read.i_to_buff)->is_valid) {
				counter++;
				memset(&cl_info, '\0', sizeof(CLIENT_INFO));
				if (counter == MAX_CLIENTS) {
					TRACE_1("cpnd max number of clients reached");
					counter = -1;
					break;
				}
			} else {
				TRACE_1("cpnd found free block for client");
				break;
			}
		} while (1);
		break;

	case CPND_CKPT_INFO:
		do {
			read_req.type = NCS_OS_POSIX_SHM_REQ_READ;
			read_req.info.read.i_addr = (void *)((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR));
			read_req.info.read.i_read_size = sizeof(CKPT_INFO);
			read_req.info.read.i_offset = counter * sizeof(CKPT_INFO);
			read_req.info.read.i_to_buff = (CKPT_INFO *)&ckpt_info;
			rc = ncs_os_posix_shm(&read_req);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_4("cpnd ckpt info read failed counter:%d",counter);
				TRACE_LEAVE();
				return -2;
			}

			if (1 == ((CKPT_INFO *)read_req.info.read.i_to_buff)->is_valid) {
				counter++;
				memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
				if (counter == MAX_CKPTS) {
					TRACE_4("cpnd max ckpts reached in read request");
					counter = -1;
					break;
				}
			} else {
				TRACE_1("cpnd ckpt free block success");
				break;
			}
		} while (1);
		break;
	default:
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return counter;
}

/*********************************************************************************
 * Name        : cpnd_ckpt_write_header
 * 
 * Description : To write the number of checkpoints 
 *
 * Arguments : nckpts  - number of checkpoints
 *
 * Return Values : Success / Error
 *
 * Notes : None
*********************************************************************************/
uint32_t cpnd_ckpt_write_header(CPND_CB *cb, uint32_t nckpts)
{
	CKPT_HDR ckpt_hdr;
	void *offset;
/*   offset = (cb->shm_addr.cli_addr+sizeof(CLIENT_HDR)+(MAX_CLIENTS*sizeof(CLIENT_INFO)));*/
	offset = cb->shm_addr.ckpt_addr;
	memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));

	ckpt_hdr.num_ckpts = nckpts;

	m_CPND_CKPTHDR_UPDATE(ckpt_hdr, offset);
	TRACE_1("cpnd ckpt write header success");
	return NCSCC_RC_SUCCESS;

}

/********************************************************************************************
 * Name        :  cpnd_cli_info_write_header
 *
 * Description : to write the client header
 *
 * Arguments : n_clients - number of clients 
 *
 * Return Values : Success / Error
*********************************************************************************************/
uint32_t cpnd_cli_info_write_header(CPND_CB *cb, int32_t n_clients)
{
	uint32_t rc = NCSCC_RC_SUCCESS, offset;
	CLIENT_HDR cl_hdr;
	memset(&cl_hdr, '\0', sizeof(CLIENT_HDR));

	cl_hdr.num_clients = n_clients;
	offset = 0;

	m_CPND_CLIHDR_INFO_WRITE((char *)cb->shm_addr.cli_addr, cl_hdr, offset);

	TRACE_1("cpnd cli info write header success");
	return rc;
}

/******************************************************************************************
 * Name         : cpnd_write_client_info
 *
 * Description  : To write the client info
 *
 * Arguments   : CPND_CKPT_CLIENT_NODE - client node , offset - to update the respective client
 *
 * Return Values : Success / Error
******************************************************************************************/
uint32_t cpnd_write_client_info(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node, int32_t offset)
{
	CLIENT_INFO cl_info;
	NCS_OS_POSIX_SHM_REQ_INFO write_req;
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));
	memset(&write_req, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));
	uint32_t rc = NCSCC_RC_SUCCESS, i_offset;

	cl_info.ckpt_app_hdl = cl_node->ckpt_app_hdl;
	cl_info.ckpt_open_ref_cnt = cl_node->ckpt_open_ref_cnt;
	cl_info.open_reader_flags_cnt = cl_node->open_reader_flags_cnt;
	cl_info.open_writer_flags_cnt  = cl_node->open_writer_flags_cnt;
	cl_info.agent_mds_dest = cl_node->agent_mds_dest;
	cl_info.version = cl_node->version;
	cl_info.is_valid = 1;
	cl_info.offset = offset;
	i_offset = offset * sizeof(CLIENT_INFO);

	m_CPND_CLINFO_UPDATE((char *)cb->shm_addr.cli_addr + sizeof(CLIENT_HDR), cl_info, i_offset);
	TRACE_1("cpnd client info update success for ckpt_app_hdl :%llx",cl_node->ckpt_app_hdl);
	return rc;
}

/******************************************************************************************
 * Name         : cpnd_restart_set_arrcb
 *
 * Description  : To set the arrival callback flag in client node
 *
 * Arguments   : CPND_CKPT_CLIENT_NODE - client node , 
 *
 * Return Values : Success / Error
******************************************************************************************/
void cpnd_restart_set_arrcb(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node)
{
	CLIENT_INFO cl_info;
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));

	m_CPND_CLINFO_READ(cl_info, ((char *)cb->shm_addr.cli_addr) + sizeof(CLIENT_HDR),
			   cl_node->offset * sizeof(CLIENT_INFO));
	cl_info.arr_flag = cl_node->arrival_cb_flag;
	m_CPND_CLINFO_UPDATE((char *)cb->shm_addr.cli_addr + sizeof(CLIENT_HDR),
			cl_info, cl_node->offset * sizeof(CLIENT_INFO));

}

/******************************************************************************************
 * Name         : reader_writer_flags_cnt
 *
 * Description  : To set the reader writer flags cnt in client node
 *
 * Arguments   : CPND_CKPT_CLIENT_NODE - client node , 
 *
 * Return Values : Success / Error
 ******************************************************************************************/
void cpnd_restart_set_reader_writer_flags_cnt(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node)
{
	CLIENT_INFO cl_info;
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));

	m_CPND_CLINFO_READ(cl_info, ((char *)cb->shm_addr.cli_addr) + sizeof(CLIENT_HDR),
			cl_node->offset * sizeof(CLIENT_INFO));
	cl_info.ckpt_open_ref_cnt = cl_node->ckpt_open_ref_cnt;
	cl_info.open_reader_flags_cnt = cl_node->open_reader_flags_cnt;
	cl_info.open_writer_flags_cnt = cl_node->open_writer_flags_cnt;

	m_CPND_CLINFO_UPDATE((char *)cb->shm_addr.cli_addr + sizeof(CLIENT_HDR),
			     cl_info, cl_node->offset * sizeof(CLIENT_INFO));

}

/***************************************************************************************
 * Name         :  cpnd_client_bitmap_set
 *
 * Description  : set the client hdl value bit 
 *
 * Arguments    : client_hdl 
 *
 * Return Values : bitmap value is returned
 **************************************************************************************/

uint32_t cpnd_client_bitmap_set(SaCkptHandleT client_hdl)
{
	uint32_t mask = 0, counter, bitmap_value = 0;
	for (counter = 0; counter <= client_hdl; counter++) {
		mask = 0x1;
		mask = mask << counter;
	}
	bitmap_value = bitmap_value | mask;
	return bitmap_value;
}

/************************************************************************************************
 * Name          :  cpnd_find_exact_ckptinfo
 *
 * Description   : find if the checkpoint info  exists in the shared memory ,
                   then findout the matching bm_offset offset value 
 *
 * Arguments     : CPND_CKPT_NODE - ckpt node
 *
 * Return Values : The offset( if same bm_pffset  present) /prev offset( if same bm_offset is not present )
                    where this checkpoint info is present 
 *
**************************************************************************************************/

bool cpnd_find_exact_ckptinfo(CPND_CB *cb, CKPT_INFO *ckpt_info, uint32_t bitmap_offset, uint32_t *offset,
				  uint32_t *prev_offset)
{
	int32_t next;
	CKPT_INFO prev_ckpt_info;
	uint32_t i_offset;
	bool found = false;

	TRACE_ENTER();
	memset(&prev_ckpt_info, 0, sizeof(CKPT_INFO));
	memcpy(&prev_ckpt_info, ckpt_info, sizeof(CKPT_INFO));
	next = ckpt_info->offset;
	*prev_offset = prev_ckpt_info.offset;

	while (next >= 0) {
		memset(&prev_ckpt_info, 0, sizeof(CKPT_INFO));
		i_offset = next * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_READ(prev_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);
		if (prev_ckpt_info.bm_offset == bitmap_offset) {
			found = true;
			*offset = prev_ckpt_info.offset;
			break;
		}
		next = prev_ckpt_info.next;
		*prev_offset = prev_ckpt_info.offset;
	}
	TRACE_LEAVE();
	return found;

}

/*******************************************************************************************************
 * Name          :  cpnd_update_ckpt_with_clienthdl
 *
 * Description   : To write the checkpoint data, which already exists in the shared memory, here we just change the bitmap
 *
 * Arguments     : CPND_CKPT_NODE - ckpt node , offset - to update the respective ckpt , client_hdl 
 *
 * Return Values : Success / Error
 * 
 * Notes         :  None
 ******************************************************************************************************/

uint32_t cpnd_update_ckpt_with_clienthdl(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaCkptHandleT client_hdl)
{
	CKPT_INFO ckpt_info, prev_ckpt_info, new_ckpt_info;
	uint32_t bitmap_offset = 0, bitmap_value = 0, i_offset, prev_offset, offset, rc = NCSCC_RC_SUCCESS;
	bool found = false;

	TRACE_ENTER();
	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
	memset(&prev_ckpt_info, '\0', sizeof(CKPT_INFO));
	memset(&new_ckpt_info, '\0', sizeof(CKPT_INFO));

	/* Read the starting shared memory entry for this cp_node */
	prev_offset = cp_node->offset;
	i_offset = prev_offset * sizeof(CKPT_INFO);
	m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);

	/* Findout the bitmap offset and bitmap value for the input client handle */
	bitmap_offset = client_hdl / 32;

	bitmap_value = cpnd_client_bitmap_set(client_hdl % 32);

	/*findout the ckpt_info with the exact bitmap_offset or findout prev ckpt_info if exact one not found */
	found = cpnd_find_exact_ckptinfo(cb, &ckpt_info, bitmap_offset, &offset, &prev_offset);

	if (!found) {
		CKPT_HDR ckpt_hdr;
		uint32_t no_ckpts = 0;
		/* Update the Next Location in the previous prev_ckpt_info.next as we have to find a new ckpt_info */
		i_offset = prev_offset * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_READ(prev_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);

		prev_ckpt_info.next = cpnd_find_free_loc(cb, CPND_CKPT_INFO);
		if (prev_ckpt_info.next == -1) {
			TRACE_4("cpnd client free block failed ");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		if (prev_ckpt_info.next == -2) {
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
			/* SHARED MEMORY READ ERROR */
		}

		/* Update the Header with incremented number of ckpt_info 's */
		memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));
		m_CPND_CKPTHDR_READ(ckpt_hdr, (char *)cb->shm_addr.ckpt_addr, 0);
		no_ckpts = ++(ckpt_hdr.num_ckpts);

		if (no_ckpts >= MAX_CKPTS)
		{
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		/* write the checkpoint info (number of ckpts)in the  header  */
		cpnd_ckpt_write_header(cb, no_ckpts);

		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), prev_ckpt_info, i_offset);

		/* Allocate New ckpt_info information */
		offset = prev_ckpt_info.next;
		/* bitmap_value = cpnd_client_bitmap_set((client_hdl%32)+1); */
		memcpy(&new_ckpt_info, &prev_ckpt_info, sizeof(CKPT_INFO));
		new_ckpt_info.offset = offset;
		new_ckpt_info.client_bitmap = bitmap_value;
		new_ckpt_info.bm_offset = bitmap_offset;
		new_ckpt_info.is_valid = 1;
		new_ckpt_info.is_first = false;
		new_ckpt_info.next = SHM_NEXT;

		i_offset = offset * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), new_ckpt_info, i_offset);

	} else {
		i_offset = offset * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_READ(prev_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);
		prev_ckpt_info.client_bitmap = prev_ckpt_info.client_bitmap | bitmap_value;
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), prev_ckpt_info, i_offset);
	}
	TRACE_LEAVE();
	return rc;
}

/************************************************************************************************************
 * Name          :  cpnd_write_ckpt_info
 *
 * Description   : To write checkpoint info 
 *
 * Arguments     : CPND_CKPT_NODE - ckpt node , offset - to write to corresponding ckpt , client_hdl
 *
 * Return Values : Success / Error
 *
 * Notes : Check if the offset is less than 31 , if so then update the information in the corresponding offset 
           else find the next free location and there update the checkpoint information
 ************************************************************************************************************/

uint32_t cpnd_write_ckpt_info(CPND_CB *cb, CPND_CKPT_NODE *cp_node, int32_t offset, SaCkptHandleT client_hdl)
{
	CKPT_INFO ckpt_info;
	uint32_t rc = NCSCC_RC_SUCCESS, i_offset;

	TRACE_ENTER();
	memset(&ckpt_info, 0, sizeof(CKPT_INFO));
	ckpt_info.ckpt_name = cp_node->ckpt_name;
	ckpt_info.ckpt_id = cp_node->ckpt_id;
	ckpt_info.maxSections = cp_node->create_attrib.maxSections;
	ckpt_info.maxSecSize = cp_node->create_attrib.maxSectionSize;
	ckpt_info.cpnd_rep_create = cp_node->cpnd_rep_create;
	ckpt_info.offset = offset;
	ckpt_info.node_id = cb->nodeid;
	ckpt_info.is_first = true;

	if (client_hdl) {
		ckpt_info.bm_offset = client_hdl / 32;
		ckpt_info.client_bitmap = cpnd_client_bitmap_set(client_hdl % 32);
	}
	ckpt_info.is_valid = 1;
	ckpt_info.next = SHM_NEXT;

	i_offset = offset * sizeof(CKPT_INFO);
	m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), ckpt_info, i_offset);

	TRACE_LEAVE2("cpnd ckpt info write success ckpt_id:%llx",cp_node->ckpt_id);
	return rc;

}

/********************************************************************
 *  Name        :   cpnd_restart_shm_client_update
 *
 * Description  :  Update the client info in the shared memory
 *
 * Arguments    : CPND_CKPT_CLIENT_NODE - client node
 *
 * Return Values : free block location number, which will be stored as offset in cb
 *
 * Notes : Update the client header , then update the client information
********************************************************************************/
int32_t cpnd_restart_shm_client_update(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node)
{
	CLIENT_INFO cl_info;
	int32_t free_shm_id, num_clients;
	uint32_t rc = NCSCC_RC_SUCCESS;
	CLIENT_HDR cli_hdr;

	TRACE_ENTER();
	memset(&cli_hdr, '\0', sizeof(CLIENT_HDR));
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));

	free_shm_id = cpnd_find_free_loc(cb, CPND_CLIENT_INFO);
	if (free_shm_id == -1) {
		TRACE_4("cpnd client free block failed");
		return free_shm_id;
	}
	if (free_shm_id == -2) {
		/* SHARED MEMORY READ FAILED */
		TRACE_4("shared memory read  failed");
		TRACE_LEAVE();
		return free_shm_id;
	}

	m_CPND_CLIHDR_INFO_READ(cli_hdr, (char *)cb->shm_addr.cli_addr, 0);
	/* num_clients = ++(cb->shm_addr.n_clients); */
	num_clients = ++(cli_hdr.num_clients);
	cpnd_cli_info_write_header(cb, num_clients);
	rc = cpnd_write_client_info(cb, cl_node, free_shm_id);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_4("cpnd client info update failed");
		TRACE_LEAVE();
		return -1;
	}
	cl_node->offset = free_shm_id;
	TRACE_LEAVE();
	return free_shm_id;
}

/*****************************************************************************************
 * Name            :  cpnd_restart_client_node_del
 *
 * Description     :  Update the client header and the corresponding client info
 *
 * Arguments       : CPND_CKPT_CLIENT_NODE - client info
 *
 * Return Values   : Success / Error
 *
 * Notes  : When client does a finalize then the shared memory is updated by decrementing the number of clients 
            and memsetting the corresponding client info in the shared memory
***************************************************************************************/
uint32_t cpnd_restart_client_node_del(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node)
{
	NCS_OS_POSIX_SHM_REQ_INFO clinfo_write;
	int32_t no_clients;
	uint32_t rc = NCSCC_RC_SUCCESS;
	CLIENT_INFO cl_info;
	CLIENT_HDR cli_hdr;

	TRACE_ENTER();
	memset(&clinfo_write, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));
	memset(&cl_info, '\0', sizeof(CLIENT_INFO));
	/* 1. Read from the cli header */

	memset(&cli_hdr, '\0', sizeof(CLIENT_HDR));
	m_CPND_CLIHDR_INFO_READ(cli_hdr, (char *)cb->shm_addr.cli_addr, 0);

	no_clients = --(cli_hdr.num_clients);
	cpnd_cli_info_write_header(cb, no_clients);

	clinfo_write.type = NCS_OS_POSIX_SHM_REQ_WRITE;
	clinfo_write.info.write.i_addr = (void *)((char *)cb->shm_addr.cli_addr + sizeof(CLIENT_HDR));
	clinfo_write.info.write.i_from_buff = (CLIENT_INFO *)&cl_info;
	clinfo_write.info.write.i_offset = cl_node->offset * sizeof(CLIENT_INFO);
	clinfo_write.info.write.i_write_size = sizeof(CLIENT_INFO);
	rc = ncs_os_posix_shm(&clinfo_write);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd ckpt info write failed"); 
		return rc;
	} else {
		TRACE_1("cpnd ckpt info write success");
	}

	TRACE_LEAVE();
	return rc;
}

/*************************************************************************************************
 * Name             : client_bitmap_reset
 *
 * Description      : To reset the bit 
 *
 * Arguments        : bitmap_value - bitmap value , client_hdl
 *
 * Return Values    : Success / Error
 *
 * Notes : To reset the bitmap when the client has finalized
 ************************************************************************************************/

uint32_t client_bitmap_reset(uint32_t *bitmap_value, uint32_t client_hdl)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	(*bitmap_value) = (*bitmap_value) & (~(0x1 << client_hdl));

	return rc;
}

/*************************************************************************************
 * Name      :  client_bitmap_isset
 *
 * Description : Returns the number of clients who opened the checkpoint 
 *
 * Arguments : bitmap_value
 *
 *  Return Values    : Number of clients
 *
 ************************************************************************************/
uint32_t client_bitmap_isset(uint32_t bitmap_value)
{
	uint32_t mask = 0x1, counter;
	uint32_t value = 0;
	uint32_t bitcount = 8 * sizeof(uint32_t);
	for (counter = 1; counter <= (bitcount); counter++) {
		if (bitmap_value & mask)
			value++;

		mask = 0x1;
		mask = mask << counter;
	}
	return value;
}

/*****************************************************************************************
 * Name          :  cpnd_restart_shm_ckpt_free
 *
 * Description   : To update the checkpoint header and info in the shared memory at the time of close
 *
 * Arguments   : CPND_CKPT_NODE - ckpt node , CPND_CKPT_CLIENT_NODE - client node
 *
 * Return Values : Success / Error
 *
 * Notes : update the checkpoint header by decrementing the ckpts , and to memset the ckpt info if n_clients = 0
 *         this is called at the time of close
 ****************************************************************************************/
uint32_t cpnd_restart_shm_ckpt_free(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{
	CKPT_INFO ckpt_info;
	CKPT_HDR ckpt_hdr;
	uint32_t rc = NCSCC_RC_SUCCESS, i_offset, no_ckpts = 0;

	TRACE_ENTER();
	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));

	/* Update the ckpt Header with number ckpt_info 's */
	memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));
	m_CPND_CKPTHDR_READ(ckpt_hdr, (char *)cb->shm_addr.ckpt_addr, 0);
	no_ckpts = --(ckpt_hdr.num_ckpts);
	cpnd_ckpt_write_header(cb, no_ckpts);

	i_offset = (cp_node->offset) * sizeof(CKPT_INFO);
	cp_node->offset = SHM_INIT;

	/*Update the prev & curr shared memory segments with the new data */
	m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), ckpt_info, i_offset);
	
	TRACE_LEAVE();

	return rc;
}

/*********************************************************************************************************
 * Name : cpnd_restart_ckpt_name_length_reset
 * 
 * Description : To reset the length of the checkpoint Name to 0 in the CPND shared memory
 *
 * Arguments : cb & cp_node
 *
 * Return Values :
*********************************************************************************************************/

void cpnd_restart_ckpt_name_length_reset(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{
	CKPT_INFO ckpt_info;

	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
	if (cp_node->offset >= 0) {
		m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				     cp_node->offset * sizeof(CKPT_INFO));
		ckpt_info.is_unlink = true;
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), ckpt_info,
				       cp_node->offset * sizeof(CKPT_INFO));
	}
	return;
}

/************************************************************************************************************
 * Name :  cpnd_restart_set_close_flag
 *
 * Description : To set the close flag in the shared memory (CPND) 
 *
 * Arguments :
 *
 * Return Values:
************************************************************************************************************/
void cpnd_restart_set_close_flag(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{
	CKPT_INFO ckpt_info;

	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
	if (cp_node->offset >= 0) {
		m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				     cp_node->offset * sizeof(CKPT_INFO));
		ckpt_info.is_close = true;
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), ckpt_info,
				       cp_node->offset * sizeof(CKPT_INFO));
	}
	return;
}

/************************************************************************************************************
 * Name :  cpnd_restart_reset_close_flag
 *
 * Description : To reset the close flag in the shared memory (CPND)
 *
 * Arguments :
 *
 * Return Values:
************************************************************************************************************/
void cpnd_restart_reset_close_flag(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{
	CKPT_INFO ckpt_info;

	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
	if (cp_node->offset >= 0) {
		m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				     cp_node->offset * sizeof(CKPT_INFO));
		ckpt_info.is_close = false;
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				       ckpt_info, cp_node->offset * sizeof(CKPT_INFO));
	}
	return;
}

/************************************************************************************************************
 * Name         :  cpnd_clear_ckpt_info 
 *
 * Description  : To start the timer and to reset the client information
 *
 * Arguments    :
 *
 * Return Values:
************************************************************************************************************/
void cpnd_clear_ckpt_info(CPND_CB *cb, CPND_CKPT_NODE *cp_node, uint32_t curr_offset, uint32_t prev_offset)
{
	CKPT_INFO prev_ckpt_info, curr_ckpt_info, next_ckpt_info;
	uint32_t i_offset, no_ckpts;
	CKPT_HDR ckpt_hdr;

	TRACE_ENTER();
	memset(&prev_ckpt_info, '\0', sizeof(CKPT_INFO));
	memset(&curr_ckpt_info, '\0', sizeof(CKPT_INFO));
	memset(&next_ckpt_info, '\0', sizeof(CKPT_INFO));

	i_offset = prev_offset * sizeof(CKPT_INFO);
	m_CPND_CKPTINFO_READ(prev_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);

	i_offset = curr_offset * sizeof(CKPT_INFO);
	m_CPND_CKPTINFO_READ(curr_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), i_offset);

	/* Update the Next Location in the previous prev_ckpt_info.next as we have to clear the curr ckpt_info */
	if (cp_node->offset != curr_offset) {
		memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));
		m_CPND_CKPTHDR_READ(ckpt_hdr, (char *)cb->shm_addr.ckpt_addr, 0);
		no_ckpts = --(ckpt_hdr.num_ckpts);
		/* write the checkpoint info (number of ckpts)in the  header  */
		cpnd_ckpt_write_header(cb, no_ckpts);

		prev_ckpt_info.next = curr_ckpt_info.next;
		/*Update the prev & curr shared memory segments with the new data */
		i_offset = prev_offset * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), prev_ckpt_info, i_offset);

		memset(&curr_ckpt_info, '\0', sizeof(CKPT_INFO));
		i_offset = curr_offset * sizeof(CKPT_INFO);
		m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), curr_ckpt_info, i_offset);
	} else {		/* This is the starting entry for this cp_node so update accordingly */

		if (curr_ckpt_info.next >= 0) {
			memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));
			m_CPND_CKPTHDR_READ(ckpt_hdr, (char *)cb->shm_addr.ckpt_addr, 0);
			no_ckpts = --(ckpt_hdr.num_ckpts);
			/* write the checkpoint info (number of ckpts)in the  header  */
			cpnd_ckpt_write_header(cb, no_ckpts);

			cp_node->offset = curr_ckpt_info.next;

			i_offset = (curr_ckpt_info.next) * sizeof(CKPT_INFO);
			m_CPND_CKPTINFO_READ(next_ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
					     i_offset);

			next_ckpt_info.is_close = curr_ckpt_info.is_close;
			next_ckpt_info.is_unlink = curr_ckpt_info.is_unlink;
			next_ckpt_info.close_time = curr_ckpt_info.close_time;
			next_ckpt_info.is_first = true;
			m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), next_ckpt_info,
					       i_offset);

			i_offset = (curr_ckpt_info.offset) * sizeof(CKPT_INFO);
			memset(&curr_ckpt_info, '\0', sizeof(CKPT_INFO));

			m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR), curr_ckpt_info,
					       i_offset);

		} else {
			/* There is only one ckpt_info is there for this cp_node so no need to delete this node as part of close     
			   This ckpt_info gets deleted as part of  unlink & lcl_ref_cnt of cp_node == 0  /  lcl_ref_cnt == 0 & ret_tmr expires */
		}
	}
	TRACE_LEAVE();
}

/************************************************************************************************************
 * Name :  cpnd_restart_client_reset
 *
 * Description : To start the timer and to reset the client information
 *
 * Arguments :
 *
 * Return Values:
************************************************************************************************************/
void cpnd_restart_client_reset(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node)
{
	CKPT_INFO ckpt_info;
	uint32_t bitmap_offset = 0, num_bitset = 0;
	bool found = false;
	uint32_t offset, prev_offset;
	SaCkptHandleT client_hdl = cl_node->ckpt_app_hdl;


	TRACE_ENTER();
	bitmap_offset = client_hdl / 32;

	memset(&ckpt_info, '\0', sizeof(CKPT_INFO));

	if (cp_node->offset >= 0) {
		m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
				     cp_node->offset * sizeof(CKPT_INFO));
		/* findour the exact ckpt_info matching the client_hdl */
		found = cpnd_find_exact_ckptinfo(cb, &ckpt_info, bitmap_offset, &offset, &prev_offset);
		if (found) {
			memset(&ckpt_info, '\0', sizeof(CKPT_INFO));
			m_CPND_CKPTINFO_READ(ckpt_info, (char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
					     offset * sizeof(CKPT_INFO));
			client_bitmap_reset(&ckpt_info.client_bitmap, (client_hdl % 32));
			m_CPND_CKPTINFO_UPDATE((char *)cb->shm_addr.ckpt_addr + sizeof(CKPT_HDR),
					       ckpt_info, offset * sizeof(CKPT_INFO));

			/* Delete the ckpt_info from shared memory if this ckpt_info's all 31 refs are closed */
			num_bitset = client_bitmap_isset(ckpt_info.client_bitmap);
			if (!num_bitset)
				cpnd_clear_ckpt_info(cb, cp_node, offset, prev_offset);

		}
	}
	TRACE_LEAVE();
	return;
}

/*********************************************************************************************
 * Name           :  cpnd_restart_shm_ckpt_update
 *
 * Description    : To update the checkpoint when new checkpoint is opened
 *
 * Arguments      : CPND_CKPT_NODE - ckpt node , client_hdl
 *
 * Return Values  : offset value where this checkpoint is stored
 * Notes      : Ckpt info of the shared memory is updated 
   1. we update the number of checkpoints in the checkpoint header i.e incrementing by 1 
   2. check if the checkpoint already exists in the shared memory 
   3. If no  -> then add the entire ckpt info to the shared memory
   4. If yes -> then just add the client hdl to the ckpt info bcos already this checkpoint is opened by someone
 *
 **********************************************************************************************/
uint32_t cpnd_restart_shm_ckpt_update(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaCkptHandleT client_hdl)
{
	int32_t ckpt_id_exists = 0, no_ckpts = 0;
	CKPT_INFO ckpt_info;
	memset(&ckpt_info, 0, sizeof(ckpt_info));
	CKPT_HDR ckpt_hdr;

	TRACE_ENTER();
	/* check if the ckpt already exists */
	if (cp_node->offset == SHM_INIT) {	/* if it is not there then find the free place to fit into */
		/* now find the free shm for placing the checkpoint info */
		ckpt_id_exists = cpnd_find_free_loc(cb, CPND_CKPT_INFO);
		if (ckpt_id_exists == -1 || ckpt_id_exists == -2) {
			/* LOG THE ERROR - MEMORY FULL */
			TRACE_4("cpnd client free block failed in SHM_INIT");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		} else {
			memset(&ckpt_hdr, '\0', sizeof(CKPT_HDR));
			m_CPND_CKPTHDR_READ(ckpt_hdr, (char *)cb->shm_addr.ckpt_addr, 0);
			no_ckpts = ++(ckpt_hdr.num_ckpts);

			if (no_ckpts >= MAX_CKPTS)
				return NCSCC_RC_FAILURE;

			/* write the checkpoint info (number of ckpts)in the  header  */
			cpnd_ckpt_write_header(cb, no_ckpts);

			/* new cp_node,add it in the cpnd shared memory */
			cpnd_write_ckpt_info(cb, cp_node, ckpt_id_exists, client_hdl);
			cp_node->offset = ckpt_id_exists;
		}
	} else {
		if (client_hdl) {
			if (cpnd_update_ckpt_with_clienthdl(cb, cp_node, client_hdl) != NCSCC_RC_SUCCESS)
			{
				TRACE_LEAVE();
				return NCSCC_RC_FAILURE;
			}
		} else {
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
