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

  DESCRIPTION:This module deals with the creation, accessing and deletion of
  the SU SI relationship list on the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <avsv_util.h>
#include <avd_util.h>
#include <avd_susi.h>
#include <immutil.h>
#include <avd_imm.h>
#include <avd_csi.h>
#include <logtrace.h>

static NCS_PATRICIA_TREE sirankedsu_db;
static void avd_susi_namet_init(const SaNameT *object_name, SaNameT *su_name, SaNameT *si_name);

static void avd_sirankedsu_db_add(AVD_SUS_PER_SI_RANK *sirankedsu)
{
        AVD_SI *avd_si = NULL;
        unsigned int rc = ncs_patricia_tree_add(&sirankedsu_db, &sirankedsu->tree_node);
        assert(rc == NCSCC_RC_SUCCESS);

        /* Find the si name. */
        avd_si = avd_si_get(&(sirankedsu->indx.si_name));
	avd_si_add_rankedsu(avd_si, &sirankedsu->su_name, sirankedsu->indx.su_rank);

        /* Add sus_per_si_rank to si */
        sirankedsu->sus_per_si_rank_on_si = avd_si;
        sirankedsu->sus_per_si_rank_list_si_next =
            sirankedsu->sus_per_si_rank_on_si->list_of_sus_per_si_rank;
        sirankedsu->sus_per_si_rank_on_si->list_of_sus_per_si_rank = sirankedsu;
}

/*****************************************************************************
 * Function: avd_sirankedsu_create
 *
 * Purpose:  This function will create and add a AVD_SUS_PER_SI_RANK structure to the
 * tree if an element with given index value doesn't exist in the tree.
 *
 * Input: cb - the AVD control block
 *        indx - The key info  needs to add a element in the petricia tree 
 *
 * Returns: The pointer to AVD_SUS_PER_SI_RANK structure allocated and added.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static AVD_SUS_PER_SI_RANK *avd_sirankedsu_create(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK_INDX indx)
{
	AVD_SUS_PER_SI_RANK *ranked_su_per_si;

	if ((ranked_su_per_si = calloc(1, sizeof(AVD_SUS_PER_SI_RANK))) == NULL) {
		LOG_ER("Mem Alloc failed");
		return NULL;
	}

	ranked_su_per_si->indx.si_name.length = indx.si_name.length;
	memcpy(ranked_su_per_si->indx.si_name.value, indx.si_name.value,
		ranked_su_per_si->indx.si_name.length);

	ranked_su_per_si->indx.su_rank = indx.su_rank;

	ranked_su_per_si->tree_node.key_info = (uns8 *)(&ranked_su_per_si->indx);
	ranked_su_per_si->tree_node.bit = 0;
	ranked_su_per_si->tree_node.left = NULL;
	ranked_su_per_si->tree_node.right = NULL;

	return ranked_su_per_si;
}

/*****************************************************************************
 * Function: avd_sirankedsu_find
 *
 * Purpose:  This function will find a AVD_SUS_PER_SI_RANK structure in the
 * tree with indx value as key.
 *
 * Input: cb - the AVD control block
 *        indx - The key.
 *
 * Returns: The pointer to AVD_SUS_PER_SI_RANK structure found in the tree.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static AVD_SUS_PER_SI_RANK *avd_sirankedsu_find(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK_INDX indx)
{
	AVD_SUS_PER_SI_RANK *ranked_su_per_si = NULL;
	AVD_SUS_PER_SI_RANK_INDX rank_indx;

	memset(&rank_indx, '\0', sizeof(AVD_SUS_PER_SI_RANK_INDX));
	rank_indx.si_name.length = indx.si_name.length;
	memcpy(rank_indx.si_name.value, indx.si_name.value, indx.si_name.length);
	rank_indx.su_rank = indx.su_rank;

	ranked_su_per_si = (AVD_SUS_PER_SI_RANK *)ncs_patricia_tree_get(&sirankedsu_db, (uns8 *)&rank_indx);

	return ranked_su_per_si;
}

/*****************************************************************************
 * Function: avd_sirankedsu_getnext
 *
 * Purpose:  This function will find the next AVD_SUS_PER_SI_RANK structure in the
 * tree whose key value is next of the given key value.
 *
 * Input: cb - the AVD control block
 *        indx - The key value.
 *
 * Returns: The pointer to AVD_SUS_PER_SI_RANK structure found in the tree. 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SUS_PER_SI_RANK *avd_sirankedsu_getnext(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK_INDX indx)
{
	AVD_SUS_PER_SI_RANK *ranked_su_per_si = NULL;
	AVD_SUS_PER_SI_RANK_INDX rank_indx;

	memset(&rank_indx, '\0', sizeof(AVD_SUS_PER_SI_RANK_INDX));
	rank_indx.si_name.length = indx.si_name.length;
	memcpy(rank_indx.si_name.value, indx.si_name.value, indx.si_name.length);
	rank_indx.su_rank = indx.su_rank;

	ranked_su_per_si = (AVD_SUS_PER_SI_RANK *)ncs_patricia_tree_getnext(&sirankedsu_db, (uns8 *)&rank_indx);

	return ranked_su_per_si;
}

/*****************************************************************************
 * Function: avd_sirankedsu_getnext_valid
 *
 * Purpose:  This function will find the next AVD_SUS_PER_SI_RANK structure in the
 * tree whose key value is next of the given key value. It also verifies if the 
 * the si & su belong to the same sg.
 *
 * Input: cb - the AVD control block
 *        indx - The key value.
 *       o_su - output field indicating the pointer to the pointer of
 *                the SU in the SISU rank list. Filled when return value is not
 *              NULL.
 *
 * Returns: The pointer to AVD_SUS_PER_SI_RANK structure found in the tree. 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SUS_PER_SI_RANK *avd_sirankedsu_getnext_valid(AVD_CL_CB *cb,
	AVD_SUS_PER_SI_RANK_INDX indx, AVD_SU **o_su)
{
	AVD_SUS_PER_SI_RANK *ranked_su_per_si = NULL;
	AVD_SUS_PER_SI_RANK_INDX rank_indx;
	AVD_SI *si = NULL;
	AVD_SU *su = NULL;

	memset(&rank_indx, '\0', sizeof(AVD_SUS_PER_SI_RANK_INDX));
	rank_indx.si_name.length = indx.si_name.length;
	memcpy(rank_indx.si_name.value, indx.si_name.value, indx.si_name.length);
	rank_indx.su_rank = indx.su_rank;

	ranked_su_per_si = (AVD_SUS_PER_SI_RANK *)ncs_patricia_tree_getnext(&sirankedsu_db, (uns8 *)&rank_indx);

	if (ranked_su_per_si == NULL) {
		/*  return NULL */
		return ranked_su_per_si;
	}

	/* get the su & si */
	su = avd_su_get(&ranked_su_per_si->su_name);
	si = avd_si_get(&indx.si_name);

	/* validate this entry */
	if ((si == NULL) || (su == NULL) || (si->sg_of_si != su->sg_of_su))
		return avd_sirankedsu_getnext_valid(cb, ranked_su_per_si->indx, o_su);

	*o_su = su;
	return ranked_su_per_si;
}

