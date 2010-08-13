/*      -*- OpenSAF  -*-
*
* (C) Copyright 2010 The OpenSAF Foundation
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
*                                                                            *
*  MODULE NAME:  plms_imm.c                                                  *
*                                                                            *
*                                                                            *
*  DESCRIPTION: This module contains the functionality of PLMS interface     * 
*		with IMM 						     *
*                                                                            *
*****************************************************************************/

#include <inttypes.h>

#include "plms.h"
#include "immutil.h"
#include "plms_utils.h"

typedef enum {
	PLMS_DOMAIN_OBJ_TYPE,
	PLMS_HPI_CFG_OBJ_TYPE,
	PLMS_HE_BASE_TYPE_OBJ_TYPE,
	PLMS_EE_BASE_TYPE_OBJ_TYPE,
	PLMS_HE_TYPE_OBJ_TYPE,
	PLMS_EE_TYPE_OBJ_TYPE,
	PLMS_HE_OBJ_TYPE,
	PLMS_EE_OBJ_TYPE,
	PLMS_DEPENDENCY_OBJ_TYPE,
}PLMS_OBJ_TYPE;

#define PLMS_MAX_IMM_API_RETRY_CNT 5 

static const SaImmOiImplementerNameT impl_name =
	(SaImmOiImplementerNameT)"safPlmService";
static SaVersionT imm_version = { 'A', 2, 1 };
static SaImmOiCallbacksT_2 imm_oi_cbks;
static SaImmCallbacksT imm_om_cbks;

extern PLMS_CB *plms_cb;

static void plms_reg_with_imm_as_oi();
static void plms_reg_with_imm_as_om();
static void plms_unreg_with_imm_as_om();
static void plms_handle_oi_init_try_again_err (SaImmOiHandleT *);
static void plms_handle_oi_sel_obj_get_try_again_err(SaSelectionObjectT *);
static void plms_handle_oi_impl_set_try_again_err();
static void plms_oi_class_impl_set(SaImmClassNameT); 
static void plms_handle_class_impl_set_try_again_err(SaImmClassNameT);
static void plms_handle_om_init_try_again_err (SaImmHandleT *);
static void plms_oi_class_impl_rel(SaImmClassNameT);
static void plms_unreg_with_imm_as_oi();
static void plms_handle_om_search_init_try_again_err(SaNameT *, SaImmScopeT, 
			SaImmSearchOptionsT, const SaImmSearchParametersT_2 *, 
			const SaImmAttrNameT *, SaImmSearchHandleT *);
static void plms_get_objects_from_imm (SaInt8T *, SaInt8T);
static void plms_create_objects(SaUint8T, SaNameT *, SaImmAttrValuesT_2 **);
static SaAisErrorT plms_imm_ccb_obj_create_cbk(SaImmOiHandleT, SaImmOiCcbIdT, 
					const SaImmClassNameT, const SaNameT *,
					const SaImmAttrValuesT_2 **);
static SaAisErrorT plms_imm_ccb_obj_delete_cbk(SaImmOiHandleT, SaImmOiCcbIdT,
					const SaNameT *);
static SaAisErrorT plms_imm_ccb_obj_modify_cbk(SaImmOiHandleT, SaImmOiCcbIdT,
					const SaNameT *,
					const SaImmAttrModificationT_2 **);
static SaAisErrorT plms_imm_ccb_completed_cbk(SaImmOiHandleT, SaImmOiCcbIdT);
static void plms_imm_ccb_apply_cbk(SaImmOiHandleT, SaImmOiCcbIdT);
static void plms_imm_ccb_abort_cbk (SaImmOiHandleT, SaImmOiCcbIdT);
static void plms_imm_admin_op_cbk (SaImmOiHandleT, SaInvocationT,
				const SaNameT *, SaImmAdminOperationIdT,
				const SaImmAdminOperationParamsT_2 **);
static SaAisErrorT validate_he_entity_paths_attr_val(PLMS_ENTITY *, 
					const SaImmAttrModificationT_2 *);
static SaAisErrorT validate_he_base_type_attr_val(SaImmOiCcbIdT, 
					const SaImmAttrValuesT_2 *);
static SaAisErrorT validate_ee_type_attr_val(SaImmOiCcbIdT, 
					const SaImmAttrValuesT_2 *);
static SaAisErrorT validate_deletion_of_he_base_type_obj(SaImmOiCcbIdT, 
						const SaNameT *);
static SaAisErrorT validate_deletion_of_ee_base_type_obj(SaImmOiCcbIdT, 
						const SaNameT *);
static SaAisErrorT validate_changes_to_he_type_obj(SaImmOiCcbIdT, 
				const SaNameT *, SaInt32T,
				const SaImmAttrModificationT_2 **);
static SaAisErrorT validate_changes_to_ee_type_obj(SaImmOiCcbIdT, 
						const SaNameT *);
static SaInt32T get_obj_type_from_obj_name(SaUint8T *);
static void plms_create_domain_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_hpi_cfg_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_dependency_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_he_base_type_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_he_type_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_ee_type_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_he_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_ee_base_type_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_create_ee_obj(SaNameT *, SaImmAttrValuesT_2 **);
static void plms_modify_domain_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_modify_dependency_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_modify_he_type_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_modify_ee_type_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_modify_he_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_modify_ee_obj(SaNameT *, SaImmAttrModificationT_2 **);
static void plms_delete_he_base_type_obj(SaNameT *); 
static void plms_delete_ee_base_type_obj(SaNameT *);
static void plms_delete_he_type_obj(SaNameT *);
static void plms_delete_ee_type_obj(SaNameT *);
static void plms_delete_dep_obj(SaNameT *);
static void plms_delete_domain_obj();
static void plms_delete_hpi_cfg_obj();
static SaInt32T add_ent2_to_dep_list_of_ent1(PLMS_ENTITY *, PLMS_ENTITY *);
static SaInt32T add_ent1_to_rev_dep_list_of_ent2(PLMS_ENTITY *, PLMS_ENTITY *);
static void plms_update_he_idr_data(PLMS_HE_TYPE_INFO *, SaStringT *);
static void rem_ent2_from_dep_list_of_ent1(PLMS_ENTITY *, PLMS_ENTITY *);
static void rem_ent1_from_rev_dep_list_of_ent2(PLMS_ENTITY *, PLMS_ENTITY *);
static void remove_ent_from_rev_dep_list_of_others(PLMS_ENTITY *);
static void free_dep_list(PLMS_ENTITY *);
static void free_he_idr_data(PLMS_HE_TYPE_INFO *);
static void plms_delete_plm_ent(PLMS_ENTITY *);
static void plms_check_n_del_ent_from_ent_groups(PLMS_ENTITY *);
static void free_entity_group_info_list(PLMS_ENTITY_GROUP_INFO_LIST *);
static void free_track_info(PLMS_TRACK_INFO *);
static void plms_free_he_type_obj(PLMS_HE_TYPE_INFO *);
static void plms_free_ee_type_obj(PLMS_EE_TYPE_INFO *);
static void free_entity_group_info(PLMS_ENTITY_GROUP_INFO *);
static void free_ckpt_track_step_info(PLMS_CKPT_TRACK_STEP_INFO *);
static void plms_ent_from_grp_ent_list_rem(PLMS_ENTITY *, 
					PLMS_GROUP_ENTITY_ROOT_LIST **);
static void free_plms_group_entity_list(PLMS_GROUP_ENTITY_ROOT_LIST *) ;


