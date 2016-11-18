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

  DESCRIPTION: ASAPi messages Allocation & Free macros 
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef MSG_MSGD_MQD_MEM_H_
#define MSG_MSGD_MQD_MEM_H_

#include "base/ncssysf_mem.h"

/******************************************************************************
                Service Sub IDs for MQD macros
*******************************************************************************/
typedef enum {
	NCS_SERVICE_MQD_SUB_ID_MQD_CB,
	NCS_SERVICE_MQD_SUB_ID_MQD_OBJECT_ELEM,
	NCS_SERVICE_MQD_SUB_ID_MQD_OBJ_NODE,
	NCS_SERVICE_MQD_SUB_ID_MQD_TRACK_OBJ,
	NCS_SERVICE_MQD_SUB_ID_MQD_A2S_MSG,
	NCS_SERVICE_MQD_SUB_ID_MQD_DEFAULT_VAL,
	NCS_SERVICE_MQD_SUB_ID_MQD_ND_DB_NODE,
	NCS_SERVICE_MQD_SUB_ID_MAX	/* This should be the last id */
} NCS_SERVICE_MQSV_MQD_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                     Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_MQD_CB                  m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_CB)

#define m_MMGR_FREE_MQD_CB(p)                m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_CB)

#define m_MMGR_ALLOC_MQD_OBJECT_ELEM            m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_OBJECT_ELEM), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_OBJECT_ELEM)

#define m_MMGR_FREE_MQD_OBJECT_ELEM(p)         m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_OBJECT_ELEM)

#define m_MMGR_ALLOC_MQD_OBJ_NODE            m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_OBJ_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_OBJ_NODE)

#define m_MMGR_FREE_MQD_OBJ_NODE(p)          m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_OBJ_NODE)

#define m_MMGR_ALLOC_MQD_TRACK_OBJ            m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_TRACK_OBJ), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_TRACK_OBJ)

#define m_MMGR_FREE_MQD_TRACK_OBJ(p)          m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_TRACK_OBJ)

#define m_MMGR_ALLOC_MQD_A2S_MSG              m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_A2S_MSG), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_A2S_MSG)

#define m_MMGR_FREE_MQD_A2S_MSG(p)            m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_A2S_MSG)

#define m_MMGR_ALLOC_MQD_DEFAULT_VAL(mem_size)  m_NCS_MEM_ALLOC( \
                                                mem_size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_DEFAULT_VAL)

#define m_MMGR_FREE_MQD_DEFAULT_VAL(p)          if (p)  m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_DEFAULT_VAL)

#define m_MMGR_ALLOC_MQD_ND_DB_NODE             m_NCS_MEM_ALLOC( \
                                                sizeof(MQD_ND_DB_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_ND_DB_NODE)

#define m_MMGR_FREE_MQD_ND_DB_NODE(p)           m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQD, \
                                                NCS_SERVICE_MQD_SUB_ID_MQD_ND_DB_NODE)

#endif  // MSG_MSGD_MQD_MEM_H_
