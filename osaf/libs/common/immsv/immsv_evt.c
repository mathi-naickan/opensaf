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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
  DESCRIPTION:
  
  This file consists of IMMSV routines used for encoding/decoding pointer
  structures in an IMMSV_EVT.
*****************************************************************************/

#include "immsv.h"

#define IMMSV_MAX_ATTRIBUTES 128
#define IMMSV_MAX_CLASSES 1000
#define IMMSV_MAX_IMPLEMENTERS 3000
#define IMMSV_MAX_ADMINOWNERS 2000
#define IMMSV_MAX_OBJECTS 10000
#define IMMSV_MAX_CCBS 1000

/* This array must match IMMD_EVT_TYPE */
static const char *immd_evt_names[] = {
	"undefined",
	"IMMD_EVT_MDS_INFO",
	"IMMD_EVT_ND2D_INTRO",
	"IMMD_EVT_ND2D_ANNOUNCE_LOADING",
	"IMMD_EVT_ND2D_REQ_SYNC",
	"IMMD_EVT_ND2D_ANNOUNCE_DUMP",
	"IMMD_EVT_ND2D_SYNC_START",
	"IMMD_EVT_ND2D_SYNC_ABORT",
	"IMMD_EVT_ND2D_ADMINIT_REQ",
	"IMMD_EVT_ND2D_ACTIVE_SET",
	"IMMD_EVT_ND2D_IMM_SYNC_INFO2",
	"IMMD_EVT_ND2D_FEVS_REQ",
	"IMMD_EVT_ND2D_CCBINIT_REQ",
	"IMMD_EVT_ND2D_IMPLSET_REQ2",
	"IMMD_EVT_ND2D_OI_OBJ_MODIFY",
	"IMMD_EVT_ND2D_DISCARD_IMPL",
	"IMMD_EVT_ND2D_ABORT_CCB",
	"IMMD_EVT_ND2D_ADMO_HARD_FINALIZE",
	"IMMD_EVT_CB_DUMP",
	"IMMD_EVT_MDS_QUIESCED_ACK_RSP",
	"IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS",
	"IMMD_EVT_ND2D_LOADING_FAILED",
	"IMMD_EVT_LGA_CB"
};

static const char *immsv_get_immd_evt_name(unsigned int id)
{
	if (id < IMMD_EVT_MAX)
		return immd_evt_names[id];
	else
		return "unknown";
}

/* This array must match IMMND_EVT_TYPE */
static const char *immnd_evt_names[] = {
	"undefined",
	"IMMND_EVT_MDS_INFO",	/* IMMA/IMMND/IMMD UP/DOWN Info */
	"IMMND_EVT_TIME_OUT",	/* Time out event */
	"IMMND_EVT_A2ND_IMM_INIT",	/* ImmOm Initialization */
	"IMMND_EVT_A2ND_IMM_OI_INIT",	/* ImmOi Initialization */
	"IMMND_EVT_A2ND_IMM_FINALIZE",	/* ImmOm finalization */
	"IMMND_EVT_A2ND_IMM_OI_FINALIZE",	/* ImmOi finalization */
	"IMMND_EVT_A2ND_IMM_ADMINIT",	/* AdminOwnerInitialize */
	"IMMND_EVT_A2ND_ADMO_FINALIZE",	/* AdminOwnerFinalize */
	"IMMND_EVT_A2ND_ADMO_SET",	/* AdminOwnerSet */
	"IMMND_EVT_A2ND_ADMO_RELEASE",	/* AdminOwnerRelease */
	"IMMND_EVT_A2ND_ADMO_CLEAR",	/* AdminOwnerClear */
	"IMMND_EVT_A2ND_IMM_ADMOP",	/* Syncronous AdminOp */
	"IMMND_EVT_A2ND_IMM_ADMOP_ASYNC",	/* Asyncronous AdminOp */
	"IMMND_EVT_A2ND_IMM_FEVS",	/* Fake EVS msg from Agent (forward) */
	"IMMND_EVT_A2ND_CCBINIT",	/* CcbInitialize */
	"IMMND_EVT_A2ND_SEARCHINIT",	/* SearchInitialize */
	"IMMND_EVT_A2ND_SEARCHNEXT",	/* SearchNext */
	"IMMND_EVT_A2ND_SEARCHFINALIZE",	/* SearchFinalize */
	"IMMND_EVT_A2ND_SEARCH_REMOTE",	/* forward fetch of pure rt attr vals */
	"IMMND_EVT_A2ND_RT_ATT_UPPD_RSP",	/* reply for fetch of pure rt attr vals */
	"IMMND_EVT_A2ND_ADMOP_RSP",	/* AdminOperation sync local Reply */
	"IMMND_EVT_A2ND_ASYNC_ADMOP_RSP",	/* AdminOperation async local Reply */
	"IMMND_EVT_A2ND_CCB_COMPLETED_RSP",	/* CcbCompleted local Reply */
	"IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP",	/*CcbObjCreate local Reply */
	"IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP",	/*CcbObjModify local Reply */
	"IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP",	/*CcbObjDelete local Reply */
	"IMMND_EVT_A2ND_CLASS_CREATE",	/* saImmOmClassCreate */
	"IMMND_EVT_A2ND_CLASS_DESCR_GET",	/* saImmOmClassDescriptionGet */
	"IMMND_EVT_A2ND_CLASS_DELETE",	/* saImmOmClassDelete */
	"IMMND_EVT_A2ND_OBJ_CREATE",	/* saImmOmCcbObjectCreate */
	"IMMND_EVT_A2ND_OBJ_MODIFY",	/* saImmOmCcbObjectModify */
	"IMMND_EVT_A2ND_OBJ_DELETE",	/* saImmOmCcbObjectDelete */
	"IMMND_EVT_A2ND_CCB_APPLY",	/* saImmOmCcbApply */
	"IMMND_EVT_A2ND_CCB_FINALIZE",	/* saImmOmCcbFinalize */
	"IMMND_EVT_A2ND_OBJ_SYNC",	/* immsv_sync */
	"IMMND_EVT_A2ND_SYNC_FINALIZE",	/* immsv_finalize_sync */
	"IMMND_EVT_A2ND_OI_OBJ_CREATE",	/* saImmOiRtObjectCreate */
	"IMMND_EVT_A2ND_OI_OBJ_MODIFY",	/* saImmOiRtObjectUpdate */
	"IMMND_EVT_A2ND_OI_OBJ_DELETE",	/* saImmOiRtObjectDelete */
	"IMMND_EVT_A2ND_OI_IMPL_SET",	/* saImmOiImplementerSet */
	"IMMND_EVT_A2ND_OI_IMPL_CLR",	/* saImmOiImplementerClear */
	"IMMND_EVT_A2ND_OI_CL_IMPL_SET",	/* saImmOiClassImplementerSet */
	"IMMND_EVT_A2ND_OI_CL_IMPL_REL",	/* saImmOiClassImplementerRelease */
	"IMMND_EVT_A2ND_OI_OBJ_IMPL_SET",	/* saImmOiObjectImplementerSet */
	"IMMND_EVT_A2ND_OI_OBJ_IMPL_REL",	/* saImmOiObjectImplementerRelease */
	"IMMND_EVT_ND2ND_ADMOP_RSP",	/* AdminOperation sync fevs Reply */
	"IMMND_EVT_ND2ND_ASYNC_ADMOP_RSP",	/* AdminOperation async fevs Reply */
	"IMMND_EVT_ND2ND_SYNC_FINALIZE",	/* Sync finalize from coord over fevs */
	"IMMND_EVT_ND2ND_SEARCH_REMOTE",	/* Forward of search request ro impl ND */
	"IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP",	/* Forward of search request ro impl ND */
	"IMMND_EVT_D2ND_INTRO_RSP",
	"IMMND_EVT_D2ND_SYNC_REQ",
	"IMMND_EVT_D2ND_LOADING_OK",
	"IMMND_EVT_D2ND_SYNC_START",
	"IMMND_EVT_D2ND_SYNC_ABORT",
	"IMMND_EVT_D2ND_DUMP_OK",
	"IMMND_EVT_D2ND_GLOB_FEVS_REQ",	/* Fake EVS msg from director (consume) */
	"IMMND_EVT_D2ND_ADMINIT",	/* Admin Owner init reply */
	"IMMND_EVT_D2ND_CCBINIT",	/* Ccb init reply */
	"IMMND_EVT_D2ND_IMPLSET_RSP",	/* Implementer set reply from D with impl id */
	"IMMND_EVT_D2ND_DISCARD_IMPL",	/* Discard implementer broadcast to NDs */
	"IMMND_EVT_D2ND_ABORT_CCB",	/* Abort CCB broadcast to NDs */
	"IMMND_EVT_D2ND_ADMO_HARD_FINALIZE",	/* Admo hard finalize broadcast to NDs */
	"IMMND_EVT_D2ND_DISCARD_NODE",	/* Crashed IMMND or node, broadcast to NDs */
	"IMMND_EVT_D2ND_RESET",	/* Restart all IMMNDs. */
	"IMMND_EVT_CB_DUMP",
	"IMMND_EVT_A2ND_IMM_OM_RESURRECT",
	"IMMND_EVT_A2ND_IMM_OI_RESURRECT",
	"IMMND_EVT_A2ND_IMM_CLIENTHIGH",
	"IMMND_EVT_ND2ND_SYNC_FINALIZE_2",
	"IMMND_EVT_A2ND_RECOVER_CCB_OUTCOME",
	"IMMND_EVT_A2ND_PBE_PRT_OBJ_CREATE_RSP",
	"IMMND_EVT_D2ND_PBE_PRTO_PURGE_MUTATIONS",
	"IMMND_EVT_A2ND_PBE_PRTO_DELETES_COMPLETED_RSP",
	"IMMND_EVT_A2ND_PBE_PRT_ATTR_UPDATE_RSP",
	"IMMND_EVT_A2ND_IMM_OM_CLIENTHIGH",
	"IMMND_EVT_A2ND_IMM_OI_CLIENTHIGH",
	"IMMND_EVT_A2ND_PBE_ADMOP_RSP"
};

static const char *immsv_get_immnd_evt_name(unsigned int id)
{
	if (id < IMMND_EVT_MAX)
		return immnd_evt_names[id];
	else
		return "unknown";
}

void immsv_evt_enc_inline_string(NCS_UBAID *o_ub, IMMSV_OCTET_STRING *os)
{
	if (os->size) {
		ncs_encode_n_octets_in_uba(o_ub, (uns8 *)os->buf, os->size);
	}
}

void immsv_evt_dec_inline_string(NCS_UBAID *i_ub, IMMSV_OCTET_STRING *os)
{
	if (os->size) {
		os->buf = (void *)calloc(1, os->size);

		ncs_decode_n_octets_from_uba(i_ub, (uns8 *)os->buf, os->size);
	} else {
		os->buf = NULL;
	}
}

static void immsv_evt_enc_att_val(NCS_UBAID *o_ub, IMMSV_EDU_ATTR_VAL *v, SaImmValueTypeT t)
{
	uns8 *p8;
	uns32 helper32;
	uns64 helper64;
	void* anonymous;
	IMMSV_OCTET_STRING *os = NULL;

	switch (t) {
	case SA_IMM_ATTR_SANAMET:
		assert(v->val.x.size <= SA_MAX_NAME_LENGTH);
		/* Intentional fall through */
	case SA_IMM_ATTR_SASTRINGT:
		/* Intentional fall through */
	case SA_IMM_ATTR_SAANYT:
		os = &(v->val.x);
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, os->size);
		ncs_enc_claim_space(o_ub, 4);
		immsv_evt_enc_inline_string(o_ub, os);
		break;
	case SA_IMM_ATTR_SAINT32T:
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, v->val.saint32);
		ncs_enc_claim_space(o_ub, 4);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, v->val.sauint32);
		ncs_enc_claim_space(o_ub, 4);
		break;
	case SA_IMM_ATTR_SAINT64T:
		p8 = ncs_enc_reserve_space(o_ub, 8);
		ncs_encode_64bit(&p8, v->val.saint64);
		ncs_enc_claim_space(o_ub, 8);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		p8 = ncs_enc_reserve_space(o_ub, 8);
		ncs_encode_64bit(&p8, v->val.sauint64);
		ncs_enc_claim_space(o_ub, 8);
		break;
	case SA_IMM_ATTR_SATIMET:
		p8 = ncs_enc_reserve_space(o_ub, 8);
		ncs_encode_64bit(&p8, v->val.satime);
		ncs_enc_claim_space(o_ub, 8);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		//helper32 = *((uns32 *)&(v->val.safloat));
		anonymous = &(v->val.safloat);
		helper32 = *((uns32*) anonymous);
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, helper32);
		ncs_enc_claim_space(o_ub, 4);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		//helper64 = *((uns64 *)&(v->val.sadouble));
		anonymous = &(v->val.sadouble);
		helper64 = *((uns64 *) anonymous);
		p8 = ncs_enc_reserve_space(o_ub, 8);
		ncs_encode_64bit(&p8, helper64);
		ncs_enc_claim_space(o_ub, 8);
		break;

	default:
		LOG_ER("Unrecognized IMM value type: %u", t);
		assert(0);
	}
}

static void immsv_evt_dec_att_val(NCS_UBAID *i_ub, IMMSV_EDU_ATTR_VAL *v, SaImmValueTypeT t)
{
	uns8 *p8;
	uns32 helper32;
	uns64 helper64;
	uns8 local_data[8];
	void* anonymous;
	IMMSV_OCTET_STRING *os = NULL;

	switch (t) {
	case SA_IMM_ATTR_SANAMET:
		/*
		   assert((*opp)->paramBuffer.val.x.size <= SA_MAX_NAME_LENGTH);
		   Assume correct encoding
		 */
	case SA_IMM_ATTR_SASTRINGT:
	case SA_IMM_ATTR_SAANYT:
		os = &(v->val.x);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);
		break;
	case SA_IMM_ATTR_SAINT32T:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		v->val.saint32 = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		v->val.sauint32 = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		break;
	case SA_IMM_ATTR_SAINT64T:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		v->val.saint64 = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		v->val.sauint64 = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);
		break;
	case SA_IMM_ATTR_SATIMET:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		v->val.satime = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		helper32 = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		//v->val.safloat = *((float *)&helper32);
		anonymous = &helper32;
		v->val.safloat = *((float *) anonymous);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		helper64 = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);
		//v->val.sadouble = *((double *)&helper64);
		anonymous = &helper64;
		v->val.sadouble = *((double *) anonymous);
		break;

	default:
		LOG_ER("Unrecognized IMM value type: %u", t);
		assert(0);
	}
}

