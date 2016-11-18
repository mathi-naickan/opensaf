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

 MODULE NAME: ncs_edu_pub.h

 REVISION HISTORY:

 Date     Version  Name          Description
 -------- -------  ------------  --------------------------------------------
 27-11-03 1.00A    MCG(SAI)      Original

 ..............................................................................

 DESCRIPTION:
  This module contains customer-exposed definitions/prototypes required for
  using the EDU library for encode/decode operations.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */

#ifndef BASE_NCS_EDU_PUB_H_
#define BASE_NCS_EDU_PUB_H_

#include "base/ncspatricia.h"
#include "base/ncs_ubaid.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef NCS_EDU_VERBOSE_PRINT
#define NCS_EDU_VERBOSE_PRINT       0   /* Turning flag default OFF. */
#endif

/* EDU Instructions's maximum Label Size(in number of characters) */
#define EDU_INSTR_LABEL_MAX_SIZE     32

#define GL_LOG_STRING_LEN       64

typedef uint16_t EDU_MSG_VERSION;

/* EDU operation type */
typedef enum {
  EDP_OP_TYPE_ENC,
  EDP_OP_TYPE_DEC,
  EDP_OP_TYPE_MAX
} EDP_OP_TYPE;

/* Qualifiers used in the EDU Instruction Set */
#define EDQ_LNKLIST             0x00000001
#define EDQ_ARRAY               0x00000002      /* exec statement would pass "value"
                                                   for the number of array elements. */
#define EDQ_POINTER             0x00000004
#define EDQ_VAR_LEN_DATA        0x00000008

/* EDU instruction set */
typedef enum {
  EDU_START,      /* initialize program */
  EDU_EXEC,
  EDU_EXEC_EXT,   /* Extension to EDU_EXEC for variable-data */
  EDU_TEST_LL_PTR,        /* test if linked-list ptr is non-NULL,
                             and invoke another program. */
  EDU_TEST,       /* "test" statement */
  EDU_VER_USR,    /*to execute an user-provided .chk_ver_usr. function app ver info */
  EDU_VER_GE,
  EDU_END,        /* exit program */
  EDU_MAX
} EDU_INST_TYPE;

/* EDU global(and constant) GO-NEXT labels */
typedef enum {
  EDU_NEXT = 0xfffffff0,
  EDU_EXIT,
  EDU_SAME,       /* Run same command with new offset(for linked-lists, etc). */
  EDU_FAIL,
  EDU_LABEL_MAX
} EDU_LABEL;

