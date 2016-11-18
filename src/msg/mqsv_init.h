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

 FILE NAME: mqnd_init.h

..............................................................................

  DESCRIPTION:
  This file consists of constats, enums and data structs used by mdnd_init.c
******************************************************************************/

#ifndef MSG_MQSV_INIT_H_
#define MSG_MQSV_INIT_H_

/* Macro to get the component name for the component type */
#define m_MQND_TASKNAME "MQND"

/* Macro for MQND task priority */
#define m_MQND_TASK_PRI (5)

/* Macro for MQND task stack size */
#define m_MQND_STACKSIZE NCS_STACKSIZE_HUGE

/*****************************************************************************
 * structure which holds the create information.
 *****************************************************************************/
typedef struct mqsv_create_info {
	uint8_t pool_id;		/* Handle manager Pool ID */
} MQSV_CREATE_INFO;

/*****************************************************************************
 * structure which holds the destroy information.
 *****************************************************************************/
typedef struct mqsv_destroy_info {
	MDS_VDEST_ID i_vcard_id;
} MQSV_DESTROY_INFO;

#endif  // MSG_MQSV_INIT_H_
