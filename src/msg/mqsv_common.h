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
  
  MQSv event definitions.
    
******************************************************************************
*/
#ifndef MSG_MQSV_COMMON_H_
#define MSG_MQSV_COMMON_H_

#define MQSV_LITTLE_ENDIAN 0
#define MQSV_BIG_ENDIAN    1

#define MQSV_DSEND_EVENT 1	/*Direct Send Event */
#define MQSV_NOT_DSEND_EVENT 0	/*Not a Direct Send Event */

#define m_MQSV_REVERSE_ENDIAN_LL(p8, endianness) (        \
   (!endianness ? (                            \
      ((uint64_t)((uint8_t*)(p8))[0] <<56) |        \
      ((uint64_t)((uint8_t*)(p8))[1] <<48) |        \
      ((uint64_t)((uint8_t*)(p8))[2] <<40) |        \
      ((uint64_t)((uint8_t*)(p8))[3] <<32) |        \
      ((uint64_t)((uint8_t*)(p8))[4] <<24) |        \
      ((uint64_t)((uint8_t*)(p8))[5] <<16) |        \
      ((uint64_t)((uint8_t*)(p8))[6] <<8 ) |        \
      ((uint64_t)((uint8_t*)(p8))[7]     )          \
     )                                        \
    :(                                        \
      ((uint64_t)((uint8_t*)(p8))[7] <<56) |        \
      ((uint64_t)((uint8_t*)(p8))[6] <<48) |        \
      ((uint64_t)((uint8_t*)(p8))[5] <<40) |        \
      ((uint64_t)((uint8_t*)(p8))[4] <<32) |        \
      ((uint64_t)((uint8_t*)(p8))[3] <<24) |        \
      ((uint64_t)((uint8_t*)(p8))[2] <<16) |        \
      ((uint64_t)((uint8_t*)(p8))[1] <<8 ) |        \
      ((uint64_t)((uint8_t*)(p8))[0]     )          \
     )                                        \
   ))

#define m_MQSV_REVERSE_ENDIAN_L(p8, endianness) (        \
   (!endianness ? (                            \
      ((uint32_t)((uint8_t*)(p8))[0] <<24) |        \
      ((uint32_t)((uint8_t*)(p8))[1] <<16) |        \
      ((uint32_t)((uint8_t*)(p8))[2] <<8 ) |        \
      ((uint32_t)((uint8_t*)(p8))[3]     )          \
     )                                        \
    :(                                        \
      ((uint32_t)((uint8_t*)(p8))[3] <<24) |        \
      ((uint32_t)((uint8_t*)(p8))[2] <<16) |        \
      ((uint32_t)((uint8_t*)(p8))[1] <<8 ) |        \
      ((uint32_t)((uint8_t*)(p8))[0]     )          \
     )                                        \
   ))

#define m_MQSV_REVERSE_ENDIAN_S(p8, endianness) (        \
   (!endianness ? (                            \
      ((uint16_t)((uint8_t*)(p8))[0] <<8 ) |        \
      ((uint16_t)((uint8_t*)(p8))[1]     )          \
     )                                        \
    :(                                        \
      ((uint16_t)((uint8_t*)(p8))[1] <<8 ) |        \
      ((uint16_t)((uint8_t*)(p8))[0]     )          \
     )                                        \
   ))

uint32_t machineEndianness(void);
uint32_t mqsv_listenerq_msg_send(SaMsgQueueHandleT listenerhandle);
SlotSubslotId mqsv_get_phy_slot_id(MDS_DEST dest);
typedef unsigned int MSG_FRMT_VER;

#define m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(t)      (t)/(10000000)	/* 10^7 */
/* New categories Specific to MQSV Logging Used by MQA/MQD/MQND */
#define NCSFL_LC_MQSV_INIT       0x00008000	/* MQSV Liblifecycle logging */
#define NCSFL_LC_MQSV_Q_MGMT     0x00004000	/* MQSV Send Receive flow logging */
#define NCSFL_LC_MQSV_QGRP_MGMT  0x00002000	/* MQSV Group Apis flows */
#define NCSFL_LC_MQSV_SEND_RCV   0x00001000	/* MQSV New Logs */

#endif  // MSG_MQSV_COMMON_H_
