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

#include <configmake.h>

/*****************************************************************************
..............................................................................

  MODULE NAME: pcs_rda_main.c

..............................................................................

  DESCRIPTION:  This file implements the RDA to interact with RDE. 

******************************************************************************
*/

/*
** Includes
*/
#include "rda.h"
#include <errno.h>
#include <sched.h>
#include "logtrace.h"
#include "osaf_poll.h"

/*
** Global data
*/
static void *pcs_rda_callback_cb = NULL;

/*
** Static functions
*/
static uint32_t rda_read_msg(int sockfd, char *msg, int size);
static uint32_t rda_write_msg(int sockfd, char *msg);
static uint32_t rda_parse_msg(const char *pmsg, RDE_RDA_CMD_TYPE *cmd_type, int *value);
static uint32_t rda_connect(int *sockfd);
static uint32_t rda_disconnect(int sockfd);
static uint32_t rda_callback_req(int sockfd);

/*****************************************************************************

  PROCEDURE NAME:       rda_callback_task

  DESCRIPTION:          Task main routine to perform callback mechanism
                        
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:                   

  NOTES:

*****************************************************************************/
static uint32_t rda_callback_task(RDA_CALLBACK_CB *rda_callback_cb)
{
	char msg[64] = { 0 };
	uint32_t rc = PCSRDA_RC_SUCCESS;
	int value = -1;
	int retry_count = 0;
	bool conn_lost = false;
	RDE_RDA_CMD_TYPE cmd_type = 0;
	PCS_RDA_CB_INFO cb_info;

	while (!rda_callback_cb->task_terminate) {
		/*
		 ** Escalate the issue if the connection with server is not  
		 ** established after certain retries.
		 */
		if (retry_count >= 100) {
			(*rda_callback_cb->callback_ptr) (rda_callback_cb->callback_handle,
							  &cb_info, PCSRDA_RC_FATAL_IPC_CONNECTION_LOST);

			break;
		}

		/*
		 ** Retry if connection with server is lost
		 */
		if (conn_lost == true) {
			m_NCS_TASK_SLEEP(1000);
			retry_count++;

			/*
			 ** Connect
			 */
			rc = rda_connect(&rda_callback_cb->sockfd);
			if (rc != PCSRDA_RC_SUCCESS) {
				continue;
			}

			/*
			 ** Send callback reg request messgae
			 */
			rc = rda_callback_req(rda_callback_cb->sockfd);
			if (rc != PCSRDA_RC_SUCCESS) {
				rda_disconnect(rda_callback_cb->sockfd);
				continue;
			}

			/*
			 ** Connection established
			 */
			retry_count = 0;
			conn_lost = false;
		}

		/*
		 ** Recv async role response
		 */
		rc = rda_read_msg(rda_callback_cb->sockfd, msg, sizeof(msg));
		if ((rc == PCSRDA_RC_TIMEOUT) || (rc == PCSRDA_RC_FATAL_IPC_CONNECTION_LOST)) {
			if (rc == PCSRDA_RC_FATAL_IPC_CONNECTION_LOST) {
				close(rda_callback_cb->sockfd);
				rda_callback_cb->sockfd = -1;
				conn_lost = true;
			}
			continue;
		} else if (rc != PCSRDA_RC_SUCCESS) {
			/* 
			 ** Escalate the problem to the application
			 */
			(*rda_callback_cb->callback_ptr) (rda_callback_cb->callback_handle, &cb_info, rc);
			continue;
		}

		rda_parse_msg(msg, &cmd_type, &value);
		if ((cmd_type != RDE_RDA_HA_ROLE) || (value < 0)) {
			/*TODO: What to do here - shankar */
			continue;
		}

		/*
		 ** Invoke callback
		 */
		cb_info.cb_type = PCS_RDA_ROLE_CHG_IND;
		cb_info.info.io_role = value;

		(*rda_callback_cb->callback_ptr) (rda_callback_cb->callback_handle, &cb_info, PCSRDA_RC_SUCCESS);
	}

	return rc;
}

