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

#include <inttypes.h>

#include "dts.h"
#include "dts_imm.h"

/************************************************************************

  FUNCTIONS INCLUDED in this module:
  dts_log_policy_change
  dts_filter_policy_change
  dts_log_device_set
  dts_buff_size_set

  dts_service_log_policy_set  - Service Policy IMMSv set operation.
  dtsv_svc_filtering_policy_change     - Service filtering policy change.
  dts_manage_service_objects
  dts_handle_service_param_set

  dts_global_log_policy_set                      - Global policy set operation.
  dtsv_global_filtering_policy_change     - Global filter policy change.
  dts_log_policy_change                 - Handle Log policy change.

  dts_node_log_policy_set      - Node Policy IMMSv set operation.
  dtsv_node_policy_change               - Node policy change.
  dts_log_policy_change                 - Handle Log policy change.

******************************************************************************/

/* Local routines */
static unsigned int dtsv_node_policy_change(DTS_CB *inst, DTS_SVC_REG_TBL *node, unsigned int param_id, unsigned int node_id);

static void dts_manage_node_objects(DTS_CB *inst, DTS_SVC_REG_TBL *node, enum CcbUtilOperationType opType);

static unsigned int dts_handle_node_param_set(DTS_CB *inst, DTS_SVC_REG_TBL *node,
				       unsigned int paramid, uns8 old_log_device, unsigned int old_buff_size);

static unsigned int dtsv_node_policy_change(DTS_CB *inst, DTS_SVC_REG_TBL *node, unsigned int param_id, unsigned int node_id);

static unsigned int dts_manage_service_objects(DTS_CB *inst, DTS_SVC_REG_TBL *node, enum CcbUtilOperationType opType);

static unsigned int dts_handle_service_param_set(DTS_CB *inst, DTS_SVC_REG_TBL *node, unsigned int paramid,
					  uns8 old_log_device, unsigned int old_buff_size);

/**************************************************************************
 Function: dts_log_policy_change

 Purpose:  Function used for handeling the policy changes. 

 Returns:  None.

 Notes:  
**************************************************************************/
void dts_log_policy_change(DTS_SVC_REG_TBL *node, POLICY *old_plcy, POLICY *new_plcy)
{
	if (old_plcy->buff_log_fmt != new_plcy->buff_log_fmt)
		node->device.buff_log_fmt_change = TRUE;

	if (old_plcy->file_log_fmt != new_plcy->file_log_fmt)
		node->device.file_log_fmt_change = TRUE;

	if (old_plcy->log_dev != new_plcy->log_dev) {
		dts_log_device_set(new_plcy, &node->device, old_plcy->log_dev);
	}

	if (old_plcy->cir_buff_size != new_plcy->cir_buff_size) {
		dts_buff_size_set(new_plcy, &node->device, old_plcy->cir_buff_size);
	}
}

/**************************************************************************
 Function: dts_filter_policy_change

 Purpose:  Function used for handling the filtering policy changes. 

 Returns:  None

 Notes:  
**************************************************************************/
void dts_filter_policy_change(DTS_SVC_REG_TBL *node, POLICY *old_plcy, POLICY *new_plcy)
{
	DTS_CB *inst = &dts_cb;
	SVC_KEY nt_key;
	DTS_SVC_REG_TBL *service;

	node->svc_policy.enable = new_plcy->enable;
	node->svc_policy.category_bit_map = new_plcy->category_bit_map;
	node->svc_policy.severity_bit_map = new_plcy->severity_bit_map;
	/* Node filter policy change */
	if (node->my_key.ss_svc_id == 0) {
		nt_key.node = node->ntwk_key.node;
		nt_key.ss_svc_id = 0;

		while (((service = (DTS_SVC_REG_TBL *)dts_get_next_svc_entry(&inst->svc_tbl, &nt_key)) != NULL)
		       && (service->my_key.node == node->my_key.node)) {
			nt_key.ss_svc_id = service->ntwk_key.ss_svc_id;
			service->svc_policy.enable = new_plcy->enable;
			service->svc_policy.category_bit_map = new_plcy->category_bit_map;
			service->svc_policy.severity_bit_map = new_plcy->severity_bit_map;

			dtsv_svc_filtering_policy_change(inst, service, 0, 0, 0);
		}
	}
	/* Service filter change */
	else if (node->dta_count != 0) {
		dtsv_svc_filtering_policy_change(inst, node, 0, 0, 0);
	}

	return;
}

/**************************************************************************
 Function: dts_log_device_set

 Purpose:  Function used for handeling the log device policy changes. 

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.

 Notes:  
**************************************************************************/
unsigned int dts_log_device_set(POLICY *policy, OP_DEVICE *device, uns8 old_value)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	/* 
	 * Check whether our log device has changed to circular buffer.
	 * If yes then allocate the buffer, if no then free the buffer.
	 */
	if ((policy->log_dev & CIRCULAR_BUFFER) && (!(old_value & CIRCULAR_BUFFER))) {
		rc = dts_circular_buffer_alloc(&device->cir_buffer, policy->cir_buff_size);
	} else if ((old_value & CIRCULAR_BUFFER) && (!(policy->log_dev & CIRCULAR_BUFFER))) {
		rc = dts_circular_buffer_free(&device->cir_buffer);
	}

	/* 
	 * Check whether our log device has changed to Log File.
	 * If yes then set the new log file to true.
	 */

	if ((policy->log_dev & LOG_FILE) && (!(old_value & LOG_FILE))) {
		device->new_file = TRUE;

		/*if (TRUE == device->file_open)
		   {
		   fclose(device->svc_fh);
		   device->file_open = FALSE;
		   } */
	} else if ((!(policy->log_dev & LOG_FILE)) && (old_value & LOG_FILE)) {
		device->new_file = TRUE;
		/*if (TRUE == device->file_open)
		   {
		   fclose(device->svc_fh);
		   device->file_open = FALSE;
		   } */
	}

	return rc;
}

