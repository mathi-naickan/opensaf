/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
 * Author(s): Hewlett-Packard Development Company, L.P.
 *
 */

/*
	libplmc_lib.h:
	routines in libplmc_lib.so
*/

#ifndef PLMC_LIB_H
#define PLMC_LIB_H

#include <plmc.h>

/* CHANGE - add ifdefs */
#define PLMC_EE_ID_MAX_LENGTH		256
#define PLMC_CMD_NAME_MAX_LENGTH	256
#define PLMC_CMD_RESULT_MAX_LENGTH	256

#define PLMC_MAX_ERROR_MSG_LENGTH	128
#define PLMC_MAX_ACTION_LENGTH		128


/* Error Callback Codes */
#define PLMC_LIBERR_MISC_ERROR		0
#define PLMC_LIBERR_SYSTEM_RESOURCES	1
#define PLMC_LIBERR_LOST_CONNECTION	2
#define PLMC_LIBERR_MSG_INVALID		3
#define PLMC_LIBERR_TIMEOUT		4
#define PLMC_LIBERR_NO_CONF		5
#define PLMC_LIBERR_ERR_CB		6
#define PLMC_LIBERR_INTERNAL		7


/* Action Codes */
#define PLMC_LIBACT_UNDEFINED		0
#define PLMC_LIBACT_CLOSE_SOCKET	1
#define PLMC_LIBACT_EXIT_THREAD		2
#define PLMC_LIBACT_DESTROY_LIBRARY	3
#define PLMC_LIBACT_IGNORING		4

/* Error API Codes */
#define PLMC_API_SUCCESS		0
#define PLMC_API_FAILURE		1
#define PLMC_API_EEID_NOT_FOUND		2
#define PLMC_API_LOCK_FAILED		3
#define PLMC_API_UNLOCK_FAILED		4
#define PLMC_API_CLIENT_BUSY		5
#define PLMC_API_NO_CONF		6
#define PLMC_API_NOT_CONNECTED		7

/********************************************************************
* This enum is for UDP messages that can come in over the UDP 
* channel
********************************************************************/
typedef enum {
	EE_INSTANTIATING,
	EE_TERMINATED,
	EE_INSTANTIATED,
	EE_TERMINATING
} udp_msg_idx;

/********************************************************************
* This struct is used for the error callback function the user has
* to register when initializing the library
*
********************************************************************/
typedef struct plmc_lib_error_struct
{
	int ecode; /* error code */
	int acode; /* action code */
	char ee_id[PLMC_EE_ID_MAX_LENGTH];
	char errormsg[PLMC_MAX_ERROR_MSG_LENGTH];
	char action[PLMC_MAX_ACTION_LENGTH];
	PLMC_cmd_idx cmd_enum;
} plmc_lib_error;


/********************************************************************
* This struct is used for the UDP callback function the user has
* to register when initializing the library
*
********************************************************************/

typedef struct udp_msg_struct {
	char ee_id[PLMC_EE_ID_MAX_LENGTH];
	char msg[PLMC_CMD_RESULT_MAX_LENGTH];
	char os_info[PLMC_CMD_RESULT_MAX_LENGTH];
	udp_msg_idx msg_idx;
} udp_msg;

/********************************************************************
* This struct is used for the command callback function the user has
* to register when initializing the library
*
********************************************************************/
typedef struct tcp_msg_struct {
	char ee_id[PLMC_EE_ID_MAX_LENGTH];
	char cmd[PLMC_CMD_NAME_MAX_LENGTH];
	char result[PLMC_CMD_RESULT_MAX_LENGTH];
	PLMC_cmd_idx cmd_enum;
} tcp_msg;


int plmc_initialize(int (*connect_cb)(char *, char *), 
		int (*udp_cb)(udp_msg *), int (*err_cb)(plmc_lib_error *));
int plmc_get_protocol_ver(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_get_os_info(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_sa_plm_admin_unlock(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_sa_plm_admin_lock_instantiation(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_sa_plm_admin_lock(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_sa_plm_admin_restart(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_osaf_start(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_osaf_stop(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_osaf_services_start(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_osaf_services_stop(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_plmcd_restart(char *ee_id, int( *cb)(tcp_msg  *));
int plmc_destroy();
#endif  /* PLMC_LIB_H */
