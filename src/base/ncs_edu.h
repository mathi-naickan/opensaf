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

 MODULE NAME: ncs_edu.h

 REVISION HISTORY:

 Date     Version  Name          Description
 -------- -------  ------------  --------------------------------------------
 27-11-03 1.00A    MCG(SAI)      Original

 ..............................................................................

 DESCRIPTION:
  This module contains definitions/prototypes required for the EDU library.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCS_EDU_H_
#define BASE_NCS_EDU_H_

#include "base/ncs_edu_pub.h"


/* TLV Header size(for Type and Length) */
#define EDU_TLV_HDR_SIZE            3

/* EDU internal operation types. */
#if (NCS_EDU_VERBOSE_PRINT == 1)
#define EDP_OP_TYPE_PP      0xffff0001
#endif
#define EDP_OP_TYPE_ADMIN   0xffff0002

/* Format types, proprietory to NCS used in EDU */
typedef enum ncs_edu_fmat_tag {
  NCS_EDU_FMAT_8BIT,  /* 8 bit    */
  NCS_EDU_FMAT_16BIT, /* 16 bit   */
  NCS_EDU_FMAT_32BIT, /* 32 bit   */
  NCS_EDU_FMAT_64BIT, /* 64 bit   */
  NCS_EDU_FMAT_OCT,   /* Octet */
  NCS_EDU_FMAT_CNT    /* Count of instances */
} NCS_EDU_FMAT;

/* Administrative operations on the EDU programs. Responsibility of EDU
   only. */
typedef enum {
  NCS_EDU_ADMIN_OP_TYPE_COMPILE,
  NCS_EDU_ADMIN_OP_TYPE_GET_ATTRB,
  NCS_EDU_ADMIN_OP_TYPE_GET_LL_NEXT_OFFSET,
  NCS_EDU_ADMIN_OP_TYPE_MAX
} NCS_EDU_ADMIN_OP_TYPE;

typedef struct ncs_edu_admin_compile_info_tag {
  uint32_t i_dummy;
  uint32_t *o_retval;
} NCS_EDU_ADMIN_COMPILE_INFO;

typedef struct ncs_edu_admin_get_attrb_info_tag {
  uint32_t *o_attrb;              /* Storage of this (most likely) to be given by EDU(not application) */
} NCS_EDU_ADMIN_GET_ATTRB_INFO;

typedef struct ncs_edu_admin_get_ll_next_offset_tag {
  uint32_t *o_next_offset;        /* Storage, the responsibility of EDU */
} NCS_EDU_ADMIN_GET_LL_NEXT_OFFSET;

typedef struct ncs_edu_admin_op_tag {
  NCS_EDU_ADMIN_OP_TYPE adm_op_type;
  union {
    NCS_EDU_ADMIN_COMPILE_INFO compile_info;
    NCS_EDU_ADMIN_GET_ATTRB_INFO get_attrb; /* Get attributes of program,
                                               like PTR | ARR | LINKEDLIST, etc. */
    NCS_EDU_ADMIN_GET_LL_NEXT_OFFSET get_ll_offset; /* Location of "next" pointer of
                                                       linked list. */
  } info;
} NCS_EDU_ADMIN_OP_INFO;

/********************* EDU Internal Macros *********************/
#define m_NCS_EDU_TKN_INIT(edu_tkn) ncs_edu_tkn_init(edu_tkn)

#define m_NCS_EDU_TKN_FLUSH(edu_tkn) ncs_edu_tkn_flush(edu_tkn)

#define m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, edp, ptr, dcnt, buf_env, optype, o_err) \
  ncs_edu_run_edp(edu_hdl, edu_tkn, rule, edp, ptr, dcnt, buf_env, optype, o_err)

