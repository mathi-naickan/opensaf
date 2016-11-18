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

  DESCRIPTION: Common MBCSv sub-part messaging formats that travels between
  MBCSv agent peers.


  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */
#ifndef MBC_MBCSV_MEM_H_
#define MBC_MBCSV_MEM_H_

#include "base/ncssysf_mem.h"

#define MBCSV_MEM_MBCSV_EVT     1
#define MBCSV_MEM_REG_INFO      2
#define MBCSV_MEM_CKPT_INST     3
#define MBCSV_MEM_PEER_INFO     4
#define MBCSV_MEM_MDS_REG       5
#define MBCSV_MEM_SVC_REG       6
#define MBCSV_MEM_PEER_LIST     7

/* Allocate and free MBCSV message */
#define m_MMGR_ALLOC_MBCSV_EVT      (MBCSV_EVT *)m_NCS_MEM_ALLOC(sizeof(MBCSV_EVT), \
                                                                 NCS_MEM_REGION_TRANSIENT, \
                                                                 NCS_SERVICE_ID_MBCSV, MBCSV_MEM_MBCSV_EVT)

#define m_MMGR_FREE_MBCSV_EVT(p)     m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                    NCS_SERVICE_ID_MBCSV, MBCSV_MEM_MBCSV_EVT)

/* Allocate and free MBCSV registration entry */
#define m_MMGR_ALLOC_REG_INFO      (MBCSV_REG *)m_NCS_MEM_ALLOC(sizeof(MBCSV_REG), \
                                                                NCS_MEM_REGION_TRANSIENT, \
                                                                NCS_SERVICE_ID_MBCSV, MBCSV_MEM_REG_INFO)

#define m_MMGR_FREE_REG_INFO(p)     m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                   NCS_SERVICE_ID_MBCSV, MBCSV_MEM_REG_INFO)

/* Allocate and free checkpoint instance */
#define m_MMGR_ALLOC_CKPT_INST      (CKPT_INST *)m_NCS_MEM_ALLOC(sizeof(CKPT_INST), \
                                                                 NCS_MEM_REGION_TRANSIENT, \
                                                                 NCS_SERVICE_ID_MBCSV, MBCSV_MEM_CKPT_INST)

#define m_MMGR_FREE_CKPT_INST(p)     m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                    NCS_SERVICE_ID_MBCSV, MBCSV_MEM_CKPT_INST)

/* Allocate and free Peer instance */
#define m_MMGR_ALLOC_PEER_INFO      (PEER_INST *)m_NCS_MEM_ALLOC(sizeof(PEER_INST), \
                                                                 NCS_MEM_REGION_TRANSIENT, \
                                                                 NCS_SERVICE_ID_MBCSV, MBCSV_MEM_PEER_INFO)

#define m_MMGR_FREE_PEER_INFO(p)     m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                    NCS_SERVICE_ID_MBCSV, MBCSV_MEM_PEER_INFO)

/* Allocate and free MDS reg info */
#define m_MMGR_ALLOC_MBX_INFO      (MBCSV_MBX_INFO *)m_NCS_MEM_ALLOC(sizeof(MBCSV_MBX_INFO), \
                                                                     NCS_MEM_REGION_TRANSIENT, \
                                                                     NCS_SERVICE_ID_MBCSV, MBCSV_MEM_MDS_REG)

#define m_MMGR_FREE_MBX_INFO(p)     m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                   NCS_SERVICE_ID_MBCSV, MBCSV_MEM_MDS_REG)

/* Allocate and free Peer list info  */
#define m_MMGR_ALLOC_PEER_LIST_IN   (MBCSV_PEER_LIST *)m_NCS_MEM_ALLOC(sizeof(MBCSV_PEER_LIST), \
                                                                       NCS_MEM_REGION_TRANSIENT, \
                                                                       NCS_SERVICE_ID_MBCSV, MBCSV_MEM_PEER_LIST)

#define m_MMGR_FREE_PEER_LIST_IN(p)  m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                    NCS_SERVICE_ID_MBCSV, MBCSV_MEM_PEER_LIST)

#endif  // MBC_MBCSV_MEM_H_