/**************************************************************************
 Function: dts_buff_size_set

 Purpose:  Function used for handeling the changes in the buffer size. 

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.

 Notes:  
**************************************************************************/
unsigned int dts_buff_size_set(POLICY *policy, OP_DEVICE *device, unsigned int old_value)
{
	/* Check whether log device is already set to buffer or not.
	 *            If not then return success, no need to call the buff_size 
	 *            increase/decrease functions.
	 */
	if (device->cir_buffer.buff_allocated == FALSE)
		return NCSCC_RC_SUCCESS;
	else {
		if (old_value < policy->cir_buff_size) {
			if (dts_buff_size_increased(&device->cir_buffer, policy->cir_buff_size) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;
		} else {
			if (dts_buff_size_decreased(&device->cir_buffer, policy->cir_buff_size) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;
		}
	}
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 Function: dts_service_log_policy_set 

 Purpose:  Service Policy IMMSv's set routine

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
unsigned int dts_service_log_policy_set(DTS_CB *inst, char *objName, void *attrib_info, enum CcbUtilOperationType UtilOp)
{
	DTS_SVC_REG_TBL *service;
	SVC_KEY key, nt_key;
	SaImmAttrModificationT_2 *attrMod, **attrMods = (SaImmAttrModificationT_2 **)attrib_info;
	SaImmAttrValuesT_2 *attribute = NULL, **attributes = (SaImmAttrValuesT_2 **)attrib_info;
	unsigned int paramid = 0, old_buff_size = 0, rc = NCSCC_RC_SUCCESS;
	uns8 old_log_device = 0;
	int i = 0;

	memset(&key, 0, sizeof(SVC_KEY));
	memset(&nt_key, 0, sizeof(SVC_KEY));

	if (dts_parse_service_policy_DN(objName, &key) == NCSCC_RC_FAILURE) {
		dts_log(NCSFL_SEV_ERROR, "Cannot proceed: Invalid DN format\n");
		return NCSCC_RC_FAILURE;
	}

	/*  Network order key added */
	nt_key.node = m_NCS_OS_HTONL(key.node);
	nt_key.ss_svc_id = m_NCS_OS_HTONL(key.ss_svc_id);

	service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key);

	if (!service) {
		rc = dts_create_new_pat_entry(inst, &service, key.node, key.ss_svc_id, FALSE);
		if (rc != NCSCC_RC_SUCCESS) {
			/*log ERROR */
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_ERROR,
				   "TILLL", DTS_EV_SVC_REG_ENT_ADD_FAIL, key.ss_svc_id, key.node, 0);
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_ERROR,
				   "TILLL", DTS_SET_FAIL, key.ss_svc_id, key.node, 0);
			return rc;
		}
		/* Smik - Send Async add update to stby */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_ADD, (MBCSV_REO_HDL)(long)service,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
	}

	if (UtilOp == CCBUTIL_DELETE) {
		/* Do processing specific to the param objects after set */
		if (NCSCC_RC_SUCCESS != dts_manage_service_objects(inst, service, CCBUTIL_DELETE)) {
			dts_log(NCSFL_SEV_ERROR, "instance of object being delete failed");
			return NCSCC_RC_FAILURE;
		}

		return NCSCC_RC_SUCCESS;
	}

	if (UtilOp == CCBUTIL_CREATE)
		attribute = attributes[i];
	else if (UtilOp == CCBUTIL_MODIFY) {
		attrMod = attrMods[i];
		attribute = &attrMod->modAttr;
	}
	while (attribute) {
		uintptr_t value;

		if (attribute->attrValuesNumber == 0) {
			return SA_AIS_ERR_BAD_OPERATION;
		}

		value = *((uintptr_t *)attribute->attrValues[0]);

		if (!strcmp(attribute->attrName, "osafDtsvServiceLogDevice")) {
			paramid = osafDtsvServiceLogDevice_ID;
			old_log_device = service->svc_policy.log_dev;
			service->svc_policy.log_dev = *(uns8 *)&value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLogDevice = %d\n", service->svc_policy.log_dev);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceLogFileSize")) {
			if ((value < 100) || (value > 10000)) {
				LOG_ER("Invalid osafDtsvServiceLogFileSize : %d , exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceLogFileSize_ID;
			service->svc_policy.log_file_size = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLogFileSize = %d\n", service->svc_policy.log_file_size);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceFileLogCompFormat")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvServiceFileLogCompFormat :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceFileLogCompFormat_ID;
			service->svc_policy.file_log_fmt = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceFileLogCompFormat = %d\n", service->svc_policy.file_log_fmt);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceCircularBuffSize")) {
			if ((value < 10) || (value > 1000)) {
				LOG_ER("Invalid osafDtsvServiceCircularBuffSize :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceCircularBuffSize_ID;
			old_buff_size = service->svc_policy.cir_buff_size;
			service->svc_policy.cir_buff_size = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceCircularBuffSize = %d\n", service->svc_policy.cir_buff_size);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceCirBuffCompFormat")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvServiceCirBuffCompFormat :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceCirBuffCompFormat_ID;
			service->svc_policy.buff_log_fmt = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceCirBuffCompFormat =  %d\n", service->svc_policy.buff_log_fmt);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceLoggingState")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvServiceLoggingState :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceLoggingState_ID;
			service->svc_policy.enable = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceLoggingState = %d\n", service->svc_policy.enable);
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceCategoryBitMap")) {
			paramid = osafDtsvServiceCategoryBitMap_ID;
			service->svc_policy.category_bit_map = *(unsigned int *)&value;
		} else if (!strcmp(attribute->attrName, "osafDtsvServiceSeverityBitMap")) {
			if ((value < 1) || (value > 255)) {
				LOG_ER("Invalid osafDtsvServiceSeverityBitMap :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvServiceSeverityBitMap_ID;
			service->svc_policy.severity_bit_map = *(uns8 *)&value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvServiceSeverityBitMap = %d\n", service->svc_policy.severity_bit_map);
		} else if (!strcmp(attribute->attrName, "opensafServiceLogPolicy")) {
			dts_log(NCSFL_SEV_DEBUG, "RDN = %s\n", (char *)value);
		} else {
			dts_log(NCSFL_SEV_ERROR, "invalid attribute %s\n", attribute->attrName);
			return SA_AIS_ERR_BAD_OPERATION;
		}

		/* Do processing specific to the param objects after set */
		if (UtilOp == CCBUTIL_MODIFY) {
			if (NCSCC_RC_SUCCESS != dts_handle_service_param_set(inst, service,
									     paramid, old_log_device, old_buff_size))
				return NCSCC_RC_FAILURE;
		}

		if (UtilOp == CCBUTIL_CREATE)
			attribute = attributes[++i];
		else if (UtilOp == CCBUTIL_MODIFY) {
			attrMod = attrMods[++i];
			if (attrMod)
				attribute = &attrMod->modAttr;
			else
				attribute = NULL;
		}

	}
	if (UtilOp == CCBUTIL_CREATE) {
		/* Do processing specific to the param objects after set */
		if (NCSCC_RC_SUCCESS != dts_manage_service_objects(inst, service, CCBUTIL_CREATE))
			return NCSCC_RC_FAILURE;

		if ((rc != NCSCC_RC_FAILURE) && (service != NULL)) {
			m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)service,
						    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
		}
	}
	return rc;
}

