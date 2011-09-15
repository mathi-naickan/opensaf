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

  DESCRIPTION:  MCM APIs

******************************************************************************
*/

#include "mds_core.h"
#include "mds_log.h"

/*********************************************************

  Function NAME: mds_validate_pwe_hdl

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_validate_pwe_hdl(MDS_PWE_HDL pwe_hdl)
{
	uns32 status = NCSCC_RC_SUCCESS;
	m_MDS_LOG_DBG("MCM_API : Entering : mds_validate_pwe_hdl");

	status = mds_pwe_tbl_query(m_MDS_GET_VDEST_HDL_FROM_VDEST_ID(m_MDS_GET_VDEST_ID_FROM_PWE_HDL(pwe_hdl)),
				   m_MDS_GET_PWE_ID_FROM_PWE_HDL(pwe_hdl));
	m_MDS_LOG_DBG("MCM_API : Leaving : S : status : mds_validate_pwe_hdl");
	return status;
}

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*              ncsmds_adm_api                  */

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*********************************************************

  Function NAME: mds_mcm_vdest_create

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_vdest_create(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_SUBTN_REF_VAL subtn_ref_ptr;
	MDS_VDEST_ID vdest_id;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_vdest_create");
/* STEP 1: Create VDEST-Table Entry  */

	vdest_id = (MDS_VDEST_ID)info->info.vdest_create.i_vdest;

	if ((vdest_id > NCSMDS_MAX_VDEST) || (vdest_id == 0)) {
		m_MDS_LOG_ERR("MCM_API : Vdest_create : FAILED : VDEST id = %d not in prescribed range ", vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_create");
		return NCSCC_RC_FAILURE;
	}

	/* Check whether VDEST already exist */
	status = mds_vdest_tbl_query(vdest_id);
	if (status == NCSCC_RC_SUCCESS) {
		/* VDEST already exist */
		m_MDS_LOG_ERR("MCM_API : vdest_create : VDEST id = %d Already exist", vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_create");
		return NCSCC_RC_FAILURE;
	} else {
		/* Add vdest in to table */

		mds_vdest_tbl_add(vdest_id, info->info.vdest_create.i_policy,
				  (MDS_VDEST_HDL *)&info->info.vdest_create.o_mds_vdest_hdl);
	}

/* STEP 2: IF Policy is MxN *
             call mdtm_vdest_subscribe (uns32 i_vdest_id) */

	if (info->info.vdest_create.i_policy == NCS_VDEST_TYPE_MxN) {
		status = mds_mdtm_vdest_subscribe(vdest_id, &subtn_ref_ptr);
		if (status != NCSCC_RC_SUCCESS) {
			/* VDEST Subscription Failed */

			/* Remove VDEST info from MCM Database */
			mds_vdest_tbl_del(vdest_id);

			m_MDS_LOG_ERR("MCM_API : vdest_create : VDEST id = %d can't subscribe : MDTM Returned Failure ",
				      vdest_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_create");
			return NCSCC_RC_FAILURE;
		}

		mds_vdest_tbl_update_ref_val(vdest_id, subtn_ref_ptr);
	}

	/* Fill output handle parameter */
	info->info.vdest_create.o_mds_vdest_hdl = (MDS_HDL)m_MDS_GET_VDEST_HDL_FROM_VDEST_ID(vdest_id);

	m_MDS_LOG_NOTIFY("MCM_API : vdest_create : VDEST id = %d Created Successfully", vdest_id);

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_create");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_vdest_destroy

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_vdest_destroy(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_VDEST_ID local_vdest_id;
	MDS_PWE_HDL pwe_hdl;
	V_DEST_RL local_vdest_role;
	NCS_VDEST_TYPE local_vdest_policy;
	MDS_SUBTN_REF_VAL subtn_ref_val;
	NCSMDS_ADMOP_INFO pwe_destroy_info;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_vdest_destroy");

	/* Check whether VDEST already exist */

	/* Get Vdest_id */
	local_vdest_id = m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->info.vdest_destroy.i_vdest_hdl);

	status = mds_vdest_tbl_query(local_vdest_id);
	if (status == NCSCC_RC_FAILURE) {
		/* VDEST Doesn't exist */
		m_MDS_LOG_ERR("MCM_API : vdest_destroy : VDEST id = %d Doesn't exist", local_vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_destroy");
		return NCSCC_RC_FAILURE;
	}

	/* Get Role, policy and subtn ref value for that vdest */
	mds_vdest_tbl_get_role(local_vdest_id, &local_vdest_role);
	mds_vdest_tbl_get_policy(local_vdest_id, &local_vdest_policy);
	mds_vdest_tbl_get_subtn_ref_val(local_vdest_id, &subtn_ref_val);

/* STEP 1: For all PWEs on this DEST
            Call mds_mcm_pwe_destroy (pwe_hdl) */

	pwe_destroy_info.i_op = MDS_ADMOP_PWE_DESTROY;

	while (mds_vdest_tbl_get_first(local_vdest_id, &pwe_hdl) != NCSCC_RC_FAILURE) {
		pwe_destroy_info.info.pwe_destroy.i_mds_pwe_hdl = (MDS_HDL)pwe_hdl;
		mds_mcm_pwe_destroy(&pwe_destroy_info);
	}

/* STEP 2: IF VDEST is Active
            Call mdtm_vdest_uninstall (vdest_id)
            To inform other VDEST about its going to Standby state. */

/* STEP 3: Call mdtm_vdest_unsubscribe(vdest_id)
            To Subscribe to active VDEST instance */

	if (local_vdest_policy == NCS_VDEST_TYPE_MxN) {
		/* Unsubscribe for this vdest */
		status = mds_mdtm_vdest_unsubscribe(local_vdest_id, subtn_ref_val);
		if (status != NCSCC_RC_SUCCESS) {
			/* VDEST Unsubscription Failed */
			m_MDS_LOG_ERR
			    ("MCM_API : vdest_create : VDEST id = %d can't Unsubscribe : MDTM Returned Failure",
			     local_vdest_id);
		}

		/* If current role is active uninstall it */
		if (local_vdest_role == V_DEST_RL_ACTIVE) {
			status = mds_mdtm_vdest_uninstall(local_vdest_id);
			if (status != NCSCC_RC_SUCCESS) {
				/* VDEST Uninstalled Failed */
				m_MDS_LOG_ERR
				    ("MCM_API : vdest_create : VDEST id = %d can't UnInstall : MDTM Returned Failure",
				     local_vdest_id);
			}
		}

	}

/* STEP 4: Delete DEST from VDEST-table entry */

	status = mds_vdest_tbl_del(m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->info.vdest_destroy.i_vdest_hdl));

	m_MDS_LOG_NOTIFY("MCM_API : vdest_destroy : VDEST id = %d Destroyed Successfully", local_vdest_id);

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_destroy");
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 *
 * Function Name: mds_mcm_vdest_query
 *
 * Purpose:       This function queries information about a local vdest.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

uns32 mds_mcm_vdest_query(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_VDEST_ID local_vdest_id;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_vdest_query");

	local_vdest_id = (MDS_VDEST_ID)info->info.vdest_query.i_local_vdest;

	/* Check whether VDEST already exist */
	status = mds_vdest_tbl_query(local_vdest_id);
	if (status == NCSCC_RC_SUCCESS) {
		/* VDEST exists */

		/* Dummy statements as vdest_id is same as vdest_hdl */

		info->info.vdest_query.o_local_vdest_hdl = (MDS_HDL)m_MDS_GET_VDEST_HDL_FROM_VDEST_ID(local_vdest_id);

		info->info.vdest_query.o_local_vdest_anc = (V_DEST_QA)m_MDS_GET_ADEST;

		m_MDS_LOG_NOTIFY("MCM_API : vdest_query for VDEST id = %d Successful", local_vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_query");
		return NCSCC_RC_SUCCESS;
	} else {
		/* VDEST Doesn't exist */
		m_MDS_LOG_NOTIFY("MCM_API : vdest_query for VDEST id = %d FAILED", local_vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_query");
		return NCSCC_RC_FAILURE;
	}
}

/*********************************************************

  Function NAME: mds_mcm_vdest_chg_role

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_vdest_chg_role(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	V_DEST_RL current_role;
	MDS_VDEST_ID vdest_id;
	NCS_VDEST_TYPE local_vdest_policy;
	MDS_VDEST_ID local_vdest_id;
	MDS_SVC_INFO *svc_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_vdest_chg_role");

/* 
From Dest_Hdl get the Current Role of VDEST

if (Current Role = Active && New Role = Quiesced)
    {
        STEP 1: Call mdtm_vdest_uninstall(vdest_id)
        (To unbind to intimate other VDEST with same id about its Role change.)

        STEP 2: Update New Role = Quiesced in Local VDEST table.

        STEP 3: For each service on this VDEST (obtained by a join on VDEST TABLE, PWE TABLE,SVC-TABLE)
                Call mdtm_svc_install(svc_hdl, NEW ROLE=Quiesced)
                Call mdtm_svc_uninstall(svc_hdl, Current ROLE)

        STEP 4: Start Quiesced timer
    }

if  (Current Role = Quiesced && New Role = Standby)
    {
        STEP 1: Update New Role = Standby in Local VDEST table.
    }
if (New Role = Active)
    {
        STEP 1: Call mdtm_vdest_install(vdest_id)
                 (To unbind to intimate other VDEST with same id about its Role change)
    }
if (new Role = Active / Standby)
    {
        STEP 1: For each service on this VDEST (obtained by a join on VDEST TABLE, PWE TABLE,SVC-TABLE)
                Call mdtm_svc_uninstall(svc_hdl, Current ROLE)
                Call mdtm_svc_install(svc_hdl, NEW ROLE=Active/Standby)
    }
*/
	local_vdest_id = (MDS_VDEST_ID)info->info.vdest_query.i_local_vdest;

	status = mds_vdest_tbl_query(local_vdest_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Vdest Doesn't exist */
		m_MDS_LOG_ERR("MCM_API : vdest_chg_role : VDEST id = %d Doesn't exist", local_vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_chg_role");
		return NCSCC_RC_FAILURE;
	}

	/* Get current Role */
	mds_vdest_tbl_get_role(local_vdest_id, &current_role);

	/* Get Policy of Vdest */
	mds_vdest_tbl_get_policy(local_vdest_id, &local_vdest_policy);

	if (current_role == V_DEST_RL_ACTIVE && info->info.vdest_config.i_new_role == V_DEST_RL_QUIESCED) {

		/* Bad: Directly accessing service tree */
		svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext(&gl_mds_mcm_cb->svc_list, NULL);
		while (svc_info != NULL) {
			vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_info->svc_hdl);
			if (local_vdest_id == vdest_id) {
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     V_DEST_RL_STANDBY,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       V_DEST_RL_ACTIVE,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       V_DEST_RL_ACTIVE,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
			}
			svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext
			    (&gl_mds_mcm_cb->svc_list, (uns8 *)&svc_info->svc_hdl);
		}

		mds_vdest_tbl_update_role(local_vdest_id, V_DEST_RL_QUIESCED, TRUE);
		/* timer started in tbl_update */

		if (local_vdest_policy == NCS_VDEST_TYPE_MxN) {
			mds_mdtm_vdest_uninstall(local_vdest_id);
		}
		m_MDS_LOG_NOTIFY("MCM_API : vdest_chg_role: VDEST id = %d : Role Changed : Active -> Quiesced",
				 local_vdest_id);
	} else if (current_role == V_DEST_RL_QUIESCED && info->info.vdest_config.i_new_role == V_DEST_RL_ACTIVE) {
		/* Bad: Directly accessing service tree */
		svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext(&gl_mds_mcm_cb->svc_list, NULL);
		while (svc_info != NULL) {
			vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_info->svc_hdl);
			if (local_vdest_id == vdest_id) {
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     V_DEST_RL_ACTIVE,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       V_DEST_RL_STANDBY,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     V_DEST_RL_ACTIVE,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
			}
			svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext
			    (&gl_mds_mcm_cb->svc_list, (uns8 *)&svc_info->svc_hdl);
		}

		mds_vdest_tbl_update_role(local_vdest_id, V_DEST_RL_ACTIVE, TRUE);
		/* timer started in tbl_update */

		if (local_vdest_policy == NCS_VDEST_TYPE_MxN) {
			mds_mdtm_vdest_install(local_vdest_id);
		}
		m_MDS_LOG_NOTIFY("MCM_API : vdest_chg_role: VDEST id = %d : Role Changed : Quiesced -> Active",
				 local_vdest_id);

	} else if (current_role == V_DEST_RL_QUIESCED && info->info.vdest_config.i_new_role == V_DEST_RL_STANDBY) {
		/* Just update the role */
		mds_vdest_tbl_update_role(local_vdest_id, V_DEST_RL_STANDBY, TRUE);
		/* timer stopped in tbl_update */

		m_MDS_LOG_NOTIFY("MCM_API : vdest_chg_role : VDEST id = %d : Role Changed : Quiesced -> Standby",
				 local_vdest_id);

	} else if (current_role == V_DEST_RL_STANDBY && info->info.vdest_config.i_new_role == V_DEST_RL_QUIESCED) {
		m_MDS_LOG_ERR
		    ("MCM_API : vdest_chg_role: VDEST id = %d : Role Changed : Standby -> Quiesced : NOT POSSIBLE",
		     local_vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_vdest_chg_role");
		return NCSCC_RC_FAILURE;
	} else if (current_role == V_DEST_RL_STANDBY && info->info.vdest_config.i_new_role == V_DEST_RL_ACTIVE) {

		/* Bad: Directly accessing service tree */
		svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext(&gl_mds_mcm_cb->svc_list, NULL);
		while (svc_info != NULL) {
			vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_info->svc_hdl);
			if (local_vdest_id == vdest_id) {
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     info->info.vdest_config.i_new_role,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       current_role,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     info->info.vdest_config.i_new_role,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
			}
			svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext
			    (&gl_mds_mcm_cb->svc_list, (uns8 *)&svc_info->svc_hdl);
		}
		/* Update the role */
		mds_vdest_tbl_update_role(local_vdest_id, info->info.vdest_config.i_new_role, TRUE);

		if (local_vdest_policy == NCS_VDEST_TYPE_MxN) {
			/* Install vdest instance */
			mds_mdtm_vdest_install(local_vdest_id);
		}

		m_MDS_LOG_NOTIFY("MCM_API : vdest_chg_role : VDEST id = %d : Role Changed : Standby -> Active",
				 local_vdest_id);

	} else if (current_role == V_DEST_RL_ACTIVE && info->info.vdest_config.i_new_role == V_DEST_RL_STANDBY) {

		/* Bad: Directly accessing service tree */
		svc_info = NULL;
		svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext(&gl_mds_mcm_cb->svc_list, NULL);
		while (svc_info != NULL) {
			vdest_id = m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_info->svc_hdl);
			if (local_vdest_id == vdest_id) {
				mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						     svc_info->install_scope,
						     info->info.vdest_config.i_new_role,
						     local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       current_role,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
				mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl),
						       svc_info->install_scope,
						       current_role,
						       local_vdest_id, local_vdest_policy, svc_info->svc_sub_part_ver);
			}
			svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext
			    (&gl_mds_mcm_cb->svc_list, (uns8 *)&svc_info->svc_hdl);
		}

		/* Update the role */
		mds_vdest_tbl_update_role(local_vdest_id, info->info.vdest_config.i_new_role, TRUE);

		if (local_vdest_policy == NCS_VDEST_TYPE_MxN) {
			/* Install vdest instance */
			mds_mdtm_vdest_uninstall(local_vdest_id);
		}

		m_MDS_LOG_NOTIFY("MCM_API : vdest_chg_role : VDEST id = %d : Role Changed : Active -> Standby",
				 local_vdest_id);

	} else if (current_role == info->info.vdest_config.i_new_role) {
		m_MDS_LOG_NOTIFY
		    ("MCM_API : vdest_chg_role : Role Changed FAILED for VDEST id = %d : OLD_ROLE = NEW_ROLE = %s",
		     local_vdest_id,
		     (current_role ==
		      V_DEST_RL_STANDBY) ? "standby" : ((current_role == V_DEST_RL_ACTIVE) ? "active" : "quieced"));
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_chg_role");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_pwe_create

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  
            

*********************************************************/
uns32 mds_mcm_pwe_create(NCSMDS_ADMOP_INFO *info)
{

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_pwe_create");

	if ((info->info.pwe_create.i_pwe_id > NCSMDS_MAX_PWES) || (info->info.pwe_create.i_pwe_id == 0)) {
		m_MDS_LOG_ERR("MCM_API : pwe_create : FAILED : PWE id = %d not in prescribed range ",
			      info->info.pwe_create.i_pwe_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_pwe_create");
		return NCSCC_RC_FAILURE;
	}

	/* check whether hdl is of adest or vdest */
	if (info->info.pwe_create.i_mds_dest_hdl == (MDS_HDL)(m_ADEST_HDL)) {
		/* It is ADEST hdl */

		if (mds_pwe_tbl_query((MDS_VDEST_HDL)m_VDEST_ID_FOR_ADEST_ENTRY,
				      info->info.pwe_create.i_pwe_id) == NCSCC_RC_SUCCESS) {
			/* PWE already present */

			m_MDS_LOG_ERR("MCM_API : pwe_create : FAILED : PWE id = %d Already exist on ADEST",
				      info->info.pwe_create.i_pwe_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_pwe_create");
			return NCSCC_RC_FAILURE;
		} else {
			mds_pwe_tbl_add((MDS_VDEST_HDL)m_VDEST_ID_FOR_ADEST_ENTRY,
					info->info.pwe_create.i_pwe_id,
					(MDS_PWE_HDL *)&info->info.pwe_create.o_mds_pwe_hdl);

			/* info->info.pwe_create.o_mds_pwe_hdl = (NCSCONTEXT)
			   m_MDS_GET_PWE_HDL_FROM_PWE_ID_AND_VDEST_ID(info->info.pwe_create.i_pwe_id, 
			   m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info.pwe_create.i_mds_dest_hdl)); */

			m_MDS_LOG_NOTIFY("MCM_API : PWE id = %d Created Successfully on ADEST",
					 info->info.pwe_create.i_pwe_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_pwe_create");
			return NCSCC_RC_SUCCESS;
		}

	} else if ((info->info.pwe_create.i_mds_dest_hdl & 0xffff0000) != 0) {
		/* It is PWE hdl(pwe+vdest) */
		/* ERROR ERROR ERROR */
		m_MDS_LOG_ERR("MCM_API : pwe_create : FAILED : DEST HDL = %llx passed is already PWE HDL",
			      info->info.pwe_create.i_mds_dest_hdl);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_pwe_create");
		return NCSCC_RC_FAILURE;
	} else {
		/* It is VDEST hdl */

		if (mds_pwe_tbl_query((MDS_VDEST_HDL)info->info.pwe_create.i_mds_dest_hdl,
				      info->info.pwe_create.i_pwe_id) == NCSCC_RC_SUCCESS) {
			/* PWE already present */
			m_MDS_LOG_ERR("MCM_API : pwe_create : FAILED : PWE id = %d Already exist on VDEST id = %d",
				      info->info.pwe_create.i_pwe_id,
				      m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->info.pwe_create.i_mds_dest_hdl));
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_pwe_create");
			return NCSCC_RC_FAILURE;
		} else {
			mds_pwe_tbl_add((MDS_VDEST_HDL)info->info.pwe_create.i_mds_dest_hdl,
					info->info.pwe_create.i_pwe_id,
					(MDS_PWE_HDL *)&info->info.pwe_create.o_mds_pwe_hdl);

			/* info->info.pwe_create.o_mds_pwe_hdl =(NCSCONTEXT) 
			   m_MDS_GET_PWE_HDL_FROM_PWE_ID_AND_VDEST_ID(info->info.pwe_create.i_pwe_id, 
			   m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info.pwe_create.i_mds_dest_hdl)); */

			m_MDS_LOG_NOTIFY("MCM_API : PWE id = %d Created Successfully on VDEST id = %d",
					 info->info.pwe_create.i_pwe_id,
					 m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->info.pwe_create.i_mds_dest_hdl));
			m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_pwe_create");
			return NCSCC_RC_SUCCESS;
		}
	}
}

/*********************************************************

  Function NAME: mds_mcm_pwe_destroy

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_pwe_destroy(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_PWE_HDL temp_pwe_hdl;
	MDS_SVC_ID temp_svc_id;
	NCSMDS_INFO temp_ncsmds_info;
	MDS_VDEST_ID vdest_id;
	PW_ENV_ID pwe_id;
	MDS_SVC_INFO *svc_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_pwe_destroy");

	pwe_id = m_MDS_GET_PWE_ID_FROM_PWE_HDL(info->info.pwe_destroy.i_mds_pwe_hdl);
	vdest_id = m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->info.pwe_destroy.i_mds_pwe_hdl);

	status = mds_pwe_tbl_query(m_MDS_GET_VDEST_HDL_FROM_VDEST_ID(vdest_id), pwe_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Pwe doesn't exist */
		m_MDS_LOG_ERR("MCM_API : pwe_destroy : PWE id = %d Doesn't exist on VDEST id = %d", pwe_id, vdest_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_pwe_destroy");
		return NCSCC_RC_FAILURE;
	}

	temp_ncsmds_info.i_op = MDS_UNINSTALL;
	temp_ncsmds_info.info.svc_uninstall.i_msg_free_cb = NULL;

/* STEP 1: For ever svc on PWE,
            - Delete subscription entries if any from  
              LOCAL-UNIQ-SUBSCRIPTION-REQUEST-Table and BLOCK-SEND-Request Table.
            - Call mdtm_svc_uninstall(svc_hdl, Current Role) */

	svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext(&gl_mds_mcm_cb->svc_list, NULL);
	while (svc_info != NULL) {
		temp_pwe_hdl = m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_info->svc_hdl);
		if (temp_pwe_hdl == (MDS_PWE_HDL)info->info.pwe_destroy.i_mds_pwe_hdl) {
			temp_svc_id = m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_info->svc_hdl);
			temp_ncsmds_info.i_mds_hdl = info->info.pwe_destroy.i_mds_pwe_hdl;
			temp_ncsmds_info.i_svc_id = temp_svc_id;

			mds_mcm_svc_uninstall(&temp_ncsmds_info);
		}
		svc_info = (MDS_SVC_INFO *)ncs_patricia_tree_getnext
		    (&gl_mds_mcm_cb->svc_list, (uns8 *)&svc_info->svc_hdl);
	}

/* STEP 2: Delete entry from PWE Table */
	status = mds_pwe_tbl_del((MDS_PWE_HDL)info->info.pwe_destroy.i_mds_pwe_hdl);

	m_MDS_LOG_NOTIFY("MCM_API : PWE id = %d on VDEST id = %d Destoryed Successfully", pwe_id, vdest_id);

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_pwe_destroy");
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 *
 * Function Name: mds_mcm_adm_pwe_query
 *
 * Purpose:       This function queries information about a pwe on a local
 *                vdest.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

uns32 mds_mcm_adm_pwe_query(NCSMDS_ADMOP_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_adm_pwe_query");

	status = mds_pwe_tbl_query
	    (m_MDS_GET_VDEST_HDL_FROM_PWE_HDL(info->info.pwe_query.i_local_dest_hdl), info->info.pwe_query.i_pwe_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Pwe Already exists */
		m_MDS_LOG_ERR("MCM_API : pwe_query : FAILURE : PWE id = %d Doesn't Exists",
			      info->info.pwe_query.i_pwe_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_adm_pwe_query");
		return NCSCC_RC_FAILURE;
	}
	/* check whether hdl is of adest or vdest */
	if ((uns32)info->info.pwe_query.i_local_dest_hdl == m_ADEST_HDL) {
		/* It is ADEST hdl */

		info->info.pwe_query.o_mds_pwe_hdl = (MDS_HDL)m_MDS_GET_PWE_HDL_FROM_VDEST_HDL_AND_PWE_ID
		    ((MDS_VDEST_HDL)m_VDEST_ID_FOR_ADEST_ENTRY, info->info.pwe_query.i_pwe_id);

		m_MDS_LOG_NOTIFY("MCM_API : pwe_query : SUCCESS for PWE id = %d on ADEST",
				 info->info.pwe_query.i_pwe_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_adm_pwe_query");
		return NCSCC_RC_SUCCESS;
	} else if ((((uns32)info->info.pwe_query.i_local_dest_hdl) & 0xffff0000) != 0) {
		/* It is PWE hdl(pwe+vdest) */
		/* ERROR ERROR ERROR */
		m_MDS_LOG_ERR
		    ("MCM_API : pwe_query : PWE Query Failed for PWE id = %d as DEST hdl = %llx passed is PWE_HDL",
		     info->info.pwe_query.i_pwe_id, info->info.pwe_query.i_local_dest_hdl);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_adm_pwe_query");
		return NCSCC_RC_FAILURE;
	} else {
		/* It is VDEST hdl */

		info->info.pwe_query.o_mds_pwe_hdl = (MDS_HDL)m_MDS_GET_PWE_HDL_FROM_VDEST_HDL_AND_PWE_ID
		    (info->info.pwe_query.i_local_dest_hdl, info->info.pwe_query.i_pwe_id);

		m_MDS_LOG_NOTIFY("MCM_API : pwe_query : SUCCESS for PWE id = %d on VDEST id = %d",
				 info->info.pwe_query.i_pwe_id,
				 m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->info.pwe_query.i_local_dest_hdl));

		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_adm_pwe_query");
		return NCSCC_RC_SUCCESS;
	}
}

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*              ncsmds_api                      */

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*********************************************************

  Function NAME: mds_mcm_svc_install

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uns32 mds_mcm_svc_install(NCSMDS_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	V_DEST_RL local_vdest_role;
	NCS_VDEST_TYPE local_vdest_policy;
	MDS_VDEST_ID local_vdest_id = 0;
	local_vdest_id = m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl);

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_install");

	if ((info->i_svc_id > NCSMDS_MAX_SVCS) || (info->i_svc_id == 0)) {
		m_MDS_LOG_ERR("MCM_API : svc_install : FAILED : svc id = %d  not in prescribed range", info->i_svc_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
		return NCSCC_RC_FAILURE;
	}

	if ((info->info.svc_install.i_install_scope < NCSMDS_SCOPE_INTRANODE)
	    || (info->info.svc_install.i_install_scope > NCSMDS_SCOPE_NONE)) {
		m_MDS_LOG_ERR
		    ("MCM_API : svc_install : FAILED : svc id = %d , Should use the proper scope of installation",
		     info->i_svc_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
		return NCSCC_RC_FAILURE;
	}

	if (info->info.svc_install.i_mds_q_ownership == FALSE) {
		if (info->i_svc_id >= NCSMDS_SVC_ID_EXTERNAL_MIN) {
			m_MDS_LOG_ERR
			    ("MCM_API : svc_install : FAILED : svc id = %d , Should use the MDS Q Ownership model",
			     info->i_svc_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
			return NCSCC_RC_FAILURE;
		}
	}

	status = mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id);
	if (status == NCSCC_RC_SUCCESS) {
		/* Service already exist */
		m_MDS_LOG_ERR("MCM_API : svc_install : SVC id = %d on VDEST id = %d FAILED : SVC Already Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
		return NCSCC_RC_FAILURE;
	}

/*    STEP 1: Create a SVC-TABLE entry
        INPUT
        -   Pwe_Hdl
        -   SVC_ID
        -   SCOPE
        -   Q-ownership Flag
        -   Selection-obj (if any)
        -   Callback Ptr.
        Output
        -   Svc_Hdl
*/
	/* Add new service entry to SVC Table */
	if (mds_svc_tbl_add(info) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MCM_API : svc_install : FAILED : svc id = %d", info->i_svc_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
		return NCSCC_RC_FAILURE;
	}

/*    STEP 2: if (Q-ownership=MDS)
                Create SYSF_MBX */
	/* Taken care by db function */

/*    STEP 3: Call mdtm_svc_install(svc_hdl) */

	/* Get current role of VDEST */
	mds_vdest_tbl_get_role(m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->i_mds_hdl), &local_vdest_role);
	mds_vdest_tbl_get_policy(m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->i_mds_hdl), &local_vdest_policy);

	if (local_vdest_role == V_DEST_RL_QUIESCED) {
		/* if Current role of VDEST is Quiesced install with Standby */
		local_vdest_role = V_DEST_RL_STANDBY;
	}
	/* Inform MDTM */
	status = mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_PWE_HDL(info->i_mds_hdl),
				      info->i_svc_id,
				      info->info.svc_install.i_install_scope,
				      local_vdest_role,
				      m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->i_mds_hdl),
				      local_vdest_policy, info->info.svc_install.i_mds_svc_pvt_ver);
	if (status != NCSCC_RC_SUCCESS) {
		/* MDTM can't bind the service */

		/* Remove SVC info from MCM database */
		mds_svc_tbl_del((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, NULL);	/* Last argument is NULL as no one else will be able to add in mailbox without getting lock */

		m_MDS_LOG_ERR("MCM_API : svc_install : SVC id = %d on VDEST id = %d FAILED : MDTM returned Failure",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_install");
		return NCSCC_RC_FAILURE;
	}

	/* Perform Another bind as current VDEST role is Active */
	if (local_vdest_role == V_DEST_RL_ACTIVE && local_vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY) {
		status = mds_mdtm_svc_install(m_MDS_GET_PWE_ID_FROM_PWE_HDL(info->i_mds_hdl),
					      info->i_svc_id,
					      info->info.svc_install.i_install_scope,
					      local_vdest_role,
					      m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(info->i_mds_hdl),
					      local_vdest_policy, info->info.svc_install.i_mds_svc_pvt_ver);
		if (status != NCSCC_RC_SUCCESS) {
			/* MDTM can't bind the service second time */

			/* Neither delete service nor return Failure as first bind is already successful
			   and second bind any way we ignore on the other end (subscriber end) so impact is minimal */

			m_MDS_LOG_ERR
			    ("MCM_API : svc_install : Second install for : SVC id = %d on VDEST id = %d FAILED : MDTM returned Failure",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		}
	}

	m_MDS_LOG_NOTIFY("MCM_API : SVC id = %d on VDEST id = %d, SVC_PVT_VER = %d Install Successfull",
			 info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
			 info->info.svc_install.i_mds_svc_pvt_ver);
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_install");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_svc_uninstall

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uns32 mds_mcm_svc_uninstall(NCSMDS_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_SVC_HDL svc_hdl;
	MDS_SUBSCRIPTION_INFO *temp_subtn_info;
	NCSMDS_INFO temp_ncsmds_info;
	MDS_SVC_INFO *svc_cb;
	MDS_VDEST_ID vdest_id;
	V_DEST_RL vdest_role;
	NCS_VDEST_TYPE vdest_policy;
	MDS_SVC_ID svc_id_max1[1];	/* Max 1 element */

	MDS_MCM_SYNC_SEND_QUEUE *q_hdr = NULL, *prev_mem = NULL;
	MDS_VDEST_ID local_vdest_id = 0;

	local_vdest_id = m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl);

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_uninstall");

/* STEP 1: From Input get svc_hdl */
/* STEP 2: Search subscr_hdl from LOCAL-UNIQ-SUBSCRIPTION-REQUEST Table and make PEER_SVC_ID_LIST

if (PEER_SVC_ID_LIST != NULL)
    {
        call mds_unsubscribe(vdest_hdl/pwe_hdl, svc_id(self),PEER_SVC_ID_LIST)
             (MDS API)
    }*/

	/* Check whether service exist */
	status = mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR("MCM_API : svc_uninstall : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_uninstall");
		return NCSCC_RC_FAILURE;
	}

	temp_ncsmds_info.i_mds_hdl = info->i_mds_hdl;
	temp_ncsmds_info.i_svc_id = info->i_svc_id;
	temp_ncsmds_info.i_op = MDS_CANCEL;
	temp_ncsmds_info.info.svc_cancel.i_num_svcs = 1;
	temp_ncsmds_info.info.svc_cancel.i_svc_ids = svc_id_max1;	/* Storage associated */

	svc_hdl = m_MDS_GET_SVC_HDL_FROM_PWE_HDL_AND_SVC_ID((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id);

	while (mds_svc_tbl_get_first_subscription(svc_hdl, &temp_subtn_info) != NCSCC_RC_FAILURE) {
		temp_ncsmds_info.info.svc_cancel.i_svc_ids[0] = temp_subtn_info->sub_svc_id;
		mds_mcm_svc_unsubscribe(&temp_ncsmds_info);
		/* this will remove subscription entry so it will 
		   get next result in first_subscription query */
	}

/* STEP 3: mdtm_svc_uninstall(vdest-id, pwe_id, svc_id, svc_hdl) */

	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, (NCSCONTEXT)&svc_cb)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR("MCM_API : svc_uninstall : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_uninstall");
		return NCSCC_RC_FAILURE;
	}

	vdest_id = m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl);
	mds_vdest_tbl_get_role(vdest_id, &vdest_role);
	mds_vdest_tbl_get_policy(vdest_id, &vdest_policy);

	status = mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_PWE_HDL(info->i_mds_hdl),
					info->i_svc_id,
					svc_cb->install_scope,
					vdest_role, vdest_id, vdest_policy, svc_cb->svc_sub_part_ver);
	if (status != NCSCC_RC_SUCCESS) {
		/* MDTM can't unsubscribe the service */
		m_MDS_LOG_ERR("MCM_API : svc_install : SVC id = %d on VDEST id = %d FAILED : MDTM returned Failure",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
	}
	/* Perform Another Unbind as current VDEST role is Active */
	if (vdest_role == V_DEST_RL_ACTIVE && local_vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY) {
		status = mds_mdtm_svc_uninstall(m_MDS_GET_PWE_ID_FROM_PWE_HDL(info->i_mds_hdl),
						info->i_svc_id,
						svc_cb->install_scope,
						vdest_role, vdest_id, vdest_policy, svc_cb->svc_sub_part_ver);
		if (status != NCSCC_RC_SUCCESS) {
			/* MDTM can't unsubscribe the service */
			m_MDS_LOG_ERR
			    ("MCM_API : svc_install : Second Uninstall for : SVC id = %d on VDEST id = %d FAILED : MDTM returned Failure",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		}
	}

/* STEP 4: if (Q-ownership==MDS)
           Destroy SYSF_MBX */

	/* Destroying MBX taken care by DB */

	/* Raise the selection object for the sync send Q and free the memory */
	q_hdr = svc_cb->sync_send_queue;
	while (q_hdr != NULL) {
		prev_mem = q_hdr;
		q_hdr = q_hdr->next_send;
		m_NCS_SEL_OBJ_IND(prev_mem->sel_obj);
		m_MMGR_FREE_SYNC_SEND_QUEUE(prev_mem);
	}
	svc_cb->sync_send_queue = NULL;

/* STEP 5: Delete a SVC-TABLE entry and do the node unsubsribe */

	if (svc_cb->i_node_subscr) {
		if (mds_mdtm_node_unsubscribe( svc_cb->node_subtn_ref_val) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MCM_API: mds_mdtm_node_unsubscribe \n");
		}
	}

	mds_svc_tbl_del((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, info->info.svc_uninstall.i_msg_free_cb);

	m_MDS_LOG_NOTIFY("MCM_API : SVC id = %d on VDEST id = %d UnInstall Successful",
			 info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_uninstall");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_svc_subscribe

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uns32 mds_mcm_svc_subscribe(NCSMDS_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	NCSMDS_SCOPE_TYPE install_scope;
	MDS_SVC_HDL svc_hdl;
	MDS_VIEW view;
	uns32 i = 0;
	NCSMDS_INFO unsubscribe_info;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_subscribe");

/* STEP 1: Validation pass: 
    - Check that "scope" is smaller or equal to install-scope of SVC_ID
    - For each SVC in PEER_SVCID_LIST, check that there is no subscription already existing (with same or different SCOPE or VIEW). 
      If any return failure. */

	status = mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Service doesn't exist */
		m_MDS_LOG_ERR("MCM_API : svc_subscribe : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_subscribe");
		return NCSCC_RC_FAILURE;
	}
	mds_svc_tbl_get_install_scope((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &install_scope);
	if ((info->info.svc_subscribe.i_scope > install_scope)
	    || (info->info.svc_subscribe.i_scope < NCSMDS_SCOPE_INTRANODE)) {
		/* Subscription scope is bigger than Install scope */

		m_MDS_LOG_ERR
		    ("MCM_API : svc_subscribe : SVC id = %d on VDEST id = %d FAILED : Subscripton Scope Mismatch",
		     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_subscribe");
		return NCSCC_RC_FAILURE;
	}
	mds_svc_tbl_get_svc_hdl((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &svc_hdl);
	if (info->i_op == MDS_SUBSCRIBE) {
		view = MDS_VIEW_NORMAL;
	} else {		/* It is MDS_RED_SUBSCRIBE */
		view = MDS_VIEW_RED;
	}

/* STEP 2: For each element of PEER_SVC_IDLIST:
   if (entry doesn't exist in SUBSCR-TABLE)
    {
        STEP 2.a: Create SUBSCR-TABLE entry
                INPUT
                -   SUB_SVCID
                -   PWE_ID
                -   SCOPE
                -   VIEW=NORMAL_VIEW/RED_VIEW
                -   Svc_Hdl
                OUTPUT
                -   Subscr_req_hdl
        STEP 2.b: Start Subscription Timer ( done in DB )
        STEP 2.c: Call mdtm_svc_subscribe(sub_svc_id, scope, svc_hdl)
    }
    else
    {
        if(Subscription is Implicit)
            change the Flag to Explicit.
    }
*/
	/* Validate whether subscription array gives is not null when count is non zero */
	if (info->info.svc_subscribe.i_num_svcs != 0 && info->info.svc_subscribe.i_svc_ids == NULL) {
		m_MDS_LOG_ERR
		    ("MCM_API : svc_subscribe : SVC id = %d on VDEST id = %d gave non zero count and NULL Subscription array",
		     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		return NCSCC_RC_FAILURE;
	}

	/* Subscription Validation */
	for (i = 0; i < info->info.svc_subscribe.i_num_svcs; i++) {

		if ((info->info.svc_subscribe.i_svc_ids[i] > NCSMDS_MAX_SVCS)
		    || (info->info.svc_subscribe.i_svc_ids[i] == 0)) {
			m_MDS_LOG_ERR
			    ("MCM_API : svc_subscribe : SVC id = %d on VDEST id = %d Subscription to SVC id = %d FAILED | not in prescribed range",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
			     info->info.svc_subscribe.i_svc_ids[i]);
			return NCSCC_RC_FAILURE;
		}

		status = NCSCC_RC_SUCCESS;
		status = mds_subtn_tbl_query(svc_hdl, info->info.svc_subscribe.i_svc_ids[i]);

		if (status == NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR
			    ("MCM_API : svc_subscribe : SVC id = %d on VDEST id = %d Subscription to SVC id = %d FAILED | ALREADY EXIST",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
			     info->info.svc_subscribe.i_svc_ids[i]);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_subscribe");
			return NCSCC_RC_FAILURE;
		}

	}

	for (i = 0; i < info->info.svc_subscribe.i_num_svcs; i++) {
		status = NCSCC_RC_SUCCESS;
		status = mds_subtn_tbl_query(svc_hdl, info->info.svc_subscribe.i_svc_ids[i]);

		if (status == NCSCC_RC_NO_CREATION) {
			/* Implicit subscription already present 
			   so change Implicit flag to explicit */
			status = mds_subtn_tbl_change_explicit
			    (svc_hdl, info->info.svc_subscribe.i_svc_ids[i],
			     (info->i_op) == MDS_SUBSCRIBE ? MDS_VIEW_NORMAL : MDS_VIEW_RED);
			m_MDS_LOG_NOTIFY
			    ("MCM_API : svc_subscribe :SVC id = %d on VDEST id = %d Subscription to SVC id = %d : Already Exist Implicitly : Changed to Explicit",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
			     info->info.svc_subscribe.i_svc_ids[i]);
		} else {	/* status = NCSCC_RC_FAILURE (It can't be SUCCESS, as it is verified in Validation) */
			/* Add new Subscription entry */
			status = mds_mcm_subtn_add(svc_hdl,
						   info->info.svc_subscribe.i_svc_ids[i],
						   info->info.svc_subscribe.i_scope, view, MDS_SUBTN_EXPLICIT);
			if (status != NCSCC_RC_SUCCESS) {
				/* There was some problem while subscribing so rollback all previous subscriptions */
				m_MDS_LOG_ERR
				    ("MCM_API : svc_subscribe :SVC id = %d on VDEST id = %d Subscription to SVC id = %d Failed",
				     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
				     info->info.svc_subscribe.i_svc_ids[i]);
				m_MDS_LOG_NOTIFY("MCM_API : svc_subscribe : Rollbacking previous subscriptions");

				/* Unsubscribing the subscribed services */
				memset(&unsubscribe_info, 0, sizeof(unsubscribe_info));
				unsubscribe_info.i_mds_hdl = info->i_mds_hdl;
				unsubscribe_info.i_svc_id = info->i_svc_id;
				unsubscribe_info.i_op = MDS_CANCEL;
				unsubscribe_info.info.svc_cancel.i_num_svcs = i;
				unsubscribe_info.info.svc_cancel.i_svc_ids = info->info.svc_subscribe.i_svc_ids;

				mds_mcm_svc_unsubscribe(&unsubscribe_info);

				m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_subscribe");
				return NCSCC_RC_FAILURE;
			}
			/* MDTM subscribe is done in above function */
			/* Tipc subtn_ref_hdl returned stored in subtn_info in above function */
		}
		m_MDS_LOG_NOTIFY
		    ("MCM_API : svc_subscribe :SVC id = %d on VDEST id = %d Subscription to SVC id = %d Successful",
		     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl),
		     info->info.svc_subscribe.i_svc_ids[i]);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_subscribe");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_svc_unsubscribe

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uns32 mds_mcm_svc_unsubscribe(NCSMDS_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_SVC_HDL svc_hdl;
	uns32 i;
	MDS_SUBTN_REF_VAL subscr_req_hdl;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_unsubscribe");

	status = mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id);
	if (status == NCSCC_RC_FAILURE) {
		/* Service doesn't exist */
		m_MDS_LOG_ERR("MCM_API : svc_unsubscribe : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL((MDS_PWE_HDL)info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_unsubscribe");
		return NCSCC_RC_FAILURE;
	}
	/* STEP 1: Fetch "svc-hdl" from SVC-TABLE.
	   INPUT
	   - SUBTN_REF_VAL
	   OUTPUT
	   -   SVC_HDL    */
	mds_svc_tbl_get_svc_hdl((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &svc_hdl);

	/* STEP 2: Find all subscription entries from LOCAL-SUBSCRIPTION-REQ-TABLE using "svc-hdl"
	   INPUT
	   -   SVC_HDL
	   OUTPUT
	   - List of "subtn_hdl" */

	/* Subscription Validation */
	for (i = 0; i < info->info.svc_cancel.i_num_svcs; i++) {
		status = NCSCC_RC_SUCCESS;
		status = mds_subtn_tbl_query(svc_hdl, info->info.svc_cancel.i_svc_ids[i]);

		if (status == NCSCC_RC_FAILURE) {
			m_MDS_LOG_ERR
			    ("MCM_API : svc_unsubscribe : SVC id = %d on VDEST id = %d Unsubscription to SVC id = %d FAILED : Not Subscribed",
			     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL((MDS_PWE_HDL)info->i_mds_hdl),
			     info->info.svc_cancel.i_svc_ids[i]);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_unsubscribe");
			return NCSCC_RC_FAILURE;
		}
	}

	for (i = 0; i < info->info.svc_cancel.i_num_svcs; i++) {
		NCSMDS_SCOPE_TYPE scope = NCSMDS_SCOPE_INTRAPCON;

		status = mds_subtn_tbl_get_ref_hdl(svc_hdl, info->info.svc_cancel.i_svc_ids[i], &subscr_req_hdl, &scope);
		if (status == NCSCC_RC_FAILURE) {
			/* Not able to get subtn ref hdl */
		} else {
			/* STEP 3: Call mcm_mdtm_unsubscribe(subtn_hdl) */
			/* Inform MDTM about unsubscription */
			mds_mdtm_svc_unsubscribe(m_MDS_GET_PWE_ID_FROM_PWE_HDL((MDS_PWE_HDL)(info->i_mds_hdl)),  
					info->info.svc_cancel.i_svc_ids[i], scope, subscr_req_hdl);
		}

		/* Delete all MDTM entries */
		mds_subtn_res_tbl_del_all(svc_hdl, info->info.svc_cancel.i_svc_ids[i]);
		mds_subtn_tbl_del(svc_hdl, info->info.svc_cancel.i_svc_ids[i]);
		m_MDS_LOG_NOTIFY
		    ("MCM_API : svc_unsubscribe : SVC id = %d on VDEST id = %d Unsubscription to SVC id = %d Successful",
		     info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL((MDS_PWE_HDL)info->i_mds_hdl),
		     info->info.svc_cancel.i_svc_ids[i]);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_unsubscribe");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_dest_query

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_dest_query(NCSMDS_INFO *info)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_SVC_HDL local_svc_hdl;
	MDS_SUBSCRIPTION_RESULTS_INFO *subtn_result_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_dest_query");

	info->info.query_dest.o_node_id = m_MDS_GET_NODE_ID_FROM_ADEST(info->info.query_dest.i_dest);
	if (info->info.query_dest.o_node_id == 0) {	/* Destination is VDEST */

		/* Get Service hdl */
		mds_svc_tbl_get_svc_hdl((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &local_svc_hdl);

		if (info->info.query_dest.i_query_for_role == TRUE) {	/* Return Role given Anchor */
			status = mds_subtn_res_tbl_get_by_adest(local_svc_hdl, info->info.query_dest.i_svc_id,
								(MDS_VDEST_ID)info->info.query_dest.i_dest,
								info->info.query_dest.info.query_for_role.i_anc,
								&info->info.query_dest.info.query_for_role.o_vdest_rl,
								&subtn_result_info);
			if (status == NCSCC_RC_FAILURE) {	/* No such subscription result present */
				m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_dest_query");
				return NCSCC_RC_FAILURE;
			}
		} else {	/* Return Anchor given Role */
			status = mds_subtn_res_tbl_getnext_any(local_svc_hdl, info->info.query_dest.i_svc_id,
							       &subtn_result_info);
			while (status != NCSCC_RC_FAILURE) {
				if (subtn_result_info->key.vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY) {	/* Subscription result table entry is VDEST entry */

					if (subtn_result_info->key.vdest_id ==
					    (MDS_VDEST_ID)info->info.query_dest.i_dest
					    && subtn_result_info->info.vdest_inst.role ==
					    info->info.query_dest.info.query_for_anc.i_vdest_rl) {
						info->info.query_dest.info.query_for_anc.o_anc =
						    (V_DEST_QA)subtn_result_info->key.adest;
						break;
					}
				}
				status = mds_subtn_res_tbl_getnext_any(local_svc_hdl, info->info.query_dest.i_svc_id,
								       &subtn_result_info);
			}
			if (status == NCSCC_RC_FAILURE) {	/* while terminated as it didnt find any result */
				m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_dest_query");
				return NCSCC_RC_FAILURE;
			}
		}
		/* Store common output parameters */
		if (subtn_result_info->key.adest == m_MDS_GET_ADEST)
			info->info.query_dest.o_local = TRUE;
		else
			info->info.query_dest.o_local = FALSE;
		/* Filling node id as previously it was checked for 0 for vdest/adest */
		info->info.query_dest.o_node_id = m_MDS_GET_NODE_ID_FROM_ADEST(subtn_result_info->key.adest);
		info->info.query_dest.o_adest = subtn_result_info->key.adest;

		m_MDS_LOG_NOTIFY("MCM_API : dest_query : Successful for DEST = %llx", info->info.query_dest.i_dest);
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_dest_query");
		return NCSCC_RC_SUCCESS;
	} else {		/* Destination is ADEST */
		m_MDS_LOG_ERR("MCM_API : dest_query : FAILED : DEST = %llx passed is ADEST ",
			      info->info.query_dest.i_dest);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_dest_query");
		return NCSCC_RC_FAILURE;
	}
}

/*********************************************************

  Function NAME: mds_mcm_pwe_query

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_pwe_query(NCSMDS_INFO *info)
{
	MDS_VDEST_ID vdest_id;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_pwe_query");

	info->info.query_pwe.o_pwe_id = (PW_ENV_ID)m_MDS_GET_PWE_ID_FROM_PWE_HDL((MDS_PWE_HDL)info->i_mds_hdl);

	if (((uns32)info->i_mds_hdl & m_ADEST_HDL) == m_ADEST_HDL) {
		/* It is ADEST hdl */
		info->info.query_pwe.o_absolute = 1;
		info->info.query_pwe.info.abs_info.o_adest = m_MDS_GET_ADEST;
	} else {
		/* It is VDEST hdl */
		info->info.query_pwe.o_absolute = 0;
		vdest_id = m_MDS_GET_VDEST_ID_FROM_PWE_HDL((MDS_PWE_HDL)info->i_mds_hdl);

		info->info.query_pwe.info.virt_info.o_vdest = (MDS_DEST)vdest_id;
		info->info.query_pwe.info.virt_info.o_anc = (V_DEST_QA)m_MDS_GET_ADEST;

		mds_vdest_tbl_get_role(vdest_id, &info->info.query_pwe.info.virt_info.o_role);

	}
	m_MDS_LOG_NOTIFY("MCM_API : query_pwe : Successful for PWE hdl = %lx", info->i_mds_hdl);
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_pwe_query");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_node_subscribe

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_node_subscribe(NCSMDS_INFO *info)
{
	MDS_SVC_HDL svc_hdl;
	MDS_SVC_INFO *local_svc_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_node_subscribe");

	if (NCSCC_RC_SUCCESS != mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id)) {
		/* Service doesn't exist */
		m_MDS_LOG_ERR("MCM_API : node_subscribe : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
				info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_node_subscribe");
		return NCSCC_RC_FAILURE;
	}
	mds_svc_tbl_get_svc_hdl((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &svc_hdl);

	/* Get Service info cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_hdl),
						m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl),
						(NCSCONTEXT)&local_svc_info)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR("MCM: SVC doesnt exists, returning from mds_mcm_node_subscribe=%d\n",
				info->i_svc_id);
		return NCSCC_RC_FAILURE;
	}

	if ( local_svc_info->i_node_subscr ) {
		m_MDS_LOG_ERR("MCM_API: node_subscribe: SVC id = %d ,VDEST id = %d FAILED : subscription Exist",
				info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		return NCSCC_RC_FAILURE;
	}
	else {
		if (mds_mdtm_node_subscribe( svc_hdl, &local_svc_info->node_subtn_ref_val) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MCM_API: mds_mdtm_node_subscribe: SVC id = %d Fail\n",info->i_svc_id);
			return NCSCC_RC_FAILURE;
		}
		local_svc_info->i_node_subscr = 1;
	}
	m_MDS_LOG_DBG("MCM_API : mds_mcm_node_subscribe : S\n");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_node_unsubscribe

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_node_unsubscribe(NCSMDS_INFO *info)
{
	MDS_SVC_HDL svc_hdl;
	MDS_SVC_INFO *local_svc_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_node_unsubscribe");

	if (NCSCC_RC_SUCCESS != mds_svc_tbl_query((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id)) {
		/* Service doesn't exist */
		m_MDS_LOG_ERR("MCM_API : node_subscribe : SVC id = %d on VDEST id = %d FAILED : SVC Doesn't Exist",
			      info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_node_subscribe");
		return NCSCC_RC_FAILURE;
	}
	mds_svc_tbl_get_svc_hdl((MDS_PWE_HDL)info->i_mds_hdl, info->i_svc_id, &svc_hdl);

	/* Get Service info cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_hdl),
						m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl),
						(NCSCONTEXT)&local_svc_info)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR("MCM: SVC doesnt exists, returning from mds_mcm_node_subscribe=%d\n",
				info->i_svc_id);
		return NCSCC_RC_FAILURE;
	}

	if (0 == local_svc_info->i_node_subscr ) {
		m_MDS_LOG_ERR("MCM_API: node_subscribe: SVC id = %d ,VDEST id = %d FAILED : node subscription doesnt Exist",
				info->i_svc_id, m_MDS_GET_VDEST_ID_FROM_PWE_HDL(info->i_mds_hdl));
		return NCSCC_RC_FAILURE;
	}
	else {
		if (mds_mdtm_node_unsubscribe( local_svc_info->node_subtn_ref_val) != NCSCC_RC_SUCCESS) {
			m_MDS_LOG_ERR("MCM_API: mds_mdtm_node_unsubscribe: SVC id = %d Fail\n",info->i_svc_id);
			return NCSCC_RC_FAILURE;
		}
		local_svc_info->i_node_subscr = 0;
		local_svc_info->node_subtn_ref_val = 0;
        }
	m_MDS_LOG_DBG("MCM_API : mds_mcm_node_unsubscribe : S\n");
	return NCSCC_RC_SUCCESS;
}

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*              MDTM to MCM if                  */

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*********************************************************

  Function NAME: mds_mcm_svc_up

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/

uns32 mds_mcm_svc_up(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, V_DEST_RL role,
		     NCSMDS_SCOPE_TYPE scope, MDS_VDEST_ID vdest_id,
		     NCS_VDEST_TYPE vdest_policy, MDS_DEST adest,
		     NCS_BOOL my_pcon,
		     MDS_SVC_HDL local_svc_hdl,
		     MDS_SUBTN_REF_VAL subtn_ref_val,
		     MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type)
{
	uns32 status = NCSCC_RC_SUCCESS;
	NCSMDS_SCOPE_TYPE local_subtn_scope;
	MDS_VIEW local_subtn_view;
	MDS_DEST active_adest;
	V_DEST_RL dest_role;
	NCS_BOOL tmr_running;
	NCSMDS_CALLBACK_INFO cbinfo;
	MDS_SUBSCRIPTION_RESULTS_INFO *subtn_result_info = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *active_subtn_result_info = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *next_active_result_info = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_up : Details below :");
	m_MDS_LOG_DBG("MCM_API : LOCAL SVC INFO : SVC id = %d | PWE id = %d | VDEST id = %d |",
		      m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
		      m_MDS_GET_PWE_ID_FROM_SVC_HDL(local_svc_hdl), m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl));
	m_MDS_LOG_DBG
	    ("MCM_API : REMOTE SVC INFO : SVC id = %d | PWE id = %d | VDEST id = %d | VDEST_POLICY = %d | INSTALL SCOPE = %d | ROLE = %d | MY_PCON = %d |",
	     svc_id, pwe_id, vdest_id, vdest_policy, scope, role, my_pcon);

/*
For Each Unique Svc_Hdl
if (entry doesnot exists corresponding to <Svc_Hdl, SVC_ID, PWE_ID, VDEST> in SUBSCRIPTION-RESULT-Table)
    {
        - Add entry in  SUBSCRIPTION-RESULT-Table
        - Call SVC_HDL User Callback function (Event=UP)
        - Call MDTM_TX_HDL_REGISTER
            INPUT
            TX_HDL
    }
else (entry exists)
    {
      IF MDEST IS VDEST
          IF VDEST is MxN
               IF ROLE = ACTIVE

                    IF Current ROLE = ACTIVE 
                        Discard (Duplicate)
                    ELSE (Current Role = STANDBY/FORCED-STANDBY)
                        IF entry exist with ROLE= ACTIVE in 
                        SUBSCRIPTION-RESULT-Table
                            - Change ROLE of that entry to FORCED-STANDBY 
                              and Current Entry to ACTIVE
                        Else (entry doesnot exist with ROLE=ACTIVE)
                            - Call SVC_HDL User Callback function (Event=UP)
                            - Check AWAIT-ACTIVE-Table for entry, IF found
                                       - Stop Await Active Guard Timer
                                       - Send queued messages
                                       - Delete entry from Table                           

               ELSE (ROLE = STANDBY)
     
                   IF Current ROLE = STANDBY
                        Discard (Duplicate)
                   IF Current ROLE = FORCED-STANDBY
                            - Change ROLE of that entry to STANDBY                  
                    ELSE (Current Role = ACTIVE)
                        - Add an entry to AWAIT-ACTIVE-Table
                        - Start Await Active Guard Timer
                        - Call SVC_HDL User Callback function 
                          (Event=NO_ACTIVE)

           IF VDEST is N-Way
               IF ROLE = ACTIVE
                   
                   IF Current ROLE = ACTIVE
                        Discard (Duplicate)
                    ELSE (Current Role = STANDBY)
                        - Change ROLE of entry to ACTIVE

                            - Check AWAIT-ACTIVE-Table for entry, IF found
                                       - Stop Await Active Guard Timer
                                       - Send queued messages
                                       - Delete entry from Table                           
                        - Call SVC_HDL User Callback function (Event=UP)

               ELSE (ROLE = STANDBY)
                   IF Current ROLE = STANDBY
                        Discard (Duplicate)
                    ELSE (Current Role = ACTIVE)
                        - Call SVC_HDL User Callback function 
                          (Event=NO_ACTIVE)
                        - IF entry Doesn't exist with ROLE=Active in 
                         SUBCRIPTION-RESULT-Table
                               - Add entry to AWAIT-ACTIVE-Table
                               - Start Await Active Guard Timer.

      ELSE MDEST IS ADEST
        Discard (Duplicate)
    }
*/

	status = mds_svc_tbl_query(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_hdl),
				   m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));

	if (status == NCSCC_RC_FAILURE) {
		m_MDS_LOG_ERR("MCM_API : svc_up : Local SVC id = %d doesn't exist",
			      m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
		return NCSCC_RC_FAILURE;
	}

	/* clear cbinfo contents */
	memset(&cbinfo, 0, sizeof(cbinfo));

    /*************** Validation for SCOPE **********************/

	/* check the installation scope of service is in subscription scope */
	status = mds_subtn_tbl_get_details(local_svc_hdl, svc_id, &local_subtn_scope, &local_subtn_view);

	/* There is no check over here for above ,this may cause crash TO DO */
	status = NCSCC_RC_SUCCESS;
	status = mds_mcm_validate_scope(local_subtn_scope, scope, adest, svc_id, my_pcon);

	if (status == NCSCC_RC_FAILURE) {
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_up");
		return NCSCC_RC_SUCCESS;
	}

    /*************** Validation for SCOPE **********************/

	status = mds_subtn_res_tbl_query_by_adest(local_svc_hdl, svc_id, vdest_id, adest);

	if (status == NCSCC_RC_FAILURE) {	/* Subscription result tabel entry doesn't exist */

		if (vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {	/* Remote svc is on ADEST */

			mds_subtn_res_tbl_add(local_svc_hdl, svc_id,
					      (MDS_VDEST_ID)vdest_id, adest, role,
					      scope, NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN, svc_sub_part_ver, archword_type);

			/* Call user call back with UP */
			status = NCSCC_RC_SUCCESS;
			status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
							     role, vdest_id, adest, NCSMDS_UP,
							     svc_sub_part_ver, archword_type);

			if (status != NCSCC_RC_SUCCESS) {
				/* Callback failure */
				m_MDS_LOG_ERR("MCM_API : svc_up : UP Callback Failure for SVC id = %d",
					      m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
				m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
				return NCSCC_RC_FAILURE;
			}

			m_MDS_LOG_NOTIFY
			    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got UP for SVC id = %d on ADEST <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
			     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
			     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
			     m_MDS_GET_NODE_ID_FROM_ADEST(adest), m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
			     svc_sub_part_ver, archword_type);
		} else {	/* Remote svc is on VDEST */

			if (vdest_policy == NCS_VDEST_TYPE_MxN) {
				if (role == V_DEST_RL_ACTIVE) {
					status = NCSCC_RC_SUCCESS;
					status = mds_subtn_res_tbl_get(local_svc_hdl,
								       svc_id,
								       (MDS_VDEST_ID)vdest_id,
								       &active_adest,
								       &tmr_running, &subtn_result_info, TRUE);
					/* check if any other active present */
					if (status == NCSCC_RC_FAILURE) {	/* No active present */
						/* Add entry to subscription result table */
						status = mds_subtn_res_tbl_add(local_svc_hdl,
									       svc_id,
									       (MDS_VDEST_ID)vdest_id,
									       adest,
									       role, scope, vdest_policy,
									       svc_sub_part_ver, archword_type);

						/* Call user call back with UP */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest, NCSMDS_UP,
										     svc_sub_part_ver, archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : UP Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;
						}
						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);

						/* If subscripton is RED, call RED_UP also */
						if (local_subtn_view == MDS_VIEW_RED) {
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_RED_UP, svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : RED_UP Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got RED_UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}

					} else {	/* Active/Await-active already present */
						status = mds_subtn_res_tbl_add(local_svc_hdl,
									       svc_id,
									       (MDS_VDEST_ID)vdest_id,
									       adest,
									       role, scope,
									       vdest_policy,
									       svc_sub_part_ver, archword_type);
						/* Here Current active if any will get replaced */

						/* If it was Awaiting active give NEW ACTIVE to user */
						if (tmr_running == TRUE) {
							/* Call user callback UP */
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_NEW_ACTIVE,
											svc_sub_part_ver,
											archword_type);

							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : NEW_ACTIVE Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NEW_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}

						/* If subscripton is RED, call RED_UP also */
						if (local_subtn_view == MDS_VIEW_RED) {
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_RED_UP, svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : RED_UP Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got RED_UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}

					}
				} else {	/* role == V_DEST_RL_STANDBY */
					status = mds_subtn_res_tbl_add(local_svc_hdl,
								       svc_id,
								       (MDS_VDEST_ID)vdest_id,
								       adest,
								       role, scope, vdest_policy,
								       svc_sub_part_ver, archword_type);

					/* check the type of subscription normal/redundant */
					if (local_subtn_view == MDS_VIEW_NORMAL) {
						/* Dont send UP for Standby */
					} else {	/* local_subtn_view = MDS_VIEW_RED */

						/* Call user callback with UP */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_RED_UP,
										     svc_sub_part_ver, archword_type);

						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : RED_UP Callback Failure for SVC_ID = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}

						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got RED_UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);

					}
				}
			} else {	/* vdest_policy == NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN */

				if (role == V_DEST_RL_ACTIVE) {
					status = NCSCC_RC_SUCCESS;
					/* To get whether current entry exist */
					/* It will just store tmr_running and status */
					status = mds_subtn_res_tbl_get(local_svc_hdl,
								       svc_id,
								       (MDS_VDEST_ID)vdest_id,
								       &active_adest,
								       &tmr_running, &subtn_result_info, TRUE);

					/* Add entry first */
					mds_subtn_res_tbl_add(local_svc_hdl,
							      svc_id,
							      (MDS_VDEST_ID)vdest_id,
							      adest,
							      role, scope, vdest_policy,
							      svc_sub_part_ver, archword_type);

					/* check if any other active present */
					if (status == NCSCC_RC_FAILURE) {	/* No active present or Await Active Entry present */

						/* Call user callback UP */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_UP,
										     svc_sub_part_ver, archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : UP Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}

						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);

					} else if (tmr_running == TRUE) {	/* Await active entry is present */

						/* Call user callback UP */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_NEW_ACTIVE,
										     svc_sub_part_ver, archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : NEW_ACTIVE Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}

						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NEW_ACTIVE for SVC id = %d on VDEST id = %d  ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);

					}

					/* If subscripton is RED, call RED_UP also */
					if (local_subtn_view == MDS_VIEW_RED) {
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_RED_UP, svc_sub_part_ver,
										     archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : RED_UP Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}
						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got RED_UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);
					}

				} else {	/* role == V_DEST_RL_STANDBY */
					status = mds_subtn_res_tbl_add(local_svc_hdl,
								       svc_id,
								       (MDS_VDEST_ID)vdest_id,
								       adest,
								       role, scope, vdest_policy,
								       svc_sub_part_ver, archword_type);

					/* check the type of subscription normal/redundant */
					if (local_subtn_view == MDS_VIEW_NORMAL) {
						/* Dont send UP for Standby */
					} else {	/* local_subtn_view == MDS_VIEW_RED */

						/* Call user callback with UP for Standby */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_RED_UP,
										     svc_sub_part_ver, archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : RED_UP Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}
						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got RED_UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);
					}
				}

			}
		}		/* End : Remote service is on VDEST */
	} else {		/* Entry exist in subscription result table */

		if (vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
			/* Discard as it is already UP */
		} else {	/* Remote svc is on VDEST */

			if (vdest_policy == NCS_VDEST_TYPE_MxN) {
				if (role == V_DEST_RL_ACTIVE) {
					status = NCSCC_RC_SUCCESS;
					status = mds_subtn_res_tbl_get(local_svc_hdl,
								       svc_id,
								       (MDS_VDEST_ID)vdest_id,
								       &active_adest,
								       &tmr_running, &active_subtn_result_info, TRUE);

					if (status == NCSCC_RC_FAILURE) {	/* No other active entry exist */

						/* Get pointer to this adest */
						mds_subtn_res_tbl_get_by_adest(local_svc_hdl,
									       svc_id,
									       vdest_id,
									       adest, &dest_role, &subtn_result_info);
						/* Add Active subtn result table entry */

						if (dest_role == V_DEST_RL_STANDBY) {
							/* Change role to Active */
							mds_subtn_res_tbl_change_role(local_svc_hdl,
										      svc_id, vdest_id, adest, role);

							/* If subscripton is RED, call CHG_ROLE */
							if (local_subtn_view == MDS_VIEW_RED) {
								status = NCSCC_RC_SUCCESS;
								status =
								    mds_mcm_user_event_callback(local_svc_hdl, pwe_id,
												svc_id, role, vdest_id,
												adest, NCSMDS_CHG_ROLE,
												svc_sub_part_ver,
												archword_type);
								if (status != NCSCC_RC_SUCCESS) {
									/* Callback failure */
									m_MDS_LOG_ERR
									    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC id = %d",
									     m_MDS_GET_SVC_ID_FROM_SVC_HDL
									     (local_svc_hdl));
									m_MDS_LOG_DBG
									    ("MCM_API : Leaving : F : mds_mcm_svc_up");
									return NCSCC_RC_FAILURE;

								}
								m_MDS_LOG_NOTIFY
								    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
								     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl),
								     svc_id, vdest_id,
								     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
								     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
								     svc_sub_part_ver, archword_type);
							}
						}

						mds_subtn_res_tbl_add_active(local_svc_hdl,
									     svc_id,
									     vdest_id,
									     vdest_policy,
									     subtn_result_info,
									     svc_sub_part_ver, archword_type);
						/* Call user callback UP */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, adest,
										     NCSMDS_UP,
										     svc_sub_part_ver, archword_type);
						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : UP Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}
						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
						     archword_type);

					} else {	/* Active or Await Active entry exist */

						if (active_adest == adest) {	/* Current Active entry is entry for which we got up */

							/* Discard as it is duplicate */

						} else {	/* Some other Active or Await entry exist */

							/* Some other entry is active and this one standby */
							/* so make this one active */

							/* Get pointer to this adest */
							mds_subtn_res_tbl_get_by_adest(local_svc_hdl,
										       svc_id,
										       vdest_id,
										       adest,
										       &dest_role, &subtn_result_info);
							if (dest_role == V_DEST_RL_STANDBY) {
								/* Change role to Active */
								mds_subtn_res_tbl_change_role(local_svc_hdl,
											      svc_id,
											      vdest_id, adest, role);

								/* If subscripton is RED, call CHG_ROLE */
								if (local_subtn_view == MDS_VIEW_RED) {
									status = NCSCC_RC_SUCCESS;
									status =
									    mds_mcm_user_event_callback(local_svc_hdl,
													pwe_id, svc_id,
													role, vdest_id,
													adest,
													NCSMDS_CHG_ROLE,
													svc_sub_part_ver,
													archword_type);
									if (status != NCSCC_RC_SUCCESS) {
										/* Callback failure */
										m_MDS_LOG_ERR
										    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC id = %d",
										     m_MDS_GET_SVC_ID_FROM_SVC_HDL
										     (local_svc_hdl));
										m_MDS_LOG_DBG
										    ("MCM_API : Leaving : F : mds_mcm_svc_up");
										return NCSCC_RC_FAILURE;

									}
									m_MDS_LOG_NOTIFY
									    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
									     m_MDS_GET_SVC_ID_FROM_SVC_HDL
									     (local_svc_hdl),
									     m_MDS_GET_VDEST_ID_FROM_SVC_HDL
									     (local_svc_hdl), svc_id, vdest_id,
									     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
									     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
									     svc_sub_part_ver, archword_type);
								}
							}

							/* Make this as active */
							mds_subtn_res_tbl_change_active(local_svc_hdl,
											svc_id,
											(MDS_VDEST_ID)vdest_id,
											subtn_result_info,
											svc_sub_part_ver,
											archword_type);

							if ((tmr_running == TRUE)
							    || (local_subtn_view == MDS_VIEW_NORMAL)) {
								/* Call user callback UP */
								status = NCSCC_RC_SUCCESS;
								status =
								    mds_mcm_user_event_callback(local_svc_hdl, pwe_id,
												svc_id, role, vdest_id,
												adest,
												NCSMDS_NEW_ACTIVE,
												svc_sub_part_ver,
												archword_type);
								if (status != NCSCC_RC_SUCCESS) {
									/* Callback failure */
									m_MDS_LOG_ERR
									    ("MCM_API : svc_up : NEW_ACTIVE Callback Failure for SVC id = %d",
									     m_MDS_GET_SVC_ID_FROM_SVC_HDL
									     (local_svc_hdl));
									m_MDS_LOG_DBG
									    ("MCM_API : Leaving : F : mds_mcm_svc_up");
									return NCSCC_RC_FAILURE;

								}
								m_MDS_LOG_NOTIFY
								    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NEW_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
								     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl),
								     svc_id, vdest_id,
								     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
								     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
								     svc_sub_part_ver, archword_type);
							}
						}
					}
				} else {	/* role == V_DEST_RL_STANDBY */
					/* check whether then existing entry is active and
					   we got UP for standby for same entry */
					mds_subtn_res_tbl_get(local_svc_hdl,
							      svc_id,
							      (MDS_VDEST_ID)vdest_id,
							      &active_adest, &tmr_running, &subtn_result_info, TRUE);
					if (active_adest == adest) {	/* This entry is going down */

						mds_subtn_res_tbl_remove_active(local_svc_hdl,
										svc_id, (MDS_VDEST_ID)vdest_id);
						mds_subtn_res_tbl_change_role(local_svc_hdl,
									      svc_id,
									      vdest_id, adest, V_DEST_RL_STANDBY);

						/* Call user call back NO ACTIVE */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, 0,
										     NCSMDS_NO_ACTIVE,
										     svc_sub_part_ver,
										     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_up : NO_ACTIVE Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
							return NCSCC_RC_FAILURE;

						}
						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NO_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver);

						/* If subscripton is RED, call CHG_ROLE also */
						if (local_subtn_view == MDS_VIEW_RED) {
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_CHG_ROLE,
											svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}
					} else {	/* Some other entry is active */

						/* It can be either duplicate in this case we will diacard */
						/* Or some other entry is forcefully made active due to 
						   remote VDEST role change in this case we will just change role */

						mds_subtn_res_tbl_get_by_adest(local_svc_hdl,
									       svc_id,
									       vdest_id,
									       adest, &dest_role, &subtn_result_info);

						if (dest_role == V_DEST_RL_ACTIVE) {	/* There is change in role of this service */

							mds_subtn_res_tbl_change_role(local_svc_hdl,
										      svc_id,
										      vdest_id,
										      adest, V_DEST_RL_STANDBY);

							/* If subscripton is RED, call CHG_ROLE also */
							if (local_subtn_view == MDS_VIEW_RED) {
								status = NCSCC_RC_SUCCESS;
								status =
								    mds_mcm_user_event_callback(local_svc_hdl, pwe_id,
												svc_id, role, vdest_id,
												adest, NCSMDS_CHG_ROLE,
												svc_sub_part_ver,
												archword_type);
								if (status != NCSCC_RC_SUCCESS) {
									/* Callback failure */
									m_MDS_LOG_ERR
									    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC id = %d",
									     m_MDS_GET_SVC_ID_FROM_SVC_HDL
									     (local_svc_hdl));
									m_MDS_LOG_DBG
									    ("MCM_API : Leaving : F : mds_mcm_svc_up");
									return NCSCC_RC_FAILURE;

								}
								m_MDS_LOG_NOTIFY
								    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
								     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl),
								     svc_id, vdest_id,
								     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
								     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
								     svc_sub_part_ver, archword_type);
							}
						}
					}
				}
			} else {	/* vdest_policy == NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN */

				mds_subtn_res_tbl_get_by_adest(local_svc_hdl,
							       svc_id,
							       (MDS_VDEST_ID)vdest_id,
							       adest, &dest_role, &subtn_result_info);

				if (role == V_DEST_RL_ACTIVE) {
					if (dest_role == V_DEST_RL_ACTIVE) {
						/* Discard as it is duplicate */
					} else {	/* dest_role == V_DEST_RL_STANDBY */

						/* Make it ACTIVE */
						mds_subtn_res_tbl_change_role(local_svc_hdl,
									      svc_id,
									      vdest_id, adest, V_DEST_RL_ACTIVE);

						/* If subscripton is RED, call CHG_ROLE also */
						if (local_subtn_view == MDS_VIEW_RED) {
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_CHG_ROLE,
											svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}

						/* check if Any Active entry exists */
						status = NCSCC_RC_SUCCESS;

						status = mds_subtn_res_tbl_get(local_svc_hdl,
									       svc_id,
									       vdest_id,
									       &active_adest,
									       &tmr_running,
									       &active_subtn_result_info, TRUE);

						if (status == NCSCC_RC_SUCCESS) {	/* Active or Await active entry exists */

							if (tmr_running == TRUE) {	/* If Await active exist then point to this one as active */

								mds_subtn_res_tbl_change_active(local_svc_hdl,
												svc_id,
												vdest_id,
												subtn_result_info,
												svc_sub_part_ver,
												archword_type);
								/* Call user callback UP */
								status = NCSCC_RC_SUCCESS;
								status =
								    mds_mcm_user_event_callback(local_svc_hdl, pwe_id,
												svc_id, role, vdest_id,
												adest,
												NCSMDS_NEW_ACTIVE,
												svc_sub_part_ver,
												archword_type);
								if (status != NCSCC_RC_SUCCESS) {
									/* Callback failure */
									m_MDS_LOG_ERR
									    ("MCM_API : svc_up : NEW_ACTIVE Callback Failure for SVC id = %d",
									     m_MDS_GET_SVC_ID_FROM_SVC_HDL
									     (local_svc_hdl));
									m_MDS_LOG_DBG
									    ("MCM_API : Leaving : F : mds_mcm_svc_up");
									return NCSCC_RC_FAILURE;

								}
								m_MDS_LOG_NOTIFY
								    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NEW_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
								     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl),
								     svc_id, vdest_id,
								     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
								     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
								     svc_sub_part_ver, archword_type);
							} else {
								/* Some other active entry exist so dont disturb it */
								/* This entry can't be active as earlier it was Standby */
							}
						} else {
							/* Add Active entry */
							mds_subtn_res_tbl_add_active(local_svc_hdl,
										     svc_id,
										     vdest_id,
										     vdest_policy,
										     subtn_result_info,
										     svc_sub_part_ver, archword_type);

							/* Call user callback UP */
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_UP, svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : UP Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got UP for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}
					}
				} else {	/* role == V_DEST_RL_STANDBY */

					if (dest_role == V_DEST_RL_STANDBY) {
						/* Discard as it is duplicate */
					} else {	/* dest_role == V_DEST_RL_ACTIVE */

						/* This active entry is going to standby */

						/* check if Active entry was referring to the same entry going standby */
						status = NCSCC_RC_SUCCESS;
						status = mds_subtn_res_tbl_get(local_svc_hdl,
									       svc_id,
									       vdest_id,
									       &active_adest,
									       &tmr_running,
									       &active_subtn_result_info, TRUE);

						if (status == NCSCC_RC_SUCCESS) {	/* Active or Await active entry exists */

							if (tmr_running == TRUE) {	/* Timer is running so active entry points to NULL */
								/* Do nothing */
							} else {	/* Active entry exist */

								/* check whether the same entry is currently active */
								if (active_subtn_result_info == subtn_result_info) {
									/* Point to next active if present */
									status = NCSCC_RC_SUCCESS;
									status =
									    mds_subtn_res_tbl_query_next_active
									    (local_svc_hdl, svc_id, vdest_id,
									     subtn_result_info,
									     &next_active_result_info);

									if (subtn_result_info ==
									    next_active_result_info) {
										/* No other active present */
										mds_subtn_res_tbl_remove_active
										    (local_svc_hdl, svc_id, vdest_id);
										/* No active timer will be started in above function */

										/* Call user callback NO Active */

										status = NCSCC_RC_SUCCESS;
										status =
										    mds_mcm_user_event_callback
										    (local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, 0,
										     NCSMDS_NO_ACTIVE, svc_sub_part_ver,
										     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

										if (status != NCSCC_RC_SUCCESS) {
											/* Callback failure */
											m_MDS_LOG_ERR
											    ("MCM_API : svc_up : NO_ACTIVE Callback Failure for SVC id = %d",
											     m_MDS_GET_SVC_ID_FROM_SVC_HDL
											     (local_svc_hdl));
											m_MDS_LOG_DBG
											    ("MCM_API : Leaving : F : mds_mcm_svc_up");
											return NCSCC_RC_FAILURE;

										}
										m_MDS_LOG_NOTIFY
										    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got NO_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
										     m_MDS_GET_SVC_ID_FROM_SVC_HDL
										     (local_svc_hdl),
										     m_MDS_GET_VDEST_ID_FROM_SVC_HDL
										     (local_svc_hdl), svc_id, vdest_id,
										     m_MDS_GET_NODE_ID_FROM_ADEST
										     (adest),
										     m_MDS_GET_PROCESS_ID_FROM_ADEST
										     (adest), svc_sub_part_ver);
									} else {
										/* Change to Active to point to next active */
										mds_subtn_res_tbl_change_active
										    (local_svc_hdl, svc_id, vdest_id,
										     next_active_result_info,
										     svc_sub_part_ver, archword_type);
									}
								} else {
									/* We dont care, and dont disturb other actives */
								}
							}
						} else {
							/* just change role, which is done in step below */
						}

						/* Make this as STANDBY */
						mds_subtn_res_tbl_change_role(local_svc_hdl,
									      svc_id,
									      (MDS_VDEST_ID)vdest_id,
									      adest, V_DEST_RL_STANDBY);

						/* If subscripton is RED, call CHG_ROLE also */
						if (local_subtn_view == MDS_VIEW_RED) {
							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, adest,
											NCSMDS_CHG_ROLE,
											svc_sub_part_ver,
											archword_type);
							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_up : CHG_ROLE Callback Failure for SVC_ID = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_up");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_up : SVC id = %d on DEST id = %d got CHG_ROLE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver,
							     archword_type);
						}
					}
				}
			}
		}
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_up");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_svc_down

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_svc_down(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, V_DEST_RL role,
		       NCSMDS_SCOPE_TYPE scope, MDS_VDEST_ID vdest_id,
		       NCS_VDEST_TYPE vdest_policy, MDS_DEST adest,
		       NCS_BOOL my_pcon,
		       MDS_SVC_HDL local_svc_hdl,
		       MDS_SUBTN_REF_VAL subtn_ref_val,
		       MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type)
{
	uns32 status = NCSCC_RC_SUCCESS;
	NCSMDS_SCOPE_TYPE local_subtn_scope;
	MDS_VIEW local_subtn_view;
	MDS_DEST active_adest;
	NCS_BOOL tmr_running;
	MDS_SUBSCRIPTION_RESULTS_INFO *subtn_result_info = NULL;
	MDS_SUBSCRIPTION_RESULTS_INFO *next_active_result_info = NULL;
	V_DEST_RL dest_role;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_svc_down : Details below :");
	m_MDS_LOG_DBG("MCM_API : LOCAL SVC INFO : SVC id = %d | PWE id = %d | VDEST id = %d |",
		      m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
		      m_MDS_GET_PWE_ID_FROM_SVC_HDL(local_svc_hdl), m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl));
	m_MDS_LOG_DBG
	    ("MCM_API : REMOTE SVC INFO : SVC id = %d | PWE id = %d | VDEST id = %d | VDEST_POLICY = %d | INSTALL SCOPE = %d | ROLE = %d | MY_PCON = %d |",
	     svc_id, pwe_id, vdest_id, vdest_policy, scope, role, my_pcon);

