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

  DESCRIPTION: This file contains the Library functions for Shared memory 
               stats update of the queue 

    This file inclused following routines:
    
    mqnd_kernel_msgparams
    mqnd_shm_create
    mqnd_reset_queue_stats
    mqnd_find_shm_ckpt_empty_section
    mqnd_send_msg_update_stats_shm
    mqnd_shm_queue_ckpt_section_invalidate
    
******************************************************************************/

#include "mqnd.h"

/**************************************************************************************************************
 * Name           : mqnd_shm_create
 *
 * Description    : To create a shared memory segment to store the stats of the queues opened on a Node
 *
 * Arguments      : MQND_CB *cb 
 *
 * Return Values  : SUCCESS/FAILURE
 * Notes          : None
**************************************************************************************************************/

uint32_t mqnd_shm_create(MQND_CB *cb)
{
	NCS_OS_POSIX_SHM_REQ_INFO mqnd_open_req;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char shm_name[] = SHM_NAME;
	MQND_SHM_VERSION mqnd_shm_version;
	TRACE_ENTER();

	cb->mqnd_shm.max_open_queues = cb->gl_msg_max_no_of_q;

	memset(&mqnd_open_req, '\0', sizeof(mqnd_open_req));

	/* Initializing shared memory version */
	memset(&mqnd_shm_version, '\0', sizeof(mqnd_shm_version));
	mqnd_shm_version.shm_version = MQSV_MQND_SHM_VERSION;

	mqnd_open_req.type = NCS_OS_POSIX_SHM_REQ_OPEN;
	mqnd_open_req.info.open.i_size =
	    sizeof(MQND_SHM_VERSION) + (sizeof(MQND_QUEUE_CKPT_INFO) * cb->mqnd_shm.max_open_queues);
	mqnd_open_req.info.open.i_offset = 0;
	mqnd_open_req.info.open.i_name = shm_name;
	mqnd_open_req.info.open.i_map_flags = MAP_SHARED;
	mqnd_open_req.info.open.o_addr = NULL;
	mqnd_open_req.info.open.i_flags = O_RDWR;

	rc = ncs_os_posix_shm(&mqnd_open_req);

	if (rc == NCSCC_RC_FAILURE) {
		/* INITIALLY IT FAILS SO CREATE A SHARED MEMORY */
		mqnd_open_req.info.open.i_flags = O_CREAT | O_RDWR;

		rc = ncs_os_posix_shm(&mqnd_open_req);
		if (rc == NCSCC_RC_FAILURE) {
			LOG_CR("Creation of shared memory segment failed");
			return rc;
		} else {
			memset(mqnd_open_req.info.open.o_addr, 0,
			       sizeof(MQND_SHM_VERSION) +
			       (sizeof(MQND_QUEUE_CKPT_INFO) * (cb->mqnd_shm.max_open_queues)));
			TRACE_1("First time starting the MQND");
			cb->is_restart_done = true;
			TRACE("Firsttime Openingthe Ckpt");
			cb->is_create_ckpt = true;
			memcpy(mqnd_open_req.info.open.o_addr, &mqnd_shm_version, sizeof(mqnd_shm_version));
		}
	} else
		cb->is_create_ckpt = false;

	/* Store Shared memory start address which contains MQSV Sharedmemory version */
	cb->mqnd_shm.shm_start_addr = mqnd_open_req.info.open.o_addr;

	/* Store Shared memory base address which is used by MQSV to checkpoint queue informaition */
	cb->mqnd_shm.shm_base_addr = mqnd_open_req.info.open.o_addr + sizeof(mqnd_shm_version);

	TRACE_LEAVE();
	return rc;
}

/**************************************************************************************************************
 * Name           : mqnd_shm_destroy
 *
 * Description    : To destroy shared memory segment 
 *
 * Arguments      : MQND_CB
 *
 * Return Values  : SUCCESS/FAILURE
 * Notes          : None
**************************************************************************************************************/

uint32_t mqnd_shm_destroy(MQND_CB *cb)
{
	NCS_OS_POSIX_SHM_REQ_INFO mqnd_dest_req;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char shm_name[] = SHM_NAME;

	mqnd_dest_req.type = NCS_OS_POSIX_SHM_REQ_UNLINK;
	mqnd_dest_req.info.unlink.i_name = shm_name;

	rc = ncs_os_posix_shm(&mqnd_dest_req);
	if (rc != NCSCC_RC_SUCCESS);
		LOG_CR("Destroying the shared memory segment failed");

	return rc;
}