void immsv_evt_free_att_val(IMMSV_EDU_ATTR_VAL *p, SaImmValueTypeT t)
{
	switch (t) {
	case SA_IMM_ATTR_SAINT32T:
	case SA_IMM_ATTR_SAUINT32T:
	case SA_IMM_ATTR_SAINT64T:
	case SA_IMM_ATTR_SAUINT64T:
	case SA_IMM_ATTR_SATIMET:
	case SA_IMM_ATTR_SAFLOATT:
	case SA_IMM_ATTR_SADOUBLET:
		return;

	case SA_IMM_ATTR_SANAMET:
	case SA_IMM_ATTR_SASTRINGT:
	case SA_IMM_ATTR_SAANYT:
		if (p->val.x.size) {
			free(p->val.x.buf);
			p->val.x.buf = NULL;
			p->val.x.size = 0;
		}
		return;

	default:
		TRACE_1("Incorrect value for SaImmValueTypeT:%u. "
			"Did you forget to set the attrValueType member in a " "SaImmAttrValuesT value ?", t);
		assert(0);
	}
}

static void immsv_evt_enc_attr_def(NCS_UBAID *o_ub, IMMSV_ATTR_DEF_LIST *ad)
{
	if (!ad)
		return;
	uns8 *p8;

	IMMSV_OCTET_STRING *os = &(ad->d.attrName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, ad->d.attrValueType);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 8);
	ncs_encode_64bit(&p8, ad->d.attrFlags);
	ncs_enc_claim_space(o_ub, 8);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, ad->d.attrNtfId);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (ad->d.attrDefaultValue) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
	if (ad->d.attrDefaultValue) {
		immsv_evt_enc_att_val(o_ub, ad->d.attrDefaultValue, ad->d.attrValueType);
	}
	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (ad->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

static void immsv_evt_enc_attr_mod(NCS_UBAID *o_ub, IMMSV_ATTR_MODS_LIST *p)
{
	if (!p)
		return;
	uns8 *p8;
	uns32 attrValuesNumber = p->attrValue.attrValuesNumber;

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, p->attrModType);
	ncs_enc_claim_space(o_ub, 4);

	IMMSV_OCTET_STRING *os = &(p->attrValue.attrName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, p->attrValue.attrValueType);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, attrValuesNumber);
	ncs_enc_claim_space(o_ub, 4);

	if (attrValuesNumber) {
		immsv_evt_enc_att_val(o_ub, &(p->attrValue.attrValue), p->attrValue.attrValueType);
		--attrValuesNumber;

		IMMSV_EDU_ATTR_VAL_LIST *al = p->attrValue.attrMoreValues;
		while (al && attrValuesNumber) {
			immsv_evt_enc_att_val(o_ub, &(al->n), p->attrValue.attrValueType);
			al = al->next;
			--attrValuesNumber;
		}
	}
	assert(!attrValuesNumber);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (p->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

static void immsv_evt_enc_attribute(NCS_UBAID *o_ub, IMMSV_ATTR_VALUES_LIST *p)
{
	if (!p)
		return;
	uns8 *p8;
	uns32 attrValuesNumber = p->n.attrValuesNumber;

	/*TRACE_2("attribute:%s values:%u", p->n.attrName.buf, attrValuesNumber); */

	IMMSV_OCTET_STRING *os = &(p->n.attrName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, p->n.attrValueType);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, attrValuesNumber);
	ncs_enc_claim_space(o_ub, 4);

	if (attrValuesNumber) {
		immsv_evt_enc_att_val(o_ub, &(p->n.attrValue), p->n.attrValueType);
		--attrValuesNumber;

		IMMSV_EDU_ATTR_VAL_LIST *al = p->n.attrMoreValues;
		while (al && attrValuesNumber) {
			immsv_evt_enc_att_val(o_ub, &(al->n), p->n.attrValueType);
			al = al->next;
			--attrValuesNumber;
		}
	}
	assert(!attrValuesNumber);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (p->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

void immsv_free_attr_list(IMMSV_EDU_ATTR_VAL_LIST *al, const SaImmValueTypeT avt)
{
	while (al) {
		IMMSV_EDU_ATTR_VAL_LIST *tmp = al;
		al = al->next;
		tmp->next = NULL;
		immsv_evt_free_att_val(&(tmp->n), avt);
		free(tmp);
	}
}

void immsv_free_attrmods(IMMSV_ATTR_MODS_LIST *p)
{
	while (p) {
		IMMSV_ATTR_MODS_LIST *tmp = p;
		p = p->next;
		tmp->next = NULL;
		free(tmp->attrValue.attrName.buf);
		tmp->attrValue.attrName.buf = NULL;
		tmp->attrValue.attrName.size = 0;
		if (tmp->attrValue.attrValuesNumber) {
			immsv_evt_free_att_val(&(tmp->attrValue.attrValue), tmp->attrValue.attrValueType);
			if (tmp->attrValue.attrValuesNumber > 1) {
				immsv_free_attr_list(tmp->attrValue.attrMoreValues, tmp->attrValue.attrValueType);
			}
		}
		free(tmp);
	}
}

void immsv_free_attrvalues_list(IMMSV_ATTR_VALUES_LIST *avl)
{
	/*TRACE_ENTER(); */
	while (avl) {
		IMMSV_ATTR_VALUES_LIST *tmp = avl;
		avl = avl->next;
		tmp->next = NULL;
		free(tmp->n.attrName.buf);
		tmp->n.attrName.buf = NULL;
		tmp->n.attrName.size = 0;
		if (tmp->n.attrValuesNumber) {
			immsv_evt_free_att_val(&(tmp->n.attrValue), tmp->n.attrValueType);
			if (tmp->n.attrValuesNumber > 1) {
				immsv_free_attr_list(tmp->n.attrMoreValues, tmp->n.attrValueType);
			}
		}
		free(tmp);
	}
	/*TRACE_LEAVE(); */
}

static void immsv_evt_dec_attrmods(NCS_UBAID *i_ub, IMMSV_ATTR_MODS_LIST **p)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*p = (IMMSV_ATTR_MODS_LIST *)calloc(1, sizeof(IMMSV_ATTR_MODS_LIST));

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->attrModType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		IMMSV_OCTET_STRING *os = &((*p)->attrValue.attrName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->attrValue.attrValueType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->attrValue.attrValuesNumber = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		uns32 attrValuesNumber = (*p)->attrValue.attrValuesNumber;

		if (attrValuesNumber) {
			immsv_evt_dec_att_val(i_ub, &((*p)->attrValue.attrValue), (*p)->attrValue.attrValueType);
			--attrValuesNumber;

			IMMSV_EDU_ATTR_VAL_LIST *al = NULL;

			while (attrValuesNumber) {
				al = (IMMSV_EDU_ATTR_VAL_LIST *)
				    calloc(1, sizeof(IMMSV_EDU_ATTR_VAL_LIST));

				immsv_evt_dec_att_val(i_ub, &(al->n), (*p)->attrValue.attrValueType);

				al->next = (*p)->attrValue.attrMoreValues;	/*NULL initially */
				(*p)->attrValue.attrMoreValues = al;
				--attrValuesNumber;
			}
		}

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		p = &((*p)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ATTRIBUTES));

	if (depth >= IMMSV_MAX_ATTRIBUTES) {
		LOG_ER("TOO MANY attribute modifications line:%u", __LINE__);
		assert(depth < IMMSV_MAX_ATTRIBUTES);
	}
}

static void immsv_evt_dec_attributes(NCS_UBAID *i_ub, IMMSV_ATTR_VALUES_LIST **p)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*p = (IMMSV_ATTR_VALUES_LIST *)
		    calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));

		IMMSV_OCTET_STRING *os = &((*p)->n.attrName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->n.attrValueType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->n.attrValuesNumber = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		uns32 attrValuesNumber = (*p)->n.attrValuesNumber;

		if (attrValuesNumber) {
			immsv_evt_dec_att_val(i_ub, &((*p)->n.attrValue), (*p)->n.attrValueType);
			--attrValuesNumber;

			IMMSV_EDU_ATTR_VAL_LIST *al = NULL;

			while (attrValuesNumber) {
				al = (IMMSV_EDU_ATTR_VAL_LIST *)
				    calloc(1, sizeof(IMMSV_EDU_ATTR_VAL_LIST));

				immsv_evt_dec_att_val(i_ub, &(al->n), (*p)->n.attrValueType);

				al->next = (*p)->n.attrMoreValues;	/*NULL initially */
				(*p)->n.attrMoreValues = al;
				--attrValuesNumber;
			}
		}

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		p = &((*p)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ATTRIBUTES));

	if (depth >= IMMSV_MAX_ATTRIBUTES) {
		LOG_ER("TOO MANY attributes line:%u", __LINE__);
		assert(depth < IMMSV_MAX_ATTRIBUTES);
	}
}

static uns32 immsv_evt_enc_name_list(NCS_UBAID *o_ub, IMMSV_OBJ_NAME_LIST *p)
{
	uns8 *p8;
	uns16 objs = 0;

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, p ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);

	while (p && (objs < IMMSV_MAX_OBJECTS)) {
		++objs;
		IMMSV_OCTET_STRING *os = &(p->name);
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, os->size);
		ncs_enc_claim_space(o_ub, 4);
		immsv_evt_enc_inline_string(o_ub, os);

		p = p->next;
		p8 = ncs_enc_reserve_space(o_ub, 1);
		ncs_encode_8bit(&p8, p ? 1 : 0);
		ncs_enc_claim_space(o_ub, 1);
	}

	if (objs >= IMMSV_MAX_OBJECTS) {
		LOG_ER("TOO MANY Object Names line:%u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}
	return NCSCC_RC_SUCCESS;
}

static uns32 immsv_evt_dec_name_list(NCS_UBAID *i_ub, IMMSV_OBJ_NAME_LIST **p)
{
	int depth = 1;
	uns8 *p8;
	uns8 local_data[8];
	uns8 c8;
	/*TRACE_ENTER(); */

	p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
	c8 = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(i_ub, 1);

	while (c8 && (depth < IMMSV_MAX_OBJECTS)) {

		*p = (IMMSV_OBJ_NAME_LIST *)calloc(1, sizeof(IMMSV_OBJ_NAME_LIST));

		IMMSV_OCTET_STRING *os = &((*p)->name);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		p = &((*p)->next);
		++depth;
	}

	if (depth >= IMMSV_MAX_OBJECTS) {
		LOG_ER("TOO MANY Object Names line:%u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}
	return NCSCC_RC_SUCCESS;
	/*TRACE_LEAVE(); */
}

void immsv_evt_free_name_list(IMMSV_OBJ_NAME_LIST *p)
{
	while (p) {
		IMMSV_OBJ_NAME_LIST *tmp = p;
		p = p->next;
		tmp->next = NULL;
		free(tmp->name.buf);
		tmp->name.buf = NULL;
		tmp->name.size = 0;
		free(tmp);
	}
}

static void immsv_evt_enc_class(NCS_UBAID *o_ub, IMMSV_CLASS_LIST *r)
{
	if (!r)
		return;
	uns8 *p8;

	IMMSV_OCTET_STRING *os = &(r->className);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	os = &(r->classImplName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, r->nrofInstances);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (r->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

static uns32 immsv_evt_dec_class(NCS_UBAID *i_ub, IMMSV_CLASS_LIST **r)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*r = (IMMSV_CLASS_LIST *)calloc(1, sizeof(IMMSV_CLASS_LIST));

		IMMSV_OCTET_STRING *os = &((*r)->className);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		os = &((*r)->classImplName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*r)->nrofInstances = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);

		r = &((*r)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_CLASSES));

	if (depth >= IMMSV_MAX_CLASSES) {
		LOG_ER("TOO MANY classes line: %u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}

	return NCSCC_RC_SUCCESS;
}

void immsv_evt_free_classList(IMMSV_CLASS_LIST *p)
{
	while (p) {
		IMMSV_CLASS_LIST *tmp = p;
		p = p->next;
		tmp->next = NULL;
		free(tmp->className.buf);
		tmp->className.buf = NULL;
		tmp->className.size = 0;
		free(tmp->classImplName.buf);
		tmp->classImplName.buf = NULL;
		tmp->classImplName.size = 0;
		free(tmp);
	}
}

void immsv_evt_free_ccbOutcomeList(IMMSV_CCB_OUTCOME_LIST *o)
{
	while(o) {
		IMMSV_CCB_OUTCOME_LIST *tmp = o;
		o = o->next;
		tmp->next =NULL;
		free(tmp);
	}
}

static void immsv_evt_enc_impl(NCS_UBAID *o_ub, IMMSV_IMPL_LIST *q)
{
	if (!q)
		return;
	uns8 *p8;

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, q->id);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, q->nodeId);
	ncs_enc_claim_space(o_ub, 4);

	IMMSV_OCTET_STRING *os = &(q->implementerName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 8);
	ncs_encode_64bit(&p8, q->mds_dest);
	ncs_enc_claim_space(o_ub, 8);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (q->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

static uns32 immsv_evt_dec_impl(NCS_UBAID *i_ub, IMMSV_IMPL_LIST **q)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*q = (IMMSV_IMPL_LIST *)calloc(1, sizeof(IMMSV_IMPL_LIST));

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*q)->id = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*q)->nodeId = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		IMMSV_OCTET_STRING *os = &((*q)->implementerName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		(*q)->mds_dest = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);

		q = &((*q)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_IMPLEMENTERS));

	if (depth >= IMMSV_MAX_IMPLEMENTERS) {
		LOG_ER("TOO MANY implementers line:%u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}

	return NCSCC_RC_SUCCESS;
}

void immsv_evt_free_impl(IMMSV_IMPL_LIST *q)
{
	while (q) {
		IMMSV_IMPL_LIST *tmp = q;
		q = q->next;
		tmp->next = NULL;
		free(tmp->implementerName.buf);
		tmp->implementerName.buf = NULL;
		tmp->implementerName.size = 0;
		free(tmp);
	}
}

static uns32 immsv_evt_enc_admo(NCS_UBAID *o_ub, IMMSV_ADMO_LIST *p)
{
	if (!p)
		return NCSCC_RC_SUCCESS;
	uns8 *p8;
	uns32 rc = NCSCC_RC_SUCCESS;

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, p->id);
	ncs_enc_claim_space(o_ub, 4);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, p->nodeId);
	ncs_enc_claim_space(o_ub, 4);

	IMMSV_OCTET_STRING *os = &(p->adminOwnerName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (p->releaseOnFinalize) ? ((p->isDying)? 0x3 : 0x1) : 0x0);
	ncs_enc_claim_space(o_ub, 1);
	if(p->isDying) {TRACE("immsv_evt_enc_admo isDying == TRUE encoded");}

	rc = immsv_evt_enc_name_list(o_ub, p->touchedObjects);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (p->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);

	return rc;
}