/**************************************************************************
 Function: dtsv_svc_filtering_policy_change

 Purpose:  Change in the Node Filtering policy will affect the service.
         Also, policy change will be sent to all the services.

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
unsigned int dtsv_svc_filtering_policy_change(DTS_CB *inst, DTS_SVC_REG_TBL *service,
				       unsigned int param_id, unsigned int node_id, SS_SVC_ID svc_id)
{
	DTA_DEST_LIST *dta;
	DTA_ENTRY *dta_entry;
	/* Configure services for this change */
	dta_entry = service->v_cd_list;
	while (dta_entry != NULL) {
		dta = dta_entry->dta;
		dts_send_filter_config_msg(inst, service, dta);
		dta_entry = dta_entry->next_in_svc_entry;
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 Function: dts_handle_service_param_set

 Purpose:  Function used for handeling node parameter changes. 

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.

 Notes:  
**************************************************************************/
static unsigned int dts_handle_service_param_set(DTS_CB *inst, DTS_SVC_REG_TBL *service,
					  unsigned int paramid, uns8 old_log_device, unsigned int old_buff_size)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	switch (paramid) {
	case osafDtsvServiceLogDevice_ID:
		/* Handle log device change */
		rc = dts_log_device_set(&service->svc_policy, &service->device, old_log_device);
		break;

	case osafDtsvServiceLogFileSize_ID:
		break;

	case osafDtsvServiceFileLogCompFormat_ID:
		service->device.file_log_fmt_change = TRUE;	/*TBD */
		break;

	case osafDtsvServiceCircularBuffSize_ID:
		/* Handle Circular buffer size change */
		rc = dts_buff_size_set(&service->svc_policy, &service->device, old_buff_size);
		break;

	case osafDtsvServiceCirBuffCompFormat_ID:
		service->device.buff_log_fmt_change = TRUE;	/*TBD */
		break;

	case osafDtsvServiceCategoryBitMap_ID:
		service->svc_policy.category_bit_map = ntohl(service->svc_policy.category_bit_map);
		/* Filtering policy is changed. So send the message to all 
		 * DTA's and tell them the correct filtering policies */
		rc = dtsv_svc_filtering_policy_change(inst, service,
						      paramid, service->my_key.node, service->my_key.ss_svc_id);
		break;

	case osafDtsvServiceLoggingState_ID:
	case osafDtsvServiceSeverityBitMap_ID:
		/* Filtering policy is changed. So send the message to all 
		 * DTA's and tell them the correct filtering policies */
		rc = dtsv_svc_filtering_policy_change(inst, service,
						      paramid, service->my_key.node, service->my_key.ss_svc_id);
		break;

	default:
		dts_log(NCSFL_SEV_ERROR, "Invalid param Id\n");
		break;
	}

	/* Smik - Send Async update for DTS_SVC_REG_TBL to stby DTS */
	if ((rc != NCSCC_RC_FAILURE) && (service != NULL)) {
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)service,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
	}

	/* Re-set cli_bit_map */
	inst->cli_bit_map = 0;

	return rc;
}