/*****************************************************************************
 * Function: avd_sirankedsu_delete
 *
 * Purpose:  This function will delete and free AVD_SUS_PER_SI_RANK structure from 
 * the tree.
 *
 * Input: cb - the AVD control block
 *        ranked_su_per_si - The AVD_SUS_PER_SI_RANK structure that needs to be deleted.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE  
 *
 * NOTES:
 *
 * 
 **************************************************************************/
static uns32 avd_sirankedsu_delete(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK *ranked_su_per_si)
{
	if (ranked_su_per_si == NULL)
		return NCSCC_RC_FAILURE;

	if (ncs_patricia_tree_del(&sirankedsu_db, &ranked_su_per_si->tree_node)
	    != NCSCC_RC_SUCCESS) {
		/* log error */
		return NCSCC_RC_FAILURE;
	}
	avd_si_remove_rankedsu(ranked_su_per_si->sus_per_si_rank_on_si, &ranked_su_per_si->su_name);

	free(ranked_su_per_si);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sirankedsu_ccb_apply_create_hdlr
 *
 * Purpose: This routine handles create operations on SaAmfSIRankedSU objects.
 *
 *
 * Input  : Control Block, Ccb Util Oper Data.
 *
 * Returns: None.
 *
 * NOTES  : None.
 *
 *
 **************************************************************************/
static AVD_SUS_PER_SI_RANK * avd_sirankedsu_ccb_apply_create_hdlr(SaNameT *dn, 
		const SaImmAttrValuesT_2 **attributes)
{
        uns32 rank = 0;
	AVD_SUS_PER_SI_RANK *avd_sus_per_si_rank = NULL;
	SaNameT su_name;
	SaNameT si_name;
	AVD_SUS_PER_SI_RANK_INDX indx;

	immutil_getAttr("saAmfRank", attributes, 0, &rank);
	

	memset(&su_name, 0, sizeof(SaNameT));
	memset(&si_name, 0, sizeof(SaNameT));
	avd_susi_namet_init(dn, &su_name, &si_name);

	/* Find the avd_sus_per_si_rank name. */
	memset(&indx, '\0', sizeof(AVD_SUS_PER_SI_RANK_INDX));
	indx.si_name = si_name;
	indx.su_rank = rank;

	avd_sus_per_si_rank = avd_sirankedsu_create(avd_cb, indx);
	assert(avd_sus_per_si_rank);

	avd_sus_per_si_rank->su_name = su_name;

	return avd_sus_per_si_rank;
}

/*****************************************************************************
 * Function: avd_sirankedsu_del_si_list
 *
 * Purpose:  This function will del the given sus_per_si_rank from si list.
 *
 * Input: cb - the AVD control block
 *        sus_per_si_rank - The sus_per_si_rank pointer
 *
 * Returns: None.
 *
 * NOTES: None
 *
 *
 **************************************************************************/
static void avd_sirankedsu_del_si_list(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK *sus_per_si_rank)
{
	AVD_SUS_PER_SI_RANK *i_sus_per_si_rank = NULL;
	AVD_SUS_PER_SI_RANK *prev_sus_per_si_rank = NULL;

	if (sus_per_si_rank->sus_per_si_rank_on_si != NULL) {
		i_sus_per_si_rank = sus_per_si_rank->sus_per_si_rank_on_si->list_of_sus_per_si_rank;

		while ((i_sus_per_si_rank != NULL) && (i_sus_per_si_rank != sus_per_si_rank)) {
			prev_sus_per_si_rank = i_sus_per_si_rank;
			i_sus_per_si_rank = i_sus_per_si_rank->sus_per_si_rank_list_si_next;
		}

		if (i_sus_per_si_rank != sus_per_si_rank) {
			LOG_CR("SI '%s' having SU '%s' with rank %u, does not exist in sirankedsu link list", 
					sus_per_si_rank->indx.si_name.value, sus_per_si_rank->su_name.value,
					sus_per_si_rank->indx.su_rank);
		} else {
			if (prev_sus_per_si_rank == NULL) {
				sus_per_si_rank->sus_per_si_rank_on_si->list_of_sus_per_si_rank =
				    sus_per_si_rank->sus_per_si_rank_list_si_next;
			} else {
				prev_sus_per_si_rank->sus_per_si_rank_list_si_next =
				    sus_per_si_rank->sus_per_si_rank_list_si_next;
			}
		}

		sus_per_si_rank->sus_per_si_rank_list_si_next = NULL;
		sus_per_si_rank->sus_per_si_rank_on_si = NULL;
	}

	return;
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
        AVD_SI *avd_si = NULL;
	SaNameT su_name;
	SaNameT si_name;
        uns32 rank = 0;
	AVD_SUS_PER_SI_RANK_INDX indx;

        memset(&su_name, 0, sizeof(SaNameT));
        memset(&si_name, 0, sizeof(SaNameT));
        avd_susi_namet_init(dn, &su_name, &si_name);

        /* Find the si name. */
        avd_si = avd_si_get(&si_name);

        if (avd_si == NULL) {
                /* SI does not exist in current model, check CCB */
                if (opdata == NULL) {
                        LOG_ER("'%s' does not exist in model", si_name.value);
                        return 0;
                }

                if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &si_name) == NULL) {
                        LOG_ER("'%s' does not exist in existing model or in CCB", si_name.value);
                        return 0;
                }
        }

	if (immutil_getAttr("saAmfRank", attributes, 0, &rank) != SA_AIS_OK) {
		LOG_ER("saAmfRank not found for %s", dn->value);
		return 0;  
	}

	indx.si_name = si_name;
	indx.su_rank = rank;
	if ((avd_sirankedsu_find(avd_cb, indx)) != NULL ) {
		if (opdata != NULL) {
			LOG_ER("saAmfRankedSu exists %s, si'%s', rank'%u'", dn->value, si_name.value, rank);
			return 0;
		}
		return SA_AIS_OK;  
	}

        return SA_AIS_OK;
}

