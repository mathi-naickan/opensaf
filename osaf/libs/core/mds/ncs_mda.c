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

  MODULE NAME:       ncs_mda.c   

  DESCRIPTION:       LEAP MDS destination utility APIs to create and manage 
                     MDS destinations.

******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncssysf_def.h"
#include "ncs_sprr_papi.h"
#include "mds_papi.h"
#include "mds_adm.h"
#include "mda_dl_api.h"
#include "mda_pvt_api.h"
#include "mda_mem.h"
#include "ncs_mda_pvt.h"
#include "ncs_mda_papi.h"
#include "mds_papi.h"

/***************************************************************************\
                         PRIVATE MDA DATA
\***************************************************************************/

/***************************************************************************\
                         PRIVATE ADA/VDA FUNCTION PROTOTYPES
\***************************************************************************/

/***************************************************************************\
                         GLOBAL MDA DATA
\***************************************************************************/

const SaNameT glmds_adest_inst_name = { 14, "NCS_ADEST_INST" };
const SaNameT glmds_vdest_inst_name_pref = { 20, "NCS_FIXED_VDEST_INST" };

#define VDEST_ID_STR_LEN 10	/* = length of the string "4294967295" */
#define VDEST_FMT_STR  "%010d"	/* Format string for creating VDEST-name */