#define m_NCS_EDU_EXEC_RULE(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, optype, o_err) \
  ncs_edu_exec_rule(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_TEST_LL_RULE(rule, ptr, optype, o_err)    \
  ncs_edu_run_test_ll_rule(rule, ptr, optype, o_err)

#define m_NCS_EDU_RUN_TEST_CONDITION(edu_hdl, rule, ptr, buf_env, optype, o_err) \
  ncs_edu_run_test_condition(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_VER_GE(edu_hdl, rule, ptr, buf_env, optype, o_err) \
  ncs_edu_run_version_ge(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_VER_USR(edu_hdl, rule, ptr, buf_env, optype, o_err) \
  ncs_edu_run_version_usr(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_GET_REFCOUNT_OF_TESTABLE_FIELD(inst_store, rule)      \
  ncs_edu_get_refcount_of_testable_field(inst_store, rule)
/********************* EDU Internal Macros *********************/

/************ EDU internal macro-related functions. ************/
void ncs_edu_tkn_init(EDU_TKN *edu_tkn);

void ncs_edu_tkn_flush(EDU_TKN *edu_tkn);

uint32_t ncs_edu_run_edcompile_on_edp(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node, EDU_ERR *o_err);

uint32_t ncs_edu_perform_pp_op(EDU_HDL *edu_hdl,
                               EDU_PROG_HANDLER edp,
                               EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
                               EDU_ERR *o_err, uint8_t var_cnt, int *var_array);

uint32_t ncs_edu_perform_enc_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
                                EDU_BUF_ENV *buf_env, uint32_t *cnt, NCSCONTEXT arg,
                                EDU_ERR *o_err, uint8_t var_cnt, int *var_array);

uint32_t ncs_edu_perform_dec_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
                                EDU_BUF_ENV *buf_env, uint32_t *cnt, NCSCONTEXT arg,
                                EDU_ERR *o_err, uint8_t var_cnt, int *var_array);

uint32_t
ncs_edu_run_edp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, EDU_INST_SET *rule,
                EDU_PROG_HANDLER edp, NCSCONTEXT ptr, uint32_t *cnt,
                EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

int
ncs_edu_exec_rule(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                  EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule,
                  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

EDU_LABEL
ncs_edu_run_rules_for_enc(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                          EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
                          NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

EDU_LABEL
ncs_edu_run_rules_for_dec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                          EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
                          NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

EDU_LABEL
ncs_edu_run_rules_for_pp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                         EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
                         NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

uint32_t
ncs_edu_run_rules_for_compile(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node,
                              EDU_INST_SET *prog, NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_ERR *o_err, int instr_count);

uint32_t
ncs_edu_perform_exec_action(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                            EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule,
                            EDP_OP_TYPE optype, NCSCONTEXT ptr,
                            uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

uint32_t
ncs_edu_perform_exec_action_on_non_ptr(EDU_HDL *edu_hdl,
                                       EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
                                       EDU_INST_SET *rule, EDP_OP_TYPE optype,
                                       NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

uint32_t
ncs_edu_prfm_enc_on_non_ptr(EDU_HDL *edu_hdl,
                            EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
                            EDU_INST_SET *rule, NCSCONTEXT ptr,
                            uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

uint32_t
ncs_edu_prfm_dec_on_non_ptr(EDU_HDL *edu_hdl,
                            EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
                            EDU_INST_SET *rule, NCSCONTEXT ptr,
                            uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

#if (NCS_EDU_VERBOSE_PRINT == 1)
uint32_t
ncs_edu_prfm_pp_on_non_ptr(EDU_HDL *edu_hdl,
                           EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
                           EDU_INST_SET *rule, NCSCONTEXT ptr,
                           uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);
#endif

bool ncs_edu_is_edp_builtin(EDU_PROG_HANDLER prog);

bool ncs_edu_return_builtin_edp_size(EDU_PROG_HANDLER prog, uint32_t *o_size);

EDU_LABEL ncs_edu_run_test_ll_rule(EDU_INST_SET *rule, NCSCONTEXT ptr, EDP_OP_TYPE optype, EDU_ERR *o_err);

int
ncs_edu_run_test_condition(EDU_HDL *edu_hdl,
                           EDU_INST_SET *rule, NCSCONTEXT ptr,
                           EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

int
ncs_edu_run_version_ge(EDU_HDL *edu_hdl,
                       EDU_INST_SET *rule, NCSCONTEXT ptr, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

int
ncs_edu_run_version_usr(EDU_HDL *edu_hdl,
                        EDU_INST_SET *rule, NCSCONTEXT ptr, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);
uint32_t
ncs_edu_validate_and_gen_test_instr_rec_list(EDP_TEST_INSTR_REC **head,
                                             EDU_INST_SET *rules_head, int instr_count, EDU_ERR *o_err);

void ncs_edu_free_test_instr_rec_list(EDP_TEST_INSTR_REC *head);

uint32_t ncs_edu_get_refcount_of_testable_field(EDP_TEST_INSTR_REC *inst_store, EDU_INST_SET *rule);

void ncs_edu_log_msg(char *string);

void ncs_edu_skip_space(EDU_TLV_ENV *tlv_env, uint32_t cnt);

uint32_t ncs_encode_tlv_8bit(uint8_t **stream, uint32_t val);

uint32_t ncs_encode_tlv_16bit(uint8_t **stream, uint32_t val);

uint32_t ncs_encode_tlv_32bit(uint8_t **stream, uint32_t val);

uint32_t ncs_encode_tlv_64bit(uint8_t **stream, uint64_t val);

uint32_t ncs_encode_tlv_n_32bit(uint8_t **stream, uint32_t *val_ptr, uint16_t n_count);

uint32_t ncs_encode_tlv_n_16bit(uint8_t **stream, uint16_t *val_ptr, uint16_t n_count);

uint32_t ncs_encode_tlv_n_octets(uint8_t **stream, uint8_t *val, uint16_t count);

uint32_t ncs_decode_tlv_32bit(uint8_t **stream);

uint64_t ncs_decode_tlv_64bit(uint8_t **stream);

uint16_t ncs_decode_tlv_16bit(uint8_t **stream);

uint16_t ncs_decode_tlv_n_32bit(uint8_t **stream, uint32_t *dest);

uint16_t ncs_decode_tlv_n_16bit(uint8_t **stream, uint16_t *dest);

uint8_t ncs_decode_tlv_8bit(uint8_t **stream);

uint8_t *ncs_decode_tlv_n_octets(uint8_t *src, uint8_t *dest, uint32_t count);

uint8_t *ncs_copy_tlv_n_octets(uint8_t *src, uint8_t *dest, uint32_t count);

/************ EDU internal macro-related functions. ************/

#endif  // BASE_NCS_EDU_H_