static uns32 immsv_evt_dec_admo(NCS_UBAID *i_ub, IMMSV_ADMO_LIST **p)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*p = (IMMSV_ADMO_LIST *)calloc(1, sizeof(IMMSV_ADMO_LIST));

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->id = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*p)->nodeId = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		IMMSV_OCTET_STRING *os = &((*p)->adminOwnerName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		(*p)->releaseOnFinalize = (c8 & ((uint8_t) 0x1));
		(*p)->isDying = (c8 & ((uint8_t) 0x2))?0x1:0x0;
		if((*p)->isDying) {TRACE("immsv_evt_dec_admo isDying == TRUE decoded");}

		immsv_evt_dec_name_list(i_ub, &((*p)->touchedObjects));

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);

		p = &((*p)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ADMINOWNERS));

	if (depth >= IMMSV_MAX_ADMINOWNERS) {
		LOG_ER("TOO MANY Admin Owners");
		return NCSCC_RC_OUT_OF_MEM;
	}

	return NCSCC_RC_SUCCESS;
}

void immsv_evt_free_admo(IMMSV_ADMO_LIST *p)
{
	while (p) {
		IMMSV_ADMO_LIST *tmp = p;
		p = p->next;
		tmp->next = NULL;

		free(tmp->adminOwnerName.buf);
		tmp->adminOwnerName.buf = NULL;
		tmp->adminOwnerName.size = 0;
		IMMSV_OBJ_NAME_LIST *onl = tmp->touchedObjects;
		while (onl) {
			IMMSV_OBJ_NAME_LIST *onlTmp = onl;
			onl = onl->next;
			onlTmp->next = NULL;
			free(onlTmp->name.buf);
			onlTmp->name.buf = NULL;
			onlTmp->name.size = 0;
			free(onlTmp);
		}
		tmp->touchedObjects = NULL;
		free(tmp);
	}
}

static void immsv_evt_enc_attrName(NCS_UBAID *o_ub, IMMSV_ATTR_NAME_LIST *p)
{
	if (!p)
		return;
	uns8 *p8;

	IMMSV_OCTET_STRING *os = &(p->name);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (p->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

static uns32 immsv_evt_dec_attrNames(NCS_UBAID *i_ub, IMMSV_ATTR_NAME_LIST **p)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*p = (IMMSV_ATTR_NAME_LIST *)calloc(1, sizeof(IMMSV_ATTR_NAME_LIST));

		IMMSV_OCTET_STRING *os = &((*p)->name);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		p = &((*p)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ATTRIBUTES));

	if (depth >= IMMSV_MAX_ATTRIBUTES) {
		LOG_ER("TOO MANY attrNames");
		return NCSCC_RC_OUT_OF_MEM;
	}
	return NCSCC_RC_SUCCESS;
}

void immsv_evt_free_attrNames(IMMSV_ATTR_NAME_LIST *p)
{
	while (p) {
		IMMSV_ATTR_NAME_LIST *tmp = p;
		p = p->next;
		tmp->next = NULL;
		free(tmp->name.buf);
		tmp->name.buf = NULL;
		tmp->name.size = 0;
		free(tmp);
	}
}

static void immsv_evt_enc_admop_param(NCS_UBAID *o_ub, IMMSV_ADMIN_OPERATION_PARAM *op)
{
	if (!op)
		return;
	uns8 *p8;

	IMMSV_OCTET_STRING *os = &(op->paramName);
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, os->size);
	ncs_enc_claim_space(o_ub, 4);
	immsv_evt_enc_inline_string(o_ub, os);

	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, op->paramType);
	ncs_enc_claim_space(o_ub, 4);

	immsv_evt_enc_att_val(o_ub, &(op->paramBuffer), op->paramType);

	p8 = ncs_enc_reserve_space(o_ub, 1);
	ncs_encode_8bit(&p8, (op->next) ? 1 : 0);
	ncs_enc_claim_space(o_ub, 1);
}

void immsv_free_attrdefs_list(IMMSV_ATTR_DEF_LIST *adp)
{
	while (adp) {
		IMMSV_ATTR_DEF_LIST *tmp = adp;
		adp = adp->next;
		tmp->next = NULL;
		free(tmp->d.attrName.buf);	/*free-3 */
		tmp->d.attrName.buf = NULL;
		tmp->d.attrName.size = 0;
		if (tmp->d.attrDefaultValue) {
			immsv_evt_free_att_val(tmp->d.attrDefaultValue,	/*free-4.2 */
					       tmp->d.attrValueType);
			free(tmp->d.attrDefaultValue);	/*free-4.1 */
			tmp->d.attrDefaultValue = NULL;
		}
		free(tmp);	/*free-2 */
	}
}

