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

  DESCRIPTION: Implementation of common EDU programs (of common LEAP
  data structures) for NCS_EDU service.

******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "patricia.h"
#include "ncs_stack.h"
#include "ncssysf_mem.h"
#include "ncsencdec_pub.h"
#include "ncs_edu.h"
#include "sysf_def.h"
#include "ncs_ubaid.h"

extern char gl_log_string[];

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_ncs_bool

  DESCRIPTION:      EDU program handler for "boolean" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "boolean" data.
  NOTES:	    A bool is _forever_ 4 bytes when en/decoded due to
		    legacy reasons with the old NCS_BOOL which was 4 bytes.
  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_ncs_bool(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		       uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint32_t u32 = 0; /* Local variable to encode/decode as 4 bytes */
	bool *uptr = NULL; /* Used only in decode */

	/* Note that, bool is defined as "unsigned int" in ncsgl_defs.h. */
	switch (op) {
	case EDP_OP_TYPE_ENC:
		u32 = (bool)*((bool *)ptr); /* Encode as 4 bytes for backward compatibility */
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 4);
			ncs_encode_32bit(&p8, u32);
			ncs_enc_claim_space(buf_env->info.uba, 4);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_32bit(&p8, u32);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded bool: 0x%x \n", u32);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			if (*(bool **)ptr == NULL) {
				/* Since "ncs_edp_ncs_bool" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(bool **)ptr) = uptr = m_MMGR_ALLOC_EDP_bool;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(bool));
			} else {
				uptr = *((bool **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}
			*uptr = (bool)u32; /* Decode bool as 4 bytes for backward compatibility and store as bool */
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded bool: 0x%x \n", u32);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_bool;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(bool));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}
			if (uptr != NULL) {
				*uptr = (bool)u32;
				(*(bool **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated bool in PPDB: 0x%x \n", u32);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP bool: 0x%x \n", u32);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_uns8

  DESCRIPTION:      EDU program handler for "uns8" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "uns8" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_uns8(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		   uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint8_t u8 = 0x00;
	uint16_t len = 0;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	uint32_t i = 0;
#endif

	switch (op) {
	case EDP_OP_TYPE_ENC:
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 1);
			ncs_encode_8bit(&p8, *(uint8_t *)ptr);
			ncs_enc_claim_space(buf_env->info.uba, 1);
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns8: 0x%x \n", *(uint8_t *)ptr);
			ncs_edu_log_msg(gl_log_string);
#endif
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			if (*ptr_data_len <= 1) {
				*ptr_data_len = 1;
			}
			/* Multiple instances of "uns8" to be encoded */
			ncs_encode_tlv_n_octets(&p8, (uint8_t *)ptr, (uint8_t)*ptr_data_len);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + (*ptr_data_len));
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns8: len = %d\n", *ptr_data_len);
			ncs_edu_log_msg(gl_log_string);
			for (i = 0; i < *ptr_data_len; i++) {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				if (i == 0)
					sprintf(gl_log_string, "Encoded uns8: 0x%x \n", *(uint8_t *)ptr);
				else
					sprintf(gl_log_string, "        uns8: 0x%x \n", *(uint8_t *)((uint8_t *)ptr + i));
				ncs_edu_log_msg(gl_log_string);
			}
#endif
		}
		break;
	case EDP_OP_TYPE_DEC:
		{
			uint8_t *uptr = NULL;

			len = 1;	/* default */
			if (!buf_env->is_ubaid) {
				/* Look into length of the Octet-stream first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}

			if (*(uint8_t **)ptr == NULL) {
				/* Since "ncs_edp_uns8" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(uint8_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_UNS8(len);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', len * sizeof(uint8_t));
			} else {
				uptr = *((uint8_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, &u8, 1);
				u8 = ncs_decode_8bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 1);
				*uptr = u8;
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns8: 0x%x \n", u8);
				ncs_edu_log_msg(gl_log_string);
#endif
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				ncs_decode_tlv_n_octets(p8, (uint8_t *)uptr, len);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + len);

#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns8: len = %d\n", len);
				ncs_edu_log_msg(gl_log_string);
				for (i = 0; i < len; i++) {
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					if (i == 0)
						sprintf(gl_log_string, "Decoded uns8: 0x%x \n", *(uint8_t *)uptr);
					else
						sprintf(gl_log_string,
							"        uns8: 0x%x \n", *(uint8_t *)((uint8_t *)uptr + i));
					ncs_edu_log_msg(gl_log_string);
				}
#endif
			}
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			uint8_t *uptr = NULL;

			len = 1;	/* default */
			if (!buf_env->is_ubaid) {
				/* Look into length of the Octet-stream first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}

			uptr = m_MMGR_ALLOC_EDP_UNS8(len);
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', len);

			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, &u8, 1);
				u8 = ncs_decode_8bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 1);
				*uptr = u8;
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns8: 0x%x \n", u8);
				ncs_edu_log_msg(gl_log_string);
#endif
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				ncs_decode_tlv_n_octets(p8, (uint8_t *)uptr, len);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + len);

#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns8: len = %d\n", len);
				ncs_edu_log_msg(gl_log_string);
				for (i = 0; i < len; i++) {
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					if (i == 0)
						sprintf(gl_log_string, "PP uns8: 0x%x \n", *(uint8_t *)uptr);
					else
						sprintf(gl_log_string, "   uns8: 0x%x \n", *(uint8_t *)((uint8_t *)uptr + i));
					ncs_edu_log_msg(gl_log_string);
				}
#endif
			}

			if (ptr != NULL) {
				(*(uint8_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated uint8_t in PPDB: len = %d\n", len);
				ncs_edu_log_msg(gl_log_string);
			} else {
				/* Free uptr */
				m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
			}
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_uns16

  DESCRIPTION:      EDU program handler for "uns16" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "uns16" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_uns16(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint16_t u16, len = 1, byte_cnt = 0;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u16 = *(uint16_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 2);
			ncs_encode_16bit(&p8, u16);
			ncs_enc_claim_space(buf_env->info.uba, 2);
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns16: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
#endif
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			if (*ptr_data_len <= 1) {
				*ptr_data_len = 1;
			}
			/* "*ptr_data_len" instances of "uns16" to be encoded */
			byte_cnt = (uint16_t)ncs_encode_tlv_n_16bit(&p8, (uint16_t *)ptr, (uint16_t)*ptr_data_len);
			ncs_edu_skip_space(&buf_env->info.tlv_env, byte_cnt);
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns16: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
	case EDP_OP_TYPE_DEC:
		{
			uint16_t *uptr = NULL;

			len = 1;	/* default */
			if (!buf_env->is_ubaid) {
				/* Look into length of the Octet-stream first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}

			if (*(uint16_t **)ptr == NULL) {
				/* Since "ncs_edp_uns16" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(uint16_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(uint16_t) * len);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', (sizeof(uint16_t) * len));
			} else {
				uptr = *((uint16_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
				*uptr = u16;
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns16: 0x%x\n", u16);
				ncs_edu_log_msg(gl_log_string);
#endif
			} else {
				uint32_t uns16_cnt = 0;

				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				uns16_cnt = ncs_decode_tlv_n_16bit(&p8, uptr);
				ncs_edu_skip_space(&buf_env->info.tlv_env,
						   EDU_TLV_HDR_SIZE + (sizeof(uint16_t) * uns16_cnt));
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns16: 0x%x\n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
#endif
			}
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			uint16_t *uptr = NULL;

			len = 1;	/* default */
			if (!buf_env->is_ubaid) {
				/* Look into length of the Octet-stream first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}

			uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(uint16_t) * len);
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', sizeof(uint16_t) * len);

			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
				*uptr = u16;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns16: 0x%x\n", u16);
				ncs_edu_log_msg(gl_log_string);
			} else {
				uint32_t uns16_cnt = 0;

				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				uns16_cnt = ncs_decode_tlv_n_16bit(&p8, uptr);
				ncs_edu_skip_space(&buf_env->info.tlv_env,
						   EDU_TLV_HDR_SIZE + (sizeof(uint16_t) * uns16_cnt));
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns16: 0x%x\n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
			}

			if (ptr != NULL) {
				(*(uint16_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated uint16_t in PPDB: 0x%x \n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
			} else {
				m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
			}
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_uns32

  DESCRIPTION:      EDU program handler for "uns32" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "uns32" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_uns32(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint32_t u32 = 0x00000000;
	uint16_t len = sizeof(uint8_t);

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u32 = *(uint32_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 4);
			ncs_encode_32bit(&p8, u32);
			ncs_enc_claim_space(buf_env->info.uba, 4);
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns32: 0x%x \n", u32);
			ncs_edu_log_msg(gl_log_string);
#endif
		} else {
			uint32_t byte_cnt = 0;
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			if (*ptr_data_len <= 1) {
				*ptr_data_len = 1;
			}
			byte_cnt = ncs_encode_tlv_n_32bit(&p8, (uint32_t *)ptr, (uint16_t)(*ptr_data_len));
			ncs_edu_skip_space(&buf_env->info.tlv_env, byte_cnt);
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Encoded uns32: 0x%x \n", u32);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
	case EDP_OP_TYPE_DEC:
		{
			uint32_t *uptr = NULL;

			if (!buf_env->is_ubaid) {
				/* Look into length of the "uns32" first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}

			if (*(uint32_t **)ptr == NULL) {
				/* Since "ncs_edp_uns32" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(uint32_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_UNS8(4 * len);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', 4 * len);
			} else {
				uptr = *((uint32_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
				*uptr = u32;
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns32: 0x%x \n", u32);
				ncs_edu_log_msg(gl_log_string);
#endif
			} else {
				uint32_t uns32_cnt = 0;

				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				uns32_cnt = ncs_decode_tlv_n_32bit(&p8, uptr);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + (4 * uns32_cnt));
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded uns32: 0x%x \n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
#endif
			}
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			uint32_t *uptr = NULL;

			if (!buf_env->is_ubaid) {
				/* Look into length of the "uns32" first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = ((uint16_t)*(p8)++) << 8;
				len |= (uint16_t)*(p8)++;
			}
			uptr = m_MMGR_ALLOC_EDP_UNS8(4 * len);
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', 4 * len);
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
				*uptr = u32;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns32: 0x%x \n", u32);
				ncs_edu_log_msg(gl_log_string);
			} else {
				uint32_t uns32_cnt = 0;

				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				uns32_cnt = ncs_decode_tlv_n_32bit(&p8, uptr);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + (4 * uns32_cnt));
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP uns32: 0x%x \n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
			}
			if (ptr != NULL) {
				(*(uint32_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated uint32_t in PPDB: 0x%x \n", (*uptr));
				ncs_edu_log_msg(gl_log_string);
			} else {
				m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
			}
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_char

  DESCRIPTION:      EDU program handler for "char" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "char" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_char(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		   uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		{
			if (buf_env->is_ubaid) {
				p8 = ncs_enc_reserve_space(buf_env->info.uba, 1);
				ncs_encode_8bit(&p8, *(uint8_t *)ptr);
				ncs_enc_claim_space(buf_env->info.uba, 1);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				ncs_encode_tlv_8bit(&p8, *(uint8_t *)ptr);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 1);
			}
#if(NCS_EDU_VERBOSE_PRINT == 1)
			if (((char *)ptr)[0] != '\0') {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Encoded char: %c \n", ((char *)ptr)[0]);
				ncs_edu_log_msg(gl_log_string);
			}
#endif
		}
		break;
	case EDP_OP_TYPE_DEC:
		{
			char *uptr = NULL;

			*ptr_data_len = 1;

			if (*(uint8_t **)ptr == NULL) {
				/* Since "ncs_edp_char" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(char **)ptr) = uptr = m_MMGR_ALLOC_EDP_CHAR(*ptr_data_len);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', (sizeof(char) * (*ptr_data_len)));
			} else {
				uptr = *((char **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, *ptr_data_len);
				memcpy(uptr, p8, *ptr_data_len);
				ncs_dec_skip_space(buf_env->info.uba, *ptr_data_len);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				*uptr = ncs_decode_tlv_8bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + (*ptr_data_len));
			}
#if(NCS_EDU_VERBOSE_PRINT == 1)
			if (((char *)uptr)[0] != '\0') {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded char: %c \n", ((char *)uptr)[0]);
				ncs_edu_log_msg(gl_log_string);
			}
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			char *uptr = NULL;

			*ptr_data_len = 1;
			uptr = m_MMGR_ALLOC_EDP_CHAR(*ptr_data_len);
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', (sizeof(char) * (*ptr_data_len)));

			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, *ptr_data_len);
				memcpy(uptr, p8, *ptr_data_len);
				ncs_dec_skip_space(buf_env->info.uba, *ptr_data_len);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				*uptr = ncs_decode_tlv_8bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + (*ptr_data_len));
			}

			if (((char *)uptr)[0] != '\0') {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP char: %c \n", ((char *)uptr)[0]);
				ncs_edu_log_msg(gl_log_string);
			}

			if (ptr != NULL) {
				(*(char **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated char in PPDB: %c \n", ((char *)uptr)[0]);
				ncs_edu_log_msg(gl_log_string);
			} else {
				m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
			}
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_string

  DESCRIPTION:      EDU program handler for "string" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "string" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_string(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		     uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8, *src_p8;
	uint16_t len = 0, byte_cnt = 0;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		if (ptr_data_len == NULL)
			return NCSCC_RC_FAILURE;

		/* Length of data to be encoded is "*ptr_data_len" */
		len = (uint16_t)(*ptr_data_len);
		src_p8 = (uint8_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 2);
			ncs_encode_16bit(&p8, len);
			ncs_enc_claim_space(buf_env->info.uba, 2);
			if (len != 0) {
				ncs_encode_n_octets_in_uba(buf_env->info.uba, src_p8, len);
			}
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			byte_cnt = (uint16_t)ncs_encode_tlv_n_octets(&p8, src_p8, (uint8_t)len);
			ncs_edu_skip_space(&buf_env->info.tlv_env, byte_cnt);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded string: len = 0x%x\n", len);
		ncs_edu_log_msg(gl_log_string);
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded string: %s\n", src_p8);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		if (ptr_data_len == NULL)
			return NCSCC_RC_FAILURE;

		if (buf_env->is_ubaid) {
			p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&len, 2);
			len = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(buf_env->info.uba, 2);
			*ptr_data_len = len;
		} else {
			/* Look into length of the string first. */
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			p8++;
			len = ((uint16_t)*(p8)++) << 8;
			len |= (uint16_t)*(p8)++;
			if (len == 0) {
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE);
#if(NCS_EDU_VERBOSE_PRINT == 1)
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Decoded string: len = 0x%x\n", len);
				ncs_edu_log_msg(gl_log_string);
#endif
			}
		}

		if (len != 0) {
			char *uptr = NULL;

			if (*(char **)ptr == NULL) {
				/* Since "ncs_edp_string" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(char **)ptr) = uptr = m_MMGR_ALLOC_EDP_CHAR(len + 1);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', (len * sizeof(char) + 1));
			} else {
				uptr = *((char **)ptr);
			}

			if (buf_env->is_ubaid) {
				if (ncs_decode_n_octets_from_uba(buf_env->info.uba,
								 (uint8_t *)uptr, len) != NCSCC_RC_SUCCESS) {
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				ncs_decode_tlv_n_octets(p8, (uint8_t *)uptr, (uint32_t)len);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + len);
			}

#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded string: len = 0x%x\n", len);
			ncs_edu_log_msg(gl_log_string);
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded string: %s\n", uptr);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			if (ptr_data_len == NULL)
				return NCSCC_RC_FAILURE;

			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&len, 2);
				len = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				/* Look into length of the string first. */
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				p8++;
				len = (uint16_t)*(p8)++ << 8;
				len |= (uint16_t)*(p8)++;
				if (len == 0)
					ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE);
			}
			*ptr_data_len = len;

			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP string: len = 0x%x\n", len);
			ncs_edu_log_msg(gl_log_string);

			if (len != 0) {
				char *uptr = NULL;

				uptr = m_MMGR_ALLOC_EDP_CHAR(len + 1);
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', (len * sizeof(char) + 1));
				/* This string pointer(uptr) is now null-terminated. */

				if (buf_env->is_ubaid) {
					if (ncs_decode_n_octets_from_uba(buf_env->info.uba,
									 (uint8_t *)uptr, len) != NCSCC_RC_SUCCESS) {
						*o_err = EDU_ERR_MEM_FAIL;
						return NCSCC_RC_FAILURE;
					}
				} else {
					p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
					ncs_decode_tlv_n_octets(p8, (uint8_t *)uptr, (uint32_t)len);
					ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + len);
				}

				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "PP string: %s\n", uptr);
				ncs_edu_log_msg(gl_log_string);

				if (ptr != NULL) {
					(*(char **)ptr) = uptr;
					memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
					sprintf(gl_log_string, "Populated string in PPDB: %s \n", uptr);
					ncs_edu_log_msg(gl_log_string);
				} else {
					m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
				}
			}
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_short

  DESCRIPTION:      EDU program handler for "short" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "short" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_short(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	short u16;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u16 = *(uint16_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 2);
			ncs_encode_16bit(&p8, u16);
			ncs_enc_claim_space(buf_env->info.uba, 2);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_16bit(&p8, u16);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded short: 0x%x\n", u16);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			short *uptr = NULL;

			if (*(short **)ptr == NULL) {
				/* Since "ncs_edp_short" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(short **)ptr) = uptr = m_MMGR_ALLOC_EDP_SHORT;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(short));
			} else {
				uptr = *((short **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			*uptr = u16;
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded short: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			short *uptr = NULL;

			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_SHORT;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(short));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}

			if (uptr != NULL) {
				*uptr = u16;
				(*(short **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated short in PPDB: 0x%x \n", u16);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP short: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_int

  DESCRIPTION:      EDU program handler for "int" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "int" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_int(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		  uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint32_t u32 = 0x00000000;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u32 = (uint32_t)*(int *)ptr;	/* Typecast int to uint32_t */
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 4);
			ncs_encode_32bit(&p8, u32);
			ncs_enc_claim_space(buf_env->info.uba, 4);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_32bit(&p8, u32);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
		}

#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded int: %d \n", u32);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			int *uptr = NULL;

			if (*(int **)ptr == NULL) {
				/* Since "ncs_edp_int" is the responsibility of LEAP, LEAP
				   is supposed to malloc this memory. */
				(*(int **)ptr) = uptr = m_MMGR_ALLOC_EDP_INT;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int));
			} else {
				uptr = *((int **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}
			*uptr = (int)u32;	/* Typecast uint32_t to int */
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded int: %d \n", u32);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			int *uptr = NULL;

			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_INT;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}

			if (uptr != NULL) {
				*uptr = (int)u32;	/* Typecast uint32_t to int */
				(*(int **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated int in PPDB: %d \n", u32);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP int: %d \n", u32);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_int8

  DESCRIPTION:      EDU program handler for "int8" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "int8" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_int8(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		   uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	int8_t u8 = 0x00;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 1);
			ncs_encode_8bit(&p8, *(int8_t *)ptr);
			ncs_enc_claim_space(buf_env->info.uba, 1);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_8bit(&p8, *(int8_t *)ptr);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 1);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded int8: 0x%x \n", *(int8_t *)ptr);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			int8_t *uptr = NULL;

			if (*(int8_t **)ptr == NULL) {
				(*(int8_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_INT8;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int8_t));
			} else {
				uptr = *((int8_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u8, 1);
				u8 = ncs_decode_8bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 1);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u8 = ncs_decode_tlv_8bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 1);
			}
			*uptr = u8;
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded int8: 0x%x \n", u8);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			int8_t *uptr = NULL;

			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_INT8;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(uint8_t));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u8, 1);
				u8 = ncs_decode_8bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 1);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u8 = ncs_decode_tlv_8bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 1);
			}

			if (uptr != NULL) {
				*uptr = u8;
				(*(int8_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated int8_t in PPDB: 0x%x \n", u8);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP int8: 0x%x\n", u8);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_int16

  DESCRIPTION:      EDU program handler for "int16" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "int16" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_int16(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	int16_t u16;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u16 = *(int16_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 2);
			ncs_encode_16bit(&p8, u16);
			ncs_enc_claim_space(buf_env->info.uba, 2);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_16bit(&p8, u16);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded int16: 0x%x\n", u16);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			int16_t *uptr = NULL;

			if (*(int16_t **)ptr == NULL) {
				(*(int16_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_INT16;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int16_t));
			} else {
				uptr = *((int16_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}
			*uptr = u16;
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded int16: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			int16_t *uptr = NULL;

			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_INT16;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int16_t));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u16, 2);
				u16 = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 2);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u16 = ncs_decode_tlv_16bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 2);
			}

			if (uptr != NULL) {
				*uptr = u16;
				(*(int16_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated int16_t in PPDB: 0x%x \n", u16);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP int16: 0x%x\n", u16);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_int32

  DESCRIPTION:      EDU program handler for "int32" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "int32" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_int32(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	int32_t u32 = 0x00000000;

	switch (op) {
	case EDP_OP_TYPE_ENC:
		u32 = *(int32_t *)ptr;
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, 4);
			ncs_encode_32bit(&p8, u32);
			ncs_enc_claim_space(buf_env->info.uba, 4);
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_32bit(&p8, u32);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
		}
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded int32: 0x%x \n", (int)u32);
		ncs_edu_log_msg(gl_log_string);
#endif
		break;
	case EDP_OP_TYPE_DEC:
		{
			int32_t *uptr = NULL;

			if (*(int32_t **)ptr == NULL) {
				(*(int32_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_INT32;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int32_t));
			} else {
				uptr = *((int32_t **)ptr);
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}
			*uptr = u32;
#if(NCS_EDU_VERBOSE_PRINT == 1)
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Decoded int32: 0x%x \n", (int)u32);
			ncs_edu_log_msg(gl_log_string);
#endif
		}
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		{
			int32_t *uptr = NULL;

			if (ptr != NULL) {
				/* We need populate the pointer here, so that it can
				   be sent back to the parent invoker(most likely
				   to be able to perform the TEST condition). */
				uptr = m_MMGR_ALLOC_EDP_INT32;
				if (uptr == NULL) {
					/* Memory failure. */
					*o_err = EDU_ERR_MEM_FAIL;
					return NCSCC_RC_FAILURE;
				}
				memset(uptr, '\0', sizeof(int32_t));
			}
			if (buf_env->is_ubaid) {
				p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)&u32, 4);
				u32 = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(buf_env->info.uba, 4);
			} else {
				p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
				u32 = ncs_decode_tlv_32bit(&p8);
				ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + 4);
			}
			if (uptr != NULL) {
				*uptr = u32;
				(*(int32_t **)ptr) = uptr;
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				sprintf(gl_log_string, "Populated int32_t in PPDB: 0x%x \n", (int)u32);
				ncs_edu_log_msg(gl_log_string);
			}
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "PP int32: 0x%x \n", (int)u32);
			ncs_edu_log_msg(gl_log_string);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_ncs_key

  DESCRIPTION:      EDU program handler for "NCS_KEY" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "NCS_KEY" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_ncs_key(EDU_HDL *hdl, EDU_TKN *edu_tkn,
		      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_KEY *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET ncs_key_rules[] = {
		{EDU_START, ncs_edp_ncs_key, 0, 0, 0, sizeof(NCS_KEY), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((NCS_KEY *)0)->svc, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((NCS_KEY *)0)->type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((NCS_KEY *)0)->fmat, 0, NULL},

		{EDU_TEST, ncs_edp_uns8, 0, 0, 0,
		 (long)&((NCS_KEY *)0)->fmat, 0, ncs_edu_ncs_key_test_fmat_fnc},

		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((NCS_KEY *)0)->val.num, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, EDU_EXIT,
		 (long)&((NCS_KEY *)0)->val.str, SYSF_MAX_KEY_LEN, NULL},

		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0,
		 (long)&((NCS_KEY *)0)->val.oct.len, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, EDU_EXIT,
		 (long)&((NCS_KEY *)0)->val.oct.data, SYSF_MAX_KEY_LEN, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (NCS_KEY *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (NCS_KEY **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(NCS_KEY));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, ncs_key_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edu_ncs_key_test_fmat_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "ncs_edp_ncs_key".

  RETURNS:          char *, denoting the label to execute next.

*****************************************************************************/
int ncs_edu_ncs_key_test_fmat_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_FMT_NUM = NCS_FMT_NUM,
		LCL_TEST_JUMP_OFFSET_FMT_STR = NCS_FMT_STR,
		LCL_TEST_JUMP_OFFSET_FMT_OCT = NCS_FMT_OCT
	};
	uint8_t fmat;

	fmat = *(uint8_t *)arg;

	switch (fmat) {
	case NCS_FMT_NUM:
		return LCL_TEST_JUMP_OFFSET_FMT_NUM;
	case NCS_FMT_STR:
		return LCL_TEST_JUMP_OFFSET_FMT_STR;
	case NCS_FMT_OCT:
		return LCL_TEST_JUMP_OFFSET_FMT_OCT;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_uns64

  DESCRIPTION:      EDU program handler for "uns64" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "uns64" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_uns64(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	uint64_t *uptr = NULL;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	uint8_t *src_p8;
	uint32_t cnt = 0;
#endif

	/* Note :- TLV-encoding/decoding support not put up for this data type, 
	   as no requirement yet on the TLV side arose.
	 */
	switch (op) {
	case EDP_OP_TYPE_ENC:
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, sizeof(uint64_t));
			ncs_encode_64bit(&p8, *(uint64_t *)ptr);
			ncs_enc_claim_space(buf_env->info.uba, sizeof(uint64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_64bit(&p8, *(uint64_t *)ptr);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(uint64_t));
		}

#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded uns64: len = 0x%x : ", sizeof(uint64_t));
		ncs_edu_log_msg(gl_log_string);
		for (cnt = 0; cnt < sizeof(uint64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(uint64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}
#endif
		break;
	case EDP_OP_TYPE_DEC:
		if (*(uint64_t **)ptr == NULL) {
			/* Since "uns64" is the responsibility of LEAP, LEAP
			   is supposed to malloc this memory. */
			(*(uint64_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(uint64_t) / sizeof(uint8_t));
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', sizeof(uint64_t));
		} else {
			uptr = *((uint64_t **)ptr);
		}
		if (buf_env->is_ubaid) {
			p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, sizeof(uint64_t));
			*uptr = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(buf_env->info.uba, sizeof(uint64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			*uptr = ncs_decode_tlv_64bit(&p8);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(uint64_t));
		}

#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Decoded uns64: len = 0x%x : ", sizeof(uint64_t));
		ncs_edu_log_msg(gl_log_string);
		src_p8 = (uint8_t *)uptr;
		for (cnt = 0; cnt < sizeof(uint64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(uint64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}
#endif
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		/* We need populate the pointer here, so that it can
		   be sent back to the parent invoker(most likely
		   to be able to perform the TEST condition). */
		uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(uint64_t) / sizeof(uint8_t));
		if (uptr == NULL) {
			/* Memory failure. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(uptr, '\0', sizeof(uint64_t));
		if (buf_env->is_ubaid) {
			p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, sizeof(uint64_t));
			*uptr = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(buf_env->info.uba, sizeof(uint64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			*uptr = ncs_decode_tlv_64bit(&p8);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(uint64_t));
		}

		src_p8 = (uint8_t *)uptr;
#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "PP uns64: len = 0x%x : ", sizeof(uint64_t));
		ncs_edu_log_msg(gl_log_string);
		for (cnt = 0; cnt < sizeof(uint64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(uint64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}
#endif

		if (ptr != NULL) {
			(*(uint64_t **)ptr) = uptr;

			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Populated uint64_t in PPDB: len = 0x%x : ", sizeof(uint64_t));
			ncs_edu_log_msg(gl_log_string);
			for (cnt = 0; cnt < sizeof(uint64_t); cnt++) {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				if (cnt == (sizeof(uint64_t) - 1))
					sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
				else
					sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
				ncs_edu_log_msg(gl_log_string);
			}
		} else {
			m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_int64

  DESCRIPTION:      EDU program handler for "int64" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "int64" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_int64(EDU_HDL *hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr,
		    uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint8_t *p8;
	int64_t *uptr = NULL;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	uint8_t *src_p8;
	uint32_t cnt = 0;
#endif

	/* Note :- TLV-encoding/decoding support not put up for this data type, 
	   as no requirement yet on the TLV side arose.
	 */
	switch (op) {
	case EDP_OP_TYPE_ENC:
		if (buf_env->is_ubaid) {
			p8 = ncs_enc_reserve_space(buf_env->info.uba, sizeof(int64_t));
			ncs_encode_64bit(&p8, *(int64_t *)ptr);
			ncs_enc_claim_space(buf_env->info.uba, sizeof(int64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			ncs_encode_tlv_64bit(&p8, *(int64_t *)ptr);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(int64_t));
		}

#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Encoded int64: len = 0x%x : ", sizeof(int64_t));
		ncs_edu_log_msg(gl_log_string);
		for (cnt = 0; cnt < sizeof(int64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(int64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}
#endif
		break;
	case EDP_OP_TYPE_DEC:
		if (*(int64_t **)ptr == NULL) {
			/* Since "int64" is the responsibility of LEAP, LEAP
			   is supposed to malloc this memory. */
			(*(int64_t **)ptr) = uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(int64_t) / sizeof(uint8_t));
			if (uptr == NULL) {
				/* Memory failure. */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(uptr, '\0', sizeof(int64_t));
		} else {
			uptr = *((int64_t **)ptr);
		}
		if (buf_env->is_ubaid) {
			p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, sizeof(int64_t));
			*uptr = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(buf_env->info.uba, sizeof(int64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			*uptr = ncs_decode_tlv_64bit(&p8);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(int64_t));
		}

#if(NCS_EDU_VERBOSE_PRINT == 1)
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "Decoded int64: len = 0x%x : ", sizeof(int64_t));
		ncs_edu_log_msg(gl_log_string);
		src_p8 = (uint8_t *)uptr;
		for (cnt = 0; cnt < sizeof(int64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(int64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}
#endif
		break;
#if(NCS_EDU_VERBOSE_PRINT == 1)
	case EDP_OP_TYPE_PP:
		/* We need populate the pointer here, so that it can
		   be sent back to the parent invoker(most likely
		   to be able to perform the TEST condition). */
		uptr = m_MMGR_ALLOC_EDP_UNS8(sizeof(int64_t) / sizeof(uint8_t));
		if (uptr == NULL) {
			/* Memory failure. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(uptr, '\0', sizeof(int64_t));
		if (buf_env->is_ubaid) {
			p8 = ncs_dec_flatten_space(buf_env->info.uba, (uint8_t *)uptr, sizeof(int64_t));
			*uptr = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(buf_env->info.uba, sizeof(int64_t));
		} else {
			p8 = (uint8_t *)buf_env->info.tlv_env.cur_bufp;
			*uptr = ncs_decode_tlv_64bit(&p8);
			ncs_edu_skip_space(&buf_env->info.tlv_env, EDU_TLV_HDR_SIZE + sizeof(int64_t));
		}

		src_p8 = (uint8_t *)uptr;
		memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
		sprintf(gl_log_string, "PP int64: len = 0x%x : ", sizeof(int64_t));
		ncs_edu_log_msg(gl_log_string);
		for (cnt = 0; cnt < sizeof(int64_t); cnt++) {
			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			if (cnt == (sizeof(int64_t) - 1))
				sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
			else
				sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
			ncs_edu_log_msg(gl_log_string);
		}

		if (ptr != NULL) {
			(*(int64_t **)ptr) = uptr;

			memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
			sprintf(gl_log_string, "Populated int64_t in PPDB: len = 0x%x : ", sizeof(int64_t));
			ncs_edu_log_msg(gl_log_string);
			for (cnt = 0; cnt < sizeof(int64_t); cnt++) {
				memset(&gl_log_string, '\0', GL_LOG_STRING_LEN);
				if (cnt == (sizeof(int64_t) - 1))
					sprintf(gl_log_string, "0x%x \n", src_p8[cnt]);
				else
					sprintf(gl_log_string, "0x%x ", src_p8[cnt]);
				ncs_edu_log_msg(gl_log_string);
			}
		} else {
			m_NCS_MEM_FREE(uptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
		}
		break;
#endif
	default:
		/* Invalid Operation */
		break;
	}
	return NCSCC_RC_SUCCESS;
}
