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

#include "cpsv.h"
#include "cpd_red.h"

FUNC_DECLARATION(CPSV_CPND_DEST_INFO);
FUNC_DECLARATION(CPD_A2S_CKPT_CREATE);
FUNC_DECLARATION(CPD_A2S_CKPT_UNLINK);
FUNC_DECLARATION(SaCkptCheckpointCreationAttributesT);
FUNC_DECLARATION(CPSV_CKPT_DEST_INFO);
FUNC_DECLARATION(CPSV_CKPT_RDSET);
FUNC_DECLARATION(CPD_A2S_CKPT_USR_INFO);

FUNC_DECLARATION(CPD_MBCSV_MSG);

#define TEST_FUNC_DECLARATION(DS) uint32_t \
                         TEST_FUNC(DS)(NCSCONTEXT arg)

#define NCS_ENC_DEC_DECLARATION(DS) \
                 uint32_t rc = NCSCC_RC_SUCCESS; \
                 DS   *struct_ptr = NULL, **d_ptr = NULL

#define NCS_ENC_DEC_ARRAY(DS) \
         EDU_INST_SET      ARRAY_NAME(DS)[] =

TEST_FUNC_DECLARATION(CPD_MBCSV_MSG);

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

#define DS CPD_A2S_CKPT_CREATE
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPD_A2S_CKPT_CREATE), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (uint64_t)(long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(SaCkptCheckpointCreationAttributesT), 0, 0, 0,
			    (long)&((DS *) 0)->ckpt_attrib, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_unlink_set, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_active_exists, 0, NULL}, {
		EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((DS *) 0)->active_dest, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((DS *) 0)->create_time, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_users, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_writers, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_readers, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_sections, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->dest_cnt, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CPND_DEST_INFO), EDQ_VAR_LEN_DATA, ncs_edp_uns32, EDU_EXIT,
			    (long)&((DS *) 0)->dest_list, (long)&((DS *) 0)->dest_cnt, NULL}, {
		EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPD_A2S_CKPT_UNLINK
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPD_A2S_CKPT_UNLINK), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((DS *) 0)->ckpt_name, 0, NULL}, {
		EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((DS *) 0)->is_unlink_set, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPD_A2S_CKPT_USR_INFO
FUNC_DECLARATION(DS)
{
	NCS_ENC_DEC_DECLARATION(DS);
	NCS_ENC_DEC_ARRAY(DS) {

		{
		EDU_START, FUNC_NAME(CPD_A2S_CKPT_USR_INFO), 0, 0, 0, sizeof(DS), 0, NULL}, {
		EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (uint64_t)(long)&((DS *) 0)->ckpt_id, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_user, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_writer, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_reader, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->num_sections, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_on_scxb1, 0, NULL}, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->ckpt_on_scxb2, 0, NULL}, {
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS

#define DS CPD_MBCSV_MSG
TEST_FUNC_DECLARATION(DS)
{
	CPD_MBCSV_MSG_TYPE addr;
	enum {

		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_CREATE = 1,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_ADD,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_DEL,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_RDSET,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_AREP_SET,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_UNLINK,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_USR_INFO,
		LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_DOWN,

	};

	if (arg == NULL)
		return (uint32_t)EDU_EXIT;

	addr = *(SaUint32T *)arg;

	switch (addr) {
	case CPD_A2S_MSG_CKPT_CREATE:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_CREATE;
	case CPD_A2S_MSG_CKPT_DEST_ADD:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_ADD;
	case CPD_A2S_MSG_CKPT_DEST_DEL:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_DEL;
	case CPD_A2S_MSG_CKPT_RDSET:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_RDSET;
	case CPD_A2S_MSG_CKPT_AREP_SET:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_AREP_SET;
	case CPD_A2S_MSG_CKPT_UNLINK:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_UNLINK;
	case CPD_A2S_MSG_CKPT_USR_INFO:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_USR_INFO;
	case CPD_A2S_MSG_CKPT_DEST_DOWN:
		return LCL_TEST_JUMP_OFFSET_CPD_MBCSV_MSG_A2S_MSG_CKPT_DEST_DOWN;
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
		EDU_START, FUNC_NAME(CPD_MBCSV_MSG), 0, 0, 0, sizeof(DS), 0, NULL}
		, {
		EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, NULL}, {
		EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((DS *) 0)->type, 0, (void *)TEST_FUNC(CPD_MBCSV_MSG)}, {
		EDU_EXEC, FUNC_NAME(CPD_A2S_CKPT_CREATE), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_create, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.dest_add, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.dest_del, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_RDSET), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.rd_set, 0, NULL}, {
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.arep_set, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPD_A2S_CKPT_UNLINK), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.ckpt_ulink, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPD_A2S_CKPT_USR_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.usr_info, 0, NULL},
		{
		EDU_EXEC, FUNC_NAME(CPSV_CKPT_DEST_INFO), 0, 0, EDU_EXIT, (long)&((DS *) 0)->info.dest_down, 0, NULL},
		{
	EDU_END, 0, 0, 0, 0, 0, 0, NULL},};
	NCS_ENC_DEC_REM_FLOW(DS)
}

#undef DS
