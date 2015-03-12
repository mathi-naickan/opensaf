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

  $Source:  $

..............................................................................

  DESCRIPTION: This file contains all the service related version 2 apis
               for MDS

******************************************************************************
*/

#include <configmake.h>
#include "mds_papi.h"
#include "mds_core.h"
#include "mds_log.h"
#include "mds_pvt.h"
#include "ncs_main_papi.h"
#include "ncssysf_mem.h"
#include "osaf_utility.h"

/**************************************************************************
 * SVC_NAME  of Service : A well known name that a service 
 ***************************************************************************/
const char ncsmds_svc_names[NCSMDS_SVC_ID_NCSMAX][MAX_SVC_NAME_LEN] =
{
        "UNKNOWN",
        "DTS",
        "DTA",
        "GLA",
        "GLND",
        "GLD",
        "VDA",
        "EDS",
        "EDA",
        "MQA",
        "MQND",
        "MQD",
        "AVD",
        "AVND",
        "AVA",
        "CLA",
        "CPD",
        "CPND",
        "CPA",
        "MBCSV",
        "LGS",
        "LGA",
        "AVND_CNTLR",
        "GFM",
        "IMMD",
        "IMMND",
        "IMMA_OM",
        "IMMA_OI",
        "NTFS",
        "NTFA",
        "SMFD",
        "SMFND",
        "SMFA",
        "RDE",
        "CLMS",
        "CLMA",
        "CLMNA",
        "PLMS",
        "PLMS_HRB",
        "PLMA",
};

/**************************************************************************
 * INTERNAL SVC_NAME  of Service : A User defined internal name that a service 
 ***************************************************************************/
const char ncsmds_internal_svc_names[NCSMDS_SVC_ID_INTMAX - NCSMDS_SVC_ID_INTERNAL_MIN][MAX_SVC_NAME_LEN] =
{
	"INTERNAL",
};

/**************************************************************************
 * EXTERNAL SVC_NAME  of Service : A User defined external name that a service
 ***************************************************************************/
const char ncsmds_external_svc_names[NCSMDS_SVC_ID_EXTMAX - NCSMDS_SVC_ID_EXTERNAL_MIN][MAX_SVC_NAME_LEN] =
{
	"EXTERNAL",
};

const char *get_svc_names(int svc_id) {

	if ( svc_id < NCSMDS_SVC_ID_INTERNAL_MIN)
	{
		if (svc_id < NCSMDS_SVC_ID_NCSMAX) 
			return ncsmds_svc_names[svc_id];
		else
			return ncsmds_svc_names[0];
	}
	else if ((svc_id >= NCSMDS_SVC_ID_INTERNAL_MIN) && (svc_id < NCSMDS_SVC_ID_EXTERNAL_MIN)) {
		if ( svc_id < NCSMDS_SVC_ID_INTMAX)	
			return ncsmds_internal_svc_names[svc_id - NCSMDS_SVC_ID_INTERNAL_MIN]; 
		else
			return	ncsmds_internal_svc_names[0];
	} else if ((svc_id >= NCSMDS_SVC_ID_EXTERNAL_MIN) && (svc_id <=  NCSMDS_MAX_SVCS)) {
		if ( svc_id < NCSMDS_SVC_ID_EXTMAX)
			return ncsmds_external_svc_names[svc_id - NCSMDS_SVC_ID_EXTERNAL_MIN];
		else
			return ncsmds_external_svc_names[0];
	}
 
	return ncsmds_svc_names[0]; 
} 

/****************************************************************************
 *
 * Function Name: ncsmds_api
 *
 * Purpose:       This API performs all the services requested by an
 *                external service.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

uint32_t ncsmds_api(NCSMDS_INFO *svc_to_mds_info)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	if (svc_to_mds_info == NULL) {
		m_MDS_LOG_ERR("MDS_PAPI : Input svc_to_mds_info = NULL in ncsmds_api()");
		return NCSCC_RC_FAILURE;
	}

	osaf_mutex_lock_ordie(&gl_mds_library_mutex);
	if (gl_mds_mcm_cb == NULL) {
		m_MDS_LOG_ERR("MDS_PAPI : ncsmds_api() : MDS is not initialized gl_mds_mcm_cb = NULL ");
		osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
		return NCSCC_RC_FAILURE;
	}

	/* Vailidate pwe hdl */
	if (svc_to_mds_info->i_op == MDS_SEND || svc_to_mds_info->i_op == MDS_DIRECT_SEND) {
		/* Don't validate pwe hdl */
	} else {
		status = mds_validate_pwe_hdl((MDS_PWE_HDL)svc_to_mds_info->i_mds_hdl);
		if (status == NCSCC_RC_FAILURE) {
			m_MDS_LOG_ERR("MDS_PAPI : Invalid pwe hdl in ncsmds_api()");
			osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
			return NCSCC_RC_FAILURE;
		}
	}

	switch (svc_to_mds_info->i_op) {
	case MDS_INSTALL:
		status = mds_mcm_svc_install(svc_to_mds_info);
		break;

	case MDS_UNINSTALL:
		status = mds_mcm_svc_uninstall(svc_to_mds_info);
		break;

	case MDS_SUBSCRIBE:
	case MDS_RED_SUBSCRIBE:
		status = mds_mcm_svc_subscribe(svc_to_mds_info);
		break;

	case MDS_CANCEL:
		status = mds_mcm_svc_unsubscribe(svc_to_mds_info);
		break;
/*
    case MDS_SYS_SUBSCRIBE:
        status = mds_sys_subscribe(svc_to_mds_info);
        break;
*/
	case MDS_SEND:
	case MDS_DIRECT_SEND:
		status = mds_send(svc_to_mds_info);
		break;

	case MDS_RETRIEVE:
		status = mds_retrieve(svc_to_mds_info);
		break;

	case MDS_QUERY_DEST:
		status = mds_mcm_dest_query(svc_to_mds_info);
		break;

	case MDS_QUERY_PWE:
		status = mds_mcm_pwe_query(svc_to_mds_info);
		break;

	case MDS_NODE_SUBSCRIBE:
		status = mds_mcm_node_subscribe(svc_to_mds_info);
		break;

	case MDS_NODE_UNSUBSCRIBE:
		status = mds_mcm_node_unsubscribe(svc_to_mds_info);
		break;

	default:
		m_MDS_LOG_ERR("MDS_PAPI : API Option Unsupported in ncsmds_api()");
		status = NCSCC_RC_FAILURE;
		break;
	}

	osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
	return status;
}