/****************************************************************************
 * Name          : pcs_rda_reg_callback
 *
 * Description   : 
 *                 
 *
 * Arguments     : PCS_RDA_CB_PTR - Callback function pointer
 *
 * Return Values : 
 *
 * Notes         : None
 *****************************************************************************/
static int pcs_rda_reg_callback(uint32_t cb_handle, PCS_RDA_CB_PTR rda_cb_ptr, void **task_cb)
{
	uint32_t rc = PCSRDA_RC_SUCCESS;
	int sockfd = -1;
	bool is_task_spawned = false;
	RDA_CALLBACK_CB *rda_callback_cb = NULL;

	if (*task_cb != NULL)
		return PCSRDA_RC_CALLBACK_ALREADY_REGD;

	*task_cb = (long)0;

	/*
	 ** Connect
	 */
	rc = rda_connect(&sockfd);
	if (rc != PCSRDA_RC_SUCCESS) {
		return rc;
	}

	do {
		/*
		 ** Init leap
		 */
		if (ncs_leap_startup() != NCSCC_RC_SUCCESS) {

			rc = PCSRDA_RC_LEAP_INIT_FAILED;
			break;
		}

		/*
		 ** Send callback reg request messgae
		 */
		rc = rda_callback_req(sockfd);
		if (rc != PCSRDA_RC_SUCCESS) {
			break;
		}

		/*
		 ** Allocate callback control block
		 */
		rda_callback_cb = m_NCS_MEM_ALLOC(sizeof(RDA_CALLBACK_CB), 0, 0, 0);
		if (rda_callback_cb == NULL) {
			rc = PCSRDA_RC_MEM_ALLOC_FAILED;
			break;
		}

		memset(rda_callback_cb, 0, sizeof(RDA_CALLBACK_CB));
		rda_callback_cb->sockfd = sockfd;
		rda_callback_cb->callback_ptr = rda_cb_ptr;
		rda_callback_cb->callback_handle = cb_handle;
		rda_callback_cb->task_terminate = false;

		/*
		 ** Spawn task
		 */
	
		int policy = SCHED_OTHER; /*root defaults */
		int prio_val = sched_get_priority_min(policy);
		
		if (m_NCS_TASK_CREATE((NCS_OS_CB)rda_callback_task,
				      rda_callback_cb,
				      (char *)"OSAF_RDA",
				       prio_val, policy, NCS_STACKSIZE_HUGE, &rda_callback_cb->task_handle) != NCSCC_RC_SUCCESS) {

			m_NCS_MEM_FREE(rda_callback_cb, 0, 0, 0);
			rc = PCSRDA_RC_TASK_SPAWN_FAILED;
			break;
		}

		if (m_NCS_TASK_START(rda_callback_cb->task_handle) != NCSCC_RC_SUCCESS) {
			m_NCS_MEM_FREE(rda_callback_cb, 0, 0, 0);
			m_NCS_TASK_RELEASE(rda_callback_cb->task_handle);
			rc = PCSRDA_RC_TASK_SPAWN_FAILED;
			break;
		}

		is_task_spawned = true;
		*task_cb = rda_callback_cb;

	} while (0);

	/*
	 ** Disconnect
	 */
	if (!is_task_spawned) {
		ncs_leap_shutdown();
		rda_disconnect(sockfd);

	}

	/*
	 ** Done
	 */
	return rc;
}

/****************************************************************************
 * Name          : pcs_rda_unreg_callback
 *
 * Description   : 
 *                 
 *
 * Arguments     : PCS_RDA_CB_PTR - Callback function pointer
 *
 * Return Values : 
 *
 * Notes         : None
 *****************************************************************************/
static int pcs_rda_unreg_callback(void *task_cb)
{
	uint32_t rc = PCSRDA_RC_SUCCESS;
	RDA_CALLBACK_CB *rda_callback_cb = NULL;

	if (!task_cb)
		return rc;

	rda_callback_cb = (RDA_CALLBACK_CB *)task_cb;
	rda_callback_cb->task_terminate = true;

	/*
	 ** Stop task
	 */
	m_NCS_TASK_STOP(rda_callback_cb->task_handle);

	/*
	 ** Disconnect
	 */
	rda_disconnect(rda_callback_cb->sockfd);

	/*
	 ** Release task
	 */
	m_NCS_TASK_RELEASE(rda_callback_cb->task_handle);

	/*
	 ** Free memory
	 */
	m_NCS_MEM_FREE(rda_callback_cb, 0, 0, 0);

	/*
	 ** Destroy leap
	 */
	ncs_leap_shutdown();

	/*
	 ** Done
	 */
	return rc;
}

