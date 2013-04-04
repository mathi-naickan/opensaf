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
  
  This file consists of encode/decode routines of CPSV
    
******************************************************************************
*/

#include "cpsv.h"
#include "ncssysf_mem.h"

/* Routines in this file will be identified while coding */

FUNC_DECLARATION(CPSV_SAERR_INFO);
FUNC_DECLARATION(CPSV_INIT_REQ);
FUNC_DECLARATION(CPSV_FINALIZE_REQ);
FUNC_DECLARATION(SaCkptCheckpointCreationAttributesT);
FUNC_DECLARATION(CPSV_A2ND_OPEN_REQ);
FUNC_DECLARATION(CPSV_A2ND_CKPT_CLOSE);
FUNC_DECLARATION(CPSV_ND2A_INIT_RSP);
FUNC_DECLARATION(CPSV_ND2A_OPEN_RSP);
FUNC_DECLARATION(SaCkptCheckpointDescriptorT);
FUNC_DECLARATION(CPSV_CKPT_STATUS);
FUNC_DECLARATION(CPSV_ND2A_READ_DATA);
FUNC_DECLARATION(CPSV_ND2A_READ_MAP);

FUNC_DECLARATION(CPSV_ND2A_DATA_ACCESS_RSP);
FUNC_DECLARATION(SaCkptSectionIdT);

FUNC_DECLARATION(SaCkptSectionDescriptorT);
FUNC_DECLARATION(CPSV_ND2A_SECT_ITER_GETNEXT_RSP);
FUNC_DECLARATION(CPSV_ND2A_SYNC_RSP);
FUNC_DECLARATION(CPSV_ND2A_ARRIVAL_MSG);
FUNC_DECLARATION(CPSV_ND2D_USR_INFO);
FUNC_DECLARATION(CPSV_ND2D_CKPT_CREATE);
FUNC_DECLARATION(CPSV_CKPT_SEC_INFO_UPD);
FUNC_DECLARATION(CPSV_CKPT_ID_INFO);
FUNC_DECLARATION(CPSV_ND2D_CKPT_UNLINK);
FUNC_DECLARATION(CPSV_CKPT_NAME_INFO);
FUNC_DECLARATION(CPSV_CKPT_RDSET);
FUNC_DECLARATION(CPSV_CKPT_DEST_INFO);
FUNC_DECLARATION(CPSV_CKPT_DESTLIST_INFO);
FUNC_DECLARATION(CPSV_A2ND_CKPT_UNLINK);
FUNC_DECLARATION(CPSV_A2ND_ACTIVE_REP_SET);
FUNC_DECLARATION(CPSV_CKPT_STATUS_GET);
FUNC_DECLARATION(SaCkptSectionCreationAttributesT);
FUNC_DECLARATION(CPSV_CKPT_SECT_CREATE);
FUNC_DECLARATION(CPSV_A2ND_SECT_DELETE);
FUNC_DECLARATION(CPSV_A2ND_SECT_EXP_TIME);
FUNC_DECLARATION(CPSV_A2ND_SECT_ITER_GETNEXT);
FUNC_DECLARATION(CPSV_CKPT_DATA);

FUNC_DECLARATION(CPSV_CKPT_ACCESS);
FUNC_DECLARATION(CPSV_A2ND_CKPT_SYNC);
FUNC_DECLARATION(CPSV_A2ND_ARRIVAL_REG);
FUNC_DECLARATION(CPSV_CPND_DEST_INFO);
FUNC_DECLARATION(CPSV_D2ND_CKPT_INFO);
FUNC_DECLARATION(CPSV_D2ND_CKPT_CREATE);
FUNC_DECLARATION(CPSV_CKPT_USED_SIZE);
FUNC_DECLARATION(CPSV_CKPT_NUM_SECTIONS);
FUNC_DECLARATION(CPSV_CKPT_SECT_INFO);
FUNC_DECLARATION(MDS_SYNC_SND_CTXT);

FUNC_DECLARATION(CPND_EVT);
FUNC_DECLARATION(CPD_EVT);
FUNC_DECLARATION(CPA_EVT);
FUNC_DECLARATION(CPSV_EVT);
FUNC_DECLARATION(CPSV_A2ND_CKPT_LIST_UPDATE);

