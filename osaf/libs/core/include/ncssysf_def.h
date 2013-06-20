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

  DESCRIPTION:

  This module contains target system specific declarations related to
  System "hooks" and other assorted defines.

*/

/*
 * Module Inclusion Control...
 */
#ifndef NCSSYSF_DEF_H
#define NCSSYSF_DEF_H

#include <time.h>

#include "ncsgl_defs.h"
#include "logtrace.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **                      Byte order conversions                             **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/
#define sysf_ntohl(x)     m_NCS_OS_NTOHL(x)
#define sysf_htonl(x)       m_NCS_OS_HTONL(x)
#define sysf_ntohs(s)       m_NCS_OS_NTOHS(s)
#define sysf_htons(s)       m_NCS_OS_HTONS(s)
/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **  Function Defines for byte order conversions with                       **
 **  autoincrement of the pointer                                           **
 **                                                                         **
 ****************************************************************************/

	uint32_t decode_32bitOS_inc(uint8_t **stream);
	uint32_t encode_32bitOS_inc(uint8_t **stream, uint32_t val);
	uint32_t encode_16bitOS_inc(uint8_t **stream, uint32_t val);
	uint16_t decode_16bitOS_inc(uint8_t **stream);

#define m_NCS_OS_HTONL_P_INC(p8,v32) encode_32bitOS_inc(&p8, v32)
#define m_NCS_OS_HTONS_P_INC(p8,v16) encode_16bitOS_inc(&p8, v16)
#define m_NCS_OS_NTOHL_P_INC(p8)     decode_32bitOS_inc(&p8)
#define m_NCS_OS_NTOHS_P_INC(p8)    decode_16bitOS_inc(&p8)

// should remove but require changes in a few service
#define m_NCS_DBG_PRINTF         printf

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **             Operating System Task Premption Lock macros                 **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/

#define m_INIT_CRITICAL                m_NCS_OS_INIT_TASK_LOCK
#define m_START_CRITICAL               m_NCS_OS_START_TASK_LOCK
#define m_END_CRITICAL                 m_NCS_OS_END_TASK_LOCK

/**
 *  Prepare for a future call to opensaf_reboot() by opening the necessary file
 *  (/proc/sysrq-trigger). Call this function before dropping root privileges
 *  (which is done by the daemonize() function in OpenSAF), if you later intend
 *  to call opensaf_reboot() to reboot the local node without having root
 *  privileges.
 *
 * NOTE: Do NOT call this function unless you have to (e.g. don't call it if
 *       your service is running as root, or if you only intend to use
 *       opensaf_reboot() to reboot non-local nodes). The reason is that is can
 *       be dangerous to hold an open file handle to /proc/sysrq-trigger, if a
 *       bug causes the service to write to the wrong file descriptor.
 */
void opensaf_reboot_prepare(void);

/**
 *  Reboot a node. Call this function with @a node_id zero to reboot the local
 *  node. If you intend to use this function to reboot the local node without
 *  having root privileges, you must first call opensaf_reboot_prepare() before
 *  dropping root privileges (which is done by the daemonize() function in
 *  OpenSAF).
 *
 *  Note that this function uses the configuration option
 *  OPENSAF_REBOOT_TIMEOUT in nid.conf. Therefore, this function must only be
 *  called from services that are started by NID.
 */
void opensaf_reboot(unsigned node_id, const char* ee_name, const char* reason);

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **                   System Timestamp Interface Macros                     **
 **                                                                         **
 ** ncs_os_get_time_stamp:       Return the current timestamp as "time_t" in **
 **                             the argument tod.                           **
 **                                                                         **
 ** ncs_os_get_ascii_time_stamp: Fetch the current timestamp (tod), as an    **
 **                             ascii string, in asc_tod. Note the size of  **
 **                             the ASCII string is limited to 32 octets.   **
 **                                                                         **
 ****************************************************************************/

#define m_GET_ASCII_TIME_STAMP(timestamp, asc_timestamp)  \
    m_NCS_OS_GET_ASCII_TIME_STAMP(timestamp, asc_timestamp)

#define m_NCS_DATE_TIME_TO_STR(timestamp, asc_timestamp)  \
    m_NCS_OS_DATE_TIME_TO_STR(timestamp, asc_timestamp)

#define m_NCS_TIME_TO_STR(timestamp, asc_timestamp)  \
    m_NCS_OS_TIME_TO_STR(timestamp, asc_timestamp)

#define m_GET_ASCII_DATE_TIME_STAMP(timestamp, asc_timestamp) \
    m_NCS_OS_GET_ASCII_DATE_TIME_STAMP(timestamp, asc_timestamp)