/**************************************************************************************************************
 * Name           : mqnd_reset_queue_stats
 *
 * Description    : To reset queue stats when an existing message queue is opened with empty flag set
 *
 * Arguments      : MQND_CB , INDEX (representing the queue offset in shm) 
 *
 * Return Values  : 
 * Notes          : None
**************************************************************************************************************/

void mqnd_reset_queue_stats(MQND_CB *cb, uint32_t index)
{
	int j;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;
	for (j = SA_MSG_MESSAGE_HIGHEST_PRIORITY; j <= SA_MSG_MESSAGE_LOWEST_PRIORITY; j++) {
		shm_base_addr[index].QueueStatsShm.saMsgQueueUsage[j].queueUsed = 0;
		shm_base_addr[index].QueueStatsShm.saMsgQueueUsage[j].numberOfMessages = 0;
	}
	shm_base_addr[index].QueueStatsShm.totalQueueUsed = 0;
	shm_base_addr[index].QueueStatsShm.totalNumberOfMessages = 0;

}

/**************************************************************************************************************
 * Name           : mqnd_find_shm_ckpt_empty_section  
 *
 * Description    : To do a linear search through the shared memory segment and find a free section 
 *
 * Arguments      : MQND_CB 
 *
 * Return Values  : Index value which is the offset to access the queue stats in shared memory
 * Notes          : None
**************************************************************************************************************/

uint32_t mqnd_find_shm_ckpt_empty_section(MQND_CB *cb, uint32_t *index)
{
	int i;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	uint32_t rc = NCSCC_RC_SUCCESS;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	for (i = 0; i < cb->mqnd_shm.max_open_queues; i++) {
		if (shm_base_addr[i].valid == SHM_QUEUE_INFO_VALID)
			continue;
		else {
			memset((shm_base_addr + i), '\0', sizeof(MQND_QUEUE_CKPT_INFO));
			shm_base_addr[i].valid = SHM_QUEUE_INFO_VALID;
			*index = i;
			return rc;
		}
	}
	return NCSCC_RC_FAILURE;

}

/****************************************************************************
 * Name          : mqnd_send_msg_update_stats_shm
 *
 * Description   : Function to update stats of queue in shm when message is sent.
 *
 * Arguments     : MQND_QUEUE_NODE *qnode
                   MQP_SEND_MSG *snd_msg 
 *                
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_send_msg_update_stats_shm(MQND_CB *cb, MQND_QUEUE_NODE *qnode, SaSizeT size, SaUint8T priority)
{
	uint32_t offset;

	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	offset = qnode->qinfo.shm_queue_index;
	if (shm_base_addr[offset].valid == SHM_QUEUE_INFO_VALID) {
		shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[priority].queueUsed += size;
		shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[priority].numberOfMessages += 1;
		shm_base_addr[offset].QueueStatsShm.totalQueueUsed += size;
		shm_base_addr[offset].QueueStatsShm.totalNumberOfMessages += 1;
	} else {
		/*log the error */
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : mqnd_shm_queue_ckpt_section_invalidate
 *
 * Description   : Function to invalidate the shared memory area of the queue 
 *
 * Arguments     : MQND_QUEUE_NODE *qnode
                   
 *
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t mqnd_shm_queue_ckpt_section_invalidate(MQND_CB *cb, MQND_QUEUE_NODE *qnode)
{
	uint32_t offset;

	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	if (!qnode) {
		LOG_ER("ERR_BAD_HANDLE: Queue Node is NULL");
		return NCSCC_RC_FAILURE;
	}

	offset = qnode->qinfo.shm_queue_index;
	if (shm_base_addr[offset].valid == SHM_QUEUE_INFO_VALID) {
		shm_base_addr[offset].valid = SHM_QUEUE_INFO_INVALID;
		memset((shm_base_addr + offset), '\0', sizeof(MQND_QUEUE_CKPT_INFO));
	}
	return NCSCC_RC_SUCCESS;
}