/* Test function declaration */
TEST_FUNC_DECLARATION(CPSV_EVT);
TEST_FUNC_DECLARATION(CPA_EVT);
TEST_FUNC_DECLARATION(CPD_EVT);
TEST_FUNC_DECLARATION(CPND_EVT);
TEST_FUNC_DECLARATION(CPSV_ND2A_DATA_ACCESS_RSP);
#define m_MMGR_ALLOC_SECTIONIDT  (SaCkptSectionIdT *)m_NCS_MEM_ALLOC(sizeof(SaCkptSectionIdT), \
                                       NCS_MEM_REGION_PERSISTENT, \
                                       NCS_SERVICE_ID_CPND, \
                                       CPSV_SVC_SUB_ID_CPSV_EVT \
                                       )

#define m_MMGR_ALLOC_CPSV_DEFAULT(size) m_NCS_MEM_ALLOC(size, \
                                       NCS_MEM_REGION_PERSISTENT, \
                                       NCS_SERVICE_ID_CPND, \
                                       CPSV_SVC_SUB_ID_CPSV_EVT \
                                     )

#define NCS_ENC_DEC_SECTIONIDT(DS) \
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
          {                                           \
             *d_ptr=m_MMGR_ALLOC_CPSV_SaCkptSectionIdT(NCS_SERVICE_ID_CPND);   \
             if(*d_ptr == NULL)                       \
             {                                        \
                *o_err = EDU_ERR_MEM_FAIL;            \
                return NCSCC_RC_FAILURE;              \
             } \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS));     \
          struct_ptr = *d_ptr;                        \
          break;                                       \
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
#define NCS_ENC_DEC_CPSV_ND2A_READ_DATA(DS) \
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
          {                                           \
             *d_ptr=m_MMGR_ALLOC_CPSV_DEFAULT(sizeof(DS));\
             if(*d_ptr == NULL)                       \
             {                                        \
                *o_err = EDU_ERR_MEM_FAIL;            \
                return NCSCC_RC_FAILURE;              \
             } \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS));     \
          struct_ptr = *d_ptr;                        \
          break;                                       \
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

#define  NCS_ENC_DEC_CPSV_ND2A_READ_MAP(DS) \
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
          {                                           \
             *d_ptr=m_MMGR_ALLOC_CPSV_DEFAULT(sizeof(DS));\
             if(*d_ptr == NULL)                       \
             {                                        \
                *o_err = EDU_ERR_MEM_FAIL;            \
                return NCSCC_RC_FAILURE;              \
             } \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS));     \
          struct_ptr = *d_ptr;                        \
          break;                                       \
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

#define NCS_ENC_DEC_CPSV_CKPT_DATA(DS) \
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
          {                                           \
             *d_ptr=m_MMGR_ALLOC_CPSV_CKPT_DATA;       \
             if(*d_ptr == NULL)                       \
             {                                        \
                *o_err = EDU_ERR_MEM_FAIL;            \
                return NCSCC_RC_FAILURE;              \
             } \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS));     \
          struct_ptr = *d_ptr;                        \
          break;                                       \
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
#define  NCS_ENC_DEC_CPSV_CPND_DEST_INFO(DS) \
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
          {                                           \
             *d_ptr=m_MMGR_ALLOC_CPSV_DEFAULT(sizeof(DS)); \
             if(*d_ptr == NULL)                       \
             {                                        \
                *o_err = EDU_ERR_MEM_FAIL;            \
                return NCSCC_RC_FAILURE;              \
             } \
          }                                           \
          memset(*d_ptr, '\0', sizeof(DS));     \
          struct_ptr = *d_ptr;                        \
          break;                                       \
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

#define DS CPSV_SAERR_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_INIT_REQ
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_INIT_REQ), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_saversiont, 0, 0, 0, (long)&((DS *) 0)->version, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_FINALIZE_REQ
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_FINALIZE_REQ), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS SaCkptCheckpointCreationAttributesT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->creationFlags, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->checkpointSize, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->retentionDuration, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->maxSections, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->maxSectionSize, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->maxSectionIdSize, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}
#undef DS
#define DS SaVersionT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {
		{
		EDU_START, FUNC_NAME(SaVersionT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->releaseCode, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->majorVersion, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->minorVersion, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_A2ND_OPEN_REQ
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_OPEN_REQ), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->ckpt_attrib, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_flags, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->invocation, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->timeout, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_A2ND_CKPT_CLOSE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_CKPT_CLOSE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/* CPA  ND -- > A
   Events encode decode routines,evts are initiated by ND */

/* temp_place
     {EDU_EXEC, , 0, 0, 0, (uint32_t)&((DS*)0)->, 0, NULL},

*/

#define DS  CPSV_ND2A_INIT_RSP
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_INIT_RSP), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckptHandle, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};

	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_ND2A_OPEN_RSP
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_OPEN_RSP), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->gbl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->creation_attr, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_active_exists, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->active_dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->invocation, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS SaCkptCheckpointDescriptorT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(SaCkptCheckpointDescriptorT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->checkpointCreationAttributes, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->numberOfSections, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->memoryUsed, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_STATUS
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_STATUS), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (uint64_t)(long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointDescriptorT), 0, 0, 0, (long)&((DS *) 0)->status, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_ND2A_READ_DATA
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_READ_DATA), 0, 0, 0, sizeof(DS), 0, NULL},
		    /* {EDU_EXEC,ncs_edp_uns32, EDQ_POINTER , 0, 0, (uint32_t)&((DS*)0)->data, 0, NULL}, */
		{
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->read_size, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
			    (long)&((DS *) 0)->data, (long)&((DS *) 0)->read_size, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPA /* Svc-ID */ , NULL, 0, 6 /* Sub-ID */ , 0, NULL},
		{
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->err, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_CPSV_ND2A_READ_DATA(DS)
}