static uns32 immsv_evt_dec_attr_def(NCS_UBAID *i_ub, IMMSV_ATTR_DEF_LIST **adp)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*adp = (IMMSV_ATTR_DEF_LIST *)calloc(1, sizeof(IMMSV_ATTR_DEF_LIST));

		IMMSV_OCTET_STRING *os = &((*adp)->d.attrName);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*adp)->d.attrValueType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
		(*adp)->d.attrFlags = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(i_ub, 8);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*adp)->d.attrNtfId = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);

		if (c8) {
			(*adp)->d.attrDefaultValue = (IMMSV_EDU_ATTR_VAL *)calloc(1, sizeof(IMMSV_EDU_ATTR_VAL));

			immsv_evt_dec_att_val(i_ub, (*adp)->d.attrDefaultValue, (*adp)->d.attrValueType);
		}

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		adp = &((*adp)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ATTRIBUTES));
	if (depth >= IMMSV_MAX_ATTRIBUTES) {
		LOG_ER("TOO MANY ATTR DEFS line:%u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}
	return NCSCC_RC_SUCCESS;
}

static uns32 immsv_evt_dec_admop_param(NCS_UBAID *i_ub, IMMSV_ADMIN_OPERATION_PARAM **opp)
{
	int depth = 1;
	uns8 c8;

	do {
		uns8 *p8;
		uns8 local_data[8];

		*opp = (IMMSV_ADMIN_OPERATION_PARAM *)
		    calloc(1, sizeof(IMMSV_ADMIN_OPERATION_PARAM));

		IMMSV_OCTET_STRING *os = &((*opp)->paramName);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		os->size = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);
		immsv_evt_dec_inline_string(i_ub, os);
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		(*opp)->paramType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		/*Then decode the potentially variable length part */
		immsv_evt_dec_att_val(i_ub, &((*opp)->paramBuffer), (*opp)->paramType);

		p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
		c8 = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(i_ub, 1);
		opp = &((*opp)->next);
		++depth;
	} while (c8 && (depth < IMMSV_MAX_ATTRIBUTES));
	if (depth >= IMMSV_MAX_ATTRIBUTES) {
		LOG_ER("TOO MANY PARAMS line:%u", __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_enc_sublevels.

 DESCRIPTION    : Encodes the sublevels of IMMSV_EVT into userbuf
                  Sublevels are encoded with byter order correction and
                  used by both encode_flat and encode (because we dont 
                  really save much space/execution by encoding the bits
                  and pieces of the sub-level as flat bits and pieces).

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
static uns32 immsv_evt_enc_sublevels(IMMSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	/* Encode the internal Pointers always using byte-order correction. */
	if (i_evt->type == IMMSV_EVT_TYPE_IMMA) {
		if (i_evt->info.imma.type == IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP) {
			int depth = 0;
			/*Encode attribute definitions. */
			IMMSV_ATTR_DEF_LIST *ad = i_evt->info.imma.info.classDescr.attrDefinitions;
			while (ad && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attr_def(o_ub, ad);
				ad = ad->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("Class description: TOO MANY ATTR DEFS");
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.imma.type == IMMA_EVT_ND2A_SEARCHNEXT_RSP) {
			uns8 *p8;
			int depth = 0;
			/*Encode searchNext response */
			IMMSV_OM_RSP_SEARCH_NEXT *sn = i_evt->info.imma.info.searchNextRsp;
			assert(sn);

			IMMSV_OCTET_STRING *os = &(sn->objectName);
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, os->size);
			ncs_enc_claim_space(o_ub, 4);
			immsv_evt_enc_inline_string(o_ub, os);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (sn->attrValuesList) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			if (sn->attrValuesList) {
				IMMSV_ATTR_VALUES_LIST *al = sn->attrValuesList;
				do {
					immsv_evt_enc_attribute(o_ub, al);
					al = al->next;
					++depth;
				} while (al && (depth < IMMSV_MAX_ATTRIBUTES));
				if (depth >= IMMSV_MAX_ATTRIBUTES) {
					LOG_ER("Search next: TOO MANY ATTR DEFS line:%u", __LINE__);
					return NCSCC_RC_OUT_OF_MEM;
				}
			}
			/*
			   else 
			   { No attrValues }
			 */
		} else if (i_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_CREATE_UC) {
			int depth = 0;
			/*Encode the className */
			IMMSV_OCTET_STRING *os = &(i_evt->info.imma.info.objCreate.className);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the parentName */
			os = &(i_evt->info.imma.info.objCreate.parentName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = i_evt->info.imma.info.objCreate.attrValues;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attribute(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attributes line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC) {
			int depth = 0;

			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.imma.info.objModify.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = i_evt->info.imma.info.objModify.attrMods;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attr_mod(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute modifications line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_DELETE_UC) {
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.imma.info.objDelete.objectName);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if ((i_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ADMOP) ||
			(i_evt->info.imma.type == IMMA_EVT_ND2A_IMM_PBE_ADMOP)) {
			int depth = 0;
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.imma.info.admOpReq.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the param list */
			IMMSV_ADMIN_OPERATION_PARAM *op = i_evt->info.imma.info.admOpReq.params;
			while (op && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_admop_param(o_ub, op);
				op = op->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY PARAMS line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.imma.type == IMMA_EVT_ND2A_SEARCH_REMOTE) {
			int depth = 0;
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.imma.info.searchRemote.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = i_evt->info.imma.info.searchRemote.attributeNames;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attrName(o_ub, p);
				p = p->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute names line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		}
	} else if (i_evt->type == IMMSV_EVT_TYPE_IMMD) {
		if (i_evt->info.immd.type == IMMD_EVT_ND2D_FEVS_REQ) {
			IMMSV_OCTET_STRING *os = &(i_evt->info.immd.info.fevsReq.msg);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if (i_evt->info.immd.type == IMMD_EVT_ND2D_IMPLSET_REQ) {
			IMMSV_OCTET_STRING *os = &(i_evt->info.immd.info.impl_set.r.impl_name);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if (i_evt->info.immd.type == IMMD_EVT_ND2D_OI_OBJ_MODIFY) {
			int depth = 0;

			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immd.info.objModify.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = i_evt->info.immd.info.objModify.attrMods;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attr_mod(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute modifications line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		}
	} else if (i_evt->type == IMMSV_EVT_TYPE_IMMND) {
		if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_FEVS) ||
		    (i_evt->info.immnd.type == IMMND_EVT_D2ND_GLOB_FEVS_REQ)) {
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.fevsReq.msg);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_IMPL_SET) ||
			   (i_evt->info.immnd.type == IMMND_EVT_D2ND_IMPLSET_RSP) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_CL_IMPL_SET) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_CL_IMPL_REL) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_IMPL_SET) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_IMPL_REL)) {
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.implSet.impl_name);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP_ASYNC)) {
			int depth = 0;
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.admOpReq.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the param list */
			IMMSV_ADMIN_OPERATION_PARAM *op = i_evt->info.immnd.info.admOpReq.params;
			while (op && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_admop_param(o_ub, op);
				op = op->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY PARAMS line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}

		} else if (i_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_DELETE) {
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.classDescr.className);
			immsv_evt_enc_inline_string(o_ub, os);
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_CREATE) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_DESCR_GET)) {
			int depth = 0;
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.classDescr.className);

			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode attribute definitions. */
			IMMSV_ATTR_DEF_LIST *ad = i_evt->info.immnd.info.classDescr.attrDefinitions;
			while (ad && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attr_def(o_ub, ad);
				ad = ad->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY ATTR DEFS line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_CREATE) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_CREATE)) {
			int depth = 0;
			/*Encode the className */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.objCreate.className);

			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the parentName */
			os = &(i_evt->info.immnd.info.objCreate.parentName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = i_evt->info.immnd.info.objCreate.attrValues;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attribute(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attributes line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_MODIFY) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_MODIFY)) {
			int depth = 0;

			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.objModify.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = i_evt->info.immnd.info.objModify.attrMods;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attr_mod(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute modifications line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_DELETE) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_DELETE)) {
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.objDelete.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

		} else if (i_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_SYNC) {
			int depth = 0;
			/*Encode the className */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.obj_sync.className);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the objecttName */
			os = &(i_evt->info.immnd.info.obj_sync.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = i_evt->info.immnd.info.obj_sync.attrValues;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attribute(o_ub, p);
				p = p->next;
				++depth;
			}
			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attributes line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if ((i_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_SET) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_RELEASE) ||
			   (i_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_CLEAR)) {
			return immsv_evt_enc_name_list(o_ub, i_evt->info.immnd.info.admReq.objectNames);
		} else if ((i_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE) ||
			(i_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2)) {
			int depth = 0;
			uns8 *p8;

			IMMSV_ADMO_LIST *p = i_evt->info.immnd.info.finSync.adminOwners;
			while (p && (depth < IMMSV_MAX_ADMINOWNERS)) {
				if (immsv_evt_enc_admo(o_ub, p) != NCSCC_RC_SUCCESS) {
					return NCSCC_RC_OUT_OF_MEM;
				}
				p = p->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_ADMINOWNERS) {
				LOG_ER("TOO MANY admin owners line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}

			depth = 0;
			IMMSV_IMPL_LIST *q = i_evt->info.immnd.info.finSync.implementers;

			while (q && (depth < IMMSV_MAX_IMPLEMENTERS)) {
				immsv_evt_enc_impl(o_ub, q);
				q = q->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_IMPLEMENTERS) {
				LOG_ER("TOO MANY implementers line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}

			depth = 0;
			IMMSV_CLASS_LIST *r = i_evt->info.immnd.info.finSync.classes;
			while (r && (depth < IMMSV_MAX_CLASSES)) {
				immsv_evt_enc_class(o_ub, r);
				r = r->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_CLASSES) {
				LOG_ER("TOO MANY classes line: %u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}

			if(i_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2) {
				TRACE("Encoding SUB level for IMMND_EVT_ND2ND_SYNC_FINALIZE_2");
				depth = 1;
				IMMSV_CCB_OUTCOME_LIST* ol = i_evt->info.immnd.info.finSync.ccbResults;
				/*Encode start marker*/
				p8 = ncs_enc_reserve_space(o_ub, 1);
				ncs_encode_8bit(&p8, ol ? 1 : 0);
				ncs_enc_claim_space(o_ub, 1);

				while(ol && (depth < IMMSV_MAX_CCBS)) {
					++depth;
					p8 = ncs_enc_reserve_space(o_ub, 4);
					ncs_encode_32bit(&p8, ol->ccbId);
					ncs_enc_claim_space(o_ub, 4);

					p8 = ncs_enc_reserve_space(o_ub, 4);
					ncs_encode_32bit(&p8, ol->ccbState);
					ncs_enc_claim_space(o_ub, 4);

					ol = ol->next;
					p8 = ncs_enc_reserve_space(o_ub, 1);
					ncs_encode_8bit(&p8, (ol && (depth < IMMSV_MAX_CCBS))? 1 : 0);
					ncs_enc_claim_space(o_ub, 1);
				}
			}
		} else if (i_evt->info.immnd.type == IMMND_EVT_A2ND_SEARCHINIT) {
			int depth = 0;
			/*Encode the rootName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.searchInit.rootName);
			if (os->size) {
				immsv_evt_enc_inline_string(o_ub, os);
			}

			if (i_evt->info.immnd.info.searchInit.searchParam.present ==
			    ImmOmSearchParameter_PR_oneAttrParam) {
				os = &(i_evt->info.immnd.info.searchInit.searchParam.choice.oneAttrParam.attrName);
				if (os->size) {
					immsv_evt_enc_inline_string(o_ub, os);
				}
				immsv_evt_enc_att_val(o_ub,
						      &(i_evt->info.immnd.info.searchInit.searchParam.choice.
							oneAttrParam.attrValue),
						      i_evt->info.immnd.info.searchInit.searchParam.choice.oneAttrParam.
						      attrValueType);
			} else {
				assert(i_evt->info.immnd.info.searchInit.searchParam.present ==
				       ImmOmSearchParameter_PR_NOTHING);
			}

			/*Encode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = i_evt->info.immnd.info.searchInit.attributeNames;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attrName(o_ub, p);
				p = p->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute names line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}

		} else if (i_evt->info.immnd.type == IMMND_EVT_A2ND_RT_ATT_UPPD_RSP) {
			int depth = 0;
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.rtAttUpdRpl.sr.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = i_evt->info.immnd.info.rtAttUpdRpl.sr.attributeNames;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attrName(o_ub, p);
				p = p->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute names line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.immnd.type == IMMND_EVT_ND2ND_SEARCH_REMOTE) {
			int depth = 0;
			/*Encode the objectName */
			IMMSV_OCTET_STRING *os = &(i_evt->info.immnd.info.searchRemote.objectName);
			immsv_evt_enc_inline_string(o_ub, os);

			/*Encode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = i_evt->info.immnd.info.searchRemote.attributeNames;

			while (p && (depth < IMMSV_MAX_ATTRIBUTES)) {
				immsv_evt_enc_attrName(o_ub, p);
				p = p->next;
				++depth;
			}

			if (depth >= IMMSV_MAX_ATTRIBUTES) {
				LOG_ER("TOO MANY attribute names line:%u", __LINE__);
				return NCSCC_RC_OUT_OF_MEM;
			}
		} else if (i_evt->info.immnd.type == IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP) {
			uns8 *p8;
			int depth = 0;
			/*Encode searchRemote response */
			IMMSV_OM_RSP_SEARCH_REMOTE *sr = &(i_evt->info.immnd.info.rspSrchRmte);
			IMMSV_OM_RSP_SEARCH_NEXT *sn = &(sr->runtimeAttrs);

			IMMSV_OCTET_STRING *os = &(sn->objectName);
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, os->size);
			ncs_enc_claim_space(o_ub, 4);
			immsv_evt_enc_inline_string(o_ub, os);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (sn->attrValuesList) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			if (sn->attrValuesList) {
				IMMSV_ATTR_VALUES_LIST *al = sn->attrValuesList;
				do {
					immsv_evt_enc_attribute(o_ub, al);
					al = al->next;
					++depth;
				} while (al && (depth < IMMSV_MAX_ATTRIBUTES));
				if (depth >= IMMSV_MAX_ATTRIBUTES) {
					LOG_ER("Search next: TOO MANY ATTR DEFS line:%u", __LINE__);
					return NCSCC_RC_OUT_OF_MEM;
				}
			}
			/*
			   else 
			   { No attrValues }
			 */
		}
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_dec_sublevel

 DESCRIPTION    : Decodes the sublevels of IMMSV_EVT from user buf
                  Inverse of immsv_evt_enc_sublevel.

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
static uns32 immsv_evt_dec_sublevels(NCS_UBAID *i_ub, IMMSV_EVT *o_evt)
{
	/* Decode the internal pointers */
	if (o_evt->type == IMMSV_EVT_TYPE_IMMA) {
		if (o_evt->info.imma.type == IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP) {
			/*Decode attribute definitons */
			IMMSV_ATTR_DEF_LIST *ad = o_evt->info.imma.info.classDescr.attrDefinitions;
			if (ad) {
				immsv_evt_dec_attr_def(i_ub, &ad);
				o_evt->info.imma.info.classDescr.attrDefinitions = ad;
			}
		} else if (o_evt->info.imma.type == IMMA_EVT_ND2A_SEARCHNEXT_RSP) {
			uns8 *p8;
			uns8 c8;
			uns8 local_data[8];

			/*Decode searchNext Response */
			o_evt->info.imma.info.searchNextRsp = calloc(1, sizeof(IMMSV_OM_RSP_SEARCH_NEXT));

			/* Decode object name */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.searchNextRsp->objectName);
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			os->size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			immsv_evt_dec_inline_string(i_ub, os);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			c8 = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			if (c8) {
				/*Decode the list of attributes */
				IMMSV_ATTR_VALUES_LIST *p = NULL;
				immsv_evt_dec_attributes(i_ub, &p);
				o_evt->info.imma.info.searchNextRsp->attrValuesList = p;
			} else {
				o_evt->info.imma.info.searchNextRsp->attrValuesList = NULL;
			}
		} else if (o_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_CREATE_UC) {
			/*Decode the className */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.objCreate.className);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the parentName */
			os = &(o_evt->info.imma.info.objCreate.parentName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = o_evt->info.imma.info.objCreate.attrValues;
			if (p) {
				immsv_evt_dec_attributes(i_ub, &p);
				o_evt->info.imma.info.objCreate.attrValues = p;
			}
		} else if (o_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.objModify.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = o_evt->info.imma.info.objModify.attrMods;
			if (p) {
				immsv_evt_dec_attrmods(i_ub, &p);
				o_evt->info.imma.info.objModify.attrMods = p;
			}
		} else if (o_evt->info.imma.type == IMMA_EVT_ND2A_OI_OBJ_DELETE_UC) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.objDelete.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

		} else if ((o_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ADMOP) ||
			(o_evt->info.imma.type == IMMA_EVT_ND2A_IMM_PBE_ADMOP)) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.admOpReq.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the param list */
			IMMSV_ADMIN_OPERATION_PARAM *op = o_evt->info.imma.info.admOpReq.params;
			if (op) {
				immsv_evt_dec_admop_param(i_ub, &op);
				o_evt->info.imma.info.admOpReq.params = op;
			}
		} else if (o_evt->info.imma.type == IMMA_EVT_ND2A_SEARCH_REMOTE) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.imma.info.searchRemote.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = o_evt->info.imma.info.searchRemote.attributeNames;
			if (p) {
				immsv_evt_dec_attrNames(i_ub, &p);
				o_evt->info.imma.info.searchRemote.attributeNames = p;
			}
		}
	} else if (o_evt->type == IMMSV_EVT_TYPE_IMMD) {
		if (o_evt->info.immd.type == IMMD_EVT_ND2D_FEVS_REQ) {
			IMMSV_OCTET_STRING *os = &(o_evt->info.immd.info.fevsReq.msg);
			immsv_evt_dec_inline_string(i_ub, os);
		} else if (o_evt->info.immd.type == IMMD_EVT_ND2D_IMPLSET_REQ) {
			IMMSV_OCTET_STRING *os = &(o_evt->info.immd.info.impl_set.r.impl_name);
			immsv_evt_dec_inline_string(i_ub, os);
		} else if (o_evt->info.immd.type == IMMD_EVT_ND2D_OI_OBJ_MODIFY) {

			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immd.info.objModify.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = o_evt->info.immd.info.objModify.attrMods;
			if (p) {
				immsv_evt_dec_attrmods(i_ub, &p);
				o_evt->info.immd.info.objModify.attrMods = p;
			}
		}
	} else if (o_evt->type == IMMSV_EVT_TYPE_IMMND) {
		if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_FEVS) ||
		    (o_evt->info.immnd.type == IMMND_EVT_D2ND_GLOB_FEVS_REQ)) {
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.fevsReq.msg);
			immsv_evt_dec_inline_string(i_ub, os);
		} else
		    if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_IMPL_SET) ||
			(o_evt->info.immnd.type == IMMND_EVT_D2ND_IMPLSET_RSP) ||
			(o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_CL_IMPL_SET) ||
			(o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_CL_IMPL_REL) ||
			(o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_IMPL_SET) ||
			(o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_IMPL_REL)) {
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.implSet.impl_name);
			immsv_evt_dec_inline_string(i_ub, os);
		} else
		    if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP) ||
			(o_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP_ASYNC)) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.admOpReq.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the param list */
			IMMSV_ADMIN_OPERATION_PARAM *op = o_evt->info.immnd.info.admOpReq.params;
			if (op) {
				immsv_evt_dec_admop_param(i_ub, &op);
				o_evt->info.immnd.info.admOpReq.params = op;
			}
		} else if (o_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_DELETE) {
			/*Decode the className */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.classDescr.className);
			immsv_evt_dec_inline_string(i_ub, os);
		} else if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_CREATE) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_CLASS_DESCR_GET)) {
			/*Decode the className */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.classDescr.className);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode attribute definitons */
			IMMSV_ATTR_DEF_LIST *ad = o_evt->info.immnd.info.classDescr.attrDefinitions;
			if (ad) {
				immsv_evt_dec_attr_def(i_ub, &ad);
				o_evt->info.immnd.info.classDescr.attrDefinitions = ad;
			}
		} else if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_CREATE) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_CREATE)) {
			/*Decode the className */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.objCreate.className);
			immsv_evt_dec_inline_string(i_ub, os);

			os = &(o_evt->info.immnd.info.objCreate.parentName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = o_evt->info.immnd.info.objCreate.attrValues;
			if (p) {
				immsv_evt_dec_attributes(i_ub, &p);
				o_evt->info.immnd.info.objCreate.attrValues = p;
			}
		} else if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_MODIFY) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_MODIFY)) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.objModify.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute modifications */
			IMMSV_ATTR_MODS_LIST *p = o_evt->info.immnd.info.objModify.attrMods;
			if (p) {
				immsv_evt_dec_attrmods(i_ub, &p);
				o_evt->info.immnd.info.objModify.attrMods = p;
			}
		} else if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_DELETE) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_OI_OBJ_DELETE)) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.objDelete.objectName);
			immsv_evt_dec_inline_string(i_ub, os);
		} else if (o_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_SYNC) {
			/*Decode the className */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.obj_sync.className);
			immsv_evt_dec_inline_string(i_ub, os);

			os = &(o_evt->info.immnd.info.obj_sync.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attributes */
			IMMSV_ATTR_VALUES_LIST *p = o_evt->info.immnd.info.obj_sync.attrValues;
			if (p) {
				immsv_evt_dec_attributes(i_ub, &p);
				o_evt->info.immnd.info.obj_sync.attrValues = p;
			}
		} else if ((o_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_SET) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_RELEASE) ||
			   (o_evt->info.immnd.type == IMMND_EVT_A2ND_ADMO_CLEAR)) {
			immsv_evt_dec_name_list(i_ub, &(o_evt->info.immnd.info.admReq.objectNames));
		} else if ((o_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE) ||
			(o_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2)) {
			IMMSV_ADMO_LIST *p = o_evt->info.immnd.info.finSync.adminOwners;
			if (p) {
				immsv_evt_dec_admo(i_ub, &p);
				o_evt->info.immnd.info.finSync.adminOwners = p;
			}

			IMMSV_IMPL_LIST *q = o_evt->info.immnd.info.finSync.implementers;
			if (q) {
				immsv_evt_dec_impl(i_ub, &q);
				o_evt->info.immnd.info.finSync.implementers = q;
			}

			IMMSV_CLASS_LIST *r = o_evt->info.immnd.info.finSync.classes;
			if (r) {
				immsv_evt_dec_class(i_ub, &r);
				o_evt->info.immnd.info.finSync.classes = r;
			}

			if (o_evt->info.immnd.type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2) {
				int depth = 1;
				uns8 *p8;
				uns8 local_data[8];
				uns8 c8;
				TRACE("Decoding SUB level for IMMND_EVT_ND2ND_SYNC_FINALIZE_2");
				o_evt->info.immnd.info.finSync.ccbResults = NULL;

				p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
				c8 = ncs_decode_8bit(&p8);
				ncs_dec_skip_space(i_ub, 1);

				while(c8 && (depth < IMMSV_MAX_CCBS)) {
					IMMSV_CCB_OUTCOME_LIST* ol = (IMMSV_CCB_OUTCOME_LIST *) 
						calloc(1, sizeof(IMMSV_CCB_OUTCOME_LIST));

					p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
					ol->ccbId = ncs_decode_32bit(&p8);
					ncs_dec_skip_space(i_ub, 4);
					
					p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
					ol->ccbState = ncs_decode_32bit(&p8);
					ncs_dec_skip_space(i_ub, 4);
					
					ol->next = o_evt->info.immnd.info.finSync.ccbResults;
					o_evt->info.immnd.info.finSync.ccbResults = ol;

					p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
					c8 = ncs_decode_8bit(&p8);
					ncs_dec_skip_space(i_ub, 1);

					++depth;
				}
			}
		} else if (o_evt->info.immnd.type == IMMND_EVT_A2ND_SEARCHINIT) {
			/*Decode the rootName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.searchInit.rootName);
			if (os->size) {
				immsv_evt_dec_inline_string(i_ub, os);
			}

			if (o_evt->info.immnd.info.searchInit.searchParam.present ==
			    ImmOmSearchParameter_PR_oneAttrParam) {
				os = &(o_evt->info.immnd.info.searchInit.searchParam.choice.oneAttrParam.attrName);
				if (os->size) {
					immsv_evt_dec_inline_string(i_ub, os);
				}
				immsv_evt_dec_att_val(i_ub,
						      &(o_evt->info.immnd.info.searchInit.searchParam.choice.
							oneAttrParam.attrValue),
						      o_evt->info.immnd.info.searchInit.searchParam.choice.oneAttrParam.
						      attrValueType);
			} else {
				if (o_evt->info.immnd.info.searchInit.searchParam.present !=
				    ImmOmSearchParameter_PR_NOTHING) {
					LOG_ER("DECODE IMMND_EVT_A2ND_SEARCHINIT WRONG ENUM VALUE");
				}
			}

			/*Decode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = o_evt->info.immnd.info.searchInit.attributeNames;
			if (p) {
				immsv_evt_dec_attrNames(i_ub, &p);
				o_evt->info.immnd.info.searchInit.attributeNames = p;
			}
		} else if (o_evt->info.immnd.type == IMMND_EVT_A2ND_RT_ATT_UPPD_RSP) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.rtAttUpdRpl.sr.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = o_evt->info.immnd.info.rtAttUpdRpl.sr.attributeNames;
			if (p) {
				immsv_evt_dec_attrNames(i_ub, &p);
				o_evt->info.immnd.info.rtAttUpdRpl.sr.attributeNames = p;
			}
		} else if (o_evt->info.immnd.type == IMMND_EVT_ND2ND_SEARCH_REMOTE) {
			/*Decode the objectName */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.searchRemote.objectName);
			immsv_evt_dec_inline_string(i_ub, os);

			/*Decode the list of attribute names */
			IMMSV_ATTR_NAME_LIST *p = o_evt->info.immnd.info.searchRemote.attributeNames;
			if (p) {
				immsv_evt_dec_attrNames(i_ub, &p);
				o_evt->info.immnd.info.searchRemote.attributeNames = p;
			}
		} else if (o_evt->info.immnd.type == IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP) {
			uns8 *p8;
			uns8 c8;
			uns8 local_data[8];

			/*Decode searchRemote Response */

			/* Decode object name */
			IMMSV_OCTET_STRING *os = &(o_evt->info.immnd.info.rspSrchRmte.runtimeAttrs.objectName);
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			os->size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			immsv_evt_dec_inline_string(i_ub, os);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			c8 = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			if (c8) {
				/*Decode the list of attributes */
				IMMSV_ATTR_VALUES_LIST *p = NULL;
				immsv_evt_dec_attributes(i_ub, &p);
				o_evt->info.immnd.info.rspSrchRmte.runtimeAttrs.attrValuesList = p;
			} else {
				o_evt->info.immnd.info.rspSrchRmte.runtimeAttrs.attrValuesList = NULL;
			}
		}
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_enc_flat

 DESCRIPTION    : Encodes the contents of IMMSV_EVT into userbuf
                  The top level is encode flat (without byte-order correction).
                  Levels below the top are encoded with byter order correction
                  (because we dont really save much space/execution by
                   encoding the bits and pieces of the sub-level as flat).

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 immsv_evt_enc_flat(IMMSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	uns32 size;

	size = sizeof(IMMSV_EVT);

	/* Encode the Top level evt without byte-order correction. */
	ncs_encode_n_octets_in_uba(o_ub, (uns8 *)i_evt, size);

	return immsv_evt_enc_sublevels(i_evt, o_ub);
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_dec_flat

 DESCRIPTION    : Decodes the contents of IMMSV_EVT from user buf
                  Inverse of immsv_evt_enc_flat.

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 immsv_evt_dec_flat(NCS_UBAID *i_ub, IMMSV_EVT *o_evt)
{
	uns32 size;

	size = sizeof(IMMSV_EVT);

	/* Decode the Top level evt without byte order correction */
	ncs_decode_n_octets_from_uba(i_ub, (uns8 *)o_evt, size);

	return immsv_evt_dec_sublevels(i_ub, o_evt);
}

static uns32 immsv_evt_enc_toplevel(IMMSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *p8;
	/*TRACE_ENTER(); */

	/*i_evt->type */
	p8 = ncs_enc_reserve_space(o_ub, 4);
	ncs_encode_32bit(&p8, i_evt->type);
	ncs_enc_claim_space(o_ub, 4);

	if (i_evt->type == IMMSV_EVT_TYPE_IMMA) {
		IMMA_EVT *immaevt = &i_evt->info.imma;

		/*Nonflat encode for IMMA needed when agent and IMMND use different 
		   compilations despite being colocated. Example: Agent is compiled 
		   32bit, IMMND compiled for 64 bit. */

		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, immaevt->type);
		ncs_enc_claim_space(o_ub, 4);

		switch (immaevt->type) {
			/*Local events, not sent over MDS */
		case IMMA_EVT_MDS_INFO:	/* IMMND UP/DOWN Info */
		case IMMA_EVT_TIME_OUT:	/* Time out events at IMMA */
			LOG_ER("Unexpected event over MDS->IMMA:%u", immaevt->type);
			break;

			/* Events from IMMND */
		case IMMA_EVT_ND2A_IMM_INIT_RSP:
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.initRsp.immHandle);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.initRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_FINALIZE_RSP:
		case IMMA_EVT_ND2A_IMM_ERROR:
		case IMMA_EVT_ND2A_IMM_RESURRECT_RSP:
		case IMMA_EVT_ND2A_PROC_STALE_CLIENTS: /* Really nothing to encode for this one */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.errRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_ADMINIT_RSP:
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admInitRsp.ownerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admInitRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_ADMOP:	//Admin-op OI callback
		case IMMA_EVT_ND2A_IMM_PBE_ADMOP:	//Admin-op OI callback
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admOpReq.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admOpReq.invocation);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.admOpReq.operationId);
			ncs_enc_claim_space(o_ub, 8);

			/*skip admOpReq.timeout */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.admOpReq.continuationId);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admOpReq.objectName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immaevt->info.admOpReq.params) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMA_EVT_ND2A_ADMOP_RSP:	//Response from AdminOp to OM client
			/*skip oi_client_hdl */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.admOpRsp.invocation);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admOpRsp.result);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.admOpRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_CCBINIT_RSP:	//Response from for CcbInit
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.ccbInitRsp.error);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.ccbInitRsp.ccbId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_SEARCHINIT_RSP:	//Response from for SearchInit
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchInitRsp.error);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchInitRsp.searchId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_SEARCHNEXT_RSP:	//Response from for SearchNext
			/*Totaly encoded in sublevel. */
			break;

		case IMMA_EVT_ND2A_SEARCH_REMOTE:	//Fetch pure runtime attributes.
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.searchRemote.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchRemote.requestNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchRemote.remoteNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchRemote.searchId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.searchRemote.objectName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immaevt->info.searchRemote.attributeNames) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP:	//Response for ClassDescriptionGet
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.classDescr.classCategory);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immaevt->info.classDescr.attrDefinitions) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMA_EVT_ND2A_IMPLSET_RSP:	//Response for saImmOiImplementerSet
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.implSetRsp.error);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.implSetRsp.implId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_CREATE_UC:	//OBJ CREATE UP-CALL
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objCreate.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objCreate.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objCreate.className.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objCreate.parentName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.objCreate.immHandle);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immaevt->info.objCreate.attrValues) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_DELETE_UC:	//OBJ DELETE UP-CALL.
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objDelete.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objDelete.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objDelete.objectName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.objDelete.immHandle);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC:	//OBJ MODIFY UP-CALL.
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objModify.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objModify.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.objModify.objectName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immaevt->info.objModify.attrMods) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.objModify.immHandle);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC:	//CCB COMPLETED UP-CALL.
		case IMMA_EVT_ND2A_OI_CCB_APPLY_UC:	//CCB APPLY UP-CALL.
		case IMMA_EVT_ND2A_OI_CCB_ABORT_UC:	//CCB ABORT UP-CALL.
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.ccbCompl.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.ccbCompl.implId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immaevt->info.ccbCompl.invocation);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immaevt->info.ccbCompl.immHandle);
			ncs_enc_claim_space(o_ub, 8);
			break;

			/*
			   case IMMA_EVT_ND2A_IMM_SYNC_RSP:
			   break;
			 */
		default:
			LOG_ER("Illegal IMMA message type:%u", immaevt->type);
			rc = NCSCC_RC_OUT_OF_MEM;
		}
	} else if (i_evt->type == IMMSV_EVT_TYPE_IMMD) {
		IMMD_EVT *immdevt = &i_evt->info.immd;

		/*immdevt->type */
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, immdevt->type);
		ncs_enc_claim_space(o_ub, 4);

		switch (immdevt->type) {
			/* ALL IMMD MDS messages come from IMMND */

			/* Messages using immdevt->info.ctrl_msg */
		case IMMD_EVT_ND2D_INTRO:	/*Identification message from IMMND to IMMD. */
		case IMMD_EVT_ND2D_ANNOUNCE_LOADING:	/*Coordinator attempts start of load. */
		case IMMD_EVT_ND2D_REQ_SYNC:	/*Straggler requests sync.  */
		case IMMD_EVT_ND2D_ANNOUNCE_DUMP:	/*Dump/backup invoked */
		case IMMD_EVT_ND2D_SYNC_START:	/*Coordinator wants to start sync. */
		case IMMD_EVT_ND2D_SYNC_ABORT:	/*Coordinator wants to abort sync. */
		case IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS:/*Coord wants to purge rt obj mutations*/
		case IMMD_EVT_ND2D_LOADING_FAILED:/*Coord informs that loading failed.*/

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.ctrl_msg.ndExecPid);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.ctrl_msg.epoch);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, immdevt->info.ctrl_msg.refresh);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, immdevt->info.ctrl_msg.pbeEnabled);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMD_EVT_ND2D_ADMINIT_REQ:	/* AdminOwnerInitialize */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.admown_init.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 2);
			ncs_encode_16bit(&p8, immdevt->info.admown_init.i.adminOwnerName.length);
			ncs_enc_claim_space(o_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */

			ncs_encode_n_octets_in_uba(o_ub,
						   immdevt->info.admown_init.i.adminOwnerName.value,
						   immdevt->info.admown_init.i.adminOwnerName.length);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, immdevt->info.admown_init.i.releaseOwnershipOnFinalize);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMD_EVT_ND2D_FEVS_REQ:	/*Fake EVS over Director. */
			assert(sizeof(MDS_DEST) == 8);
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.fevsReq.reply_dest);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.fevsReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.fevsReq.msg.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immdevt->info.fevsRe.msg.buf encoded by sublevel */
			break;

		case IMMD_EVT_ND2D_CCBINIT_REQ:	/* CcbInitialize */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.ccb_init.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.ccb_init.ccbFlags);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.ccb_init.client_hdl);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMD_EVT_ND2D_IMPLSET_REQ:	/*OiImplementerSet */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.impl_set.reply_dest);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.impl_set.r.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.impl_set.r.impl_name.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immdevt->info.impl_set.r.impl_name.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.impl_set.r.scope);
			ncs_enc_claim_space(o_ub, 4);

			/*intentional fall through - encode impl_id */
		case IMMD_EVT_ND2D_DISCARD_IMPL:	/*Internal discard implementer message */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.impl_set.r.impl_id);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMD_EVT_ND2D_OI_OBJ_MODIFY:	/*saImmOiRtObjectUpdate */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immdevt->info.objModify.immHandle);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.objModify.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.objModify.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immdevt->info.objModify.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immdevt->info.objModify.attrMods) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMD_EVT_ND2D_ABORT_CCB:	/*Broadcast attempt to abort ccb. */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.ccbId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMD_EVT_ND2D_ADMO_HARD_FINALIZE:	/* Broadcast hard admo finalize */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immdevt->info.admoId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMD_EVT_MDS_INFO:
			LOG_WA("Unexpected event over MDS->IMMD:%u", immdevt->type);
		case IMMD_EVT_ND2D_IMM_SYNC_INFO:	/*May need for Director<->standby sync */
		case IMMD_EVT_ND2D_ACTIVE_SET:	/*may need this or similar */
		default:
			LOG_ER("Illegal IMMD message type:%u", immdevt->type);
			rc = NCSCC_RC_FAILURE;
		}
	} else if (i_evt->type == IMMSV_EVT_TYPE_IMMND) {
		IMMND_EVT *immndevt = &i_evt->info.immnd;

		/*immndevt->type */
		p8 = ncs_enc_reserve_space(o_ub, 4);
		ncs_encode_32bit(&p8, immndevt->type);
		ncs_enc_claim_space(o_ub, 4);
		/*TRACE_1("Encoding IMMND event:%u %s",immndevt->type, 
		   immsv_get_immnd_evt_name(immndevt->type)); */
		switch (immndevt->type) {

			/*Nonflat encode for IMMA needed when agent and IMMND use different 
			   compilations despite being colocated. Example: Agent is compiled 
			   32bit, IMMND compiled for 64 bit. */

		case IMMND_EVT_A2ND_IMM_INIT:	/* ImmOm Initialization */
		case IMMND_EVT_A2ND_IMM_OI_INIT:	/* ImmOi Initialization */
		case IMMND_EVT_A2ND_IMM_CLIENTHIGH:  /* For client resurrections */
		case IMMND_EVT_A2ND_IMM_OM_CLIENTHIGH:  /* For client resurrections */
		case IMMND_EVT_A2ND_IMM_OI_CLIENTHIGH:  /* For client resurrections */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.initReq.version.releaseCode);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.initReq.version.majorVersion);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.initReq.version.minorVersion);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.initReq.client_pid);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_A2ND_IMM_FINALIZE:	  /* ImmOm finalization */
		case IMMND_EVT_A2ND_IMM_OI_FINALIZE:  /* ImmOi finalization */
		case IMMND_EVT_A2ND_IMM_OM_RESURRECT: /* ImmOm resurrect hdl */
		case IMMND_EVT_A2ND_IMM_OI_RESURRECT: /* ImmOi resurrect hdl */
		case IMMND_EVT_A2ND_SYNC_FINALIZE:	  /* immsv_finalize_sync */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.finReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMND_EVT_A2ND_IMM_ADMINIT:	/* AdminOwnerInitialize */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.adminitReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 2);
			ncs_encode_16bit(&p8, immndevt->info.adminitReq.i.adminOwnerName.length);
			ncs_enc_claim_space(o_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */
			ncs_encode_n_octets_in_uba(o_ub,
						   immndevt->info.adminitReq.i.adminOwnerName.value,
						   immndevt->info.adminitReq.i.adminOwnerName.length);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, immndevt->info.adminitReq.i.releaseOwnershipOnFinalize);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_IMM_FEVS:	/*Fake EVS msg from Agent (forward) */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.sender_count);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.reply_dest);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.fevsReq.msg.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.fevsReq.msg.buf encoded by encode sublevel */
			break;

		case IMMND_EVT_A2ND_CCBINIT:	/* CcbInitialize */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbinitReq.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ccbinitReq.ccbFlags);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ccbinitReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMND_EVT_A2ND_SEARCHINIT:	/* SearchInitialize */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.searchInit.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchInit.rootName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchInit.scope);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.searchInit.searchOptions);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchInit.searchParam.present);
			ncs_enc_claim_space(o_ub, 4);

			if (immndevt->info.searchInit.searchParam.present == ImmOmSearchParameter_PR_oneAttrParam) {
				p8 = ncs_enc_reserve_space(o_ub, 4);
				ncs_encode_32bit(&p8,
						 immndevt->info.searchInit.searchParam.choice.oneAttrParam.attrName.
						 size);
				ncs_enc_claim_space(o_ub, 4);

				p8 = ncs_enc_reserve_space(o_ub, 4);
				ncs_encode_32bit(&p8,
						 immndevt->info.searchInit.searchParam.choice.oneAttrParam.
						 attrValueType);
				ncs_enc_claim_space(o_ub, 4);
			}

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.searchInit.attributeNames) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_SEARCHNEXT:	/* SearchNext */
		case IMMND_EVT_A2ND_SEARCHFINALIZE:	/* SearchFinalize */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.searchOp.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchOp.searchId);
			ncs_enc_claim_space(o_ub, 4);
			break;

			/*
			   case IMMND_EVT_A2ND_SEARCH_REMOTE:
			 */

		case IMMND_EVT_A2ND_RT_ATT_UPPD_RSP:	/* reply for fetch of rt attr vals */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.rtAttUpdRpl.sr.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rtAttUpdRpl.sr.requestNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rtAttUpdRpl.sr.remoteNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rtAttUpdRpl.sr.searchId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rtAttUpdRpl.sr.objectName.size);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.rtAttUpdRpl.sr.attributeNames) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rtAttUpdRpl.result);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_A2ND_PBE_ADMOP_RSP: /* PBE AdminOperation fevs Reply */
		case IMMND_EVT_A2ND_ADMOP_RSP:	/* AdminOperation sync local Reply */
		case IMMND_EVT_A2ND_ASYNC_ADMOP_RSP:	/* AdminOperation async local Reply */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpRsp.oi_client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpRsp.invocation);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpRsp.result);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_A2ND_CLASS_DESCR_GET:	/* saImmOmClassDescriptionGet */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.classDescr.className.size);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_A2ND_OI_IMPL_SET:	/* saImmOiImplementerSet */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.implSet.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.impl_name.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.implSet.impl_name.buf encoded by sublevel */

			/*skip scope & impl_id */
			break;

			/*Fevs call IMMA->IMMD->IMMNDs have to suport non-flat encoding */
		case IMMND_EVT_A2ND_OBJ_CREATE:	/* saImmOmCcbObjectCreate */
		case IMMND_EVT_A2ND_OI_OBJ_CREATE:	/* saImmOiRtObjectCreate */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objCreate.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objCreate.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objCreate.className.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.objCreate.className.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objCreate.parentName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.objCreate.parentName.buf encoded by sublevel */

			/* immhandle field not used for these immnd messages 
			   p8 = ncs_enc_reserve_space(o_ub, 8);
			   ncs_encode_64bit(&p8, immndevt->info.objCreate.immHandle);
			   ncs_enc_claim_space(o_ub, 8);
			 */
			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.objCreate.attrValues) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_OBJ_MODIFY:	/* saImmOmCcbObjectModify */
			/* IMMND_EVT_A2ND_OI_OBJ_MODIFY is "converted" to fevs by immd_evt.c */
		case IMMND_EVT_A2ND_OI_OBJ_MODIFY:	/* saImmOiRtObjectUpdate */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.objModify.immHandle);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objModify.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objModify.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objModify.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.objModify.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.objModify.attrMods) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_OBJ_DELETE:	/* saImmOmCcbObjectDelete */
		case IMMND_EVT_A2ND_OI_OBJ_DELETE:	/* saImmOiRtObjectDelete */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objDelete.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objDelete.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.objDelete.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.objModify.objectName.buf encoded by sublevel */

			/*immndevt->info.objDelete.immHandle not used in these calls */
			break;

		case IMMND_EVT_A2ND_CCB_APPLY:	/* saImmOmCcbApply */
		case IMMND_EVT_A2ND_CCB_FINALIZE:	/* saImmOmCcbFinalize */
		case IMMND_EVT_A2ND_RECOVER_CCB_OUTCOME:
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			break;

		case IMMND_EVT_A2ND_IMM_ADMOP:	/* Syncronous AdminOp */
		case IMMND_EVT_A2ND_IMM_ADMOP_ASYNC:	/* Asyncronous AdminOp */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpReq.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpReq.operationId);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpReq.continuationId);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpReq.timeout);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpReq.invocation);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpReq.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.admOpReq.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.admOpReq.params) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_CLASS_CREATE:	/* saImmOmClassCreate */
		case IMMND_EVT_A2ND_CLASS_DELETE:	/* saImmOmClassDelete */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.classDescr.className.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.classDescr.className.buf encoded by sublevel */

			if (immndevt->type == IMMND_EVT_A2ND_CLASS_CREATE) {
				p8 = ncs_enc_reserve_space(o_ub, 4);
				ncs_encode_32bit(&p8, immndevt->info.classDescr.classCategory);
				ncs_enc_claim_space(o_ub, 4);

				p8 = ncs_enc_reserve_space(o_ub, 1);
				ncs_encode_8bit(&p8, (immndevt->info.classDescr.attrDefinitions) ? 1 : 0);
				ncs_enc_claim_space(o_ub, 1);
			}
			break;

		case IMMND_EVT_A2ND_OBJ_SYNC:	/* immsv_sync */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.obj_sync.className.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.obj_sync.className.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.obj_sync.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.obj_sync.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.obj_sync.attrValues) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_A2ND_ADMO_SET:	/* AdminOwnerSet */
		case IMMND_EVT_A2ND_ADMO_RELEASE:	/* AdminOwnerRelease */
		case IMMND_EVT_A2ND_ADMO_CLEAR:	/* AdminOwnerClear */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admReq.adm_owner_id);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admReq.scope);
			ncs_enc_claim_space(o_ub, 4);

			/* objectNames encoded by sublevel. */
			break;

		case IMMND_EVT_A2ND_ADMO_FINALIZE:	/* AdminOwnerFinalize */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admFinReq.adm_owner_id);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_A2ND_OI_IMPL_CLR:	/* saImmOiImplementerClear */
		case IMMND_EVT_A2ND_OI_CL_IMPL_SET:	/* saImmOiClassImplementerSet */
		case IMMND_EVT_A2ND_OI_CL_IMPL_REL:	/* saImmOiClassImplementerRelease */
		case IMMND_EVT_A2ND_OI_OBJ_IMPL_SET:	/* saImmOiObjectImplementerSet */
		case IMMND_EVT_A2ND_OI_OBJ_IMPL_REL:	/* saImmOiObjectImplementerRelease */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.implSet.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.impl_id);
			ncs_enc_claim_space(o_ub, 4);

			if (immndevt->type != IMMND_EVT_A2ND_OI_IMPL_CLR) {
				p8 = ncs_enc_reserve_space(o_ub, 4);
				ncs_encode_32bit(&p8, immndevt->info.implSet.impl_name.size);
				ncs_enc_claim_space(o_ub, 4);
				/* immndevt->info.implSet.impl_name.buf encoded by sublevel */
				if ((immndevt->type == IMMND_EVT_A2ND_OI_OBJ_IMPL_SET) ||
				    (immndevt->type == IMMND_EVT_A2ND_OI_OBJ_IMPL_REL)) {
					p8 = ncs_enc_reserve_space(o_ub, 4);
					ncs_encode_32bit(&p8, immndevt->info.implSet.scope);
					ncs_enc_claim_space(o_ub, 4);
				}
			}
			break;

		case IMMND_EVT_A2ND_CCB_COMPLETED_RSP:	/* CcbCompleted local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP:	/*CcbObjCreate local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP:	/*CcbObjModify local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP:	/*CcbObjDelete local Reply */
		case IMMND_EVT_A2ND_PBE_PRT_OBJ_CREATE_RSP: /* Pbe OI rt obj create response */
		case IMMND_EVT_A2ND_PBE_PRTO_DELETES_COMPLETED_RSP:/*Pbe PRTO deletes done */
		case IMMND_EVT_A2ND_PBE_PRT_ATTR_UPDATE_RSP:/* Pbe OI rt attr update response*/

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ccbUpcallRsp.oi_client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbUpcallRsp.ccbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbUpcallRsp.implId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbUpcallRsp.inv);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbUpcallRsp.result);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 2);
			ncs_encode_16bit(&p8, immndevt->info.ccbUpcallRsp.name.length);
			ncs_enc_claim_space(o_ub, 2);

			/* name.value is top level because type is SaNameT */

			ncs_encode_n_octets_in_uba(o_ub,
						   immndevt->info.ccbUpcallRsp.name.value,
						   immndevt->info.ccbUpcallRsp.name.length);

			break;

			/* Events IMMND->IMMND (asyncronous) type); */
		case IMMND_EVT_ND2ND_ADMOP_RSP:	/* AdminOperation sync fevs Reply */
		case IMMND_EVT_ND2ND_ASYNC_ADMOP_RSP:	/* AdminOperation async fevs Reply */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpRsp.oi_client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.admOpRsp.invocation);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpRsp.result);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admOpRsp.error);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_ND2ND_SYNC_FINALIZE:	/* Sync finalize from coord over fevs */
		case IMMND_EVT_ND2ND_SYNC_FINALIZE_2:

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.finSync.lastContinuationId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.finSync.adminOwners) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.finSync.implementers) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.finSync.classes) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			if(immndevt->type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2) {
				TRACE("Encoding top level for IMMND_EVT_ND2ND_SYNC_FINALIZE_2");
				/* Start marker always explicitly encoded*/
			}

			break;

		case IMMND_EVT_ND2ND_SEARCH_REMOTE:	/* Forward search request from impl ND */

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.searchRemote.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchRemote.requestNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchRemote.remoteNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchRemote.searchId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.searchRemote.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.searchRemote.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.searchRemote.attributeNames) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP:	/*Forward search req from impl ND */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rspSrchRmte.result);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rspSrchRmte.requestNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rspSrchRmte.remoteNodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rspSrchRmte.searchId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.rspSrchRmte.runtimeAttrs.objectName.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.rspSrchRmte.runtimeAttrs.objectName.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.rspSrchRmte.runtimeAttrs.attrValuesList) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);
			break;

			/* Events from IMMD to IMMND */
		case IMMND_EVT_D2ND_INTRO_RSP:
		case IMMND_EVT_D2ND_SYNC_REQ:
		case IMMND_EVT_D2ND_LOADING_OK:	/*rulingEpoch & fevsMsgStart */
		case IMMND_EVT_D2ND_SYNC_START:
		case IMMND_EVT_D2ND_SYNC_ABORT:
		case IMMND_EVT_D2ND_DUMP_OK:
		case IMMND_EVT_D2ND_DISCARD_NODE:	/* Crashed IMMND or node, bcast to NDs */
		case IMMND_EVT_D2ND_PBE_PRTO_PURGE_MUTATIONS:

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ctrl.nodeId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ctrl.rulingEpoch);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ctrl.fevsMsgStart);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ctrl.ndExecPid);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.ctrl.canBeCoord) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.ctrl.isCoord) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			/*syncStarted & nodeEpoch not really used D->ND, 
			   only D->D mbcp 
			 */
			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, (immndevt->info.ctrl.syncStarted) ? 1 : 0);
			ncs_enc_claim_space(o_ub, 1);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ctrl.nodeEpoch);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_D2ND_GLOB_FEVS_REQ:	/* Fake EVS msg from director (consume) */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.sender_count);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.reply_dest);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.fevsReq.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.fevsReq.msg.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.fevsReq.msg.buf encoded by encode sublevel */
			break;

		case IMMND_EVT_D2ND_ADMINIT:	/* Admin Owner init reply */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.adminitGlobal.globalOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 2);
			ncs_encode_16bit(&p8, immndevt->info.adminitGlobal.i.adminOwnerName.length);
			ncs_enc_claim_space(o_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */
			ncs_encode_n_octets_in_uba(o_ub,
						   immndevt->info.adminitGlobal.i.adminOwnerName.value,
						   immndevt->info.adminitGlobal.i.adminOwnerName.length);

			p8 = ncs_enc_reserve_space(o_ub, 1);
			ncs_encode_8bit(&p8, immndevt->info.adminitGlobal.i.releaseOwnershipOnFinalize);
			ncs_enc_claim_space(o_ub, 1);
			break;

		case IMMND_EVT_D2ND_CCBINIT:	/* Ccb init reply */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbinitGlobal.globalCcbId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbinitGlobal.i.adminOwnerId);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ccbinitGlobal.i.ccbFlags);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.ccbinitGlobal.i.client_hdl);
			ncs_enc_claim_space(o_ub, 8);
			break;

		case IMMND_EVT_D2ND_IMPLSET_RSP:	/* Impl set reply from D with impl id */
			p8 = ncs_enc_reserve_space(o_ub, 8);
			ncs_encode_64bit(&p8, immndevt->info.implSet.client_hdl);
			ncs_enc_claim_space(o_ub, 8);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.impl_id);
			ncs_enc_claim_space(o_ub, 4);

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.impl_name.size);
			ncs_enc_claim_space(o_ub, 4);
			/* immndevt->info.implSet.impl_name.buf encoded by sublevel */

			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.scope);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_D2ND_DISCARD_IMPL:	/* Discard implementer broadcast to NDs */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.implSet.impl_id);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_D2ND_ABORT_CCB:	/* Abort CCB broadcast to NDs */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.ccbId);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_D2ND_ADMO_HARD_FINALIZE:	/* Admo hard fnlz bcast to NDs */
			p8 = ncs_enc_reserve_space(o_ub, 4);
			ncs_encode_32bit(&p8, immndevt->info.admFinReq.adm_owner_id);
			ncs_enc_claim_space(o_ub, 4);
			break;

		case IMMND_EVT_D2ND_RESET:
			/* message has no contents */
			break;

		case IMMND_EVT_MDS_INFO:	/* IMMA/IMMND/IMMD UP/DOWN Info */
		case IMMND_EVT_TIME_OUT:	/* Time out event */
		case IMMND_EVT_CB_DUMP:
			LOG_ER("Unexpected event over MDS->IMMND:%u", immndevt->type);
			/*Fall through */
		default:
			LOG_ER("Illegal IMMND message type:%u", immndevt->type);

			rc = NCSCC_RC_FAILURE;
		}
	} else {
		LOG_ER("Illegal event message type or format");
		rc = NCSCC_RC_FAILURE;
	}
	/*TRACE_LEAVE(); */
	return rc;
}

