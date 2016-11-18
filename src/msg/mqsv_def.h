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

 FILE NAME: mqsv_def.h

..............................................................................

  DESCRIPTION:
  This file consists of constats, enums and data structures commonly used in
  MQSV .c & .h
******************************************************************************/

#ifndef MSG_MQSV_DEF_H_
#define MSG_MQSV_DEF_H_

#define MQSV_MQND_SHM_VERSION        1
#define MQSV_MSG_VERSION             1

#define MQSV_COMP_NAME_SIZE   10	/* Need to move to proper place */
#define MQSV_MSG_OVERHEAD 8192	/* added as a Quick fix for SA_AIS_ERR_QUEUE_FULL Problem */

typedef enum {
	MQSV_OBJ_QUEUE = 1,	/* Queue */
	MQSV_OBJ_QGROUP,	/* Queue Group */
} MQSV_OBJ_TYPE;

/* Enum for defining queue object types */
typedef enum {
	MQSV_QUEUE_OWN_STATE_ORPHAN = 1,
	MQSV_QUEUE_OWN_STATE_OWNED = 2,
	MQSV_QUEUE_OWN_STATE_PROGRESS = 3
} MQSV_QUEUE_OWN_STATE;

/* The enum SaMsgQueueSendingStateT is withdrawn from B.1.1, i
   As MQSv is using this internally, This structure is defined here */
typedef enum {
	MSG_QUEUE_UNAVAILABLE = 1,
	MSG_QUEUE_AVAILABLE = 2
} SaMsgQueueSendingStateT;

typedef struct mqsv_send_info {
	MDS_SVC_ID to_svc;	/* The service at the destination */
	MDS_SENDTYPES stype;	/* Send type */
	MDS_SYNC_SND_CTXT ctxt;	/* MDS Opaque context */
	MDS_DEST dest;		/* Who to send */
} MQSV_SEND_INFO;

typedef struct mqsv_dsend_info {
	MDS_SVC_ID to_svc;	/* The service at the destination */
	MDS_SENDTYPES stype;	/* Send type */
	MDS_DEST dest;		/* Who to send */
	MDS_SYNC_SND_CTXT ctxt;	/* MDS Opaque context */
	uint8_t padding[3];	/*Req for proper alignment in 32/64 arch */
} MQSV_DSEND_INFO;

/*** Macro used to get the AMF version used ****/
#define m_MQSV_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;

#define m_MQSV_IS_ACKFLAGS_NOT_VALID(ackFlags)         \
             ((ackFlags) && ((ackFlags) != SA_MSG_MESSAGE_DELIVERED_ACK))

#endif  // MSG_MQSV_DEF_H_