#undef DS

#define DS CPSV_ND2A_READ_MAP
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_READ_MAP), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->offset_index, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_CPSV_ND2A_READ_MAP(DS)
}

#undef DS

#define DS  CPSV_ND2A_DATA_ACCESS_RSP

TEST_FUNC_DECLARATION(DS)
{

	SaUint32T addr;

	enum {

		LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_LCL_READ_RSP = 3,
		LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_RMT_READ_RSP = 3,
		LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_WRITE_RSP = 5,
		LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_OVWRITE_RSP = 7,

	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(SaUint32T *)arg;

	switch (addr) {
	case CPSV_DATA_ACCESS_LCL_READ_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_LCL_READ_RSP;

	case CPSV_DATA_ACCESS_RMT_READ_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_RMT_READ_RSP;

	case CPSV_DATA_ACCESS_WRITE_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_WRITE_RSP;

	case CPSV_DATA_ACCESS_OVWRITE_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_DATA_ACCESS_OVWRITE_RSP;

	default:
		return (uint32_t)EDU_FAIL;
	}
	return (uint32_t)EDU_FAIL;
}

FUNC_DECLARATION(DS)
{
	uint16_t ver_compare = 0;
	ver_compare = 3; /* CPND_MDS_PVT_SUBPART_VERSION */
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

      {EDU_START,FUNC_NAME(CPSV_ND2A_DATA_ACCESS_RSP), 0, 0, 0, sizeof(DS), 0, NULL},
      {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->type, 0, NULL},
      {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->num_of_elmts, 0, NULL},
      {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->error, 0, NULL},
      {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->size, 0, NULL},
      {EDU_EXEC, ncs_edp_uns64 , 0, 0, 0, (long)&((DS*)0)->ckpt_id, 0, NULL},
      {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->error_index, 0, NULL},
      {EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS*)0)->from_svc, 0, NULL},
      {EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS*)0)->type, 0, (void *)TEST_FUNC(CPSV_ND2A_DATA_ACCESS_RSP)},


      {EDU_EXEC,FUNC_NAME(CPSV_ND2A_READ_MAP),EDQ_VAR_LEN_DATA,ncs_edp_uns32, 0,(long)&((DS*)0)->info.read_mapping,(long)&((DS*)0)->size, NULL},
      {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPA/* Svc-ID */, NULL, EDU_EXIT, 0 /* Sub-ID */, 0, NULL},

      {EDU_EXEC,FUNC_NAME(CPSV_ND2A_READ_DATA),EDQ_VAR_LEN_DATA,ncs_edp_uns32, 0, (long)&((DS*)0)->info.read_data,(long)&((DS*)0)->size, NULL},
      {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPA/* Svc-ID */, NULL, EDU_EXIT, CPSV_SVC_SUB_ID_CPSV_ND2A_READ_DATA/* Sub-ID */, 0, NULL},

      {EDU_EXEC,ncs_edp_uns32,EDQ_VAR_LEN_DATA,ncs_edp_uns32, 0, (long)&((DS*)0)->info.write_err_index,(long)&((DS*)0)->size, NULL},
      {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPA/* Svc-ID */, NULL, EDU_EXIT, CPSV_SVC_SUB_ID_CPSV_SaUint32T /* Sub-ID */, 0, NULL},

      {EDU_EXEC,FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS*)0)->info.ovwrite_error, 0, NULL},

      {EDU_VER_GE, NULL, 0, 0, 1, 0, 0, (EDU_EXEC_RTINE)((uint16_t *)(&(ver_compare)))},
      {EDU_EXEC,ncs_edp_uns64 , 0, 0, 0, (long)&((DS*)0)->lcl_ckpt_id, 0, NULL},

      {EDU_END, 0, 0, 0, 0, 0, 0, NULL},

    };
    NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS SaCkptSectionIdT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((DS *) 0)->idLen, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_VAR_LEN_DATA, ncs_edp_uns16, 0,
			    (long)&((DS *) 0)->id, (long)&((DS *) 0)->idLen, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPND /* Svc-ID */ , NULL, 0,
			    CPSV_SVC_SUB_ID_CPSV_DEFAULT_VAL /* Sub-ID */ , 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_SECTIONIDT(DS)
}