/**************************************************************************
 Function: dts_manage_service_objects

 Purpose:  Function used for handeling the IMM actions. 
        
 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
                
 Notes:         
**************************************************************************/
static unsigned int dts_manage_service_objects(DTS_CB *inst, DTS_SVC_REG_TBL *service, enum CcbUtilOperationType opType)
{
	OP_DEVICE *dev = &service->device;
	/*
	 * If Ccb operations is Delete, we will have to remove 
	 * patritia tree entry provided none of the service registered 
	 * with that service and node id.
	 */
	if (opType == CCBUTIL_DELETE) {
		dts_log_policy_change(service, &service->svc_policy, &inst->dflt_plcy.svc_dflt.policy);
		/* Changing the filtering policy as well. don't return
		 *            Print a DBG SINK. 
		 */
		dts_filter_policy_change(service, &service->svc_policy, &inst->dflt_plcy.svc_dflt.policy);

		if (service->dta_count == 0) {
			/* No need of policy handles */
			/*ncshm_destroy_hdl(NCS_SERVICE_ID_DTSV, service->svc_hdl); */
			dts_circular_buffer_free(&dev->cir_buffer);
			ncs_patricia_tree_del(&inst->svc_tbl, (NCS_PATRICIA_NODE *)service);
			/* Cleanup the DTS_FILE_LIST datastructure for svc */
			m_DTS_FREE_FILE_LIST(dev);
			/* Cleanup the console devices associated with the node */
			m_DTS_RMV_ALL_CONS(dev);
			/* Send RMV updt here itself, cuz service is going to be deleted */
			m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_RMV, (MBCSV_REO_HDL)(long)service,
						    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);

			if (NULL != service)
				m_MMGR_FREE_SVC_REG_TBL(service);
			/* Set service to NULL, since service would be used later in
			 * dts_handle_service_param_set() 
			 */
			service = NULL;
		}
	} else if (opType == CCBUTIL_CREATE) {
		dts_log_policy_change(service, &inst->dflt_plcy.svc_dflt.policy, &service->svc_policy);
		dts_filter_policy_change(service, &inst->dflt_plcy.svc_dflt.policy, &service->svc_policy);
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 Function: dts_global_log_policy_set

 Purpose:  Global Policy table's set object routine

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
unsigned int dts_global_log_policy_set(DTS_CB *inst, struct CcbUtilOperationData *ccbUtilOperationData)
{
	unsigned int rc = NCSCC_RC_SUCCESS, old_buff_size = 0;
	uns8 old_log_device = 0;
	unsigned int old_num_log_files = 0, i = 0;
	DTS_SVC_REG_TBL *node;
	OP_DEVICE *device;
	DTS_LOG_CKPT_DATA data;
	SaImmAttrModificationT_2 *attrMod;
	SaImmAttrValuesT_2 *attribute = NULL;

	strcpy(data.file_name, "");

	attrMod = (SaImmAttrModificationT_2 *)ccbUtilOperationData->param.modify.attrMods[i++];
	if (!attrMod) {
		dts_log(NCSFL_SEV_ERROR, "Invalid ccbUtilOperationData value");
		return NCSCC_RC_FAILURE;
	}

	while (attrMod) {
		SaUint32T value;
		attribute = &attrMod->modAttr;

		if (attribute->attrValuesNumber == 0) {
			return SA_AIS_ERR_BAD_OPERATION;
		}

		value = *(SaUint32T *)attribute->attrValues[0];
		if (!strcmp(attribute->attrName, "osafDtsvGlobalMessageLogging")) {
			if (inst->g_policy.global_logging != value) {
				inst->g_policy.device.new_file = TRUE;

				dts_circular_buffer_clear(&inst->g_policy.device.cir_buffer);
				m_LOG_DTS_CBOP(DTS_CBOP_CLEARED, 0, 0);
				inst->g_policy.global_logging = value;
				/* Update the cli_bit_map field in DTS_CB */
				inst->cli_bit_map = osafDtsvGlobalMessageLogging_ID;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalMessageLogging_ID))
					return NCSCC_RC_FAILURE;
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogDevice")) {
			if (inst->g_policy.g_policy.log_dev != *(uns8 *)&value) {
				old_log_device = inst->g_policy.g_policy.log_dev;
				inst->g_policy.g_policy.log_dev = *(uns8 *)&value;

				rc = dts_log_device_set(&inst->g_policy.g_policy, &inst->g_policy.device,
							old_log_device);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogFileSize")) {
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogFileSize %d\n", value);
			inst->g_policy.g_policy.log_file_size = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalFileLogCompFormat")) {
			inst->g_policy.device.file_log_fmt_change = TRUE;
			inst->g_policy.g_policy.file_log_fmt = value;
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCircularBuffSize")) {
			if (inst->g_policy.g_policy.cir_buff_size != value) {
				old_buff_size = inst->g_policy.g_policy.cir_buff_size;
				inst->g_policy.g_policy.cir_buff_size = value;
				rc = dts_buff_size_set(&inst->g_policy.g_policy, &inst->g_policy.device, old_buff_size);
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCircularBuffSize %d\n", value);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCirBuffCompFormat")) {
			inst->g_policy.device.buff_log_fmt_change = TRUE;
			inst->g_policy.g_policy.buff_log_fmt = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCirBuffCompFormat %d\n", value);
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLoggingState")) {
			if (inst->g_policy.g_policy.enable != value) {
				inst->cli_bit_map = osafDtsvGlobalLoggingState_ID;
				inst->g_policy.g_policy.enable = value;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalLoggingState_ID))
					return NCSCC_RC_FAILURE;
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLoggingState %d\n", value);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCategoryBitMap")) {
			if (inst->g_policy.g_policy.category_bit_map != value) {
				inst->g_policy.g_policy.category_bit_map = value;
				inst->g_policy.g_policy.category_bit_map =
				    ntohl(inst->g_policy.g_policy.category_bit_map);
				/* Smik - update the cli_bit_map field in DTS_CB */
				inst->cli_bit_map = osafDtsvGlobalCategoryBitMap_ID;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalCategoryBitMap_ID))
					return NCSCC_RC_FAILURE;
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCategoryBitMap %d\n", value);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalSeverityBitMap")) {
			if (inst->g_policy.g_policy.severity_bit_map != *(uns8 *)&value) {
				inst->g_policy.g_policy.severity_bit_map = *(uns8 *)&value;
				/* Smik - update the cli_bit_map field in DTS_CB */
				inst->cli_bit_map = osafDtsvGlobalSeverityBitMap_ID;
				if (NCSCC_RC_SUCCESS !=
				    dtsv_global_filtering_policy_change(inst, osafDtsvGlobalSeverityBitMap_ID))
					return NCSCC_RC_FAILURE;
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalSeverityBitMap %d\n", value);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalNumOfLogFiles")) {
			{
				old_num_log_files = inst->g_policy.g_num_log_files;
				inst->g_policy.g_num_log_files = value;
				if (old_num_log_files < inst->g_policy.g_num_log_files)
					continue;
				/* Remove log files for global level logging */
				device = &inst->g_policy.device;
				while (m_DTS_NUM_LOG_FILES(device) > inst->g_policy.g_num_log_files) {
					m_DTS_RMV_FILE(device);
					data.key.node = 0;
					data.key.ss_svc_id = 0;
					m_DTSV_SEND_CKPT_UPDT_ASYNC(&dts_cb, NCS_MBCSV_ACT_RMV,
								    NCS_PTR_TO_UNS64_CAST(&data),
								    DTSV_CKPT_DTS_LOG_FILE_CONFIG);
				}

				/* Remove log files for node & service level logging */
				node = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, NULL);
				while (node != NULL) {
					device = &node->device;
					while (m_DTS_NUM_LOG_FILES(device) > inst->g_policy.g_num_log_files) {
						m_DTS_RMV_FILE(device);
						data.key = node->my_key;
						m_DTSV_SEND_CKPT_UPDT_ASYNC(&dts_cb, NCS_MBCSV_ACT_RMV, (long)&data,
									    DTSV_CKPT_DTS_LOG_FILE_CONFIG);
					}

					node = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl,
											    (const uns8 *)
											    &node->ntwk_key);
				}
			}
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalNumOfLogFiles %d\n", value);
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalLogMsgSequencing")) {
			if (inst->g_policy.g_enable_seq != value) {
				inst->g_policy.g_enable_seq = value;
				if (inst->g_policy.g_enable_seq == TRUE)
					dts_enable_sequencing(inst);
				else
					dts_disable_sequencing(inst);
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalLogMsgSequencing %d\n", value);
			}
		} else if (!strcmp(attribute->attrName, "osafDtsvGlobalCloseOpenFiles")) {
			if (inst->g_policy.g_close_files != value) {
				inst->g_policy.g_close_files = value;
				dts_close_opened_files();
				dts_log(NCSFL_SEV_DEBUG, "osafDtsvGlobalCloseOpenFiles %d\n", value);
			}
		} else {
			return SA_AIS_ERR_BAD_OPERATION;
		}

		attrMod = (SaImmAttrModificationT_2 *)ccbUtilOperationData->param.modify.attrMods[i++];
	}

	if (rc != NCSCC_RC_FAILURE) {
		/* Smik - Send Async update to stby DTS */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (long)&inst->g_policy,
					    DTSV_CKPT_GLOBAL_POLICY_CONFIG);
	}

	/* Re-set cli_bit_map */
	inst->cli_bit_map = 0;

	return rc;
}