/*
From subtn_ref_val get Svc_Hdl of service, for which event has received.

For Each Unique Svc_Hdl
if( entry doesnot exists corresponding to <Svc_Hdl, SVC_ID, PWE_ID, VDEST> in SUBSCRIPTION-RESULT-Table)
  - Discard (Getting Down before getting UP)
else (entry exists) 
    {
  IF MDEST IS VDEST
      IF VDEST is MxN
           IF ROLE = STANDBY

                IF Current ROLE = STANDBY
                    - Delete entry from SUBSCRIPTION- 
                       RESULT-Table.
                    - Call MDTM_TX_HDL_UNREGISTER
                 INPUT
                 TX_HDL
                IF Current ROLE = FORCED-STANDBY
                    Discard (if it is in FORCED-STANDBY 
                ELSE (Current Role = ACTIVE)
                    Discard (It should never reach here as it will not get 
                  "Withdraw for Standby" if earlier it is "published with 
                 Active" from same destination)

           ELSE (ROLE = ACTIVE)
 
               IF Current ROLE = STANDBY
                   Discard
               IF Current ROLE = FORCED_STANDBY / ACTIVE
                    - Check SUBSCRIPTION-RESULT-Table for any entries 
                       with ROLE=ACTIVE 
                               IF NOT FOUND
                                 - Add an entry to AWAIT-ACTIVE-Table
                                 - Start Await Active Guard Timer
                                 - Call SVC_HDL User Callback function 
                                   (Event=NO_ACTIVE)
                    - Delete entry from SUBSCRIPTION- RESULT-Table.
                    - Call MDTM_TX_HDL_UNREGISTER
                 INPUT
                 TX_HDL

       IF VDEST is N-Way
           IF ROLE = ACTIVE              
               
                IF Current ROLE = ACTIVE
                     - Delete entry from SUBSCRIPTION-RESULT-Table.
                     - Call MDTM_TX_HDL_UNREGISTER
                 INPUT
                 TX_HDL
                    - Check SUBSCRIPTION-RESULT-Table for any entries 
                       with ROLE=ACTIVE 
                               IF NOT FOUND
                                 - Add an entry to AWAIT-ACTIVE-Table
                                 - Start Await Active Guard Timer
                                 - Call SVC_HDL User Callback function 
                                   (Event=NO_ACTIVE)
                  
                ELSE (Current Role = Standby)
                      - DISCARD
 
          ELSE (ROLE = Standby)
                
                IF Current ROLE = Standby
                   - Delete entry from SUBSCRIPTION-RESULT-Table.
                   - Call MDTM_TX_HDL_UNREGISTER
               INPUT
               TX_HDL
                ELSE (Current Role = Active)
                   - DISCARD

  ELSE MDEST IS ADEST
       - Delete entry from SUBSCRIPTION-RESULT-Table.
       - Call MDTM_TX_HDL_UNREGISTER
    INPUT
    TX_HDL
       - Call SVC_HDL User Callback function 
          (Event=DOWN)
    }
*/

	status = mds_svc_tbl_query(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_hdl),
				   m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));

	if (status == NCSCC_RC_FAILURE) {
		/* Local Service Doesn't exist */
		m_MDS_LOG_ERR("MCM_API : svc_down : Local SVC doesn't exist");
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_down");
		return NCSCC_RC_FAILURE;
	}

    /*************** Validation for SCOPE **********************/

	/* check the installation scope of service is in subscription scope */
	status = mds_subtn_tbl_get_details(local_svc_hdl, svc_id, &local_subtn_scope, &local_subtn_view);
	status = NCSCC_RC_SUCCESS;
	status = mds_mcm_validate_scope(local_subtn_scope, scope, adest, svc_id, my_pcon);

	if (status == NCSCC_RC_FAILURE) {
		return NCSCC_RC_SUCCESS;
	}
    /*************** Validation for SCOPE **********************/

	status = mds_subtn_res_tbl_query_by_adest(local_svc_hdl, svc_id, (MDS_VDEST_ID)vdest_id, adest);
	if (status == NCSCC_RC_FAILURE) {	/* Subscription result tabel entry doesn't exist */

		/* Discard : Getting down before getting up */
	} else {		/* Entry exist in subscription result table */

		if (vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
			status = mds_subtn_res_tbl_del(local_svc_hdl, svc_id, vdest_id,
						       adest, NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
						       svc_sub_part_ver, archword_type);

			/* Call user call back with DOWN */

			status = NCSCC_RC_SUCCESS;
			status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
							     role, vdest_id, adest,
							     NCSMDS_DOWN,
							     svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

			if (status != NCSCC_RC_SUCCESS) {
				/* Callback failure */
				m_MDS_LOG_ERR("MCM_API : svc_down : DOWN Callback Failure for SVC id = %d",
					      m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
				m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_down");
				return NCSCC_RC_FAILURE;

			}
			m_MDS_LOG_NOTIFY
			    ("MCM_API : svc_down : SVC id = %d on DEST id = %d got DOWN for SVC id = %d on ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
			     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
			     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
			     m_MDS_GET_NODE_ID_FROM_ADEST(adest), m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
			     svc_sub_part_ver);

		} else {	/* Remote svc is on VDEST */

			mds_subtn_res_tbl_get_by_adest(local_svc_hdl,
						       svc_id, vdest_id, adest, &dest_role, &subtn_result_info);
			if (dest_role != role) {	/* Role Mismatch */

				/* Already role changed by Other role publish */
				/* If svc is really going down it will get down for Other role also */
				/* so Discard */
				m_MDS_LOG_INFO
				    ("MCM_API : svc_down : ROLE MISMATCH : SVC id = %d on DEST id = %d got DOWN for SVC id = %d on VDEST id = %d on ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d, rem_svc_archword=%d",
				     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
				     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
				     m_MDS_GET_NODE_ID_FROM_ADEST(adest), m_MDS_GET_PROCESS_ID_FROM_ADEST(adest),
				     svc_sub_part_ver, archword_type);

				/* Return success as we have to discard this event */
				m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_down");
				return NCSCC_RC_SUCCESS;
			}

			if (role == V_DEST_RL_ACTIVE) {
				/* Get info about subtn entry before deleting */
				status = mds_subtn_res_tbl_get(local_svc_hdl,
							       svc_id,
							       vdest_id,
							       &active_adest, &tmr_running, &subtn_result_info, TRUE);

				/* First delete the entry */
				mds_subtn_res_tbl_del(local_svc_hdl, svc_id, vdest_id,
						      adest, vdest_policy, svc_sub_part_ver, archword_type);

				if (active_adest == adest) {
					if (vdest_policy == NCS_VDEST_TYPE_MxN) {
						mds_subtn_res_tbl_remove_active(local_svc_hdl, svc_id, vdest_id);

						/* Call user call back with NO ACTIVE */
						status = NCSCC_RC_SUCCESS;
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
										     role, vdest_id, 0,
										     NCSMDS_NO_ACTIVE,
										     svc_sub_part_ver,
										     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

						if (status != NCSCC_RC_SUCCESS) {
							/* Callback failure */
							m_MDS_LOG_ERR
							    ("MCM_API : svc_down : NO_ACTIVE Callback Failure for SVC id = %d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
							m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_down");
							return NCSCC_RC_FAILURE;

						}

						m_MDS_LOG_NOTIFY
						    ("MCM_API : svc_down : SVC id = %d on DEST id = %d got NO_ACTIVE for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
						     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver);
						{
							MDS_SUBSCRIPTION_RESULTS_INFO *subtn_result_info = NULL;
							NCS_BOOL adest_exists = FALSE;

							/* if no adest remains for this svc, send MDS_DOWN */
							status = mds_subtn_res_tbl_getnext_any (local_svc_hdl, svc_id, &subtn_result_info);

							while (status != NCSCC_RC_FAILURE)
							{
								if (subtn_result_info->key.vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY)
								{
									adest_exists = TRUE;
									break;
								}

								status = mds_subtn_res_tbl_getnext_any (local_svc_hdl, svc_id, &subtn_result_info);
							}

							if (adest_exists == FALSE)
							{
								/* No other adest exists for this svc_id, Call user callback DOWN */
								status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id, role, vdest_id, 0, 
										NCSMDS_DOWN, svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);
							}
						}
					} else {	/* vdest_policy == NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN */

						status = NCSCC_RC_SUCCESS;
						status = mds_subtn_res_tbl_query_next_active(local_svc_hdl,
											     svc_id, vdest_id,
											     subtn_result_info,
											     &next_active_result_info);
						if (status == NCSCC_RC_FAILURE) {
							/* No other active present */
							mds_subtn_res_tbl_remove_active(local_svc_hdl, svc_id,
											vdest_id);
							/* No active timer will be started in above function */

							/* Call user callback NO Active */

							status = NCSCC_RC_SUCCESS;
							status =
							    mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
											role, vdest_id, 0,
											NCSMDS_NO_ACTIVE,
											svc_sub_part_ver,
											MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

							if (status != NCSCC_RC_SUCCESS) {
								/* Callback failure */
								m_MDS_LOG_ERR
								    ("MCM_API : svc_down : NO_ACTIVE Callback Failure for SVC id = %d",
								     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
								m_MDS_LOG_DBG
								    ("MCM_API : Leaving : F : mds_mcm_svc_down");
								return NCSCC_RC_FAILURE;

							}
							m_MDS_LOG_NOTIFY
							    ("MCM_API : svc_down : SVC id = %d on DEST id = %d got NO_ACTIVE for SVC id = %d on VDEST id = %d, ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
							     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
							     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id,
							     vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver);
						} else {
							/* Change to Active to point to next active */
							mds_subtn_res_tbl_change_active(local_svc_hdl, svc_id, vdest_id,
											next_active_result_info,
											svc_sub_part_ver,
											archword_type);
						}
					}
				}
				/* If subscripton is RED, call RED_DOWN also */
				if (local_subtn_view == MDS_VIEW_RED) {
					status = NCSCC_RC_SUCCESS;
					status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
									     role, vdest_id, adest,
									     NCSMDS_RED_DOWN,
									     svc_sub_part_ver,
									     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);
					if (status != NCSCC_RC_SUCCESS) {
						/* Callback failure */
						m_MDS_LOG_ERR
						    ("MCM_API : svc_down : RED_DOWN Callback Failure for SVC id = %d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
						m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_down");
						return NCSCC_RC_FAILURE;

					}
					m_MDS_LOG_NOTIFY
					    ("MCM_API : svc_down : SVC id = %d on DEST id = %d got RED_DOWN for SVC id = %d on VDEST id = %d on ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
					     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
					     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
					     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
					     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver);
				}
			} else {	/* role == V_DEST_RL_STANDBY */

				/* Delete backup entry */
				status = mds_subtn_res_tbl_del(local_svc_hdl, svc_id, vdest_id,
							       adest, vdest_policy, svc_sub_part_ver, archword_type);

				if (local_subtn_view == MDS_VIEW_NORMAL) {
					/* Dont send DOWN for Standby */
				} else {	/* local_subtn_view = MDS_VIEW_RED */

					/* Call user callback with DOWN */

					status = NCSCC_RC_SUCCESS;
					status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id,
									     role, vdest_id, adest,
									     NCSMDS_RED_DOWN,
									     svc_sub_part_ver,
									     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);

					if (status != NCSCC_RC_SUCCESS) {
						/* Callback failure */
						m_MDS_LOG_ERR
						    ("MCM_API : svc_down : RED_DOWN Callback Failure for SVC id = %d",
						     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl));
						m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_svc_down");
						return NCSCC_RC_FAILURE;

					}

					m_MDS_LOG_NOTIFY
					    ("MCM_API : svc_down : SVC id = %d on DEST id = %d got RED_DOWN for SVC id = %d on VDEST id = %d ADEST = <0x%08x, %u>, rem_svc_pvt_ver=%d",
					     m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
					     m_MDS_GET_VDEST_ID_FROM_SVC_HDL(local_svc_hdl), svc_id, vdest_id,
					     m_MDS_GET_NODE_ID_FROM_ADEST(adest),
					     m_MDS_GET_PROCESS_ID_FROM_ADEST(adest), svc_sub_part_ver);
				}
				{
					MDS_SUBSCRIPTION_RESULTS_INFO *subtn_result_info = NULL;
					NCS_BOOL adest_exists = FALSE;

					/* if no adest remains for this svc, send MDS_DOWN */
					status = mds_subtn_res_tbl_getnext_any (local_svc_hdl, svc_id, &subtn_result_info);

					while (status != NCSCC_RC_FAILURE)
					{
						if (subtn_result_info->key.vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY)
						{
							adest_exists = TRUE;
							break;
						}

						status = mds_subtn_res_tbl_getnext_any (local_svc_hdl, svc_id, &subtn_result_info);
					}

					if (adest_exists == FALSE)
					{
						/* No other adest exists for this svc_id, Call user callback DOWN */
						status = mds_mcm_user_event_callback(local_svc_hdl, pwe_id, svc_id, role, vdest_id, 0, 
								NCSMDS_DOWN, svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);
					}
				}
			}
		}
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_svc_down");
	return NCSCC_RC_SUCCESS;
}


