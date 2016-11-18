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

  This file contains macros for CPD memory operations.
  
******************************************************************************
*/

#ifndef CKPT_CKPTD_CPD_MEM_H_
#define CKPT_CKPTD_CPD_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for CPD */
typedef enum {
	CPD_SVC_SUB_ID_CPD_CB = CPSV_SVC_SUB_ID_MAX + 1,
	CPD_SVC_SUB_ID_DEFAULT_VAL,
	CPD_SVC_SUB_ID_CPD_CKPT_INFO_NODE,
	CPD_SVC_SUB_ID_CPD_CKPT_MAP_INFO,
	CPD_SVC_SUB_ID_CPD_CKPT_REPLOC_INFO,
	CPD_SVC_SUB_ID_CPD_CPND_INFO_NODE,
	CPD_SVC_SUB_ID_CPSV_CPND_DEST_INFO,
	CPD_SVC_SUB_ID_CPD_NODE_REF_INFO,
	CPD_SVC_SUB_ID_CPD_CKPT_REF_INFO,
	CPD_SVC_SUB_ID_CPD_MBCSV_MSG,
	CPD_SVC_SUB_ID_CPD_A2S_CKPT_CREATE,
	CPD_SVC_SUB_ID_MAX
} CPD_SVC_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_CPD_CB               (CPD_CB*)m_NCS_MEM_ALLOC(sizeof(CPD_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CB)

#define m_MMGR_FREE_CPD_CB(p)             m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CB)

#define m_MMGR_ALLOC_CPD_DEFAULT(size)   m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_DEFAULT_VAL)

#define m_MMGR_FREE_CPD_DEFAULT(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_DEFAULT_VAL)
#define m_MMGR_ALLOC_CPD_CKPT_INFO_NODE                                       \
            (CPD_CKPT_INFO_NODE*)m_NCS_MEM_ALLOC(sizeof(CPD_CKPT_INFO_NODE), \
                                                NCS_MEM_REGION_PERSISTENT,    \
                                                NCS_SERVICE_ID_CPD,           \
                                                CPD_SVC_SUB_ID_CPD_CKPT_INFO_NODE)

#define m_MMGR_FREE_CPD_CKPT_INFO_NODE(p) 									\
							if(p->ckpt_name != 0) free((void*)p->ckpt_name);\
							m_NCS_MEM_FREE(p,								\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CKPT_INFO_NODE)

#define m_MMGR_ZERO_CPD_CKPT_INFO_NODE(p)					\
			if(p->ckpt_name != 0) free((void*)p->ckpt_name);\
			memset(p, 0, sizeof(CPD_CKPT_INFO_NODE))

#define m_MMGR_ALLOC_CPD_CKPT_MAP_INFO  (CPD_CKPT_MAP_INFO*)                  \
                                   m_NCS_MEM_ALLOC(sizeof(CPD_CKPT_MAP_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CKPT_MAP_INFO)

#define m_MMGR_FREE_CPD_CKPT_MAP_INFO(p) 						\
								if (p->ckpt_name != NULL)		\
									free((void *)p->ckpt_name);	\
								m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CKPT_MAP_INFO)

#define m_MMGR_ALLOC_CPD_CKPT_REPLOC_INFO (CPD_CKPT_REPLOC_INFO *)          \
                               m_NCS_MEM_ALLOC(sizeof(CPD_CKPT_REPLOC_INFO), \
                                        NCS_MEM_REGION_PERSISTENT, \
                                        NCS_SERVICE_ID_CPD, \
                                        CPD_SVC_SUB_ID_CPD_CKPT_REPLOC_INFO)

#define m_MMGR_FREE_CPD_CKPT_REPLOC_INFO(p)								\
								if (p->rep_key.node_name != 0)			\
									free((void *)p->rep_key.node_name);	\
								if (p->rep_key.ckpt_name != 0)			\
									free((void *)p->rep_key.ckpt_name);	\
								m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_CPD, \
                                            CPD_SVC_SUB_ID_CPD_CKPT_REPLOC_INFO)