/****************************************************************************
 * Name          : pcs_rda_set_role
 *
 * Description   : 
 *                 
 *
 * Arguments     : role - HA role to set
 *
 * Return Values : 
 *
 * Notes         : None
 *****************************************************************************/
static int pcs_rda_set_role(PCS_RDA_ROLE role)
{
	uint32_t rc = PCSRDA_RC_SUCCESS;
	int sockfd;
	char msg[64] = { 0 };
	int value = -1;
	RDE_RDA_CMD_TYPE cmd_type = 0;

	/*
	 ** Connect
	 */
	rc = rda_connect(&sockfd);
	if (rc != PCSRDA_RC_SUCCESS) {
		return rc;
	}

	do {
		/*
		 ** Send set-role request messgae
		 */
		sprintf(msg, "%d %d", RDE_RDA_SET_ROLE_REQ, role);
		rc = rda_write_msg(sockfd, msg);
		if (rc != PCSRDA_RC_SUCCESS) {
			break;
		}

		/*
		 ** Recv set-role ack
		 */
		rc = rda_read_msg(sockfd, msg, sizeof(msg));
		if (rc != PCSRDA_RC_SUCCESS) {
			break;
		}

		rda_parse_msg(msg, &cmd_type, &value);
		if (cmd_type != RDE_RDA_SET_ROLE_ACK) {
			rc = PCSRDA_RC_ROLE_SET_FAILED;
			break;
		}

	} while (0);

	/*
	 ** Disconnect
	 */
	rda_disconnect(sockfd);

	/*
	 ** Done
	 */
	return rc;
}

/****************************************************************************
 * Name          : pcs_rda_get_role
 *
 * Description   : 
 *                 
 *
 * Arguments     : PCS_RDA_ROLE - pointer to variable to return init role
 *
 * Return Values : 
 *
 * Notes         : None
 *****************************************************************************/