/* EDU Error values */
typedef enum {
  EDU_NORMAL = 0xffff0000,
  EDU_ERR_MEM_FAIL,
  EDU_ERR_UBUF_PARSE_FAIL,
  EDU_ERR_INV_EDP_VALUE,
  EDU_ERR_EDU_START_NOT_FIRST_INSTR,
  EDU_ERR_EDP_NOT_MATCHING_IN_EDU_START_INSTR,
  EDU_ERR_INV_NUMBER_OF_EDU_INSTRUCTIONS,
  EDU_ERR_EDU_END_NOT_LAST_INSTR,
  EDU_ERR_ILLEGAL_INSTR_GIVEN,
  EDU_ERR_DUPLICATE_EDU_START_INSTR_FOUND,
  EDU_ERR_DUPLICATE_EDU_END_INSTR_FOUND,
  EDU_ERR_DUPLICATE_EDU_TEST_LL_PTR_INSTR_FOUND,
  EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE,
  EDU_ERR_VAR_LEN_PARAMETER_NOT_BASIC_EDP_TYPE,
  EDU_ERR_INV_ATTRIBUTE_FOR_LINKED_LIST_EDP,
  EDU_ERR_INV_ATTRIBUTE_COMBINATION_IN_START_INSTR,
  EDU_ERR_INV_ATTRIBUTE_FOR_EXEC_INSTR,
  EDU_ERR_EDP_REFERENCES_SELF,
  EDU_ERR_EXEC_INSTR_DOES_NOT_EXIST_FOR_OFFSET_OF_TEST_INSTR,
  EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC,
  EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_GE,
  EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_USR,
  EDU_ERR_EDP_NULL,
  EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME,
  EDU_ERR_INV_LEN_SIZE_FOUND_FOR_VAR_SIZED_DATA,
  EDU_ERR_TEST_FUNC_NULL,
  EDU_ERR_SRC_POINTER_NULL,
  EDU_ERR_DEST_DOUBLE_POINTER_NULL,
  EDU_ERR_EDU_HDL_NULL,
  EDU_ERR_UBAID_POINTER_NULL,
  EDU_ERR_POINTER_TO_CNT_NULL,
  EDU_ERR_POINTER_TO_EDU_ERR_RET_VAL_NULL,
  EDU_ERR_INV_OP_TYPE,
  EDU_ERR_EDU_TEST_LL_PTR_INSTR_NOT_FOUND,
  EDU_ERR_ILLEGAL_NEXT_LABEL_VALUE,
  EDU_ERR_EDP_NOT_USABLE_AT_EXEC_TIME,
  EDU_ERR_TLV_BUF_POINTER_NULL,
  EDU_ERR_INV_TLV_BUF_SIZE,
  EDU_ERR_EDU_HDL_NOT_INITED_BY_OWNER,
  EDU_ERR_SELECTIVE_EXECUTE_OP_FAIL,      /* Selective encode/decode of rules failed */
  EDU_ERR_VER_GE_FIELD_NULL,
  EDU_ERR_VER_USR_FIELD_NULL,
  EDU_ERR_MAX
} EDU_ERR;

/* EDU Handle which resides in the application's Control Block(CB).
   This gets passed to EDU at time of encode/decode operation.
   This is used by EDU to store(per-thread/process) :
   - label information of each EDP
   - EDCOMPILE status of EDP
*/
typedef struct edu_hdl_tag {
  bool is_inited; /* Is the tree initialised */
  NCS_PATRICIA_TREE tree;
  EDU_MSG_VERSION to_version;
} EDU_HDL;

struct edu_tkn_tag;     /* Forward declaration required here. */

/* Definition of EDU_TLV_ENV, used in EDU internal API. */
typedef struct edu_tlv_env_tag {
  NCSCONTEXT cur_bufp;    /* Running buffer pointer */
  uint32_t size;
  uint32_t bytes_consumed;
} EDU_TLV_ENV;

/* Definition of envelope of EDU_TLV_ENV and NCS_UBAID */
typedef struct edu_buf_env_tag {
  bool is_ubaid;  /* Is this NCS_UBAID or not? */
  union {
    NCS_UBAID *uba;
    EDU_TLV_ENV tlv_env;
  } info;
} EDU_BUF_ENV;

/* Prototype for function handler for test-functionality. This
   function is expected to return the label of the next
   instruction to execute. */