static uns32 immsv_evt_dec_toplevel(NCS_UBAID *i_ub, IMMSV_EVT *o_evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *p8;
	uns8 local_data[8];
	/*TRACE_ENTER(); */

	/*o_evt->type */
	p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
	o_evt->type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(i_ub, 4);

	if (o_evt->type == IMMSV_EVT_TYPE_IMMA) {
		IMMA_EVT *immaevt = &o_evt->info.imma;

		/*Nonflat decode for IMMA needed when agent and IMMND use different 
		   compilations despite being colocated. Example: Agent is compiled 
		   32bit, IMMND compiled for 64 bit. */

		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		immaevt->type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		switch (immaevt->type) {
			/*Local events, not sent over MDS */
		case IMMA_EVT_MDS_INFO:	/* IMMND UP/DOWN Info */
		case IMMA_EVT_TIME_OUT:	/* Time out events at IMMA */
			LOG_ER("Unexpected event over MDS->IMMA:%u", immaevt->type);
			break;

			/* Events from IMMND */
		case IMMA_EVT_ND2A_IMM_INIT_RSP:
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.initRsp.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.initRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_FINALIZE_RSP:
		case IMMA_EVT_ND2A_IMM_ERROR:	//Generic error reply
		case IMMA_EVT_ND2A_IMM_RESURRECT_RSP:
		case IMMA_EVT_ND2A_PROC_STALE_CLIENTS: /* Really nothing to encode for this one */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.errRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_ADMINIT_RSP:
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admInitRsp.ownerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admInitRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_IMM_ADMOP:	/*Admin-op OI callback */
		case IMMA_EVT_ND2A_IMM_PBE_ADMOP:	/*PBE Admin-op callback */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admOpReq.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admOpReq.invocation = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.admOpReq.operationId = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			/*skip admOpReq.timeout */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.admOpReq.continuationId = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admOpReq.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode params. */
				immaevt->info.admOpReq.params = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			break;

		case IMMA_EVT_ND2A_ADMOP_RSP:	//Response from AdminOp to OM client, normal call

			/*skip oi_client_hdl */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.admOpRsp.invocation = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admOpRsp.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.admOpRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_CCBINIT_RSP:	//Response from for CcbInit
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.ccbInitRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.ccbInitRsp.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_SEARCHINIT_RSP:	//Response from for SearchInit
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchInitRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchInitRsp.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_SEARCHNEXT_RSP:	//Response from for SearchNext
			/*Totaly decoded in sublevel. */
			break;

		case IMMA_EVT_ND2A_SEARCH_REMOTE:	//Fetch pure runtime attributes.
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.searchRemote.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchRemote.requestNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchRemote.remoteNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchRemote.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.searchRemote.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attributeNames. */
				immaevt->info.searchRemote.attributeNames = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP:	//Response for SaImmOmClassDescriptionGet
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.classDescr.classCategory = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attributeNames. */
				immaevt->info.classDescr.attrDefinitions = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMA_EVT_ND2A_IMPLSET_RSP:	//Response for saImmOiImplementerSet
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.implSetRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.implSetRsp.implId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_CREATE_UC:	//OBJ CREATE UP-CALL
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objCreate.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objCreate.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objCreate.className.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objCreate.parentName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.objCreate.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attrValues. */
				immaevt->info.objCreate.attrValues = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_DELETE_UC:	//OBJ DELETE UP-CALL.
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objDelete.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objDelete.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objDelete.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.objDelete.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC:	//OBJ MODIFY UP-CALL.
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objModify.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objModify.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.objModify.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attrMods. */
				immaevt->info.objModify.attrMods = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.objModify.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC:	//CCB COMPLETED UP-CALL.
		case IMMA_EVT_ND2A_OI_CCB_APPLY_UC:	//CCB APPLY UP-CALL.
		case IMMA_EVT_ND2A_OI_CCB_ABORT_UC:	//CCB ABORT UP-CALL.
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.ccbCompl.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.ccbCompl.implId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immaevt->info.ccbCompl.invocation = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immaevt->info.ccbCompl.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

			/*
			   case IMMA_EVT_ND2A_IMM_SYNC_RSP:
			   break;
			 */
		default:
			LOG_ER("Illegal IMMA message type:%u", immaevt->type);
			rc = NCSCC_RC_FAILURE;
		}
	} else if (o_evt->type == IMMSV_EVT_TYPE_IMMD) {
		IMMD_EVT *immdevt = &o_evt->info.immd;

		/*immdevt->type */
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		immdevt->type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		/*TRACE_5("Decoding IMMD EVENT %u", immdevt->type); */

		switch (immdevt->type) {
			/* Events from IMMND */
		case IMMD_EVT_ND2D_INTRO:	/*Identification message from IMMND to IMMD. */
		case IMMD_EVT_ND2D_ANNOUNCE_LOADING:	/*Coordinator attempts start of load. */
		case IMMD_EVT_ND2D_REQ_SYNC:	/*Straggler requests sync.  */
		case IMMD_EVT_ND2D_ANNOUNCE_DUMP:	/*Dump/backup invoked */
		case IMMD_EVT_ND2D_SYNC_START:	/*Coordinator wants to start sync. */
		case IMMD_EVT_ND2D_SYNC_ABORT:	/*Coordinator wants to abort sync. */
		case IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS: /*Coord wants to purge rt obj mutations */
		case IMMD_EVT_ND2D_LOADING_FAILED:/*Coord informs that loading failed.*/
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.ctrl_msg.ndExecPid = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.ctrl_msg.epoch = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			immdevt->info.ctrl_msg.refresh = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			immdevt->info.ctrl_msg.pbeEnabled = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMD_EVT_ND2D_ADMINIT_REQ:	/* AdminOwnerInitialize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.admown_init.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 2);
			immdevt->info.admown_init.i.adminOwnerName.length = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(i_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */
			ncs_decode_n_octets_from_uba(i_ub,
						     immdevt->info.admown_init.i.adminOwnerName.value,
						     immdevt->info.admown_init.i.adminOwnerName.length);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			immdevt->info.admown_init.i.releaseOwnershipOnFinalize = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMD_EVT_ND2D_FEVS_REQ:	/*Fake EVS over Director. */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.fevsReq.reply_dest = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.fevsReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.fevsReq.msg.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immdevt->info.fevsReq.msg.buf decoded by sublevel */
			break;

		case IMMD_EVT_ND2D_CCBINIT_REQ:	/* CcbInitialize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.ccb_init.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.ccb_init.ccbFlags = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.ccb_init.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMD_EVT_ND2D_IMPLSET_REQ:	/*OiImplementerSet */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.impl_set.reply_dest = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.impl_set.r.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.impl_set.r.impl_name.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immdevt->info.impl_set.r.impl_name.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.impl_set.r.scope = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			/*intentional fall through - decode impl_id */
		case IMMD_EVT_ND2D_DISCARD_IMPL:	/*Internal discard implementer message */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.impl_set.r.impl_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMD_EVT_ND2D_OI_OBJ_MODIFY:	/*saImmOiRtObjectUpdate */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immdevt->info.objModify.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.objModify.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.objModify.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immdevt->info.objModify.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attrMods. */
				immdevt->info.objModify.attrMods = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMD_EVT_ND2D_ABORT_CCB:	/*Broadcast attempt to abort ccb. */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMD_EVT_ND2D_ADMO_HARD_FINALIZE:	/* Broadcast hard admo finalize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immdevt->info.admoId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMD_EVT_MDS_INFO:
			LOG_ER("Unexpected event over MDS->IMMD:%u", immdevt->type);
		case IMMD_EVT_ND2D_ACTIVE_SET:	/*ABT may need this or something similar */
		case IMMD_EVT_ND2D_IMM_SYNC_INFO:	/*May need this for Director<->standby sync */
		default:
			LOG_ER("Illegal IMMD message type:%u", immdevt->type);
			rc = NCSCC_RC_FAILURE;
		}
	} else if (o_evt->type == IMMSV_EVT_TYPE_IMMND) {
		IMMND_EVT *immndevt = &o_evt->info.immnd;

		/*immndevt->type */
		p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
		immndevt->type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(i_ub, 4);

		/*TRACE_5("Decoding IMMND event:%u %s",immndevt->type, 
		   immsv_get_immnd_evt_name(immndevt->type)); */

		switch (immndevt->type) {

			/* Events from IMMA */

			/* IMMA commincation to the local IMMND => should have been flat encode. */
		case IMMND_EVT_A2ND_IMM_INIT:	/* ImmOm Initialization */
		case IMMND_EVT_A2ND_IMM_OI_INIT:	/* ImmOi Initialization */
		case IMMND_EVT_A2ND_IMM_CLIENTHIGH: /* For client resurrections */
		case IMMND_EVT_A2ND_IMM_OM_CLIENTHIGH: /* For client resurrections */
		case IMMND_EVT_A2ND_IMM_OI_CLIENTHIGH: /* For client resurrections */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.initReq.version.releaseCode = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.initReq.version.majorVersion = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.initReq.version.minorVersion = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.initReq.client_pid = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_IMM_FINALIZE:	/* ImmOm finalization */
		case IMMND_EVT_A2ND_IMM_OI_FINALIZE:	/* ImmOi finalization */
		case IMMND_EVT_A2ND_IMM_OM_RESURRECT: /* ImmOm resurrect hdl*/
		case IMMND_EVT_A2ND_IMM_OI_RESURRECT: /* ImmOi resurrect hdl*/
		case IMMND_EVT_A2ND_SYNC_FINALIZE:	/* immsv_finalize_sync */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.finReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMND_EVT_A2ND_IMM_ADMINIT:	/* AdminOwnerInitialize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.adminitReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 2);
			immndevt->info.adminitReq.i.adminOwnerName.length = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(i_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */
			ncs_decode_n_octets_from_uba(i_ub,
						     immndevt->info.adminitReq.i.adminOwnerName.value,
						     immndevt->info.adminitReq.i.adminOwnerName.length);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			immndevt->info.adminitReq.i.releaseOwnershipOnFinalize = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_IMM_FEVS:	/*Fake EVS msg from Agent (forward) */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.sender_count = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.reply_dest = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.fevsReq.msg.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.fevsReq.msg.buf decoded by decode sublevel */
			break;

		case IMMND_EVT_A2ND_CCBINIT:	/* CcbInitialize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbinitReq.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ccbinitReq.ccbFlags = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ccbinitReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMND_EVT_A2ND_SEARCHINIT:	/* SearchInitialize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.searchInit.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchInit.rootName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchInit.scope = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.searchInit.searchOptions = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchInit.searchParam.present = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			if (immndevt->info.searchInit.searchParam.present == ImmOmSearchParameter_PR_oneAttrParam) {
				p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
				immndevt->info.searchInit.searchParam.choice.oneAttrParam.attrName.size =
				    ncs_decode_32bit(&p8);
				ncs_dec_skip_space(i_ub, 4);

				p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
				immndevt->info.searchInit.searchParam.choice.oneAttrParam.attrValueType =
				    ncs_decode_32bit(&p8);
				ncs_dec_skip_space(i_ub, 4);
			}

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attributeNames. */
				immndevt->info.searchInit.attributeNames = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_SEARCHNEXT:	/* SearchNext */
		case IMMND_EVT_A2ND_SEARCHFINALIZE:	/* SearchFinalize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.searchOp.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchOp.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

			/*
			   case IMMND_EVT_A2ND_SEARCH_REMOTE:
			 */

		case IMMND_EVT_A2ND_RT_ATT_UPPD_RSP:	/* reply for fetch of rt attr vals */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.rtAttUpdRpl.sr.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rtAttUpdRpl.sr.requestNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rtAttUpdRpl.sr.remoteNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rtAttUpdRpl.sr.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rtAttUpdRpl.sr.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attributeNames. */
				immndevt->info.rtAttUpdRpl.sr.attributeNames = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rtAttUpdRpl.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_PBE_ADMOP_RSP:	/* AdminOperation sync local Reply */
		case IMMND_EVT_A2ND_ADMOP_RSP:	/* AdminOperation sync local Reply */
		case IMMND_EVT_A2ND_ASYNC_ADMOP_RSP:	/* AdminOperation async local Reply */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpRsp.oi_client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpRsp.invocation = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpRsp.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_CLASS_DESCR_GET:	/* saImmOmClassDescriptionGet */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.classDescr.className.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_OI_IMPL_SET:	/* saImmOiImplementerSet */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.implSet.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.impl_name.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.implSet.impl_name.buf decoded by sublevel */

			/*skip scope & impl_id */
			break;

			/*Fevs call IMMA->IMMD->IMMNDs have to suport non-flat encoding */
		case IMMND_EVT_A2ND_OBJ_CREATE:	/* saImmOmCcbObjectCreate */
		case IMMND_EVT_A2ND_OI_OBJ_CREATE:	/* saImmOiRtObjectCreate */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objCreate.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objCreate.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objCreate.className.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.objCreate.className.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objCreate.parentName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.objCreate.parentName.buf decoded by sublevel */

			/* immhandle field not used for these immnd messages 
			   p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			   immndevt->info.objCreate.immHandle = ncs_decode_64bit(&p8);
			   ncs_dec_skip_space(i_ub, 8);
			 */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attrValues. */
				immndevt->info.objCreate.attrValues = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_OBJ_MODIFY:	/* saImmOmCcbObjectModify */
			/* IMMND_EVT_A2ND_OI_OBJ_MODIFY is "converted" to fevs by immd_evt.c */
		case IMMND_EVT_A2ND_OI_OBJ_MODIFY:	/* saImmOiRtObjectUpdate */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.objModify.immHandle = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objModify.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objModify.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objModify.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.objModify.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode attrMods. */
				immndevt->info.objModify.attrMods = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_OBJ_DELETE:	/* saImmOmCcbObjectDelete */
		case IMMND_EVT_A2ND_OI_OBJ_DELETE:	/* saImmOiRtObjectDelete */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objDelete.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objDelete.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.objDelete.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.objDelete.objectName.buf decoded by sublevel */

			/*immndevt->info.objDelete.immHandle not used in these calls */
			break;

		case IMMND_EVT_A2ND_CCB_APPLY:	/* saImmOmCcbApply */
		case IMMND_EVT_A2ND_CCB_FINALIZE:	/* saImmOmCcbFinalize */
		case IMMND_EVT_A2ND_RECOVER_CCB_OUTCOME:
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_IMM_ADMOP:	/* Syncronous AdminOp */
		case IMMND_EVT_A2ND_IMM_ADMOP_ASYNC:	/* Asyncronous AdminOp */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpReq.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpReq.operationId = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpReq.continuationId = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpReq.timeout = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpReq.invocation = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpReq.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.admOpReq.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to decode params. */
				immndevt->info.admOpReq.params = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_CLASS_CREATE:	/* saImmOmClassCreate */
		case IMMND_EVT_A2ND_CLASS_DELETE:	/* saImmOmClassDelete */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.classDescr.className.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.classDescr.className.buf decoded by sublevel */

			if (immndevt->type == IMMND_EVT_A2ND_CLASS_CREATE) {
				p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
				immndevt->info.classDescr.classCategory = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(i_ub, 4);

				p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
				if (ncs_decode_8bit(&p8)) {
					/*Bogus pointer-val forces decode_sublevel to 
					   decode attrDefinitions. */
					immndevt->info.classDescr.attrDefinitions = (void *)0x1;
				}
				ncs_dec_skip_space(i_ub, 1);
			}
			break;

		case IMMND_EVT_A2ND_OBJ_SYNC:	/* immsv_sync */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.obj_sync.className.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.obj_sync.className.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.obj_sync.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.obj_sync.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attrValues. */
				immndevt->info.obj_sync.attrValues = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_A2ND_ADMO_SET:	/* AdminOwnerSet */
		case IMMND_EVT_A2ND_ADMO_RELEASE:	/* AdminOwnerRelease */
		case IMMND_EVT_A2ND_ADMO_CLEAR:	/* AdminOwnerClear */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admReq.adm_owner_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admReq.scope = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			/* objectNames decoded by sublevel. */
			break;

		case IMMND_EVT_A2ND_ADMO_FINALIZE:	/* AdminOwnerFinalize */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admFinReq.adm_owner_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_A2ND_OI_IMPL_CLR:	/* saImmOiImplementerClear */
		case IMMND_EVT_A2ND_OI_CL_IMPL_SET:	/* saImmOiClassImplementerSet */
		case IMMND_EVT_A2ND_OI_CL_IMPL_REL:	/* saImmOiClassImplementerRelease */
		case IMMND_EVT_A2ND_OI_OBJ_IMPL_SET:	/* saImmOiObjectImplementerSet */
		case IMMND_EVT_A2ND_OI_OBJ_IMPL_REL:	/* saImmOiObjectImplementerRelease */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.implSet.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.impl_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			if (immndevt->type != IMMND_EVT_A2ND_OI_IMPL_CLR) {
				p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
				immndevt->info.implSet.impl_name.size = ncs_decode_32bit(&p8);
				ncs_dec_skip_space(i_ub, 4);
				/* immndevt->info.implSet.impl_name.buf decoded by sublevel */
				if ((immndevt->type == IMMND_EVT_A2ND_OI_OBJ_IMPL_SET) ||
				    (immndevt->type == IMMND_EVT_A2ND_OI_OBJ_IMPL_REL)) {
					p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
					immndevt->info.implSet.scope = ncs_decode_32bit(&p8);
					ncs_dec_skip_space(i_ub, 4);
				}
			}
			break;

		case IMMND_EVT_A2ND_CCB_COMPLETED_RSP:	/* CcbCompleted local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP:	/*CcbObjCreate local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP:	/*CcbObjModify local Reply */
		case IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP:	/*CcbObjDelete local Reply */
		case IMMND_EVT_A2ND_PBE_PRT_OBJ_CREATE_RSP: /* Pbe OI rt obj create response */
		case IMMND_EVT_A2ND_PBE_PRTO_DELETES_COMPLETED_RSP:/*Pbe PRTO deletes done */
		case IMMND_EVT_A2ND_PBE_PRT_ATTR_UPDATE_RSP:/* Pbe OI rt attr update response*/

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ccbUpcallRsp.oi_client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbUpcallRsp.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbUpcallRsp.implId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbUpcallRsp.inv = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbUpcallRsp.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 2);
			immndevt->info.ccbUpcallRsp.name.length = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(i_ub, 2);

			/* name.value is top level because type is SaNameT */
			ncs_decode_n_octets_from_uba(i_ub,
						     immndevt->info.ccbUpcallRsp.name.value,
						     immndevt->info.ccbUpcallRsp.name.length);

			break;

			/* Events IMMND->IMMND (asyncronous) */
		case IMMND_EVT_ND2ND_ADMOP_RSP:	/* AdminOperation sync fevs Reply */
		case IMMND_EVT_ND2ND_ASYNC_ADMOP_RSP:	/* AdminOperation async fevs Reply */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpRsp.oi_client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.admOpRsp.invocation = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpRsp.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admOpRsp.error = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			break;

		case IMMND_EVT_ND2ND_SYNC_FINALIZE_2:
		case IMMND_EVT_ND2ND_SYNC_FINALIZE:	/* Sync finalize from coord over fevs */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.finSync.lastContinuationId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode adminOwners. */
				immndevt->info.finSync.adminOwners = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode implementers. */
				immndevt->info.finSync.implementers = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode classes. */
				immndevt->info.finSync.classes = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);

			if(immndevt->type == IMMND_EVT_ND2ND_SYNC_FINALIZE_2) {
				TRACE("Decoding top level for IMMND_EVT_ND2ND_SYNC_FINALIZE_2");
				/*Start marker always explicitly decoded. */
			}
			break;

		case IMMND_EVT_ND2ND_SEARCH_REMOTE:	/* Forward search request from impl ND */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.searchRemote.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchRemote.requestNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchRemote.remoteNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchRemote.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.searchRemote.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.searchRemote.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attributeNames. */
				immndevt->info.searchRemote.attributeNames = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP:	/*Forward search req from impl ND */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rspSrchRmte.result = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rspSrchRmte.requestNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rspSrchRmte.remoteNodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rspSrchRmte.searchId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.rspSrchRmte.runtimeAttrs.objectName.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.rspSrchRmte.objectName.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				/*Bogus pointer-val forces decode_sublevel to 
				   decode attrValuesList. */
				immndevt->info.rspSrchRmte.runtimeAttrs.attrValuesList = (void *)0x1;
			}
			ncs_dec_skip_space(i_ub, 1);
			break;

			/* Events from IMMD to IMMND */
		case IMMND_EVT_D2ND_INTRO_RSP:
		case IMMND_EVT_D2ND_SYNC_REQ:
		case IMMND_EVT_D2ND_LOADING_OK:
		case IMMND_EVT_D2ND_SYNC_START:
		case IMMND_EVT_D2ND_SYNC_ABORT:
		case IMMND_EVT_D2ND_DUMP_OK:
		case IMMND_EVT_D2ND_DISCARD_NODE:	/* Crashed IMMND or node, bcast to NDs */
		case IMMND_EVT_D2ND_PBE_PRTO_PURGE_MUTATIONS:

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ctrl.nodeId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ctrl.rulingEpoch = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ctrl.fevsMsgStart = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ctrl.ndExecPid = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				immndevt->info.ctrl.canBeCoord = TRUE;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				immndevt->info.ctrl.isCoord = TRUE;
			}
			ncs_dec_skip_space(i_ub, 1);

			/*syncStarted & nodeEpoch not really used D->ND, 
			   only D->D mbcp 
			 */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			if (ncs_decode_8bit(&p8)) {
				immndevt->info.ctrl.syncStarted = TRUE;
			}
			ncs_dec_skip_space(i_ub, 1);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ctrl.nodeEpoch = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_D2ND_GLOB_FEVS_REQ:	/* Fake EVS msg from director (consume) */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.sender_count = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.reply_dest = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.fevsReq.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.fevsReq.msg.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.fevsReq.msg.buf decoded by decode sublevel */
			break;

		case IMMND_EVT_D2ND_ADMINIT:	/* Admin Owner init reply */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.adminitGlobal.globalOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 2);
			immndevt->info.adminitGlobal.i.adminOwnerName.length = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(i_ub, 2);

			/* adminOwnerName.value is top level because type is SaNameT */
			ncs_decode_n_octets_from_uba(i_ub,
						     immndevt->info.adminitGlobal.i.adminOwnerName.value,
						     immndevt->info.adminitGlobal.i.adminOwnerName.length);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 1);
			immndevt->info.adminitGlobal.i.releaseOwnershipOnFinalize = ncs_decode_8bit(&p8);
			ncs_dec_skip_space(i_ub, 1);
			break;

		case IMMND_EVT_D2ND_CCBINIT:	/* Ccb init reply */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbinitGlobal.globalCcbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbinitGlobal.i.adminOwnerId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ccbinitGlobal.i.ccbFlags = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.ccbinitGlobal.i.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);
			break;

		case IMMND_EVT_D2ND_IMPLSET_RSP:	/* Implter set reply from D with impl id */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 8);
			immndevt->info.implSet.client_hdl = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(i_ub, 8);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.impl_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.impl_name.size = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			/* immndevt->info.implSet.impl_name.buf decoded by sublevel */

			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.scope = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_D2ND_DISCARD_IMPL:	/* Discard implementer broadcast to NDs */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.implSet.impl_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_D2ND_ABORT_CCB:	/* Abort CCB broadcast to NDs */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.ccbId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_D2ND_ADMO_HARD_FINALIZE:	/* Admo hard fin bcast to NDs */
			p8 = ncs_dec_flatten_space(i_ub, local_data, 4);
			immndevt->info.admFinReq.adm_owner_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(i_ub, 4);
			break;

		case IMMND_EVT_D2ND_RESET:
			/* message has no contents */
			break;

		case IMMND_EVT_MDS_INFO:	/* IMMA/IMMND/IMMD UP/DOWN Info */
		case IMMND_EVT_TIME_OUT:	/* Time out event */
		case IMMND_EVT_CB_DUMP:
			LOG_ER("Unexpected event over MDS->IMMND:%u", immndevt->type);
			break;
		default:
			LOG_ER("Illegal IMMND message type:%u", immndevt->type);
			rc = NCSCC_RC_FAILURE;
		}
	} else {
		LOG_ER("Illegal event message type or format");
		rc = NCSCC_RC_FAILURE;
	}
	/*TRACE_LEAVE(); */
	return rc;
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_enc

 DESCRIPTION    : Encodes the contents of IMMSV_EVT into userbuf
                  Both top level and sub-levels are encoded with 
                  byte-order correction.

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub  - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 immsv_evt_enc(IMMSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	uns32 rc;

	rc = immsv_evt_enc_toplevel(i_evt, o_ub);
	if (rc == NCSCC_RC_SUCCESS) {
		rc = immsv_evt_enc_sublevels(i_evt, o_ub);
	}

	return rc;
}