#define m_NCSMDA_TRACE_ARG1(X)
/***************************************************************************\
                         PUBLIC ADA/VDA FUNCTIONS
\***************************************************************************/
uint32_t mda_lib_req(NCS_LIB_REQ_INFO *req)
{
	NCS_SPLR_REQ_INFO splr_req;
	NCS_SPIR_REQ_INFO spir_req;
	NCSMDS_ADMOP_INFO admop_info;

	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:
		m_NCSMDA_TRACE_ARG1("MDA:LIB_CREATE\n");
		/* STEP : Register MDA as a service-provider */
		memset(&splr_req, 0, sizeof(splr_req));
		splr_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		splr_req.type = NCS_SPLR_REQ_REG;
		splr_req.info.reg.instantiation_api = mda_lib_req;
		splr_req.info.reg.instantiation_flags =
		    NCS_SPLR_INSTANTIATION_PER_INST_NAME | NCS_SPLR_INSTANTIATION_PER_ENV_ID;
		splr_req.info.reg.user_se_api = (NCSCONTEXT)ncsmds_api;

		if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}

		/* STEP : Register ADA as a service-provider */
		if (ada_lib_req(req) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;

		/* STEP : Pre-create the ADEST entry. FIXME: This pre-creation
		   can be avoided if 
		   (1) The NCSADA_GET_HDLS can be matched by a 
		   NCSADA_REL_HDLS. 

		   (2) LOOKUP_CREATE_INST is done in place of LOOKUP_INST
		   in NCSADA_GET_HDLS, etc. 
		   Till that is done, the ADEST SPIR entry is pre-created
		 */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;
		spir_req.i_environment_id = 1;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.info.lookup_create_inst.i_inst_attrs.number = 0;
		spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;
		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* STEP : Register VDA as a service-provider */
		if (vda_lib_req(req) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;

		return NCSCC_RC_SUCCESS;

	case NCS_LIB_REQ_DESTROY:
		m_NCSMDA_TRACE_ARG1("MDA:LIB_DESTROY\n");
		vda_lib_req(req);

		/* Destroy pre-created ADEST-PWE1 - stuff */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.type = NCS_SPIR_REQ_REL_INST;
		spir_req.i_environment_id = 1;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.info.lookup_create_inst.i_inst_attrs.number = 0;
		spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;

		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		ada_lib_req(req);

		/* STEP : Deregister MDA as a service-provider */
		memset(&splr_req, 0, sizeof(splr_req));
		splr_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		splr_req.type = NCS_SPLR_REQ_DEREG;
		splr_req.info.dereg.dummy = 0;

		if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}
		m_NCSMDA_TRACE_ARG1("MDA:LIB_DESTROY:DONE\n");
		return NCSCC_RC_SUCCESS;

	case NCS_LIB_REQ_INSTANTIATE:
		m_NCSMDA_TRACE_ARG1("MDA:LIB_INSTANTIATE\n");

		/* Get a handle to the underlying the DESTINATION. We shall
		   go to our service-providers namely ADA/VDA to get it */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.i_environment_id = 0;	/* ADA/VDA libs don't create PWEs */
		spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;

		switch (mda_get_inst_name_type(&req->info.inst.i_inst_name)) {
		case MDA_INST_NAME_TYPE_ADEST:
			spir_req.i_sp_abstract_name = m_ADA_SP_ABST_NAME;
			if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			break;

		case MDA_INST_NAME_TYPE_UNNAMED_VDEST:
		case MDA_INST_NAME_TYPE_NAMED_VDEST:
			spir_req.i_environment_id = req->info.inst.i_env_id;
			spir_req.i_instance_name = req->info.inst.i_inst_name;
			spir_req.i_sp_abstract_name = m_VDA_SP_ABST_NAME;
			spir_req.info.lookup_create_inst.i_inst_attrs = req->info.inst.i_inst_attrs;
			if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			break;

		default:
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* STEP : Create PWE on the ADEST/VDEST */
		memset(&admop_info, 0, sizeof(admop_info));
		admop_info.i_op = MDS_ADMOP_PWE_CREATE;
		admop_info.info.pwe_create.i_mds_dest_hdl = (MDS_HDL)spir_req.info.lookup_create_inst.o_handle;
		admop_info.info.pwe_create.i_pwe_id = (PW_ENV_ID)req->info.inst.i_env_id;
		/* Invoke MDS api */
		if (ncsmds_adm_api(&admop_info) != NCSCC_RC_SUCCESS)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		/* Return the handle to the PWE */
		req->info.inst.o_inst_hdl = (uint32_t)admop_info.info.pwe_create.o_mds_pwe_hdl;
		req->info.inst.o_arg = NULL;

		m_NCSMDA_TRACE_ARG1("MDA:LIB_INSTANTIATE:DONE\n");
		return NCSCC_RC_SUCCESS;

	case NCS_LIB_REQ_UNINSTANTIATE:
		m_NCSMDA_TRACE_ARG1("MDA:LIB_UNINSTANTIATE\n");

		/* STEP : Destroy PWE on the ADEST/VDEST */
		memset(&admop_info, 0, sizeof(admop_info));
		admop_info.i_op = MDS_ADMOP_PWE_DESTROY;
		admop_info.info.pwe_destroy.i_mds_pwe_hdl = (MDS_HDL)req->info.uninst.i_inst_hdl;
		/* Invoke MDS api */
		if (ncsmds_adm_api(&admop_info) != NCSCC_RC_SUCCESS)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		/* STEP : Now release our lookup on the destination instance */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.i_environment_id = 0;	/* ADA/VDA libs don't create PWEs */
		spir_req.type = NCS_SPIR_REQ_REL_INST;

		switch (mda_get_inst_name_type(&req->info.inst.i_inst_name)) {
		case MDA_INST_NAME_TYPE_ADEST:
			spir_req.i_sp_abstract_name = m_ADA_SP_ABST_NAME;
			if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
				m_LEAP_DBG_SINK_VOID;
			}
			break;

		case MDA_INST_NAME_TYPE_UNNAMED_VDEST:
		case MDA_INST_NAME_TYPE_NAMED_VDEST:
			spir_req.i_instance_name = req->info.inst.i_inst_name;
			spir_req.i_sp_abstract_name = m_VDA_SP_ABST_NAME;
			if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
				m_LEAP_DBG_SINK_VOID;
			}
			break;
		default:
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		m_NCSMDA_TRACE_ARG1("MDA:LIB_UNINSTANTIATE:DONE\n");
		return NCSCC_RC_SUCCESS;

	default:
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
}

uint32_t ncsada_api(NCSADA_INFO *ada_info)
{
	NCS_SPIR_REQ_INFO spir_req;
	NCSMDS_INFO svc_info;
	NCSMDS_INFO mds_info;
	/* STEP : Now release our lookup on the destination instance */

	switch (ada_info->req) {
	case NCSADA_GET_HDLS:

		/* FIXME:The ADEST-PWE1 entry is pre-created during MDA-LIB creation. 
		   This is due to the fact that that the NCSADA_GET_HDLS does not 
		   have corresponding NCSADA_REL_HDLS. So if the NCSADA_GET_HDLS
		   is deprecated or NCSADA_REL_HDLS is introduced then the pre-creation
		   can be avoided. 

		   So here we simply need to LOOKUP the handle (instead of LOOKUP_CREATE)
		 */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.type = NCS_SPIR_REQ_LOOKUP_INST;
		spir_req.i_environment_id = 1;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;

		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		ada_info->info.adest_get_hdls.o_mds_pwe1_hdl = (MDS_HDL)spir_req.info.lookup_inst.o_handle;

		/* Get the adest and adest-hdl. We can use the SPRR to get 
		   the adest-hdl but not the ADEST. Hence, we instead use the
		   MDS_QUERY_PWE_INFO for "one-shot-two-birds" deal.

		   The alternative is to pick the ADEST from "gl_ada_info" a
		   and the handle using an SPRR query to the ADA. That complication
		   needs to be explored only if required. */

		memset(&svc_info, 0, sizeof(svc_info));
		svc_info.i_mds_hdl = ada_info->info.adest_get_hdls.o_mds_pwe1_hdl;
		svc_info.i_op = MDS_QUERY_PWE;
		svc_info.i_svc_id = NCSMDS_SVC_ID_VDA;	/* Doesn't matter */
		if (ncsmds_api(&svc_info) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		ada_info->info.adest_get_hdls.o_adest = svc_info.info.query_pwe.info.abs_info.o_adest;
		return NCSCC_RC_SUCCESS;

	case NCSADA_PWE_CREATE:
		/* Get the PWE-HDL */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.i_environment_id = ada_info->info.pwe_create.i_pwe_id;
		spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;
		spir_req.info.lookup_create_inst.i_inst_attrs.number = 0;

		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		ada_info->info.pwe_create.o_mds_pwe_hdl = (MDS_HDL)spir_req.info.lookup_create_inst.o_handle;

		return NCSCC_RC_SUCCESS;

	case NCSADA_PWE_DESTROY:
		/*  We need to fetch PWE-ID first */
		/* Get pweid from pwe_hdl */
		/* NOTE: The PWE-ID from PWE-handle is done a little
		   bit differently from that in NCSVDA_PWE_DESTROY
		 */
		memset(&mds_info, 0, sizeof(mds_info));
		mds_info.i_op = MDS_QUERY_PWE;
		mds_info.i_mds_hdl = ada_info->info.pwe_destroy.i_mds_pwe_hdl;
		if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}

		/* Release the handle to that PWE */
		spir_req.i_environment_id = mds_info.info.query_pwe.o_pwe_id;
		spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.type = NCS_SPIR_REQ_REL_INST;
		spir_req.info.rel_inst = 0;	/* Dummy unused value */
		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		return NCSCC_RC_SUCCESS;

	default:
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

MDA_INST_NAME_TYPE mda_get_inst_name_type(SaNameT *name)
{
	if (name->length == 0)
		return MDA_INST_NAME_TYPE_NULL;

	/* Check if it qualifies for an ADEST */
	if (m_CMP_HORDER_SANAMET(*name, m_MDS_SPIR_ADEST_NAME) == 0) {
		return MDA_INST_NAME_TYPE_ADEST;
	}

	if ((name->length == (glmds_vdest_inst_name_pref.length + VDEST_ID_STR_LEN)) &&
	    (memcmp(name->value, glmds_vdest_inst_name_pref.value, name->length - VDEST_ID_STR_LEN) == 0)) {
		return MDA_INST_NAME_TYPE_UNNAMED_VDEST;
	}
	return MDA_INST_NAME_TYPE_NAMED_VDEST;
}

void mds_fixed_vdest_to_inst_name(uint32_t i_vdest_id, SaNameT *o_name)
{
	memset(o_name, 0, sizeof(*o_name));
	o_name->length = (unsigned short)(glmds_vdest_inst_name_pref.length + VDEST_ID_STR_LEN);
	memcpy(o_name->value, glmds_vdest_inst_name_pref.value, o_name->length - VDEST_ID_STR_LEN);

	sprintf((char *)(o_name->value + glmds_vdest_inst_name_pref.length), VDEST_FMT_STR, i_vdest_id);
}

uint32_t mds_inst_name_to_fixed_vdest(SaNameT *i_name, uint32_t *o_vdest_id)
{
	char *vdest_read_ptr;
	if (mda_get_inst_name_type(i_name) != MDA_INST_NAME_TYPE_UNNAMED_VDEST)
		return NCSCC_RC_FAILURE;

	vdest_read_ptr = (char *)(i_name->value + (i_name->length - VDEST_ID_STR_LEN));
	if (sscanf(vdest_read_ptr, "%d", o_vdest_id) != 1)
		return NCSCC_RC_FAILURE;

	return NCSCC_RC_SUCCESS;
}