#undef DS

#define DS SaCkptSectionDescriptorT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(SaCkptSectionDescriptorT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->sectionId, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->expirationTime, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->sectionSize, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->sectionState, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lastUpdate, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_ND2A_SECT_ITER_GETNEXT_RSP

FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_SECT_ITER_GETNEXT_RSP), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->iter_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionDescriptorT), 0, 0, 0, (long)&((DS *) 0)->sect_desc, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->n_secs_trav, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_ND2A_SYNC_RSP
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_SYNC_RSP), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->invocation, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/* CPD evts  ND-->D*/

#define DS CPSV_ND2D_USR_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2D_USR_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->info_type, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_flags, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_ND2D_CKPT_CREATE
FUNC_DECLARATION(DS)
{
	uint16_t ver_compare = 0;
	ver_compare = 3;	/* CPD_MDS_PVT_SUBPART_VERSION/CPND_MDS_PVT_SUBPART_VERSION */
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2D_CKPT_CREATE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->attributes, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_flags, 0, NULL}, {
		EDU_VER_GE, NULL, 0, 0, 2, 0, 0, (EDU_EXEC_RTINE)((uint16_t *)(&(ver_compare)))}, {
		EDU_EXEC, FUNC_NAME(SaVersionT), 0, 0, 0, (long)&((DS *) 0)->client_version, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_CKPT_SEC_INFO_UPD
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_SEC_INFO_UPD), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->info_type, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_ID_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_ND2D_CKPT_UNLINK
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2D_CKPT_UNLINK), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_NAME_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_NAME_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_RDSET
FUNC_DECLARATION(DS)
{
	uint16_t ver_compare = 0;
	ver_compare = 3;	/* CPD_MDS_PVT_SUBPART_VERSION/CPND_MDS_PVT_SUBPART_VERSION */
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_RDSET), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->reten_time, 0, NULL}, {
		EDU_VER_GE, NULL, 0, 0, 2, 0, 0, (EDU_EXEC_RTINE)((uint16_t *)(&(ver_compare)))}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_DEST_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->mds_dest, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_CKPT_DESTLIST_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_DESTLIST_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->mds_dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->active_dest, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->attributes, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_flags, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_cpnd_restart, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->dest_cnt, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CPND_DEST_INFO), EDQ_VAR_LEN_DATA, ncs_edp_uns32, EDU_EXIT,
			    (long)&((DS *) 0)->dest_list, (long)&((DS *) 0)->dest_cnt, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/*  CPND  A --> ND ND --> ND D -- > ND ND --- > D */

/* CPA --> CPND */

#define DS   CPSV_A2ND_CKPT_UNLINK
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_CKPT_UNLINK), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS
#define DS   CPSV_A2ND_ACTIVE_REP_SET
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_ACTIVE_REP_SET), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_CKPT_STATUS_GET
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_STATUS_GET), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS SaCkptSectionCreationAttributesT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(SaCkptSectionCreationAttributesT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), EDQ_POINTER, 0, 0, (long)&((DS *) 0)->sectionId, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->expirationTime, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_CKPT_SECT_CREATE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_SECT_CREATE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->agent_mdest, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionCreationAttributesT), 0, 0, 0, (long)&((DS *) 0)->sec_attri, 0, NULL},
		{
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->init_size, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_VAR_LEN_DATA, ncs_edp_uns64, 0, (long)&((DS *) 0)->init_data,
			    (long)&((DS *) 0)->init_size, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_CPND /* Svc-ID */ , NULL, 0,
			    CPSV_SVC_SUB_ID_CPSV_DEFAULT_VAL /* Sub-ID */ , 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS
#define DS   CPSV_A2ND_SECT_DELETE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_SECT_DELETE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->sec_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->agent_mdest, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS
#define DS   CPSV_A2ND_SECT_EXP_TIME
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_SECT_EXP_TIME), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->sec_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->exp_time, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_A2ND_SECT_ITER_GETNEXT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_SECT_ITER_GETNEXT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->section_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->iter_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->filter, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->n_secs_trav, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->exp_tmr, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_CKPT_DATA
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_DATA), EDQ_LNKLIST, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->sec_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->expirationTime, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->dataSize, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->readSize, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_VAR_LEN_DATA, ncs_edp_uns64, 0, (long)&((DS *) 0)->data,
			    (long)&((DS *) 0)->dataSize, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->dataOffset, 0, NULL}, {
		EDU_TEST_LL_PTR, FUNC_NAME(CPSV_CKPT_DATA), 0, 0, 0, (long)&((DS *) 0)->next, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_CPSV_CKPT_DATA(DS)
}

