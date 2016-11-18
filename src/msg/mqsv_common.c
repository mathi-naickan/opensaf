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

  DESCRIPTION: This file inclused following routines:

  machineEndianness() .........function to determine endianess

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "msg/mqsv.h"

/******************************** LOCAL ROUTINES *****************************/
/*****************************************************************************/

/*****************************************************************************
  PROCEDURE NAME: machineEndianness

  DESCRIPTION: Function which identifies the endianess of a machine

  ARGUMENTS: None

  RETURNS: ENDIAN_TYPE
*****************************************************************************/
uint32_t machineEndianness()
{
	uint32_t i = 1;
	char *p = (char *)&i;

	if (p[0] == 1)		/* Lowest address contains the least significant byte */
		return MQSV_LITTLE_ENDIAN;
	else
		return MQSV_BIG_ENDIAN;
}

/****************************************************************************
 * Function Name: mqsv_listenerq_msg_send
 * Purpose: Used to Send the 1-byte message to listener queue
 * Return Value:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 ****************************************************************************/
uint32_t mqsv_listenerq_msg_send(SaMsgQueueHandleT listenerHandle)
{
	NCS_OS_POSIX_MQ_REQ_INFO info;
	NCS_OS_MQ_MSG mq_msg;
	uint32_t actual_qsize = 0, actual_qused = 0;
	/* No listener queue present, return */
	if (!listenerHandle)
		return NCSCC_RC_SUCCESS;

	/* Get actual queue size and usage stats. This is needed to determine if queue size needs to be increased */
	memset(&info, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
	info.req = NCS_OS_POSIX_MQ_REQ_GET_ATTR;
	info.info.attr.i_mqd = listenerHandle;

	if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	actual_qsize = info.info.attr.o_attr.mq_maxmsg;
	actual_qused = info.info.attr.o_attr.mq_msgsize;

	if ((actual_qsize - actual_qused) < 1000) {
		memset(&info, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
		info.req = NCS_OS_POSIX_MQ_REQ_RESIZE;
		info.info.resize.mqd = listenerHandle;

		/* Increase queue size so that its able to hold 5 such messages */
		info.info.resize.i_newqsize = actual_qsize + 10000;

		if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;
	}

	memset(&mq_msg, 0, sizeof(NCS_OS_MQ_MSG));
	memcpy(mq_msg.data, "A", 1);

	memset(&info, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
	info.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
	info.info.send.mqd = listenerHandle;
	info.info.send.datalen = 1;
	info.info.send.i_msg = &mq_msg;
	info.info.send.i_mtype = 1;

	if (m_NCS_OS_POSIX_MQ(&info) != NCSCC_RC_SUCCESS)
		return (NCSCC_RC_FAILURE);

	return NCSCC_RC_SUCCESS;
}

/********************************************************************************
 Name    :  mqsv_get_phy_slot_id

 Description :  To get the physical slot id from the node id

 Arguments   :

*************************************************************************************/

SlotSubslotId mqsv_get_phy_slot_id(MDS_DEST dest)
{
	return GetSlotSubslotIdFromNodeId(m_NCS_NODE_ID_FROM_MDS_DEST(dest));
}
