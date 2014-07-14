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

  DESCRIPTION:

  This file contains AVND specific constants, timeout values & return codes.
  
******************************************************************************
*/

#ifndef AVND_DEFS_H
#define AVND_DEFS_H

/****************************************************************************
                        AVND Error Return Codes 
*****************************************************************************/
/* TBD Currently putting failure return codes used by routines in AVND
 * Also fix the range of these error values. It'd help if these error values
 * are unique across AvSv or even entire NCS.
 */
#define  AVND_ERR            1000
#define  AVND_ERR_NO_MEMORY  AVND_ERR+1
#define  AVND_ERR_TREE       AVND_ERR+2
#define  AVND_ERR_DLL        AVND_ERR+3
#define  AVND_ERR_HDL        AVND_ERR+4
#define  AVND_ERR_NO_SU      AVND_ERR+5
#define  AVND_ERR_DUP_SU     AVND_ERR+6
#define  AVND_ERR_NO_COMP    AVND_ERR+7
#define  AVND_ERR_DUP_COMP   AVND_ERR+8
#define  AVND_ERR_NO_SI      AVND_ERR+9
#define  AVND_ERR_DUP_SI     AVND_ERR+10
#define  AVND_ERR_NO_CSI     AVND_ERR+11
#define  AVND_ERR_DUP_CSI    AVND_ERR+12
#define  AVND_ERR_NO_HC      AVND_ERR+13
#define  AVND_ERR_DUP_HC     AVND_ERR+14
#define  AVND_ERR_NO_PG      AVND_ERR+15
#define  AVND_ERR_DUP_PG     AVND_ERR+16

/****************************************************************************
                        Timeout Values (in millisecs) 
*****************************************************************************/

#define AVND_COMP_CBK_RESP_TIME       5000	/* time out callback response */
#define AVND_AVD_MSG_RESP_TIME   1000	/* time out AvD message response */

#define m_AVND_STACKSIZE       NCS_STACKSIZE_HUGE

typedef enum {
	AVND_TERM_STATE_UP = 0,
	AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED,
	AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED,
	AVND_TERM_STATE_NODE_FAILOVER_TERMINATING,
	AVND_TERM_STATE_NODE_FAILOVER_TERMINATED,
	AVND_TERM_STATE_NODE_SWITCHOVER_STARTED,
	AVND_TERM_STATE_MAX
} AVND_TERM_STATE;

typedef enum {
	AVND_LED_STATE_RED = 0,
	AVND_LED_STATE_GREEN,
	AVND_LED_STATE_MAX
} AVND_LED_STATE;

#define m_AVND_IS_SHUTTING_DOWN(x) \
	((AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED == (x)->term_state) || \
	 (AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED == (x)->term_state))

#endif   /* !AVND_DEFS_H */