typedef uint32_t (*EDU_PROG_HANDLER) (EDU_HDL *edu_hdl, struct edu_tkn_tag * edu_tkn,
                                      NCSCONTEXT data_ptr, uint32_t *ptr_data_len,
                                      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
typedef int (*EDU_EXEC_RTINE) (NCSCONTEXT arg);

/* EDU's Prettyprint database(Internal use only). */
#if (NCS_EDU_VERBOSE_PRINT == 1)
typedef struct edu_ppdb {
  NCS_PATRICIA_TREE tree;
  bool is_up;     /* Has the tree been init'ed */
} EDU_PPDB;

/* Key to the EDU_PPDB database. */
typedef struct edu_ppdb_key {
  EDU_PROG_HANDLER parent_edp;    /* EDP of parent structure containing this field */
  EDU_PROG_HANDLER self_edp;      /* EDP of the data type of this field */
  uint32_t field_offset;  /* Offset of the field in the parent structure */
} EDU_PPDB_KEY;

/* Structure for a node of the EDU's prettyprint database. */
typedef struct edu_ppdb_node_info {
  NCS_PATRICIA_NODE pat_node;
  EDU_PPDB_KEY key;
  NCSCONTEXT data_ptr;    /* Pointer to data memory. This
                             memory is malloc'ed by EDU
                             while Pretty-printing. */
  uint32_t data_size;     /* Size of the data present. */
  uint8_t refcount;       /* Number of instructions
                             referencing this node. */
} EDU_PPDB_NODE_INFO;
#endif

/* EDU Token definition. This is the application's passport
   for passing the EDP program that has to be used for encoding/decoding.
*/
typedef struct edu_tkn_tag {
  EDU_PROG_HANDLER i_edp; /* EDP(of type EDU_PROG_HANDLER) used for
                             encoding/decoding. */

  EDU_PROG_HANDLER parent_edp;    /* EDP of the parent message envelope */
  uint8_t var_cnt;        /* Variable number of "selected-rules"(user-provided) for
                             performing encode/decode operation. */

  int *var_array; /* Alloc'ed Array of "selected-rules"(user-provided) */

#if(NCS_EDU_VERBOSE_PRINT == 1)
  EDU_PPDB ppdb;
#endif
} EDU_TKN;

/* EDU Instruction Set Definition */
typedef struct edu_inst_set {
  EDU_INST_TYPE instr;    /* EDU instruction type */

  EDU_PROG_HANDLER fld1;

  uint32_t fld2;

  EDU_PROG_HANDLER fld3;

  int nxt_lbl;    /* Next label to execute */

  long fld5;

  long fld6;

  EDU_EXEC_RTINE fld7;    /* pointer to "test" function */
} EDU_INST_SET;

/* EDU "test"able Instructions list(Internal use only) */
typedef struct edp_test_instr_rec_tag {
  uint32_t offset;        /* Of the "test"able field */
  EDU_PROG_HANDLER edp;   /* Of the "test"able field */
  uint32_t refcount;      /* Number of references by EDU_TEST instructions. */
  struct edp_test_instr_rec_tag *next;
} EDP_TEST_INSTR_REC;

/* EDU handle node type definition */
typedef struct edu_hdl_node_tag {
  NCS_PATRICIA_NODE pat_node;

  EDU_PROG_HANDLER edp;   /* EDU Program handler(Key to Patricia-tree) */

  uint32_t size;  /* Size of data structure */

  bool edcompile_pass;    /* Updated by EDU */

  uint32_t attrb; /* EDP attributes */

  EDP_TEST_INSTR_REC *test_instr_store;   /* Internal only */

} EDU_HDL_NODE;

/************* EDU EXTERNAL API (to Service Users) *************/
/* To be invoked for encoding/decoding into/from NCS_UBAID. */
#define m_NCS_EDU_EXEC(edu_hdl, edp_ptr, uba, op, data_ptr, o_err)      \
  ncs_edu_ver_exec(edu_hdl, edp_ptr, uba, op, data_ptr, o_err, 1 /* to_version */, 0 /* var_cnt */ )

#define m_NCS_EDU_VER_EXEC(edu_hdl,edp_ptr,uba,op,data_ptr,o_err,to_version) \
  ncs_edu_ver_exec(edu_hdl, edp_ptr, uba, op, data_ptr,o_err,to_version,0)

#define ncs_edu_exec(edu_hdl, edp_ptr, uba, op, data_ptr,o_err,var_cnt, ...) \
  ncs_edu_ver_exec(edu_hdl,edp_ptr,uba, op,data_ptr,o_err, 1 ,var_cnt,## __VA_ARGS__)

#define m_NCS_EDU_SEL_VER_EXEC(edu_hdl,edp_ptr,uba,op,data_ptr,o_err,to_version,var_cnt,...) \
  ncs_edu_ver_exec(edu_hdl, edp_ptr, uba, op, data_ptr,o_err,to_version,var_cnt, ## __VA_ARGS__)

/* To be invoked for encoding/decoding into/from TLV-buffer. */
#define m_NCS_EDU_TLV_EXEC(edu_hdl, edp_ptr, bufp, buf_size, op, data_ptr, o_err) \
  ncs_edu_tlv_exec(edu_hdl, edp_ptr, bufp, buf_size, op, data_ptr, o_err, 0)

/* To be invoked in an EDU program for executing it's rules on NCS_UBAID/TLV-buffer. */
#define m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, prog, ptr, ptr_data_len, buf_env, optype, o_err) \
  ncs_edu_run_rules(edu_hdl, edu_tkn, prog, ptr, ptr_data_len, buf_env, optype, o_err, \
                    sizeof(prog)/sizeof(EDU_INST_SET))

/* To be invoked by application for validating EDP */
#define m_NCS_EDU_COMPILE_EDP(edu_hdl, prog, o_err)     \
  ncs_edu_compile_edp(edu_hdl, prog, NULL, o_err)

#define m_NCS_EDU_HDL_INIT(edu_hdl) ncs_edu_hdl_init(edu_hdl)

#define m_NCS_EDU_HDL_FLUSH(edu_hdl) ncs_edu_hdl_flush(edu_hdl)

#if (NCS_EDU_VERBOSE_PRINT == 1)
#define m_NCS_EDU_PRINT_ERROR_STRING(eduerr) ncs_edu_print_error_string(eduerr)
#else
#define m_NCS_EDU_PRINT_ERROR_STRING(eduerr)
#endif
/************* EDU EXTERNAL API (to Service Users) *************/

/************ EDU external macro-related functions. ************/
uint32_t
ncs_edu_run_rules(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, EDU_INST_SET prog[],
                  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
                  EDP_OP_TYPE optype, EDU_ERR *o_err, int instr_count);


uint32_t ncs_edu_compile_edp(EDU_HDL *edu_hdl, EDU_PROG_HANDLER prog,
                             EDU_HDL_NODE **p_hdl_node, EDU_ERR *o_err);

uint32_t ncs_edu_hdl_init(EDU_HDL *edu_hdl);

uint32_t ncs_edu_hdl_flush(EDU_HDL *edu_hdl);

uint32_t
ncs_edu_ver_exec(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp, NCS_UBAID *uba,
                 EDP_OP_TYPE op, NCSCONTEXT arg, EDU_ERR *o_err, EDU_MSG_VERSION to_version,
                 uint8_t var_cnt, ...);

uint32_t
ncs_edu_tlv_exec(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp, NCSCONTEXT bufp,
                 uint32_t buf_size, EDP_OP_TYPE op, NCSCONTEXT data_ptr, EDU_ERR *o_err, uint8_t var_cnt, ...);

void ncs_edu_print_error_string(int enum_val);
/************ EDU external macro-related functions. ************/

/************* NCS Built-in EDU Program Prototypes *************/
uint32_t ncs_edp_ncs_bool(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                          NCSCONTEXT ptr, uint32_t *ptr_data_len,
                          EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_uns8(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                      NCSCONTEXT ptr, uint32_t *ptr_data_len,
                      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_uns16(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_uns32(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t ncs_edp_uns64(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_int64(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t ncs_edp_char(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                      NCSCONTEXT ptr, uint32_t *ptr_data_len,
                      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_string(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                        NCSCONTEXT ptr, uint32_t *ptr_data_len,
                        EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_short(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_int(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                     NCSCONTEXT ptr, uint32_t *ptr_data_len,
                     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t ncs_edp_int8(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                      NCSCONTEXT ptr, uint32_t *ptr_data_len,
                      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_int16(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t ncs_edp_int32(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                       NCSCONTEXT ptr, uint32_t *ptr_data_len,
                       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#define ncs_edp_double ncs_edp_int64
#define ncs_edp_float ncs_edp_int32
#define ncs_edp_float ncs_edp_int32

#define ncs_edp_mds_dest ncs_edp_uns64

uint32_t ncs_edp_ncs_key(EDU_HDL *hdl, EDU_TKN *edu_tkn,
                         NCSCONTEXT ptr, uint32_t *ptr_data_len,
                         EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

int ncs_edu_ncs_key_test_fmat_fnc(NCSCONTEXT arg);

/************* NCS Built-in EDU Program Prototypes *************/

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_EDU_PUB_H_