SaUint32T plms_imm_intf_initialize()
{
	plms_reg_with_imm_as_oi();
	plms_reg_with_imm_as_om();
	plms_unreg_with_imm_as_om();
	return NCSCC_RC_SUCCESS;
}
static void plms_reg_with_imm_as_oi()
{
	SaImmOiHandleT imm_oi_hdl;
	SaImmOiCallbacksT_2 imm_oi_cbks;
	SaAisErrorT error=SA_AIS_OK;
	SaSelectionObjectT sel_obj;

	TRACE_ENTER();

	imm_oi_cbks.saImmOiAdminOperationCallback = plms_imm_admin_op_cbk;
	imm_oi_cbks.saImmOiCcbAbortCallback = plms_imm_ccb_abort_cbk;
	imm_oi_cbks.saImmOiCcbApplyCallback = plms_imm_ccb_apply_cbk;
	imm_oi_cbks.saImmOiCcbCompletedCallback = plms_imm_ccb_completed_cbk;
	imm_oi_cbks.saImmOiCcbObjectCreateCallback= plms_imm_ccb_obj_create_cbk;
	imm_oi_cbks.saImmOiCcbObjectDeleteCallback= plms_imm_ccb_obj_delete_cbk;
	imm_oi_cbks.saImmOiCcbObjectModifyCallback= plms_imm_ccb_obj_modify_cbk;
	imm_oi_cbks.saImmOiRtAttrUpdateCallback = NULL;
	error = saImmOiInitialize_2(&imm_oi_hdl, &imm_oi_cbks, &imm_version);
	if (error != SA_AIS_OK) {
		LOG_ER("ImmOiInit returned error %u", error);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again */
			plms_handle_oi_init_try_again_err(&imm_oi_hdl);
		}
	}
	plms_cb->oi_hdl = imm_oi_hdl;
	TRACE_2("ImmOiInit successful");
	error = saImmOiSelectionObjectGet(imm_oi_hdl, &sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("ImmOiSelObjGet returned error %u", error);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again */
			plms_handle_oi_sel_obj_get_try_again_err(&sel_obj);
		}
	}
	plms_cb->imm_sel_obj = sel_obj;
	TRACE_2("ImmOiSelObjGet successful");
	error = saImmOiImplementerSet(imm_oi_hdl, impl_name);
	if (error != SA_AIS_OK) {
		LOG_ER("ImmOiImplSet returned error %u", error);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again */
			plms_handle_oi_impl_set_try_again_err();
		}
	}
	plms_oi_class_impl_set("SaPlmDomain");
	plms_oi_class_impl_set("SaHpiConfig");
	plms_oi_class_impl_set("SaPlmHEBaseType");
	plms_oi_class_impl_set("SaPlmHEType");
	plms_oi_class_impl_set("SaPlmHE");
	plms_oi_class_impl_set("SaPlmEEBaseType");
	plms_oi_class_impl_set("SaPlmEEType");
	plms_oi_class_impl_set("SaPlmEE");
	plms_oi_class_impl_set("SaPlmDependency");
	TRACE_LEAVE();
}
static void plms_reg_with_imm_as_om()
{
	SaAisErrorT error;
	SaImmHandleT imm_om_hdl;

	TRACE_ENTER();
	imm_om_cbks.saImmOmAdminOperationInvokeCallback=NULL;
	/* Initialise with IMM as OM */
	error = saImmOmInitialize(&imm_om_hdl, &imm_om_cbks, &imm_version);
	if (error != SA_AIS_OK) {
		/* Log the error */
		LOG_ER("ImmOmInit returned error %u", error);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again */
			plms_handle_om_init_try_again_err(&imm_om_hdl);
		}
	}
	plms_cb->imm_hdl = imm_om_hdl;
	TRACE_2("OmInit successful");
	/* No need to call saImmOmSelectionObjectGet() */
        plms_get_objects_from_imm("safDomain", PLMS_DOMAIN_OBJ_TYPE);
        plms_get_objects_from_imm("safHpiCfg", PLMS_HPI_CFG_OBJ_TYPE);
        plms_get_objects_from_imm("safHEType", PLMS_HE_BASE_TYPE_OBJ_TYPE);
        plms_get_objects_from_imm("safEEType", PLMS_EE_BASE_TYPE_OBJ_TYPE);
        plms_get_objects_from_imm("safVersion", PLMS_HE_TYPE_OBJ_TYPE);
        plms_get_objects_from_imm("safHE", PLMS_HE_OBJ_TYPE);
        plms_get_objects_from_imm("safEE", PLMS_EE_OBJ_TYPE);
        plms_get_objects_from_imm("safDependency", PLMS_DEPENDENCY_OBJ_TYPE);
	plms_cb->is_initialized = TRUE;
	TRACE_LEAVE();
}
static void plms_unreg_with_imm_as_om()
{
	SaAisErrorT error;
	TRACE_ENTER();
	error = saImmOmFinalize(plms_cb->imm_hdl);
	if (error != SA_AIS_OK) {
		LOG_ER("OmFinalize returned error %u", error);
	}
	plms_cb->imm_hdl = 0;
	TRACE_LEAVE();
}
static void plms_handle_oi_init_try_again_err (SaImmOiHandleT *imm_oi_hdl)
{
	SaUint8T retry_cnt = 0;
	SaAisErrorT error;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OiInit retry count reached max, asserting now");
			assert(0);
		}
		sleep(1);
		error = saImmOiInitialize_2(imm_oi_hdl, &imm_oi_cbks, 
			&imm_version);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OiInit Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OiInit returned error %u, so asserting now", error);
		assert(0);
	}
	TRACE_LEAVE2("OiInit successful");
}
static void plms_handle_oi_sel_obj_get_try_again_err(SaSelectionObjectT 
							*sel_obj)
{
	SaUint8T retry_cnt = 0;
	SaAisErrorT error;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OiSelObjGet retry count reached max, asserting \
				now");
			assert(0);
		}
		sleep(1);
		error = saImmOiSelectionObjectGet(plms_cb->oi_hdl, sel_obj);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OiSelObjGet Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OiSelObjGet returned error %u, asserting now", error);
		assert(0);
	}
	TRACE_LEAVE2("OiSelObjGet successful");
}
static void plms_handle_oi_impl_set_try_again_err()
{
	SaUint8T retry_cnt = 0;
	SaAisErrorT error;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OiImplSet retry count reached max, asserting \
			now");
			assert(0);
		}
		sleep(1);
		error = saImmOiImplementerSet(plms_cb->oi_hdl, impl_name);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OiImplSet Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OiImplSet returned error %u, so asserting now", error);
		assert(0);
	}
	TRACE_LEAVE2("OiImplSet successful with impl_name %s", impl_name);
}
static void plms_oi_class_impl_set(SaImmClassNameT class_name) 
{
	SaAisErrorT error;
	TRACE_ENTER();
	error = saImmOiClassImplementerSet(plms_cb->oi_hdl, class_name);
	if (error != SA_AIS_OK) {
		LOG_ER("ImmOiClassImplSet for class %s returned error %u", 
						class_name, error);
		if (error == SA_AIS_ERR_NOT_EXIST) {
			return;
		}
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again error */
			plms_handle_class_impl_set_try_again_err(class_name);
		}
	}
	TRACE_LEAVE2("ImmOiClassImplSet successful for class_name %s", 
			class_name);
}
static void plms_handle_class_impl_set_try_again_err(SaImmClassNameT class_name)
{
	SaAisErrorT error;
	SaUint8T retry_cnt = 0;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OiClassImplSet retry count reached max, \
				asserting now");
			assert(0);
		}
		sleep(1);
		error = saImmOiClassImplementerSet(plms_cb->oi_hdl, class_name);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OiClassImplSet Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OiClassImplSet returned error %u, asserting now",error);
		assert(0);
	}
	TRACE_LEAVE();
}
static void plms_handle_om_init_try_again_err (SaImmHandleT *imm_om_hdl)
{
	SaUint8T retry_cnt = 0;
	SaAisErrorT error;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("ImmOmInit retry count reached max, asserting \
				now %u", error);
			assert(0);
		}
		sleep(1);
		error = saImmOmInitialize(imm_om_hdl, &imm_om_cbks, 
			&imm_version);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OmInit Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OmInit returned error %u, asserting now", error);
		assert(0);
	}
	TRACE_LEAVE();
}
static void plms_oi_class_impl_rel(SaImmClassNameT class_name)
{
	SaAisErrorT error;
	TRACE_ENTER();
	error = saImmOiClassImplementerRelease(plms_cb->oi_hdl, class_name);
	if (error != SA_AIS_OK) {
		LOG_ER("OiClassImplRelease for class %s returned error %u", 
						class_name, error);
	}
	TRACE_LEAVE();
}
static void plms_unreg_with_imm_as_oi()
{
	SaAisErrorT error;
	TRACE_ENTER();
	plms_oi_class_impl_rel("SaPlmDependency");
	plms_oi_class_impl_rel("SaPlmEE");
	plms_oi_class_impl_rel("SaPlmEEType");
	plms_oi_class_impl_rel("SaPlmEEBaseType");
	plms_oi_class_impl_rel("SaPlmHE");
	plms_oi_class_impl_rel("SaPlmHEType");
	plms_oi_class_impl_rel("SaPlmHEBaseType");
	plms_oi_class_impl_rel("SaPlmDomain");
	plms_oi_class_impl_rel("SaHpiConfig");
	error = saImmOiImplementerClear(plms_cb->oi_hdl);
	if (error != SA_AIS_OK) {
		LOG_ER("OiImplClear returned error %u", error);
	}
	error = saImmOiFinalize(plms_cb->oi_hdl);
	if (error != SA_AIS_OK) {
		LOG_ER("OiFinalize returned error %u", error);
	}
	plms_cb->oi_hdl = 0;
	TRACE_LEAVE2("Unregister as OI successful");
}
static void plms_handle_om_search_init_try_again_err(SaNameT *root_name, 
			SaImmScopeT scope, SaImmSearchOptionsT search_opts,
			const SaImmSearchParametersT_2 *search_param, 
			const SaImmAttrNameT *attr_names, 
			SaImmSearchHandleT *search_hdl)
{
	SaAisErrorT error;
	SaUint8T retry_cnt = 0;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OmSearchInit retry count reached max, \
				asserting now");
			assert(0);
		}
		sleep(1);
		error = saImmOmSearchInitialize_2(plms_cb->imm_hdl, root_name, 
					scope, search_opts, search_param, 
					attr_names, search_hdl);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OmSearchInit Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OmSearchInit returned error %u, asserting now", error);
		assert(0);
	}
	TRACE_LEAVE();
}
static void plms_handle_om_search_next_try_again_err(SaImmSearchHandleT 
				search_hdl, SaNameT *obj_name, 
				SaImmAttrValuesT_2 ***attrs)
{
	SaAisErrorT error;
	SaUint8T retry_cnt = 0;
	TRACE_ENTER();
	do {
		if (retry_cnt == PLMS_MAX_IMM_API_RETRY_CNT) {
			LOG_CR("OmSearchNext retry count reached max, asserting\
				now");
			assert(0);
		}
		sleep(1);
		error = saImmOmSearchNext_2(search_hdl, obj_name, attrs);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			break; /* from the do while */
		}
		++retry_cnt;
		LOG_ER("OmSearchNext Retry count %u", retry_cnt);
	}while (1);
	if (error != SA_AIS_OK) {
		LOG_CR("OmSearchNext returned error %u, asserting now", error);
		assert(0);
	}
	TRACE_LEAVE();
}
static void plms_get_objects_from_imm (SaInt8T *rdn_attr_name, SaInt8T obj_type)
{
	SaNameT *root_name = NULL;
	SaImmScopeT scope = SA_IMM_SUBTREE;
	SaImmSearchOptionsT search_opts = SA_IMM_SEARCH_ONE_ATTR | 
					SA_IMM_SEARCH_GET_ALL_ATTR ;
	SaImmSearchParametersT_2 search_param;
	SaImmAttrNameT *attr_names = NULL;
	SaImmSearchHandleT search_hdl;
	SaNameT obj_name;
	SaImmAttrValuesT_2 **attrs;
	SaAisErrorT error;

	TRACE_ENTER();
	/* Get all the objects having given rdn_attr_name using searchAPIs */
	search_param.searchOneAttr.attrName  = rdn_attr_name;
	search_param.searchOneAttr.attrValue = NULL;
	error = saImmOmSearchInitialize_2(plms_cb->imm_hdl, root_name, scope,
					search_opts, &search_param, attr_names,
					&search_hdl);
	if (error != SA_AIS_OK) {
		LOG_ER("OmSearchInit returned error %u", error);
		if (error != SA_AIS_ERR_TRY_AGAIN) {
			LOG_CR("asserting here");
			assert(0);
		}
		else { /* try_again */
			plms_handle_om_search_init_try_again_err(root_name, 
					scope, search_opts, &search_param, 
					attr_names, &search_hdl);
		}
	}
	TRACE_2("OmSearchInit successful");
	do {
		error = saImmOmSearchNext_2(search_hdl, &obj_name, &attrs);
		if (error != SA_AIS_OK) {
			TRACE_2("OmSearchNext returned error %u", error);
			if (error == SA_AIS_ERR_NOT_EXIST) {
				TRACE_2("All objects matching search criteria \
				were returned");
				break;
			}
			if (error != SA_AIS_ERR_TRY_AGAIN) {
				LOG_CR("saImmOmSearchNext_2 returned error %d,\
					calling assert", error);
				assert(0);
			}
			else { /* try_again */
				plms_handle_om_search_next_try_again_err(
				search_hdl, &obj_name, &attrs);
			}
		}
		TRACE_2("OmSearchNext successful for object");
		plms_create_objects(obj_type, &obj_name, attrs);
	}while(1);
	TRACE_2("OmSearchNext completed");
	error = saImmOmSearchFinalize(search_hdl);
	if (error != SA_AIS_OK) {
		TRACE_LEAVE2("OmSearchFinalize returned error %u", error);
		return;
	}
	TRACE_LEAVE2("OmSearchFinalize successful");
}
static void plms_create_objects(SaUint8T obj_type, SaNameT *obj_name, 
				SaImmAttrValuesT_2 **attrs)
{

	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER2("obj_type: %u", obj_type);
	switch (obj_type) {
	case PLMS_DOMAIN_OBJ_TYPE:
		plms_create_domain_obj(obj_name, attrs);
		break;
	case PLMS_HPI_CFG_OBJ_TYPE:
		plms_create_hpi_cfg_obj(obj_name, attrs);
		break;
	case PLMS_DEPENDENCY_OBJ_TYPE:	
		plms_create_dependency_obj(obj_name, attrs);
		break;
	case PLMS_HE_BASE_TYPE_OBJ_TYPE:
		plms_create_he_base_type_obj(obj_name, attrs);
		break;
	case PLMS_HE_TYPE_OBJ_TYPE:
	case PLMS_EE_TYPE_OBJ_TYPE:
		memcpy(dn_name, obj_name->value, obj_name->length);

		if (strstr(dn_name, "safHEType") != NULL) {
			plms_create_he_type_obj(obj_name, attrs);
		}
		else if (strstr(dn_name, "safEEType") != NULL) {
			plms_create_ee_type_obj(obj_name, attrs);
		}
		break;
	case PLMS_HE_OBJ_TYPE:
		plms_create_he_obj(obj_name, attrs);
		break;
	case PLMS_EE_BASE_TYPE_OBJ_TYPE:
		plms_create_ee_base_type_obj(obj_name, attrs);
		break;
	case PLMS_EE_OBJ_TYPE:
		plms_create_ee_obj(obj_name, attrs);
		break;
	}
	TRACE_LEAVE();
}
void plms_modify_objects(SaInt32T obj_type, SaNameT *obj_name, 
			SaImmAttrModificationT_2 **attr_mods)
{
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER2("obj_type: %u", obj_type);
	switch (obj_type) {
	
	case PLMS_DOMAIN_OBJ_TYPE:
		plms_modify_domain_obj(obj_name, attr_mods);
		break;

	case PLMS_DEPENDENCY_OBJ_TYPE:	
		plms_modify_dependency_obj(obj_name, attr_mods);
		break;

	case PLMS_HE_BASE_TYPE_OBJ_TYPE:
		/* This object cannot be modified */
		break;
		
	case PLMS_HE_TYPE_OBJ_TYPE:
	case PLMS_EE_TYPE_OBJ_TYPE:
		memcpy(dn_name, obj_name->value, obj_name->length);

		if (strstr(dn_name, "safHEType") != NULL) {
			plms_modify_he_type_obj(obj_name, attr_mods);
		}
		else if (strstr(dn_name, "safEEType") != NULL) {
			plms_modify_ee_type_obj(obj_name, attr_mods);
		}
		break;
	case PLMS_HE_OBJ_TYPE:
		plms_modify_he_obj(obj_name, attr_mods);
		break;

	case PLMS_EE_BASE_TYPE_OBJ_TYPE:
		/* This object cannot be modified */
		break;

	case PLMS_EE_OBJ_TYPE:
		plms_modify_ee_obj(obj_name, attr_mods);
		break;
	}
	TRACE_LEAVE();
}
static void plms_delete_objects(SaInt32T obj_type, SaNameT *obj_name)
{
	PLMS_GROUP_ENTITY *child_list, *temp;
	PLMS_ENTITY *plm_ent, *parent, *tmp;
	SaUint32T rc;
	TRACE_ENTER();
	switch (obj_type) {
	case PLMS_HE_OBJ_TYPE:
	case PLMS_EE_OBJ_TYPE:
		/* Get the node from the EE patricia tree */
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_get(
				&plms_cb->entity_info, (SaUint8T *)obj_name);
		/* No need to check if plm_ent is null */
		/* Update the parent/child relationship */
		parent = plm_ent->parent;
		if (parent == NULL) {
			/* Parent is domain object */
			if (plms_cb->domain_info.leftmost_child == plm_ent) {
				plms_cb->domain_info.leftmost_child = plm_ent->right_sibling;
			}
			else {
				/* traverse the children of the parent */
				tmp = plms_cb->domain_info.leftmost_child;
				while (tmp->right_sibling != plm_ent) {
					tmp = tmp->right_sibling;
				}
				tmp->right_sibling = plm_ent->right_sibling;
			}
		}
		else 
		{
			if (parent->leftmost_child == plm_ent) {
				parent->leftmost_child = plm_ent->right_sibling;
			}
			else {
				/* traverse the children of the parent */
				tmp = parent->leftmost_child;
				while (tmp->right_sibling != plm_ent) {
					tmp = tmp->right_sibling;
				}
				tmp->right_sibling = plm_ent->right_sibling;
			}
		}
		/* Get the child list of the target entity plm_ent */
		rc = plms_chld_get(plm_ent->leftmost_child, &child_list);
		if (rc == NCSCC_RC_FAILURE) {
			/* Log the error */
			TRACE_LEAVE2("plms_chld_get() for entity %s returned \
			%u", plm_ent->dn_name_str, rc);
			assert(0);
		}
		/* Delete/Free all the child entities */
		temp = child_list;
		while (temp != NULL) {
			plms_delete_plm_ent(temp->plm_entity);
			temp = temp->next;
			free(child_list);
			child_list = temp;
		}
		/* Delete/Free the plm_ent */
		plms_delete_plm_ent(plm_ent);
		break;
	case PLMS_HE_BASE_TYPE_OBJ_TYPE:
		plms_delete_he_base_type_obj(obj_name);
		break;
	case PLMS_EE_BASE_TYPE_OBJ_TYPE:
		plms_delete_ee_base_type_obj(obj_name);
		break;
	case PLMS_HE_TYPE_OBJ_TYPE:
		plms_delete_he_type_obj(obj_name);
		break;
	case PLMS_EE_TYPE_OBJ_TYPE:
		plms_delete_ee_type_obj(obj_name);
		break;
	case PLMS_DEPENDENCY_OBJ_TYPE:
		plms_delete_dep_obj(obj_name);
		break;
	case PLMS_DOMAIN_OBJ_TYPE:
		plms_delete_domain_obj();
		break;
	case PLMS_HPI_CFG_OBJ_TYPE:
		plms_delete_hpi_cfg_obj();
		break;
	}
	TRACE_LEAVE();
}
static SaAisErrorT plms_imm_ccb_obj_create_cbk(SaImmOiHandleT imm_oi_hdl,
					SaImmOiCcbIdT ccb_id, 
					const SaImmClassNameT class_name,
					const SaNameT *parent_name,
					const SaImmAttrValuesT_2 **attr)
{
	/* Not checking for errors like parent object not exists, class name 
	   not exists or object already exists as IMM will return such errors 
	   as return values of IMM OM API */
	CcbUtilCcbData_t *ccb_util_ccb_data;
	CcbUtilOperationData_t *operation;
	SaInt8T *obj_rdn = NULL;
	SaAisErrorT rc;
	SaUint8T j, k, dep_names_num = 0, dep_min_num = 0;
	SaPlmHEDeactivationPolicyT *attr_value;
	SaUint32T len;
	SaNameT *dep_name;
	PLMS_ENTITY *dep_node;

	TRACE_ENTER2("ccb_id: %llu, class_name: %s", ccb_id, class_name);
	if (strcmp(class_name, "SaPlmDomain") == 0) {
		if (plms_cb->domain_info.domain.safDomain != NULL) {
			TRACE_LEAVE2("Domain object already exists");
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strcmp(class_name, "SaHpiConfig") == 0) {
		if (plms_cb->hpi_cfg.safHpiCfg != NULL) {
			TRACE_LEAVE2("HPI Config object already exists");
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strcmp(class_name, "SaPlmDependency") == 0) {
		for (j=0; attr[j] != NULL; j++) {	
			if(strcmp((attr[j])->attrName, "saPlmDepNames") == 0) {
				dep_names_num = (attr[j])->attrValuesNumber;
				for (k=0; k < dep_names_num; k++) {
					dep_name = (SaNameT *)*((attr[j]->
							attrValues)+k);
					dep_node = (PLMS_ENTITY *)
					ncs_patricia_tree_get(&plms_cb->
					entity_info, (SaUint8T *)dep_name);
					if (dep_node == NULL) {
						TRACE_2("DepName not exists"); 
						return SA_AIS_ERR_BAD_OPERATION;
					}
				}
			}
			else if (strcmp((attr[j])->attrName, 
					"saPlmDepMinNumber") ==0) {
				dep_min_num = *(SaUint32T *)*((attr[j])->
						attrValues);
			}
		}
		if (dep_min_num > dep_names_num) {
			TRACE_LEAVE2("min no of dependencies: %u, \
			num_dep_names: %u", dep_min_num, dep_names_num);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	for (j=0; attr[j] != NULL; j++) {
		if (*((attr[j])->attrValues) == NULL) {
			/* Log the error */
			TRACE_LEAVE2("attr_val is null for attr_name %s", 
				(*attr)->attrName);
			return SA_AIS_ERR_BAD_OPERATION;
		}
		if (strcmp((attr[j])->attrName, "saPlmHEBaseHEType")== 0) {
			rc = validate_he_base_type_attr_val(ccb_id, attr[j]);
			if (rc != SA_AIS_OK) {
				TRACE_LEAVE2("he_base_type attr validation \
					returned error %u", rc);
				return rc;
			}
		}
		else if (strcmp((attr[j])->attrName, "saPlmHEEntityPaths")== 0) {
			if ((attr[j])->attrValuesNumber > SAHPI_MAX_ENTITY_PATH) {
				/* Log the error */
				TRACE_LEAVE2("No of values in \
					saPlmHEEntityPaths is %u, which is out \
					of range", (attr[j])->attrValuesNumber);
				return SA_AIS_ERR_BAD_OPERATION;
			}
		}
		else if (strcmp((attr[j])->attrName, "saPlmHetIdr")== 0) {
			if ((attr[j])->attrValuesNumber > 1) {
				/* Log the error */
				TRACE_LEAVE2("No of values in \
					saPlmHetIdr is %u, which is out \
					of range", (attr[j])->attrValuesNumber);
				return SA_AIS_ERR_BAD_OPERATION;
			}
		}
		else if (strcmp((attr[j])->attrName, "saPlmEEType")== 0) {
			rc = validate_ee_type_attr_val(ccb_id, attr[j]);
			if (rc != SA_AIS_OK) {
				/* Log the error */
				TRACE_LEAVE2("ee_type attr validation returned \
					error %u", rc);
				return rc;
			}
		}
		else if(strcmp((attr[j])->attrName, "saPlmHEDeactivationPolicy")
			== 0) {
			attr_value = (SaPlmHEDeactivationPolicyT *)
					*((attr[j])->attrValues);
			if ((*attr_value < SA_PLM_DP_REJECT_NOT_OOS) || 
				(*attr_value > SA_PLM_DP_UNCONDITIONAL)) {
				TRACE_LEAVE2("attr_val %u is out-of-range for \
					saPlmHEDeactivationPolicy",*attr_value);
				return SA_AIS_ERR_BAD_OPERATION;
			}
		}
		else if (strncmp((attr[j])->attrName, "saf", 3) == 0) {
			/* RDN attribute, store the name and value */
			obj_rdn = *(SaStringT *)*((attr[j])->attrValues);
			TRACE_2("Object RDN: %s", obj_rdn);
		}
	}
	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) == NULL) {
		TRACE_LEAVE2("ccbutil_getCcbData returned null");
		return SA_AIS_ERR_NO_MEMORY;
	}
	operation = ccbutil_ccbAddCreateOperation(ccb_util_ccb_data, 
			class_name, parent_name, attr);
	if (operation == NULL) {
		TRACE_LEAVE2("ccbutil_ccbAddCreateOperation returned null");
		return SA_AIS_ERR_NO_MEMORY;
	}
	/* Fill the operation->objectName as the ccbUtil create routine will
	   not do that */
	if ((parent_name != NULL) && (parent_name->length > 0)) {
		len = strlen(obj_rdn) + parent_name->length + 2;
		operation->objectName.length = snprintf((SaInt8T *)operation->objectName.
				value, len, "%s,%s", obj_rdn, (SaInt8T *)parent_name->value);
	}
	else {
		operation->objectName.length = sprintf((SaInt8T *)operation->objectName.
				value, "%s", obj_rdn);
	}
	TRACE_LEAVE();
	return SA_AIS_OK;
}
static SaAisErrorT plms_imm_ccb_obj_delete_cbk(SaImmOiHandleT imm_oi_hdl,
					SaImmOiCcbIdT ccb_id,
					const SaNameT *obj_name)
{
	struct CcbUtilCcbData *ccb_util_ccb_data;
	PLMS_ENTITY *plm_ent;
	PLMS_HE_BASE_INFO *plm_he_base_info; 
	PLMS_EE_BASE_INFO *plm_ee_base_info; 
	SaStringT rdn_val;
	SaUint32T rc;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};

	memcpy(dn_name, obj_name->value, obj_name->length);
	TRACE_ENTER2("ccb_id: %llu", ccb_id);
	if (strncmp((SaInt8T *)obj_name->value, "safHEType", 9) == 0) {
		plm_he_base_info = (PLMS_HE_BASE_INFO *)
		ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *)
					obj_name);
		if (plm_he_base_info == NULL) {
			TRACE_LEAVE2("obj does not exist");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		rc = validate_deletion_of_he_base_type_obj(ccb_id, obj_name);
		if (rc != SA_AIS_OK) {
			TRACE_LEAVE2("he_base_type obj validation returned \
						error %u", rc);
			return rc;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safHE", 5) == 0) {
		plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *)
					obj_name);
		if (plm_ent == NULL) {
			TRACE_LEAVE2("obj does not exist");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		if ((plm_ent->entity.he_entity.saPlmHEReadinessState != 
			SA_PLM_READINESS_OUT_OF_SERVICE) || (plm_ent->
			rev_dep_list != NULL)) {
			TRACE_LEAVE2("readiness state: %u, rev_dep_list: %" PRIdPTR ", \
				deletion cannot be done", 
				plm_ent->entity.he_entity.saPlmHEReadinessState,
				(intptr_t)plm_ent->rev_dep_list);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safEEType", 9) == 0) {
		plm_ee_base_info = (PLMS_EE_BASE_INFO *)
		ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)
					obj_name);
		if (plm_ee_base_info == NULL) {
			TRACE_LEAVE2("obj does not exist");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		rc = validate_deletion_of_ee_base_type_obj(ccb_id, obj_name);
		if (rc != SA_AIS_OK) {
			TRACE_LEAVE2("ee_base_type obj validation returned \
						error %u", rc);
			return rc;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safEE", 5) == 0) {
		plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *)
					obj_name);
		if (plm_ent == NULL) {
			TRACE_LEAVE2("obj does not exist");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		if ((plm_ent->entity.ee_entity.saPlmEEReadinessState != 
			SA_PLM_READINESS_OUT_OF_SERVICE) || (plm_ent->
			rev_dep_list != NULL)) {
			TRACE_LEAVE2("readiness state: %u, rev_dep_list: %" PRIdPTR ", \
				deletion cannot be done", 
				plm_ent->entity.ee_entity.saPlmEEReadinessState,
				(intptr_t)plm_ent->rev_dep_list);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safVersion", 10) == 0) {
		/* check if this is SaPlmHEType or SaPlmEEType object */
		if (strstr(dn_name, "safHEType") != NULL) {
			rc = validate_changes_to_he_type_obj(ccb_id, obj_name,
			CCBUTIL_DELETE, NULL);
			if (rc != SA_AIS_OK) {
				TRACE_LEAVE2("he_type obj validation returned \
						error %u", rc);
				return rc;
			}
		}	
		else if (strstr(dn_name, "safEEType") != NULL) {
			rc = validate_changes_to_ee_type_obj(ccb_id, obj_name);
			if (rc != SA_AIS_OK) {
				TRACE_LEAVE2("he_type obj validation returned \
						error %u", rc);
				return rc;
			}
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safDomain", 9) == 0) {
		rdn_val = strtok(dn_name, "\0");
		if ((strcmp(plms_cb->domain_info.domain.safDomain, rdn_val)!= 0)
			|| (plms_cb->domain_info.leftmost_child != NULL)) {
			TRACE_LEAVE2("leftmost_child ptr of domain: %" PRIdPTR, 
				(intptr_t)plms_cb->domain_info.leftmost_child);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safHpiCfg", 9) == 0) {
		rdn_val = strtok(dn_name, "\0");
		if ((strcmp(plms_cb->hpi_cfg.safHpiCfg, rdn_val) != 0)
			|| (plms_cb->domain_info.leftmost_child != NULL)) {
			TRACE_LEAVE2("leftmost_child ptr of domain: %" PRIdPTR, 
				(intptr_t)plms_cb->domain_info.leftmost_child);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (strncmp((SaInt8T *)obj_name->value, "safDependency", 13)==0) {
		/* This can be allowed to go through, log the same if reqd */
	}
	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != NULL) {
		/* memorize the request */
		ccbutil_ccbAddDeleteOperation(ccb_util_ccb_data, obj_name);
	} 
	else {
		TRACE_LEAVE2("ccbutil_getCcbData() returned null");
		return SA_AIS_ERR_NO_MEMORY;
	}
	TRACE_LEAVE();
	return SA_AIS_OK;
}
static SaAisErrorT plms_imm_ccb_obj_modify_cbk(SaImmOiHandleT imm_oi_hdl,
					SaImmOiCcbIdT ccb_id,
					const SaNameT *obj_name,
				const SaImmAttrModificationT_2 **attr_mods)
{
	struct CcbUtilCcbData *ccb_util_ccb_data;
	PLMS_ENTITY *plm_ent, *dep_node;
	SaUint8T j, k;
	SaUint32T parent_dn_len, *attr_value;
	SaStringT rdn_val, rdn, parent_dn;
	SaNameT key_dn, *dep_name;
	SaAisErrorT rc;
	SaUint8T dep_names_num=0, dep_min_num=0, cur_names_num=0;
	PLMS_GROUP_ENTITY *tmp_dep;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};

	memcpy(dn_name, obj_name->value, obj_name->length);
	TRACE_ENTER2("ccb_id: %llu", ccb_id);
	if (memcmp(obj_name->value, "safHEType", 9)==0) {
		/* No writable attributes in this obj, cannot be modified */
		/* IMM does not take care of this..? */
		TRACE_2("HEBaseType object cannot be modified");
		return SA_AIS_ERR_BAD_OPERATION;
	}
	else if (memcmp(obj_name->value, "safHE", 5) == 0) {
		plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *)
					obj_name);
		if (plm_ent == NULL) {
			TRACE_LEAVE2("obj does not exist");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		if (plm_ent->entity.he_entity.saPlmHEPresenceState != 
			SA_PLM_HE_PRESENCE_NOT_PRESENT) {
			TRACE_LEAVE2("HE's presence state: %u, modify cannot be\
				done", plm_ent->entity.he_entity.
					saPlmHEPresenceState);
			return SA_AIS_ERR_BAD_OPERATION;
		}
		else {
			/* saPlmHEBaseHEType attr cannot be modified */
			for (j=0; attr_mods[j] != NULL; j++) {
				if (strcmp(attr_mods[j]->modAttr.attrName, 
				"saPlmHEBaseHEType")==0) {
					/* This attr cannot be modified, IMM
					   itself will return error, without
					   forwarding the request to PLM.*/
					TRACE_LEAVE2("saPlmHEBaseHEType attr\
						cannot be modified");   
					return SA_AIS_ERR_BAD_OPERATION;
				}
				else if (strcmp(attr_mods[j]->modAttr.attrName,
				"saPlmHEEntityPaths")==0) {
					rc = validate_he_entity_paths_attr_val(
					plm_ent, attr_mods[j]);
					if (rc != SA_AIS_OK) {
						TRACE_LEAVE2("he_ent_paths \
						attr validation failed with \
						error %u", rc); 
						return rc;
					}
				}
			}
		}
	}
	else if (memcmp(obj_name->value, "safEEType", 9)==0) {
		/* No writable attributes in this obj, cannot be modified */
		/* IMM does not take care of this..? */
		TRACE_2("EEBaseType object cannot be modified");
		return SA_AIS_ERR_BAD_OPERATION;
	}
	else if (memcmp(obj_name->value, "safEE", 5) == 0) {
		plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *)
					obj_name);
		if (NULL == plm_ent){
			TRACE_LEAVE2("ncs_patricia_tree_get Failed.");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		
		if ((plm_ent->entity.ee_entity.saPlmEEPresenceState != 
			SA_PLM_EE_PRESENCE_UNINSTANTIATED)) {
			TRACE_LEAVE2("EE's presence state: %u, modify cannot be\
				done", plm_ent->entity.ee_entity.
					saPlmEEPresenceState);
			return SA_AIS_ERR_BAD_OPERATION;
		}
		else {
			/* check for saPlmEEType attr, whether the 
			   SaPlmEEType object with the attr value exists*/
			for (j=0; attr_mods[j] != NULL; j++) {
				if (strcmp(attr_mods[j]->modAttr.attrName, 
				"saPlmEEType")==0) {
					if ((attr_mods[j]->modType == 
						SA_IMM_ATTR_VALUES_ADD) ||
						(attr_mods[j]->modType ==
						SA_IMM_ATTR_VALUES_DELETE)) {
						TRACE_LEAVE2("attr mod_type is\
						%u, modify cannot be done", 
							attr_mods[j]->modType);
						return SA_AIS_ERR_BAD_OPERATION;
					}
					rc = validate_ee_type_attr_val(ccb_id,
						&(attr_mods[j]->modAttr));
					if (rc != SA_AIS_OK) {
						TRACE_LEAVE2("ee_type attr \
						value validation failed with \
						error %u", rc); 
						return rc;
					}
				}
				/* validation is not required for timeout attrs
				   for add/del/replace type of modification */
			}
		}
	}
	else if (memcmp(obj_name->value, "safVersion", 10)==0) {
		/* check if this is SaPlmHEType or SaPlmEEType object */
		if (strstr(dn_name, "safHEType") != NULL) {
			rc = validate_changes_to_he_type_obj(ccb_id, obj_name, 
				CCBUTIL_MODIFY, attr_mods);
			if (rc != SA_AIS_OK) {
				TRACE_LEAVE2("validation of he_type obj failed \
						with error %u", rc);
				return rc;
			}

		}
		else if (strstr(dn_name, "safEEType") != NULL) {
			rc = validate_changes_to_ee_type_obj(ccb_id, obj_name);
			if (rc != SA_AIS_OK) {
				TRACE_LEAVE2("validation of ee_type obj failed \
						with error %u", rc);
				return rc;
			}
			/* No specific validation is required for timeout attrs
			   of this object for add/del/replace types */
		}
	}
	else if (memcmp(obj_name->value, "safDomain", 9)==0) {
		rdn_val = strtok(dn_name, ",");
		if (strcmp(plms_cb->domain_info.domain.safDomain, rdn_val)
			!= 0) {
			TRACE_LEAVE2("domain %s does not exist", rdn_val);
			return SA_AIS_ERR_BAD_OPERATION;
		}
		else {
			/* Check if saPlmHEDeactivationPolicy attr value is 
			   within the range */
			for (j=0; attr_mods[j] != NULL; j++) {
				if (strcmp(attr_mods[j]->modAttr.attrName, 
				"saPlmHEDeactivationPolicy")==0) {
					/* No specific validation is required 
					   for this  attr for add/del/replace 
					   types */
					attr_value = (SaUint32T *)*(attr_mods[j]
							->modAttr.attrValues); 
					if ((*attr_value < 
						SA_PLM_DP_REJECT_NOT_OOS) || 
						(*attr_value > 
						SA_PLM_DP_UNCONDITIONAL)) {
						TRACE_LEAVE2("attr_val %u is \
						out-of-range for \
						saPlmHEDeactivationPolicy",
						*attr_value);
						return SA_AIS_ERR_BAD_OPERATION;
					}
					
				}
			}
		}
	}
	else if (memcmp(obj_name->value, "safDependency", 13)==0) {
		/* This object can be modified only if the associated HE/EE 
		   is out-of-service. */
		/* Get the parent DN and parent object in patricia tree */
		rdn = strtok(dn_name, ",");
		parent_dn = strtok(NULL, "\0");
		parent_dn_len = strlen(parent_dn);
		memset(&key_dn, 0, sizeof(SaNameT));
		memcpy(key_dn.value, parent_dn, parent_dn_len);
		key_dn.length = parent_dn_len;
		plm_ent = (PLMS_ENTITY *)
			ncs_patricia_tree_get(&plms_cb->entity_info, 
					(SaUint8T *)&key_dn);
		/* plm_ent cannot be null, otherwise IMM would not have sent
		   the request to PLM */
		if (((plm_ent->entity_type == PLMS_HE_ENTITY) &&
			(plm_ent->entity.he_entity.saPlmHEReadinessState 
			!= SA_PLM_READINESS_OUT_OF_SERVICE)) || 
			((plm_ent->entity_type == PLMS_EE_ENTITY) &&
			(plm_ent->entity.ee_entity.saPlmEEReadinessState 
			!= SA_PLM_READINESS_OUT_OF_SERVICE))) {
			TRACE_LEAVE2("Entity's readiness state is not OOS, \
				modify cannot be done");
			return SA_AIS_ERR_BAD_OPERATION;
		}
		/* Check the existing no of dep names */
		tmp_dep = plm_ent->dependency_list;
		while (tmp_dep != NULL) {
			cur_names_num++;
			tmp_dep = tmp_dep->next;
		}
		dep_min_num = plm_ent->min_no_dep;
		for (j=0; attr_mods[j] != NULL; j++) {	
			if(strcmp(attr_mods[j]->modAttr.attrName, 
				"saPlmDepNames") == 0) {
				dep_names_num = attr_mods[j]->modAttr.
						attrValuesNumber;
				for (k=0; k < dep_names_num; k++) {
					dep_name = (SaNameT *)*((attr_mods[j]->
							modAttr.attrValues)+k);
					dep_node = (PLMS_ENTITY *)
					ncs_patricia_tree_get(&plms_cb->
					entity_info, (SaUint8T *)dep_name);
					if (dep_node == NULL) {
						TRACE_LEAVE2("DepName not \
							exists"); 
						return SA_AIS_ERR_BAD_OPERATION;
					}
					if (attr_mods[j]->modType == 
						SA_IMM_ATTR_VALUES_DELETE) {
						tmp_dep = plm_ent->
							dependency_list;
						while (tmp_dep != NULL) {
							if (strcmp(tmp_dep->
							plm_entity->dn_name_str,
							dep_node->dn_name_str) 
							== 0) {
								break;
							}
							tmp_dep = tmp_dep->next;
						}
						if (tmp_dep == NULL) {
						TRACE_LEAVE2("Given DepName \
						to be deleted is not present \
						in the existing \
						configured depNames");
						return SA_AIS_ERR_BAD_OPERATION;
						}
					}
				}
				if (attr_mods[j]->modType == 
					SA_IMM_ATTR_VALUES_ADD) {
					dep_names_num += cur_names_num;
				}
				else if (attr_mods[j]->modType ==
					SA_IMM_ATTR_VALUES_DELETE) {
					if (dep_names_num > cur_names_num) {
						TRACE_LEAVE2("No of depNames:\
						%d > no of existing depNames:\
						%d", dep_names_num, 
						cur_names_num); 
						return SA_AIS_ERR_BAD_OPERATION;
					}
					dep_names_num = cur_names_num -
							dep_names_num;
				}
			}
			else if (strcmp(attr_mods[j]->modAttr.attrName, 
					"saPlmDepMinNumber") ==0) {
				dep_min_num = *(SaUint32T *)*(attr_mods[j]->
						modAttr.attrValues);
			}
		}
		if (dep_min_num > dep_names_num) {
			TRACE_LEAVE2("min no of dependencies: %u, \
			num_dep_names: %u", dep_min_num, dep_names_num);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != NULL) {
		/* "memorize the request" */
		if(ccbutil_ccbAddModifyOperation(ccb_util_ccb_data, 
						obj_name, attr_mods) != 0) {
			TRACE_LEAVE2("ccbutil_ccbAddModifyOperation failed");
			return SA_AIS_ERR_BAD_OPERATION;
		}
	} 
	else {
		TRACE_LEAVE2("ccbutil_getCcbData() returned null");
		return SA_AIS_ERR_NO_MEMORY;
	}
	TRACE_LEAVE();
	return SA_AIS_OK;
}
static SaAisErrorT plms_imm_ccb_completed_cbk(SaImmOiHandleT imm_oi_hdl, 
				SaImmOiCcbIdT ccb_id)
{
	/* Nothing to do in this function, return success */
	/* Log the same */
	TRACE_ENTER2("Nothing to be done in completed callback");
	TRACE_LEAVE();
	return SA_AIS_OK;
}
static void plms_imm_ccb_apply_cbk(SaImmOiHandleT imm_oi_hdl, 
			SaImmOiCcbIdT ccb_id)
{
	CcbUtilOperationData_t *opdata = NULL;
	SaUint32T obj_type;
	TRACE_ENTER2("ccb_id: %llu", ccb_id);
	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		obj_type = get_obj_type_from_obj_name(opdata->objectName.value);
		TRACE_2("obj_type: %u, op_type: %u", obj_type, 
			opdata->operationType);

		if (opdata->operationType == CCBUTIL_CREATE) {
			plms_create_objects(obj_type, &opdata->objectName, 
			(SaImmAttrValuesT_2 **)
			(opdata->param.create.attrValues));
		}
		else if (opdata->operationType == CCBUTIL_DELETE) {
			plms_delete_objects(obj_type, &opdata->objectName);
		}
		else if (opdata->operationType == CCBUTIL_MODIFY) {
			plms_modify_objects(obj_type, &opdata->objectName, 
			(SaImmAttrModificationT_2 **)
			opdata->param.modify.attrMods);
		}
	}
	TRACE_LEAVE();
	return;
}
static void plms_imm_ccb_abort_cbk(SaImmOiHandleT imm_oi_hdl, 
					SaImmOiCcbIdT ccb_id)
{
	CcbUtilCcbData_t *ccb_data;
	TRACE_ENTER2("ccb-id: %llu", ccb_id);
	/* find and free the ccb util data */
	ccb_data = ccbutil_findCcbData(ccb_id);
	if (ccb_data == NULL) {
		TRACE_2("ccb_data is not found");
		TRACE_LEAVE();
		return;
	}
	ccbutil_deleteCcbData(ccb_data);
	TRACE_LEAVE();
	return;
}
static void plms_imm_admin_op_cbk (SaImmOiHandleT imm_oi_hdl,
				SaInvocationT inv_id,
				const SaNameT *obj_name,
				SaImmAdminOperationIdT op_id,
				const SaImmAdminOperationParamsT_2 **params)
{
	PLMS_EVT *plms_evt;
	SaUint32T rc, rej_admin_op = FALSE;
	TRACE_ENTER2("Invocation-Id: %llu, Operation-Id: %llu", inv_id, op_id);
	if ((op_id != SA_PLM_ADMIN_LOCK) && (op_id != SA_PLM_ADMIN_RESTART)) {
		if (params[0] != NULL) {
			/* There are no parameter options applicable for admin
			   operations other than lock and restart. Hence reject
			   this operation. */
			rej_admin_op = TRUE;
		}
	}
	else {
		if (params[0] != NULL) {
			if (params[1] != NULL) {
				/* Both LOCK and RESTART admin operations 
				   take only one parameter. Hence reject 
				   this operation */
				rej_admin_op = TRUE;
			}
			else if (op_id == SA_PLM_ADMIN_LOCK) {
				if ((strcmp(params[0]->paramName, 
					"lockOption") != 0)
				|| (params[0]->paramType != 
					SA_IMM_ATTR_SASTRINGT)
				|| ((strcmp(*(SaStringT *)
				(params[0]->paramBuffer), "trylock") != 0)
				&& (strcmp(*(SaStringT *)
				(params[0]->paramBuffer), "forced") != 0))) {
					/* param name or type or value not set
					   correctly. Reject this operation */
					rej_admin_op = TRUE;
				}
			}
			else { /* (op_id == SA_PLM_ADMIN_RESTART) */
				if ((strcmp(params[0]->paramName, 
					"restartOption") != 0)
				|| (params[0]->paramType != 
					SA_IMM_ATTR_SASTRINGT)
				 || (strcmp(*(SaStringT *)
				(params[0]->paramBuffer),"abrupt") != 0)) {
					/* param name or type or value not set
					   correctly. Reject this operation */
					rej_admin_op = TRUE;
				}
			}
		}
	}
	if (rej_admin_op == TRUE) {
		LOG_ER("Rejecting admin op:%llu with invalid parameter error", 
			op_id);
		rc = saImmOiAdminOperationResult(plms_cb->oi_hdl, inv_id, 
			SA_AIS_ERR_INVALID_PARAM);
		if (SA_AIS_OK != rc) {
			LOG_ER("Sending response to IMM failed with error:%d", 
				rc);
		}
		return;
	}
	plms_evt = calloc(1, sizeof(PLMS_EVT));
	if (plms_evt == NULL) {
		LOG_CR("memory allocation failed for plms_evt, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	plms_evt->req_res = PLMS_REQ;
	plms_evt->req_evt.req_type = PLMS_IMM_ADM_OP_EVT_T;
	plms_evt->req_evt.admin_op.inv_id = inv_id;
	plms_evt->req_evt.admin_op.operation_id = op_id;
	plms_evt->req_evt.admin_op.dn_name.length = obj_name->length;
	memcpy(plms_evt->req_evt.admin_op.dn_name.value, obj_name->value, 
		obj_name->length);
	if (params[0] != NULL) {
		strcpy((SaInt8T *)plms_evt->req_evt.admin_op.option,
			*(SaStringT *) params[0]->paramBuffer);
		TRACE_2("param buffer: %s", *(SaStringT *)params[0]->
				paramBuffer);
	}
	/* post this to mailbox */
	rc = m_NCS_IPC_SEND(&plms_cb->mbx, (NCS_IPC_MSG *)plms_evt, NCS_IPC_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_CR("m_NCS_IPC_SEND returned error %u", rc);
	}
	TRACE_LEAVE();
}
static SaAisErrorT validate_he_entity_paths_attr_val(PLMS_ENTITY *plm_ent, 
					const SaImmAttrModificationT_2 *attr)
{
	SaStringT *attr_val, *ent_path;
	SaUint32T attr_val_num, i, j;
	TRACE_ENTER2("obj_dn: %s, attr mod_type %u", plm_ent->dn_name_str, 
			attr->modType);
	ent_path = plm_ent->entity.he_entity.saPlmHEEntityPaths;
	attr_val_num = attr->modAttr.attrValuesNumber;
	if (attr->modType == SA_IMM_ATTR_VALUES_DELETE)
	{
		/* Check if the given attr values exist in data structure */
		/* IMM does not take care of this..? */
		if (attr_val_num > (plm_ent->num_cfg_ent_paths)) {
			TRACE_LEAVE2("No of values:%d to be deleted is more \
				than no of existing values:%d for this attr",
				attr_val_num, plm_ent->num_cfg_ent_paths);
			return SA_AIS_ERR_BAD_OPERATION;
		}
		for (i=0; i < attr_val_num; i++) {
			attr_val = (SaStringT *)*(attr->modAttr.attrValues+i);
			for (j=0; j < plm_ent->num_cfg_ent_paths; j++) {
				if (strcmp(*attr_val, ent_path[j])==0) {
					break; /* from for loop of j */
				}
			}
			if (j == plm_ent->num_cfg_ent_paths) {
				   TRACE_LEAVE2("ent-path %s which is to be \
				   deleted is not present in the object", 
				   *attr_val);
				   return SA_AIS_ERR_BAD_OPERATION;
			}
		}
	}
	else if (attr->modType == SA_IMM_ATTR_VALUES_ADD) {
		if ((attr_val_num + (plm_ent->num_cfg_ent_paths)) > 
			SAHPI_MAX_ENTITY_PATH) {
			TRACE_LEAVE2("attr values %u cannot be added as current\
				no of values is %u", attr_val_num, plm_ent->
				num_cfg_ent_paths);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	else if (attr->modType == SA_IMM_ATTR_VALUES_REPLACE) {
		if (attr_val_num > SAHPI_MAX_ENTITY_PATH) {
			TRACE_LEAVE2("no of attr values %u is greater than \
			SAHPI_MAX_ENTITY_PATH", attr_val_num);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	TRACE_LEAVE2("HEEntityPaths attr validation successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_he_base_type_attr_val(SaImmOiCcbIdT ccb_id, 
					const SaImmAttrValuesT_2 *attr)
{
	/* Check if the object having the attr value as the
	   DN name exists */
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_HE_BASE_INFO *plm_he_base_info;
	SaNameT *attr_value;
	TRACE_ENTER();
	attr_value = (SaNameT *)*(attr->attrValues);
	if (attr_value == (SaNameT *)NULL) {
		TRACE_LEAVE2("attr_value is NULL");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	plm_he_base_info = (PLMS_HE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *)attr_value);
	if (plm_he_base_info == NULL) {
		TRACE_2("he_base_type_obj with the given attr value does not\
			exist in the patricia tree");
		/* Check if it is created in same CCB */
		opdata = NULL;
		while ((opdata=ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
			if ((opdata->operationType == CCBUTIL_CREATE) && 
			(memcmp(&opdata->objectName,attr_value,sizeof(SaNameT)) 
				== 0)) {
				break;
			}
		}
		if (opdata == NULL) {
			TRACE_2("he_base_type_obj with the given attr value \
			does not exist in the CCB also");
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	TRACE_LEAVE2("HEBaseType attr validation successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_ee_type_attr_val(SaImmOiCcbIdT ccb_id, 
				const SaImmAttrValuesT_2 *attr)
{
	/* Check if the object having the attr value as the
	   DN name exists */
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_EE_BASE_INFO *ee_base_info;
	PLMS_EE_TYPE_INFO *ee_type_node;
	SaNameT key_dn, *attr_value;
	SaStringT rdn, parent_dn;
	SaUint32T parent_dn_len;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	attr_value = (SaNameT *)*(attr->attrValues);
	if (attr_value == (SaNameT *)NULL) {
		/* Log the error */
		TRACE_LEAVE2("attr_value is NULL");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	memcpy(tmp_dn, attr_value->value, attr_value->length);
	rdn = strtok(tmp_dn, ",");
	if (memcmp(rdn, "safVersion", 10) != 0) {
		TRACE_LEAVE2("ee_type_attr_val does not have safVersion rdn\
			attribute name itself"); 
		return SA_AIS_ERR_BAD_OPERATION;
	}
	parent_dn = strtok(NULL, "\0");
	parent_dn_len = strlen(parent_dn);
	memset(&key_dn, 0, sizeof(SaNameT));
	strcpy((SaInt8T *)key_dn.value, parent_dn);
	key_dn.length = parent_dn_len;

	ee_base_info = (PLMS_EE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)&key_dn);

	if (ee_base_info == NULL) {
		/* The parent SaPlmEEBaseType object itself 
		   does not exist in the tree, check if 
		   SaPlmEEType object is created in this same 
		   CCB. No need to check for presence of 
		   SaPlmEEBaseType object in this CCB as 
		   SaPlmEEType object would not have been
		   created without its parent SaPlmEEBaseType 
		   object */
		TRACE_2("parent EEBaseType object %s does not exist in patricia\
			tree", parent_dn);
		ee_type_node = NULL;
	}
	else {
		/* Parent SaPlmEEBaseType object exists, check
		   if the SaPlmEEType object also exists */
		ee_type_node = ee_base_info->
				ee_type_info_list;
		while(ee_type_node != NULL) {
			if (strcmp(ee_type_node->ee_type.
				safVersion, rdn) ==0) {
				break;
			}
			ee_type_node = ee_type_node->next;
		}
	}
	if (ee_type_node == NULL) {
		/* SaPlmEEType object not found in the 
		   patricia tree, check in the current CCB */
		TRACE_2("EEType object with rdn %s under parent %s does not \
			exist in the patricia tree", rdn, parent_dn);   
		opdata = NULL;
		while ((opdata = ccbutil_getNextCcbOp(ccb_id, 
			opdata)) != NULL) {
			if ((opdata->operationType == 
				CCBUTIL_CREATE) && 
				(memcmp(&opdata->objectName, 
				attr_value, sizeof(SaNameT)) 
				== 0)) {
				break;
			}
		}
		if (opdata == NULL) {
			TRACE_LEAVE2("Object not found in CCB also");
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}
	TRACE_LEAVE2("EEType attr validation successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_deletion_of_he_base_type_obj(SaImmOiCcbIdT ccb_id, 
						const SaNameT *obj_name)
{
	SaImmAttrValuesT_2 **attr;
	SaNameT *attr_val;
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_ENTITY *plm_ent;
	SaUint8T j;
	/* Check if this obj DN is set in saPlmHEBaseHEType attr of any SaPlmHE
	   object, If so, return error */
	TRACE_ENTER();   
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(&plms_cb->
		entity_info, (SaUint8T *) NULL); 
	while (plm_ent != NULL) {
		if ((plm_ent->entity_type == PLMS_HE_ENTITY) && (memcmp(
			&plm_ent->entity.he_entity.saPlmHEBaseHEType, obj_name, 
			sizeof(SaNameT))==0)) {
			TRACE_LEAVE2("HE object %s still points to given\
				HEBaseType object, so HEBaseType object cannot\
				be deleted", plm_ent->dn_name_str);   
			return SA_AIS_ERR_BAD_OPERATION;
		}
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(
			&plms_cb->entity_info, (SaUint8T *)&plm_ent->dn_name);
	}
	/* We are here as there is no HE in patricia tree which refers to 
	   SaPlmHEBaseHEType object being modified/deleted. Check if there
	   is any HE created in the current CCB which refers to this 
	   SaPlmHEBaseHEType object*/
	
	opdata = NULL;
	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		if ((opdata->operationType == CCBUTIL_CREATE) && 
		(memcmp(opdata->objectName.value, "safHE", 5) == 0)) {
			attr = (SaImmAttrValuesT_2 **) opdata->param.create.
							attrValues;
			for (j=0; *attr != 0; j++) {
				if (strcmp((*attr)->attrName, 
				"saPlmHEBaseHEType")== 0) {
					attr_val = (SaNameT *)
					*((*attr)->attrValues);
					if(memcmp(attr_val, obj_name, 
						sizeof(SaNameT))==0) {
						TRACE_LEAVE2("An HE object in \
						CCB still points to given\
						HEBaseType object, so it cannot\
						be deleted");   
						return SA_AIS_ERR_BAD_OPERATION;
					}
				}
			}
		}
	}
	TRACE_LEAVE2("Validation of HEBaseType object deletion is successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_deletion_of_ee_base_type_obj(SaImmOiCcbIdT ccb_id, 
						const SaNameT *obj_name)
{
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_ENTITY *plm_ent;
	SaStringT parent_dn, tmp;
	SaUint8T j;
	SaNameT *attr_val;
	SaImmAttrValuesT_2 **attr;
	SaInt8T ee_type[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();   
	/* Check if any child of this object is pointed to by 
	   saPlmEEType attr of SaPlmEE object, If so, return error */
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(&plms_cb->
		entity_info, (SaUint8T *) NULL); 
	while (plm_ent != NULL) {
		if (plm_ent->entity_type == PLMS_EE_ENTITY) {
			memset(ee_type, 0, SA_MAX_NAME_LENGTH+1);
			strncpy(ee_type, (SaInt8T *)plm_ent->entity.ee_entity.
				saPlmEEType.value, plm_ent->entity.ee_entity.
				saPlmEEType.length);
			tmp = strtok(ee_type, ",");
			parent_dn = strtok(NULL, "\0");
			if ((obj_name->length == strlen(parent_dn)) &&
				(memcmp(parent_dn, obj_name->value, obj_name->
				length) ==0)) {
				TRACE_LEAVE2("EE object %s still points to \
				child EEType object of target EEBaseType \
				object, EEBaseType object cannot be deleted", 
				plm_ent->dn_name_str);   
				return SA_AIS_ERR_BAD_OPERATION;
			}
		}
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(
			&plms_cb->entity_info, (SaUint8T *)&plm_ent->dn_name);
	}
	/* We are here as there is no EE in patricia tree which refers to 
	   child SaPlmEEType object of the target SaPlmEEBaseType object being 
	   modified/deleted. Check if there is any EE created in the current 
	   CCB which refers to child of this SaPlmEEBaseType object*/
	
	opdata = NULL;
	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		if ((opdata->operationType == CCBUTIL_CREATE) && 
		(memcmp(opdata->objectName.value, "safEE", 5) == 0)) {
			attr = (SaImmAttrValuesT_2 **)opdata->param.create.
						attrValues;
			for (j=0; *attr != 0; j++) {
				if (strcmp((*attr)->attrName, 
				"saPlmEEType")== 0) {
					attr_val = (SaNameT *)
					*((*attr)->attrValues);
					memset(ee_type,0,SA_MAX_NAME_LENGTH+1);
					strncpy(ee_type, (SaInt8T *)attr_val->
						value, attr_val->length);
					tmp = strtok(ee_type, ",");
					parent_dn = strtok(NULL, "\0");
					if((obj_name->length == strlen(
					parent_dn)) && (memcmp(parent_dn, 
					obj_name->value,obj_name->length)==0)) {
						TRACE_LEAVE2("An EE object in \
						CCB still points to child \
						EEType object of target \
						EEBaseType object, so it \
						cannot be deleted"); 
						return SA_AIS_ERR_BAD_OPERATION;
					}
				}
			}
		}
	}
	TRACE_LEAVE2("Validation of EEBaseType object deletion is successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_changes_to_he_type_obj(SaImmOiCcbIdT ccb_id, 
				const SaNameT *obj_name, SaInt32T op_type,
				const SaImmAttrModificationT_2 **attr_mods)
{
	/* Check for the presence of this object in base_he_info tree */
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_HE_BASE_INFO *he_base_info;
	PLMS_HE_TYPE_INFO *he_type_node;
	SaStringT *attr_val_str, rdn, parent_dn;
	SaNameT *attr_val;
	SaUint32T parent_dn_len;
	SaUint8T j;
	SaNameT key_dn;
	PLMS_ENTITY *plm_ent;
	SaImmAttrValuesT_2 **attr;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	memcpy(tmp_dn, obj_name->value, obj_name->length);
	rdn = strtok(tmp_dn, ",");
	parent_dn = strtok(NULL, "\0");
	parent_dn_len = strlen(parent_dn);
	memset(&key_dn, 0, sizeof(SaNameT));
	strcpy((SaInt8T *)key_dn.value, parent_dn);
	key_dn.length = parent_dn_len;
	he_base_info = (PLMS_HE_BASE_INFO *) ncs_patricia_tree_get(
				&plms_cb->base_he_info, (SaUint8T *)&key_dn);
	if (he_base_info == NULL) {
		TRACE_LEAVE2("Parent HEBaseType object %s does not exist", 
			parent_dn);
		return SA_AIS_ERR_BAD_OPERATION;
	}
	he_type_node = he_base_info->he_type_info_list;
	while(he_type_node != NULL) {
		if (strcmp(he_type_node->he_type.safVersion, rdn) ==0) {
			break;
		}
		he_type_node = he_type_node->next;
	}
	if (he_type_node == NULL) {
		TRACE_LEAVE2("HEType object not found in the patricia tree");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	/* Check if this DN is pointed to by the saPlmHECurrHEType attr of 
	   SaPlmHE object. If so, return error. Also check if parent 
	   SaPlmHEBaseHEType object is pointed to by saPlmHEBaseHEType attr 
	   of any HE object. If so return error */
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(&plms_cb->
		entity_info, (SaUint8T *) NULL);	
	while (plm_ent != NULL) {
		if ((plm_ent->entity_type == PLMS_HE_ENTITY) && ((memcmp(
			&plm_ent->entity.he_entity.saPlmHECurrHEType, obj_name, 
			sizeof(SaNameT))==0) || 
		((plm_ent->entity.he_entity.saPlmHEBaseHEType.length == 
			parent_dn_len)
		&& (memcmp(plm_ent->entity.he_entity.saPlmHEBaseHEType.value, 
		parent_dn, parent_dn_len)==0)))) {
			TRACE_LEAVE2("HE object %s still points to given HEType\
				object, so it cannot be modified/deleted", 
				plm_ent->dn_name_str);   
			return SA_AIS_ERR_BAD_OPERATION;
		}
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(
				&plms_cb->entity_info, (SaUint8T *)
				&(plm_ent->dn_name));
	}
	/* We are here as there is no HE which has the attributes pointing to 
	   the target SaPlmHEType object or its parent SaPlmHEBaseHEType
	   object. Check if there is any HE in the current CCB which has 
	   saPlmHEBaseHEType attr pointing to the parent of target SaPlmHEType 
	   object. */
	opdata = NULL;
	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		if ((opdata->operationType == CCBUTIL_CREATE) && 
		(memcmp(opdata->objectName.value, "safHE", 5) == 0)) {
			attr = (SaImmAttrValuesT_2 **)opdata->param.create.
						attrValues;
			for (j=0; *attr != NULL; j++) {
				if (strcmp((*attr)->attrName, 
				"saPlmHEBaseHEType")== 0) {
					attr_val = (SaNameT *)
					*((*attr)->attrValues);
					if((attr_val->length == parent_dn_len)
					&& (memcmp(attr_val->value, parent_dn,
					attr_val->length)==0)) {
						TRACE_LEAVE2("An HE object in \
						CCB still points to given \
						HEType object, so it cannot be \
						modified/deleted");   
						return SA_AIS_ERR_BAD_OPERATION;
					}
				}
			}
		}
	}
	/* validate saPlmHetIdr attr modification as we support only one 
	   string value for this multi-valued attribute */
	if (op_type == CCBUTIL_MODIFY) {
	for (j=0; attr_mods[j] != NULL; j++) {
		if (strcmp(attr_mods[j]->modAttr.attrName, "saPlmHetIdr")==0) {
			if (attr_mods[j]->modType == SA_IMM_ATTR_VALUES_ADD) {
				if (he_type_node->he_type.saPlmHetIdr != NULL) {
					TRACE_LEAVE2("saPlmHetIdr has one value\
					already, we do not support multiple \
					values, so it cannot be modified");   
					return SA_AIS_ERR_BAD_OPERATION;
				}
				else if (attr_mods[j]->modAttr.attrValuesNumber 
					> 1) {
					TRACE_LEAVE2("we do not support \
					multiple values to saPlmHetIdr, so it \
					cannot be modified");   
					return SA_AIS_ERR_BAD_OPERATION;   
				}
				else if ((SaStringT *)*(attr_mods[j]->modAttr.
					attrValues) == NULL) {
					/* for ADD modify, can this be null? */
					/* Does IMM not check this..? Log and
					   return error */
					TRACE_LEAVE2("attrValues is NULL for\
					saPlmHetIdr, cannot be modified");   
					return SA_AIS_ERR_BAD_OPERATION;   
				}
			}
			else if (attr_mods[j]->modType == 
				SA_IMM_ATTR_VALUES_DELETE) {
				/* check if the value is same as the one which
				   exists in the data structure */
				/* IMM does not check this..? */   
				attr_val_str = (SaStringT *)*(attr_mods[j]->
					modAttr.attrValues);
				if (he_type_node->he_type.saPlmHetIdr == NULL) {
					TRACE_LEAVE2("Existing value is null");
					return SA_AIS_ERR_BAD_OPERATION;   
				}
				if (strcmp(*attr_val_str, he_type_node->he_type.
					saPlmHetIdr) != 0) {
					TRACE_LEAVE2("Given attr value %s is\
					not same as the value of saPlmHetIdr\
					attribute in the patricia tree", 
					*attr_val_str);
					return SA_AIS_ERR_BAD_OPERATION;   
				}
			}
			/* Nothing to be validated for REPLACE modification */
		}
	}
	}
	TRACE_LEAVE2("Validation of HEType object delete/modify successful");
	return SA_AIS_OK;
}
static SaAisErrorT validate_changes_to_ee_type_obj(SaImmOiCcbIdT ccb_id, 
						const SaNameT *obj_name) 
{
	/* Check for the presence of this object in base_ee_info tree */
	PLMS_EE_BASE_INFO *ee_base_info;
	PLMS_EE_TYPE_INFO *ee_type_list;
	SaStringT rdn, parent_dn;
	SaNameT key_dn;
	SaUint32T parent_dn_len;
	CcbUtilOperationData_t *opdata = NULL;
	PLMS_ENTITY *plm_ent;
	SaImmAttrValuesT_2 **attr;
	SaNameT *attr_val;
	SaUint8T j;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	memcpy(tmp_dn, obj_name->value, obj_name->length);
	rdn = strtok(tmp_dn, ",");
	parent_dn = strtok(NULL, "\0");
	parent_dn_len = strlen(parent_dn);
	memset(&key_dn, 0, sizeof(SaNameT));
	strcpy((SaInt8T *)key_dn.value, parent_dn);
	key_dn.length = parent_dn_len;
	ee_base_info = (PLMS_EE_BASE_INFO *) ncs_patricia_tree_get(
				&plms_cb->base_ee_info, (SaUint8T *)&key_dn);
	if (ee_base_info == NULL) {
		TRACE_LEAVE2("parent EEBaseType object %s does not exist", 
			parent_dn);
		return SA_AIS_ERR_BAD_OPERATION;
	}
	ee_type_list = ee_base_info->ee_type_info_list;
	while(ee_type_list != NULL) {
		if (strcmp(ee_type_list->ee_type.safVersion, rdn) ==0) {
			break;
		}
		ee_type_list = ee_type_list->next;
	}
	if (ee_type_list == NULL) {
		TRACE_LEAVE2("EEType object not found in the patricia tree");
		return SA_AIS_ERR_BAD_OPERATION;
	}
	/* Check if this DN is set in saPlmEEType attr of SaPlmEE object, 
	   If so, return error */
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(
			&plms_cb->entity_info, (SaUint8T *) NULL);
	while (plm_ent != NULL) {
		if ((plm_ent->entity_type == PLMS_EE_ENTITY) && (memcmp(
			&plm_ent->entity.ee_entity.saPlmEEType, obj_name, 
			sizeof(SaNameT))==0)) {
			TRACE_LEAVE2("EE object %s still points to given EEType\
				object, so it cannot be modified/deleted", 
				plm_ent->dn_name_str);   
			return SA_AIS_ERR_BAD_OPERATION;
		}
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(
				&plms_cb->entity_info, (SaUint8T *)&plm_ent->
					dn_name);
	}
	/* We are here as there is no EE in patricia tree which refers to 
	   target SaPlmEEType object being modified/deleted. Check if there is 
	   any EE created in the current CCB which refers to this SaPlmEEType 
	   object*/
	opdata = NULL;
	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		if ((opdata->operationType == CCBUTIL_CREATE) && 
		(memcmp(opdata->objectName.value, "safEE", 5) == 0)) {
			attr = (SaImmAttrValuesT_2 **)opdata->param.create.
				attrValues;
			for (j=0; *attr != 0; j++) {
				if (strcmp((*attr)->attrName, 
				"saPlmEEType")== 0) {
					attr_val = (SaNameT *)
					*((*attr)->attrValues);
					if(memcmp(attr_val, obj_name, 
						sizeof(SaNameT))==0) {
						TRACE_LEAVE2("An EE object in \
						CCB still points to given \
						EEType object, so it cannot be \
						modified/deleted");   
						return SA_AIS_ERR_BAD_OPERATION;
					}
				}
			}
		}
	}
	TRACE_LEAVE2("Validation of EEType object delete/modify successful");
	return SA_AIS_OK;
}
static SaInt32T get_obj_type_from_obj_name(SaUint8T *str) 
{
	SaInt32T obj_type = 0;

	if (memcmp(str, "safHEType", 9) == 0) 
		obj_type = PLMS_HE_BASE_TYPE_OBJ_TYPE;
	else if (memcmp(str, "safHE", 5) == 0) 
		obj_type = PLMS_HE_OBJ_TYPE;
	else if (memcmp(str, "safEEType", 9) == 0) 
		obj_type = PLMS_EE_BASE_TYPE_OBJ_TYPE;
	else if (memcmp(str, "safEE", 5) == 0) 
		obj_type = PLMS_EE_OBJ_TYPE;
	else if (memcmp(str, "safVersion", 10) == 0) {
		if (strstr((SaInt8T *)str, "safHEType") != NULL)
			obj_type = PLMS_HE_TYPE_OBJ_TYPE;
		else if (strstr((SaInt8T *)str, "safEEType") != NULL)
			obj_type = PLMS_EE_TYPE_OBJ_TYPE;
	}
	else if (memcmp(str, "safDependency", 5) == 0) 
		obj_type = PLMS_DEPENDENCY_OBJ_TYPE;
	else if (memcmp(str, "safDomain", 9) == 0) 
		obj_type = PLMS_DOMAIN_OBJ_TYPE;
	else if (memcmp(str, "safHpiCfg", 9) == 0) 
		obj_type = PLMS_HPI_CFG_OBJ_TYPE;
	return obj_type;
}
static void plms_create_domain_obj(SaNameT *obj_name, 
				SaImmAttrValuesT_2 **attrs)
{
	SaUint8T i;
	SaStringT attr_name;
	TRACE_ENTER();
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safDomain")==0) {
			plms_cb->domain_info.domain.safDomain = calloc(
			SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (plms_cb->domain_info.domain.safDomain == NULL) {
				LOG_CR("memory allocation failed for safDomain,\
					calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(plms_cb->domain_info.domain.safDomain, 
			*(SaStringT *)*((attrs[i])->attrValues));
			TRACE_2("RDN attr safDomain value: %s", plms_cb->
				domain_info.domain.safDomain);
			/* Set the default val for deactivation policy */
			plms_cb->domain_info.domain.saPlmHEDeactivationPolicy =
				SA_PLM_DP_VALIDATE;
		}
		else if (strcmp(attr_name, "saPlmHEDeactivationPolicy")
			==0) {
			plms_cb->domain_info.domain.saPlmHEDeactivationPolicy = 
			*(SaPlmHEDeactivationPolicyT *)
			*((attrs[i])->attrValues);
			TRACE_2("SaPlmHEDeactivationPolicy attr val: %u", 
			plms_cb->domain_info.domain.saPlmHEDeactivationPolicy);
		}
	}
	TRACE_LEAVE2("domain object created");  
}
static void plms_create_hpi_cfg_obj(SaNameT *obj_name, 
				SaImmAttrValuesT_2 **attrs)
{
	SaUint8T i;
	SaStringT attr_name;
	TRACE_ENTER();
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safHpiCfg")==0) {
			plms_cb->hpi_cfg.safHpiCfg = calloc(
			SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (plms_cb->hpi_cfg.safHpiCfg == NULL) {
				LOG_CR("memory allocation failed for safHpiCfg,\
					calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(plms_cb->hpi_cfg.safHpiCfg, 
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("RDN attr safHpiCfg value: %s", 
				plms_cb->hpi_cfg.safHpiCfg);
		}
		else if (strcmp(attr_name, "isHpiSupported") ==0) {
			plms_cb->hpi_cfg.hpi_support = 
			*(SaUint32T *) *((attrs[i])->attrValues);
			TRACE_2("isHpiSupported attr value: %u", 
				plms_cb->hpi_cfg.hpi_support);
		}
		else if (strcmp(attr_name, "extrPendingTimeout") ==0) {
			plms_cb->hpi_cfg.extr_pending_timeout = 
			*(SaTimeT *) *((attrs[i])->attrValues);
			TRACE_2("extrPendingTimeout attr value: %llu", 
				plms_cb->hpi_cfg.extr_pending_timeout);
		}
		else if (strcmp(attr_name, "insPendingTimeout") ==0) {
			plms_cb->hpi_cfg.insert_pending_timeout = 
			*(SaTimeT *) *((attrs[i])->attrValues);
			TRACE_2("insPendingTimeout attr value: %llu", 
				plms_cb->hpi_cfg.insert_pending_timeout);
		}
	}
	TRACE_LEAVE2("hpi_cfg object created"); 
}
static void plms_create_dependency_obj(SaNameT *obj_name, 
				SaImmAttrValuesT_2 **attrs)
{
	PLMS_ENTITY *plm_ent, *ent_node;
	SaStringT temp, attr_name;
	SaNameT key_dn, *temp_dn;
	SaUint32T rc, values_num;
	SaUint8T i, j;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	/* Get the parent DN */
	memcpy(dn_name, obj_name->value, obj_name->length);
	   
	/* Skip the rdn attribute of the dependency object */   
	/* The string safDependency=<> is skipped in below stmt */ 
	temp = strtok(dn_name, ",");
	temp = strtok(NULL, "\0");
	/* Now the parent's dn name is pointed by temp */
	
	memset(&key_dn, 0, sizeof(SaNameT));
	memcpy(key_dn.value, temp, strlen(temp));
	key_dn.length = strlen(temp);
	plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *) 
					&key_dn);
	TRACE_2("Parent of dependency object: %s", temp);
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safDependency")==0) {
			TRACE_2("RDN attr safDependency value: %s", 
			*(SaStringT *)*(attrs[i]->attrValues));
		}
		else if (strcmp(attr_name, "saPlmDepNames")==0) {
			values_num = attrs[i]->attrValuesNumber;
			for (j=0; j < values_num; j++) {
				/* Fill the dependency list of entity */
				/*Get the ptr to this dependent entity*/
				temp_dn =(SaNameT *)*((attrs[i]->attrValues)+j);
				ent_node = (PLMS_ENTITY *)
				ncs_patricia_tree_get(&plms_cb->entity_info,
						(SaUint8T *)temp_dn);
				if (ent_node == NULL) {
					LOG_ER("Dep name not exists,\
					dependency list not updated for entity\
					:%s with this name, this is not correct\
					configuration from IMM.xml", temp);
					return;
				}
				TRACE_2("DepNames attr value: %s", ent_node->
					dn_name_str);
				rc = add_ent2_to_dep_list_of_ent1(plm_ent, 
					ent_node);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("add_ent2_to_dep_list_of_ent1 \
					returned error, calling assert %u", rc);
					assert(0);
				}
				/* Update the rev_dep_list of ent_node 
				   with the current plm_ent */
				rc = add_ent1_to_rev_dep_list_of_ent2(plm_ent, 
					ent_node);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("add_ent1_2_rev_dep_list_of_ent2\
					returned error, calling assert %u", rc);
					assert(0);
				}
			}
		}
		else if (strcmp(attr_name, "saPlmDepMinNumber")==0) {
			plm_ent->min_no_dep = *(SaUint32T *)*((attrs[i])->
						attrValues);
			TRACE_2("saPlmDepMinNumber attr value: %u", plm_ent->
								min_no_dep);
		}
	}
	TRACE_LEAVE2("Dependency object created");
}
static void plms_create_he_base_type_obj(SaNameT *obj_name, 
					SaImmAttrValuesT_2 **attrs)
{
	PLMS_HE_BASE_INFO *he_base_info;
	SaUint8T i;
	SaUint32T attr_val_len, rc;
	SaStringT attr_name;
	TRACE_ENTER();
	he_base_info = (PLMS_HE_BASE_INFO *) calloc(1, sizeof(PLMS_HE_BASE_INFO));
	if (he_base_info == NULL) {
		LOG_CR("memory allocation failed for he_base_info, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safHEType") == 0) {
			he_base_info->he_base_type.safHEType = 
			(SaStringT) calloc(SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (he_base_info->he_base_type.safHEType == NULL) {
				LOG_CR("memory allocation failed for safHEType,\
					calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(he_base_info->he_base_type.safHEType, 
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("RDN attr value safHEType: %s", he_base_info->
				he_base_type.safHEType);
		}
		else if (strcmp(attr_name, 
			"saPlmHetHpiEntityType") == 0) {
			attr_val_len = strlen(*(SaStringT *)*(attrs[i]->
					attrValues));
			he_base_info->he_base_type.saPlmHetHpiEntityType
			= (SaInt8T *) calloc(attr_val_len+1, sizeof(SaInt8T));
			if (he_base_info->he_base_type.saPlmHetHpiEntityType ==
				NULL) {
				LOG_CR("memory allocation failed for \
				saPlmHetHpiEntityType, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(he_base_info->he_base_type.saPlmHetHpiEntityType,
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("saPlmHetHpiEntityType attr value: %s", 
			he_base_info->he_base_type.saPlmHetHpiEntityType);
		}
	}
	/*memcpy(&he_base_info->dn_name, obj_name, sizeof(SaNameT));*/
	memcpy(he_base_info->dn_name.value, obj_name->value, obj_name->length);
	he_base_info->dn_name.length = obj_name->length;
	he_base_info->pat_node.key_info = (SaUint8T *)&(he_base_info->dn_name);
	rc = ncs_patricia_tree_add (&plms_cb->base_he_info, 
				&(he_base_info->pat_node));
	if (rc == NCSCC_RC_FAILURE) {
		LOG_CR("ncs_patricia_tree_add() failed, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	TRACE_LEAVE2("HEBaseType object created");
}
static void plms_create_he_obj(SaNameT *obj_name, SaImmAttrValuesT_2 **attrs)
{
	PLMS_ENTITY *he, *parent_ent, *tmp;
	PLMS_DOMAIN *domain;
	SaStringT rdn_name, temp, attr_name, *attr_val;
	SaNameT key_dn;
	SaUint32T values_num, attr_val_len, rc;
	SaUint8T i, j;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	SaInt8T parent_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();	
	he = (PLMS_ENTITY *) calloc(1, sizeof(PLMS_ENTITY));
	if (he == NULL) {
		LOG_CR("memory allocation failed for he, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	he->entity_type = PLMS_HE_ENTITY;
	memcpy(&he->dn_name, obj_name, sizeof(SaNameT));
	he->pat_node.key_info = (SaUint8T *)&he->dn_name;
	/* set the default values for state attributes */
	he->entity.he_entity.saPlmHEAdminState = SA_PLM_HE_ADMIN_UNLOCKED;
	he->entity.he_entity.saPlmHEReadinessState = SA_PLM_READINESS_OUT_OF_SERVICE;
	he->entity.he_entity.saPlmHEPresenceState = SA_PLM_HE_PRESENCE_NOT_PRESENT;
	he->entity.he_entity.saPlmHEOperationalState = SA_PLM_OPERATIONAL_ENABLED;
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No attr value specified");
			/* This is valid case for runtime/cached attributes */
			continue;
		}
		if (strcmp(attr_name, "safHE")==0) {
			he->entity.he_entity.safHE = calloc(
			SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (he->entity.he_entity.safHE == NULL) {
				LOG_CR("memory allocation failed for safHE, \
					calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(he->entity.he_entity.safHE,
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("RDN attr safHE value: %s", he->entity.
				he_entity.safHE);
		}
		else if (strcmp(attr_name, "saPlmHEBaseHEType") ==0) {
			he->entity.he_entity.saPlmHEBaseHEType.length =
			((SaNameT *)*((attrs[i])->attrValues))->length;
			memcpy(he->entity.he_entity.saPlmHEBaseHEType.value,
			((SaNameT *)*((attrs[i])->attrValues))->value,
			he->entity.ee_entity.saPlmEEType.length);
		}
		else if (strcmp(attr_name, "saPlmHEEntityPaths") ==0) {
			values_num = attrs[i]->attrValuesNumber;
			TRACE_2("saPlmHEEntityPaths attr values number: %u", 
				values_num);
			for(j=0; j < values_num; j++) {
				attr_val = (SaStringT *)
					*((attrs[i]->attrValues)+j);
				attr_val_len = strlen(*attr_val);
				temp = calloc(attr_val_len+1, sizeof(SaInt8T));
				if (temp == NULL) {
					LOG_CR("memory allocation failed for \
					saPlmHEEntityPaths,calling assert now");
					TRACE_LEAVE();
					assert(0);
				}
				strcpy(temp, *attr_val);
				he->entity.he_entity.saPlmHEEntityPaths[j]=temp;
				TRACE_2("saPlmHEEntityPaths attr value: %s", 
					temp);
			}
			he->num_cfg_ent_paths = values_num;
		}
		else if (strcmp(attr_name, "saPlmHECurrHEType") ==0) {
			he->entity.he_entity.saPlmHECurrHEType.length =
			((SaNameT *)*((attrs[i])->attrValues))->length;
			memcpy(he->entity.he_entity.saPlmHECurrHEType.value,
			((SaNameT *)*((attrs[i])->attrValues))->value, 
			he->entity.he_entity.saPlmHECurrHEType.length);
		}
		else if (strcmp(attr_name, "saPlmHECurrEntityPath") ==0) {
			attr_val = (SaStringT *)*(attrs[i]->attrValues);
			attr_val_len = strlen(*attr_val);
			temp = calloc(attr_val_len+1, sizeof(SaInt8T));
			if (temp == NULL) {
				LOG_CR("memory allocation failed for \
				saPlmHECurrEntityPath, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(temp, *attr_val);
			he->entity.he_entity.saPlmHECurrEntityPath=temp;
			TRACE_2("saPlmHECurrEntityPath attr value: %s", temp);
		}
		else if (strcmp(attr_name, "saPlmHEAdminState") ==0) {
			he->entity.he_entity.saPlmHEAdminState = 
				*(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmHEAdminState attr value: %u", he->entity.
				he_entity.saPlmHEAdminState);
		}
		else if (strcmp(attr_name, "saPlmHEReadinessState") ==0) {
			he->entity.he_entity.saPlmHEReadinessState = 
				*(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmHEReadinessState attr value: %u", 
				he->entity.he_entity.saPlmHEReadinessState);
		}
		else if (strcmp(attr_name, "saPlmHEReadinessFlags") ==0) {
			he->entity.he_entity.saPlmHEReadinessFlags = 
				*(SaUint64T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmHEReadinessFlags attr value: %llu", 
				he->entity.he_entity.saPlmHEReadinessFlags);
		}
		else if (strcmp(attr_name, "saPlmHEPresenceState") ==0) {
			he->entity.he_entity.saPlmHEPresenceState = 
				*(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmHEPresenceState attr value: %u", 
				he->entity.he_entity.saPlmHEPresenceState);
		}
		else if (strcmp(attr_name, "saPlmHEOperationalState") ==0) {
			he->entity.he_entity.saPlmHEOperationalState = 
				*(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmHEOperationalState attr value: %u", 
				he->entity.he_entity.saPlmHEOperationalState);
		}
	}
	/* Add the node to patricia tree */
	rc = ncs_patricia_tree_add(&plms_cb->entity_info, &he->pat_node);
	if (rc == NCSCC_RC_FAILURE) {
		LOG_CR("ncs_patricia_tree_add() failed, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	memcpy(dn_name, obj_name->value, obj_name->length);
	he->dn_name_str = calloc(SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
	if (he->dn_name_str == NULL) {
		LOG_CR("memory allocation failed for he->dn_name_str, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	strcpy(he->dn_name_str, dn_name); 
	/* Get the parent's DN */   
	/* The string safHE=<> is skipped in below stmt */ 
	temp = strtok(dn_name, ",");
	temp = strtok(NULL, "\0");
	/* Now the parent's dn name is pointed by temp */
	/* Copy it to parent_dn variable */
	strcpy(parent_dn, temp);
	/* Get the rdn attr name from parent's dn pointed to by 
	   temp variable */
	rdn_name = strtok(temp, "=");
	/* Set the containment relationship now */
	/* If this is the first child added under it's parent,  
	   Set the leftmost child pointer of the parent to this
	   node. If the parent already has children, then 
	   traverse till the last child of the parent and then 
	   set the right sibling pointer of the last child to
	   this node. */
	if (strcmp(rdn_name, "safDomain") == 0) {
		domain = &plms_cb->domain_info;
		if(domain->leftmost_child == NULL) {
			domain->leftmost_child = he;
		}
		else {
			tmp = domain->leftmost_child;
			while (tmp->right_sibling != NULL) {
				tmp = tmp->right_sibling;
			}
			tmp->right_sibling = he;
		}
		he->parent = NULL; /* This is because parent ptr is of type 
		PLMS_ENTITY */
	}
	else {
		memset(&key_dn, 0, sizeof(SaNameT));
		key_dn.length = strlen(parent_dn);
		memcpy(key_dn.value, parent_dn, key_dn.length);
		parent_ent=(PLMS_ENTITY *)ncs_patricia_tree_get(
		&plms_cb->entity_info, (SaUint8T *)&key_dn);
		if (parent_ent->leftmost_child == NULL) {
			parent_ent->leftmost_child = he;
		}
		else {
			tmp = parent_ent->leftmost_child;
			while(tmp->right_sibling != NULL) {
				tmp = tmp->right_sibling;
			}
			tmp->right_sibling = he;
		}
		he->parent = parent_ent;
	}
	/* Admin state need not be updated as default value will be in XML*/ 
	rc = plms_attr_imm_update(he,"saPlmHEReadinessState",
				SA_IMM_ATTR_SAUINT32T,he->entity.he_entity.
				saPlmHEReadinessState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Readiness state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", he->dn_name_str, 
		he->entity.he_entity.saPlmHEReadinessState, rc);
	}
	rc = plms_attr_imm_update(he,"saPlmHEPresenceState",
				SA_IMM_ATTR_SAUINT32T,he->entity.he_entity.
				saPlmHEPresenceState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Presence state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", he->dn_name_str, 
		he->entity.he_entity.saPlmHEPresenceState, rc);
	}
	rc = plms_attr_imm_update(he,"saPlmHEOperationalState",
				SA_IMM_ATTR_SAUINT32T,he->entity.he_entity.
				saPlmHEOperationalState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Operational state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", he->dn_name_str, 
		he->entity.he_entity.saPlmHEOperationalState, rc);
	}
	rc = plms_attr_imm_update(he,"saPlmHEReadinessFlags",
				SA_IMM_ATTR_SAUINT64T,he->entity.he_entity.
				saPlmHEReadinessFlags);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Readiness flags IMM update FAILED. Ent: %s, \
		State: %llu, error: %d", he->dn_name_str, 
		he->entity.he_entity.saPlmHEReadinessFlags, rc);
	}
	/*plms_check_n_add_ent_to_ent_groups(he);*/
	TRACE_LEAVE2("HE object %s created", he->dn_name_str);
}
static void plms_create_ee_base_type_obj(SaNameT *obj_name, 
					SaImmAttrValuesT_2 **attrs)
{
	PLMS_EE_BASE_INFO *ee_base_info;
	SaUint32T rc, attr_val_len;
	SaUint8T i;
	SaStringT attr_name;
	TRACE_ENTER();
	ee_base_info = (PLMS_EE_BASE_INFO *) calloc(1, sizeof(PLMS_EE_BASE_INFO));
	if (ee_base_info == NULL) {
		LOG_CR("memory allocation failed for ee_base_info, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safEEType") == 0) {
			ee_base_info->ee_base_type.safEEType =
			(SaInt8T *) calloc(SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (ee_base_info->ee_base_type.safEEType == NULL) {
				LOG_CR("memory allocation failed for safEEType,\
				calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee_base_info->ee_base_type.safEEType, 
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("RDN attr value safEEType: %s", ee_base_info->
				ee_base_type.safEEType);
		}
		else if (strcmp(attr_name, "saPlmEetProduct") == 0) {
			attr_val_len = strlen(*(SaStringT *)*(attrs[i]->
					attrValues));
			ee_base_info->ee_base_type.saPlmEetProduct = 
			(SaInt8T *) calloc(attr_val_len+1, sizeof(SaInt8T));
			if (ee_base_info->ee_base_type.saPlmEetProduct 
				== NULL) {
				LOG_CR("memory allocation failed for \
					saPlmEetProduct, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee_base_info->ee_base_type.saPlmEetProduct, 
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("saPlmEetProduct attr value: %s", ee_base_info->
				ee_base_type.saPlmEetProduct);
		}
		else if (strcmp(attr_name, "saPlmEetVendor") == 0) {
			attr_val_len = strlen(*(SaStringT *)*(attrs[i]->
					attrValues));
			ee_base_info->ee_base_type.saPlmEetVendor = 
			(SaInt8T *)calloc(attr_val_len+1, sizeof(SaInt8T));
			if (ee_base_info->ee_base_type.saPlmEetVendor 
				== NULL) {
				LOG_CR("memory allocation failed for \
					saPlmEetVendor, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee_base_info->ee_base_type.saPlmEetVendor, 
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("saPlmEetVendor attr value: %s", ee_base_info->
				ee_base_type.saPlmEetVendor);
		}
		else if (strcmp(attr_name, "saPlmEetRelease") == 0) {
			attr_val_len = strlen(*(SaStringT *)*(attrs[i]->
					attrValues));
			ee_base_info->ee_base_type.saPlmEetRelease = 
			(SaInt8T *)calloc(attr_val_len+1, sizeof(SaInt8T));
			if (ee_base_info->ee_base_type.saPlmEetRelease 
				== NULL) {
				LOG_CR("memory allocation failed for \
					saPlmEetRelease, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee_base_info->ee_base_type.saPlmEetRelease,
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("saPlmEetRelease attr value: %s", ee_base_info->
				ee_base_type.saPlmEetRelease);
		}
	}
	memcpy(&ee_base_info->dn_name, obj_name, sizeof(SaNameT));
	ee_base_info->pat_node.key_info = (SaUint8T *)&ee_base_info->dn_name;
	rc = ncs_patricia_tree_add (&plms_cb->base_ee_info, 
				&ee_base_info->pat_node);
	if (rc == NCSCC_RC_FAILURE)
	{
		LOG_CR("ncs_patricia_tree_add() failed, calling assert now"); 
		TRACE_LEAVE();
		assert(0);
	}
	TRACE_LEAVE2("EEBaseType object created");
}
static void plms_create_ee_obj(SaNameT *obj_name, SaImmAttrValuesT_2 **attrs)
{
	PLMS_ENTITY *ee, *parent_ent, *tmp;
	PLMS_DOMAIN *domain;
	SaStringT rdn_name, temp, attr_name;
	SaUint8T i;
	SaUint32T rc;
	SaNameT key_dn;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	SaInt8T parent_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	ee = (PLMS_ENTITY *) calloc(1, sizeof(PLMS_ENTITY));
	if (ee == NULL) {
		LOG_CR("memory allocation failed for ee, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	ee->entity_type = PLMS_EE_ENTITY;
	memcpy(&ee->dn_name, obj_name, sizeof(SaNameT));
	ee->pat_node.key_info = (SaUint8T *) &ee->dn_name;
	/* set the default values for state attributes */
	ee->entity.ee_entity.saPlmEEAdminState = SA_PLM_EE_ADMIN_UNLOCKED;
	ee->entity.ee_entity.saPlmEEReadinessState = SA_PLM_READINESS_OUT_OF_SERVICE;
	ee->entity.ee_entity.saPlmEEPresenceState = SA_PLM_EE_PRESENCE_UNINSTANTIATED;
	ee->entity.ee_entity.saPlmEEOperationalState = SA_PLM_OPERATIONAL_ENABLED;
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No attr value specified");
			/* This is valid case for runtime/cached attributes */
			continue;
		}
		if (strcmp(attr_name, "safEE")==0) {
			ee->entity.ee_entity.safEE = calloc(
			SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (ee->entity.ee_entity.safEE == NULL) {
				LOG_CR("memory allocation failed for safEE, \
					calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee->entity.ee_entity.safEE,
			*(SaStringT *)*(attrs[i]->attrValues));
			TRACE_2("RDN attr safEE value: %s", ee->entity.
				ee_entity.safEE);
		}
		else if (strcmp(attr_name, "saPlmEEType") == 0) {
			ee->entity.ee_entity.saPlmEEType.length =
			((SaNameT *)*((attrs[i])->attrValues))->length;
			memcpy(ee->entity.ee_entity.saPlmEEType.value,
			((SaNameT *)*((attrs[i])->attrValues))->value, 
			ee->entity.ee_entity.saPlmEEType.length);
		}
		else if (strcmp(attr_name, "saPlmEEInstantiateTimeout")==0) {
			ee->entity.ee_entity.saPlmEEInstantiateTimeout
			= *(SaTimeT *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEInstantiateTimeout attr value: %llu", 
				ee->entity.ee_entity.saPlmEEInstantiateTimeout);
		}	
		else if (strcmp(attr_name, "saPlmEETerminateTimeout")==0) {
			ee->entity.ee_entity.saPlmEETerminateTimeout
			= *(SaTimeT *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEETerminateTimeout attr value: %llu", 
				ee->entity.ee_entity.saPlmEETerminateTimeout);
		}
		else if (strcmp(attr_name, "saPlmEEAdminState")==0) {
			ee->entity.ee_entity.saPlmEEAdminState
			= *(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEAdminState attr value: %u", 
				ee->entity.ee_entity.saPlmEEAdminState);
		}
		else if (strcmp(attr_name, "saPlmEEReadinessState")==0) {
			ee->entity.ee_entity.saPlmEEReadinessState
			= *(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEReadinessState attr value: %u", 
				ee->entity.ee_entity.saPlmEEReadinessState);
		}
		else if (strcmp(attr_name, "saPlmEEReadinessFlags")==0) {
			ee->entity.ee_entity.saPlmEEReadinessFlags
			= *(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEReadinessFlags attr value: %llu", 
				ee->entity.ee_entity.saPlmEEReadinessFlags);
		}
		else if (strcmp(attr_name, "saPlmEEPresenceState")==0) {
			ee->entity.ee_entity.saPlmEEPresenceState
			= *(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEPresenceState attr value: %u", 
				ee->entity.ee_entity.saPlmEEPresenceState);
		}
		else if (strcmp(attr_name, "saPlmEEOperationalState")==0) {
			ee->entity.ee_entity.saPlmEEOperationalState
			= *(SaUint32T *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEEOperationalState attr value: %u", 
				ee->entity.ee_entity.saPlmEEOperationalState);
		}
	}
	/* Add the node to patricia tree */
	rc = ncs_patricia_tree_add(&plms_cb->entity_info, &ee->pat_node);
	if (rc == NCSCC_RC_FAILURE) {
		/* Log the error */
		LOG_CR("ncs_patricia_tree_add failed, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	memcpy(dn_name, obj_name->value, obj_name->length);
	ee->dn_name_str = calloc(SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
	if (ee->dn_name_str == NULL) {
		LOG_CR("memory allocation failed for ee->dn_name_str, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	strcpy(ee->dn_name_str, dn_name); 
	/* Get the parent's DN */   
	/* The string safEE=<> is skipped in below stmt */ 
	temp = strtok(dn_name, ",");
	temp = strtok(NULL, "\0");
	/* Now the parent's dn name is pointed by temp */
	/* Copy it to parent_dn variable */
	strcpy(parent_dn, temp);
	/* Get the rdn attr name from parent's dn pointed to by 
	   temp variable */
	rdn_name = strtok(temp, "=");
	/* Set the containment relationship now */
	/* If this is the first child added under it's parent,  
	   Set the leftmost child pointer of the parent to this
	   node. If the parent already has children, then 
	   traverse till the last child of the parent and then 
	   set the right sibling pointer of the last child to
	   this node. */
	if (strcmp(rdn_name, "safDomain") == 0) {
		domain = &plms_cb->domain_info;
		if(domain->leftmost_child == NULL) {
			domain->leftmost_child = ee;
		}
		else {
			tmp = domain->leftmost_child;
			while (tmp->right_sibling != NULL) {
				tmp = tmp->right_sibling;
			}
			tmp->right_sibling = ee;
		}
		ee->parent = NULL;/* This is because parent ptr is of type 
		PLMS_ENTITY */
	}
	else {
		memset(&key_dn, 0, sizeof(SaNameT));
		key_dn.length = strlen(parent_dn);
		memcpy(key_dn.value, parent_dn, key_dn.length);
		parent_ent=(PLMS_ENTITY *)ncs_patricia_tree_get(
		&plms_cb->entity_info, (SaUint8T *)&key_dn);
		if (parent_ent->leftmost_child == NULL) {
			parent_ent->leftmost_child = ee;
		}
		else {
			tmp = parent_ent->leftmost_child;
			while(tmp->right_sibling != NULL) {
				tmp = tmp->right_sibling;
			}
			tmp->right_sibling = ee;
		}
		ee->parent = parent_ent;
	}
	/* Admin state need not be updated as default value will be in XML*/ 
	rc = plms_attr_imm_update(ee,"saPlmEEReadinessState",
				SA_IMM_ATTR_SAUINT32T,ee->entity.ee_entity.
				saPlmEEReadinessState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Readiness state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", ee->dn_name_str, 
		ee->entity.ee_entity.saPlmEEReadinessState, rc);
	}
	rc = plms_attr_imm_update(ee,"saPlmEEPresenceState",
				SA_IMM_ATTR_SAUINT32T,ee->entity.ee_entity.
				saPlmEEPresenceState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Presence state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", ee->dn_name_str, 
		ee->entity.ee_entity.saPlmEEPresenceState, rc);
	}
	rc = plms_attr_imm_update(ee,"saPlmEEOperationalState",
				SA_IMM_ATTR_SAUINT32T,ee->entity.ee_entity.
				saPlmEEOperationalState);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Operational state IMM update FAILED. Ent: %s, \
		State: %d, error: %d", ee->dn_name_str, 
		ee->entity.ee_entity.saPlmEEOperationalState, rc);
	}
	rc = plms_attr_imm_update(ee,"saPlmEEReadinessFlags",
				SA_IMM_ATTR_SAUINT64T,ee->entity.ee_entity.
				saPlmEEReadinessFlags);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Readiness flags IMM update FAILED. Ent: %s, \
		State: %llu, error: %d", ee->dn_name_str, 
		ee->entity.ee_entity.saPlmEEReadinessFlags, rc);
	}
	/*plms_check_n_add_ent_to_ent_groups(ee);*/
	TRACE_LEAVE2("EE object %s created", ee->dn_name_str);
}
static void plms_create_he_type_obj(SaNameT *obj_name, 
					SaImmAttrValuesT_2 **attrs)
{
	PLMS_HE_BASE_INFO *he_base_info;
	PLMS_HE_TYPE_INFO *tmp, *he_type; 
	SaStringT parent_dn_str, attr_name, *attr_val;
	SaNameT key_dn;
	SaUint8T i;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	memcpy(dn_name, obj_name->value, obj_name->length);
	parent_dn_str = strstr(dn_name, "safHEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(parent_dn_str);
	memcpy(key_dn.value, parent_dn_str, key_dn.length);
	he_base_info = (PLMS_HE_BASE_INFO *)
		ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *) 
					&key_dn);
	he_type = (PLMS_HE_TYPE_INFO *) calloc(1, sizeof(PLMS_HE_TYPE_INFO));
	if (he_type == NULL) {
		LOG_CR("memory allocation failed for he_type, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	he_type->next = NULL;
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safVersion") == 0) {
			he_type->he_type.safVersion = calloc(
			SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (he_type->he_type.safVersion == NULL) {
				LOG_CR("memory allocation failed for \
					safVersion, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(he_type->he_type.safVersion,
			*(SaStringT *)*(attrs[i]->attrValues));
		}
		else if (strcmp(attr_name, "saPlmHetIdr") == 0) {
			attr_val = (SaStringT *)*(attrs[i]->attrValues);
			if (*attr_val == NULL) {
				/* Log the message */
				TRACE_2("No value given for saPlmHetIdr attr");
			}
			else {
				he_type->he_type.saPlmHetIdr = calloc(
					strlen(*attr_val)+1, sizeof(SaInt8T));
				if (he_type == NULL) {
					LOG_CR("memory allocation failed for \
					saPlmHetIdr, calling assert now");
					TRACE_LEAVE();
					assert(0);
				}
				strcpy(he_type->he_type.saPlmHetIdr, *attr_val);
				plms_update_he_idr_data(he_type, attr_val);
				TRACE_2("saPlmHetIdr attr value: %s", 
					he_type->he_type.saPlmHetIdr);
			}
		}
	}
	tmp = he_base_info->he_type_info_list;
	if (tmp == NULL) {
		he_base_info->he_type_info_list = he_type;
		TRACE_2("First HEType object being created under its parent \
			HEBaseType");
	}
	else {			
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}	
		tmp->next = he_type; 
	}
	TRACE_LEAVE2("HEType object created");
}
static void plms_create_ee_type_obj(SaNameT *obj_name, 
					SaImmAttrValuesT_2 **attrs)
{
	PLMS_EE_BASE_INFO *ee_base_info;
	PLMS_EE_TYPE_INFO *tmp, *ee_type_info; 
	SaStringT parent_dn, attr_name;
	SaNameT key_dn;
	SaUint8T i;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();	
	memcpy(dn_name, obj_name->value, obj_name->length);
	parent_dn = strstr(dn_name, "safEEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(parent_dn);
	memcpy(key_dn.value, parent_dn, key_dn.length);
	ee_base_info = (PLMS_EE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)&key_dn);
	ee_type_info = (PLMS_EE_TYPE_INFO *) calloc(1, sizeof(PLMS_EE_TYPE_INFO));
	if (ee_type_info == NULL) {
		LOG_CR("memory allocation failed for ee_type_info, \
			calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	ee_type_info->next = NULL;
	ee_type_info->parent = ee_base_info;
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = (attrs[i])->attrName;
		if (attrs[i]->attrValues == NULL) {
			TRACE_2("No value given for attr %s", attr_name);
			continue;
		}
		if (strcmp(attr_name, "safVersion") == 0) {
			ee_type_info->ee_type.safVersion = calloc(
				SA_MAX_NAME_LENGTH+1, sizeof(SaInt8T));
			if (ee_type_info->ee_type.safVersion == NULL) {
				LOG_CR("memory allocation failed for \
					safVersion, calling assert now");
				TRACE_LEAVE();
				assert(0);
			}
			strcpy(ee_type_info->ee_type.safVersion,
			*(SaStringT *)*(attrs[i]->attrValues));
		}
		else if (strcmp(attr_name,
		"saPlmEetDefInstantiateTimeout") == 0) {
			ee_type_info->ee_type.saPlmEeDefaultInstantiateTimeout
			= *(SaTimeT *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEeDefaultInstantiateTimeout attr value:\
			%llu", ee_type_info->ee_type.
					saPlmEeDefaultInstantiateTimeout);
		}
		else if (strcmp(attr_name,
		"saPlmEetDefTerminateTimeout") == 0) {
			ee_type_info->ee_type.saPlmEeDefTerminateTimeout
			= *(SaTimeT *)*((attrs[i])->attrValues);
			TRACE_2("saPlmEeDefTerminateTimeout attr value: %llu", 
			ee_type_info->ee_type.saPlmEeDefTerminateTimeout);
		}
	}
	tmp = ee_base_info->ee_type_info_list;
	if (tmp == NULL) {
		ee_base_info->ee_type_info_list = ee_type_info;
		TRACE_2("First EEType object being created under its parent \
			EEBaseType");
	}
	else {			
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}	
		tmp->next = ee_type_info; 
	}
	TRACE_LEAVE2("EEType object created");
}

static void plms_modify_domain_obj(SaNameT *obj_name, 
					SaImmAttrModificationT_2 **attrs)
{
	SaInt32T i;
	SaImmAttrNameT attr_name;
	TRACE_ENTER();	
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = attrs[i]->modAttr.attrName;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, 
			attrs[i]->modType);
		if (strcmp(attr_name, "saPlmHEDeactivationPolicy") ==0) {
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				/* set to default */
				plms_cb->domain_info.domain.
				saPlmHEDeactivationPolicy = SA_PLM_DP_VALIDATE;
			}
			else {
				plms_cb->domain_info.domain.
				saPlmHEDeactivationPolicy = 
				*(SaPlmHEDeactivationPolicyT *)
				*(attrs[i]->modAttr.attrValues);
				TRACE_2("new attr val: %u", plms_cb->
				domain_info.domain.saPlmHEDeactivationPolicy);
			}
		}
	}
	TRACE_LEAVE2("Domain object modified");
}
static void plms_modify_dependency_obj(SaNameT *obj_name, 
				SaImmAttrModificationT_2 **attrs)
{
	PLMS_ENTITY *plm_ent, *ent_node;
	SaStringT temp, attr_name;
	SaNameT key_dn, *temp_dn;
	SaUint8T i, j;
	SaUint32T attr_val_num, rc;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();	
	memcpy(dn_name, obj_name->value, obj_name->length);

	/* Get the parent DN */
	/* Skip the rdn attribute of the dependency object */ 
	/* The string safDependency=<> is skipped in below stmt */ 
	temp = strtok(dn_name, ",");
	temp = strtok(NULL, "\0");
	/* Now the parent's dn name is pointed by temp */
	
	memset(&key_dn, 0, sizeof(SaNameT));
	memcpy(key_dn.value, temp, strlen(temp));
	key_dn.length = strlen(temp);
	plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_get(&plms_cb->entity_info, (SaUint8T *) 
					&key_dn);
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = attrs[i]->modAttr.attrName;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->
			modType);
		if (strcmp(attr_name, "saPlmDepNames")==0) {
			attr_val_num = attrs[i]->modAttr.attrValuesNumber;
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				for (j=0; j < attr_val_num; j++) {
					temp_dn = (SaNameT *)
					*(attrs[i]->modAttr.attrValues+j);
					ent_node = (PLMS_ENTITY *)
					ncs_patricia_tree_get(&plms_cb->
						entity_info, (SaUint8T *)
						temp_dn);
					TRACE_2("DepName value: %s", 
						ent_node->dn_name_str);
					/* remove ent_node from dep_list of 
					   plm_ent */
					rem_ent2_from_dep_list_of_ent1(plm_ent,
					ent_node); 
					/* remove plm_ent from rev_dep_list of
					   ent_node */
					rem_ent1_from_rev_dep_list_of_ent2(
					plm_ent, ent_node); 
				}
				continue; /* go to the next attribute */
			}
			else if(attrs[i]->modType==SA_IMM_ATTR_VALUES_REPLACE) {
				/* Free the dependency list */
				remove_ent_from_rev_dep_list_of_others(plm_ent);
				free_dep_list(plm_ent);
			}
			/* Following stmts common for replace and add */
			attr_val_num = attrs[i]->modAttr.attrValuesNumber;
			for (j=0; j < attr_val_num; j++) {
				temp_dn = (SaNameT *)
				*(attrs[i]->modAttr.attrValues+j);
				/* Fill the dependency list of entity */
				/*Get the ptr to this dependent entity*/
				ent_node = (PLMS_ENTITY *)
				ncs_patricia_tree_get(&plms_cb->entity_info,
					(SaUint8T *)(temp_dn));
				TRACE_2("DepName value: %s", 
						ent_node->dn_name_str);
				rc = add_ent2_to_dep_list_of_ent1(plm_ent, 
					ent_node);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("add_ent2_to_dep_list_of_ent1 \
					returned error %u, calling assert", rc);
					assert(0);
				}
				/* Update the rev_dep_list of ent_node 
				   with the current plm_ent */
				rc = add_ent1_to_rev_dep_list_of_ent2(plm_ent, 
					ent_node);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("add_ent1_2_rev_dep_list_of_ent2\
					returned error %u, calling assert", rc);
					assert(0);
				}
			}
		}
		else if (strcmp(attr_name, "saPlmDepMinNumber")==0) {
			plm_ent->min_no_dep =
			*(SaUint32T *)(*(attrs[i])->modAttr.attrValues);
			TRACE_2("new attr value: %u", plm_ent->min_no_dep);
		}
	}
	TRACE_LEAVE2("Dependency object modified");
}
static void plms_modify_he_obj(SaNameT *obj_name, 
				SaImmAttrModificationT_2 **attrs)
{
	PLMS_ENTITY *he;
	SaStringT *attr_val, *ent_path, str, attr_name;
	SaUint32T i,j,k,n, attr_val_num;
	TRACE_ENTER();	
	he = (PLMS_ENTITY *) ncs_patricia_tree_get(&plms_cb->entity_info, 
		(SaUint8T *)obj_name);
	if (he == NULL) {
		LOG_ER("HE object to be modified is not found, calling assert\
			now");
		TRACE_LEAVE();
		assert(0);
	}
	ent_path = he->entity.he_entity.saPlmHEEntityPaths;
	for (k=0; attrs[k] != NULL; k++) {
		attr_name = attrs[k]->modAttr.attrName;
		attr_val_num = attrs[k]->modAttr.attrValuesNumber;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[k]->
			modType);
		if (strcmp(attr_name, "saPlmHEEntityPaths") ==0) {
			if (attrs[k]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				for(i=0; i < attr_val_num; i++) {
					attr_val = (SaStringT *)*(attrs[k]->
						modAttr.attrValues+i);
					for(j=0; j<he->num_cfg_ent_paths; j++) {
						TRACE_2("attr value: %s", 
							*attr_val);
						if (strcmp(*attr_val, 
							ent_path[j])==0) {
							TRACE_2("attr value \
							found");
							break; /* from loop j */
						}
					}
					if (j == he->num_cfg_ent_paths) {
						   LOG_ER("attr value not found\
						   in the data structures,\
						   calling assert now");
						   TRACE_LEAVE();
						   assert(0);
					}
					free(ent_path[j]);
					for (n=j; n < (he->num_cfg_ent_paths); 
						n++) {
						ent_path[n] = ent_path[n+1];
					}
					--he->num_cfg_ent_paths;
				}
				continue;
			}
			else if (attrs[k]->modType == 
				SA_IMM_ATTR_VALUES_REPLACE) {
				/* free all the pointers in ent_path */
				for (j=0; j < he->num_cfg_ent_paths; j++) {
					free(ent_path[j]);
					ent_path[j] = NULL;
				}
				he->num_cfg_ent_paths = 0;
			}
			/* Following stmts common for REPLACE and ADD */
			for (i=0; i < attr_val_num; i++) {
				attr_val = (SaStringT *)*(attrs[k]->modAttr.
						attrValues+i);
				str = calloc(strlen(*attr_val)+1, 
					sizeof(SaInt8T));
				if (str == NULL) {
					LOG_CR("memory allocation failed for \
					str, calling assert now");
					TRACE_LEAVE();
					assert(0);
				}
				strcpy(str, *attr_val);
				ent_path[he->num_cfg_ent_paths++] = str;
			}
			TRACE_2("No of config entity paths: %u", 
					he->num_cfg_ent_paths);
		}
	}
	TRACE_LEAVE2("HE object %s modified", he->dn_name_str);
	return;
}
static void plms_modify_ee_obj(SaNameT *obj_name, 
				SaImmAttrModificationT_2 **attrs)
{
	PLMS_ENTITY *ee;
	SaNameT *sa_name_ptr;
	SaUint8T i;
	SaStringT attr_name;
		
	TRACE_ENTER();	
	ee = (PLMS_ENTITY *) ncs_patricia_tree_get(&plms_cb->entity_info, 
		(SaUint8T *)obj_name);
	if (ee == NULL) {
		LOG_ER("EE object to be modified is not found, calling assert \
			now");
		TRACE_LEAVE();
		assert(0);
	}
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = attrs[i]->modAttr.attrName;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->
			modType);
		if (strcmp(attr_name, "saPlmEEType") ==0) {
			sa_name_ptr =(SaNameT *)*(attrs[i]->modAttr.attrValues);
			ee->entity.ee_entity.saPlmEEType.length = 
							sa_name_ptr->length;
			memcpy(ee->entity.ee_entity.saPlmEEType.value,
			sa_name_ptr->value, sa_name_ptr->length);
		}
		else if (strcmp(attr_name, "saPlmEEInstantiateTimeout")==0) {
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				ee->entity.ee_entity.saPlmEEInstantiateTimeout
				= 0;
			}
			else {
				ee->entity.ee_entity.saPlmEEInstantiateTimeout
				= *(SaTimeT *)*(attrs[i]->modAttr.attrValues);
				TRACE_2("attr value: %llu",ee->entity.ee_entity.
					saPlmEEInstantiateTimeout);
			}
		}	
		else if (strcmp(attr_name, "saPlmEETerminateTimeout")==0) {
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				ee->entity.ee_entity.saPlmEETerminateTimeout
				= 0;
			}
			else {
				ee->entity.ee_entity.saPlmEETerminateTimeout
				= *(SaTimeT *)*(attrs[i]->modAttr.attrValues);
				TRACE_2("attr value: %llu",ee->entity.ee_entity.
					saPlmEETerminateTimeout);
			}
		}
	}
	TRACE_LEAVE2("EE object %s modified", ee->dn_name_str);
}
static void plms_modify_he_type_obj(SaNameT *obj_name, 
				SaImmAttrModificationT_2 **attrs)
{
	PLMS_HE_BASE_INFO *he_base_info;
	SaStringT parent_dn, rdn, attr_name, *attr_val;
	SaNameT key_dn;
	SaUint8T i;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();	
	memcpy(dn_name, obj_name->value, obj_name->length);
	parent_dn = strstr(dn_name, "safHEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(parent_dn);
	memcpy(key_dn.value, parent_dn, key_dn.length);
	he_base_info = (PLMS_HE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *)&key_dn);
	if (he_base_info == NULL) {
		LOG_ER("Parent HEBaseType object is not found, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	rdn = strtok(dn_name, ",");

	PLMS_HE_TYPE_INFO *he_type_node = he_base_info->he_type_info_list;
	while(he_type_node != NULL) {
		if (strcmp(he_type_node->he_type.safVersion, rdn) ==0) {
			break;
		}
		he_type_node = he_type_node->next;
	}
	if (he_type_node == NULL) {
		LOG_ER("HEType object to be modified is not found, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = attrs[i]->modAttr.attrName;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->
			modType);
		if (strcmp(attr_name,"saPlmHetIdr") == 0) {
			attr_val = (SaStringT *)*(attrs[i]->modAttr.attrValues);
			if ((attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) ||
				((attrs[i]->modType == 
				SA_IMM_ATTR_VALUES_REPLACE) && (*attr_val == 
					NULL))) {
				free_he_idr_data(he_type_node);
				free(he_type_node->he_type.saPlmHetIdr);
				he_type_node->he_type.saPlmHetIdr = NULL;
			}
			else { 
				he_type_node->he_type.saPlmHetIdr = realloc(
					he_type_node->he_type.saPlmHetIdr, 
					strlen(*attr_val)+1);
				memset(he_type_node->he_type.saPlmHetIdr, 0, 
					strlen(*attr_val)+1);
				strcpy(he_type_node->he_type.saPlmHetIdr, 
					*attr_val);
				TRACE_2("attr value: %s", *attr_val);
				plms_update_he_idr_data(he_type_node, attr_val);
			}
		}
	}
	TRACE_LEAVE2("HEType object modified");
}
static void plms_modify_ee_type_obj(SaNameT *obj_name, 
				SaImmAttrModificationT_2 **attrs)
{
	PLMS_EE_BASE_INFO *ee_base_info;
	SaStringT parent_dn, rdn, attr_name;
	SaNameT key_dn;
	SaUint8T i;
	SaInt8T dn_name[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();	
	memcpy(dn_name, obj_name->value, obj_name->length);
	parent_dn = strstr(dn_name, "safEEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(parent_dn);
	memcpy(key_dn.value, parent_dn, key_dn.length);
	ee_base_info = (PLMS_EE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)&key_dn);
	if (ee_base_info == NULL) {
		LOG_ER("parent EEBaseType object is not found, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	rdn = strtok(dn_name, ",");

	PLMS_EE_TYPE_INFO *ee_type_node = ee_base_info->ee_type_info_list;
	while(ee_type_node != NULL) {
		if (strcmp(ee_type_node->ee_type.safVersion, rdn) ==0) {
			break;
		}
		ee_type_node = ee_type_node->next;
	}
	if (ee_type_node == NULL) {
		LOG_ER("EEType object to be modified is not found, calling \
			assert now");
		TRACE_LEAVE();
		assert(0);
	}
	for (i=0; attrs[i] != NULL; i++) {
		attr_name = attrs[i]->modAttr.attrName;
		TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->
			modType);
		if (strcmp(attr_name, "saPlmEetDefInstantiateTimeout") == 0) {
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				ee_type_node->ee_type.
				saPlmEeDefaultInstantiateTimeout = 0;
			}
			else {
				ee_type_node->ee_type.
				saPlmEeDefaultInstantiateTimeout
				= *(SaTimeT *)*(attrs[i]->modAttr.attrValues);
				TRACE_2("attr value: %llu", ee_type_node->
				ee_type.saPlmEeDefaultInstantiateTimeout);
			}
		}
		else if(strcmp(attr_name, "saPlmEetDefTerminateTimeout") == 0) {
			if (attrs[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
				ee_type_node->ee_type.
				saPlmEeDefTerminateTimeout = 0;
			}
			else {
				ee_type_node->ee_type.saPlmEeDefTerminateTimeout
				= *(SaTimeT *)*(attrs[i]->modAttr.attrValues);
				TRACE_2("attr value: %llu", ee_type_node->
				ee_type.saPlmEeDefTerminateTimeout);
			}
		}
	}
	TRACE_LEAVE2("EEType object modified");
	return;
}
static void rem_ent2_from_dep_list_of_ent1(PLMS_ENTITY *ent1, 
						PLMS_ENTITY *ent2) 
{
	PLMS_GROUP_ENTITY *temp_dep, *next_dep;
	TRACE_ENTER2("ent1:%s, ent2:%s", ent1->dn_name_str, ent2->dn_name_str);
	temp_dep = ent1->dependency_list;
	if (temp_dep->plm_entity == ent2) {
		TRACE_2("ent2 to be removed is found in the first node itself");
		ent1->dependency_list = temp_dep->next;
		free(temp_dep);
	}
	else {
		while(temp_dep->next !=NULL) {
			next_dep=temp_dep->next;
			if (next_dep->plm_entity==ent2) {
				TRACE_2("ent2 to be removed is found");
				temp_dep->next = next_dep->next;
				free(next_dep);
				break;
			}
			temp_dep = temp_dep->next;
		}
	}
	TRACE_LEAVE();
	return;
}
static void rem_ent1_from_rev_dep_list_of_ent2(PLMS_ENTITY *ent1, 
						PLMS_ENTITY *ent2)
{
	PLMS_GROUP_ENTITY *rev_dep, *next_rev_dep;
	TRACE_ENTER2("ent1:%s, ent2:%s", ent1->dn_name_str, ent2->dn_name_str);
	rev_dep = ent2->rev_dep_list;
	if (rev_dep == NULL) {
		LOG_ER("rev_dep_list of ent2 is null, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	if (rev_dep->plm_entity == ent1) {
		TRACE_2("ent1 to be removed is found in the first node itself");
		ent2->rev_dep_list = rev_dep->next;
		free(rev_dep);
	}
	else {
		while (rev_dep->next != NULL) {
			next_rev_dep = rev_dep->next;
			if (next_rev_dep->plm_entity == ent1) {
				/* Entity matched, remove it */
				TRACE_2("ent2 to be removed is found");
				rev_dep->next = next_rev_dep->next;
				free(next_rev_dep);
				break;
			}
			rev_dep = rev_dep->next;
		}
	}
	TRACE_LEAVE();
	return;
}
static SaInt32T add_ent2_to_dep_list_of_ent1(PLMS_ENTITY *ent1, 
						PLMS_ENTITY *ent2)
{
	PLMS_GROUP_ENTITY *dep_ent, *temp_dep;
	TRACE_ENTER2("ent1:%s, ent2:%s", ent1->dn_name_str, ent2->dn_name_str);
	dep_ent = (PLMS_GROUP_ENTITY *) malloc(sizeof(PLMS_GROUP_ENTITY));
	if (dep_ent == NULL) {
		LOG_ER("memory allocation failed for dep_ent");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	dep_ent->plm_entity = ent2;
	dep_ent->next = NULL;
	if (ent1->dependency_list == NULL) {
		ent1->dependency_list = dep_ent;
	}
	else {
		temp_dep = ent1->dependency_list;
		while (temp_dep->next != NULL) {
			temp_dep=temp_dep->next;
		}
		temp_dep->next = dep_ent;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
static SaInt32T add_ent1_to_rev_dep_list_of_ent2(PLMS_ENTITY *ent1, 
						PLMS_ENTITY *ent2)
{
	PLMS_GROUP_ENTITY *rev_dep, *temp_dep;
	TRACE_ENTER2("ent1:%s, ent2:%s", ent1->dn_name_str, ent2->dn_name_str);
	rev_dep = (PLMS_GROUP_ENTITY *) malloc(sizeof(PLMS_GROUP_ENTITY));   
	if (rev_dep == NULL) {
		LOG_ER("memory allocation failed for rev_dep");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	rev_dep->plm_entity = ent1;
	rev_dep->next = NULL;
	if (ent2->rev_dep_list == NULL) {
		ent2->rev_dep_list = rev_dep;
	}
	else {
		temp_dep = ent2->rev_dep_list;
		while (temp_dep->next != NULL) {
			temp_dep=temp_dep->next;
		}
		temp_dep->next = rev_dep;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
static void plms_update_he_idr_data(PLMS_HE_TYPE_INFO *he_type, 
						SaStringT *attr_val)
{
	SaStringT str, idr_area, idr_field, idr_field_val;
	SaUint32T val_len;
	TRACE_ENTER();
	/* Parse the attr_val for product/chassis/board related information
	   and copy them to PLMS_INV_DATA in PLM_HE_TYPE_INFO */
	/* Currently we are supporting only one string in the attribute value,
	   so only attr_val[0] is parsed */
	/* copy the attr_val[0] to local string variable */
	str = calloc(strlen(attr_val[0])+1, sizeof(SaInt8T));
	if (str == NULL) {
		LOG_CR("memory allocation failed for str, calling assert now");
		TRACE_LEAVE();
		assert(0);
	}
	strcpy(str, attr_val[0]);
	idr_area = strtok(str, "/");
	if (idr_area == NULL) {
		LOG_ER("IDR area name not specified");
		/* This is possible if null value is replacing the existing value */
		free_he_idr_data(he_type);
		free(str);
		return;
	}
	TRACE_2("IDR area name: %s", idr_area);
	do {
		idr_field = strtok(NULL, "=");
		if (idr_field == NULL) {
			LOG_ER("IDR field name not specified");   
			break;   
		}
		TRACE_2("IDR field name: %s", idr_field);
		idr_field_val = strtok(NULL, ",");
		if (idr_field_val == NULL) {
			LOG_ER("IDR field value not specified");   
			break;   
		}
		val_len = strlen(idr_field_val);
		TRACE_2("IDR field value: %s", idr_field_val);
		/* check if this is product/chassis/board area info */
		if (strcmp(idr_area, "PRODUCT")==0) {
			if (strcmp(idr_field, "ASSET_TAG")==0) {
				memcpy(he_type->inv_data.product_area.
				asset_tag.Data, idr_field_val, 
				val_len);
				he_type->inv_data.product_area.
				asset_tag.DataLength = val_len;
			}
			else if (strcmp(idr_field, "MANUFACTURER")==0) {
				memcpy(he_type->inv_data.product_area.
				manufacturer_name.Data, idr_field_val,
				val_len);
				he_type->inv_data.product_area.
				manufacturer_name.DataLength = val_len;
			}
			else if (strcmp(idr_field, "PART_NUMBER")==0) {
				memcpy(he_type->inv_data.product_area.
				part_no.Data, idr_field_val, val_len);
				he_type->inv_data.product_area.
				part_no.DataLength = val_len;
			}
			else if (strcmp(idr_field, "PRODUCT_NAME")==0) {
				memcpy(he_type->inv_data.product_area.
				product_name.Data, idr_field_val, 
				val_len);
				he_type->inv_data.product_area.
				product_name.DataLength = val_len;
			}
			else if (strcmp(idr_field, "PRODUCT_VERSION")
				==0) {
				memcpy(he_type->inv_data.product_area.
				product_version.Data, idr_field_val, 
				val_len);
				he_type->inv_data.product_area.
				product_version.DataLength = val_len;
			}
			else if (strcmp(idr_field, "SERIAL_NUMBER")
				==0) {
				memcpy(he_type->inv_data.product_area.
				serial_no.Data, idr_field_val,val_len);
				he_type->inv_data.product_area.
				serial_no.DataLength = val_len;
			}
			else {
				/* Field which we are not storing 
				   might have come, log it */
			}
		}
		else if (strcmp(idr_area, "CHASSIS")==0) {
			if (strcmp(idr_field, "CHASSIS_TYPE")==0) {
				he_type->inv_data.chassis_area.
				chassis_type = atoi(idr_field_val); 
			}
			else if (strcmp(idr_field, "PART_NUMBER") ==0) {
				memcpy(he_type->inv_data.chassis_area.
				part_no.Data, idr_field_val,val_len);
				he_type->inv_data.chassis_area.
				part_no.DataLength = val_len;
			}
			else if (strcmp(idr_field, "SERIAL_NUMBER")
				==0) {
				memcpy(he_type->inv_data.chassis_area.
				serial_no.Data, idr_field_val,val_len);
				he_type->inv_data.chassis_area.
				serial_no.DataLength = val_len;
			}
			else {
				/* Field which we are not storing 
				   might have come, log it */
			}
		}
		else if (strcmp(idr_area, "BOARD")==0) {
			if (strcmp(idr_field, "MANUFACTURER")==0) {
				memcpy(he_type->inv_data.board_area.
				manufacturer_name.Data, idr_field_val,
				val_len);
				he_type->inv_data.board_area.
				manufacturer_name.DataLength = val_len;
			}
			else if (strcmp(idr_field, "PART_NUMBER") ==0) {
				memcpy(he_type->inv_data.board_area.
				part_no.Data, idr_field_val,val_len);
				he_type->inv_data.board_area.
				part_no.DataLength = val_len;
			}
			else if (strcmp(idr_field, "PRODUCT_NAME")==0) {
				memcpy(he_type->inv_data.board_area.
				product_name.Data, idr_field_val, 
				val_len);
				he_type->inv_data.board_area.
				product_name.DataLength = val_len;
			}
			else if (strcmp(idr_field, "SERIAL_NUMBER")
				==0) {
				memcpy(he_type->inv_data.board_area.
				serial_no.Data, idr_field_val,val_len);
				he_type->inv_data.board_area.
				serial_no.DataLength = val_len;
			}
			else {
				/* Field which we are not storing 
				   might have come, log it */
			}
		}
		else {
			/* Area which we are not storing has come, log it */
		}
		idr_area = strtok(NULL, "/");
		if (idr_area == NULL) {
			LOG_ER("IDR area name not specified");
			free(str);
			return;
		}
		TRACE_2("IDR area name: %s", idr_area);
	}while(idr_area != NULL);
	free(str);
	TRACE_LEAVE();
	return;
}
static void free_he_idr_data(PLMS_HE_TYPE_INFO *he_type)
{
	TRACE_ENTER();
	he_type->inv_data.product_area.asset_tag.DataLength = 0;
	he_type->inv_data.product_area.manufacturer_name.DataLength =0;
	he_type->inv_data.product_area.part_no.DataLength = 0;
	he_type->inv_data.product_area.product_name.DataLength = 0;
	he_type->inv_data.product_area.product_version.DataLength = 0;
	he_type->inv_data.product_area.serial_no.DataLength = 0;
	he_type->inv_data.chassis_area.chassis_type = 0;
	he_type->inv_data.chassis_area.part_no.DataLength = 0;
	he_type->inv_data.chassis_area.serial_no.DataLength = 0;
	he_type->inv_data.board_area.manufacturer_name.DataLength = 0;
	he_type->inv_data.board_area.part_no.DataLength = 0;
	he_type->inv_data.board_area.product_name.DataLength = 0;
	he_type->inv_data.board_area.serial_no.DataLength = 0;
	TRACE_LEAVE();
	return;
}
static void plms_delete_plm_ent(PLMS_ENTITY *plm_ent)
{
	SaUint8T i;
	TRACE_ENTER2("entity being deleted: %s", plm_ent->dn_name_str);
	/* Free the rdn, dep_list and rev_dep_list in plm_ent */
	if (plm_ent->entity_type == PLMS_HE_ENTITY) {
		free(plm_ent->entity.he_entity.safHE);
		/* Free ent_path pointers */
		for (i=0; i<plm_ent->num_cfg_ent_paths; i++) {
			free(plm_ent->entity.he_entity.saPlmHEEntityPaths[i]);
		}
		if (plm_ent->entity.he_entity.saPlmHECurrEntityPath != NULL) {
			free(plm_ent->entity.he_entity.saPlmHECurrEntityPath);
		}
	}
	else if (plm_ent->entity_type == PLMS_EE_ENTITY) {
		free(plm_ent->entity.ee_entity.safEE);
	}
	remove_ent_from_rev_dep_list_of_others(plm_ent);
	free_dep_list(plm_ent);
	if (plm_ent->rev_dep_list != NULL) {
		/* This should not happen, log or assert */
		LOG_ER("entity's rev_dep_list is not null, asserting now");
		assert(0);
	}
	plms_check_n_del_ent_from_ent_groups(plm_ent);
	free_entity_group_info_list(plm_ent->part_of_entity_groups);
	free_entity_group_info_list(plm_ent->cbk_grp_list);
	if (plm_ent->trk_info != NULL) {
		free_track_info(plm_ent->trk_info);
	}
	ncs_patricia_tree_del(&plms_cb->entity_info, &plm_ent->pat_node);
	free(plm_ent->dn_name_str);
	free(plm_ent);
	TRACE_LEAVE2("HE/EE object deleted");
}
static void plms_delete_he_base_type_obj(SaNameT *obj_name) 
{
	PLMS_HE_BASE_INFO *he_base_type_node;
	PLMS_HE_TYPE_INFO *he_type_node, *tmp;
	TRACE_ENTER();
	he_base_type_node = (PLMS_HE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *)obj_name);
	/* No need to check if he_base_type_node is null */
	free (he_base_type_node->he_base_type.safHEType);
	free (he_base_type_node->he_base_type.saPlmHetHpiEntityType);
	/* Delete/Free the child HE Type object list */
	tmp = he_base_type_node->he_type_info_list;
	while (tmp != NULL) {
		he_type_node = tmp;
		tmp = tmp->next;
		plms_free_he_type_obj(he_type_node);
	}
	ncs_patricia_tree_del(&plms_cb->base_he_info, 
				&he_base_type_node->pat_node);
	free(he_base_type_node);
	TRACE_LEAVE2("HEBaseType object deleted");
}
static void plms_delete_ee_base_type_obj(SaNameT *obj_name) 
{
	PLMS_EE_BASE_INFO *ee_base_type_node;
	PLMS_EE_TYPE_INFO *ee_type_node, *tmp;
	TRACE_ENTER();
	ee_base_type_node = (PLMS_EE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)obj_name);
	/* No need to check if ee_base_type_node is null */
	free (ee_base_type_node->ee_base_type.safEEType);
	free (ee_base_type_node->ee_base_type.saPlmEetProduct);
	free (ee_base_type_node->ee_base_type.saPlmEetVendor);
	free (ee_base_type_node->ee_base_type.saPlmEetRelease);
	/* Delete/Free the child EE Type object list */
	tmp = ee_base_type_node->ee_type_info_list;
	while (tmp != NULL) {
		ee_type_node = tmp;
		tmp = tmp->next;
		plms_free_ee_type_obj(ee_type_node);
	}
	ncs_patricia_tree_del(&plms_cb->base_ee_info, &ee_base_type_node->
				pat_node);
	free(ee_base_type_node);
	TRACE_LEAVE2("EEBaseType object deleted");
}
static void plms_delete_he_type_obj(SaNameT *obj_name)
{
	SaStringT he_base_type_dn, rdn;
	SaNameT key_dn;
	PLMS_HE_BASE_INFO *he_base_type_node;
	PLMS_HE_TYPE_INFO *he_type_node, *next_he_type_node;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};

	TRACE_ENTER();
	strncpy(tmp_dn, (SaInt8T *)obj_name->value, obj_name->length);
	he_base_type_dn = strstr(tmp_dn, "safHEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(he_base_type_dn);
	strncpy((SaInt8T *)key_dn.value, he_base_type_dn, key_dn.length);
	he_base_type_node = (PLMS_HE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_he_info, (SaUint8T *)&key_dn);
	rdn = strtok(tmp_dn, ",");
	he_type_node = he_base_type_node->he_type_info_list;
	if (strcmp(rdn, he_type_node->he_type.safVersion) == 0) {
		/* This is the node to be deleted */
		TRACE_2("First HEType child under HEBaseType is the object \
			being deleted");
		he_base_type_node->he_type_info_list = he_type_node->next;
		plms_free_he_type_obj(he_type_node);
	}
	else {
		while (he_type_node->next != NULL) {
			next_he_type_node = he_type_node->next;
			if (strcmp(rdn, next_he_type_node->he_type.safVersion) 
				== 0) {
				/* Found the node to be deleted, adjust the 
				   pointers */
				TRACE_2("Found the HEType object and it is \
					being deleted");
				he_type_node->next = next_he_type_node->next;
				plms_free_he_type_obj(next_he_type_node);
				break;
			}
			he_type_node = he_type_node->next;
		}
	}
	TRACE_LEAVE2("HEType object deleted");
}
static void plms_delete_ee_type_obj(SaNameT *obj_name)
{
	SaStringT ee_base_type_dn, rdn;
	SaNameT key_dn;
	PLMS_EE_BASE_INFO *ee_base_type_node;
	PLMS_EE_TYPE_INFO *ee_type_node, *next_ee_type_node;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};

	TRACE_ENTER();
	strncpy(tmp_dn, (SaInt8T *)obj_name->value, obj_name->length);
	ee_base_type_dn = strstr(tmp_dn, "safEEType");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(ee_base_type_dn);
	strncpy((SaInt8T *)key_dn.value, ee_base_type_dn, key_dn.length);
	ee_base_type_node = (PLMS_EE_BASE_INFO *)
	ncs_patricia_tree_get(&plms_cb->base_ee_info, (SaUint8T *)&key_dn);
	rdn = strtok(tmp_dn, ",");
	ee_type_node = ee_base_type_node->ee_type_info_list;
	if (strcmp(rdn, ee_type_node->ee_type.safVersion) == 0) {
		/* This is the node to be deleted */
		TRACE_2("First EEType child under EEBaseType is the object \
			being deleted");
		ee_base_type_node->ee_type_info_list = ee_type_node->next;
		plms_free_ee_type_obj(ee_type_node);
	}
	else {
		while (ee_type_node->next != NULL) {
			next_ee_type_node = ee_type_node->next;
			if (strcmp(rdn, next_ee_type_node->ee_type.safVersion) 
				== 0) {
				/* Found the node to be deleted, adjust the 
				   pointers */
				TRACE_2("Found the EEType object and it is \
					being deleted");
				ee_type_node->next = next_ee_type_node->next;
				plms_free_ee_type_obj(next_ee_type_node);
				break;
			}
			ee_type_node = ee_type_node->next;
		}
	}
	TRACE_LEAVE2("EEType object deleted");
}
static void plms_free_he_type_obj(PLMS_HE_TYPE_INFO *he_type_node)
{
	TRACE_ENTER();
	/* Free the members of the he_type_node */
	free(he_type_node->he_type.safVersion);
	free(he_type_node->he_type.saPlmHetIdr);
	/* Free plms_inv_data */
	free_he_idr_data(he_type_node);
	free(he_type_node);
	TRACE_LEAVE();
}
static void plms_free_ee_type_obj(PLMS_EE_TYPE_INFO *ee_type_node)
{
	TRACE_ENTER();
	/* Free the members of the ee_type_node */
	free(ee_type_node->ee_type.safVersion);
	free(ee_type_node);
	TRACE_LEAVE();
}
static void plms_delete_dep_obj(SaNameT *obj_name)
{
	SaStringT rdn, parent_dn;
	SaNameT key_dn;
	PLMS_ENTITY *plm_ent;
	SaInt8T tmp_dn[SA_MAX_NAME_LENGTH+1] = {0};
	TRACE_ENTER();
	/* Get the parent_dn and then plm_ent from the patricia tree */
	strncpy(tmp_dn, (SaInt8T *)obj_name->value, obj_name->length);
	rdn = strtok(tmp_dn, ",");
	parent_dn = strtok(NULL, "\0");
	memset(&key_dn, 0, sizeof(SaNameT));
	key_dn.length = strlen(parent_dn);
	memcpy(key_dn.value, parent_dn, key_dn.length);
	/* Get the node from the patricia tree */
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_get(&plms_cb->entity_info, 
				(SaUint8T *)&key_dn);
	remove_ent_from_rev_dep_list_of_others(plm_ent);
	free_dep_list(plm_ent);
	TRACE_LEAVE2("dependency object of %s deleted", 
		plm_ent->dn_name_str);
}
static void free_plms_group_entity(PLMS_GROUP_ENTITY *grp_ent) 
{
	PLMS_GROUP_ENTITY *tmp;
	TRACE_ENTER();
	while (grp_ent != NULL) {
		tmp = grp_ent;
		grp_ent = grp_ent->next;
		free(tmp); 
	}
	TRACE_LEAVE();
}
static void free_dep_list(PLMS_ENTITY *plm_ent)
{
	TRACE_ENTER2("dep_list of %s being freed", plm_ent->dn_name_str);
	/* Free the dependency list */
	free_plms_group_entity(plm_ent->dependency_list);
	plm_ent->dependency_list = NULL;
	plm_ent->min_no_dep = 0;
	TRACE_LEAVE2("dep_list freed");
	return;
}
static void free_rev_dep_list(PLMS_ENTITY *plm_ent)
{
	TRACE_ENTER2("rev_dep_list of %s being freed", plm_ent->dn_name_str);
	/* Free the reverse dependency list */
	free_plms_group_entity(plm_ent->rev_dep_list);
	plm_ent->rev_dep_list = NULL;
	TRACE_LEAVE2("rev_dep_list freed");
	return;
}
static void remove_ent_from_rev_dep_list_of_others(PLMS_ENTITY *plm_ent)
{
	PLMS_GROUP_ENTITY *tmp;
	TRACE_ENTER2("object being removed from rev_dep_list of others: %s", 
			plm_ent->dn_name_str);
	/* Remove the plm_ent from rev_dep_list of each entity in dep_list */
	tmp = plm_ent->dependency_list;
	while (tmp != NULL) {
		/* Remove the plm_ent from the rev_dep_list of entity in tmp */
		rem_ent1_from_rev_dep_list_of_ent2(plm_ent, tmp->plm_entity);
		tmp = tmp->next;
	}
	TRACE_LEAVE();
	return;
}
static void plms_delete_domain_obj()
{
	/* Free the pointers in domain structure */
	TRACE_ENTER();
	free(plms_cb->domain_info.domain.safDomain);
	plms_cb->domain_info.domain.safDomain = NULL;
	plms_cb->domain_info.domain.saPlmHEDeactivationPolicy = 0;
	plms_cb->domain_info.leftmost_child = NULL;
	TRACE_LEAVE();
	return;
}
static void plms_delete_hpi_cfg_obj()
{
	TRACE_ENTER();
	/* Free the pointers in domain structure */
	free(plms_cb->hpi_cfg.safHpiCfg);
	plms_cb->hpi_cfg.hpi_support = 0;
	plms_cb->hpi_cfg.extr_pending_timeout = 0;
	plms_cb->hpi_cfg.insert_pending_timeout = 0;
	TRACE_LEAVE();
	return;
}
void plms_proc_active_quiesced_role_change()
{
	TRACE_ENTER();
	plms_unreg_with_imm_as_oi();
	TRACE_LEAVE();
}
void plms_proc_quiesced_standby_role_change()
{
	PLMS_ENTITY *plm_ent;
	PLMS_HE_BASE_INFO *he_base_info;
	PLMS_HE_TYPE_INFO *he_type_node;
	PLMS_EE_BASE_INFO *ee_base_info;
	PLMS_EE_TYPE_INFO *ee_type_node;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *ent_map_info;
	PLMS_CLIENT_INFO *client_info;
	PLMS_ENTITY_GROUP_INFO *ent_grp_info;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	SaUint32T i;
	TRACE_ENTER();	
	/* clean up the imm configuration in the data structures */
	/* remove all the nodes in patricia tree plms_cb->entity_info */
	while ((plm_ent = (PLMS_ENTITY *)
		ncs_patricia_tree_getnext(&plms_cb->entity_info, 
		(SaUint8T *) 0)) != NULL) {
		TRACE_2("Entity %s being freed", plm_ent->dn_name_str);
		/* Free all the pointers in the plm_ent */
		if (plm_ent->entity_type == PLMS_HE_ENTITY) {
			for (i=0; i <plm_ent->num_cfg_ent_paths; i++) {
				free(plm_ent->entity.he_entity.
					saPlmHEEntityPaths[i]);
			}
			if (plm_ent->entity.he_entity.saPlmHECurrEntityPath != 
				NULL) {
				free(plm_ent->entity.he_entity.
					saPlmHECurrEntityPath);
			}
			free(plm_ent->entity.he_entity.safHE);
		}
		else if (plm_ent->entity_type == PLMS_EE_ENTITY) {
			free(plm_ent->entity.ee_entity.safEE);
		}
		/* Free dependency list */
		free_dep_list(plm_ent);
		/* Free reverse dependency list */
		free_rev_dep_list(plm_ent);
		/* Free part_of_entity_groups */
		free_entity_group_info_list(plm_ent->part_of_entity_groups);
		/* Free plm_ent->cbk_grp_list */
		free_entity_group_info_list(plm_ent->cbk_grp_list);
		/* Free trk_info */
		if (plm_ent->trk_info != NULL) {
			free_track_info(plm_ent->trk_info);
		}
		ncs_patricia_tree_del(&plms_cb->entity_info, 
			&plm_ent->pat_node);
		free(plm_ent->dn_name_str);
		free(plm_ent);
	}
	TRACE_2("Freed all the HE, EE and Dependency objects");

	/* remove all the nodes in patricia tree plms_cb->base_he_info */
	while ((he_base_info = (PLMS_HE_BASE_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->base_he_info, 
		(SaUint8T *) 0)) != NULL) {
		free(he_base_info->he_base_type.safHEType);
		free(he_base_info->he_base_type.saPlmHetHpiEntityType);
		/* Remove all the he_type objects in this node */
		while (he_base_info->he_type_info_list != NULL) {
			he_type_node = he_base_info->he_type_info_list;
			he_base_info->he_type_info_list = he_base_info->
				he_type_info_list->next;
			plms_free_he_type_obj(he_type_node);
		}
		ncs_patricia_tree_del(&plms_cb->base_he_info,
			&he_base_info->pat_node);
		free(he_base_info);
	}
	TRACE_2("Freed all the HEBaseType objects");
	/* remove all the nodes in patricia tree plms_cb->base_ee_info */
	while ((ee_base_info = (PLMS_EE_BASE_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->base_ee_info, 
		(SaUint8T *) 0)) != NULL) {
		free(ee_base_info->ee_base_type.safEEType);
		/* Remove all the he_type objects in this node */
		while (ee_base_info->ee_type_info_list != NULL) {
			ee_type_node = ee_base_info->ee_type_info_list;
			ee_base_info->ee_type_info_list = ee_base_info->
				ee_type_info_list->next;
			plms_free_ee_type_obj(ee_type_node);
		}
		ncs_patricia_tree_del(&plms_cb->base_ee_info,
			&ee_base_info->pat_node);
		free(ee_base_info);
	}
	TRACE_2("Freed all the EEBaseType objects");
	/* Free domain info */
	free(plms_cb->domain_info.domain.safDomain);
	plms_cb->domain_info.domain.safDomain = NULL;
	TRACE_2("Freed the domain object");
	/* Free the entity_path_to_entity patricia tree */
	while ((ent_map_info = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->epath_to_entity_map_info, 
		(SaUint8T *) 0)) != NULL) {
		/* Do we need to free SaHpiEntityPath ? */
		ncs_patricia_tree_del(&plms_cb->epath_to_entity_map_info,
			&ent_map_info->pat_node);
		free(ent_map_info);
	}
	TRACE_2("Freed the entity path to entity nodes mapping tree");
	/* Free client_info patricia tree */
	while ((client_info = (PLMS_CLIENT_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->client_info,(SaUint8T *) 0)) != NULL) {
			PLMS_ENTITY_GROUP_INFO_LIST *tmp_ent_grp_ptr1 = NULL, *tmp_ent_grp_ptr2 = NULL;
			tmp_ent_grp_ptr1 = client_info->entity_group_list;
			while (tmp_ent_grp_ptr1)
			{
				tmp_ent_grp_ptr2 = tmp_ent_grp_ptr1->next;
				free(tmp_ent_grp_ptr1);
				tmp_ent_grp_ptr1 = tmp_ent_grp_ptr2;
			}

			client_info->entity_group_list = NULL;

			ncs_patricia_tree_del(&plms_cb->client_info, 
						&client_info->pat_node);
			free(client_info);
	}

	TRACE_2("Freed the client_info tree");
	/* Free entity group info patricia tree */
	while ((ent_grp_info = (PLMS_ENTITY_GROUP_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->entity_group_info,
		(SaUint8T *) 0)) != NULL) {
		free_entity_group_info(ent_grp_info);
		ncs_patricia_tree_del(&plms_cb->entity_group_info, 
			&ent_grp_info->pat_node);
		free(ent_grp_info);
	}
	TRACE_2("Freed the entity group info tree");
	/* Free epath_to_ent_map patricia tree */
	while ((epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)
		ncs_patricia_tree_getnext(&plms_cb->epath_to_entity_map_info,
		(SaUint8T *) 0)) != NULL) {
		free(epath_to_ent->entity_path);
		ncs_patricia_tree_del(&plms_cb->epath_to_entity_map_info,
			&epath_to_ent->pat_node);
		free(epath_to_ent);
	}
	TRACE_2("Freed the epath_to_ent_map patricia  tree");
	plms_cb->is_initialized = FALSE;
	plms_cb->async_update_cnt = 0;
	free_ckpt_track_step_info(plms_cb->prev_trk_step_rec);
	plms_cb->prev_ent_grp_hdl = 0; /* Shall we do this..? */
	plms_delete_domain_obj();
	plms_delete_hpi_cfg_obj();
	TRACE_LEAVE();
}
void plms_proc_quiesced_active_role_change()
{
	TRACE_ENTER();
	plms_reg_with_imm_as_oi();
	TRACE_LEAVE();
}
#if 0
void plms_proc_standby_active_role_change()
{
	TRACE_ENTER();	
	plms_imm_intf_initialize();
	TRACE_LEAVE();
}
#endif
static void free_entity_group_info_list(PLMS_ENTITY_GROUP_INFO_LIST 
					*grp_info_list)
{
	PLMS_ENTITY_GROUP_INFO_LIST *tmp_grp_info_list; 
	TRACE_ENTER();	
	while (grp_info_list != NULL) {
		tmp_grp_info_list = grp_info_list; 
		grp_info_list = grp_info_list->next; 
		free(tmp_grp_info_list);
	}
	TRACE_LEAVE();
}
static void free_entity_group_info(PLMS_ENTITY_GROUP_INFO *group_info)
{
	TRACE_ENTER();	
	free_plms_group_entity_list(group_info->plm_entity_list);
	TRACE_LEAVE();
}
static void free_track_info(PLMS_TRACK_INFO *track_info)
{
	TRACE_ENTER();	
	free_entity_group_info_list(track_info->group_info_list);
	free_plms_group_entity(track_info->aff_ent_list);
	TRACE_LEAVE();
}
static void free_ckpt_track_step_info(PLMS_CKPT_TRACK_STEP_INFO *step_info)
{
	TRACE_ENTER();	
	PLMS_CKPT_TRACK_STEP_INFO *tmp_step_info; 
	while (step_info != NULL) {
		tmp_step_info = step_info; 
		step_info = step_info->next; 
		free(tmp_step_info);
	}
	TRACE_LEAVE();
}
#if 0
void plms_check_n_add_ent_to_ent_groups(PLMS_ENTITY *plm_ent)
{
	PLMS_ENTITY_GROUP_INFO *ent_grp_info;
	PLMS_GROUP_ENTITY *ent;
	PLMS_ENTITY *tmp;
	TRACE_ENTER2("Entity: %s", plm_ent->dn_name_str);	
	ent_grp_info = (PLMS_ENTITY_GROUP_INFO *) ncs_patricia_tree_getnext(
		&plms_cb->entity_group_info, (SaUint8T *) 0);
	while (ent_grp_info != NULL) {
		if (ent_grp_info->options != SA_PLM_GROUP_SUBTREE) {
			if ((plm_ent->entity_type == PLMS_HE_ENTITY) && 
				(ent_grp_info->options != 
				SA_PLM_GROUP_SUBTREE_HES_ONLY)) {
				continue;
			}
			else if ((plm_ent->entity_type == PLMS_EE_ENTITY) && 
				(ent_grp_info->options != 
				SA_PLM_GROUP_SUBTREE_EES_ONLY)) {
				continue;
			}
		}
		ent = ent_grp_info->plm_entity_list;
		while (ent != NULL) {
			/* check if any ancestor of he is present as entity */
			tmp = plm_ent->parent;
			while (tmp != NULL) {
				if (tmp == ent->plm_entity) {
					/* ancestor entity matched, so add 
					   the target entity as well to this
					   group */
					TRACE_2("ancestor entity %s is added \
					with subtree option, so adding this\
					entity to the group", 
					ent->plm_entity->dn_name_str);   
					plms_ent_to_ent_list_add(plm_ent, 
						&ent_grp_info->plm_entity_list);
					break; /* from while(tmp != NULL) */
				}
				tmp = tmp->parent;
			}
			if (tmp != NULL) {
				/* No need to go to next entity of same group */
				break; /* break from while(ent != NULL) */
			}
			ent = ent->next;
		}
		ent_grp_info = (PLMS_ENTITY_GROUP_INFO *) 
		ncs_patricia_tree_getnext(&plms_cb->entity_group_info, 
		(SaUint8T *) &ent_grp_info->entity_grp_hdl);
	}
	TRACE_LEAVE();
}
#endif
static void plms_check_n_del_ent_from_ent_groups(PLMS_ENTITY *plm_ent)
{
	PLMS_ENTITY_GROUP_INFO_LIST *group_list;
	TRACE_ENTER();	
	group_list = plm_ent->part_of_entity_groups;
	while (group_list != NULL) {
		plms_ent_from_grp_ent_list_rem(plm_ent, &group_list->ent_grp_inf->plm_entity_list);
		group_list = group_list->next;
	}
	TRACE_LEAVE();
}
void plms_get_str_from_dn_name(SaNameT *obj_name, SaStringT str)
{
	strncpy(str, (SaInt8T *)obj_name->value, obj_name->length);
	str[obj_name->length] = 0;
}
static void plms_ent_from_grp_ent_list_rem(PLMS_ENTITY *plm_ent, 
				PLMS_GROUP_ENTITY_ROOT_LIST **ent_list)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *temp, *trav;
	TRACE_ENTER2("entity: %s", plm_ent->dn_name_str);
	if (strcmp((*ent_list)->plm_entity->dn_name_str, plm_ent->dn_name_str)
			== 0) {
		TRACE_2("Entity matched in the first node itself");
		temp = *ent_list;
		*ent_list = (*ent_list)->next;
		free(temp);
	}
	else {
		trav = *ent_list;
		while (trav->next != NULL) {
			temp = trav->next;
			if (strcmp(temp->plm_entity->dn_name_str, plm_ent->
				dn_name_str) == 0) {
				TRACE_2("Entity to be removed found");
				trav->next = temp->next;
				free(temp);
				break;
			}
			trav = trav->next;
		}
	}
	TRACE_LEAVE();
}
static void free_plms_group_entity_list(PLMS_GROUP_ENTITY_ROOT_LIST *grp_ent) 
{
	PLMS_GROUP_ENTITY_ROOT_LIST *tmp;
	TRACE_ENTER();
	while (grp_ent != NULL) {
		tmp = grp_ent;
		grp_ent = grp_ent->next;
		free(tmp); 
	}
	TRACE_LEAVE();
}


static void *plm_imm_reinit_thread(void *_cb)
{
	SaImmOiCallbacksT_2 imm_oi_cbks;
	SaAisErrorT error=SA_AIS_OK;

	TRACE_ENTER();

	imm_oi_cbks.saImmOiAdminOperationCallback = plms_imm_admin_op_cbk;
	imm_oi_cbks.saImmOiCcbAbortCallback = plms_imm_ccb_abort_cbk;
	imm_oi_cbks.saImmOiCcbApplyCallback = plms_imm_ccb_apply_cbk;
	imm_oi_cbks.saImmOiCcbCompletedCallback = plms_imm_ccb_completed_cbk;
	imm_oi_cbks.saImmOiCcbObjectCreateCallback= plms_imm_ccb_obj_create_cbk;
	imm_oi_cbks.saImmOiCcbObjectDeleteCallback= plms_imm_ccb_obj_delete_cbk;
	imm_oi_cbks.saImmOiCcbObjectModifyCallback= plms_imm_ccb_obj_modify_cbk;
	imm_oi_cbks.saImmOiRtAttrUpdateCallback = NULL;


	if ((error = immutil_saImmOiInitialize_2(&plms_cb->oi_hdl, &imm_oi_cbks, &imm_version)) != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize_2 failed %u", error);
		exit(EXIT_FAILURE);
	}

	if ((error = immutil_saImmOiSelectionObjectGet(plms_cb->oi_hdl, &plms_cb->imm_sel_obj)) != SA_AIS_OK) {
		LOG_ER("saImmOiSelectionObjectGet failed %u", error);
		exit(EXIT_FAILURE);
	}
	if (plms_cb->ha_state == SA_AMF_HA_ACTIVE) {
		/* Update IMM */
		if ((error = immutil_saImmOiImplementerSet(plms_cb->oi_hdl,impl_name )) != SA_AIS_OK) {

			LOG_ER("saImmOiImplementerSet failed %u", error);
			exit(EXIT_FAILURE);
		}

		plms_oi_class_impl_set("SaPlmDomain");
		plms_oi_class_impl_set("SaHpiConfig");
		plms_oi_class_impl_set("SaPlmHEBaseType");
		plms_oi_class_impl_set("SaPlmHEType");
		plms_oi_class_impl_set("SaPlmHE");
		plms_oi_class_impl_set("SaPlmEEBaseType");
		plms_oi_class_impl_set("SaPlmEEType");
		plms_oi_class_impl_set("SaPlmEE");
		plms_oi_class_impl_set("SaPlmDependency");
	}
	TRACE_LEAVE();
	return NULL;
}	


/**                     
 * Start a background thread to do IMM reinitialization.
 *      
 * @param cb
 */     
void plm_imm_reinit_bg(PLMS_CB *cb)
{               
        pthread_t thread; 
        pthread_attr_t attr;
        pthread_attr_init(&attr); 
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                                
        TRACE_ENTER();  
        if (pthread_create(&thread, &attr, plm_imm_reinit_thread, cb) != 0) {
                LOG_ER("pthread_create FAILED: %s", strerror(errno));
                exit(EXIT_FAILURE);
        }       
                
        pthread_attr_destroy(&attr);
                
        TRACE_LEAVE();
}               