/**************************************************************************
 Function: dtsv_global_filtering_policy_change

 Purpose:  Change in the Global Filtering policy will affect the entire system
           This function will set the filtering policies of all the nodes and 
           all the services which are currently present in the node and the 
           service registration table respectively. Also, policy change will
           be sent to all the services.

 Input:    cb       : DTS_CB pointer
           param_id : Parameter for which change is done.

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
unsigned int dtsv_global_filtering_policy_change(DTS_CB *inst, unsigned int param_id)
{
	SVC_KEY nt_key;
	DTS_SVC_REG_TBL *service;
	DTA_DEST_LIST *dta;
	DTA_ENTRY *dta_entry;

	/* Search through registration table, Set all the policies,
	 * configure all the DTA's using this policy. 
	 */
	service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, NULL);
	while (service != NULL) {
		/* Setup key for new search */
		/*  Network order key added */
		nt_key = service->ntwk_key;
		/* Set the Node Policy as per the Global Policy */
		if (param_id == osafDtsvGlobalLoggingState_ID)
			service->svc_policy.enable = inst->g_policy.g_policy.enable;
		else if (param_id == osafDtsvGlobalCategoryBitMap_ID)
			service->svc_policy.category_bit_map = inst->g_policy.g_policy.category_bit_map;
		else if (param_id == osafDtsvGlobalSeverityBitMap_ID)
			service->svc_policy.severity_bit_map = inst->g_policy.g_policy.severity_bit_map;
		else if (param_id == osafDtsvGlobalMessageLogging_ID) {
			service->device.new_file = TRUE;
			dts_circular_buffer_clear(&service->device.cir_buffer);
			m_LOG_DTS_CBOP(DTS_CBOP_CLEARED, service->my_key.ss_svc_id, service->my_key.node);
		} else
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dtsv_global_filtering_policy_change: Wrong param id passed to this function. Need to debug flow.");

		/* Configure services for this change */
		if ((service->my_key.ss_svc_id != 0) && (param_id != osafDtsvGlobalMessageLogging_ID)) {
			dta_entry = service->v_cd_list;
			while (dta_entry != NULL) {
				dta = dta_entry->dta;
				dts_send_filter_config_msg(inst, service, dta);
				dta_entry = dta_entry->next_in_svc_entry;
			}
		}

		service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, (const uns8 *)&nt_key);
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 Function: dts_node_log_policy_set 

 Purpose:  Node Policy IMMSv's set routine

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
unsigned int dts_node_log_policy_set(DTS_CB *inst, char *objName, void *attrib_info, enum CcbUtilOperationType UtilOp)
{
	DTS_SVC_REG_TBL *node;
	unsigned int paramid = 0, old_buff_size = 0, rc = NCSCC_RC_SUCCESS;
	uns8 log_device = 0;
	SVC_KEY key, nt_key;
	SaImmAttrModificationT_2 *attrMod, **attrMods = (SaImmAttrModificationT_2 **)attrib_info;
	SaImmAttrValuesT_2 *attribute = NULL, **attributes = (SaImmAttrValuesT_2 **)attrib_info;
	unsigned int i = 0;

	if (dts_parse_node_policy_DN(objName, &key) == NCSCC_RC_FAILURE) {
		dts_log(NCSFL_SEV_ERROR, "Cannot proceed: Invalid DN format");
		return NCSCC_RC_FAILURE;
	}

	/*  Network order key added */
	nt_key.node = m_NCS_OS_HTONL(key.node);
	nt_key.ss_svc_id = 0;

	node = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key);

	if (!node) {
		rc = dts_create_new_pat_entry(inst, &node, key.node, key.ss_svc_id, NODE_LOGGING);
		if (rc != NCSCC_RC_SUCCESS) {
			/*log ERROR */
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_ERROR,
				   "TILLL", DTS_EV_SVC_REG_ENT_ADD_FAIL, key.ss_svc_id, key.node, 0);
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_ERROR,
				   "TILLL", DTS_SET_FAIL, key.ss_svc_id, key.node, 0);
			return rc;
		}
		/* Send Async add to stby DTS */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_ADD, (MBCSV_REO_HDL)(long)node,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
	}

	if (UtilOp == CCBUTIL_DELETE) {
		dts_manage_node_objects(inst, node, CCBUTIL_DELETE);
		return NCSCC_RC_SUCCESS;
	} else if (UtilOp == CCBUTIL_CREATE)
		attribute = attributes[i];
	else if (UtilOp == CCBUTIL_MODIFY) {
		attrMod = attrMods[i];
		attribute = &attrMod->modAttr;
	}
	while (attribute) {
		uintptr_t value;

		if (attribute->attrValuesNumber == 0) {
			return SA_AIS_ERR_BAD_OPERATION;
		}

		value = *((uintptr_t *)attribute->attrValues[0]);

		if (!strcmp(attribute->attrName, "osafDtsvNodeMessageLogging")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvNodeMessageLogging :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeMessageLogging_ID;
			node->per_node_logging = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeMessageLogging = %d\n", node->per_node_logging);
		} else if (!strcmp(attribute->attrName, "opensafNodeLogPolicy")) {
			dts_log(NCSFL_SEV_DEBUG, "RDN = %s\n", (char *)value);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeLogDevice")) {
			paramid = osafDtsvNodeLogDevice_ID;
			log_device = node->svc_policy.log_dev;
			node->svc_policy.log_dev = *(uns8 *)&value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLogDevice = %d\n", node->svc_policy.log_dev);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeLogFileSize")) {
			if ((value < 100) || (value > 100000)) {
				LOG_ER("Invalid osafDtsvNodeLogFileSize :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeLogFileSize_ID;
			node->svc_policy.log_file_size = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLogFileSize = %d\n", node->svc_policy.log_file_size);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeFileLogCompFormat")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvNodeFileLogCompFormat :%d exiting",value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeFileLogCompFormat_ID;
			node->svc_policy.file_log_fmt = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeFileLogCompFormat = %d\n", node->svc_policy.file_log_fmt);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeCircularBuffSize")) {
			if ((value < 10) || (value > 10000)) {
				LOG_ER("Invalid osafDtsvNodeCircularBuffSize :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeCircularBuffSize_ID;
			old_buff_size = node->svc_policy.cir_buff_size;
			node->svc_policy.cir_buff_size = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeCircularBuffSize = %d\n", node->svc_policy.cir_buff_size);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeCirBuffCompFormat")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvNodeCirBuffCompFormat :%d exiting", value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeCirBuffCompFormat_ID;
			node->svc_policy.buff_log_fmt = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeCirBuffCompFormat =%d\n", node->svc_policy.buff_log_fmt);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeLoggingState")) {
			if ((value < 0) || (value > 1)) {
				LOG_ER("Invalid osafDtsvNodeLoggingState :%d exiting",value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeLoggingState_ID;
			node->svc_policy.enable = value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeLoggingState = %d\n", node->svc_policy.enable);
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeCategoryBitMap")) {
			paramid = osafDtsvNodeCategoryBitMap_ID;
			node->svc_policy.category_bit_map = *(unsigned int *)&value;
		} else if (!strcmp(attribute->attrName, "osafDtsvNodeSeverityBitMap")) {
			if ((value < 1) || (value > 255)) {
				LOG_ER("Invalid osafDtsvNodeSeverityBitMap :%d exiting",value);
				exit(EXIT_FAILURE);
			}
			paramid = osafDtsvNodeSeverityBitMap_ID;
			node->svc_policy.severity_bit_map = *(uns8 *)&value;
			dts_log(NCSFL_SEV_DEBUG, "osafDtsvNodeSeverityBitMap = %d\n", node->svc_policy.severity_bit_map);
		} else {
			LOG_ER("Invalid attribute value");
			exit(EXIT_FAILURE);
		}

		/* Do processing specific to the param objects after set */
		if (UtilOp == CCBUTIL_MODIFY) {
			if (NCSCC_RC_SUCCESS != dts_handle_node_param_set(inst, node,
									  paramid, log_device, old_buff_size))
				return NCSCC_RC_FAILURE;
		}

		if (UtilOp == CCBUTIL_CREATE)
			attribute = attributes[++i];
		else if (UtilOp == CCBUTIL_MODIFY) {
			attrMod = attrMods[++i];
			if (attrMod)
				attribute = &attrMod->modAttr;
			else
				attribute = NULL;
		}

	}			/* end of while(attrMod) */
	if (UtilOp == CCBUTIL_CREATE) {
		/* Do processing specific to the param objects after set */
		dts_manage_node_objects(inst, node, CCBUTIL_CREATE);

		/* Smik - Send Async update for DTS_SVC_REG_TBL to stby DTS */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)node,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
		return NCSCC_RC_SUCCESS;
	}
	return rc;
}

