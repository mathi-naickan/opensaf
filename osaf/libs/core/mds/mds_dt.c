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

#include "mds_dt_tipc.h"
/* #include "mds_dt_blah.h" */

#define MDS_OVER_TIPC 1

/*********************************************************

  Function NAME: mds_mdtm_init

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_init(NODE_ID node_id, uns32 *mds_tipc_ref)
{

	/* INit all transports */
#if MDS_OVER_TIPC
	return mdtm_tipc_init(node_id, mds_tipc_ref);
#endif

}

/*********************************************************

  Function NAME: mds_mdtm_destroy

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Destroying the MDTM Module*/
uns32 mds_mdtm_destroy(void)
{
	/* Destroy all transports */
#if MDS_OVER_TIPC
	return mdtm_tipc_destroy();
#endif

}

/*********************************************************

  Function NAME: mds_mdtm_svc_subscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Subscribing to services */
uns32 mds_mdtm_svc_subscribe(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
			     MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
	/* We will need to subscribe on all domains.
	   Not just over TIPC
	 */
#if MDS_OVER_TIPC
	return mds_mdtm_svc_subscribe_tipc(pwe_id, svc_id, install_scope, svc_hdl, subtn_ref_val);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_svc_unsubscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Unsubscribing to services */
uns32 mds_mdtm_svc_unsubscribe(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, 
		NCSMDS_SCOPE_TYPE install_scope, MDS_SUBTN_REF_VAL subtn_ref_val)
{
	/* We will need to subscribe on all domains.
	   Not just over TIPC
	 */
#if MDS_OVER_TIPC
	return mds_mdtm_svc_unsubscribe_tipc(pwe_id, svc_id, install_scope, subtn_ref_val);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_svc_install

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Installing services */
uns32 mds_mdtm_svc_install(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
			   V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
			   MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	/* We will need to subscribe on all domains.
	   Not just over TIPC
	 */
#if MDS_OVER_TIPC
	return mds_mdtm_svc_install_tipc(pwe_id, svc_id, install_scope, role, vdest_id, vdest_policy, mds_svc_pvt_ver);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_svc_uninstall

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
/* Uninstalling the services */
uns32 mds_mdtm_svc_uninstall(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
			     V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
			     MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver)
{
	/* We will need to subscribe on all domains.
	   Not just over TIPC
	 */
#if MDS_OVER_TIPC
	return mds_mdtm_svc_uninstall_tipc(pwe_id, svc_id, install_scope, role, vdest_id, vdest_policy,
					   mds_svc_pvt_ver);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_install

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_vdest_install(MDS_VDEST_ID vdest_id)
{
#if MDS_OVER_TIPC
	return mds_mdtm_vdest_install_tipc(vdest_id);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_uninstall

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_vdest_uninstall(MDS_VDEST_ID vdest_id)
{
#if MDS_OVER_TIPC
	return mds_mdtm_vdest_uninstall_tipc(vdest_id);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_subscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_vdest_subscribe(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
#if MDS_OVER_TIPC
	return mds_mdtm_vdest_subscribe_tipc(vdest_id, subtn_ref_val);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_vdest_unsubscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_vdest_unsubscribe(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val)
{
#if MDS_OVER_TIPC
	return mds_mdtm_vdest_unsubscribe_tipc(vdest_id, subtn_ref_val);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_tx_hdl_register

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_tx_hdl_register(MDS_DEST adest)
{
#if MDS_OVER_TIPC
	return mds_mdtm_tx_hdl_register_tipc(adest);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_tx_hdl_unregister

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_tx_hdl_unregister(MDS_DEST adest)
{
#if MDS_OVER_TIPC
	return mds_mdtm_tx_hdl_unregister_tipc(adest);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_send

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_send(MDTM_SEND_REQ *req)
{
#if MDS_OVER_TIPC
	return mds_mdtm_send_tipc(req);
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_node_subscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_node_subscribe(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val)
{
#if MDS_OVER_TIPC
	return mds_mdtm_node_subscribe_tipc(svc_hdl, subtn_ref_val); 
#endif
}

/*********************************************************

  Function NAME: mds_mdtm_node_unsubscribe

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mds_mdtm_node_unsubscribe(MDS_SUBTN_REF_VAL subtn_ref_val)
{
#if MDS_OVER_TIPC
	return mds_mdtm_node_unsubscribe_tipc(subtn_ref_val); 
#endif
}