/****************************************************************************
 *
 * Function Name: ncsmds_adm_api
 *
 * Purpose:       This API performs all the administrative services
 *                requested by an administrator.
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/

uint32_t ncsmds_adm_api(NCSMDS_ADMOP_INFO *mds_adm)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	if (mds_adm == NULL) {
		m_MDS_LOG_ERR("MDS_PAPI : Invalid Input mds_adm = NULL in ncsmds_adm_api()");
		return NCSCC_RC_FAILURE;
	}
	osaf_mutex_lock_ordie(&gl_mds_library_mutex);
	if (gl_mds_mcm_cb == NULL) {
		m_MDS_LOG_ERR("MDS_PAPI : ncsmds_adm_api() : MDS is not initialized gl_mds_mcm_cb = NULL ");
		osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
		return NCSCC_RC_FAILURE;
	}

	switch (mds_adm->i_op) {
	case MDS_ADMOP_VDEST_CREATE:
		status = mds_mcm_vdest_create(mds_adm);
		break;

	case MDS_ADMOP_VDEST_CONFIG:
		status = mds_mcm_vdest_chg_role(mds_adm);
		break;

	case MDS_ADMOP_VDEST_DESTROY:
		status = mds_mcm_vdest_destroy(mds_adm);
		break;

	case MDS_ADMOP_VDEST_QUERY:
		status = mds_mcm_vdest_query(mds_adm);
		break;

	case MDS_ADMOP_PWE_CREATE:
		status = mds_mcm_pwe_create(mds_adm);
		break;

	case MDS_ADMOP_PWE_DESTROY:
		status = mds_mcm_pwe_destroy(mds_adm);
		break;

	case MDS_ADMOP_PWE_QUERY:
		status = mds_mcm_adm_pwe_query(mds_adm);
		break;

	default:
		m_MDS_LOG_ERR("MDS_PAPI : API Option Unsupported in ncsmds_adm_api()");
		status = NCSCC_RC_FAILURE;
		break;
	}

	osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
	return status;
}

/****************************************************************************
 *
 * Function Name: mds_fetch_qa
 *
 * Purpose:
 *
 * Return Value:  NODE_ID
 *
 ****************************************************************************/

MDS_DEST mds_fetch_qa()
{
	/* phani : todo */
	return m_MDS_GET_ADEST;
}

/****************************************************************************
 *
 * Function Name: ncs_get_vdest_id_from_mds_dest
 *
 * Purpose:
 *
 * Return Value:  NODE_ID
 *
 ****************************************************************************/
MDS_VDEST_ID ncs_get_vdest_id_from_mds_dest(MDS_DEST mdsdest)
{
	return (MDS_VDEST_ID)(m_NCS_NODE_ID_FROM_MDS_DEST(mdsdest) == 0 ? mdsdest : 0);
}

/****************************************************************************
 *
 * Function Name: mds_alloc_direct_buff
 *
 * Purpose:
 *
 * Return Value:  MDS_DIRECT_BUFF 
 *
 ****************************************************************************/
#define m_MMGR_ALLOC_DIRECT_BUFF(size)  m_NCS_MEM_ALLOC(size,\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_DIRECT_BUFF_AL)

#define m_MMGR_FREE_DIRECT_BUFF(p)  m_NCS_MEM_FREE(p,\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_DIRECT_BUFF_AL)
MDS_DIRECT_BUFF mds_alloc_direct_buff(uint16_t size)
{
	if (size > MDS_DIRECT_BUF_MAXSIZE) {
		m_MDS_LOG_ERR
		    ("MDS_PAPI : Requested Memory allocation for direct buff is greater than the Max Direct buff send size\n");
		return NULL;
	} else
		return (MDS_DIRECT_BUFF)(m_MMGR_ALLOC_DIRECT_BUFF(size));
}

/****************************************************************************
 *
 * Function Name: mds_free_direct_buff
 *
 * Purpose:
 *
 * Return Value:  None
 *
 ****************************************************************************/
void mds_free_direct_buff(MDS_DIRECT_BUFF buff)
{
	if (buff != NULL) {
		m_MMGR_FREE_DIRECT_BUFF(buff);
		buff = NULL;	/* Safer check */
	} else {
		m_MDS_LOG_ERR("MDS_PAPI : Trying to free the direct buffer with pointer as NULL\n");
	}
}
