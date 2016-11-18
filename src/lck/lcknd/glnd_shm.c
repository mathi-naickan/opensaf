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

  DESCRIPTION: This file contains the Library functions for shared memory interface 

    This file inclused following routines:
   
    glnd_shm_open 
    glnd_shm_create
    glnd_shm_destroy
    glnd_find_res_shm_ckpt_empty_section
    glnd_find_lck_shm_ckpt_empty_section
    glnd_find_evt_shm_ckpt_empty_section
    glnd_res_shm_section_invalidate
    glnd_lck_shm_section_invalidate
    glnd_evt_shm_section_invalidate
    
******************************************************************************/

#include "lck/lcknd/glnd.h"
/**************************************************************************************************************
 * Name           : glnd_shm_open
 *
 * Description    : To create a shared memory segment to store res_info lock_info & backup_evt
 *
 * Arguments      : GLND_CB *cb
 *                  char shm_name[]
 *
 * Return Values  : SUCCESS/FAILURE
 * Notes          : None
**************************************************************************************************************/
static uint32_t glnd_shm_open(GLND_CB *cb, char *shm_name)
{
	NCS_OS_POSIX_SHM_REQ_INFO glnd_open_req;
	uint64_t shm_size;
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLND_SHM_VERSION glnd_shm_version;

	/* Initializing shared memory vesion */
	memset(&glnd_shm_version, '\0', sizeof(glnd_shm_version));
	glnd_shm_version.shm_version = GLSV_GLND_SHM_VERSION;

	/*Set the shared memory size to be created */
	if (memcmp(shm_name, RES_SHM_NAME, strlen(shm_name)) == 0)
		shm_size = sizeof(GLND_RESTART_RES_INFO) * GLND_RESOURCE_INFO_CKPT_MAX_SECTIONS;
	if (memcmp(shm_name, LCK_SHM_NAME, strlen(shm_name)) == 0)
		shm_size = sizeof(GLND_RESTART_RES_LOCK_LIST_INFO) * GLND_RES_LOCK_INFO_CKPT_MAX_SECTIONS;
	if (memcmp(shm_name, EVT_SHM_NAME, strlen(shm_name)) == 0)
		shm_size = sizeof(GLSV_RESTART_BACKUP_EVT_INFO) * GLND_BACKUP_EVT_CKPT_MAX_SECTIONS;

	memset(&glnd_open_req, '\0', sizeof(glnd_open_req));

	glnd_open_req.type = NCS_OS_POSIX_SHM_REQ_OPEN;
	glnd_open_req.info.open.i_size = shm_size;
	glnd_open_req.info.open.ensures_space = false;
	glnd_open_req.info.open.i_offset = 0;
	glnd_open_req.info.open.i_name = shm_name;
	glnd_open_req.info.open.i_map_flags = MAP_SHARED;
	glnd_open_req.info.open.o_addr = NULL;
	glnd_open_req.info.open.i_flags = O_RDWR;

	rc = ncs_os_posix_shm(&glnd_open_req);

	if (rc == NCSCC_RC_FAILURE) {
		/* Initially Shared memory open fails so create a shared memory */
		glnd_open_req.info.open.i_flags = (unsigned int)O_CREAT | O_RDWR;

		rc = ncs_os_posix_shm(&glnd_open_req);
		if (rc == NCSCC_RC_FAILURE) {
			LOG_CR("GLND  shm create failure: rc %u Error %s ", rc, strerror(errno));
			assert(0);
		} else {
			TRACE_1("GLND shm create success: rc %u", rc);

			if (memcmp(shm_name, RES_SHM_NAME, strlen(shm_name)) == 0) {
				/* Store the shared memory base address */
				cb->shm_base_addr = glnd_open_req.info.open.o_addr;
				memcpy(glnd_open_req.info.open.o_addr, &glnd_shm_version, sizeof(glnd_shm_version));

				/* Stor the Resource shared memory base address */
				cb->glnd_res_shm_base_addr = glnd_open_req.info.open.o_addr + sizeof(glnd_shm_version);
			}
			if (memcmp(shm_name, LCK_SHM_NAME, strlen(shm_name)) == 0)
				cb->glnd_lck_shm_base_addr = glnd_open_req.info.open.o_addr;
			if (memcmp(shm_name, EVT_SHM_NAME, strlen(shm_name)) == 0)
				cb->glnd_evt_shm_base_addr = glnd_open_req.info.open.o_addr;

			/* Set the the state of GLND as GLND_OPERATIONAL_STATE */
			cb->node_state = GLND_OPERATIONAL_STATE;
		}
	} else {
		TRACE_1("GLND shm open success: rc %u", rc);

		if (memcmp(shm_name, RES_SHM_NAME, strlen(shm_name)) == 0) {
			/* Store the shared memory base address */
			cb->shm_base_addr = glnd_open_req.info.open.o_addr;
			memcpy(glnd_open_req.info.open.o_addr, &glnd_shm_version, sizeof(glnd_shm_version));

			/* Store the Resource shared memory base address */
			cb->glnd_res_shm_base_addr = glnd_open_req.info.open.o_addr + sizeof(glnd_shm_version);
		}
		if (memcmp(shm_name, LCK_SHM_NAME, strlen(shm_name)) == 0)
			cb->glnd_lck_shm_base_addr = glnd_open_req.info.open.o_addr;
		if (memcmp(shm_name, EVT_SHM_NAME, strlen(shm_name)) == 0)
			cb->glnd_evt_shm_base_addr = glnd_open_req.info.open.o_addr;

		/* Set the the state of GLND as GLND_CLIENT_INFO_GET_STATE */
		cb->node_state = GLND_CLIENT_INFO_GET_STATE;
	}

	return rc;

}