#define m_GET_ASCII_HDR_DATE_TIME_STAMP(timestamp, asc_timestamp) \
    m_NCS_OS_GET_ASCII_HDR_DATE_TIME_STAMP(timestamp, asc_timestamp)

#define m_GET_TIME_STAMP(timestamp) \
    m_NCS_OS_GET_TIME_STAMP(timestamp)

#define m_NCS_GET_TIME_MS  \
    m_NCS_OS_GET_TIME_MS

#define m_NCS_GET_TIME_NS  \
    m_NCS_OS_GET_TIME_NS

#define m_NCS_GET_UPTIME \
    m_NCS_OS_GET_UPTIME

#define m_GET_TIME_STAMP_STR(timestamp, asc_timestamp)  \
    m_NCS_OS_GET_TIME_STAMP_STR(timestamp, asc_timestamp)


/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **             LEAP Debug conditional compile stuff                        **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/

#define m_KEY_CHK_FMT(k,f)  { if (k.fmat != f) m_LEAP_DBG_SINK(0);}
#define m_KEY_CHK_LEN(l)    { if (l > SYSF_MAX_KEY_LEN) m_LEAP_DBG_SINK(0); }
#define m_KEY_CHK_SLEN(s)   { uint32_t l = m_NCS_STRLEN(s); m_KEY_CHK_LEN(l); }

/*
 * m_LEAP_DBG_SINK
 *
 * If LEAP fails an if-conditional or other test that we would not expect
 * under normal conditions, it will call this macro.
 *
 */
#define m_LEAP_DBG_SINK(r)	(TRACE("IN LEAP_DBG_SINK"), r)
#define m_LEAP_DBG_SINK_VOID	TRACE("IN LEAP_DBG_SINK")

/*****************************************************************************
 **                                                                         **
 **                   Task Priorities                                       **
 **                                                                         **
 ****************************************************************************/

#define NCS_TASK_PRIORITY_0               NCS_OS_TASK_PRIORITY_0
#define NCS_TASK_PRIORITY_1               NCS_OS_TASK_PRIORITY_1
#define NCS_TASK_PRIORITY_2               NCS_OS_TASK_PRIORITY_2
#define NCS_TASK_PRIORITY_3               NCS_OS_TASK_PRIORITY_3
#define NCS_TASK_PRIORITY_4               NCS_OS_TASK_PRIORITY_4
#define NCS_TASK_PRIORITY_5               NCS_OS_TASK_PRIORITY_5
#define NCS_TASK_PRIORITY_6               NCS_OS_TASK_PRIORITY_6
#define NCS_TASK_PRIORITY_7               NCS_OS_TASK_PRIORITY_7
#define NCS_TASK_PRIORITY_8               NCS_OS_TASK_PRIORITY_8
#define NCS_TASK_PRIORITY_9               NCS_OS_TASK_PRIORITY_9
#define NCS_TASK_PRIORITY_10              NCS_OS_TASK_PRIORITY_10
#define NCS_TASK_PRIORITY_11              NCS_OS_TASK_PRIORITY_11
#define NCS_TASK_PRIORITY_12              NCS_OS_TASK_PRIORITY_12
#define NCS_TASK_PRIORITY_13              NCS_OS_TASK_PRIORITY_13
#define NCS_TASK_PRIORITY_14              NCS_OS_TASK_PRIORITY_14
#define NCS_TASK_PRIORITY_15              NCS_OS_TASK_PRIORITY_15
#define NCS_TASK_PRIORITY_16              NCS_OS_TASK_PRIORITY_16

/*****************************************************************************
 **                                                                         **
 **                   Task Stack Size                                       **
 **                                                                         **
 ****************************************************************************/

/* (relative) stack size options... Most all tasks use MEDIUM */

#ifndef NCS_STACKSIZE_SMALL
#define NCS_STACKSIZE_SMALL    16000
#endif

#ifndef NCS_STACKSIZE_MEDIUM
#define NCS_STACKSIZE_MEDIUM   32000
#endif

#ifndef NCS_STACKSIZE_LARGE
#define NCS_STACKSIZE_LARGE    64000
#endif

#ifndef NCS_STACKSIZE_HUGE
#define NCS_STACKSIZE_HUGE     128000
#endif

#ifndef NCS_STACKSIZE_HUGEX2
#define NCS_STACKSIZE_HUGEX2   256000
#endif

#ifdef  __cplusplus
}
#endif
/*****************************************************************************
 **                                                                         **
 **             LEAP ENVIRONMENT INITIALIZATION AND CLEAN UP                **
 **                                                                         **
 ****************************************************************************/
    uint32_t leap_env_init(void);
uint32_t leap_env_destroy(void);

#endif