#undef DS

#define DS CPSV_ND2A_ARRIVAL_MSG
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_ND2A_ARRIVAL_MSG), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->mdest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_of_elmts, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DATA), EDQ_POINTER, 0, 0, (long)&((DS *) 0)->ckpt_data, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_CKPT_ACCESS
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_ACCESS), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->agent_mdest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_of_elmts, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->all_repl_evt_flag, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DATA), EDQ_POINTER, 0, 0, (long)&((DS *) 0)->data, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->seqno, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->last_seq, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.invocation, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.is_ckpt_open, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.cpa_sinfo.dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.cpa_sinfo.stype, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.cpa_sinfo.to_svc, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->ckpt_sync.cpa_sinfo.ctxt.length, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0, (long)&((DS *) 0)->ckpt_sync.cpa_sinfo.ctxt.data,
			    MDS_SYNC_SND_CTXT_LEN_MAX, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_A2ND_CKPT_SYNC
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_CKPT_SYNC), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->invocation, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_ckpt_open, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->cpa_sinfo.dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->cpa_sinfo.stype, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->cpa_sinfo.to_svc, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->cpa_sinfo.ctxt.length, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0, (long)&((DS *) 0)->cpa_sinfo.ctxt.data,
			    MDS_SYNC_SND_CTXT_LEN_MAX, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS  CPSV_A2ND_ARRIVAL_REG
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_ARRIVAL_REG), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/* CPD --> CPND */
#define DS   CPSV_CPND_DEST_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CPND_DEST_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->dest, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_CPSV_CPND_DEST_INFO(DS)
}

#undef DS

#define DS   CPSV_D2ND_CKPT_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_D2ND_CKPT_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_active_exists, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->attributes, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->active_dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->ckpt_rep_create, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->dest_cnt, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CPND_DEST_INFO), EDQ_VAR_LEN_DATA, ncs_edp_uns32, EDU_EXIT,
			    (long)&((DS *) 0)->dest_list, (long)&((DS *) 0)->dest_cnt, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_D2ND_CKPT_CREATE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_D2ND_CKPT_CREATE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_D2ND_CKPT_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->ckpt_info, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_CKPT_USED_SIZE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_USED_SIZE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_used_size, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_CKPT_NUM_SECTIONS
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_NUM_SECTIONS), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_num_sections, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/* CPND --> CPND */
/* CPSV_CKPT_SECT_CREATE == CPSV_A2ND_SECT_CREATE */

#define DS   CPSV_CKPT_SECT_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_CKPT_SECT_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptSectionIdT), 0, 0, 0, (long)&((DS *) 0)->sec_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->lcl_ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->agent_mdest, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

/* CPSV EVT */