/**************************************************************************************************************
 * Name           : glnd_shm_create
 *
 * Description    : To create a shared memory segment to store res_info lock_info & backup_evt 
 *
 * Arguments      : GLND_CB *cb 
 *
 * Return Values  : SUCCESS/FAILURE
 * Notes          : None
**************************************************************************************************************/
uint32_t glnd_shm_create(GLND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Create NCS_GLND_RES_CKPT_INFO shared_memory */
	rc = glnd_shm_open(cb, RES_SHM_NAME);
	if (rc == NCSCC_RC_FAILURE)
		return rc;

	/* Create NCS_GLND_LCK_CKPT_INFO shared_memory */
	rc = glnd_shm_open(cb, LCK_SHM_NAME);
	if (rc == NCSCC_RC_FAILURE) {
		glnd_shm_destroy(cb, RES_SHM_NAME);
		return rc;
	}
	/* Create NCS_GLND_EVT_CKPT_INFO shared_memory */
	rc = glnd_shm_open(cb, EVT_SHM_NAME);
	if (rc == NCSCC_RC_FAILURE) {
		glnd_shm_destroy(cb, RES_SHM_NAME);
		glnd_shm_destroy(cb, LCK_SHM_NAME);
		return rc;
	}

	if (cb->node_state == GLND_CLIENT_INFO_GET_STATE) {
		/* Node Restarted so start the timer to get get client info from agents */
		cb->agent_info_get_tmr.cb_hdl = cb->cb_hdl_id;
		glnd_start_tmr(cb, &cb->agent_info_get_tmr,
			       GLND_TMR_AGENT_INFO_GET_WAIT, GLND_AGENT_INFO_GET_TIMEOUT, (uint32_t)cb->cb_hdl_id);
		TRACE_1("GLND Restarted: rc %u", rc);
	} else
		TRACE_1("GLND coming up first time: rc %u", rc);
	return rc;
}

/**************************************************************************************************************
 * Name           : glnd_shm_destroy
 *
 * Description    : To destroy shared memory segment 
 *
 * Arguments      : GLND_CB
 *
 * Return Values  : SUCCESS/FAILURE
 * Notes          : None
**************************************************************************************************************/
uint32_t glnd_shm_destroy(GLND_CB *cb, char *shm_name)
{
	NCS_OS_POSIX_SHM_REQ_INFO glnd_dest_req;
	uint32_t rc = NCSCC_RC_SUCCESS;

	glnd_dest_req.type = NCS_OS_POSIX_SHM_REQ_UNLINK;

	glnd_dest_req.info.unlink.i_name = shm_name;
	rc = ncs_os_posix_shm(&glnd_dest_req);
	return rc;
}

/**************************************************************************************************************
 * Name           : glnd_find_res_shm_ckpt_empty_section  
 *
 * Description    : To do a linear search through the shared memory segment and find a free section 
 *
 * Arguments      : GLND_CB 
 *
 * Return Values  : Index value which is the offset to access the queue stats in shared memory
 * Notes          : None
**************************************************************************************************************/
uint32_t glnd_find_res_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index)
{
	GLND_RESTART_RES_INFO *shm_base_addr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	int i;

	shm_base_addr = cb->glnd_res_shm_base_addr;

	for (i = 0; i < GLND_RESOURCE_INFO_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_addr[i].valid == GLND_SHM_INFO_VALID)
			continue;
		else {
			memset((shm_base_addr + i), '\0', sizeof(GLND_RESTART_RES_INFO));
			shm_base_addr[i].valid = GLND_SHM_INFO_VALID;
			*index = i;
			return rc;
		}
	}
	return NCSCC_RC_FAILURE;

}