static void avd_susi_namet_init(const SaNameT *object_name, SaNameT *su_name, SaNameT *si_name)
{
	char *p = NULL;

	if (su_name) {
		SaNameT temp_name;
		int i;

		/* Take out Su Name. safRankedSu=safSu=SuName\,safSg=SgName\,
		safApp=AppName,safSi=SiName,safApp=AppName */
		temp_name = *object_name;
		p = strstr((char *)temp_name.value, "safSi=");
		*(--p) = '\0';	/* null terminate at comma before si name */

		/* Skip past the RDN tag */
		p = strchr((char *)temp_name.value, '=') + 1;
		assert(p);
		memset(su_name, 0, sizeof(SaNameT));
		/* Copy the RDN value which is a DN with escaped commas */
		i = 0;
		while (*p) {
			if (*p != '\\')
				su_name->value[i++] = *p;

			p++;
		}
		/* i Points just after SU name ends, so it will give the name length
		as it starts with zero. */
		su_name->length = i;
	}

	if (si_name) {
		memset(si_name, 0, sizeof(SaNameT));
		p = strstr((char *)object_name->value, "safSi=");
		si_name->length = strlen(p);
		memcpy(si_name->value, p, si_name->length);
	}
}

static void sirankedsu_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SUS_PER_SI_RANK *avd_sirankedsu = NULL;

        TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		avd_sirankedsu = avd_sirankedsu_ccb_apply_create_hdlr(&opdata->objectName, opdata->param.create.attrValues);
                avd_sirankedsu_db_add(avd_sirankedsu);

		break;
	case CCBUTIL_DELETE:
		/* delete and free the structure */
		avd_sirankedsu_del_si_list(avd_cb, opdata->userData);
		avd_sirankedsu_delete(avd_cb, opdata->userData);
		break;
	default:
		assert(0);
		break;
	}
	TRACE_LEAVE();
}