#define DS MDS_SYNC_SND_CTXT
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(MDS_SYNC_SND_CTXT), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((DS *) 0)->length, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0, (long)&((DS *) 0)->data, 10, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS   CPSV_A2ND_CKPT_LIST_UPDATE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_A2ND_CKPT_LIST_UPDATE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->client_hdl, 0, NULL},{
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPND_EVT
TEST_FUNC_DECLARATION(DS)
{

	CPND_EVT_TYPE addr;
	enum {
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_INIT = 1,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_FINALIZE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_OPEN,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_CLOSE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_UNLINK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_RDSET,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_AREP_SET,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_STATUS_GET,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_CREATE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_DELETE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_EXP_SET,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_ITER_GETNEXT,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_WRITE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_READ,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SYNC,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_ARRIVAL_CB_REG,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_INFO,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_SIZE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_REP_ADD,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_REP_DEL,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_CREATE,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_DESTROY,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_UNLINK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_RDSET,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_ACTIVE_SET,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_CLOSE_ACK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_UNLINK_ACK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_RDSET_ACK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_DESTROY_ACK,

		LCL_TEST_JUMP_OFFSET_CPSV_D2ND_RESTART,
		LCL_TEST_JUMP_OFFSET_CPSV_D2ND_RESTART_DONE,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_ACTIVE_STATUS,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_ACTIVE_STATUS_ACK,

		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP,

		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP,

		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_SYNC_REQ,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC,

		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ,	/* (req,active_req) */
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT,

		LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_NUM_SECTIONS,
		LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_LIST_UPDATE,

	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(SaUint32T *)arg;

	switch (addr) {
	case CPND_EVT_MDS_INFO:
	case CPND_EVT_TIME_OUT:
		return EDU_FAIL;
	case CPND_EVT_A2ND_CKPT_INIT:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_INIT;
	case CPND_EVT_A2ND_CKPT_FINALIZE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_FINALIZE;
	case CPND_EVT_A2ND_CKPT_OPEN:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_OPEN;
	case CPND_EVT_A2ND_CKPT_CLOSE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_CLOSE;
	case CPND_EVT_A2ND_CKPT_UNLINK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_UNLINK;
	case CPND_EVT_A2ND_CKPT_RDSET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_RDSET;
	case CPND_EVT_A2ND_CKPT_AREP_SET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_AREP_SET;
	case CPND_EVT_A2ND_CKPT_STATUS_GET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_STATUS_GET;
	case CPND_EVT_A2ND_CKPT_SECT_CREATE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_CREATE;
	case CPND_EVT_A2ND_CKPT_SECT_DELETE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_DELETE;
	case CPND_EVT_A2ND_CKPT_SECT_EXP_SET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SECT_EXP_SET;
	case CPND_EVT_A2ND_CKPT_ITER_GETNEXT:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_ITER_GETNEXT;
	case CPND_EVT_A2ND_CKPT_WRITE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_WRITE;
	case CPND_EVT_A2ND_CKPT_READ:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_READ;
	case CPND_EVT_A2ND_CKPT_SYNC:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_SYNC;
	case CPND_EVT_A2ND_ARRIVAL_CB_REG:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_ARRIVAL_CB_REG;
	case CPND_EVT_ND2ND_ACTIVE_STATUS:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_ACTIVE_STATUS;
	case CPND_EVT_ND2ND_ACTIVE_STATUS_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_ACTIVE_STATUS_ACK;
	case CPND_EVT_ND2ND_CKPT_SYNC_REQ:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_SYNC_REQ;
	case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC;
	case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ;
	case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP;
	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP;
	case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ;
	case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP;
	case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ;
	case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP;
	case CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ;
	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ;
	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP;
	case CPND_EVT_D2ND_CKPT_INFO:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_INFO;
	case CPND_EVT_D2ND_CKPT_SIZE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_SIZE;
	case CPND_EVT_D2ND_CKPT_NUM_SECTIONS:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_NUM_SECTIONS;
	case CPND_EVT_D2ND_CKPT_REP_ADD:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_REP_ADD;
	case CPND_EVT_D2ND_CKPT_REP_DEL:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_REP_DEL;
	case CPND_EVT_D2ND_CKPT_CREATE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_CREATE;
	case CPND_EVT_D2ND_CKPT_DESTROY:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_DESTROY;
	case CPND_EVT_D2ND_CKPT_DESTROY_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_DESTROY_ACK;
	case CPND_EVT_D2ND_CKPT_CLOSE_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_CLOSE_ACK;
	case CPND_EVT_D2ND_CKPT_UNLINK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_UNLINK;
	case CPND_EVT_D2ND_CKPT_UNLINK_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_UNLINK_ACK;
	case CPND_EVT_D2ND_CKPT_RDSET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_RDSET;
	case CPND_EVT_D2ND_CKPT_RDSET_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_RDSET_ACK;
	case CPND_EVT_D2ND_CKPT_ACTIVE_SET:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_ACTIVE_SET;
	case CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK;

	case CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ;

	case CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT;

	case CPSV_D2ND_RESTART:
		return LCL_TEST_JUMP_OFFSET_CPSV_D2ND_RESTART;
	case CPSV_D2ND_RESTART_DONE:
		return LCL_TEST_JUMP_OFFSET_CPSV_D2ND_RESTART_DONE;
	case CPND_EVT_A2ND_CKPT_LIST_UPDATE:
		return LCL_TEST_JUMP_OFFSET_CPND_EVT_A2ND_CKPT_LIST_UPDATE;
	default:
		return EDU_FAIL;
	}
}

FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPND_EVT), 0, 0, 0, sizeof(DS), 0, NULL}
		, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->error, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, (void *)TEST_FUNC(CPND_EVT)}, {
		EDU_EXEC, FUNC_NAME(CPSV_INIT_REQ), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.initReq, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_FINALIZE_REQ), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.finReq, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_OPEN_REQ), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.openReq, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_CKPT_CLOSE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.closeReq, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_CKPT_UNLINK), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ulinkReq, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_RDSET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rdsetReq, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_ACTIVE_REP_SET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arsetReq,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_STATUS_GET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.statReq, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_CREATE), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.sec_creatReq, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_SECT_DELETE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_delReq,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_SECT_EXP_TIME), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.sec_expset, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_SECT_ITER_GETNEXT), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.iter_getnext, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ACCESS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_write, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ACCESS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_read, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_CKPT_SYNC), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_sync, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_ARRIVAL_REG), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arr_ntfy, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_D2ND_CKPT_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_info, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_USED_SIZE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_mem_size,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DESTLIST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_add,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_del, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_D2ND_CKPT_CREATE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_create,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_destroy, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_ulink, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_RDSET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rdset, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.active_set, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.cl_ack, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ulink_ack, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rdset_ack, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arep_ack, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.destroy_ack, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.cpnd_restart, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DESTLIST_INFO), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.cpnd_restart_done, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_STATUS_GET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.stat_get, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_STATUS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.status, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_CREATE), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.active_sec_creat, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_creat_rsp,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_INFO), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.active_sec_creat_rsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_INFO), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.sec_delete_req, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_delete_rsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_SECT_EXP_TIME), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.sec_exp_set, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_exp_rsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_CKPT_SYNC), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sync_req, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ACCESS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_nd2nd_sync,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ACCESS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_nd2nd_data,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_DATA_ACCESS_RSP), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_nd2nd_data_rsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_SECT_ITER_GETNEXT), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.getnext_req, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_SECT_ITER_GETNEXT_RSP), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_nd2nd_getnext_rsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_NUM_SECTIONS), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_sections, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_A2ND_CKPT_LIST_UPDATE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckptListUpdate, 0, NULL},{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPD_EVT
TEST_FUNC_DECLARATION(DS)
{
	CPD_EVT_TYPE addr;
	enum {

		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CREATE = 1,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_UNLINK,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_RDSET,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_ACTIVE_SET,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CLOSE,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_DESTROY,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_SEC_INFO_UPD,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_USR_INFO,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_MEM_USED,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_DESTROY_BYNAME,
		LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CREATED_SECTIONS,

	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(SaUint32T *)arg;

	switch (addr) {

	case CPD_EVT_MDS_INFO:
		return EDU_FAIL;

	case CPD_EVT_ND2D_CKPT_CREATE:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CREATE;
	case CPD_EVT_ND2D_CKPT_UNLINK:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_UNLINK;
	case CPD_EVT_ND2D_CKPT_RDSET:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_RDSET;
	case CPD_EVT_ND2D_ACTIVE_SET:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_ACTIVE_SET;
	case CPD_EVT_ND2D_CKPT_CLOSE:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CLOSE;
	case CPD_EVT_ND2D_CKPT_DESTROY:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_DESTROY;
	case CPD_EVT_ND2D_CKPT_SEC_INFO_UPD:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_SEC_INFO_UPD;
	case CPD_EVT_ND2D_CKPT_USR_INFO:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_USR_INFO;
	case CPD_EVT_ND2D_CKPT_MEM_USED:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_MEM_USED;
	case CPD_EVT_ND2D_CKPT_CREATED_SECTIONS:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_CREATED_SECTIONS;
	case CPD_EVT_ND2D_CKPT_DESTROY_BYNAME:
		return LCL_TEST_JUMP_OFFSET_CPD_EVT_ND2D_CKPT_DESTROY_BYNAME;
	default:
		return EDU_FAIL;
	}

	return EDU_FAIL;
}

FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPD_EVT), 0, 0, 0, sizeof(DS), 0, NULL}
		, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, (void *)TEST_FUNC(CPD_EVT)}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2D_CKPT_CREATE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_create,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2D_CKPT_UNLINK), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_ulink,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_RDSET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rd_set, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arep_set, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_close, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_ID_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_destroy, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SEC_INFO_UPD), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_sec_info, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2D_USR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_usr_info,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_USED_SIZE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_mem_used,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_NAME_INFO), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_destroy_byname, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_NUM_SECTIONS), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.ckpt_created_sections, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPA_EVT