/**************************************************************************************************************
 * Name           : glnd_find_lck_shm_ckpt_empty_section
 *
 * Description    : To do a linear search through the shared memory segment and find a free section
 *
 * Arguments      : GLND_CB
 *
 * Return Values  : Index value which is the offset to access the queue stats in shared memory
 * Notes          : None
**************************************************************************************************************/
uint32_t glnd_find_lck_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index)
{
	GLND_RESTART_RES_LOCK_LIST_INFO *shm_base_addr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	int i;

	shm_base_addr = cb->glnd_lck_shm_base_addr;

	for (i = 0; i < GLND_RES_LOCK_INFO_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_addr[i].valid == GLND_SHM_INFO_VALID)
			continue;
		else {
			memset((shm_base_addr + i), '\0', sizeof(GLND_RESTART_RES_LOCK_LIST_INFO));
			shm_base_addr[i].valid = GLND_SHM_INFO_VALID;
			*index = i;
			return rc;
		}
	}
	return NCSCC_RC_FAILURE;

}

/**************************************************************************************************************
 * Name           : glnd_find_evt_shm_ckpt_empty_section
 *
 * Description    : To do a linear search through the shared memory segment and find a free section
 *
 * Arguments      : GLND_CB
 *
 * Return Values  : Index value which is the offset to access the queue stats in shared memory
 * Notes          : None
**************************************************************************************************************/
uint32_t glnd_find_evt_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index)
{
	GLSV_RESTART_BACKUP_EVT_INFO *shm_base_addr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	int i;

	shm_base_addr = cb->glnd_evt_shm_base_addr;

	for (i = 0; i < GLND_BACKUP_EVT_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_addr[i].valid == GLND_SHM_INFO_VALID)
			continue;
		else {
			memset((shm_base_addr + i), '\0', sizeof(GLSV_RESTART_BACKUP_EVT_INFO));
			shm_base_addr[i].valid = GLND_SHM_INFO_VALID;
			*index = i;
			return rc;
		}
	}
	
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : glnd_res_shm_section_invalidate
 *
 * Description   : Function to invalidate the shared memory area of the queue 
 *
 * Arguments     :GLND_CB, GLND_RESOURCE_INFO 
                   
 *
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_res_shm_section_invalidate(GLND_CB *cb, GLND_RESOURCE_INFO *res_info)
{
	GLND_RESTART_RES_INFO *shm_base_addr = NULL;
	uint32_t offset;

	if (!res_info)
		return NCSCC_RC_FAILURE;

	shm_base_addr = cb->glnd_res_shm_base_addr;

	offset = res_info->shm_index;
	if (shm_base_addr[offset].valid == GLND_SHM_INFO_VALID) {
		shm_base_addr[offset].valid = GLND_SHM_INFO_INVALID;
		memset((shm_base_addr + offset), '\0', sizeof(GLND_RESTART_RES_INFO));
	}
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : glnd_lck_shm_section_invalidate
 *
 * Description   : Function to invalidate the shared memory area of the queue
 *
 * Arguments     :GLND_CB, GLND_RESOURCE_INFO

 *
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_lck_shm_section_invalidate(GLND_CB *cb, GLND_RES_LOCK_LIST_INFO *lock_info)
{
	GLND_RESTART_RES_LOCK_LIST_INFO *shm_base_addr = NULL;
	uint32_t offset;

	if (!lock_info)
		return NCSCC_RC_FAILURE;

	shm_base_addr = cb->glnd_lck_shm_base_addr;

	offset = lock_info->shm_index;
	if (shm_base_addr[offset].valid == GLND_SHM_INFO_VALID) {
		shm_base_addr[offset].valid = GLND_SHM_INFO_INVALID;
		memset((shm_base_addr + offset), '\0', sizeof(GLND_RESTART_RES_LOCK_LIST_INFO));
	}
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : glnd_evt_shm_section_invalidate
 *
 * Description   : Function to invalidate the shared memory area of the queue
 *
 * Arguments     :GLND_CB, 

 *
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_evt_shm_section_invalidate(GLND_CB *cb, GLSV_GLND_EVT *glnd_evt)
{
	GLSV_RESTART_BACKUP_EVT_INFO *shm_base_addr = NULL;
	uint32_t offset;

	if (!glnd_evt)
		return NCSCC_RC_FAILURE;

	shm_base_addr = cb->glnd_evt_shm_base_addr;

	offset = glnd_evt->shm_index;
	if (shm_base_addr[offset].valid == GLND_SHM_INFO_VALID) {
		shm_base_addr[offset].valid = GLND_SHM_INFO_INVALID;
		memset((shm_base_addr + offset), '\0', sizeof(GLSV_RESTART_BACKUP_EVT_INFO));
	}

	return NCSCC_RC_SUCCESS;
}