#define m_MMGR_ALLOC_CPD_CPND_INFO_NODE  (CPD_CPND_INFO_NODE*)                \
                                   m_NCS_MEM_ALLOC(sizeof(CPD_CPND_INFO_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CPND_INFO_NODE)

#define m_MMGR_FREE_CPD_CPND_INFO_NODE(p)								\
								if (p->node_name != 0)			\
									free((void *)p->node_name);	\
								m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CPND_INFO_NODE)

#define m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(cnt)  (CPSV_CPND_DEST_INFO*)        \
                                m_NCS_MEM_ALLOC(cnt*sizeof(CPSV_CPND_DEST_INFO), \
                                                NCS_MEM_REGION_PERSISTENT,   \
                                                NCS_SERVICE_ID_CPD,          \
                                                CPD_SVC_SUB_ID_CPSV_CPND_DEST_INFO)

#define m_MMGR_FREE_CPSV_CPND_DEST_INFO(p) m_NCS_MEM_FREE(p,                  \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPSV_CPND_DEST_INFO)

#define m_MMGR_ALLOC_CPD_NODE_REF_INFO  (CPD_NODE_REF_INFO*)        \
                                m_NCS_MEM_ALLOC(sizeof(CPD_NODE_REF_INFO), \
                                                NCS_MEM_REGION_PERSISTENT,   \
                                                NCS_SERVICE_ID_CPD,          \
                                                CPD_SVC_SUB_ID_CPD_NODE_REF_INFO)

#define m_MMGR_FREE_CPD_NODE_REF_INFO(p) m_NCS_MEM_FREE(p,                  \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_NODE_REF_INFO)

#define m_MMGR_ALLOC_CPD_CKPT_REF_INFO  (CPD_CKPT_REF_INFO*)        \
                                m_NCS_MEM_ALLOC(sizeof(CPD_CKPT_REF_INFO), \
                                                NCS_MEM_REGION_PERSISTENT,   \
                                                NCS_SERVICE_ID_CPD,          \
                                                CPD_SVC_SUB_ID_CPD_CKPT_REF_INFO)

#define m_MMGR_FREE_CPD_CKPT_REF_INFO(p) m_NCS_MEM_FREE(p,                  \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPD, \
                                                CPD_SVC_SUB_ID_CPD_CKPT_REF_INFO)

#define m_MMGR_ALLOC_CPD_MBCSV_MSG    (CPD_MBCSV_MSG *) \
                                      m_NCS_MEM_ALLOC(sizeof(CPD_MBCSV_MSG), \
                                      NCS_MEM_REGION_PERSISTENT, \
                                      NCS_SERVICE_ID_CPD, \
                                      CPD_SVC_SUB_ID_CPD_MBCSV_MSG)

#define m_MMGR_FREE_CPD_MBCSV_MSG(p)  m_NCS_MEM_FREE(p,               \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_CPD, \
                                            CPD_SVC_SUB_ID_CPD_MBCSV_MSG)

#define m_MMGR_ALLOC_CPD_A2S_CKPT_CREATE  (CPD_A2S_CKPT_CREATE *) \
                                   m_NCS_MEM_ALLOC(sizeof(CPD_A2S_CKPT_CREATE),\
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_CPD, \
                                   CPD_SVC_SUB_ID_CPD_A2S_CKPT_CREATE)

#define m_MMGR_FREE_CPD_A2S_CKPT_CREATE(p) m_NCS_MEM_FREE(p,   \
                                              NCS_MEM_REGION_PERSISTENT, \
                                              NCS_SERVICE_ID_CPD, \
                                         CPD_SVC_SUB_ID_CPD_A2S_CKPT_CREATE)

#endif  // CKPT_CKPTD_CPD_MEM_H_