TEST_FUNC_DECLARATION(DS)
{

	CPA_EVT_TYPE addr;
	enum {

		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_INIT_RSP = 1,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_FINALIZE_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_OPEN_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_CLOSE_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_UNLINK_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_RDSET_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_AREP_SET_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_STATUS,

		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_CREATE_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_DELETE_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_EXPTIME_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP,

		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_DATA_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_SYNC_RSP,
		LC_TEST_JUMP_OFFSET_CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY,
		LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND,
		/* add new event hear */
		LC_TEST_JUMP_OFFSET_END
	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(SaUint32T *)arg;

	switch (addr) {
	case CPA_EVT_MDS_INFO:
	case CPA_EVT_TIME_OUT:
		return EDU_FAIL;

	case CPA_EVT_ND2A_CKPT_INIT_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_INIT_RSP;
	case CPA_EVT_ND2A_CKPT_FINALIZE_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_FINALIZE_RSP;
	case CPA_EVT_ND2A_CKPT_OPEN_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_OPEN_RSP;
	case CPA_EVT_ND2A_CKPT_CLOSE_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_CLOSE_RSP;
	case CPA_EVT_ND2A_CKPT_UNLINK_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_UNLINK_RSP;
	case CPA_EVT_ND2A_CKPT_RDSET_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_RDSET_RSP;
	case CPA_EVT_ND2A_CKPT_AREP_SET_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_AREP_SET_RSP;
	case CPA_EVT_ND2A_CKPT_STATUS:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_STATUS;
	case CPA_EVT_ND2A_SEC_CREATE_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_CREATE_RSP;
	case CPA_EVT_ND2A_SEC_DELETE_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_DELETE_RSP;
	case CPA_EVT_ND2A_SEC_EXPTIME_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_EXPTIME_RSP;
	case CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP;
	case CPA_EVT_ND2A_CKPT_DATA_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_DATA_RSP;
	case CPA_EVT_ND2A_CKPT_SYNC_RSP:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_SYNC_RSP;
	case CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND;
	case CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND:
	case CPA_EVT_D2A_NDRESTART:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND;
	case CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY:
		return LC_TEST_JUMP_OFFSET_CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY;
	case CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT:
		return LC_TEST_JUMP_OFFSET_END;
	case CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED:
		return LC_TEST_JUMP_OFFSET_END;
	case CPA_EVT_ND2A_CKPT_BCAST_SEND:
		return LC_TEST_JUMP_OFFSET_END;
	case CPA_EVT_ND2A_SEC_ITER_RSP:
		return LC_TEST_JUMP_OFFSET_END;
	default:
		return EDU_FAIL;
	}
}

FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPA_EVT), 0, 0, 0, sizeof(DS), 0, NULL}
		, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, (void *)TEST_FUNC(CPA_EVT)}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_INIT_RSP), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.initRsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.finRsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_OPEN_RSP), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.openRsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.closeRsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ulinkRsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rdsetRsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arsetRsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_STATUS), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.status, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_SECT_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_creat_rsp,
			    0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_delete_rsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_SAERR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sec_exptmr_rsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_SECT_ITER_GETNEXT_RSP), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.iter_next_rsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_DATA_ACCESS_RSP), 0, 0, EDU_EXIT,
			    (long)&((DS *) 0)->info.sec_data_rsp, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_SYNC_RSP), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.sync_rsp, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ackpt_info, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_ND2A_ARRIVAL_MSG), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arr_msg, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ackpt_info, 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPSV_EVT
TEST_FUNC_DECLARATION(DS)
{

	CPSV_EVT_TYPE addr;

	enum {
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPA = 1,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPND,
		LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPD,
	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(CPSV_EVT_TYPE *)arg;

	switch (addr) {

	case CPSV_EVT_TYPE_CPA:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPA;
		break;

	case CPSV_EVT_TYPE_CPND:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPND;
		break;

	case CPSV_EVT_TYPE_CPD:
		return LCL_TEST_JUMP_OFFSET_CPSV_EVT_TYPE_CPD;
		break;

	default:
		return EDU_FAIL;
	}
}

FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPSV_EVT), 0, 0, 0, sizeof(DS), 0, NULL}
		, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, (void *)TEST_FUNC(CPSV_EVT)}, {
		EDU_EXEC, FUNC_NAME(CPA_EVT), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.cpa, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPND_EVT), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.cpnd, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPD_EVT), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.cpd, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS
