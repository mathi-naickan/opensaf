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

  MODULE NAME:       ncs_vda.c   

  DESCRIPTION:       LEAP MDS destination utility APIs to create and manage 
                     MDS destinations.

******************************************************************************
*/
#include <ncsgl_defs.h>
#include "ncssysf_def.h"
#include "ncs_sprr_papi.h"
#include "mds_papi.h"
#include "mds_adm.h"
#include "mds_core.h"
#include "mda_pvt_api.h"
#include "mda_mem.h"
#include "ncs_mda_papi.h"


#define m_NCSVDA_TRACE_ARG1(X)

/***************************************************************************\
                         PRIVATE DATA STRUCTURES
\***************************************************************************/

typedef struct vda_pvt_info {
	/* Used by VDA module to send MDS messages */
	MDS_HDL mds_adest_pwe1_hdl;

	/* Information on the VDS's MDS-address. Required when a VDA needs
	   to communicate with a VDS. */
	bool vds_primary_up;
	MDS_DEST vds_vdest;
	bool vds_sync_awaited;
	NCS_SEL_OBJ vds_sync_sel;

} VDA_PVT_INFO;

static VDA_PVT_INFO gl_vda_info;	/* For internal use within MDA only */
static bool vda_vdest_create = false;

/***************************************************************************\
                         PRIVATE FUNCTION PROTOTYPES
\***************************************************************************/
static uint32_t vda_create(NCS_LIB_REQ_INFO *req);
static uint32_t vda_destroy(NCS_LIB_REQ_INFO *req);
static uint32_t vda_instantiate(NCS_LIB_REQ_INFO *req);
static uint32_t vda_uninstantiate(NCS_LIB_REQ_INFO *req);

static uint32_t vda_destroy_vdest_locally(uint32_t vdest_handle);


/***************************************************************************\
                         PUBLIC VDA FUNCTIONS
\***************************************************************************/
uint32_t vda_lib_req(NCS_LIB_REQ_INFO *req)
{
	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:
		m_NCSVDA_TRACE_ARG1("VDA:LIB_CREATE\n");
		return vda_create(req);

	case NCS_LIB_REQ_INSTANTIATE:
		m_NCSVDA_TRACE_ARG1("VDA:LIB_INSTANTIATE\n");
		return vda_instantiate(req);

	case NCS_LIB_REQ_UNINSTANTIATE:
		m_NCSVDA_TRACE_ARG1("VDA:LIB_UNINSTANTIATE\n");
		return vda_uninstantiate(req);

	case NCS_LIB_REQ_DESTROY:
		m_NCSVDA_TRACE_ARG1("VDA:LIB_DESTROY\n");
		return vda_destroy(req);

	default:
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
}

uint32_t ncsvda_api(NCSVDA_INFO *vda_info)
{
	NCS_SPIR_REQ_INFO spir_req;
	NCSMDS_INFO svc_info;
	uint32_t policy;
	SaAmfCSIAttributeT attr[1];

	switch (vda_info->req) {
	case NCSVDA_VDEST_CREATE:

		/* Get the PWE-HDL */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;
		spir_req.i_environment_id = 1;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		policy = vda_info->info.vdest_create.i_policy;
		spir_req.info.lookup_create_inst.i_inst_attrs.number = 1;
		spir_req.info.lookup_create_inst.i_inst_attrs.attr = attr;
		spir_req.info.lookup_create_inst.i_inst_attrs.attr[0].attrName = (SaUint8T *)m_SPRR_VDEST_POLICY_ATTR_NAME;	/* Name-value name */
		/* typecasting to avoid warning in 64bit compilation */
		spir_req.info.lookup_create_inst.i_inst_attrs.attr[0].attrValue = (SaUint8T *)&policy;	/* Must be an uint32_t pointer */

		if (vda_info->info.vdest_create.i_create_type == NCSVDA_VDEST_CREATE_SPECIFIC) {
			if ((vda_info->info.vdest_create.info.specified.i_vdest == 0)
			    || (vda_info->info.vdest_create.info.specified.i_vdest > NCSMDS_MAX_VDEST)) {
				return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			/* End Fix */

			/* Check if the VDEST is in allowed range */
			if (m_MDS_GET_VDEST_ID_FROM_MDS_DEST(vda_info->info.vdest_create.info.specified.i_vdest)
			    > NCSVDA_UNNAMED_MAX) {
				return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}

			m_MDS_FIXED_VDEST_TO_INST_NAME(m_MDS_GET_VDEST_ID_FROM_MDS_DEST
						       (vda_info->info.vdest_create.info.specified.i_vdest),
						       &spir_req.i_instance_name);
		} else {
			return NCSCC_RC_FAILURE;

		}

		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		vda_info->info.vdest_create.o_mds_pwe1_hdl = (MDS_HDL)spir_req.info.lookup_create_inst.o_handle;

		/* Get the vdest and vdest-hdl. We can use the SPRR to get 
		   the vdest-hdl but not the VDEST. Hence, we instead use the
		   MDS_QUERY_PWE_INFO for "one-shot-two-birds" deal.

		   There is no other alternative here, other than a MDS_INSTALL
		   and MDS_UNINSTALL merely to get the VDEST-IDENTIFIER. */

		memset(&svc_info, 0, sizeof(svc_info));
		svc_info.i_mds_hdl = vda_info->info.vdest_create.o_mds_pwe1_hdl;
		svc_info.i_op = MDS_QUERY_PWE;
		svc_info.i_svc_id = NCSMDS_SVC_ID_VDA;	/* Doesn't matter */
		if (ncsmds_api(&svc_info) != NCSCC_RC_SUCCESS) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* VDEST to be created is specific */
		vda_info->info.vdest_create.o_mds_vdest_hdl =
		    (MDS_HDL)vda_info->info.vdest_create.info.specified.i_vdest;

		return vda_info->o_result = NCSCC_RC_SUCCESS;

	case NCSVDA_VDEST_CHG_ROLE:
		return vda_info->o_result = vda_chg_role_vdest(&vda_info->info.vdest_chg_role.i_vdest,
							       vda_info->info.vdest_chg_role.i_new_role);

	case NCSVDA_VDEST_LOOKUP:
		syslog(LOG_INFO,"NCSVDA_VDEST_LOOKUP Not supported");
		return vda_info->o_result = NCSCC_RC_SUCCESS;

	case NCSVDA_VDEST_DESTROY:
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.i_environment_id = 1;
		/* Release the handle to that PWE */
		if (m_MDS_GET_VDEST_ID_FROM_MDS_DEST(vda_info->info.vdest_destroy.i_vdest)
				> NCSVDA_UNNAMED_MAX) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		m_MDS_FIXED_VDEST_TO_INST_NAME(m_MDS_GET_VDEST_ID_FROM_MDS_DEST
				(vda_info->info.vdest_destroy.i_vdest),
				&spir_req.i_instance_name);
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.type = NCS_SPIR_REQ_REL_INST;
		spir_req.info.rel_inst = 0;	/* Dummy unused value */
		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		return vda_info->o_result = NCSCC_RC_SUCCESS;

	case NCSVDA_PWE_CREATE:

		/* First create pwe on give VDEST with LOOKUP_CREATE query */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;
		spir_req.i_environment_id = (PW_ENV_ID)vda_info->info.pwe_create.i_pwe_id;
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		m_MDS_FIXED_VDEST_TO_INST_NAME(m_MDS_GET_VDEST_ID_FROM_MDS_DEST
					       (vda_info->info.pwe_create.i_mds_vdest_hdl), &spir_req.i_instance_name);
		spir_req.info.lookup_create_inst.i_inst_attrs.number = 0;
		spir_req.info.lookup_create_inst.i_inst_attrs.attr = attr;

		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		vda_info->info.pwe_create.o_mds_pwe_hdl = spir_req.info.lookup_create_inst.o_handle;

		return vda_info->o_result = NCSCC_RC_SUCCESS;

	case NCSVDA_PWE_DESTROY:
		/* NOTE: The PWE-ID from PWE-handle is done a little
		   bit differently from that in NCSVDA_PWE_DESTROY
		 */
		memset(&spir_req, 0, sizeof(spir_req));
		spir_req.i_environment_id = m_MDS_GET_PWE_ID_FROM_PWE_HDL(vda_info->info.pwe_destroy.i_mds_pwe_hdl);
		/* Release the handle to that PWE */

		m_MDS_FIXED_VDEST_TO_INST_NAME(m_MDS_GET_VDEST_ID_FROM_PWE_HDL
				(vda_info->info.pwe_destroy.i_mds_pwe_hdl), &spir_req.i_instance_name);
		spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
		spir_req.type = NCS_SPIR_REQ_REL_INST;
		spir_req.info.rel_inst = 0;	/* Dummy unused value */
		if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
			return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
		return vda_info->o_result = NCSCC_RC_SUCCESS;

	default:
		return vda_info->o_result = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
}


/***************************************************************************\
                        PRIVATE ADA FUNCTIONS
\***************************************************************************/
static uint32_t vda_create(NCS_LIB_REQ_INFO *req)
{
	NCS_SPLR_REQ_INFO splr_req;
	NCS_SPIR_REQ_INFO spir_req;
	memset(&splr_req, 0, sizeof(splr_req));

	/* STEP : Register VDA as a service provider */
	splr_req.i_sp_abstract_name = m_VDA_SP_ABST_NAME;
	splr_req.type = NCS_SPLR_REQ_REG;
	splr_req.info.reg.instantiation_api = vda_lib_req;
	splr_req.info.reg.instantiation_flags = NCS_SPLR_INSTANTIATION_PER_INST_NAME;
	splr_req.info.reg.user_se_api = (NCSCONTEXT)ncsvda_api;

	if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	/* STEP : Get the MDS-handle to the ADEST */
	memset(&spir_req, 0, sizeof(spir_req));
	spir_req.type = NCS_SPIR_REQ_LOOKUP_CREATE_INST;
	spir_req.i_environment_id = 1;
	spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;
	spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
	spir_req.info.lookup_create_inst.i_inst_attrs.number = 0;

	if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	gl_vda_info.mds_adest_pwe1_hdl = (MDS_HDL)spir_req.info.lookup_create_inst.o_handle;

	return NCSCC_RC_SUCCESS;
}

static uint32_t vda_destroy(NCS_LIB_REQ_INFO *req)
{
	NCS_SPLR_REQ_INFO splr_req;
	NCS_SPIR_REQ_INFO spir_req;

	NCSMDS_INFO svc_info;

	if (vda_vdest_create == true) {
		/* STEP : Uninstall from ADEST with MDS as service NCSMDS_SVC_ID_VDA. */
		memset(&svc_info, 0, sizeof(svc_info));
		svc_info.i_mds_hdl = gl_vda_info.mds_adest_pwe1_hdl;
		svc_info.i_svc_id = NCSMDS_SVC_ID_VDA;
		svc_info.i_op = MDS_UNINSTALL;
		svc_info.info.svc_uninstall.i_msg_free_cb = NULL;	/* NULL for callback-model services */
		/* Invoke MDS api */
		if (ncsmds_api(&svc_info) != NCSCC_RC_SUCCESS)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	}
	/* STEP : Release the use of ADEST MDS handle */
	memset(&spir_req, 0, sizeof(spir_req));
	spir_req.type = NCS_SPIR_REQ_REL_INST;
	spir_req.i_environment_id = 1;
	spir_req.i_instance_name = m_MDS_SPIR_ADEST_NAME;
	spir_req.i_sp_abstract_name = m_MDS_SP_ABST_NAME;
	spir_req.info.rel_inst = 0;	/* dummy value */
	if (ncs_spir_api(&spir_req) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	gl_vda_info.mds_adest_pwe1_hdl = 0;

	/* STEP : Deregister VDA as a service provider */
	memset(&splr_req, 0, sizeof(splr_req));
	splr_req.i_sp_abstract_name = m_VDA_SP_ABST_NAME;
	splr_req.type = NCS_SPLR_REQ_DEREG;
	splr_req.info.dereg.dummy = 0;
	if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	vda_vdest_create = false;

	return NCSCC_RC_SUCCESS;
}

static uint32_t vda_instantiate(NCS_LIB_REQ_INFO *req)
{
	uint32_t vdest_id;
	bool is_named_vdest;
	MDS_DEST new_vdest;
	MDS_HDL new_vdest_hdl;
	uint32_t policy;
	uint32_t attr_num;

	memset(&new_vdest, 0, sizeof(new_vdest));

	/* STEP : Check for "reserved" instance names */
	switch (mda_get_inst_name_type(&req->info.inst.i_inst_name)) {
	case MDA_INST_NAME_TYPE_UNNAMED_VDEST:
		is_named_vdest = false;
		break;

	case MDA_INST_NAME_TYPE_NAMED_VDEST:
		is_named_vdest = true;
		break;

	default:
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Set VDEST Policy to default */
	policy = NCS_VDEST_TYPE_DEFAULT;
	/* Override the default, if specific attribute for policy is provided */
	for (attr_num = 0; attr_num < req->info.inst.i_inst_attrs.number; attr_num++) {
		if (0 ==
		    memcmp(req->info.inst.i_inst_attrs.attr[attr_num].attrName, m_SPRR_VDEST_POLICY_ATTR_NAME,
			   strlen(m_SPRR_VDEST_POLICY_ATTR_NAME))) {
			policy = *(uint32_t *)(req->info.inst.i_inst_attrs.attr[attr_num].attrValue);
		}
	}

	if (is_named_vdest == false) {
		/* A fixed VDEST has been specified, Find it */
		mds_inst_name_to_fixed_vdest(&req->info.inst.i_inst_name, &vdest_id);

		/* Check if the VDEST is in allowed range */
		if (vdest_id > NCSVDA_UNNAMED_MAX) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		/* Specified VDEST has to be created. Need not go to a
		 ** VDS for this purpose. 
		 */
		m_NCSVDA_SET_VDEST(new_vdest, vdest_id);
	} else {
		return NCSCC_RC_FAILURE;
	}

	/* STEP : Create VDEST locally. */
	if (vda_create_vdest_locally(policy, &new_vdest, &new_vdest_hdl)
	    != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	req->info.inst.o_inst_hdl = (MDS_HDL)new_vdest_hdl;
	req->info.inst.o_arg = NULL;

	return NCSCC_RC_SUCCESS;
}

static uint32_t vda_uninstantiate(NCS_LIB_REQ_INFO *req)
{
	MDS_DEST gone_vdest;
	bool is_named_vdest;

	memset(&gone_vdest, 0, sizeof(gone_vdest));

	/* STEP : Check for "reserved" instance names */
	switch (mda_get_inst_name_type(&req->info.uninst.i_inst_name)) {
	case MDA_INST_NAME_TYPE_UNNAMED_VDEST:
		is_named_vdest = false;
		break;

	case MDA_INST_NAME_TYPE_NAMED_VDEST:
		is_named_vdest = true;
		break;

	default:
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	if (is_named_vdest) {
		return NCSCC_RC_FAILURE;
	}

	/* STEP : Create VDEST locally. */
	if (vda_destroy_vdest_locally(req->info.uninst.i_inst_hdl) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	return NCSCC_RC_FAILURE;
}

/*-------------------------------------------------------------------------*\
     FUNCTION NAME : vda_create_vdest_locally (public)
\*-------------------------------------------------------------------------*/
uint32_t vda_create_vdest_locally(uint32_t i_pol, MDS_DEST *i_vdest, MDS_HDL *o_mds_vdest_hdl)
{
	NCSMDS_ADMOP_INFO admop_info;

	memset(&admop_info, 0, sizeof(admop_info));

	/* STEP : We first check if the VDEST has already been created locally */
	admop_info.i_op = MDS_ADMOP_VDEST_QUERY;
	admop_info.info.vdest_query.i_local_vdest = *i_vdest;
	if (ncsmds_adm_api(&admop_info) == NCSCC_RC_SUCCESS) {
		/* VDEST already created */
		MDA_TRACE1_ARG1("VDEST already created locally.\n");
		*o_mds_vdest_hdl = admop_info.info.vdest_query.o_local_vdest_hdl;
		return NCSCC_RC_SUCCESS;
	}

	/* STEP : We need to created the VDEST locally, because it is not already 
	   created */
	admop_info.i_op = MDS_ADMOP_VDEST_CREATE;
	admop_info.info.vdest_create.i_vdest = *i_vdest;
	admop_info.info.vdest_create.i_policy = i_pol;
	/* Invoke MDS api */
	if (ncsmds_adm_api(&admop_info) != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	*o_mds_vdest_hdl = admop_info.info.vdest_create.o_mds_vdest_hdl;
	return NCSCC_RC_SUCCESS;
}

/*-------------------------------------------------------------------------*\
     FUNCTION NAME : vda_chg_role_vdest  (public)
\*-------------------------------------------------------------------------*/
uint32_t vda_chg_role_vdest(MDS_DEST *i_vdest, V_DEST_RL i_new_role)
{
	NCSMDS_ADMOP_INFO admop_info;

	memset(&admop_info, 0, sizeof(admop_info));

	admop_info.i_op = MDS_ADMOP_VDEST_CONFIG;
	admop_info.info.vdest_config.i_new_role = i_new_role;
	admop_info.info.vdest_config.i_vdest = *i_vdest;

	/* Invoke MDS api */
	return ncsmds_adm_api(&admop_info);
}

/*-------------------------------------------------------------------------*\
     FUNCTION NAME : vda_destroy_vdest_locally (private)
\*-------------------------------------------------------------------------*/
static uint32_t vda_destroy_vdest_locally(uint32_t vdest_handle)
{
	NCSMDS_ADMOP_INFO admop_info;

	memset(&admop_info, 0, sizeof(admop_info));

	admop_info.i_op = MDS_ADMOP_VDEST_DESTROY;
	admop_info.info.vdest_destroy.i_vdest_hdl = (MDS_HDL)vdest_handle;
	/* Invoke MDS api */
	return ncsmds_adm_api(&admop_info);
}

