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

  DESCRIPTION: Implementation for NCS_EDU service, a generic library service
  to facilitate encoding/decoding/pretty-printing of USRBUFs.

******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncssysf_def.h"
#include "ncssysf_mem.h"
#include "sysf_def.h"
#include "patricia.h"
#include "ncs_stack.h"
#include "ncssysfpool.h"
#include "ncsencdec_pub.h"
#include "ncs_edu.h"
#include "usrbuf.h"

 char gl_log_string[GL_LOG_STRING_LEN];

#if(NCS_EDU_VERBOSE_PRINT == 1)
static uint32_t ncs_edu_ppdb_init(EDU_PPDB * ppdb);
static void ncs_edu_ppdb_destroy(EDU_PPDB * ppdb);
static void
edu_populate_ppdb_key(EDU_PPDB_KEY * key, EDU_PROG_HANDLER parent_edp, EDU_PROG_HANDLER self_edp, uint32_t offset);
static EDU_PPDB_NODE_INFO *edu_ppdb_node_findadd(EDU_PPDB * ppdb,
						 EDU_PROG_HANDLER parent_edp,
						 EDU_PROG_HANDLER self_edp, uint32_t offset, bool add);
static void edu_ppdb_node_del(EDU_PPDB * ppdb, EDU_PPDB_NODE_INFO * node);
#endif   /* #if(NCS_EDU_VERBOSE_PRINT == 1) */

static void ncs_edu_free_uba_contents(NCS_UBAID *p_uba);

static void edu_hdl_node_del(EDU_HDL *edu_hdl, EDU_HDL_NODE *node);

static uint32_t ncs_edu_get_size_of_var_len_data(EDU_PROG_HANDLER edp, NCSCONTEXT cptr, uint32_t *p_data_len, EDU_ERR *o_err);

static int edu_chk_ver_ge(EDU_MSG_VERSION *local_version, EDU_MSG_VERSION *to_version, int skip_cnt_if_ne);

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_hdl_init

  DESCRIPTION:      Function to initialise EDU_HDL before every encode/decode
                    operation. This is an internal API invoked by EDU.

  RETURNS:
     NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