/**************************************************************************
 Function: dtsv_node_policy_change

 Purpose:  Change in the Node Filtering policy will affect the entire node.
           This function will set the filtering policies of all the  services 
           which are currently present in the service registration table. 
           Also, policy change will be sent to all the services.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
static unsigned int dtsv_node_policy_change(DTS_CB *inst, DTS_SVC_REG_TBL *node, unsigned int param_id, unsigned int node_id)
{
	SVC_KEY nt_key;
	DTS_SVC_REG_TBL *service;
	DTA_DEST_LIST *dta;
	DTA_ENTRY *dta_entry;

	/* Search through Service per node registration table, Set all the policies,
	 * configure all the DTA's using this policy 
	 */
	/*  Network order key added */
	nt_key.node = m_NCS_OS_HTONL(node_id);
	nt_key.ss_svc_id = 0;

	while (((service = (DTS_SVC_REG_TBL *)dts_get_next_svc_entry(&inst->svc_tbl, &nt_key)) != NULL) &&
	       (service->my_key.node == node_id)) {
		/* Setup key for new search */
		nt_key.ss_svc_id = service->ntwk_key.ss_svc_id;

		/* Set the Node Policy as per the Node Policy */
		if (param_id == osafDtsvNodeLoggingState_ID)
			service->svc_policy.enable = node->svc_policy.enable;
		else if (param_id == osafDtsvNodeCategoryBitMap_ID)
			service->svc_policy.category_bit_map = node->svc_policy.category_bit_map;
		else if (param_id == osafDtsvNodeSeverityBitMap_ID)
			service->svc_policy.severity_bit_map = node->svc_policy.severity_bit_map;
		else if (param_id == osafDtsvNodeMessageLogging_ID) {
			if (node->per_node_logging == FALSE)
				service->device.new_file = TRUE;

			dts_circular_buffer_clear(&service->device.cir_buffer);
			m_LOG_DTS_CBOP(DTS_CBOP_CLEARED, service->my_key.ss_svc_id, service->my_key.node);
		} else
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dtsv_node_policy_change: Wrong param ID passed to the function dtsv_node_policy_change");

		/* Configure services for this change */
		dta_entry = service->v_cd_list;

		while (dta_entry != NULL) {
			dta = dta_entry->dta;
			dts_send_filter_config_msg(inst, service, dta);
			dta_entry = dta_entry->next_in_svc_entry;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 Function: dts_handle_node_param_set

 Purpose:  Function used for handeling node parameter changes. 

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 Notes:  
**************************************************************************/
static unsigned int dts_handle_node_param_set(DTS_CB *inst, DTS_SVC_REG_TBL *node,
				       unsigned int paramid, uns8 old_log_device, unsigned int old_buff_size)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	switch (paramid) {
	case osafDtsvNodeLogDevice_ID:
		rc = dts_log_device_set(&node->svc_policy, &node->device, old_log_device);
		break;

	case osafDtsvNodeLogFileSize_ID:
		break;

	case osafDtsvNodeFileLogCompFormat_ID:
		node->device.file_log_fmt_change = TRUE;
		break;

	case osafDtsvNodeCircularBuffSize_ID:
		rc = dts_buff_size_set(&node->svc_policy, &node->device, old_buff_size);
		break;

	case osafDtsvNodeCirBuffCompFormat_ID:
		node->device.buff_log_fmt_change = TRUE;
		break;

	case osafDtsvNodeMessageLogging_ID:
		if (node->per_node_logging == TRUE)
			node->device.new_file = TRUE;

		/* Smik - Update the cli_bit_map field in DTS_CB */
		inst->cli_bit_map = paramid;
		dts_circular_buffer_clear(&node->device.cir_buffer);
		m_LOG_DTS_CBOP(DTS_CBOP_CLEARED, 0, node->my_key.node);
		rc = dtsv_node_policy_change(inst, node, paramid, node->my_key.node);
		break;

	case osafDtsvNodeCategoryBitMap_ID:
		inst->cli_bit_map = paramid;
		node->svc_policy.category_bit_map = ntohl(node->svc_policy.category_bit_map);
		rc = dtsv_node_policy_change(inst, node, paramid, node->my_key.node);
		break;

	case osafDtsvNodeLoggingState_ID:
	case osafDtsvNodeSeverityBitMap_ID:
		inst->cli_bit_map = paramid;
		rc = dtsv_node_policy_change(inst, node, paramid, node->my_key.node);
		break;

	default:
		break;
	}

	if ((rc != NCSCC_RC_FAILURE)) {
		/* Smik - Send Async update for DTS_SVC_REG_TBL to stby DTS */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)node,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);
	}

	/* Re-set cli_bit_map */
	inst->cli_bit_map = 0;

	return rc;
}

