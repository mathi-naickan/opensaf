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

  DESCRIPTION:

  This file contains all library creation APIs for the Mbessage Based 
  Checkpointing Service (MBCSV).

  ..............................................................................

  FUNCTIONS INCLUDED in this module:

  PUBLIC:
  mbcsv_lib_req                  - SE API to create MBCSV.

  PRIVATE:
  mbcsv_lib_init                 - Create MBCSV service.
  mbcsv_lib_destroy              - Distroy MBCSV service.
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "mbcsv.h"

MBCSV_CB mbcsv_cb;

/****************************************************************************
 * PROCEDURE: mbcsv_lib_req
 *
 * Description   : This is the NCS SE API which is used to init/destroy or 
 *                 Create/destroy MBCSV. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : SA_AIS_OK/Error code..
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mbcsv_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t res = SA_AIS_OK;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		res = mbcsv_lib_init(req_info);
		break;

	case NCS_LIB_REQ_DESTROY:
		res = mbcsv_lib_destroy();
		break;

	default:
		TRACE_4("Lib init request failed: Bad operation type supplied");
		res = SA_AIS_ERR_INVALID_PARAM;
		break;
	}
	TRACE_LEAVE();
	return (res);
}

/****************************************************************************
 * PROCEDURE: mbcsv_lib_init
 *
 * Description   : This is the function which initalize the mbcsv libarary.
 *                 This function creates an global lock, creates MBCSV linked 
 *                 list, etc.
 *
 * Arguments     : req_info - Request info.
 *
 * Return Values : SA_AIS_OK/Failure code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mbcsv_lib_init(NCS_LIB_REQ_INFO *req_info)
{
	NCS_PATRICIA_PARAMS pt_params;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	if (mbcsv_cb.created == true) {
		TRACE_LEAVE2("Lib init request failed: MBCA already created");
		return SA_AIS_ERR_INIT;
	}

	/*
	 * Create global lock 
	 */
	m_NCS_LOCK_INIT(&mbcsv_cb.global_lock);

	/* 
	 * Create patricia tree for the MBCA registration instance 
	 */
	pt_params.key_size = sizeof(uint32_t);

	if (ncs_patricia_tree_init(&mbcsv_cb.reg_list, &pt_params) != NCSCC_RC_SUCCESS) {
		TRACE_4("pat tree init failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err1;
	}

	if (NCSCC_RC_SUCCESS != mbcsv_initialize_mbx_list()) {
		TRACE_4("pat tree init for mailbox failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err2;
	}

	/* 
	 * Create patricia tree for the peer list 
	 */
	if (mbcsv_initialize_peer_list() != NCSCC_RC_SUCCESS) {
		TRACE_4("pat tree init for peer list failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err3;
	}

	mbcsv_cb.created = true;

	return rc;

	/* Handle Different Error Situations */
 err3:
	ncs_patricia_tree_destroy(&mbcsv_cb.mbx_list);
	m_NCS_LOCK_DESTROY(&mbcsv_cb.mbx_list_lock);
 err2:
	ncs_patricia_tree_destroy(&mbcsv_cb.reg_list);
 err1:
	m_NCS_LOCK_DESTROY(&mbcsv_cb.global_lock);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * PROCEDURE     : mbcsv_lib_destroy
 *
 * Description   : This is the function which destroys the MBCSV.  
 *
 * Arguments     : None.
 *
 * Return Values : SA_AIS_OK/Failure code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mbcsv_lib_destroy(void)
{
	MBCSV_REG *mbc_reg;
	SS_SVC_ID svc_id = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	if (mbcsv_cb.created == false) {
		TRACE_LEAVE2("Lib destroy request failed: Create MBCA before destroying");
		return SA_AIS_ERR_EXIST;
	}

	m_NCS_LOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);

	mbcsv_cb.created = false;
	/* 
	 * Walk through MBCSv reg list and destroy all the registration instances.
	 */
	while (NULL != (mbc_reg = (MBCSV_REG *)ncs_patricia_tree_getnext(&mbcsv_cb.reg_list, (const uint8_t *)&svc_id))) {
		svc_id = mbc_reg->svc_id;

		if (NCSCC_RC_SUCCESS != mbcsv_rmv_reg_inst((MBCSV_REG *)&mbcsv_cb.reg_list, mbc_reg)) {
			/* Not required to return for failure, log the err message and go 
			   ahead with cleanup */
			TRACE_4("Failed to remove this service instance:%u", mbc_reg->svc_id);
		}
	}

	ncs_patricia_tree_destroy(&mbcsv_cb.reg_list);

	/*
	 * Call function which will destroy and free all the entries of the peer list.
	 */
	mbcsv_destroy_peer_list();

	/*
	 * Call function which will destroy mail box list.
	 */
	mbcsv_destroy_mbx_list();

	m_NCS_UNLOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);
	m_NCS_LOCK_DESTROY(&mbcsv_cb.global_lock);

	TRACE_LEAVE();
	return rc;
}
