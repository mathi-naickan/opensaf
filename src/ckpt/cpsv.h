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

  This module is the main include file for the entire CheckPoint Service.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef CKPT_CPSV_H_
#define CKPT_CPSV_H_

#include "base/ncsgl_defs.h"
#include "base/logtrace.h"
#include "base/ncs_saf.h"
#include "base/ncs_lib.h"
#include "mds/mds_papi.h"
#include "base/ncs_edu_pub.h"

/* EDU Includes... */
#include "base/ncs_edu_pub.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_saf_edu.h"

/* Versioning 3.0.B related common Macros */
#include "base/ncs_util.h"

#include "ckpt/cpsv_evt.h"
#include "ckpt/cpsv_mem.h"
/* CPSV Common Macros */
/*** Macro used to get the AMF version used ****/
#define m_CPSV_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;

#define m_COMPARE_CREATE_ATTR(attr1, attr2)                       \
  (((attr1)->creationFlags == (attr2)->creationFlags) &&          \
   ((attr1)->checkpointSize == (attr2)->checkpointSize) &&        \
   ((attr1)->maxSections == (attr2)->maxSections) &&              \
   ((attr1)->maxSectionSize == (attr2)->maxSectionSize) &&        \
   ((attr1)->maxSectionIdSize == (attr2)->maxSectionIdSize))      \

#define m_COMPARE_CREATE_ATTR_B_1_1(attr1, attr2)                       \
  (((attr1)->creationFlags == (attr2)->creationFlags) &&          \
   ((attr1)->checkpointSize == (attr2)->checkpointSize) &&        \
   ((attr1)->retentionDuration == (attr2)->retentionDuration) &&  \
   ((attr1)->maxSections == (attr2)->maxSections) &&              \
   ((attr1)->maxSectionSize == (attr2)->maxSectionSize) &&        \
   ((attr1)->maxSectionIdSize == (attr2)->maxSectionIdSize))      \

#define m_CPA_VER_IS_ABOVE_B_1_1(ver) \
  ((((ver)->releaseCode == 'B') || \
    ((ver)->releaseCode > 'B') )&& \
    ((ver)->majorVersion > 0x01 ) && \
    ((ver)->minorVersion > 0x01 ) )

#define m_CPA_VER_IS_BELOW_B_2_3(ver) \
	(((ver)->releaseCode != 'B') || \
	((ver)->majorVersion != 0x02 ) || \
	((ver)->minorVersion < 0x03 ) )


#define m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(attr)                  \
    ((attr)->creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED)

#define m_IS_ASYNC_UPDATE_OPTION(attr)                            \
   ((attr)->creationFlags & (SA_CKPT_WR_ACTIVE_REPLICA | SA_CKPT_WR_ACTIVE_REPLICA_WEAK))

#define FUNC_NAME(DS) \
       cpsv_edp_##DS##_info

#define ARRAY_NAME(DS) \
       cpsv_edp_##DS##_rules

#define TEST_FUNC(DS) \
       cpsv_edp_##DS##_fnc

#define FUNC_DECLARATION(DS)   uint32_t \
                   FUNC_NAME(DS)(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, \
                   NCSCONTEXT ptr, uint32_t *ptr_data_len, \
                   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, \
                   EDU_ERR *o_err)

#define TEST_FUNC_DECLARATION(DS) uint32_t \
                         TEST_FUNC(DS)(NCSCONTEXT arg)

#define NCS_ENC_DEC_DECLARATION(DS) \
                 uint32_t rc = NCSCC_RC_SUCCESS; \
                 DS   *struct_ptr = NULL, **d_ptr = NULL

#define NCS_ENC_DEC_ARRAY(DS) \
         EDU_INST_SET      ARRAY_NAME(DS)[] =

#define NCS_ENC_DEC_REM_FLOW(DS)  \
 \
   switch (op) \
   { \
      case EDP_OP_TYPE_ENC: \
         struct_ptr = (DS *)ptr; \
         break; \
                 \
      case EDP_OP_TYPE_DEC: \
          d_ptr = (DS **)ptr;  \
          if(*d_ptr == NULL)                          \
          { \
             *o_err = EDU_ERR_MEM_FAIL;               \
             return NCSCC_RC_FAILURE;                 \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS)); \
          struct_ptr = *d_ptr;                                \
         break; \
                   \
      default:  \
         struct_ptr = ptr;                                   \
         break; \
      }  \
   \
   rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn,ARRAY_NAME(DS), \
       struct_ptr, ptr_data_len, buf_env, op, o_err); \
   \
   return rc;

#define m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(t)      (t)/(10000000)	/* 10^7 */

#endif  // CKPT_CPSV_H_