static int avd_sirankedsu_ccb_complete_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SI *si = NULL;
	SaNameT su_name;
	SaNameT si_name;
	AVD_SUS_PER_SI_RANK_INDX indx;
	AVD_SUS_PER_SI_RANK *su_rank_rec = 0;
	bool found = FALSE;

        TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	memset(&su_name, 0, sizeof(SaNameT));
	memset(&si_name, 0, sizeof(SaNameT));
	avd_susi_namet_init(opdata->param.deleteOp.objectName, &su_name, &si_name);

	/* determine if the su is ranked per si */
	memset((uns8 *)&indx, '\0', sizeof(indx));
	indx.si_name = si_name;
	indx.su_rank = 0;
	for (su_rank_rec = avd_sirankedsu_getnext(avd_cb, indx);
			su_rank_rec && (memcmp(&(su_rank_rec->indx.si_name), &si_name, sizeof(SaNameT))
				== 0);
			su_rank_rec = avd_sirankedsu_getnext(avd_cb, su_rank_rec->indx)) {
		if (memcmp(&su_rank_rec->su_name.value, &su_name.value, su_name.length) == 0) {
			found = TRUE;
			break;
		}
	}

	if (FALSE == found) {
		LOG_ER("'%s' not found", opdata->objectName.value);
		goto error;
	}

	/* Find the si name. */
	si = avd_si_get(&si_name);

	if (si == NULL) {
		LOG_ER("SI '%s' not found", si_name.value);
		goto error;
	}

	if (si != NULL) {
		/* SI should not be assigned while SI ranked SU needs to be deleted */
		if (si->list_of_sisu != NULL) {
			TRACE("Parent SI is in assigned state '%s'", si->name.value);
			goto error;
		}
	}
	opdata->userData = su_rank_rec; /* Save for later use in apply */

        TRACE_LEAVE2("%u", 1);
	return 1;
error:
        TRACE_LEAVE2("%u", 0);
	return 0;

}

static SaAisErrorT sirankedsu_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

        TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfSIRankedSU not supported");
		break;
	case CCBUTIL_DELETE:
		{
			if (avd_sirankedsu_ccb_complete_delete_hdlr(opdata))
				rc = SA_AIS_OK;
		}
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

SaAisErrorT avd_sirankedsu_config_get(SaNameT *si_name, AVD_SI *si)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSIRankedSU";
	AVD_SUS_PER_SI_RANK_INDX indx;
	SaNameT dn;
	AVD_SUS_PER_SI_RANK *avd_sirankedsu = NULL;

        TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, si_name, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
	      NULL, &searchHandle) != SA_AIS_OK) {

		LOG_ER("No objects found (1)");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		LOG_NO("'%s'", dn.value);

		indx.si_name = *si_name;
		if (immutil_getAttr("saAmfRank", attributes, 0, &indx.su_rank) != SA_AIS_OK) {
			LOG_ER("Get saAmfRank FAILED for '%s'", dn.value);
			goto done1;
		}

                if (!is_config_valid(&dn, attributes, NULL))
			goto done2;

		if ((avd_sirankedsu = avd_sirankedsu_find(avd_cb, indx)) == NULL) {
			if ((avd_sirankedsu = avd_sirankedsu_ccb_apply_create_hdlr(&dn, attributes)) == NULL)
				goto done2;

			avd_sirankedsu_db_add(avd_sirankedsu);
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

void avd_sirankedsu_constructor(void)
{
        NCS_PATRICIA_PARAMS patricia_params;

        patricia_params.key_size = sizeof(AVD_SUS_PER_SI_RANK_INDX);
        assert(ncs_patricia_tree_init(&sirankedsu_db, &patricia_params) == NCSCC_RC_SUCCESS);
	avd_class_impl_set("SaAmfSIRankedSU", NULL, NULL, sirankedsu_ccb_completed_cb, sirankedsu_ccb_apply_cb);
}

