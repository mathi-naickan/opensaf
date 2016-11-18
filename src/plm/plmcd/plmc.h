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
 * PLMc header file
 *
 * This header file is used by the plmcd, and plmc_lib
 * implementations.  Users of the PLMc library should not include
 * this header file.  Users of the PLmc library should instead 
 * include the plmc_lib.h header file.
 *
 */

#ifndef PLM_PLMCD_PLMC_H_
#define PLM_PLMCD_PLMC_H_

/* Success and Failure return codes. */
#define PLMC_EXIT_SUCCESS	0
#define PLMC_EXIT_FAILURE	1

/* Socket timeout values */
#define SOCK_KEEPALIVE 	0
#define KEEPIDLE_TIME	7200
#define KEEPALIVE_INTVL 75
#define KEEPALIVE_PROBES 9


/* Tag value and message data lengths. */
#define PLMC_MAX_TAG_LEN	256
#define PLMC_MAX_PID_LEN	128
#define PLMC_MAX_SERVICES 	128
#define PLMC_MAX_UDP_DATA_LEN   1024
#define PLMC_MAX_TCP_DATA_LEN   1024

/* Default timeout values for PLMc daemon operations. */
#define PLMC_TCP_TIMEOUT_SECS			5
#define PLMC_CHILD_THREAD_WATCHDOG_PERIOD_SECS	2

/* Command return values. */
#define PLMC_ACK_MSG_RECD	"PLMC_ACK_MSG_RECD"
#define PLMC_ACK_SUCCESS	"PLMC_ACK_SUCCESS"
#define PLMC_ACK_FAILURE	"PLMC_ACK_FAILURE"
#define PLMC_ACK_TIMEOUT	"PLMC_ACK_TIMEOUT"

/* Default location of the plmcd.conf configuration file. */
#define PLMC_CONFIG_FILE_LOC	"/etc/plmcd.conf"

/* Default location of the plmc daemon pid file. */
#define PLMCD_PID	"/var/run/plmcd.pid"

/* UDP message strings. */
#define PLMC_BOOT_START_MSG	"EE_INSTANTIATING"
#define PLMC_BOOT_STOP_MSG	"EE_TERMINATED"
#define PLMC_D_START_MSG	"EE_INSTANTIATED"
#define PLMC_D_STOP_MSG		"EE_TERMINATING"

/* These are the start/stop actions that the PLMc daemon has implemented. */
typedef enum
{
    PLMC_START = 1,
    PLMC_STOP = 2,
} PLMC_action;

/* These are the numerical values of the tags found in the plmcd.conf */
/* configuration file.                                               */
typedef enum
{
    PLMC_EE_ID = 1,
    PLMC_MSG_PROTOCOL_VERSION,
    PLMC_CONTROLLER_1_IP,
    PLMC_CONTROLLER_2_IP,
    PLMC_SERVICES,
    PLMC_OSAF,
    PLMC_TCP_PLMS_LISTENING_PORT,
    PLMC_UDP_BROADCAST_PORT,
    PLMC_OS_TYPE,
    PLMC_CMD_TIMEOUT_SECS,
    PLMC_OS_REBOOT_CMD,
    PLMC_OS_SHUTDOWN_CMD,
    SKEEPALIVE,
    TCP_KEEPIDLE_TIME,
    TCP_KEEPALIVE_INTVL,
    TCP_KEEPALIVE_PROBES,
} PLMC_config_tags;

/* This struct holds the contents of the plmcd.conf configuration file. */
typedef struct {
    char ee_id[PLMC_MAX_TAG_LEN];
    char msg_protocol_version[PLMC_MAX_TAG_LEN];
    char controller_1_ip[PLMC_MAX_TAG_LEN];
    char controller_2_ip[PLMC_MAX_TAG_LEN];
    int  num_services;
    char services[PLMC_MAX_SERVICES][PLMC_MAX_TAG_LEN];
    char osaf[PLMC_MAX_TAG_LEN];
    char tcp_plms_listening_port[PLMC_MAX_TAG_LEN];
    char udp_broadcast_port[PLMC_MAX_TAG_LEN];
    char os_type[PLMC_MAX_TAG_LEN];
    int  plmc_cmd_timeout_secs;
    char os_reboot_cmd[PLMC_MAX_TAG_LEN];
    char os_shutdown_cmd[PLMC_MAX_TAG_LEN];
    int so_keepalive;
    int tcp_keepidle_time;
    int tcp_keepalive_intvl;
    int tcp_keepalive_probes;
} PLMC_config_data;

/* The PLMC daemon command numerical index. */
typedef enum
{
  PLMC_NOOP_CMD = 0,
  PLMC_GET_ID_CMD = 1,
  PLMC_GET_PROTOCOL_VER_CMD = 2,
  PLMC_GET_OSINFO_CMD = 3,
  PLMC_SA_PLM_ADMIN_UNLOCK_CMD = 4,
  PLMC_SA_PLM_ADMIN_LOCK_INSTANTIATION_CMD = 5,
  PLMC_SA_PLM_ADMIN_LOCK_CMD = 6,
  PLMC_SA_PLM_ADMIN_RESTART_CMD = 7,
  PLMC_OSAF_START_CMD = 8,
  PLMC_OSAF_STOP_CMD = 9,
  PLMC_OSAF_SERVICES_START_CMD = 10,
  PLMC_OSAF_SERVICES_STOP_CMD = 11,
  PLMC_PLMCD_RESTART_CMD = 12,
  PLMC_LAST_CMD = 12
} PLMC_cmd_idx;

#endif  // PLM_PLMCD_PLMC_H_