/*********************************************************

  Function NAME: mds_mcm_node_up

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_node_up(MDS_SVC_HDL local_svc_hdl, NODE_ID node_id)

{
	MDS_MCM_MSG_ELEM *event_msg = NULL;
	MDS_SVC_INFO *local_svc_info = NULL;
	NCSMDS_CALLBACK_INFO *cbinfo = NULL;
	uns32 status = NCSCC_RC_SUCCESS;

	/* Get Service info cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_hdl),
						m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						(NCSCONTEXT)&local_svc_info)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR(" SVC doesnt exists, returning from mds_mcm_node_up\n");
		return NCSCC_RC_FAILURE;
	}


	if (0 == local_svc_info->i_node_subscr ) {
		/* Node Subscription Doesn't exist */
		m_MDS_LOG_ERR(" Node subscription doesnt exists, returning from mds_mcm_node_up\n");
		return NCSCC_RC_FAILURE;
	}

	event_msg = m_MMGR_ALLOC_MSGELEM;
	if (event_msg == NULL) {
		m_MDS_LOG_ERR("mds_mcm_node_up out of memory\n");
		return NCSCC_RC_FAILURE;
	}
	memset(event_msg, 0, sizeof(MDS_MCM_MSG_ELEM));
	event_msg->type = MDS_EVENT_TYPE;
	event_msg->pri = MDS_SEND_PRIORITY_MEDIUM;

	/* Temp ptr to cbinfo in event_msg */
	cbinfo = &event_msg->info.event.cbinfo;	/* NOTE: Aliased pointer */

	cbinfo->i_op = MDS_CALLBACK_NODE_EVENT;
	cbinfo->i_yr_svc_hdl = local_svc_info->yr_svc_hdl;
	cbinfo->i_yr_svc_id = local_svc_info->svc_id;

	cbinfo->info.node_evt.node_chg = NCSMDS_NODE_UP;

	cbinfo->info.node_evt.node_id = node_id;

	/* Post to mail box If Q Ownership is enabled Else Call user callback */
	if (local_svc_info->q_ownership == TRUE) {

		if ((m_NCS_IPC_SEND(&local_svc_info->q_mbx, event_msg, NCS_IPC_PRIORITY_NORMAL)) != NCSCC_RC_SUCCESS) {
			/* Message Queuing failed */
			m_MMGR_FREE_MSGELEM(event_msg);
			m_MDS_LOG_ERR("SVC Mailbox IPC_SEND : NODE UP EVENT : FAILED\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_node_up");
			return NCSCC_RC_FAILURE;
		} else {
			m_MDS_LOG_INFO("SVC mailbox IPC_SEND : NODE UP EVENT : Success\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_node_up");
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Call user callback */
		status = local_svc_info->cback_ptr(cbinfo);
		m_MMGR_FREE_MSGELEM(event_msg);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : status : mds_mcm_node_up");
	return status;

}

/*********************************************************

  Function NAME: mds_mcm_node_down

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_node_down(MDS_SVC_HDL local_svc_hdl, NODE_ID node_id)

{
	MDS_MCM_MSG_ELEM *event_msg = NULL;
	MDS_SVC_INFO *local_svc_info = NULL;
	NCSMDS_CALLBACK_INFO *cbinfo = NULL;
	uns32 status = NCSCC_RC_SUCCESS;

	/* Get Service info cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_hdl),
						m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						(NCSCONTEXT)&local_svc_info)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR(" SVC doesnt exists, returning from mds_mcm_node_down\n");
		return NCSCC_RC_FAILURE;
	}

	if (0 == local_svc_info->i_node_subscr ) {
		/* Node Subscription Doesn't exist */
		m_MDS_LOG_ERR(" Node subscription doesnt exists, returning from mds_mcm_node_down\n");
		return NCSCC_RC_FAILURE;
        }

	event_msg = m_MMGR_ALLOC_MSGELEM;
	if (event_msg == NULL) {
		m_MDS_LOG_ERR("mds_mcm_node_up out of memory\n");
		return NCSCC_RC_FAILURE;
	}
	memset(event_msg, 0, sizeof(MDS_MCM_MSG_ELEM));
	event_msg->type = MDS_EVENT_TYPE;
	event_msg->pri = MDS_SEND_PRIORITY_MEDIUM;

	/* Temp ptr to cbinfo in event_msg */
	cbinfo = &event_msg->info.event.cbinfo;	/* NOTE: Aliased pointer */

	cbinfo->i_op = MDS_CALLBACK_NODE_EVENT;
	cbinfo->i_yr_svc_hdl = local_svc_info->yr_svc_hdl;
	cbinfo->i_yr_svc_id = local_svc_info->svc_id;

	cbinfo->info.node_evt.node_chg = NCSMDS_NODE_DOWN;

	cbinfo->info.node_evt.node_id = node_id;

	/* Post to mail box If Q Ownership is enabled Else Call user callback */
	if (local_svc_info->q_ownership == TRUE) {

		if ((m_NCS_IPC_SEND(&local_svc_info->q_mbx, event_msg, NCS_IPC_PRIORITY_NORMAL)) != NCSCC_RC_SUCCESS) {
			/* Message Queuing failed */
			m_MMGR_FREE_MSGELEM(event_msg);
			m_MDS_LOG_ERR("SVC Mailbox IPC_SEND : NODE DOWN EVENT : FAILED\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_node_down");
			return NCSCC_RC_FAILURE;
		} else {
			m_MDS_LOG_INFO("SVC mailbox IPC_SEND : NODE DOWN EVENT : Success\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_node_down");
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Call user callback */
		status = local_svc_info->cback_ptr(cbinfo);
		m_MMGR_FREE_MSGELEM(event_msg);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : status : mds_mcm_node_down");
	return status;

}

/*********************************************************

  Function NAME: mds_mcm_vdest_up

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_vdest_up(MDS_VDEST_ID vdest_id, MDS_DEST adest)
{
	V_DEST_RL current_role;
	NCSMDS_ADMOP_INFO chg_role_info;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_vdest_up");

	if (adest == gl_mds_mcm_cb->adest) {	/* Got up from this vdest itself */
		/* Discard */
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_up");
		return NCSCC_RC_SUCCESS;
	}