void ncs_edu_tkn_init(EDU_TKN *edu_tkn)
{
#if(NCS_EDU_VERBOSE_PRINT == 1)
	ncs_edu_ppdb_init(&edu_tkn->ppdb);
#endif
	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_tkn_flush

  DESCRIPTION:      Function to flush EDU_TKN after every encode/decode
                    operation(apart from freeing any malloc'ed data). This is 
                    an internal API invoked by EDU.

  RETURNS:
     NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
void ncs_edu_tkn_flush(EDU_TKN *edu_tkn)
{
#if(NCS_EDU_VERBOSE_PRINT == 1)
	ncs_edu_ppdb_destroy(&edu_tkn->ppdb);
#endif
	return;
}

static int edu_chk_ver_ge(EDU_MSG_VERSION *local_version, EDU_MSG_VERSION *to_version, int skip_cnt_if_ne)
{
	if (*to_version >= *local_version)
		return EDU_NEXT;
	else
		return skip_cnt_if_ne;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_ver_exec

  DESCRIPTION:      Main function to execute encode/decode operations on 
                    USRBUFs. This function also manages the EDU header.
                    This is the External API provided to EDU-service-users
                    (i.e., the applications). 
                    
                    After every encode operation, the encoded-NCS_UBAID 
                    contents are pretty-printed onto the log file(if the 
                    build flag NCS_EDU_VERBOSE_PRINT is turned On). Similarly, 
                    before every decode operation, the contents of NCS_UBAID 
                    are pretty-printed(if the build flag NCS_EDU_VERBOSE_PRINT 
                    is turned On).

  RETURNS:
     NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_ver_exec(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp, NCS_UBAID *uba,
		       EDP_OP_TYPE op, NCSCONTEXT arg, EDU_ERR *o_err, EDU_MSG_VERSION to_version, uint8_t var_cnt, ...)
{
	uint32_t rc = NCSCC_RC_SUCCESS, lcl_cnt = 0;
	EDU_BUF_ENV lcl_edu_buf;
	va_list arguments;	/* A place to store the list of arguments */
	int *var_array = NULL;

	/* Error checks done here. */
	if (o_err == NULL) {
		m_NCS_EDU_PRINT_ERROR_STRING(EDU_ERR_POINTER_TO_EDU_ERR_RET_VAL_NULL);
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	if (edu_hdl == NULL) {
		*o_err = EDU_ERR_EDU_HDL_NULL;
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	if (!edu_hdl->is_inited) {
		/* The application didn't init the EDU_HDL. Fatal Error!!!! */
		*o_err = EDU_ERR_EDU_HDL_NOT_INITED_BY_OWNER;
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	if (edp == NULL) {
		*o_err = EDU_ERR_EDP_NULL;
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	if (uba == NULL) {
		*o_err = EDU_ERR_UBAID_POINTER_NULL;
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	switch (op) {
	case EDP_OP_TYPE_ENC:
	case EDP_OP_TYPE_DEC:
		break;
	default:
		*o_err = EDU_ERR_INV_OP_TYPE;
		m_LEAP_DBG_SINK_VOID;
		ncs_edu_free_uba_contents(uba);
		return NCSCC_RC_FAILURE;
	}
	if (arg == NULL) {
		if (op == EDP_OP_TYPE_ENC) {
			/* At encode time, source pointer can't be NULL. */
			*o_err = EDU_ERR_SRC_POINTER_NULL;
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return NCSCC_RC_FAILURE;
		} else {
			*o_err = EDU_ERR_DEST_DOUBLE_POINTER_NULL;
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return NCSCC_RC_FAILURE;
		}
	}
	if (var_cnt != 0) {
		/* Get the variable-arguments of this function into our storage */
		int x = 0;

		var_array = m_MMGR_ALLOC_EDP_SELECT_ARRAY(var_cnt);

		va_start(arguments, var_cnt);
		for (x = 0; x < var_cnt; x++) {
			var_array[x] = va_arg(arguments, int);
		}
		va_end(arguments);
	}
	/* Error checks end here. */

	memset(&lcl_edu_buf, '\0', sizeof(lcl_edu_buf));
	lcl_edu_buf.is_ubaid = true;
	lcl_edu_buf.info.uba = uba;

	edu_hdl->to_version = to_version;

#if (NCS_EDU_VERBOSE_PRINT == 1)
	/* Perform Pretty-print before Decoding. */
	if (op == EDP_OP_TYPE_DEC) {
		rc = ncs_edu_perform_pp_op(edu_hdl, edp, &lcl_edu_buf, op, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return rc;
		}
	}
#endif

	/* Perform actual encode/decode operation here. */
	if (op == EDP_OP_TYPE_ENC) {
		rc = ncs_edu_perform_enc_op(edu_hdl, edp, &lcl_edu_buf, &lcl_cnt, arg, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return rc;
		}
	} else if (op == EDP_OP_TYPE_DEC) {
		rc = ncs_edu_perform_dec_op(edu_hdl, edp, &lcl_edu_buf, &lcl_cnt, arg, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return rc;
		}
	}
#if (NCS_EDU_VERBOSE_PRINT == 1)
	/* Perform Pretty-print after Encoding. */
	if (op == EDP_OP_TYPE_ENC) {
		rc = ncs_edu_perform_pp_op(edu_hdl, edp, &lcl_edu_buf, op, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			ncs_edu_free_uba_contents(uba);
			return rc;
		}
	}
#endif

	if (var_array != NULL) {
		m_MMGR_FREE_EDP_SELECT_ARRAY(var_array);
		var_array = NULL;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_tlv_exec

  DESCRIPTION:      Main function to execute encode/decode operations on 
                    TLV-buffers. This is the External API provided to 
                    EDU-service-users(i.e., the applications). 

                    After every encode operation, the encoded-TLV-buffer
                    contents are pretty-printed onto the log file(if the 
                    build flag NCS_EDU_VERBOSE_PRINT is turned On). Similarly, 
                    before every decode operation, the contents of TLV-buffer
                    are pretty-printed(if the build flag NCS_EDU_VERBOSE_PRINT 
                    is turned On).

  RETURNS:
     NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_tlv_exec(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
		       NCSCONTEXT bufp, uint32_t buf_size, EDP_OP_TYPE op,
		       NCSCONTEXT arg, EDU_ERR *o_err, uint8_t var_cnt, ...)
{
	uint32_t rc = NCSCC_RC_SUCCESS, lcl_cnt = 0;
	EDU_BUF_ENV lcl_edu_buf;
	va_list arguments;	/* A place to store the list of arguments */
	int *var_array = NULL;

	/* Error checks done here. */
	if (o_err == NULL) {
		m_NCS_EDU_PRINT_ERROR_STRING(EDU_ERR_POINTER_TO_EDU_ERR_RET_VAL_NULL);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (edu_hdl == NULL) {
		*o_err = EDU_ERR_EDU_HDL_NULL;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (!edu_hdl->is_inited) {
		/* The application didn't init the EDU_HDL. Fatal Error!!!! */
		*o_err = EDU_ERR_EDU_HDL_NOT_INITED_BY_OWNER;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (edp == NULL) {
		*o_err = EDU_ERR_EDP_NULL;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (bufp == NULL) {
		*o_err = EDU_ERR_TLV_BUF_POINTER_NULL;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (buf_size == 0) {
		*o_err = EDU_ERR_INV_TLV_BUF_SIZE;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	switch (op) {
	case EDP_OP_TYPE_ENC:
	case EDP_OP_TYPE_DEC:
		break;
	default:
		*o_err = EDU_ERR_INV_OP_TYPE;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (arg == NULL) {
		if (op == EDP_OP_TYPE_ENC) {
			/* At encode time, source pointer can't be NULL. */
			*o_err = EDU_ERR_SRC_POINTER_NULL;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		} else {
			*o_err = EDU_ERR_DEST_DOUBLE_POINTER_NULL;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	}
	if (var_cnt != 0) {
		/* Get the variable-arguments of this function into our storage */
		int x = 0;

		var_array = m_MMGR_ALLOC_EDP_SELECT_ARRAY(var_cnt);

		va_start(arguments, var_cnt);
		for (x = 0; x < var_cnt; x++) {
			var_array[x] = va_arg(arguments, int);
		}
		va_end(arguments);
	}
	/* Error checks end here. */

	memset(&lcl_edu_buf, '\0', sizeof(lcl_edu_buf));
	lcl_edu_buf.info.tlv_env.cur_bufp = bufp;
	lcl_edu_buf.info.tlv_env.bytes_consumed = 0;
	lcl_edu_buf.info.tlv_env.size = buf_size;

#if (NCS_EDU_VERBOSE_PRINT == 1)
	/* Perform Pretty-print before Decoding. */
	if (op == EDP_OP_TYPE_DEC) {
		rc = ncs_edu_perform_pp_op(edu_hdl, edp, &lcl_edu_buf, op, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
	}
#endif

	/* Perform actual encode/decode operation here. */
	if (op == EDP_OP_TYPE_ENC) {
		rc = ncs_edu_perform_enc_op(edu_hdl, edp, &lcl_edu_buf, &lcl_cnt, arg, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
	} else if (op == EDP_OP_TYPE_DEC) {
		rc = ncs_edu_perform_dec_op(edu_hdl, edp, &lcl_edu_buf, &lcl_cnt, arg, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
	}
#if (NCS_EDU_VERBOSE_PRINT == 1)
	/* Perform Pretty-print after Encoding. */
	if (op == EDP_OP_TYPE_ENC) {
		/* Reset the lcl_edu_buf, for Pretty-print operation */
		lcl_edu_buf.info.tlv_env.cur_bufp = bufp;
		lcl_edu_buf.info.tlv_env.bytes_consumed = 0;
		rc = ncs_edu_perform_pp_op(edu_hdl, edp, &lcl_edu_buf, op, o_err, var_cnt, var_array);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
	}
#endif

	if (var_array != NULL) {
		m_MMGR_FREE_EDP_SELECT_ARRAY(var_array);
		var_array = NULL;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_edp

  DESCRIPTION:      This is an utility function, which triggers the EDP program
                    execution either from :
                        - external API
                        - internal rules
                    It also manages multiple invocation of the EDPs in case of
                    linked-lists.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_run_edp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, EDU_INST_SET *rule,
		      EDU_PROG_HANDLER edp, NCSCONTEXT ptr, uint32_t *dcnt,
		      EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	NCSCONTEXT lcl_ptr = NULL;
	EDU_HDL_NODE *lcl_hdl_node = NULL;
	uint32_t lcl_rc = NCSCC_RC_SUCCESS, dtype_attrb = 0x00000000;
	NCS_EDU_ADMIN_OP_INFO admin_op;
	uint32_t next_offset = 0, cnt = 0;	/* for linked list */
	uint8_t *p8, u8 = 0;
	uint16_t u16 = 0;

	if (edp == NULL) {
		*o_err = EDU_ERR_EDP_NULL;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Get hdl_node entry for the EDP. Run COMPILE if necessary. 
	   This function won't run COMPILE, if it's already COMPILEd
	   successfully. */
	lcl_rc = ncs_edu_compile_edp(edu_hdl, edp, &lcl_hdl_node, o_err);
	if (lcl_rc != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(lcl_rc);
	}

	if ((optype == EDP_OP_TYPE_DEC)
#if (NCS_EDU_VERBOSE_PRINT == 1)
	    || (optype == EDP_OP_TYPE_PP)
#endif
	    ) {
		/* Lookup data-type attributes, and if it's a linked-list/array,
		   run the loop here. */
		dtype_attrb = lcl_hdl_node->attrb;

		/* Now that dtype_attrb got filled, compare it with expected
		   values. */
		if ((dtype_attrb & EDQ_LNKLIST) == EDQ_LNKLIST) {
			/* Get next_offset. */
			admin_op.adm_op_type = NCS_EDU_ADMIN_OP_TYPE_GET_LL_NEXT_OFFSET;
			admin_op.info.get_ll_offset.o_next_offset = &next_offset;
			edp(edu_hdl, edu_tkn, (NCSCONTEXT)&admin_op, NULL, NULL, EDP_OP_TYPE_ADMIN, o_err);

			/* Decode the "count"(in network order) from the USRBUF */
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			cnt = (uint32_t)u16;
		} else if ((rule != NULL) && ((rule->fld2 & EDQ_POINTER) == EDQ_POINTER)) {
			/* Decode the "ptr_count" from the USRBUF */
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u8, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			cnt = u16;
		} else {
			cnt = 1;
			/* Note :- For array, loop'ing is done in m_NCS_EDU_EXEC_RULE. */
		}

		lcl_ptr = ptr;
		while (cnt != 0) {
			lcl_rc = edp(edu_hdl, edu_tkn, lcl_ptr, dcnt, buf_env, optype, o_err);
			if (lcl_rc != NCSCC_RC_SUCCESS) {
				m_LEAP_DBG_SINK_VOID;
				return lcl_rc;
			}
			if (optype == EDP_OP_TYPE_DEC) {
				/* Note for decoding, "ptr" is a double pointer. */
				if ((dtype_attrb & EDQ_LNKLIST) == EDQ_LNKLIST) {
					lcl_ptr = (NCSCONTEXT)((*(long *)lcl_ptr) + (long)next_offset);
				}
			} else {
				/* For Pretty-print. This might not be required. */
				lcl_ptr = NULL;
			}
			cnt--;
		}
	} else {
		/* Encode/Admin operations */
		if (optype == EDP_OP_TYPE_ADMIN) {
			admin_op = *((NCS_EDU_ADMIN_OP_INFO *)ptr);
		}
		lcl_rc = edp(edu_hdl, edu_tkn, ptr, dcnt, buf_env, optype, o_err);
	}

	return lcl_rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_rules

  DESCRIPTION:      This function is supposed to read the array of 
                    instructions, and implement them according to 
                    Encode/Decode/Prettyprint/Admin operation types.

  RETURNS:
     NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_run_rules(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			EDU_INST_SET prog[], NCSCONTEXT ptr,
			uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err, int instr_count)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	int cur_inst_indx = 0;
	EDU_HDL_NODE *hdl_node = NULL;
	EDU_LABEL edu_ret = EDU_EXIT;

	if (optype == EDP_OP_TYPE_ENC) {
		edu_ret = ncs_edu_run_rules_for_enc(edu_hdl, edu_tkn, hdl_node /* NULL */ ,
						    prog, ptr, ptr_data_len, buf_env, o_err, instr_count);
		if (edu_ret == EDU_FAIL) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	} else if (optype == EDP_OP_TYPE_DEC) {
		edu_ret = ncs_edu_run_rules_for_dec(edu_hdl, edu_tkn, hdl_node /* NULL */ ,
						    prog, ptr, ptr_data_len, buf_env, o_err, instr_count);
		if (edu_ret == EDU_FAIL) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	}
#if(NCS_EDU_VERBOSE_PRINT == 1)
	else if (optype == EDP_OP_TYPE_PP) {
		if ((hdl_node = (EDU_HDL_NODE *)
		     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&prog[0].fld1)) == NULL) {
			rc = ncs_edu_compile_edp(edu_hdl, prog[0].fld1, &hdl_node, o_err);
			if (rc != NCSCC_RC_SUCCESS) {
				*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}
		edu_ret = ncs_edu_run_rules_for_pp(edu_hdl, edu_tkn, hdl_node, prog,
						   ptr, ptr_data_len, buf_env, o_err, instr_count);
		if (edu_ret == EDU_FAIL) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	}
#endif
	else if (optype == EDP_OP_TYPE_ADMIN) {
		NCS_EDU_ADMIN_OP_INFO *admin_info = (NCS_EDU_ADMIN_OP_INFO *)ptr;

		switch (admin_info->adm_op_type) {
		case NCS_EDU_ADMIN_OP_TYPE_COMPILE:
			if ((hdl_node = (EDU_HDL_NODE *)
			     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&prog[0].fld1)) == NULL) {
				rc = ncs_edu_compile_edp(edu_hdl, prog[0].fld1, &hdl_node, o_err);
				if (rc != NCSCC_RC_SUCCESS) {
					return m_LEAP_DBG_SINK(rc);
				}
			}
			rc = ncs_edu_run_rules_for_compile(edu_hdl, hdl_node, prog,
							   ptr, ptr_data_len, o_err, instr_count);
			if (rc != NCSCC_RC_SUCCESS) {
				return m_LEAP_DBG_SINK(rc);
			}
			break;
		case NCS_EDU_ADMIN_OP_TYPE_GET_ATTRB:
			*(admin_info->info.get_attrb.o_attrb) = prog[0].fld2;
			break;
		case NCS_EDU_ADMIN_OP_TYPE_GET_LL_NEXT_OFFSET:
			{
				bool lcl_fnd = false;

				while ((cur_inst_indx != instr_count) && (prog[cur_inst_indx].instr != EDU_END)) {
					if (prog[cur_inst_indx].instr == EDU_TEST_LL_PTR) {
						lcl_fnd = true;
						*(admin_info->info.get_ll_offset.o_next_offset) =
						    prog[cur_inst_indx].fld5;
					}
					cur_inst_indx++;
				}
				if (!lcl_fnd) {
					*o_err = EDU_ERR_EDU_TEST_LL_PTR_INSTR_NOT_FOUND;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
			break;
		default:
			break;
		}
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_exec_rule

  DESCRIPTION:      Interprets and executes an individual EDP-rule.

  RETURNS:          uint32_t(depicting the label to execute next)
                    - EDU_SAME, in case of looping for linked-lists.
                    - EDU_FAIL, in case failure is encountered.
                    - EDU_EXIT, in case EDP program is executed completely.
                    - else, (uint32_t), i.e., label of next executable 
                      instruction.

*****************************************************************************/
int ncs_edu_exec_rule(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
		      EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule,
		      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	int rc_lbl = EDU_NEXT;
	EDU_LABEL ret_val = EDU_NEXT, rc = NCSCC_RC_SUCCESS;

	switch (rule->instr) {
	case EDU_START:
		break;
	case EDU_EXEC:
		rc = ncs_edu_perform_exec_action(edu_hdl, edu_tkn, hdl_node, rule, optype,
						 ptr, ptr_data_len, buf_env, o_err);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			return EDU_FAIL;
		}
		break;
	case EDU_TEST_LL_PTR:
#if (NCS_EDU_VERBOSE_PRINT == 1)
		if (optype == EDP_OP_TYPE_PP) {
			/* For Pretty-print, it is taken care in run_edp routine. */
			return EDU_SAME;
		}
#endif
		ret_val = m_NCS_EDU_RUN_TEST_LL_RULE(rule, ptr, optype, o_err);
		if (ret_val == EDU_SAME) {
			rc_lbl = EDU_SAME;
		} else if (ret_val == EDU_EXIT) {
			rc_lbl = EDU_EXIT;
		}
		break;
	case EDU_TEST:
#if (NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Test condition...\n");
		ncs_edu_log_msg(gl_log_string);

		/* Retrieve "ptr" from "PPDB" and pass to the "test" function.
		   To retrieve, use "self-edp", "parent-edp", "offset" as key
		   to the PPDB tree. 
		 */
		if (optype == EDP_OP_TYPE_PP) {
			EDU_PPDB_NODE_INFO *ppdb_node = NULL;
			if ((ppdb_node = edu_ppdb_node_findadd(&edu_tkn->ppdb,
							       hdl_node->edp, rule->fld1, rule->fld5, false)) == NULL) {
				/* Malloc failed. */
				m_LEAP_DBG_SINK_VOID;
				*o_err = EDU_ERR_MEM_FAIL;
				return EDU_FAIL;
			}
			/* Now, "ppdb_node->data_ptr" should contain the value to be
			   compared. */
			ptr = ppdb_node->data_ptr;
			rc_lbl = m_NCS_EDU_RUN_TEST_CONDITION(edu_hdl, rule, ptr, buf_env, optype, o_err);
			if (--ppdb_node->refcount == 0) {
				/* Destroy the node. */
				edu_ppdb_node_del(&edu_tkn->ppdb, ppdb_node);
				m_MMGR_FREE_EDU_PPDB_NODE_INFO(ppdb_node);
			}
		} else
#endif
		{
			rc_lbl = m_NCS_EDU_RUN_TEST_CONDITION(edu_hdl, rule, ptr, buf_env, optype, o_err);
		}
		break;
	case EDU_VER_GE:
		{
			rc_lbl = m_NCS_EDU_RUN_VER_GE(edu_hdl, rule, ptr, buf_env, optype, o_err);
		}

		break;
	case EDU_VER_USR:
		{

			rc_lbl = m_NCS_EDU_RUN_VER_USR(edu_hdl, rule, ptr, buf_env, optype, o_err);
		}
		break;
	case EDU_EXEC_EXT:
		break;
	case EDU_END:
		break;
	default:
		m_LEAP_DBG_SINK_VOID;
		*o_err = EDU_ERR_ILLEGAL_INSTR_GIVEN;
		return EDU_FAIL;
	}

	return rc_lbl;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_perform_exec_action

  DESCRIPTION:      Broader utility function to execute "EDU_EXEC" instruction
                    during all the 3 operations, encode/decode/pretty-print.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_perform_exec_action(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				  EDU_HDL_NODE *hdl_node,
				  EDU_INST_SET *rule, EDP_OP_TYPE optype,
				  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err)
{
	NCSCONTEXT newptr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if ((rule->fld2 & EDQ_POINTER) == EDQ_POINTER) {
		if (optype == EDP_OP_TYPE_ENC) {
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
					       (NCSCONTEXT)(*((long *)((long)ptr + (long)rule->fld5))),
					       ptr_data_len, buf_env, optype, o_err);
		} else if (optype == EDP_OP_TYPE_DEC) {
			newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
					       (NCSCONTEXT)newptr, ptr_data_len, buf_env, optype, o_err);
		}
#if (NCS_EDU_VERBOSE_PRINT == 1)
		else if (optype == EDP_OP_TYPE_PP) {
			/* It would create a tree element, and malloc the
			   required data(of the specified size) and store it
			   in the tree element. This would be done only if
			   this field has to be test'ed at a later stage. 
			   - Master-EDP
			   - EDP of the program which we in now.
			   - Offset of the field which is declared at 'test'able.
			 */
			uint32_t *p_data_len = ptr_data_len;
			bool lcl_is_tested = false;
			EDU_PPDB_NODE_INFO *ppdb_node = NULL;
			uint32_t refcount = 0;

			refcount = m_NCS_EDU_GET_REFCOUNT_OF_TESTABLE_FIELD(hdl_node->test_instr_store, rule);
			if (refcount != 0) {
				lcl_is_tested = true;
				if ((ppdb_node = edu_ppdb_node_findadd(&edu_tkn->ppdb,
								       hdl_node->edp, rule->fld1,
								       rule->fld5, true)) == NULL) {
					/* Malloc failed. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				ptr = &ppdb_node->data_ptr;
				p_data_len = &ppdb_node->data_size;
				rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, ptr,
						       p_data_len, buf_env, optype, o_err);
				if (rc != NCSCC_RC_SUCCESS)
					return rc;
			} else {
				/* Pass pointer as NULL */
				rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, NULL,
						       p_data_len, buf_env, optype, o_err);
				if (rc != NCSCC_RC_SUCCESS)
					return rc;
			}
			if (lcl_is_tested) {
				*ptr_data_len = *p_data_len;
			}
		}		/* else if(optype == EDP_OP_TYPE_PP) */
#endif   /* #if (NCS_EDU_VERBOSE_PRINT == 1) */
	} /* if((rule->fld2 & EDQ_POINTER) == EDQ_POINTER) */
	else {
		rc = ncs_edu_perform_exec_action_on_non_ptr(edu_hdl, edu_tkn, hdl_node,
							    rule, optype, ptr, ptr_data_len, buf_env, o_err);
	}
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_perform_exec_action_on_non_ptr

  DESCRIPTION:      Utility function to execute "EDU_EXEC" instruction on 
                    non-pointer data-fields during all the 3 operations, 
                    encode/decode/pretty-print.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t
ncs_edu_perform_exec_action_on_non_ptr(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				       EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule, EDP_OP_TYPE optype,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (optype == EDP_OP_TYPE_ENC) {
		rc = ncs_edu_prfm_enc_on_non_ptr(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, o_err);
	} else if (optype == EDP_OP_TYPE_DEC) {
		rc = ncs_edu_prfm_dec_on_non_ptr(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, o_err);
	}
#if (NCS_EDU_VERBOSE_PRINT == 1)
	else if (optype == EDP_OP_TYPE_PP) {
		rc = ncs_edu_prfm_pp_on_non_ptr(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, o_err);
	}
#endif

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_prfm_enc_on_non_ptr

  DESCRIPTION:      Utility function to execute "EDU_EXEC" instruction on 
                    non-pointer data-fields during encode operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_prfm_enc_on_non_ptr(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				  EDU_HDL_NODE *hdl_node,
				  EDU_INST_SET *rule, NCSCONTEXT ptr,
				  uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err)
{
	NCSCONTEXT newptr = NULL;
	EDU_HDL_NODE *lcl_hdl_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, lcnt = 0, l_size = 0;
	uint32_t lcl_cnt = 0;
	uint8_t *p8 = NULL;

	if ((rule->fld2 & EDQ_ARRAY) == EDQ_ARRAY) {
		uint32_t loop_cnt = rule->fld6;	/* Array size */

		newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);
		if (rule->fld1 == ncs_edp_char) {
			uint16_t str_len = strlen((char *)newptr);

			/* Encode in the form of character string, rather than an array of
			   characters. */
			if (buf_env->is_ubaid) {
				p8 = ncs_enc_reserve_space(buf_env->info.uba, 2);
				ncs_encode_16bit(&p8, str_len);
				ncs_enc_claim_space(buf_env->info.uba, 2);
			} else {
				p8 = buf_env->info.tlv_env.cur_bufp;
				ncs_encode_tlv_16bit(&p8, str_len);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			loop_cnt = str_len;
		}

		for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, newptr,
					       &lcl_cnt, buf_env, EDP_OP_TYPE_ENC, o_err);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
			if (!ncs_edu_return_builtin_edp_size(rule->fld1, &l_size)) {
				/* This is not builtin EDP */
				if ((lcl_hdl_node = (EDU_HDL_NODE *)
				     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&rule->fld1)) == NULL) {
					*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
				l_size = lcl_hdl_node->size;
			}
			newptr = (NCSCONTEXT)((long)newptr + (long)l_size);
		}
	} else {
		if ((rule->fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
			bool is_builtin_edp = true;

			/* The length of variable-sized-data is passed in the 
			   offset "fld6" of the data structure. */
			rc = ncs_edu_get_size_of_var_len_data(rule->fld3,
							      (NCSCONTEXT)((long)ptr + (long)rule->fld6),
							      ptr_data_len, o_err);
			if (rc != NCSCC_RC_SUCCESS) {
				return m_LEAP_DBG_SINK(rc);
			}
			if (!ncs_edu_return_builtin_edp_size(rule->fld1, &l_size)) {
				/* This is not builtin EDP */
				is_builtin_edp = false;

				if ((lcl_hdl_node = (EDU_HDL_NODE *)
				     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&rule->fld1)) == NULL) {
					*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
				l_size = lcl_hdl_node->size;
			}
			/* "*ptr_data_len" now contains the number of instances
			   of the data-type "rule->fld1". */
			newptr = (NCSCONTEXT)(*(long **)((long)ptr + (long)rule->fld5));
			if (is_builtin_edp) {
				if (buf_env->is_ubaid) {
					/* Enc/dec optimization only for uint8_t or char var-len-data */
					if ((l_size != 0)
					    && ((rule->fld1 == ncs_edp_char) || (rule->fld1 == ncs_edp_uns8))) {
						/* For uint8_t*, or char*, compute effective
						   bytes to encode in a single go. */
						lcl_cnt = (l_size * (*ptr_data_len));
						if (lcl_cnt != 0) {
							rc = ncs_encode_n_octets_in_uba(buf_env->info.uba, newptr,
											lcl_cnt);
							if (rc != NCSCC_RC_SUCCESS)
								return rc;
						}
					} else if (rule->fld1 == ncs_edp_string) {
						/* This is ncs_edp_string, can't be used with var-len-data attribute. */
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					} else {
						uint32_t loop_cnt = *ptr_data_len;

						for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
							rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
									       newptr, &lcl_cnt, buf_env,
									       EDP_OP_TYPE_ENC, o_err);
							if (rc != NCSCC_RC_SUCCESS)
								return rc;
							newptr = (NCSCONTEXT)((long)newptr + (long)l_size);
						}
					}
				} else {
					/* "*ptr_data_len" has instance-count, so we are done 
					   here. */
					if (*ptr_data_len != 0) {
						lcl_cnt = *ptr_data_len;
						rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
								       newptr, &lcl_cnt, buf_env,
								       EDP_OP_TYPE_ENC, o_err);
						if (rc != NCSCC_RC_SUCCESS)
							return rc;
					}
				}
			} else {
				uint32_t loop_cnt = *ptr_data_len;

				for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
					rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
							       newptr, &lcl_cnt, buf_env, EDP_OP_TYPE_ENC, o_err);
					if (rc != NCSCC_RC_SUCCESS)
						return rc;
					newptr = (NCSCONTEXT)((long)newptr + (long)l_size);

				}
			}
		} else if (rule->fld1 == ncs_edp_string) {
			char *lcl_str = NULL;
			uint32_t lcl_str_len = 0;

			/* Populate length of the source-data here. */
			newptr = (NCSCONTEXT)(*(long **)((long)ptr + (long)rule->fld5));
			lcl_str = (char *)newptr;
			if (lcl_str != NULL) {
				lcl_str_len = strlen(lcl_str);
			}
			*ptr_data_len = lcl_str_len;
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, newptr,
					       ptr_data_len, buf_env, EDP_OP_TYPE_ENC, o_err);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		} else {
			newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, newptr,
					       &lcl_cnt, buf_env, EDP_OP_TYPE_ENC, o_err);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}
	}
	*ptr_data_len = 0;
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_prfm_dec_on_non_ptr

  DESCRIPTION:      Utility function to execute "EDU_EXEC" instruction on 
                    non-pointer data-fields during decode operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_prfm_dec_on_non_ptr(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				  EDU_HDL_NODE *hdl_node,
				  EDU_INST_SET *rule, NCSCONTEXT ptr,
				  uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err)
{
	NCSCONTEXT newptr = NULL;
	EDU_HDL_NODE *lcl_hdl_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, lcnt = 0, lcl_cnt = 0;
	uint32_t l_size = 0;

	if ((rule->fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
		bool is_builtin_edp = true;

		/* The length of variable-sized-data is passed in the 
		   offset "fld6" of the data structure. */
		rc = ncs_edu_get_size_of_var_len_data(rule->fld3,
						      (NCSCONTEXT)((long)ptr + (long)rule->fld6), ptr_data_len, o_err);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
		if (!ncs_edu_return_builtin_edp_size(rule->fld1, &l_size)) {
			/* This is not builtin EDP */
			is_builtin_edp = false;

			if ((lcl_hdl_node = (EDU_HDL_NODE *)
			     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&rule->fld1)) == NULL) {
				*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			l_size = lcl_hdl_node->size;
		}

		/* "*ptr_data_len" now contains the number of instances
		   of the data-type "rule->fld1".
		   Malloc the data now itself. 
		 */
		if ((*ptr_data_len) != 0) {
			NCSCONTEXT lcl_mem_ptr = NULL;

			lcl_mem_ptr = malloc(l_size * (*ptr_data_len));
			if (lcl_mem_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			memset(lcl_mem_ptr, '\0', l_size * (*ptr_data_len));
			(*(long **)((long)ptr + (long)rule->fld5)) = lcl_mem_ptr;
		}

		newptr = (NCSCONTEXT)(*(long **)((long)ptr + (long)rule->fld5));
		if (is_builtin_edp) {
			if (buf_env->is_ubaid) {
				/* Enc/dec optimization only for uint8_t or char var-len-data */
				if ((l_size != 0) && ((rule->fld1 == ncs_edp_char) || (rule->fld1 == ncs_edp_uns8))) {
					uint32_t lcl_cnt = 0;

					/* For uint8_t*, or char*, compute effective
					   bytes to encode in a single go. */
					lcl_cnt = (l_size * (*ptr_data_len));

					if (lcl_cnt != 0) {
						if (ncs_decode_n_octets_from_uba(buf_env->info.uba,
										 (uint8_t *)newptr,
										 lcl_cnt) != NCSCC_RC_SUCCESS) {
							*o_err = EDU_ERR_MEM_FAIL;
							return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
						}
						if (rc != NCSCC_RC_SUCCESS)
							return rc;
					}
				} else if (rule->fld1 == ncs_edp_string) {
					/* This is ncs_edp_string, can't be used with var-len-data attribute. */
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				} else {
					uint32_t loop_cnt = *ptr_data_len;

					for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
						rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
								       &newptr, ptr_data_len, buf_env,
								       EDP_OP_TYPE_DEC, o_err);
						if (rc != NCSCC_RC_SUCCESS)
							return rc;
						newptr = (NCSCONTEXT)((long)newptr + (long)l_size);
					}
				}
			} else {
				/* "*ptr_data_len" has instance-count, so we are done 
				   here. */
				if (*ptr_data_len != 0) {
					rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
							       &newptr, ptr_data_len, buf_env, EDP_OP_TYPE_DEC, o_err);
					if (rc != NCSCC_RC_SUCCESS)
						return rc;
				}
			}
		} else {
			uint32_t loop_cnt = *ptr_data_len;

			for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
				rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
						       &newptr, ptr_data_len, buf_env, EDP_OP_TYPE_DEC, o_err);
				if (rc != NCSCC_RC_SUCCESS)
					return rc;
				newptr = (NCSCONTEXT)((long)newptr + (long)l_size);
			}
		}
	} /* if((rule->fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) */
	else if (rule->fld1 == ncs_edp_string) {
		newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);
		rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
				       (NCSCONTEXT)newptr, ptr_data_len, buf_env, EDP_OP_TYPE_DEC, o_err);
		if (rc != NCSCC_RC_SUCCESS)
			return rc;
	} else if ((rule->fld2 & EDQ_ARRAY) == EDQ_ARRAY) {
		uint32_t loop_cnt = rule->fld6;	/* Array size */

		if (rule->fld1 == ncs_edp_char) {
			uint16_t u16 = 0;
			uint8_t *p8 = NULL;

			/* Decode in the form of character string, rather than an array of
			   characters. */
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			loop_cnt = u16;
		}
		newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);

		for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, &newptr,
					       &lcl_cnt, buf_env, EDP_OP_TYPE_DEC, o_err);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
			if (!ncs_edu_return_builtin_edp_size(rule->fld1, &l_size)) {
				/* This is not builtin EDP */
				if ((lcl_hdl_node = (EDU_HDL_NODE *)
				     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&rule->fld1)) == NULL) {
					*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
				l_size = lcl_hdl_node->size;
			}
			newptr = (NCSCONTEXT)((long)newptr + (long)l_size);

		}
	} /* if((rule->fld2 & EDQ_ARRAY) == EDQ_ARRAY) */
	else {
		newptr = (NCSCONTEXT)((long)ptr + (long)rule->fld5);
		rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
				       (NCSCONTEXT)&newptr, &lcl_cnt, buf_env, EDP_OP_TYPE_DEC, o_err);
		if (rc != NCSCC_RC_SUCCESS)
			return rc;
		if ((rule->fld2 & EDQ_POINTER) == EDQ_POINTER) {
			if (*(uint8_t **)((long)ptr + (long)rule->fld5) == NULL)
			{
				*(uint8_t **)((long)ptr + (long)rule->fld5) = newptr;
			}
		}
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_prfm_pp_on_non_ptr

  DESCRIPTION:      Utility function to execute "EDU_EXEC" instruction on 
                    non-pointer data-fields during pretty-print operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
#if (NCS_EDU_VERBOSE_PRINT == 1)
uint32_t ncs_edu_prfm_pp_on_non_ptr(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 EDU_HDL_NODE *hdl_node,
				 EDU_INST_SET *rule, NCSCONTEXT ptr,
				 uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err)
{
	uint32_t *p_data_len = ptr_data_len;
	NCSCONTEXT lclptr = ptr;
	bool lcl_is_tested = false;
	EDU_PPDB_NODE_INFO *ppdb_node = NULL;
	EDU_PROG_HANDLER lcl_edp;
	uint32_t lcl_offset = 0, lcl_cnt = 0, lcnt = 0;
	uint32_t refcount = 0, rc = NCSCC_RC_SUCCESS;
	uint32_t l_size = 0;

	refcount = m_NCS_EDU_GET_REFCOUNT_OF_TESTABLE_FIELD(hdl_node->test_instr_store, rule);
	if (refcount != 0) {
		lcl_is_tested = true;

		lcl_edp = rule->fld1;
		lcl_offset = rule->fld5;
		if ((ppdb_node = edu_ppdb_node_findadd(&edu_tkn->ppdb,
						       hdl_node->edp, lcl_edp, lcl_offset, true)) == NULL) {
			/* Malloc failed. */
			*o_err = EDU_ERR_MEM_FAIL;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		lclptr = &ppdb_node->data_ptr;
		p_data_len = &ppdb_node->data_size;
	}
	if ((rule->fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
		bool is_builtin_edp = true;

		lcl_edp = rule->fld3;
		lcl_offset = rule->fld6;
		if ((ppdb_node = edu_ppdb_node_findadd(&edu_tkn->ppdb, hdl_node->edp,
						       lcl_edp, lcl_offset, false)) == NULL) {
			/* Malloc failed. */
			*o_err = EDU_ERR_MEM_FAIL;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* The length of variable-sized-data is passed in the 
		   offset "fld6" of the data structure(whose's EDP is defined in
		   "fld3"). */
		rc = ncs_edu_get_size_of_var_len_data(rule->fld3, ppdb_node->data_ptr, &lcl_cnt, o_err);
		if (rc != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(rc);
		}
		p_data_len = &lcl_cnt;

		/* In "fld6", the length of variable-sized-data is passed, which is now
		   available in "*p_data_len" by the functions invoked above 
		   "ncs_edu_get_size_of_var_len_data" after reading the value from PPDB database. */
		if (is_builtin_edp) {
			if (buf_env->is_ubaid) {
				/* Enc/dec optimization only for uint8_t or char var-len-data */
				if ((l_size != 0) && ((rule->fld1 == ncs_edp_char) || (rule->fld1 == ncs_edp_uns8))) {
					uint32_t lcl_cnt = 0;

					/* For uns*, or uns16*, or int*, etc, compute effective
					   bytes to encode in a single go. */
					lcl_cnt = (l_size * (*ptr_data_len));

					if (ncs_decode_n_octets_from_uba(buf_env->info.uba,
									 (uint8_t *)lclptr, lcl_cnt) != NCSCC_RC_SUCCESS) {
						*o_err = EDU_ERR_MEM_FAIL;
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}

					/* Write code to pretty-print this data. TBD. */
					;

					if (rc != NCSCC_RC_SUCCESS)
						return rc;
				} else if (rule->fld1 == ncs_edp_string) {
					/* This is ncs_edp_string. */
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				} else {
					uint32_t loop_cnt = *p_data_len;

					for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
						if (refcount != 0)
							rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
									       lclptr, p_data_len, buf_env,
									       EDP_OP_TYPE_PP, o_err);
						else
							rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
									       NULL, p_data_len, buf_env,
									       EDP_OP_TYPE_PP, o_err);
					}
				}
			} else {
				/* "*p_data_len" has instance-count, so we are done 
				   here. */
				if (*ptr_data_len != 0) {
					if (refcount != 0)
						rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
								       lclptr, p_data_len, buf_env,
								       EDP_OP_TYPE_PP, o_err);
					else
						rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
								       NULL, p_data_len, buf_env,
								       EDP_OP_TYPE_PP, o_err);
					if (rc != NCSCC_RC_SUCCESS)
						return rc;
				}
			}
		} else {
			uint32_t loop_cnt = *p_data_len;

			for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
				if (refcount != 0)
					rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
							       lclptr, p_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
				else
					rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
							       NULL, p_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
				if (rc != NCSCC_RC_SUCCESS)
					return rc;
			}
		}
	} /* if((rule->fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) */
	else if ((rule->fld2 & EDQ_ARRAY) == EDQ_ARRAY) {
		uint32_t loop_cnt = rule->fld6;	/* Array size */

		if (rule->fld1 == ncs_edp_char) {
			uint16_t u16 = 0;
			uint8_t *p8 = NULL;

			/* Decode in the form of character string, rather than an array of
			   characters. */
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			loop_cnt = u16;
		}

		for (lcnt = 0; lcnt < loop_cnt; lcnt++) {
			if (refcount != 0) {
				rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1,
						       lclptr, ptr_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
			} else {
				/* Pass NULL pointer in "arg" to run_edp. */
				rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, NULL,
						       ptr_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
			}
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}
	} /* if((rule->fld2 & EDQ_ARRAY) == EDQ_ARRAY) */
	else {
		if (refcount != 0) {
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, lclptr,
					       p_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
		} else {
			/* Pass NULL pointer in "arg" to run_edp. */
			rc = m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, rule->fld1, NULL,
					       p_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
		}
		if (rc != NCSCC_RC_SUCCESS)
			return rc;
	}

	if (lcl_is_tested) {
		*ptr_data_len = *p_data_len;
	}

	return NCSCC_RC_SUCCESS;
}
#endif

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_test_condition

  DESCRIPTION:      Utility function to invoke user-provided "test" function,
                    along with passing appropriate input parameters to the 
                    "test" function.

  RETURNS:          char *, denoting next operation to perform.

*****************************************************************************/
int ncs_edu_run_test_condition(EDU_HDL *edu_hdl,
			       EDU_INST_SET *rule, NCSCONTEXT ptr,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	int rc_lbl = EDU_EXIT;	/* default label to return. */
	long lclval = 0;
	NCSCONTEXT lptr = NULL;

	if (rule->fld7 == NULL) {
		/* Error!!!! Rule doesn't have "test" function pointer. */
#if (NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "test_condition function returning...fld7 = NULL...\n");
		ncs_edu_log_msg(gl_log_string);
#endif
		return rc_lbl;
	}
	/* This section of code uses the "test-function" passed by the application. */
	if ((optype == EDP_OP_TYPE_ENC) || (optype == EDP_OP_TYPE_DEC)) {
		if (rule->fld1 == ncs_edp_string) {
			/* Pointer to pointer. */
			lclval = *(long *)((long)ptr + (long)rule->fld5);
		} else {
			lclval = (long)ptr + (long)rule->fld5;

		}
	}
#if (NCS_EDU_VERBOSE_PRINT == 1)
	else if (optype == EDP_OP_TYPE_PP) {
		/* "ptr" contains value to be compared with. */
		lclval = (long)ptr;

	}
#endif

	lptr = (NCSCONTEXT)lclval;
	rc_lbl = rule->fld7(lptr);

	return rc_lbl;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_version_usr

  DESCRIPTION:      Utility function to invoke user-provided "chk_ver_usr" function,
                    along with passing appropriate input parameters to the 
                    "chk_ver_usr" function.

  RETURNS:          int ,returns skip count if Success.
                         (Skip count is an offset of the next-instruction to be executed
                         (relative from excluding EDU_VER_USR instruction)).
 
                         returns EDU_EXIT in case of Failure.

*****************************************************************************/
int ncs_edu_run_version_usr(EDU_HDL *edu_hdl,
			    EDU_INST_SET *rule, NCSCONTEXT ptr,
			    EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	int rc_lbl = EDU_EXIT;	/* default label to return. */

	if (rule->fld7 == NULL) {
		/* Error!!!! Rule doesn't have "test" function pointer. */
#if (NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "run_version_usr function returning...fld7 = NULL...\n");
		ncs_edu_log_msg(gl_log_string);
#endif
		return rc_lbl;
	}
	/* This section of code uses the "usr_version_check-function" passed by the application. */

	rc_lbl = rule->fld7((void *)&edu_hdl->to_version);

	return rc_lbl;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_version_ge

  DESCRIPTION:      Utility function to invoke edu_chk_ver_ge ,
                    along with passing appropriate input parameters to the 
                    "edu_chk_ver_ge" function.

  RETURNS:          int ,returns skip count if Success.
                         (Skip count is an offset of the next-instruction to be executed
                         (relative from excluding EDU_VER_GE instruction)).
                         returns EDU_EXIT in case of Failure.  

*****************************************************************************/
int ncs_edu_run_version_ge(EDU_HDL *edu_hdl,
			   EDU_INST_SET *rule, NCSCONTEXT ptr, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	int rc_lbl = EDU_EXIT;	/* default label to return. */

	if (rule->fld7 == NULL) {
		/* Error!!!! Rule doesn't have "test" function pointer. */
#if (NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "run_version_ge function returning...fld7 = NULL...\n");
		ncs_edu_log_msg(gl_log_string);
#endif
		return rc_lbl;
	}

	rc_lbl = edu_chk_ver_ge(((uint16_t *)rule->fld7), &edu_hdl->to_version, rule->nxt_lbl);

	return rc_lbl;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_test_ll_rule

  DESCRIPTION:      Utility function to test "next" pointer. If "next" pointer
                    is non-NULL, return EDU_SAME. Else, return EDU_NEXT.

  RETURNS:          uint32_t, denoting the EDU return values.

*****************************************************************************/
EDU_LABEL ncs_edu_run_test_ll_rule(EDU_INST_SET *rule, NCSCONTEXT ptr, EDP_OP_TYPE optype, EDU_ERR *o_err)
{
	long lclval = 0;

	switch (optype) {
	case EDP_OP_TYPE_DEC:
#if (NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
#endif
		/* While decoding/Prettyprinting, return EDU_NEXT. */
		return EDU_NEXT;

	case EDP_OP_TYPE_ENC:
		lclval = *((long *)((long)ptr + (long)rule->fld5));
		if (lclval == 0) {
			*o_err = EDU_NORMAL;
			return EDU_EXIT;
		}
		/* While encoding, return EDU_SAME so that the main program can call
		   this program again. */
		return EDU_SAME;

	default:
		break;
	}

	/* Land here, only in case of invalid Operation types */
	m_LEAP_DBG_SINK_VOID;
	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_rules_for_enc

  DESCRIPTION:      EDU internal function to execute-EDP-rules during encode
                    operation.

  RETURNS:          uint32_t, denoting the EDU return values.

*****************************************************************************/
EDU_LABEL ncs_edu_run_rules_for_enc(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count)
{
	int cur_inst_indx = 0;
	uint32_t dtype_attrb = 0;
	uint16_t cnt = 0, ptr_cnt = 0;
	NCS_EDU_ADMIN_OP_INFO admin_op;
	int rc_lbl = EDU_NEXT;
	uint8_t *encoded_cnt_loc = NULL, *encoded_ptr_cnt_loc = NULL;
	NCSCONTEXT lclptr = ptr;
	EDU_HDL_NODE *lcl_hdl_node = NULL;
	bool is_select_on = false;
	uint8_t select_index = 0;

	if ((prog[0].fld2 & EDQ_LNKLIST) == EDQ_LNKLIST) {
		/* This is a linked list. So, increment the counter for the 
		   first time itself. */
		if (buf_env->is_ubaid) {
			encoded_cnt_loc = ncs_enc_reserve_space(buf_env->info.uba, 2);
			if (!encoded_cnt_loc) {
				m_LEAP_DBG_SINK_VOID;
				*o_err = EDU_ERR_MEM_FAIL;
				return EDU_FAIL;
			}
			ncs_enc_claim_space(buf_env->info.uba, 2);
		} else {
			encoded_cnt_loc = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
		}
		cnt++;
	}

	if ((edu_tkn != NULL) && (edu_tkn->var_cnt != 0)) {
		if ((edu_tkn->parent_edp == prog[0].fld1) && (edu_tkn->var_array != NULL)) {
			/* Execute selectively only for this parent_edp */
			is_select_on = true;
		}
	}

	cur_inst_indx = 1;	/* 0th rule is always for EDU_START, which need not be executed now. */
	while ((cur_inst_indx != instr_count) && (prog[cur_inst_indx].instr != EDU_END)) {
		/* New changes. */
		if ((prog[cur_inst_indx].instr == EDU_EXEC) &&
		    ((prog[cur_inst_indx].fld2 & EDQ_POINTER) == EDQ_POINTER)) {
			/* For all pointers in a structure, we would have a "count"
			   field of "uint32_t" size to denote whether the pointer is
			   NULL or not. */
			if ((lcl_hdl_node = (EDU_HDL_NODE *)
			     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&prog[cur_inst_indx].fld1)) != NULL) {
				dtype_attrb = lcl_hdl_node->attrb;
			} else {
				/* Not there in EDU_HDL. */
				memset(&admin_op, '\0', sizeof(admin_op));
				admin_op.adm_op_type = NCS_EDU_ADMIN_OP_TYPE_GET_ATTRB;
				admin_op.info.get_attrb.o_attrb = &dtype_attrb;
				prog[cur_inst_indx].fld1(edu_hdl, NULL, (NCSCONTEXT)&admin_op, NULL,
							 NULL, EDP_OP_TYPE_ADMIN, o_err);
			}

			if (*(long *)((long)lclptr + (long)prog[cur_inst_indx].fld5) == 0) {
				/* If the pointer is NULL, count has to ZERO. */
				ptr_cnt = 0;
			} else {
				/* If the pointer is non-NULL, the count has to be 
				   ONE. */
				ptr_cnt = 1;
			}
			/* Now that dtype_attrb got filled, compare it with expected
			   values. */
			if (ptr_cnt == 0) {
				/* Encode "CNT" into buffer. */
				if (buf_env->is_ubaid) {
					encoded_ptr_cnt_loc = ncs_enc_reserve_space(buf_env->info.uba, 2);
					if (!encoded_ptr_cnt_loc) {
						m_LEAP_DBG_SINK_VOID;
						*o_err = EDU_ERR_MEM_FAIL;
						return EDU_FAIL;
					}
					ncs_enc_claim_space(buf_env->info.uba, 2);
					ncs_encode_16bit(&encoded_ptr_cnt_loc, ptr_cnt);
				} else {
					encoded_ptr_cnt_loc = buf_env->info.tlv_env.cur_bufp;
					ncs_encode_tlv_16bit(&encoded_ptr_cnt_loc, ptr_cnt);
					ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
				}
				/* Go to the next instruction. */
				if ((prog[cur_inst_indx].nxt_lbl == 0) || (prog[cur_inst_indx].nxt_lbl == EDU_NEXT)) {
					/* This is the EDU_NEXT statement. */
					++cur_inst_indx;
					continue;
				} else if (prog[cur_inst_indx].nxt_lbl == EDU_EXIT) {
					return EDU_EXIT;
				} else {
					/* Lookup the label, and switch to that instruction. */
					if (prog[cur_inst_indx].instr == EDU_EXEC) {
						cur_inst_indx = prog[cur_inst_indx].nxt_lbl;
#if (NCS_EDU_VERBOSE_PRINT == 1)
						memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
						sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
						ncs_edu_log_msg(gl_log_string);
#endif
					} else {
						++cur_inst_indx;
					}
					continue;
				}
			} /* if(ptr_cnt == 0) */
			else {
				if ((dtype_attrb & EDQ_LNKLIST) != EDQ_LNKLIST) {
					if (buf_env->is_ubaid) {
						encoded_ptr_cnt_loc = ncs_enc_reserve_space(buf_env->info.uba, 2);
						if (!encoded_ptr_cnt_loc) {
							m_LEAP_DBG_SINK_VOID;
							*o_err = EDU_ERR_MEM_FAIL;
							return EDU_FAIL;
						}
						ncs_enc_claim_space(buf_env->info.uba, 2);
						ncs_encode_16bit(&encoded_ptr_cnt_loc, ptr_cnt);
					} else {
						encoded_ptr_cnt_loc = buf_env->info.tlv_env.cur_bufp;
						ncs_encode_tlv_16bit(&encoded_ptr_cnt_loc, ptr_cnt);
						ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
					}
				}
			}
		}
		if (is_select_on) {
			if (select_index <= (edu_tkn->var_cnt - 1)) {
				cur_inst_indx = edu_tkn->var_array[select_index];
				if (cur_inst_indx >= instr_count) {
					/* This is a very fatal error. */
					m_LEAP_DBG_SINK_VOID;
					*o_err = EDU_ERR_SELECTIVE_EXECUTE_OP_FAIL;
					return EDU_FAIL;
				}
				++select_index;
			} else {
				/* The Select-rules are done. Return from this EDP. */
				return EDU_EXIT;
			}
		}
		rc_lbl = m_NCS_EDU_EXEC_RULE(edu_hdl, NULL, hdl_node, &prog[cur_inst_indx],
					     lclptr, ptr_data_len, buf_env, EDP_OP_TYPE_ENC, o_err);
		if ((rc_lbl == 0) || (rc_lbl == EDU_NEXT)) {
			/* This is typically EDU_NEXT statement. */
			if (is_select_on) {
				continue;	/* The new "cur_inst_indx" value wil be retrieved from "select_index" 
						   in the next iteration. */
				/* Note that, the EDU_TEST instruction is not honoured in this "select" operation */
			}

			if ((prog[cur_inst_indx].instr == EDU_VER_GE) || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				++cur_inst_indx;
				continue;
			} else if ((prog[cur_inst_indx].fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
				++cur_inst_indx;	/* To skip the EDU_EXEC_EXT instruction */
			}
			if ((prog[cur_inst_indx].nxt_lbl == 0) || (prog[cur_inst_indx].nxt_lbl == EDU_NEXT)) {
				++cur_inst_indx;
				continue;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_EXIT) {
				return EDU_EXIT;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_SAME) {
				/* This is invalid label, that was given in the EDP rules. This 
				   should NEVER be defined in the EDP rules. Note that, this check
				   would be cleaned-out in the EDCOMPILE operation. */
				return EDU_FAIL;
			} else {	/* if(prog[cur_inst_indx].nxt_lbl != 0) */

				if (prog[cur_inst_indx].instr == EDU_EXEC) {
					cur_inst_indx = prog[cur_inst_indx].nxt_lbl;
#if (NCS_EDU_VERBOSE_PRINT == 1)
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
					ncs_edu_log_msg(gl_log_string);
#endif
				} else {
					++cur_inst_indx;
				}
			}
			continue;
		} else if (rc_lbl == EDU_EXIT) {
			/* Exiting normally. */
			if (encoded_cnt_loc != NULL) {
				if (buf_env->is_ubaid) {
					ncs_encode_16bit(&encoded_cnt_loc, cnt);
				} else {
					ncs_encode_tlv_16bit(&encoded_cnt_loc, cnt);
				}
			}
			return EDU_EXIT;
		} else if (rc_lbl == EDU_SAME) {
			long lclval = 0;

			/* Re-run the same rules, with new offset value */
			cnt++;	/* Increment counter. */

			lclval = *(long *)((long)lclptr + (long)prog[cur_inst_indx].fld5);

			lclptr = (NCSCONTEXT)lclval;
			cur_inst_indx = 0;
			continue;
		} else if (rc_lbl == EDU_FAIL) {
			return EDU_FAIL;
		} else {
			/* Some unknown return value. Continue as if 
			   it is EDU_NEXT */
			if ((prog[cur_inst_indx].instr == EDU_TEST) || (prog[cur_inst_indx].instr == EDU_VER_GE)
			    || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				EDU_INST_TYPE temp_instr = prog[cur_inst_indx].instr;

				/* rc_lbl would be the relative-offset of the EDU instruction 
				   to JUMP-TO, after the test instruction was executed. */

				cur_inst_indx = cur_inst_indx + rc_lbl;
				if (cur_inst_indx >= instr_count) {
					m_LEAP_DBG_SINK_VOID;
					switch (temp_instr) {
					case EDU_TEST:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;
						break;
					case EDU_VER_GE:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_GE;
						break;
					case EDU_VER_USR:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_USR;
						break;
					default:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;
					}
					return EDU_FAIL;
				}
			} else {
				cur_inst_indx = rc_lbl;
			}

#if (NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
			ncs_edu_log_msg(gl_log_string);
#endif
			continue;
		}
	}			/* while(prog[cur_inst_indx].instr != EDU_END) */

	return EDU_EXIT;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_rules_for_dec

  DESCRIPTION:      EDU internal function to execute-EDP-rules during decode
                    operation.

  RETURNS:          uint32_t, denoting the EDU return values.

*****************************************************************************/
EDU_LABEL ncs_edu_run_rules_for_dec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count)
{
	int cur_inst_indx = 0;
	int rc_lbl = EDU_NEXT;
	NCSCONTEXT lclptr = ptr;
	bool is_select_on = false;
	uint8_t select_index = 0;

	if ((edu_tkn != NULL) && (edu_tkn->var_cnt != 0)) {
		if ((edu_tkn->parent_edp == prog[0].fld1) && (edu_tkn->var_array != NULL)) {
			/* Execute selectively only for this parent_edp */
			is_select_on = true;
		}
	}

	cur_inst_indx = 1;	/* 0th rule is always for EDU_START, which need not be executed now. */
	while ((cur_inst_indx != instr_count) && (prog[cur_inst_indx].instr != EDU_END)) {
		if (is_select_on) {
			if (select_index <= (edu_tkn->var_cnt - 1)) {
				cur_inst_indx = edu_tkn->var_array[select_index];
				if (cur_inst_indx >= instr_count) {
					/* This is a very fatal error. */
					m_LEAP_DBG_SINK_VOID;
					*o_err = EDU_ERR_SELECTIVE_EXECUTE_OP_FAIL;
					return EDU_FAIL;
				}
				++select_index;
			} else {
				/* The Select-rules are done. Return from this EDP. */
				return EDU_EXIT;
			}
		}
		rc_lbl = m_NCS_EDU_EXEC_RULE(edu_hdl, NULL, hdl_node, &prog[cur_inst_indx],
					     lclptr, ptr_data_len, buf_env, EDP_OP_TYPE_DEC, o_err);
		if ((rc_lbl == 0) || (rc_lbl == EDU_NEXT)) {

			/* This is typically EDU_NEXT statement. */
			if (is_select_on) {
				continue;	/* The new "cur_inst_indx" value wil be retrieved from "select_index" 
						   in the next iteration. */
				/* Note that, the EDU_TEST instruction is not honoured in this "select" operation */
			}

			if ((prog[cur_inst_indx].instr == EDU_VER_GE) || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				++cur_inst_indx;
				continue;
			} else if ((prog[cur_inst_indx].fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
				++cur_inst_indx;	/* To skip the EDU_EXEC_EXT instruction */
			}
			if ((prog[cur_inst_indx].nxt_lbl == 0) || (prog[cur_inst_indx].nxt_lbl == EDU_NEXT)) {
				++cur_inst_indx;
				continue;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_EXIT) {
				return EDU_EXIT;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_SAME) {
				/* Wrong label switch. */
				;
			} else {	/* if(prog[cur_inst_indx].nxt_lbl != 0) */

				if (prog[cur_inst_indx].instr == EDU_EXEC) {
					cur_inst_indx = prog[cur_inst_indx].nxt_lbl;
#if (NCS_EDU_VERBOSE_PRINT == 1)
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
					ncs_edu_log_msg(gl_log_string);
#endif
				} else {
					++cur_inst_indx;
				}
				continue;
			}
		} else if (rc_lbl == EDU_EXIT) {
			/* Exiting normally. */
			return EDU_EXIT;
		} else if (rc_lbl == EDU_SAME) {
			/* This shouldn't come, as this is not returned by the
			   m_NCS_EDU_EXEC_RULE( ) macro. */
			return EDU_EXIT;
		} else if (rc_lbl == EDU_FAIL) {
			return EDU_FAIL;
		} else {
			/* Some unknown return value. Continue as if
			   it is EDU_NEXT */
			if ((prog[cur_inst_indx].instr == EDU_TEST) || (prog[cur_inst_indx].instr == EDU_VER_GE)
			    || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				EDU_INST_TYPE temp_instr = prog[cur_inst_indx].instr;

				/* rc_lbl would be the relative-offset of the EDU instruction
				   to JUMP-TO, after the test instruction was executed. */

				cur_inst_indx = cur_inst_indx + rc_lbl;
				if (cur_inst_indx >= instr_count) {
					m_LEAP_DBG_SINK_VOID;
					switch (temp_instr) {
					case EDU_TEST:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;
						break;
					case EDU_VER_GE:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_GE;
						break;
					case EDU_VER_USR:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_USR;
						break;
					default:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;
					}
					return EDU_FAIL;
				}
			} else {
				cur_inst_indx = rc_lbl;
			}
#if (NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
			ncs_edu_log_msg(gl_log_string);
#endif
			continue;
		}
		++cur_inst_indx;
	}			/* while(prog[cur_inst_indx].instr != EDU_END) */
	return EDU_EXIT;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_rules_for_pp

  DESCRIPTION:      EDU internal function to execute-EDP-rules during 
                    pretty-print operation.

  RETURNS:          uint32_t, denoting the EDU return values.

*****************************************************************************/
#if(NCS_EDU_VERBOSE_PRINT == 1)
EDU_LABEL ncs_edu_run_rules_for_pp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   EDU_HDL_NODE *hdl_node,
				   EDU_INST_SET *prog, NCSCONTEXT ptr,
				   uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count)
{
	int cur_inst_indx = 0;
	int rc_lbl = EDU_NEXT;
	NCSCONTEXT lclptr = ptr;
	bool is_select_on = false;
	uint8_t select_index = 0;

	if (edu_tkn->var_cnt != 0) {
		if ((edu_tkn->parent_edp == hdl_node->edp) && (edu_tkn->var_array != NULL)) {
			/* Execute selectively only for this parent_edp */
			is_select_on = true;
		}
	}

	cur_inst_indx = 1;	/* 0th rule is always for EDU_START, which need not be executed now. */
	while ((cur_inst_indx != instr_count) && (prog[cur_inst_indx].instr != EDU_END)) {
		if (is_select_on) {
			if (select_index <= (edu_tkn->var_cnt - 1)) {
				cur_inst_indx = edu_tkn->var_array[select_index];
				if (cur_inst_indx >= instr_count) {
					/* This is a very fatal error. */
					m_LEAP_DBG_SINK_VOID;
					*o_err = EDU_ERR_SELECTIVE_EXECUTE_OP_FAIL;
					return EDU_FAIL;
				}
				++select_index;
			} else {
				/* The Select-rules are done. Return from this EDP. */
				return EDU_EXIT;
			}
		}
		rc_lbl = m_NCS_EDU_EXEC_RULE(edu_hdl, edu_tkn, hdl_node, &prog[cur_inst_indx],
					     lclptr, ptr_data_len, buf_env, EDP_OP_TYPE_PP, o_err);
		if ((rc_lbl == 0) || (rc_lbl == EDU_NEXT)) {

			/* This is typically EDU_NEXT statement. */
			if (is_select_on) {
				continue;	/* The new "cur_inst_indx" value wil be retrieved from "select_index" 
						   in the next iteration. */
				/* Note that, the EDU_TEST instruction is not honoured in this "select" operation */
			}

			if ((prog[cur_inst_indx].instr == EDU_VER_GE) || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				++cur_inst_indx;
				continue;
			} else if ((prog[cur_inst_indx].fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
				++cur_inst_indx;
			}
			if ((prog[cur_inst_indx].nxt_lbl == 0) || (prog[cur_inst_indx].nxt_lbl == EDU_NEXT)) {
				++cur_inst_indx;
				continue;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_EXIT) {
				return EDU_EXIT;
			} else if (prog[cur_inst_indx].nxt_lbl == EDU_SAME) {
				/* Wrong label switch. */
				;
			} else {	/* if(prog[cur_inst_indx].nxt_lbl != 0) */

				if (prog[cur_inst_indx].instr == EDU_EXEC) {
					cur_inst_indx = prog[cur_inst_indx].nxt_lbl;
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
					ncs_edu_log_msg(gl_log_string);
				} else {
					++cur_inst_indx;
				}
				continue;
			}
		} else if (rc_lbl == EDU_EXIT) {
			/* Exiting normally. */
			return EDU_EXIT;
		} else if (rc_lbl == EDU_SAME) {
			;	/* Nothing required here. */
		} else if (rc_lbl == EDU_FAIL) {
			return EDU_FAIL;
		} else {
			/* Some unknown return value. Continue as if
			   it is EDU_NEXT */
			if ((prog[cur_inst_indx].instr == EDU_TEST) || (prog[cur_inst_indx].instr == EDU_VER_GE)
			    || (prog[cur_inst_indx].instr == EDU_VER_USR)) {
				EDU_INST_TYPE temp_instr = prog[cur_inst_indx].instr;

				/* rc_lbl would be the relative-offset of the EDU instruction
				   to JUMP-TO, after the test instruction was executed. */

				cur_inst_indx = cur_inst_indx + rc_lbl;
				if (cur_inst_indx >= instr_count) {
					m_LEAP_DBG_SINK_VOID;
					switch (temp_instr) {
					case EDU_TEST:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;
						break;
					case EDU_VER_GE:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_GE;
						break;
					case EDU_VER_USR:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_USR;
						break;
					default:
						*o_err = EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC;

					}
					return EDU_FAIL;
				}
			} else {
				cur_inst_indx = rc_lbl;
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Switching to index = %d...\n", cur_inst_indx);
			ncs_edu_log_msg(gl_log_string);
			continue;
		}
		++cur_inst_indx;
	}			/* while(prog[cur_inst_indx].instr != EDU_END) */
	return EDU_EXIT;
}
#endif

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_rules_for_compile

  DESCRIPTION:      EDU internal function to execute-EDP-rules during compile
                    operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_run_rules_for_compile(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node,
				    EDU_INST_SET *prog, NCSCONTEXT ptr,
				    uint32_t *ptr_data_len, EDU_ERR *o_err, int instr_count)
{
	uint32_t l_size = 0;
	int i = 0;
	bool start_fnd = false, end_fnd = false, testll_fnd = false;
	bool ptr_set = false, ll_set = false, arr_set = false, var_len_set = false;
	EDU_HDL_NODE *lcl_hdl_node = NULL;

	/*
	 *  List of validations performed during EDCOMPILE operation :
	 *  Step 1: The EDP should have attributes(EDQ_*) set in rule[0](default 
	 *          value is 0).
	 *  Step 2: All the labels in the rules should be unique, and valid. 
	 *  Step 3: Each of the rule should have valid and required data, according
	 *          to the specific semantics of the instruction.
	 *  Step 4: All offset values in the rules should be valid, and within
	 *          the limits of the data-structure-size(if other than ZERO).
	 *  Step 5: Self-referencing, i.e., EDU_EXEC on self-EDP should not
	 *          be present.
	 *  Step 6: If any EDP referenced in the rules is not present in the 
	 *          EDU_HDL, or LEAP-EDPs, log/print a message, and continue the 
	 *          edcompile operation.
	 */

	hdl_node->edcompile_pass = false;

	if (instr_count <= 1) {
		/* Invalid number of EDU instructions. */
		*o_err = EDU_ERR_INV_NUMBER_OF_EDU_INSTRUCTIONS;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (prog[0].instr != EDU_START) {
		*o_err = EDU_ERR_EDU_START_NOT_FIRST_INSTR;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (prog[0].fld1 != hdl_node->edp) {
		/* Function handlers not matching. This is a very fatal error. */
		*o_err = EDU_ERR_EDP_NOT_MATCHING_IN_EDU_START_INSTR;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (prog[instr_count - 1].instr != EDU_END) {
		/* Last EDU instruction is not EDU_END. */
		*o_err = EDU_ERR_EDU_END_NOT_LAST_INSTR;
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Copy the size of this data structure into the "hdl_node->size" */
	hdl_node->size = prog[0].fld5;

	/* Copy "attributes" into hdl_node */
	hdl_node->attrb = prog[0].fld2;

	/* Look for duplicate "EDU_START", "EDU_END" and "EDU_TEST_LL_PTR" 
	   instructions.  Also, while looking for this instructions, populate the
	   "labels" list. */
	for (i = 0; ((i != instr_count) && (prog[i].instr != EDU_END)); i++) {
		if (prog[i].instr >= EDU_MAX) {
			/* Wrong Instruction given. */
			*o_err = EDU_ERR_ILLEGAL_INSTR_GIVEN;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		/* Check for illegal/duplicate labels.
		 *      (prog[i].nxt_lbl == 0) is GO-NEXT label.(Synonym 
		 *       for EDU_NEXT) 
		 */
		if ((prog[i].nxt_lbl != 0) && (prog[i].nxt_lbl >= instr_count)) {
			if (!((prog[i].nxt_lbl == EDU_NEXT) || (prog[i].nxt_lbl == EDU_EXIT))) {
				/* This is critical error, since this "jump" statement
				   would land outside prog[ ] boundary. */
				*o_err = EDU_ERR_ILLEGAL_NEXT_LABEL_VALUE;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		if (prog[i].instr == EDU_START) {
			if (start_fnd) {
				/* Error condition. Duplicate EDU_START found. */
				/* Log the particular error, and return. */
				*o_err = EDU_ERR_DUPLICATE_EDU_START_INSTR_FOUND;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			} else {
				start_fnd = true;
				/* In EDU_START instruction, only EDQ_LNKLIST can be 
				   allowed. */
				ll_set = false;
				if ((prog[i].fld2 & EDQ_LNKLIST) == EDQ_LNKLIST) {
					ll_set = true;
				}
				if (!(ll_set || (prog[i].fld2 == 0))) {
					/* INVALID ATTRIBUTE COMBINATION in EDU_START
					   instruction. */
					*o_err = EDU_ERR_INV_ATTRIBUTE_COMBINATION_IN_START_INSTR;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
		} else if (prog[i].instr == EDU_END) {
			if (end_fnd) {
				/* Error condition. Duplicate EDU_END found. */
				*o_err = EDU_ERR_DUPLICATE_EDU_END_INSTR_FOUND;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			} else {
				end_fnd = true;
			}
		} else if (prog[i].instr == EDU_TEST_LL_PTR) {
			if (prog[i].fld1 != prog[0].fld1) {
				/* Wrong EDP value given as argument. */
				*o_err = EDU_ERR_INV_EDP_VALUE;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			if ((prog[0].fld2 & EDQ_LNKLIST) == EDQ_LNKLIST) {
				/* For linked-list EDP, the instruction "EDU_TEST_LL_PTR" should
				   be present, with valid parameters. */
				if (testll_fnd) {
					/* Error condition. Duplicate EDU_TEST_LL_PTR found. */
					*o_err = EDU_ERR_DUPLICATE_EDU_TEST_LL_PTR_INSTR_FOUND;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				} else {
					testll_fnd = true;
					if ((prog[i].fld5 == 0) || ((prog[i].fld5 + sizeof(uint32_t *)) > hdl_node->size)) {
						/* The offset for the "next" pointer in the linked
						   list structure is invalid. */
						*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}
				}
			} else {
				/* Invalid EDU instruction given. */
				*o_err = EDU_ERR_INV_ATTRIBUTE_FOR_LINKED_LIST_EDP;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}
	}

	for (i = 0; ((i != instr_count) && (prog[i].instr != EDU_END)); i++) {
		if (prog[i].instr == EDU_START) {
			/* EDU_START validation already done above. */
			;
		} else if (prog[i].instr == EDU_END) {
			/* EDU_END validation already done above. */
			;
		} else if (prog[i].instr == EDU_EXEC_EXT) {
			/* Validation done in EDU_EXEC itself. */
			;
		} else if (prog[i].instr == EDU_EXEC) {
			if (prog[i].fld1 == hdl_node->edp) {
				/* This is self-referencing. Log/print error, and return. */
				*o_err = EDU_ERR_EDP_REFERENCES_SELF;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}

			/* Look if this EDP is builtin */
			if (!ncs_edu_is_edp_builtin(prog[i].fld1)) {
				/* Run EDCOMPILE on this EDP also. */
				if (m_NCS_EDU_COMPILE_EDP(edu_hdl, prog[i].fld1, o_err)
				    != NCSCC_RC_SUCCESS) {
					/* Error already set in "o_err" */
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}

			ptr_set = ll_set = arr_set = var_len_set = false;
			if ((prog[i].fld2 & EDQ_POINTER) == EDQ_POINTER) {
				ptr_set = true;
			}
			if ((prog[i].fld2 & EDQ_LNKLIST) == EDQ_LNKLIST) {
				ll_set = true;
			}
			if ((prog[i].fld2 & EDQ_ARRAY) == EDQ_ARRAY) {
				arr_set = true;
			}
			if ((prog[i].fld2 & EDQ_VAR_LEN_DATA) == EDQ_VAR_LEN_DATA) {
				var_len_set = true;
				if (prog[i + 1].instr != EDU_EXEC_EXT) {
					/* Error */
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
			/* INVALID ATTRIBUTE COMBINATION.
			   In this EDP, only one of the attributes have to be set.
			   Why ???
			   - If this is a linked list, a pointer attribute
			   CAN NEVER be defined in this EDP. It CAN HOWEVER be defined 
			   in the parent data structure's EDP rules.
			   - If this is a linked list, an array attribute CAN NEVER
			   be defined in this EDP. It CAN HOWEVER be defined in the 
			   parent data structure's EDP rules.
			   - If this is an array, a pointer attribute CAN NEVER be
			   defined in this EDP. It CAN HOWEVER be defined in the 
			   parent data structure's EDP rules.
			   - If this is an array, a linked-list attribute CAN NEVER
			   be defined in this EDP.
			   - If this is a pointer, a array attribute CAN NEVER be 
			   defined in this EDP. It CAN HOWEVER be defined in the 
			   parent data structure's EDP rules.
			   - If this is a pointer, a linked-list attribute CAN NEVER
			   be defined in this EDP.
			 */

			/* Now, verify attributes, and offset. */
			if (hdl_node->size != 0) {
				if (ll_set || (ptr_set && arr_set) ||
				    (ptr_set && var_len_set) || (arr_set && var_len_set)) {
					/* Invalid attribute set here. */
					*o_err = EDU_ERR_INV_ATTRIBUTE_FOR_EXEC_INSTR;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
				if (arr_set || ptr_set || var_len_set) {
					if (!ncs_edu_return_builtin_edp_size(prog[i].fld1, &l_size)) {
						/* This is not builtin EDP */
						if ((lcl_hdl_node = (EDU_HDL_NODE *)
						     ncs_patricia_tree_get(&edu_hdl->tree,
									   (uint8_t *)&prog[i].fld1)) == NULL) {
							*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
							return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
						}
						l_size = lcl_hdl_node->size;
					}
					if (l_size != 0) {
						if (arr_set) {
							if ((prog[i].fld5 + (prog[i].fld6 * l_size)) > hdl_node->size) {
								/* This is another failure case. */
								*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
								return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
							}
						}	/* if(arr_set) */
						if (ptr_set || var_len_set) {
							if ((prog[i].fld5 + sizeof(uint32_t *)) > hdl_node->size) {
								/* This is another failure case. */
								*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
								return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
							}
							if (var_len_set) {
								uint32_t t_size = 0;

								/* Check "fld3" and "fld6" also */
								if (!ncs_edu_return_builtin_edp_size(prog[i].fld3,
												     &t_size)) {
									/* This is not builtin EDP. Fatal Error!!! */
									*o_err =
									    EDU_ERR_VAR_LEN_PARAMETER_NOT_BASIC_EDP_TYPE;
									return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
								}
							}
						}	/* if(ptr_set) */
					}	/* if(l_size != 0) */
				} /* if(arr_set || ptr_set) */
				else {
					if (prog[i].fld5 >= hdl_node->size) {
						/* This is one of the many checks that can cause failure. */
						*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}

				}
			}	/* if(hdl_node->size != 0) */
			if ((prog[i].nxt_lbl == EDU_SAME) || (prog[i].nxt_lbl == EDU_FAIL)) {
				/* Wrong values given to EDP's labels */
				*o_err = EDU_ERR_ILLEGAL_NEXT_LABEL_VALUE;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		} else if (prog[i].instr == EDU_TEST) {
			if (prog[i].fld1 == NULL) {
				/* Wrong EDP value given as argument. */
				*o_err = EDU_ERR_INV_EDP_VALUE;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			if (prog[i].fld7 == NULL) {
				/* "test" function empty. Log/print error, and return. */
				*o_err = EDU_ERR_TEST_FUNC_NULL;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			/* Verify offset */
			if (hdl_node->size != 0) {
				if (prog[i].fld5 >= hdl_node->size) {
					/* This is one of the many checks that can cause failure. */
					*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
				if (!ncs_edu_return_builtin_edp_size(prog[i].fld1, &l_size)) {
					/* This is not builtin EDP */
					if ((lcl_hdl_node = (EDU_HDL_NODE *)
					     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&prog[i].fld1)) == NULL) {
						*o_err = EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME;
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}
					l_size = lcl_hdl_node->size;
				}
				if (l_size != 0) {
					if ((prog[i].fld5 + l_size) > hdl_node->size) {
						/* This is another failure case. */
						*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}
				}
			}	/* if(hdl_node->size != 0) */
		} else if (prog[i].instr == EDU_TEST_LL_PTR) {
			/* Lookup "prog[i].fld1" and get the size of the data structure.
			   Then, verify that size of the "next" pointer with the boundary
			   limits of the EDP data structure. 
			 */
			if (hdl_node->size != 0) {
				if ((prog[i].fld5 + sizeof(uint32_t *)) > hdl_node->size) {
					/* This exceeds the boundary of hdl_node */
					*o_err = EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE;
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
		} else if (prog[i].instr == EDU_VER_GE) {

			if (prog[i].fld7 == NULL) {
				*o_err = EDU_ERR_VER_GE_FIELD_NULL;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}

		} else if (prog[i].instr == EDU_VER_USR) {

			if (prog[i].fld7 == NULL) {
				*o_err = EDU_ERR_VER_USR_FIELD_NULL;
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}

		} else {
			/* Unknown instruction type. Log/print error, and return, */
			*o_err = EDU_ERR_ILLEGAL_INSTR_GIVEN;
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	}

	/* Generate EDP_TEST_INSTR_REC list in "hdl_node->test_instr_store". */
	if (ncs_edu_validate_and_gen_test_instr_rec_list(&hdl_node->test_instr_store,
							 &prog[0], instr_count, o_err) != NCSCC_RC_SUCCESS) {
		/* o_err already populated within this function. */
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	hdl_node->edcompile_pass = true;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_get_refcount_of_testable_field

  DESCRIPTION:      Gets the reference-count of the data-field, defined by
                    the EDU instruction, identified by "rule".

  RETURNS:          Number of references of this data-field(defined in the 
                    "rule" EDU instruction) in the entire EDP program.
                    Default return value is 0.

*****************************************************************************/
uint32_t ncs_edu_get_refcount_of_testable_field(EDP_TEST_INSTR_REC *inst_store, EDU_INST_SET *rule)
{
	EDP_TEST_INSTR_REC *lrec = inst_store;

	if ((inst_store == NULL) || (rule == NULL) || (rule->instr != EDU_EXEC))
		return 0;

	for (; (lrec != NULL); lrec = lrec->next) {
		if ((lrec->edp == rule->fld1) && (lrec->offset == rule->fld5)) {
			return lrec->refcount;
		}
	}

	return 0;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_run_edcompile_on_edp

  DESCRIPTION:      Function which invokes EDCOMPILE operation on EDP.

  RETURNS:          NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

*****************************************************************************/
uint32_t ncs_edu_run_edcompile_on_edp(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node, EDU_ERR *o_err)
{
	NCS_EDU_ADMIN_OP_INFO admin_op;
	EDU_TKN edu_tkn;

	memset(&admin_op, '\0', sizeof(admin_op));
	memset(&edu_tkn, '\0', sizeof(edu_tkn));

	edu_tkn.i_edp = hdl_node->edp;
	admin_op.adm_op_type = NCS_EDU_ADMIN_OP_TYPE_COMPILE;

	if (hdl_node->edp == NULL) {
		*o_err = EDU_ERR_EDP_NULL;
		return NCSCC_RC_FAILURE;
	}

	return hdl_node->edp(edu_hdl, &edu_tkn, (NCSCONTEXT)&admin_op, NULL, NULL, EDP_OP_TYPE_ADMIN, o_err);
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_validate_and_gen_test_instr_rec_list

  DESCRIPTION:      Validates the EDU instructions, and generates a list of
                    "test"able instructions of this EDP. This generated list 
                    is populated in "*head".

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_validate_and_gen_test_instr_rec_list(EDP_TEST_INSTR_REC **head,
						   EDU_INST_SET *rules_head, int instr_count, EDU_ERR *o_err)
{
	int i, j = 0;
	EDP_TEST_INSTR_REC *prvnode = NULL, *tmp = NULL;

	if ((head == NULL) || (rules_head == NULL)) {
		return NCSCC_RC_FAILURE;
	}
	prvnode = *head;
	while (prvnode != NULL) {
		prvnode = prvnode->next;
	}

	for (i = 0; ((i != instr_count) && (rules_head[i].instr != EDU_END)); i++) {
		if (rules_head[i].instr == EDU_TEST) {
			bool already_added = false, lclfnd = false;

			/* If already in the "*head" list, just increment the "refcount"
			   of that entry. */
			for (tmp = *head; (tmp != NULL); tmp = tmp->next) {
				if ((tmp->edp == rules_head[i].fld1) && (tmp->offset == rules_head[i].fld5)) {
					/* Found the entry. Increment the "refcount". */
					already_added = true;
					tmp->refcount++;
					break;
				}
			}
			if (!already_added) {
				/* Verify whether there exists an "EDU_EXEC" instruction
				   with offset matching "rules_head[i].fld5", and EDP value
				   matching "rules_head[i].fld1". If yes, then only this
				   "test" condition is valid. Else, this is a non-existent
				   field/edpid combination. */
				for (j = 0; ((j != instr_count) && (rules_head[j].instr != EDU_END)); j++) {
					if ((rules_head[i].fld5 == rules_head[j].fld5) &&
					    (rules_head[i].fld1 == rules_head[j].fld1)) {
						/* Matching field/edpid found. */
						lclfnd = true;
						break;
					}
				}
				if (!lclfnd) {
					*o_err = EDU_ERR_EXEC_INSTR_DOES_NOT_EXIST_FOR_OFFSET_OF_TEST_INSTR;
					return NCSCC_RC_FAILURE;
				}
				if ((tmp = m_MMGR_ALLOC_EDP_TEST_INSTR_REC) == NULL) {
					ncs_edu_free_test_instr_rec_list(*head);
					*head = NULL;
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(tmp, '\0', sizeof(EDP_TEST_INSTR_REC));
				tmp->edp = rules_head[i].fld1;
				tmp->offset = rules_head[i].fld5;
				tmp->refcount = 1;
				if (prvnode != NULL) {
					prvnode->next = tmp;
					prvnode = tmp;
				} else {
					/* Starting element in the list. */
					*head = prvnode = tmp;
				}
			}	/* if(!already_added) */
		} /* if(rules_head[i].instr == EDU_TEST) */
		else if (rules_head[i].instr == EDU_EXEC) {
			bool already_added = false, lcl_to_be_added = false;

			for (j = 0; (((j != instr_count) && rules_head[j].instr != EDU_END)); j++) {
				if (rules_head[j].instr == EDU_EXEC) {
					if (((rules_head[j].fld2 & EDQ_VAR_LEN_DATA) ==
					     EDQ_VAR_LEN_DATA) &&
					    (rules_head[j].fld3 == rules_head[i].fld1) &&
					    (rules_head[j].fld6 == rules_head[i].fld5)) {
						/* Count EDP matched. */
						lcl_to_be_added = true;
						break;
					}
				}
			}
			if (lcl_to_be_added) {
				/* Lookup "fld6" offset in the structure, and store the instruction
				   as a test-able field */
				for (tmp = *head; (tmp != NULL); tmp = tmp->next) {
					/* Lookup EDP from EDU_HDL, and see if size if 1/2/4 bytes.
					   TBD. */
					if ((tmp->edp == rules_head[i].fld1) && (tmp->offset == rules_head[i].fld6)) {
						/* Found the entry. Increment the "refcount". */
						already_added = true;
						tmp->refcount++;
						break;
					}
				}
				if (!already_added) {
					if ((tmp = m_MMGR_ALLOC_EDP_TEST_INSTR_REC) == NULL) {
						ncs_edu_free_test_instr_rec_list(*head);
						*head = NULL;
						*o_err = EDU_ERR_MEM_FAIL;
						return NCSCC_RC_FAILURE;
					}
					memset(tmp, '\0', sizeof(EDP_TEST_INSTR_REC));
					tmp->edp = rules_head[i].fld1;
					tmp->offset = rules_head[i].fld5;
					tmp->refcount = 1;
					if (prvnode != NULL) {
						prvnode->next = tmp;
						prvnode = tmp;
					} else {
						/* Starting element in the list. */
						*head = prvnode = tmp;
					}
				}	/* if(!already_added) */
			}
		}		/* variable-sized-data */
	}			/* for(i = 0; (rules_head[i].instr != EDU_END); i++) */

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_free_test_instr_rec_list

  DESCRIPTION:      The list of "test"able instructions are freed up.

  RETURNS:          void

*****************************************************************************/
void ncs_edu_free_test_instr_rec_list(EDP_TEST_INSTR_REC *head)
{
	EDP_TEST_INSTR_REC *tmp = NULL;

	while (head != NULL) {
		tmp = head->next;
		m_MMGR_FREE_EDP_TEST_INSTR_REC(head);
		head = tmp;
	}

	return;
}

#if(NCS_EDU_VERBOSE_PRINT == 1)
/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_ppdb_init

  DESCRIPTION:      Initialises the EDU_PPDB database(patricia tree, defined 
                    in EDU_HDL)

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t ncs_edu_ppdb_init(EDU_PPDB * ppdb)
{
	NCS_PATRICIA_PARAMS list_params;

	if (!ppdb->is_up) {
		/* Init the tree first */
		list_params.key_size = sizeof(EDU_PPDB_KEY);
		/*    list_params.info_size = 0;  */
		if ((ncs_patricia_tree_init(&ppdb->tree, &list_params))
		    != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* Init the PPDB */
		ppdb->is_up = true;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_ppdb_destroy

  DESCRIPTION:      Destroys the EDU_PPDB database(patricia tree, defined 
                    in EDU_HDL)

  RETURNS:          void

*****************************************************************************/
static void ncs_edu_ppdb_destroy(EDU_PPDB * ppdb)
{
	EDU_PPDB_KEY lclkey, *key = NULL;
	NCS_PATRICIA_NODE *pnode = NULL;

	if (ppdb->is_up) {
		for (;;) {
			if ((pnode = ncs_patricia_tree_getnext(&ppdb->tree, (const uint8_t *)key)) != 0) {
				/* Store the key for the next getnext call */
				edu_populate_ppdb_key(&lclkey,
						      ((EDU_PPDB_NODE_INFO *) pnode)->key.parent_edp,
						      ((EDU_PPDB_NODE_INFO *) pnode)->key.self_edp,
						      ((EDU_PPDB_NODE_INFO *) pnode)->key.field_offset);
				key = &lclkey;

				/* Detach and free the nh-node */
				edu_ppdb_node_del(ppdb, (EDU_PPDB_NODE_INFO *) pnode);
			} else
				break;
		}

		/* Nuke the tree */
		ncs_patricia_tree_destroy(&ppdb->tree);
		ppdb->is_up = false;
	}

	return;
}

/*****************************************************************************

  PROCEDURE NAME:   edu_populate_ppdb_key

  DESCRIPTION:      Populates key information of the EDU_PPDB node.

  RETURNS:          void

*****************************************************************************/
static void edu_populate_ppdb_key(EDU_PPDB_KEY * key,
				  EDU_PROG_HANDLER parent_edp, EDU_PROG_HANDLER self_edp, uint32_t offset)
{
	memset(key, '\0', sizeof(EDU_PPDB_KEY));
	key->parent_edp = parent_edp;
	key->self_edp = self_edp;
	key->field_offset = offset;
	return;
}

/*****************************************************************************

  PROCEDURE NAME:   edu_ppdb_node_findadd

  DESCRIPTION:      Adds a node entry to the EDU_PPDB(defined in EDU_HDL)

  RETURNS:          pointer to PPDB node of type EDU_PPDB_NODE_INFO .

*****************************************************************************/
static EDU_PPDB_NODE_INFO *edu_ppdb_node_findadd(EDU_PPDB * ppdb,
						 EDU_PROG_HANDLER parent_edp,
						 EDU_PROG_HANDLER self_edp, uint32_t offset, bool add)
{
	EDU_PPDB_NODE_INFO *node = NULL, *new_node = NULL;
	EDU_PPDB_KEY key;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (ppdb == (EDU_PPDB *) NULL)
		return NULL;

	edu_populate_ppdb_key(&key, parent_edp, self_edp, offset);

	/* Check for the presence of the node in the PPDB */
	if ((node = (EDU_PPDB_NODE_INFO *)
	     ncs_patricia_tree_get(&ppdb->tree, (uint8_t *)&key)) == NULL) {
		if (add) {
			/* This is a new one */
			if ((node = (new_node = m_MMGR_ALLOC_EDU_PPDB_NODE_INFO)) == NULL) {
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
			memset(node, 0, sizeof(EDU_PPDB_NODE_INFO));

			/* Init the node info */
			node->key = key;
			node->pat_node.key_info = (uint8_t *)&node->key;

			/* Add the node to the tree */
			if (ncs_patricia_tree_add(&ppdb->tree, &node->pat_node)
			    != NCSCC_RC_SUCCESS) {
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
		}
	}

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		/* If an new entry is created free it. No need of any other cleanup */
		if (new_node)
			m_MMGR_FREE_EDU_PPDB_NODE_INFO(new_node);
		return NULL;
	}
	return node;
}

/*****************************************************************************

  PROCEDURE NAME:   edu_ppdb_node_del

  DESCRIPTION:      Deletes node entry in the EDU_PPDB(defined in EDU_HDL)

  RETURNS:          void

*****************************************************************************/
static void edu_ppdb_node_del(EDU_PPDB * ppdb, EDU_PPDB_NODE_INFO * node)
{
	if (ppdb == (EDU_PPDB *) NULL)
		return;

	/* Detach it from tree */
	ncs_patricia_tree_del(&ppdb->tree, (NCS_PATRICIA_NODE *)&node->pat_node);

	/* Free the node contents now. */
	if (node->data_ptr != NULL) {
		{
			m_NCS_MEM_FREE(node->data_ptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
			node->data_size = 0;
		}
		node->data_ptr = NULL;
	}

	/* Free the node. */
	m_MMGR_FREE_EDU_PPDB_NODE_INFO(node);

	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_log_msg

  DESCRIPTION:      Utility function to log messages into a fixed log file.

  RETURNS:          void

*****************************************************************************/
void ncs_edu_log_msg(char *string)
{
	FILE *log_fh = NULL;

	if (string == NULL)
		return;

	log_fh = sysf_fopen("hj_edu_log.txt", "a+");
	if (log_fh == NULL)
		return;

	fprintf(log_fh, string);
	fclose(log_fh);

	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_perform_pp_op

  DESCRIPTION:      Utility function to perform Pretty-print of USRBUF/TLV-buffer
                    contents into the fixed log file.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_perform_pp_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
			    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err, uint8_t var_cnt, int *var_array)
{
	EDU_TKN lcl_edu_tkn;
	USRBUF *pp_ubuf = NULL;
	NCS_UBAID pp_ubaid;
	EDU_BUF_ENV lcl_buf_env;
	uint32_t lclrc = NCSCC_RC_SUCCESS;
	uint32_t lcl_cnt = 0;

	memset(&lcl_buf_env, '\0', sizeof(lcl_buf_env));
	memset(&pp_ubaid, '\0', sizeof(pp_ubaid));
	if (buf_env->is_ubaid) {
		if (buf_env->info.uba->start != NULL) {
			if ((pp_ubuf = m_MMGR_DITTO_BUFR(buf_env->info.uba->start)) == NULL) {
				/* Duplication of USRBUF failed. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		} else if (buf_env->info.uba->ub != NULL) {
			if ((pp_ubuf = m_MMGR_DITTO_BUFR(buf_env->info.uba->ub)) == NULL) {
				/* Duplication of USRBUF failed. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		} else {
			*o_err = EDU_ERR_UBAID_POINTER_NULL;
			return NCSCC_RC_FAILURE;
		}
		memset(&pp_ubaid, '\0', sizeof(pp_ubaid));
		pp_ubaid = *buf_env->info.uba;
		pp_ubaid.ub = pp_ubuf;
		lcl_buf_env.is_ubaid = true;
		lcl_buf_env.info.uba = &pp_ubaid;
		if ((pp_ubaid.max == 0) || (pp_ubaid.max == 0xcccccccc)) {
			ncs_dec_init_space(&pp_ubaid, pp_ubaid.ub);
		}
	} else {
		lcl_buf_env.info.tlv_env = buf_env->info.tlv_env;
	}

	memset(&lcl_edu_tkn, '\0', sizeof(lcl_edu_tkn));
	m_NCS_EDU_TKN_INIT(&lcl_edu_tkn);
	lcl_edu_tkn.i_edp = edp;
	lcl_edu_tkn.parent_edp = edp;	/* Used only during selective-encoding */
	if (var_cnt != 0) {
		/* Used only during selective-encoding */
		lcl_edu_tkn.var_cnt = var_cnt;
		lcl_edu_tkn.var_array = var_array;
	}

	*o_err = EDU_NORMAL;

	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	if (op == EDP_OP_TYPE_ENC)
		sprintf(gl_log_string, "*****PRETTY-PRINT-AFTER-ENCODE***STARTS*****\n");
	else
		sprintf(gl_log_string, "*****PRETTY-PRINT-BEFORE-DECODE***STARTS*****\n");
	ncs_edu_log_msg(gl_log_string);

	lclrc = m_NCS_EDU_RUN_EDP(edu_hdl, &lcl_edu_tkn, NULL, edp, NULL, &lcl_cnt,
				  &lcl_buf_env, EDP_OP_TYPE_PP, o_err);

	m_NCS_EDU_TKN_FLUSH(&lcl_edu_tkn);
	if (buf_env->is_ubaid) {
		if (pp_ubaid.start != NULL) {
			m_MMGR_FREE_BUFR_LIST(pp_ubaid.start);
		} else {
			if (pp_ubaid.ub != NULL)
				m_MMGR_FREE_BUFR_LIST(pp_ubaid.ub);
		}
	}

	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	if (op == EDP_OP_TYPE_ENC)
		sprintf(gl_log_string, "*****PRETTY-PRINT-AFTER-ENCODE***ENDS*****\n");
	else
		sprintf(gl_log_string, "*****PRETTY-PRINT-BEFORE-DECODE***ENDS*****\n");
	ncs_edu_log_msg(gl_log_string);

	return lclrc;
}
#endif   /* #if(NCS_EDU_VERBOSE_PRINT == 1) */

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_perform_enc_op

  DESCRIPTION:      Utility function required for performing encode operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_perform_enc_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
			     EDU_BUF_ENV *buf_env, uint32_t *cnt, NCSCONTEXT arg,
			     EDU_ERR *o_err, uint8_t var_cnt, int *var_array)
{
	EDU_TKN tkn;
	uint32_t lclrc = NCSCC_RC_SUCCESS;

	memset(&tkn, '\0', sizeof(tkn));
	m_NCS_EDU_TKN_INIT(&tkn);
	tkn.i_edp = edp;
	tkn.parent_edp = edp;	/* Used only during selective-encoding */
	if (var_cnt != 0) {
		/* Used only during selective-encoding */
		tkn.var_cnt = var_cnt;
		tkn.var_array = var_array;
	}
	if (buf_env->is_ubaid && (buf_env->info.uba->start == NULL)) {
		/* Init only if it is not already done. */
		ncs_enc_init_space(buf_env->info.uba);
	}

	*o_err = EDU_NORMAL;

#if (NCS_EDU_VERBOSE_PRINT == 1)
	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	sprintf(gl_log_string, "*****ENCODE***STARTS*****\n");
	ncs_edu_log_msg(gl_log_string);
#endif

	lclrc = m_NCS_EDU_RUN_EDP(edu_hdl, &tkn, NULL, edp, arg, cnt, buf_env, EDP_OP_TYPE_ENC, o_err);
	m_NCS_EDU_TKN_FLUSH(&tkn);

#if (NCS_EDU_VERBOSE_PRINT == 1)
	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	sprintf(gl_log_string, "*****ENCODE***ENDS*****\n");
	ncs_edu_log_msg(gl_log_string);
#endif

	return lclrc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_perform_dec_op

  DESCRIPTION:      Utility function required for performing decode operation.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_perform_dec_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
			     EDU_BUF_ENV *buf_env, uint32_t *cnt,
			     NCSCONTEXT arg, EDU_ERR *o_err, uint8_t var_cnt, int *var_array)
{
	uint32_t lclrc = NCSCC_RC_SUCCESS;
	EDU_TKN tkn;

	/* Perform actual operation(Encode/Decode) below. */
	memset(&tkn, '\0', sizeof(tkn));
	m_NCS_EDU_TKN_INIT(&tkn);
	tkn.i_edp = edp;
	tkn.parent_edp = edp;	/* Used only during selective-decoding */
	if (var_cnt != 0) {
		/* Used only during selective-decoding */
		tkn.var_cnt = var_cnt;
		tkn.var_array = var_array;
	}

	if (buf_env->is_ubaid && ((buf_env->info.uba->start != NULL) || (buf_env->info.uba->max == 0))) {
		/* Init only if it is not already done. */
		if (buf_env->info.uba->start != NULL)
			ncs_dec_init_space(buf_env->info.uba, buf_env->info.uba->start);
		else {
			if (buf_env->info.uba->ub != NULL)
				ncs_dec_init_space(buf_env->info.uba, buf_env->info.uba->ub);
		}
	}

	*o_err = EDU_NORMAL;

#if (NCS_EDU_VERBOSE_PRINT == 1)
	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	sprintf(gl_log_string, "*****DECODE***STARTS*****\n");
	ncs_edu_log_msg(gl_log_string);
#endif

	lclrc = m_NCS_EDU_RUN_EDP(edu_hdl, &tkn, NULL, edp, arg, cnt, buf_env, EDP_OP_TYPE_DEC, o_err);
	m_NCS_EDU_TKN_FLUSH(&tkn);

#if (NCS_EDU_VERBOSE_PRINT == 1)
	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	sprintf(gl_log_string, "*****DECODE***ENDS*****\n");
	ncs_edu_log_msg(gl_log_string);
#endif

	return lclrc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_print_error_string

  DESCRIPTION:      Utility function for printing error strings onto the
                    "gl_log_string".

  RETURNS:          void

*****************************************************************************/
#if (NCS_EDU_VERBOSE_PRINT == 1)
void ncs_edu_print_error_string(int enum_val)
{
	memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
	switch (enum_val) {
	case EDU_ERR_MEM_FAIL:
		sprintf(gl_log_string, "EDU-ERR:Memory alloc failed...\n");
		break;
	case EDU_ERR_UBUF_PARSE_FAIL:
		sprintf(gl_log_string, "EDU-ERR:USRBUF parse failed...\n");
		break;
	case EDU_ERR_INV_EDP_VALUE:
		sprintf(gl_log_string, "EDU-ERR:Invalid EDP value encountered...\n");
		break;
	case EDU_ERR_EDU_START_NOT_FIRST_INSTR:
		sprintf(gl_log_string, "EDU-ERR:EDU_START is not the first instruction...\n");
		break;
	case EDU_ERR_EDP_NOT_MATCHING_IN_EDU_START_INSTR:
		sprintf(gl_log_string, "EDU-ERR:EDP not matching in EDU_START instruction...\n");
		break;
	case EDU_ERR_INV_NUMBER_OF_EDU_INSTRUCTIONS:
		sprintf(gl_log_string, "EDU-ERR:Invalid number of EDU instructions in EDP...\n");
		break;
	case EDU_ERR_EDU_END_NOT_LAST_INSTR:
		sprintf(gl_log_string, "EDU-ERR:EDU_END is not last instruction...\n");
		break;
	case EDU_ERR_ILLEGAL_INSTR_GIVEN:
		sprintf(gl_log_string, "EDU-ERR:Illegal EDU instruction given in EDP rules...\n");
		break;
	case EDU_ERR_DUPLICATE_EDU_START_INSTR_FOUND:
		sprintf(gl_log_string, "EDU-ERR:Duplicate EDU_START instruction in EDP rules...\n");
		break;
	case EDU_ERR_DUPLICATE_EDU_END_INSTR_FOUND:
		sprintf(gl_log_string, "EDU-ERR:Duplicate EDU_END instruction in EDP rules...\n");
		break;
	case EDU_ERR_DUPLICATE_EDU_TEST_LL_PTR_INSTR_FOUND:
		sprintf(gl_log_string, "EDU-ERR:Duplicate EDU_TEST_LL_PTR instruction in EDP rules...\n");
		break;
	case EDU_ERR_FIELD_OFFSET_EXCEEDS_EDP_SIZE:
		sprintf(gl_log_string, "EDU-ERR:Field offset exceeds EDP-size...\n");
		break;
	case EDU_ERR_VAR_LEN_PARAMETER_NOT_BASIC_EDP_TYPE:
		sprintf(gl_log_string, "EDU-ERR:Variable-length-parameter is not of basic EDP type...\n");
		break;
	case EDU_ERR_INV_ATTRIBUTE_FOR_LINKED_LIST_EDP:
		sprintf(gl_log_string, "EDU-ERR:Invalid attribute for linked list EDP...\n");
		break;
	case EDU_ERR_INV_ATTRIBUTE_COMBINATION_IN_START_INSTR:
		sprintf(gl_log_string, "EDU-ERR:Invalid attribute combination in EDU_START instruction...\n");
		break;
	case EDU_ERR_INV_ATTRIBUTE_FOR_EXEC_INSTR:
		sprintf(gl_log_string, "EDU-ERR:Invalid attribute for EDU_EXEC instruction...\n");
		break;
	case EDU_ERR_EDP_REFERENCES_SELF:
		sprintf(gl_log_string, "EDU-ERR:EDP executing self EDP, infinite loop...\n");
		break;
	case EDU_ERR_EXEC_INSTR_DOES_NOT_EXIST_FOR_OFFSET_OF_TEST_INSTR:
		sprintf(gl_log_string, "EDU-ERR:No EDU_EXEC existing for offset of EDU_TEST instruction...\n");
		break;
	case EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_TEST_FNC:
		sprintf(gl_log_string, "EDU-ERR:Invalid JUMP-TO offset provided by TEST fnc...\n");
		break;
	case EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_GE:
		sprintf(gl_log_string, "EDU-ERR:Invalid JUMP-TO offset provided by VER_GE...\n");
		break;
	case EDU_ERR_INV_JUMPTO_OFFSET_PROVIDED_BY_VER_USR:
		sprintf(gl_log_string, "EDU-ERR:Invalid JUMP-TO offset provided by VER_USR...\n");
		break;
	case EDU_ERR_EDP_NULL:
		sprintf(gl_log_string, "EDU-ERR:EDP Program handler NULL...\n");
		break;
	case EDU_ERR_SRC_POINTER_NULL:
		sprintf(gl_log_string, "EDU-ERR:Source data structure pointer is NULL...\n");
		break;
	case EDU_ERR_DEST_DOUBLE_POINTER_NULL:
		sprintf(gl_log_string, "EDU-ERR:Double pointer to destination data structure is NULL...\n");
		break;
	case EDU_ERR_POINTER_TO_EDU_ERR_RET_VAL_NULL:
		sprintf(gl_log_string, "EDU-ERR:Pointer to EDU_ERR return value is NULL...\n");
		break;
	case EDU_ERR_INV_OP_TYPE:
		sprintf(gl_log_string, "EDU-ERR:Invalid operation type specified...\n");
		break;
	case EDU_ERR_EDU_HDL_NULL:
		sprintf(gl_log_string, "EDU-ERR:Pointer to EDU_HDL is NULL...\n");
		break;
	case EDU_ERR_UBAID_POINTER_NULL:
		sprintf(gl_log_string, "EDU-ERR:Pointer to NCS_UBAID is NULL...\n");
		break;
	case EDU_ERR_POINTER_TO_CNT_NULL:
		sprintf(gl_log_string, "EDU-ERR:Pointer to count-argument is NULL...\n");
		break;
	case EDU_ERR_TEST_FUNC_NULL:
		sprintf(gl_log_string, "EDU-ERR:EDU_TEST function is NULL...\n");
		break;
	case EDU_ERR_VER_GE_FIELD_NULL:
		sprintf(gl_log_string, "EDU-ERR:EDU_VER_GE variable is NULL...\n");
		break;
	case EDU_ERR_VER_USR_FIELD_NULL:
		sprintf(gl_log_string, "EDU-ERR:EDU_VER_USR function is NULL...\n");
		break;
	case EDU_ERR_EDU_TEST_LL_PTR_INSTR_NOT_FOUND:
		sprintf(gl_log_string, "EDU-ERR:EDU_TEST_LL_PTR instruction not found...\n");
		break;
	case EDU_ERR_ILLEGAL_NEXT_LABEL_VALUE:
		sprintf(gl_log_string, "EDU-ERR:Illegal Next-Label found...\n");
		break;
	case EDU_ERR_EDP_NOT_USABLE_AT_EXEC_TIME:
		sprintf(gl_log_string, "EDU-ERR:EDP not usable at exec-time...\n");
		break;
	case EDU_ERR_EDP_NOT_FOUND_AT_EXEC_TIME:
		sprintf(gl_log_string, "EDU-ERR:EDP not found at exec-time...\n");
		break;
	case EDU_ERR_INV_LEN_SIZE_FOUND_FOR_VAR_SIZED_DATA:
		sprintf(gl_log_string, "EDU-ERR:Invalid length-size found for Variable-sized-data...\n");
		break;
	case EDU_ERR_TLV_BUF_POINTER_NULL:
		sprintf(gl_log_string, "EDU-ERR:Pointer to TLV-buffer is NULL...\n");
		break;
	case EDU_ERR_INV_TLV_BUF_SIZE:
		sprintf(gl_log_string, "EDU-ERR:Invalid TLV-buffer size specified...\n");
		break;
	case EDU_ERR_EDU_HDL_NOT_INITED_BY_OWNER:
		sprintf(gl_log_string, "EDU-ERR:EDU_HDL not inited by owner...\n");
		break;
	}
	ncs_edu_log_msg(gl_log_string);

	return;
}
#endif

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_is_edp_builtin

  DESCRIPTION:      Verifies whether this EDP is a LEAP-builtin EDP.

  RETURNS:          true    - If EDP is LEAP-builtin
                    false   - otherwise

*****************************************************************************/
bool ncs_edu_is_edp_builtin(EDU_PROG_HANDLER prog)
{
	if ((prog == ncs_edp_ncs_bool) ||
	    (prog == ncs_edp_uns8) ||
	    (prog == ncs_edp_uns16) ||
	    (prog == ncs_edp_uns32) ||
	    (prog == ncs_edp_int8) ||
	    (prog == ncs_edp_int16) ||
	    (prog == ncs_edp_int32) ||
	    (prog == ncs_edp_char) ||
	    (prog == ncs_edp_short) ||
	    (prog == ncs_edp_int) ||
	    (prog == ncs_edp_double) ||
	    (prog == ncs_edp_float) ||
	    (prog == ncs_edp_float) ||
	    (prog == ncs_edp_uns64) || (prog == ncs_edp_int64) || (prog == ncs_edp_string)) {
		return true;
	}

	return false;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_return_builtin_edp_size

  DESCRIPTION:      Verifies whether this EDP is a LEAP-builtin EDP, and returns
                    sizeof(edp-struct), if it is builtin EDP.

  RETURNS:          true    - If EDP is LEAP-builtin. Returns size of data 
                              structure in "o_size"
                    false   - otherwise

*****************************************************************************/
bool ncs_edu_return_builtin_edp_size(EDU_PROG_HANDLER prog, uint32_t *o_size)
{
	if (prog == ncs_edp_ncs_bool)
		*o_size = sizeof(bool);
	else if (prog == ncs_edp_uns8)
		*o_size = sizeof(uint8_t);
	else if (prog == ncs_edp_uns16)
		*o_size = sizeof(uint16_t);
	else if (prog == ncs_edp_uns32)
		*o_size = sizeof(uint32_t);
	else if (prog == ncs_edp_int8)
		*o_size = sizeof(int8_t);
	else if (prog == ncs_edp_int16)
		*o_size = sizeof(int16_t);
	else if (prog == ncs_edp_int32)
		*o_size = sizeof(int32_t);
	else if (prog == ncs_edp_char)
		*o_size = sizeof(char);
	else if (prog == ncs_edp_short)
		*o_size = sizeof(short);
	else if (prog == ncs_edp_int)
		*o_size = sizeof(int);
	else if (prog == ncs_edp_double)
		*o_size = sizeof(double);
	else if (prog == ncs_edp_float)
		*o_size = sizeof(float);
	else if (prog == ncs_edp_float)
		*o_size = sizeof(float);
	else if (prog == ncs_edp_uns64)
		*o_size = sizeof(uint64_t);
	else if (prog == ncs_edp_int64)
		*o_size = sizeof(int64_t);
	else if (prog == ncs_edp_string)
		*o_size = 0;	/* variable length */
	else
		return false;

	return true;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_compile_edp

  DESCRIPTION:      Runs EDCOMPILE operation on EDP.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_compile_edp(EDU_HDL *edu_hdl, EDU_PROG_HANDLER prog, EDU_HDL_NODE **hdl_node, EDU_ERR *o_err)
{
	EDU_HDL_NODE *lcl_hdl_node = NULL, *new_node = NULL;

	/* If hdl_node entry not present, add now. */
	if ((lcl_hdl_node = (EDU_HDL_NODE *)
	     ncs_patricia_tree_get(&edu_hdl->tree, (uint8_t *)&prog)) == NULL) {
		if ((lcl_hdl_node = (new_node = m_MMGR_ALLOC_EDU_HDL_NODE)) == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(lcl_hdl_node, 0, sizeof(EDU_HDL_NODE));

		/* Init the node info */
		lcl_hdl_node->edp = prog;
		lcl_hdl_node->pat_node.key_info = (uint8_t *)&lcl_hdl_node->edp;

		/* Add the node to the tree */
		if (ncs_patricia_tree_add(&edu_hdl->tree, &lcl_hdl_node->pat_node)
		    != NCSCC_RC_SUCCESS) {
			/* If an new entry is created free it. No need of any other cleanup */
			*o_err = EDU_ERR_MEM_FAIL;
			if (new_node)
				m_MMGR_FREE_EDU_HDL_NODE(new_node);
			return NCSCC_RC_FAILURE;
		}
		if (hdl_node != NULL)
			*hdl_node = lcl_hdl_node;
	} else {
		if (hdl_node != NULL)
			(*hdl_node) = lcl_hdl_node;
	}

	if (lcl_hdl_node->edcompile_pass)
		return NCSCC_RC_SUCCESS;

	return ncs_edu_run_edcompile_on_edp(edu_hdl, lcl_hdl_node, o_err);
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_hdl_init

  DESCRIPTION:      Initialises the application's EDU_HDL for using EDU.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_hdl_init(EDU_HDL *edu_hdl)
{
	NCS_PATRICIA_PARAMS list_params;

	memset(edu_hdl, '\0', sizeof(EDU_HDL));
	/* Init the tree first */
	list_params.key_size = sizeof(EDU_PROG_HANDLER);
	/*    list_params.info_size = 0;  */
	if ((ncs_patricia_tree_init(&edu_hdl->tree, &list_params))
	    != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Init the PPDB */
	edu_hdl->is_inited = true;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_hdl_flush

  DESCRIPTION:      Flushes the application's EDU_HDL, so that the application
                    can shut down.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edu_hdl_flush(EDU_HDL *edu_hdl)
{
	EDU_PROG_HANDLER lcl_key, *key = NULL;
	NCS_PATRICIA_NODE *pnode = NULL;

	if (edu_hdl->is_inited == true) {
		for (;;) {
			if ((pnode = ncs_patricia_tree_getnext(&edu_hdl->tree, (const uint8_t *)key)) != NULL) {
				/* Store the key for the next getnext call */
				lcl_key = ((EDU_HDL_NODE *)pnode)->edp;
				key = &lcl_key;

				/* Detach and free the nh-node */
				edu_hdl_node_del(edu_hdl, (EDU_HDL_NODE *)pnode);
			} else
				break;
		}

		/* Nuke the tree */
		ncs_patricia_tree_destroy(&edu_hdl->tree);
		/*edu_hdl->is_inited = false; */
	}
	memset(edu_hdl, '\0', sizeof(EDU_HDL));

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   edu_hdl_node_del

  DESCRIPTION:      Deletes EDU_HDL_NODE from application's EDU_HDL.

  RETURNS:          void

*****************************************************************************/
static void edu_hdl_node_del(EDU_HDL *edu_hdl, EDU_HDL_NODE *node)
{
	if (edu_hdl == (EDU_HDL *)NULL)
		return;

	/* Detach it from tree */
	ncs_patricia_tree_del(&edu_hdl->tree, (NCS_PATRICIA_NODE *)&node->pat_node);

	/* Free the node contents now. */
	ncs_edu_free_test_instr_rec_list(node->test_instr_store);
	node->test_instr_store = NULL;

	/* Free the node. */
	m_MMGR_FREE_EDU_HDL_NODE(node);

	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_skip_space

  DESCRIPTION:      Moves the pointer to current-offset in the tlv-buffer.

  RETURNS:          void

*****************************************************************************/
void ncs_edu_skip_space(EDU_TLV_ENV *tlv_env, uint32_t cnt)
{
	tlv_env->cur_bufp = (NCSCONTEXT)((uint8_t *)tlv_env->cur_bufp + cnt);
	tlv_env->bytes_consumed += cnt;

	if (tlv_env->bytes_consumed > tlv_env->size) {
		/* Error!!! Not sufficient space for parsing. TBD. */
		return;
	}

	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_8bit

  DESCRIPTION:      Encodes 8bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_8bit(uint8_t **stream, uint32_t val)
{
	uint16_t len = 1;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_8BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */
	*(*stream)++ = (uint8_t)(val);
	return EDU_TLV_HDR_SIZE + 1;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_16bit

  DESCRIPTION:      Encodes 16bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_16bit(uint8_t **stream, uint32_t val)
{
	uint16_t len = 2;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_16BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */
	*(*stream)++ = (uint8_t)(val >> 8);
	*(*stream)++ = (uint8_t)(val);
	return EDU_TLV_HDR_SIZE + 2;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_n_16bit

  DESCRIPTION:      Encodes "n"-16bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_n_16bit(uint8_t **stream, uint16_t *val_ptr, uint16_t n_count)
{
	uint16_t lcnt = 0, len = n_count, val = 0;

	if (n_count == 0)
		return 0;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_16BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */
	for (; lcnt < n_count; lcnt++) {
		val = *val_ptr;
		*(*stream)++ = (uint8_t)(val >> 8);
		*(*stream)++ = (uint8_t)(val);
		val_ptr++;
	}
	return (EDU_TLV_HDR_SIZE + (2 * n_count));
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_32bit

  DESCRIPTION:      Encodes 32bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_32bit(uint8_t **stream, uint32_t val)
{
	uint16_t len = 4;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_32BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */
	*(*stream)++ = (uint8_t)(val >> 24);
	*(*stream)++ = (uint8_t)(val >> 16);
	*(*stream)++ = (uint8_t)(val >> 8);
	*(*stream)++ = (uint8_t)(val);
	return EDU_TLV_HDR_SIZE + 4;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_n_32bit

  DESCRIPTION:      Encodes "n"-32bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_n_32bit(uint8_t **stream, uint32_t *val_ptr, uint16_t n_count)
{
	uint16_t lcnt = 0, len = n_count;
	uint32_t val = 0;

	if (n_count == 0)
		return 0;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_32BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */
	for (; lcnt < n_count; lcnt++) {
		val = *val_ptr;
		*(*stream)++ = (uint8_t)(val >> 24);
		*(*stream)++ = (uint8_t)(val >> 16);
		*(*stream)++ = (uint8_t)(val >> 8);
		*(*stream)++ = (uint8_t)(val);
		val_ptr++;
	}
	return (EDU_TLV_HDR_SIZE + (4 * n_count));
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_n_octets

  DESCRIPTION:      Encodes "octet" data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_n_octets(uint8_t **stream, uint8_t *val, uint16_t count)
{
	int i;
	uint16_t lcnt = count;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_OCT);	/* type */
	*(*stream)++ = (uint8_t)(lcnt >> 8);	/* length */
	*(*stream)++ = (uint8_t)lcnt;	/* length */
	if (count != 0) {
		if (val == NULL) {
			/* Source pointer is NULL. Still encode the "TL" part. */
			return (uint32_t)EDU_TLV_HDR_SIZE;
		}

		for (i = 0; i < count; i++)
			*(*stream)++ = *val++;
	}
	return (uint32_t)(EDU_TLV_HDR_SIZE + count);
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_encode_tlv_64bit

  DESCRIPTION:      Encodes 64bit data in "TLV" format. 

  RETURNS:          Number of bytes encoded.

*****************************************************************************/
uint32_t ncs_encode_tlv_64bit(uint8_t **stream, uint64_t val)
{
	uint16_t len = 8;

	*(*stream)++ = (uint8_t)(NCS_EDU_FMAT_64BIT);	/* type */
	*(*stream)++ = (uint8_t)(len >> 8);	/* length */
	*(*stream)++ = (uint8_t)len;	/* length */

	m_NCS_OS_HTONLL_P((*stream), val);
	(*stream) += 8;

	return EDU_TLV_HDR_SIZE + 8;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_64bit

  DESCRIPTION:      Encodes 64bit data in "TLV" format. 

  RETURNS:          Decoded "64bit" value.

*****************************************************************************/
uint64_t ncs_decode_tlv_64bit(uint8_t **stream)
{
	uint64_t val = 0;		/* Accumulator */

	(*stream)++;
	(*stream)++;
	(*stream)++;
	val = m_NCS_OS_NTOHLL_P((*stream));

	return val;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_32bit

  DESCRIPTION:      Encodes 32bit data in "TLV" format. 

  RETURNS:          Decoded "32bit" value.

*****************************************************************************/
uint32_t ncs_decode_tlv_32bit(uint8_t **stream)
{
	uint32_t val = 0;		/* Accumulator */

	(*stream)++;
	(*stream)++;
	(*stream)++;
	val = (uint32_t)*(*stream)++ << 24;
	val |= (uint32_t)*(*stream)++ << 16;
	val |= (uint32_t)*(*stream)++ << 8;
	val |= (uint32_t)*(*stream)++;

	return val;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_n_32bit

  DESCRIPTION:      Encodes "n"-32bit data in "TLV" format. 

  RETURNS:          Number of "uint32_t" decoded.

*****************************************************************************/
uint16_t ncs_decode_tlv_n_32bit(uint8_t **stream, uint32_t *dest)
{
	uint32_t val = 0;		/* Accumulator */
	uint16_t lcnt = 0, len = 0;

	(*stream)++;		/* type */

	len = (uint16_t)((uint8_t)(*(*stream)++) << 8);
	len |= (uint16_t)*(*stream)++;

	for (; lcnt < len; lcnt++) {
		val = (uint32_t)*(*stream)++ << 24;
		val |= (uint32_t)*(*stream)++ << 16;
		val |= (uint32_t)*(*stream)++ << 8;
		val |= (uint32_t)*(*stream)++;
		/* Convert to host-order and Write it back */
		dest[lcnt] = val;
	}

	return len;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_16bit

  DESCRIPTION:      Encodes 16bit data in "TLV" format. 

  RETURNS:          Decoded "16bit" value.

*****************************************************************************/
uint16_t ncs_decode_tlv_16bit(uint8_t **stream)
{
	uint32_t val = 0;		/* Accumulator */

	(*stream)++;
	(*stream)++;
	(*stream)++;
	val = (uint32_t)*(*stream)++ << 8;
	val |= (uint32_t)*(*stream)++;

	return (uint16_t)(val & 0x0000FFFF);
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_n_16bit

  DESCRIPTION:      Encodes "n"-16bit data in "TLV" format. 

  RETURNS:          Number of "uns16" decoded.

*****************************************************************************/
uint16_t ncs_decode_tlv_n_16bit(uint8_t **stream, uint16_t *dest)
{
	uint16_t val = 0;		/* Accumulator */
	uint16_t lcnt = 0, len = 0;

	(*stream)++;		/* type */

	len = (uint16_t)((uint8_t)(*(*stream)++) << 8);
	len |= (uint16_t)*(*stream)++;

	for (; lcnt < len; lcnt++) {
		val = (uint16_t)((uint16_t)*(*stream)++ << 8);
		val |= (uint16_t)*(*stream)++;
		/* Convert to host-order and Write it back */
		dest[lcnt] = val;
	}

	return len;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_8bit

  DESCRIPTION:      Encodes 8bit data in "TLV" format. 

  RETURNS:          Decoded "8bit" value.

*****************************************************************************/
uint8_t ncs_decode_tlv_8bit(uint8_t **stream)
{
	uint32_t val = 0;		/* Accumulator */

	(*stream)++;
	(*stream)++;
	(*stream)++;
	val = (uint32_t)*(*stream)++;

	return (uint8_t)(val & 0x000000FF);
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_decode_tlv_n_octets

  DESCRIPTION:      Encodes "octet" data in "TLV" format. 

  RETURNS:          Decoded "octet" value.

*****************************************************************************/
uint8_t *ncs_decode_tlv_n_octets(uint8_t *src, uint8_t *dest, uint32_t count)
{
	if (src == NULL)
		return NULL;

	(src)++;
	(src)++;
	(src)++;
	if (count != 0) {
		memcpy(dest, src, count);
	}

	return dest;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_copy_tlv_n_octets

  DESCRIPTION:      Copies "octet" data in "TLV" format. 

  RETURNS:          Copied "octet" data.

*****************************************************************************/
uint8_t *ncs_copy_tlv_n_octets(uint8_t *src, uint8_t *dest, uint32_t count)
{
	if (src == NULL)
		return NULL;

	(src)++;
	(src)++;
	(src)++;
	memcpy(dest, src, count);

	return dest;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_get_size_of_var_len_data

  DESCRIPTION:      Utility function for getting length of variable-sized data,
                    or TLV.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.

*****************************************************************************/
static uint32_t ncs_edu_get_size_of_var_len_data(EDU_PROG_HANDLER edp, NCSCONTEXT cptr, uint32_t *p_data_len, EDU_ERR *o_err)
{
	if (edp == ncs_edp_uns8)
		*p_data_len = (uint32_t)(*(uint8_t *)cptr);
	else if (edp == ncs_edp_int8)
		*p_data_len = (uint32_t)(*(int8_t *)cptr);
	else if (edp == ncs_edp_uns16)
		*p_data_len = (uint32_t)(*(uint16_t *)cptr);
	else if (edp == ncs_edp_int16)
		*p_data_len = (uint32_t)(*(int16_t *)cptr);
	else if (edp == ncs_edp_short)
		*p_data_len = (uint32_t)(*(short *)cptr);
	else if (edp == ncs_edp_int)
		*p_data_len = (uint32_t)(*(int *)cptr);
	else if (edp == ncs_edp_uns32)
		*p_data_len = (uint32_t)(*(uint32_t *)cptr);
	else if (edp == ncs_edp_int32)
		*p_data_len = (uint32_t)(*(int32_t *)cptr);
	else if (edp == ncs_edp_double)
		*p_data_len = (uint32_t)(*(double *)cptr);
	else if (edp == ncs_edp_uns64)
		*p_data_len = (uint32_t)(*(uint64_t *)cptr);	/* Downsizing 64-bit to 32-bit */
	else if (edp == ncs_edp_int64)
		*p_data_len = (uint32_t)(*(int64_t *)cptr);	/* Downsizing 64-bit to 32-bit */
	else {
		/* Failure. */
		*o_err = EDU_ERR_INV_LEN_SIZE_FOUND_FOR_VAR_SIZED_DATA;
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_free_uba_contents

  DESCRIPTION:      Utility function to free NCS_UBAID contents.

  RETURNS:          void

*****************************************************************************/
static void ncs_edu_free_uba_contents(NCS_UBAID *p_uba)
{
	if (p_uba->start) {
		m_MMGR_FREE_BUFR_LIST(p_uba->start);
	} else {
		m_MMGR_FREE_BUFR_LIST(p_uba->ub);
	}
	memset(p_uba, '\0', sizeof(NCS_UBAID));

	return;
}
