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

  ******************************************************************************
  */
#ifndef MDS_MDA_PVT_API_H_
#define MDS_MDA_PVT_API_H_

#include "base/ncs_ubaid.h"
#include "base/ncs_util.h"
#include "base/ncs_lib.h"

#define VDA_SVC_PVT_VER  1
#define VDA_WRT_VDS_SUBPART_VER_MIN 1
#define VDA_WRT_VDS_SUBPART_VER_MAX 1
#define VDA_WRT_VDS_SUBPART_VER_RANGE           \
  (VDA_WRT_VDS_SUBPART_VER_MAX -                \
   VDA_WRT_VDS_SUBPART_VER_MIN +1)

#define m_NCS_VDA_ENC_MSG_FMT_GET      m_NCS_ENC_MSG_FMT_GET
#define m_NCS_VDA_MSG_FORMAT_IS_VALID  m_NCS_MSG_FORMAT_IS_VALID

/***********************************************************************\
                    VDA-PRIVATE APIs used by VDS.
\***********************************************************************/
uint32_t vda_chg_role_vdest(MDS_DEST *i_vdest, V_DEST_RL i_new_role);

uint32_t vda_create_vdest_locally(uint32_t i_policy, MDS_DEST *i_vdest, MDS_HDL *o_mds_vdest_hdl);

uint32_t vda_util_enc_8bit(NCS_UBAID *uba, uint8_t data);
uint8_t vda_util_dec_8bit(NCS_UBAID *uba);

#define vda_util_enc_n_octets(uba, size, buff) ncs_encode_n_octets_in_uba(uba, buff, size)
#define vda_util_dec_n_octets(uba, size, buff) ncs_decode_n_octets_from_uba(uba, buff, size)

uint32_t vda_util_enc_vdest_name(NCS_UBAID *uba, SaNameT *name);
uint32_t vda_util_dec_vdest_name(NCS_UBAID *uba, SaNameT *name);

uint32_t vda_util_enc_vdest(NCS_UBAID *uba, MDS_DEST *dest);
uint32_t vda_util_dec_vdest(NCS_UBAID *uba, MDS_DEST *dest);

/***********************************************************************\
    ada_lib_req :  This API initializes ADA (Absolute Destination Agent code)
\***********************************************************************/
/* Service provider abstract name */
#define m_ADA_SP_ABST_NAME  "NCS_ADA"
uint32_t ada_lib_req(NCS_LIB_REQ_INFO *req);

/***********************************************************************\
    vda_lib_req :  This API initializes VDA (Virtual Destination Agent code)
\***********************************************************************/
/* Service provider abstract name */
#define m_VDA_SP_ABST_NAME  "NCS_VDA"
uint32_t vda_lib_req(NCS_LIB_REQ_INFO *req);

typedef enum {
  MDA_INST_NAME_TYPE_NULL,
  MDA_INST_NAME_TYPE_ADEST,
  MDA_INST_NAME_TYPE_UNNAMED_VDEST,
  MDA_INST_NAME_TYPE_NAMED_VDEST,
} MDA_INST_NAME_TYPE;

MDA_INST_NAME_TYPE mda_get_inst_name_type(SaNameT *name);

#ifndef MDA_TRACE_LEVEL
#define MDA_TRACE_LEVEL 0
#endif

#if (MDA_TRACE_LEVEL == 1)
#define MDA_TRACE1_ARG1(x)  printf(x)
#define MDA_TRACE1_ARG2(x,y)  printf(x,y)
#else
#define MDA_TRACE1_ARG1(x)
#define MDA_TRACE1_ARG2(x,y)
#endif

#endif  // MDS_MDA_PVT_API_H_