static int pcs_rda_get_role(PCS_RDA_ROLE *role)
{
	uint32_t rc = PCSRDA_RC_SUCCESS;
	int sockfd;
	char msg[64] = { 0 };
	int value = -1;
	RDE_RDA_CMD_TYPE cmd_type = 0;

	*role = PCS_RDA_UNDEFINED;

	/*
	 ** Connect
	 */
	rc = rda_connect(&sockfd);
	if (rc != PCSRDA_RC_SUCCESS) {
		return rc;
	}

	do {
		/*
		 ** Send get-role request messgae
		 */
		sprintf(msg, "%d", RDE_RDA_GET_ROLE_REQ);
		rc = rda_write_msg(sockfd, msg);
		if (rc != PCSRDA_RC_SUCCESS) {
			break;
		}

		/*
		 ** Recv get-role response
		 */
		rc = rda_read_msg(sockfd, msg, sizeof(msg));
		if (rc != PCSRDA_RC_SUCCESS) {
			break;
		}

		rda_parse_msg(msg, &cmd_type, &value);
		if ((cmd_type != RDE_RDA_GET_ROLE_RES) || (value < 0)) {
			rc = PCSRDA_RC_ROLE_GET_FAILED;
			break;
		}

		/*
		 ** We have a role
		 */
		*role = value;

	} while (0);

	rda_disconnect(sockfd);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_get_control_block

  DESCRIPTION:          Singleton implementation for RDE Context
                     
  ARGUMENTS:

  RETURNS:

  NOTES:

*****************************************************************************/
static RDA_CONTROL_BLOCK *rda_get_control_block(void)
{
	static bool initialized = false;
	static RDA_CONTROL_BLOCK rda_cb;

	/*
	 ** Initialize CB
	 */
	if (!initialized) {
		initialized = true;
		memset(&rda_cb, 0, sizeof(rda_cb));

		/* Init necessary members */
		strncpy(rda_cb.sock_address.sun_path, RDE_RDA_SOCK_NAME, sizeof(rda_cb.sock_address.sun_path));
		rda_cb.sock_address.sun_family = AF_UNIX;
	}

	return &rda_cb;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_connect

  DESCRIPTION:          Open the socket associated with 
                        the RDA Interface 
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:   

  NOTES:

*****************************************************************************/
static uint32_t rda_connect(int *sockfd)
{
	RDA_CONTROL_BLOCK *rda_cb = rda_get_control_block();
	/*
	 ** Create the socket descriptor
	 */
	*sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return PCSRDA_RC_IPC_CREATE_FAILED;
	}

	/*
	 ** Connect to the server.
	 */
	if (connect(*sockfd, (struct sockaddr *)&rda_cb->sock_address, sizeof(rda_cb->sock_address)) < 0) {
		close(*sockfd);
		return PCSRDA_RC_IPC_CONNECT_FAILED;
	}

	return PCSRDA_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_disconnect

  DESCRIPTION:          Close the socket associated with 
                        the RDA Interface 
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:   

  NOTES:

*****************************************************************************/
static uint32_t rda_disconnect(int sockfd)
{
	char msg[64] = { 0 };

	/*
	 ** Format message
	 */
	sprintf(msg, "%d", RDE_RDA_DISCONNECT_REQ);

	if (rda_write_msg(sockfd, msg) != PCSRDA_RC_SUCCESS) {
		/* Nothing to do here */
	}

	/*
	 ** Close
	 */
	close(sockfd);

	return PCSRDA_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_callback_req

  DESCRIPTION:          Sends callback request to RDE

  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:

  NOTES:

*****************************************************************************/
static uint32_t rda_callback_req(int sockfd)
{
	char msg[64] = { 0 };
	uint32_t rc = PCSRDA_RC_SUCCESS;
	int value = -1;
	RDE_RDA_CMD_TYPE cmd_type = 0;

	/*
	 ** Send callback reg request messgae
	 */
	sprintf(msg, "%d", RDE_RDA_REG_CB_REQ);
	rc = rda_write_msg(sockfd, msg);
	if (rc != PCSRDA_RC_SUCCESS) {
		return rc;
	}

	/*
	 ** Recv callback reg response
	 */
	rc = rda_read_msg(sockfd, msg, sizeof(msg));
	if (rc != PCSRDA_RC_SUCCESS) {
		return rc;
	}

	rda_parse_msg(msg, &cmd_type, &value);
	if (cmd_type != RDE_RDA_REG_CB_ACK) {
		return PCSRDA_RC_CALLBACK_REG_FAILED;
	}

	return PCSRDA_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_write_msg

  DESCRIPTION:    Write a message to a peer on the socket
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:                   

  NOTES:

*****************************************************************************/
static uint32_t rda_write_msg(int sockfd, char *msg)
{
	int msg_size = 0;

	/*
	 ** Read from socket into input buffer
	 */

	msg_size = send(sockfd, msg, strlen(msg) + 1, 0);
	if (msg_size < 0) {
		return PCSRDA_RC_IPC_SEND_FAILED;
	}

	return PCSRDA_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_read_msg

  DESCRIPTION:    Read a complete line from the socket
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:                   

  NOTES:

*****************************************************************************/
static uint32_t rda_read_msg(int sockfd, char *msg, int size)
{
	int rc;
	int msg_size = 0;

	rc = osaf_poll_one_fd(sockfd, 30000);
	if (rc < 0) {
		if (errno == EPIPE) return PCSRDA_RC_FATAL_IPC_CONNECTION_LOST;
		LOG_ER("poll: PCSRDA_RC_IPC_RECV_FAILED: rc=%d-%s\n", errno, strerror(errno));
		return PCSRDA_RC_IPC_RECV_FAILED;
	}

	if (rc == 0) {
		/*
		 ** Timed out
		 */
		return PCSRDA_RC_TIMEOUT;
	}

	/*
	 ** Read from socket into input buffer
	 */
	msg_size = recv(sockfd, msg, size, 0);
	if (msg_size < 0) {
		LOG_ER("recv: PCSRDA_RC_IPC_RECV_FAILED: rc=%d-%s\n", errno, strerror(errno));
		return PCSRDA_RC_IPC_RECV_FAILED;
	}

	/*
	 ** Is connection shutdown by server?
	 */
	if (msg_size == 0) {
		/*
		 ** Yes
		 */
		return PCSRDA_RC_FATAL_IPC_CONNECTION_LOST;
	}

	return PCSRDA_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:       rda_parse_msg

  DESCRIPTION:    
                     
  ARGUMENTS:

  RETURNS:

    PCSRDA_RC_SUCCESS:                   

  NOTES:

*****************************************************************************/
static uint32_t rda_parse_msg(const char *pmsg, RDE_RDA_CMD_TYPE *cmd_type, int *value)
{
	char msg[64] = { 0 };
	char *ptr;

	strcpy(msg, pmsg);
	*value = -1;
	*cmd_type = RDE_RDA_UNKNOWN;

	/*
	 ** Parse the message for cmd type and value
	 */
	ptr = strchr(msg, ' ');
	if (ptr == NULL) {
		*cmd_type = atoi(msg);

	} else {
		*ptr = '\0';
		*cmd_type = atoi(msg);
		*value = atoi(++ptr);
	}

	return PCSRDA_RC_SUCCESS;
}

uint32_t rda_get_role(SaAmfHAStateT *ha_state)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	PCS_RDA_ROLE role;

	if (pcs_rda_get_role(&role) != PCSRDA_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	switch (role) {
	case PCS_RDA_ACTIVE:
		*ha_state = SA_AMF_HA_ACTIVE;
		break;
	case PCS_RDA_STANDBY:
		*ha_state = SA_AMF_HA_STANDBY;
		break;
	default:
		return NCSCC_RC_FAILURE;
	}

done:
	return rc;
}

uint32_t rda_register_callback(uint32_t cb_handle, PCS_RDA_CB_PTR rda_cb_ptr)
{
	if (pcs_rda_reg_callback(cb_handle, rda_cb_ptr, &pcs_rda_callback_cb) == PCSRDA_RC_SUCCESS)
		return NCSCC_RC_SUCCESS;
	else
		return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : pcs_rda_request
 *
 * Description   : This is the single entry API function is used to 
 *                 initialize/destroy the RDA shared library and  to interact 
 *                 with RDA to register callback function, set HA role and to 
 *                 get HA role. 
 *                 
 *
 * Arguments     : pcs_rda_req - Pointer to a structure of type PCS_RDA_REQ 
 *                               containing information pertaining to the request. 
 *                               This structure could be a staticallly allocated 
 *                               or dynamically allocated strcuture. If it is 
 *                               dynamically allocated it is the user's responsibility 
 *                               to free it. . 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
int pcs_rda_request(PCS_RDA_REQ *pcs_rda_req)
{
	int ret = PCSRDA_RC_SUCCESS;

	switch (pcs_rda_req->req_type) {

	case PCS_RDA_LIB_INIT:
		break;

	case PCS_RDA_LIB_DESTROY:
		ret = pcs_rda_unreg_callback(pcs_rda_callback_cb);
		break;

	case PCS_RDA_REGISTER_CALLBACK:
		ret = pcs_rda_reg_callback(pcs_rda_req->callback_handle,
			pcs_rda_req->info.call_back, &pcs_rda_callback_cb);
		break;

	case PCS_RDA_UNREGISTER_CALLBACK:
		ret = pcs_rda_unreg_callback(pcs_rda_callback_cb);
		break;

	case PCS_RDA_SET_ROLE:
		ret = pcs_rda_set_role(pcs_rda_req->info.io_role);
		break;

	case PCS_RDA_GET_ROLE:
		ret = pcs_rda_get_role(&pcs_rda_req->info.io_role);
		break;

	default:
		ret = PCSRDA_RC_INVALID_PARAMETER;

	}			/* switch */

	return ret;
}