/**************************************************************************
 Function: dts_manage_node_objects

 Purpose:  Function used for handeling the IMM actions. 

 Returns:  None.

 Notes:  
**************************************************************************/
static void dts_manage_node_objects(DTS_CB *inst, DTS_SVC_REG_TBL *node, enum CcbUtilOperationType opType)
{
	OP_DEVICE *dev;

	if (opType == CCBUTIL_CREATE) {
		dts_log_policy_change(node, &inst->dflt_plcy.node_dflt.policy, &node->svc_policy);

		/* Change the filtering policy aswell */
		dts_filter_policy_change(node, &inst->dflt_plcy.node_dflt.policy, &node->svc_policy);

		/*
		 * Check if we need to change the logging policy. If yes then 
		 * new logging policy handle should be sent the DTA for logging.
		 */
		if (((node->per_node_logging == TRUE) &&
		     (inst->dflt_plcy.node_dflt.per_node_logging == FALSE)) ||
		    ((inst->dflt_plcy.node_dflt.per_node_logging == TRUE) && (node->per_node_logging == FALSE))) {
			dtsv_node_policy_change(inst, node, osafDtsvNodeMessageLogging_ID, node->my_key.node);
		}
	} else if (opType == CCBUTIL_DELETE) {
		/*
		 * First handle the changes in the logging policy due to CCB_DELETE .
		 */
		dts_log_policy_change(node, &node->svc_policy, &inst->dflt_plcy.node_dflt.policy);
		/* Changing the filtering policy as well. don't return
		 *            Print a DBG SINK. 
		 */
		dts_filter_policy_change(node, &node->svc_policy, &inst->dflt_plcy.node_dflt.policy);

		/*
		 * Check if we need to change the logging policy. If yes then 
		 * new logging policy handle should be sent the DTA for logging.
		 */
		if (((node->per_node_logging == TRUE) &&
		     (inst->dflt_plcy.node_dflt.per_node_logging == FALSE)) ||
		    ((inst->dflt_plcy.node_dflt.per_node_logging == TRUE) && (node->per_node_logging == FALSE))) {
			dtsv_node_policy_change(inst, node, osafDtsvNodeMessageLogging_ID, node->my_key.node);
		}

		/* With CCB_DELETE, memory for node should be
		 * freed up.
		 */
		dev = &node->device;
		dts_circular_buffer_free(&dev->cir_buffer);
		ncs_patricia_tree_del(&inst->svc_tbl, (NCS_PATRICIA_NODE *)node);
		/* Cleanup the DTS_FILE_LIST datastructure for node */
		m_DTS_FREE_FILE_LIST(dev);
		/* Cleanup the console devices associated with the node */
		m_DTS_RMV_ALL_CONS(dev);
		/* Send RMV updt here itself, cuz node is going to be deleted */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_RMV, (MBCSV_REO_HDL)(long)node,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);

		if (NULL != node)
			m_MMGR_FREE_SVC_REG_TBL(node);

		node = NULL;
	}
}