/****************************************************************************\
 PROCEDURE NAME : immsv_evt_dec

 DESCRIPTION    : Decodes the contents of IMMSV_EVT from user buf
                  Inverse of immsv_evt_enc.

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 immsv_evt_dec(NCS_UBAID *i_ub, IMMSV_EVT *o_evt)
{
	uns32 rc;

	rc = immsv_evt_dec_toplevel(i_ub, o_evt);
	if (rc == NCSCC_RC_SUCCESS) {
		rc = immsv_evt_dec_sublevels(i_ub, o_evt);
	}

	return rc;
}

void immsv_msg_trace_send(MDS_DEST to, IMMSV_EVT *evt)
{
	if (evt->type == IMMSV_EVT_TYPE_IMMD) {
		TRACE_8("Sending:  %s to %x", immsv_get_immd_evt_name(evt->info.immd.type),
			m_NCS_NODE_ID_FROM_MDS_DEST(to));
	}

	if (evt->type == IMMSV_EVT_TYPE_IMMND) {
		TRACE_8("Sending:  %s to %x", immsv_get_immnd_evt_name(evt->info.immnd.type),
			m_NCS_NODE_ID_FROM_MDS_DEST(to));
	}
}

void immsv_msg_trace_rec(MDS_DEST from, IMMSV_EVT *evt)
{
	if (evt->type == IMMSV_EVT_TYPE_IMMD) {
		TRACE_8("Received: %s from %x", immsv_get_immd_evt_name(evt->info.immd.type),
			m_NCS_NODE_ID_FROM_MDS_DEST(from));
	}

	if (evt->type == IMMSV_EVT_TYPE_IMMND) {
		TRACE_8("Received: %s from %x", immsv_get_immnd_evt_name(evt->info.immnd.type),
			m_NCS_NODE_ID_FROM_MDS_DEST(from));
	}
}