/* STEP 1: Get the Current role and Dest_hdl of VDEST 
            with id VDEST_ID from LOCAL-VDEST table. */
	mds_vdest_tbl_get_role(vdest_id, &current_role);
/* STEP 2: if (Current Role == Active)
        call dest_chg_role(dest_hdl, New Role = QUIESCED) */

	if (current_role == V_DEST_RL_ACTIVE) {
		/* change role of local vdest to quiesced */
		chg_role_info.i_op = MDS_ADMOP_VDEST_CONFIG;
		chg_role_info.info.vdest_config.i_vdest = vdest_id;
		chg_role_info.info.vdest_config.i_new_role = V_DEST_RL_QUIESCED;
		mds_mcm_vdest_chg_role(&chg_role_info);
	} else {
		/* Do nothing. Each service subscribed will take care
		   of services coming up on that vdest */
	}

	m_MDS_LOG_NOTIFY("MCM_API : vdest_up : Got UP from VDEST id = %d at ADEST <0x%08x, %u>",
			 vdest_id, m_MDS_GET_NODE_ID_FROM_ADEST(adest), m_MDS_GET_PROCESS_ID_FROM_ADEST(adest));

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_vdest_up");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_vdest_down

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_vdest_down(MDS_VDEST_ID vdest_id, MDS_DEST adest)
{
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_user_callback

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_user_event_callback(MDS_SVC_HDL local_svc_hdl, PW_ENV_ID pwe_id, MDS_SVC_ID svc_id,
				  V_DEST_RL role, MDS_VDEST_ID vdest_id, MDS_DEST adest,
				  NCSMDS_CHG event_type,
				  MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type)
{
	uns32 status = NCSCC_RC_SUCCESS;
	MDS_SVC_INFO *local_svc_info = NULL;
	MDS_SUBSCRIPTION_INFO *local_subtn_info = NULL;
	MDS_MCM_MSG_ELEM *event_msg = NULL;
	NCSMDS_CALLBACK_INFO *cbinfo = NULL;
	MDS_AWAIT_DISC_QUEUE *curr_queue_element = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_user_event_callback");

	/* Get Service info cb */
	if (NCSCC_RC_SUCCESS != mds_svc_tbl_get(m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_hdl),
						m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl),
						(NCSCONTEXT)&local_svc_info)) {
		/* Service Doesn't exist */
		m_MDS_LOG_ERR("MDS_SND_RCV: SVC doesnt exists, returning from mds_mcm_user_event_callback=%d\n",
			      svc_id);
		return NCSCC_RC_FAILURE;
	}

	/* Get Subtn info */
	mds_subtn_tbl_get(local_svc_hdl, svc_id, &local_subtn_info);
	/* If this function returns failure, then its invalid state. Handling required */
	if (NULL == local_subtn_info) {
		m_MDS_LOG_ERR("MCM_API:Sub fr svc=%d to =%d doesnt exists, ret fr mds_mcm_user_event_callback\n",
				m_MDS_GET_SVC_ID_FROM_SVC_HDL(local_svc_hdl), svc_id);
		return NCSCC_RC_FAILURE;
	}

	/* If it is up check for subscription timer running or not */
	/* If running raise selection objects of blocking send */
	if (local_subtn_info->tmr_flag == TRUE && (event_type == NCSMDS_UP || event_type == NCSMDS_RED_UP)) {	/* Subscription timer is running */

		curr_queue_element = local_subtn_info->await_disc_queue;

		while (curr_queue_element != NULL) {
			if (curr_queue_element->send_type != MDS_SENDTYPE_BCAST &&
			    curr_queue_element->send_type != MDS_SENDTYPE_RBCAST) {
				/* Condition to raise selection object for pending SEND is not checked properly in mds_c_api.c */
				if (vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {	/* We got UP from ADEST so check if any send to svc on this ADEST is remaining */

					if (curr_queue_element->vdest == m_VDEST_ID_FOR_ADEST_ENTRY) {	/* Blocked send is to send to ADEST */

						if (adest == curr_queue_element->adest) {	/* Raise selection object for this entry as we got up from that adest */
							m_NCS_SEL_OBJ_IND(curr_queue_element->sel_obj);
							m_MDS_LOG_INFO
							    ("MCM_API : event_callback : Raising SEL_OBJ for Send to ADEST <0x%08x, %u>",
							     curr_queue_element->vdest,
							     m_MDS_GET_NODE_ID_FROM_ADEST(curr_queue_element->adest),
							     m_MDS_GET_PROCESS_ID_FROM_ADEST(curr_queue_element->
											     adest));
						}
					}
				} else {	/* We got up for VDEST so check if any send to svc on this VDEST is remaining */

					if (curr_queue_element->adest == 0 && curr_queue_element->vdest == vdest_id && event_type == NCSMDS_UP) {	/* Blocked send is send to VDEST and we got atleast one ACTIVE */
						m_NCS_SEL_OBJ_IND(curr_queue_element->sel_obj);
						m_MDS_LOG_INFO
						    ("MCM_API : event_callback : Raising SEL_OBJ for Send to VDEST id = %d on ADEST <0x%08x, %u>",
						     curr_queue_element->vdest,
						     m_MDS_GET_NODE_ID_FROM_ADEST(curr_queue_element->adest),
						     m_MDS_GET_PROCESS_ID_FROM_ADEST(curr_queue_element->adest));
					}
				}
			}
			/* Increment current_queue_element pointer */
			curr_queue_element = curr_queue_element->next_msg;
		}
	}

	/* Check whether subscription is Implicit or Explicit */
	if (local_subtn_info->subtn_type == MDS_SUBTN_IMPLICIT) {
		/* Dont send event to user */
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_user_event_callback");
		return NCSCC_RC_SUCCESS;
	}

	/*  Fix for mem-leak found during testing: Allocate and set 
	 ** struct to send message-element to client (only for non-implicit 
	 ** callbacks)
	 */
	event_msg = m_MMGR_ALLOC_MSGELEM;
	if (event_msg == NULL) {
		m_MDS_LOG_ERR("Subscription callback processing:Out of memory\n");
		return NCSCC_RC_FAILURE;
	}
	memset(event_msg, 0, sizeof(MDS_MCM_MSG_ELEM));
	event_msg->type = MDS_EVENT_TYPE;
	event_msg->pri = MDS_SEND_PRIORITY_MEDIUM;

	/* Temp ptr to cbinfo in event_msg */
	cbinfo = &event_msg->info.event.cbinfo;	/* NOTE: Aliased pointer */

	cbinfo->i_op = MDS_CALLBACK_SVC_EVENT;
	cbinfo->i_yr_svc_hdl = local_svc_info->yr_svc_hdl;
	cbinfo->i_yr_svc_id = local_svc_info->svc_id;

	cbinfo->info.svc_evt.i_change = event_type;

	if (vdest_id == m_VDEST_ID_FOR_ADEST_ENTRY) {
		/* Service is on remote ADEST */
		cbinfo->info.svc_evt.i_dest = adest;
		cbinfo->info.svc_evt.i_anc = 0;	/* anchor same as adest */
	} else {
		/* Service is on remote VDEST */
		cbinfo->info.svc_evt.i_dest = (MDS_DEST)vdest_id;

		if (event_type == NCSMDS_RED_UP || event_type == NCSMDS_RED_DOWN || event_type == NCSMDS_CHG_ROLE) {
			cbinfo->info.svc_evt.i_anc = adest;	/* anchor same as adest */
		} else {
			cbinfo->info.svc_evt.i_anc = 0;
		}
	}

	cbinfo->info.svc_evt.i_role = role;
	cbinfo->info.svc_evt.i_node_id = m_MDS_GET_NODE_ID_FROM_ADEST(adest);
	cbinfo->info.svc_evt.i_pwe_id = pwe_id;
	cbinfo->info.svc_evt.i_svc_id = svc_id;
	cbinfo->info.svc_evt.i_your_id = local_svc_info->svc_id;

	cbinfo->info.svc_evt.svc_pwe_hdl = (MDS_HDL)m_MDS_GET_PWE_HDL_FROM_SVC_HDL(local_svc_info->svc_hdl);
	cbinfo->info.svc_evt.i_rem_svc_pvt_ver = svc_sub_part_ver;

	/* Post to mail box If Q Ownership is enabled Else Call user callback */
	if (local_svc_info->q_ownership == TRUE) {

		if ((m_NCS_IPC_SEND(&local_svc_info->q_mbx, event_msg, NCS_IPC_PRIORITY_NORMAL)) != NCSCC_RC_SUCCESS) {
			/* Message Queuing failed */
			m_MMGR_FREE_MSGELEM(event_msg);
			m_MDS_LOG_ERR("SVC Mailbox IPC_SEND : SVC EVENT : FAILED\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_user_event_callback");
			return NCSCC_RC_FAILURE;
		} else {
			m_MDS_LOG_INFO("SVC mailbox IPC_SEND : SVC EVENT : Success\n");
			m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_user_event_callback");
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Call user callback */
		status = local_svc_info->cback_ptr(cbinfo);
		m_MMGR_FREE_MSGELEM(event_msg);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : status : mds_mcm_user_event_callback");
	return status;
}

/*********************************************************

  Function NAME: mds_mcm_quiesced_tmr_expiry

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_quiesced_tmr_expiry(MDS_VDEST_ID vdest_id)
{
	uns32 status = NCSCC_RC_SUCCESS;
	NCSMDS_CALLBACK_INFO *cbinfo;
	MDS_SVC_INFO *local_svc_info = NULL;
	MDS_MCM_MSG_ELEM *event_msg = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_quiesced_tmr_expiry");

	m_MDS_LOG_INFO("MCM_API : quieseced_tmr expired for VDEST id = %d", vdest_id);

	/* Update vdest role to Standby */
	mds_vdest_tbl_update_role(vdest_id, V_DEST_RL_STANDBY, FALSE);
	/* Turn off timer done in update_role */

	/* Send Quiesced ack event to all services on this vdest */

	status = mds_svc_tbl_getnext_on_vdest(vdest_id, 0, &local_svc_info);
	while (status == NCSCC_RC_SUCCESS) {
		event_msg = m_MMGR_ALLOC_MSGELEM;
		memset(event_msg, 0, sizeof(MDS_MCM_MSG_ELEM));

		event_msg->type = MDS_EVENT_TYPE;
		event_msg->pri = MDS_SEND_PRIORITY_MEDIUM;

		cbinfo = &event_msg->info.event.cbinfo;
		cbinfo->i_yr_svc_hdl = local_svc_info->yr_svc_hdl;
		cbinfo->i_yr_svc_id = local_svc_info->svc_id;
		cbinfo->i_op = MDS_CALLBACK_QUIESCED_ACK;
		cbinfo->info.quiesced_ack.i_dummy = 1;	/* dummy */

		/* Post to mail box If Q Ownership is enabled Else Call user callback */
		if (local_svc_info->q_ownership == TRUE) {

			if ((m_NCS_IPC_SEND(&local_svc_info->q_mbx, event_msg, NCS_IPC_PRIORITY_NORMAL)) !=
			    NCSCC_RC_SUCCESS) {
				/* Message Queuing failed */
				m_MMGR_FREE_MSGELEM(event_msg);
				m_MDS_LOG_ERR("SVC Mailbox IPC_SEND : Quiesced Ack : FAILED\n");
				m_MDS_LOG_INFO("MCM_API : Entering : mds_mcm_quiesced_tmr_expiry");
				return NCSCC_RC_FAILURE;
			} else {
				m_MDS_LOG_DBG("SVC mailbox IPC_SEND : Quiesced Ack : SUCCESS\n");
			}
		} else {
			/* Call user callback */
			status = local_svc_info->cback_ptr(cbinfo);
			m_MMGR_FREE_MSGELEM(event_msg);
		}
		status = mds_svc_tbl_getnext_on_vdest(vdest_id, local_svc_info->svc_hdl, &local_svc_info);
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_quiesced_tmr_expiry");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_subscription_tmr_expiry

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_subscription_tmr_expiry(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id)
{
	MDS_SUBSCRIPTION_INFO *subtn_info = NULL;
	MDS_AWAIT_DISC_QUEUE *tmp_await_active_queue = NULL, *bk_ptr = NULL;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_subscription_tmr_expiry");

	m_MDS_LOG_INFO("MCM_API : subscription_tmr expired for SVC id = %d Subscribed to SVC id = %d",
		       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), sub_svc_id);

	mds_subtn_tbl_get(svc_hdl, sub_svc_id, &subtn_info);

	if (subtn_info == NULL) {
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_subscription_tmr_expiry");
		return NCSCC_RC_SUCCESS;
	}

	/* Turn timer flag off */
	subtn_info->tmr_flag = FALSE;
	/* Point tmr_req_info to NULL, freeding will be done in expiry function */
	subtn_info->tmr_req_info = NULL;
	/* Destroy timer as it will never be started again */
	m_NCS_TMR_DESTROY(subtn_info->discovery_tmr);
	/* Destroying Handle and freeing tmr_req_info done in expiry function itself */

	tmp_await_active_queue = subtn_info->await_disc_queue;

	if (tmp_await_active_queue == NULL) {
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_subscription_tmr_expiry");
		return NCSCC_RC_SUCCESS;
	}

	if (tmp_await_active_queue->next_msg == NULL) {
		m_NCS_SEL_OBJ_IND(tmp_await_active_queue->sel_obj);
		m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_subscription_tmr_expiry");
		return NCSCC_RC_SUCCESS;
	}

	while (tmp_await_active_queue != NULL) {
		bk_ptr = tmp_await_active_queue;
		tmp_await_active_queue = tmp_await_active_queue->next_msg;

		/* Raise selection object */
		m_NCS_SEL_OBJ_IND(bk_ptr->sel_obj);
		/* Memory for this will be get freed by awaiting request */

	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_subscription_tmr_expiry");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_await_active_tmr_expiry

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mcm_await_active_tmr_expiry(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id, MDS_VDEST_ID vdest_id)
{
	MDS_SUBSCRIPTION_RESULTS_INFO *active_subtn_result_info;
	MDS_DEST active_adest;
	NCS_BOOL tmr_running;
	uns32 status = NCSCC_RC_SUCCESS;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_await_active_tmr_expiry");

	m_MDS_LOG_INFO("MCM_API : await_active_tmr expired for SVC id = %d Subscribed to SVC id = %d on VDEST id = %d",
		       m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), sub_svc_id, vdest_id);

	mds_subtn_res_tbl_get(svc_hdl, sub_svc_id, vdest_id, &active_adest,
			      &tmr_running, &active_subtn_result_info, TRUE);
	/* Delete all pending messages */
	mds_await_active_tbl_del(active_subtn_result_info->info.active_vdest.active_route_info->await_active_queue);

	/* Call user callback DOWN */
	status = mds_mcm_user_event_callback(svc_hdl, m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_hdl),
					     sub_svc_id, V_DEST_RL_STANDBY,
					     vdest_id, 0, NCSMDS_DOWN, active_subtn_result_info->rem_svc_sub_part_ver,
					     MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED);
	if (status != NCSCC_RC_SUCCESS) {
		/* Callback failure */
		m_MDS_LOG_ERR
		    ("MCM_API : await_active_tmr_expiry : DOWN Callback Failure for SVC id = %d subscribed to SVC id = %d on VDEST id = %d",
		     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_hdl), sub_svc_id,
		     vdest_id);
	}

	m_MDS_LOG_NOTIFY
	    ("MCM_API : svc_down : await_active_tmr_expiry : SVC id = %d on DEST id = %d got DOWN for SVC id = %d on VDEST id = %d",
	     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_hdl), sub_svc_id, vdest_id);

	/* Destroy timer */
	m_NCS_TMR_DESTROY(active_subtn_result_info->info.active_vdest.active_route_info->await_active_tmr);

	/* Freeing Handle and tmr_req_info done in expiry function itself */

	/* Delete Active entry from Subscription result tree */
	m_MMGR_FREE_SUBTN_ACTIVE_RESULT_INFO(active_subtn_result_info->info.active_vdest.active_route_info);

	/* Delete subscription entry from tree */
	ncs_patricia_tree_del(&gl_mds_mcm_cb->subtn_results, (NCS_PATRICIA_NODE *)active_subtn_result_info);

	/* Delete memory for active entry */
	m_MMGR_FREE_SUBTN_RESULT_INFO(active_subtn_result_info);

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_await_active_tmr_expiry");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_subtn_add

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
uns32 mds_mcm_subtn_add(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, NCSMDS_SCOPE_TYPE scope,
			MDS_VIEW view, MDS_SUBTN_TYPE subtn_type)
{
	MDS_SUBTN_REF_VAL subtn_ref_val = 0;
	uns32 status = NCSCC_RC_SUCCESS;

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_subtn_add");

	/* Add in Subscription Tabel */
	mds_subtn_tbl_add(svc_hdl, subscr_svc_id, scope, view, subtn_type);
	/*  STEP 2.c: Call mdtm_svc_subscribe(sub_svc_id, scope, svc_hdl) */
	status = mds_mdtm_svc_subscribe((PW_ENV_ID)m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_hdl),
					subscr_svc_id, scope, svc_hdl, &subtn_ref_val);
	if (status != NCSCC_RC_SUCCESS) {
		/* MDTM is unable to subscribe */

		/* Remove Subscription info from MCM database */
		mds_subtn_tbl_del(svc_hdl, subscr_svc_id);

		m_MDS_LOG_ERR
		    ("MCM_API : mcm_subtn_add : Can't Subscribe from SVC id = %c on DEST = %d to SVC id = %d : MDTM Returned Failure",
		     m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl), m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_hdl), subscr_svc_id);
		m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_subtn_add");
		return NCSCC_RC_FAILURE;
	}

	/* Update subscription handle to subscription table */
	mds_subtn_tbl_update_ref_hdl(svc_hdl, subscr_svc_id, subtn_ref_val);

	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_subtn_add");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_validate_scope

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
uns32 mds_mcm_validate_scope(NCSMDS_SCOPE_TYPE local_scope, NCSMDS_SCOPE_TYPE remote_scope,
			     MDS_DEST remote_adest, MDS_SVC_ID remote_svc_id, NCS_BOOL my_pcon)
{

	m_MDS_LOG_DBG("MCM_API : Entering : mds_mcm_validate_scope");

	switch (local_scope) {
	case NCSMDS_SCOPE_INTRANODE:

		if (m_MDS_GET_NODE_ID_FROM_ADEST(remote_adest) != m_MDS_GET_NODE_ID_FROM_ADEST(m_MDS_GET_ADEST)) {
			m_MDS_LOG_NOTIFY("MCM_API : svc_up : Node Scope Mismatch for SVC id = %d", remote_svc_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_validate_scope");
			return NCSCC_RC_FAILURE;
		}
		break;

	case NCSMDS_SCOPE_NONE:

		if (remote_scope == NCSMDS_SCOPE_INTRANODE &&
		    m_MDS_GET_NODE_ID_FROM_ADEST(remote_adest) != m_MDS_GET_NODE_ID_FROM_ADEST(m_MDS_GET_ADEST)) {
			m_MDS_LOG_NOTIFY
			    ("MCM_API : svc_up : NONE or CHASSIS Scope Mismatch (remote scope = NODE) for SVC id = %d",
			     remote_svc_id);
			m_MDS_LOG_DBG("MCM_API : Leaving : F : mds_mcm_validate_scope");
			return NCSCC_RC_FAILURE;
		}
		break;

	case NCSMDS_SCOPE_INTRACHASSIS:
		/* This is not supported as of now */
		break;
	default:
		/* Intrapcon is no more supported */
		break;
	}
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_mcm_validate_scope");
	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_adm_get_adest_hdl

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
uns32 mds_adm_get_adest_hdl(void)
{
	m_MDS_LOG_DBG("MCM_API : Entering : mds_adm_get_adest_hdl");
	m_MDS_LOG_DBG("MCM_API : Leaving : S : mds_adm_get_adest_hdl");
	return m_ADEST_HDL;
}

/*********************************************************

  Function NAME: ncs_get_internal_vdest_id_from_mds_dest

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
MDS_VDEST_ID ncs_get_internal_vdest_id_from_mds_dest(MDS_DEST mdsdest)
{
	m_MDS_LOG_DBG("MCM_API : Entering : ncs_get_internal_vdest_id_from_mds_dest");
	m_MDS_LOG_DBG("MCM_API : Leaving : s : ncs_get_internal_vdest_id_from_mds_dest");
	return (MDS_VDEST_ID)(m_NCS_NODE_ID_FROM_MDS_DEST(mdsdest) == 0 ? mdsdest : m_VDEST_ID_FOR_ADEST_ENTRY);
}

/*********************************************************

  Function NAME: mds_mcm_init

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
uns32 mds_mcm_init(void)
{
	NCS_PATRICIA_PARAMS pat_tree_params;
	MDS_VDEST_INFO *vdest_for_adest_node;

	/* STEP 1: Initialize MCM-CB. */
	gl_mds_mcm_cb = m_MMGR_ALLOC_MCM_CB;

	/* VDEST TREE */
	memset(&pat_tree_params, 0, sizeof(NCS_PATRICIA_PARAMS));
	pat_tree_params.key_size = sizeof(MDS_VDEST_ID);
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&gl_mds_mcm_cb->vdest_list, &pat_tree_params)) {
		m_MDS_LOG_ERR("MCM_API : patricia_tree_init: vdest :failure, L mds_mcm_init");
		return NCSCC_RC_FAILURE;
	}

	/* SERVICE TREE */
	memset(&pat_tree_params, 0, sizeof(NCS_PATRICIA_PARAMS));
	pat_tree_params.key_size = sizeof(MDS_SVC_HDL);
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&gl_mds_mcm_cb->svc_list, &pat_tree_params)) {
		m_MDS_LOG_ERR("MCM_API : patricia_tree_init:service :failure, L mds_mcm_init");
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_destroy(&gl_mds_mcm_cb->vdest_list)) {
			m_MDS_LOG_ERR("MCM_API : patricia_tree_destroy: service :failure, L mds_mcm_init");
		}
		return NCSCC_RC_FAILURE;
	}

	/* SUBSCRIPTION RESULT TREE */
	memset(&pat_tree_params, 0, sizeof(NCS_PATRICIA_PARAMS));
	pat_tree_params.key_size = sizeof(MDS_SUBSCRIPTION_RESULTS_KEY);
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&gl_mds_mcm_cb->subtn_results, &pat_tree_params)) {
		m_MDS_LOG_ERR("MCM_API : patricia_tree_init: subscription: failure, L mds_mcm_init");
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_destroy(&gl_mds_mcm_cb->svc_list)) {
			m_MDS_LOG_ERR("MCM_API : patricia_tree_destroy: service :failure, L mds_mcm_init");
		}
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_destroy(&gl_mds_mcm_cb->vdest_list)) {
			m_MDS_LOG_ERR("MCM_API : patricia_tree_destroy: vdest :failure, L mds_mcm_init");
		}
		return NCSCC_RC_FAILURE;
	}

	/* Add VDEST for ADEST entry in tree */
	vdest_for_adest_node = m_MMGR_ALLOC_VDEST_INFO;
	memset(vdest_for_adest_node, 0, sizeof(MDS_VDEST_INFO));

	vdest_for_adest_node->vdest_id = m_VDEST_ID_FOR_ADEST_ENTRY;
	vdest_for_adest_node->policy = NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN;
	vdest_for_adest_node->role = V_DEST_RL_ACTIVE;

	vdest_for_adest_node->node.key_info = (uns8 *)&vdest_for_adest_node->vdest_id;

	ncs_patricia_tree_add(&gl_mds_mcm_cb->vdest_list, (NCS_PATRICIA_NODE *)vdest_for_adest_node);

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mds_mcm_destroy

  DESCRIPTION:

  ARGUMENTS: 

  RETURNS:  Adest Hdl

*********************************************************/
uns32 mds_mcm_destroy(void)
{
	/* cleanup mcm database */
	mds_mcm_cleanup();

	/* destroy all patricia trees */

	/* SUBSCRIPTION RESULT TREE */
	ncs_patricia_tree_destroy(&gl_mds_mcm_cb->subtn_results);

	/* SERVICE TREE */
	ncs_patricia_tree_destroy(&gl_mds_mcm_cb->svc_list);

	/* VDEST TREE */
	ncs_patricia_tree_destroy(&gl_mds_mcm_cb->vdest_list);

	/* Free MCM control block */
	m_MMGR_FREE_MCM_CB(gl_mds_mcm_cb);

	gl_mds_mcm_cb = NULL;

	return NCSCC_RC_SUCCESS;
}
