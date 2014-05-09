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
 **             Operating System Task Premption Lock macros                 **
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
 ** ncs_os_get_time_stamp:      Return the current timestamp as "time_t" in **
 **                             the argument tod.                           **
 **                                                                         **
 ****************************************************************************/
#define m_GET_TIME_STAMP(timestamp) \
    m_NCS_OS_GET_TIME_STAMP(timestamp)

/*****************************************************************************
 **                                                                         **
 **             LEAP Debug conditional compile stuff                        **
 **                                                                         **
 ****************************************************************************/

#define m_KEY_CHK_FMT(k,f)  { if (k.fmat != f) m_LEAP_DBG_SINK_VOID;}
#define m_KEY_CHK_LEN(l)    { if (l > SYSF_MAX_KEY_LEN) m_LEAP_DBG_SINK_VOID; }
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
